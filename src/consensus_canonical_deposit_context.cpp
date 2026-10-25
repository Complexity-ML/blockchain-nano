#include "nano/consensus_canonical_deposit_context.hpp"
#include <rocksdb/db.h>
#include <rocksdb/write_batch.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <fcntl.h>
#include <limits>
#include <map>
#include <mutex>
#include <sys/stat.h>
#include <unistd.h>

namespace nano {
namespace {
using Checkpoint = std::function<void()>;
constexpr size_t maximum_node_bytes = 256;
constexpr size_t locator_bytes = 40;
constexpr size_t tuple_index_bytes = 10 + 2 * (34 + locator_bytes);
constexpr size_t metadata_bytes = 1024;

void require(bool ok, const char* message) {
 if (!ok) throw std::invalid_argument(message);
}
void checkpoint(const Checkpoint& check) { if (check) check(); }
bool valid_lookup_kind(CanonicalDepositLookup kind) {
 return kind == CanonicalDepositLookup::BeaconRoot ||
        kind == CanonicalDepositLookup::ExecutionHash;
}
void status(const rocksdb::Status& result) {
 if (!result.ok()) throw std::runtime_error("deposit context RocksDB: " + result.ToString());
}
uint64_t add(uint64_t a, uint64_t b) {
 if (b > UINT64_MAX - a) throw std::overflow_error("deposit context size overflow");
 return a + b;
}
ConsensusRoot digest(std::string_view text) {
 const auto bytes = sha256(Bytes(text.begin(), text.end()));
 ConsensusRoot out{};
 require(bytes.size() == out.size(), "deposit context digest width");
 std::copy(bytes.begin(), bytes.end(), out.begin());
 return out;
}
std::string raw(const ConsensusRoot& root) {
 return std::string(reinterpret_cast<const char*>(root.data()), root.size());
}
std::string root_hex(const ConsensusRoot& root) {
 return nano::hex(Bytes(root.begin(), root.end()));
}
ConsensusRoot root(const Json& value) {
 require(value.is_string(), "deposit context root type");
 const auto& text = value.get_ref<const std::string&>();
 require(text.size() == 66, "deposit context root size");
 const auto bytes = unhex(text);
 require(bytes.size() == 32 && hex(bytes) == text, "deposit context root encoding");
 ConsensusRoot out{};
 std::copy(bytes.begin(), bytes.end(), out.begin());
 return out;
}
uint64_t number(const Json& value) {
 require(value.is_string(), "deposit context number type");
 const auto n = u64(quantity(value));
 require(quantity(Int(std::to_string(n))) == value.get<std::string>(),
         "deposit context number encoding");
 return n;
}
std::string integer(uint64_t n) { return quantity(Int(std::to_string(n))); }
std::string ordinal_key(uint64_t n) {
 std::string out(8, '\0');
 for (unsigned i = 0; i < 8; ++i) out[7 - i] = char(n >> (8 * i));
 return out;
}
struct Locator { uint64_t ordinal; ConsensusRoot digest; };
std::string encode_hint_locator(const Locator& location) {
 return ordinal_key(location.ordinal) + raw(location.digest);
}
Locator decode_hint_locator(const std::string& bytes) {
 require(bytes.size() == locator_bytes, "deposit context locator size");
 Locator out{};
 for (unsigned i = 0; i < 8; ++i) out.ordinal = (out.ordinal << 8) | uint8_t(bytes[i]);
 std::copy(bytes.begin() + 8, bytes.end(), out.digest.begin());
 return out;
}
std::string tuple_key(uint64_t ordinal) { return "t/" + ordinal_key(ordinal); }
std::string root_key(const ConsensusRoot& id) { return "r/" + raw(id); }
std::string hash_key(const Bytes& id) {
 require(id.size() == 32, "deposit context execution hash width");
 return "h/" + std::string(id.begin(), id.end());
}
std::string blob_key(const ConsensusRoot& id) { return "b/" + raw(id); }

struct File {
 int fd = -1;
 File() = default;
 explicit File(int value):fd(value) {}
 File(const File&) = delete;
 File& operator=(const File&) = delete;
 ~File() { if (fd >= 0) ::close(fd); }
};
struct Material {
 uint64_t dev, ino, mode, nlink, uid, gid, size;
 int64_t msec, mnsec, csec, cnsec;
 bool operator==(const Material&) const = default;
};
Material material(const struct stat& st) {
#ifdef __APPLE__
 const auto mt = st.st_mtimespec, ct = st.st_ctimespec;
#else
 const auto mt = st.st_mtim, ct = st.st_ctim;
#endif
 return {uint64_t(st.st_dev), uint64_t(st.st_ino), uint64_t(st.st_mode),
         uint64_t(st.st_nlink), uint64_t(st.st_uid), uint64_t(st.st_gid),
         uint64_t(st.st_size), int64_t(mt.tv_sec), int64_t(mt.tv_nsec),
         int64_t(ct.tv_sec), int64_t(ct.tv_nsec)};
}
Material stat_fd(int fd) {
 struct stat st{};
 if (::fstat(fd, &st) != 0) throw std::runtime_error("deposit context fstat");
 return material(st);
}
Material stat_path(const std::filesystem::path& path) {
 struct stat st{};
 if (::lstat(path.c_str(), &st) != 0) throw std::runtime_error("deposit context lstat");
 return material(st);
}
bool same_directory_identity(const Material& a, const Material& b) {
 return a.dev == b.dev && a.ino == b.ino && a.mode == b.mode &&
        a.uid == b.uid && a.gid == b.gid;
}
void write_all(int fd, std::string_view bytes) {
 size_t at = 0;
 while (at < bytes.size()) {
  const auto n = ::write(fd, bytes.data() + at, bytes.size() - at);
  if (n < 0 && errno == EINTR) continue;
  if (n <= 0) throw std::runtime_error("deposit context marker write");
  at += size_t(n);
 }
 if (::fsync(fd) != 0) throw std::runtime_error("deposit context marker fsync");
}
std::string read_marker(int fd, size_t size) {
 std::string bytes(size, '\0');
 size_t at = 0;
 while (at < size) {
  const auto n = ::pread(fd, bytes.data() + at, size - at, off_t(at));
  if (n < 0 && errno == EINTR) continue;
  if (n <= 0) throw std::runtime_error("deposit context marker read");
  at += size_t(n);
 }
 char extra{};
 if (::pread(fd, &extra, 1, off_t(size)) != 0)
  throw std::runtime_error("deposit context marker grew");
 return bytes;
}
template<class Database = rocksdb::DB>
std::unique_ptr<Database> open_database(const rocksdb::Options& options,
                                      const std::string& path) {
 std::unique_ptr<Database> db;
 if constexpr (requires { Database::Open(options, path, &db); }) {
  status(Database::Open(options, path, &db));
 } else {
  Database* result = nullptr;
  const auto opened = Database::Open(options, path, &result);
  db.reset(result);
  status(opened);
 }
 return db;
}
struct Row {
 uint64_t ordinal;
 std::optional<ConsensusRoot> parent_digest;
 CapellaDepositAncestor tuple;
 ConsensusRoot digest;
 std::vector<Locator> jumps;
};
Json beacon_json(const ConsensusBeaconHeader& h) {
 return Json::array({integer(h.slot), integer(h.proposer_index),
                     root_hex(h.parent_root), root_hex(h.state_root), root_hex(h.body_root)});
}
std::string encode_row(uint64_t ordinal, const std::optional<ConsensusRoot>& parent,
                       const CapellaDepositAncestor& tuple, const Bytes& profile,
                       const std::vector<Locator>& jumps) {
 std::string encoded_jumps;
 for (const auto& location : jumps) encoded_jumps += encode_hint_locator(location);
 return Json::array({"nano-canonical-deposit-tuple-v1", hex(profile), integer(ordinal),
                     parent ? Json(root_hex(*parent)) : Json(nullptr),
                     beacon_json(tuple.beacon), tuple.execution.to_json(),
                     integer(tuple.count), root_hex(tuple.root),
                     hex(Bytes(encoded_jumps.begin(), encoded_jumps.end()))}).dump();
}
bool same_tuple(const CapellaDepositAncestor& a, const CapellaDepositAncestor& b) {
 return a.beacon == b.beacon && a.execution.to_json() == b.execution.to_json() &&
        a.count == b.count && a.root == b.root;
}
void validate_link(const CapellaDepositAncestor& parent,
                   const CapellaDepositAncestor& child) {
 require(child.beacon.slot > parent.beacon.slot &&
         child.beacon.parent_root == consensus_beacon_header_root(parent.beacon) &&
         child.execution.number == parent.execution.number + 1 &&
         child.execution.parent_hash == parent.execution.hash() &&
         child.count >= parent.count, "deposit context noncanonical append");
}
void validate_durable_limits(const Bytes& profile,
                              const CanonicalDepositContextLimits& limits) {
 require(profile.size() == 32 && limits.maximum_tuples && limits.maximum_tuple_bytes &&
         limits.maximum_tuple_record_bytes &&
         limits.maximum_tuple_record_bytes <= limits.maximum_tuple_bytes &&
         limits.maximum_blob_records && limits.maximum_blob_bytes &&
         limits.maximum_cached_blob_records && limits.maximum_cached_blob_bytes,
         "deposit context invalid durable limits");
}
// Byte leases can outlive their cache entry/context. In particular a weak_ptr
// may be locked concurrently with eviction: only the final shared_ptr deleter
// releases this quota, never the decision to erase a cache entry.
struct BlobCharges {
 std::mutex mutex;
 size_t records = 0, bytes = 0;
 std::pair<size_t, size_t> read() {
  std::lock_guard lock(mutex);
  return {records, bytes};
 }
 void reserve(size_t amount, size_t maximum_records, size_t maximum_bytes) {
  std::lock_guard lock(mutex);
  if (records >= maximum_records || amount > maximum_bytes - bytes)
   throw std::length_error("deposit context cache allocations pinned");
  ++records;
  bytes += amount;
 }
 void release(size_t amount) noexcept {
  std::lock_guard lock(mutex);
  --records;
  bytes -= amount;
 }
};
struct DepositBlobCache {
 struct Entry { std::shared_ptr<const Bytes> bytes; uint64_t used; };
 std::map<ConsensusRoot, Entry> cache;
 std::shared_ptr<BlobCharges> charges = std::make_shared<BlobCharges>();
 uint64_t cache_clock = 0;
};
struct DepositProviderOwner {
 std::recursive_timed_mutex mutex;
 bool operating = false;
};
}

struct CanonicalDepositContextState {
 std::recursive_timed_mutex mutex;
 std::atomic<bool> closed{false};
 bool operating = false;
 std::shared_ptr<DepositProviderOwner> provider_owner;
 CanonicalDepositContext::Fetch fetch;
 CanonicalDepositContext::FetchLocator fetch_locator;
 CanonicalDepositContext::Stage stage;
 CanonicalDepositContext::StageLocator stage_locator;
 bool writable = false; // Immutable after construction; safe for owner predicates.
 ConsensusRoot network_root{};
 size_t anchor_bytes = 0, tip_bytes = 0;
 std::filesystem::path directory;
 File directory_fd, marker_fd;
 Material directory_material{}, marker_material{};
 std::string marker;
 Bytes profile;
 CanonicalDepositContextLimits limits;
 std::unique_ptr<rocksdb::DB> db;
 uint64_t tuples = 0, tuple_bytes = 0, blob_records = 0, blob_bytes = 0;
 ConsensusRoot anchor_digest{}, tip_digest{};
 std::string metadata;
 using CacheEntry = DepositBlobCache::Entry;
 std::shared_ptr<DepositBlobCache> blob_cache = std::make_shared<DepositBlobCache>();
 std::recursive_timed_mutex& operation_mutex() {
  return provider_owner ? provider_owner->mutex : mutex;
 }
 bool& operation_active() {
  return provider_owner ? provider_owner->operating : operating;
 }

 struct Operation {
  CanonicalDepositContextState& state;
  std::unique_lock<std::recursive_timed_mutex> lock;
  Operation(CanonicalDepositContextState& owner, const Checkpoint& check)
   :state(owner),lock(owner.operation_mutex(), std::defer_lock) {
   checkpoint(check);
   while (!lock.try_lock_for(std::chrono::milliseconds(10))) checkpoint(check);
   checkpoint(check);
   require(!state.closed.load(), "deposit context closed");
   if (state.operation_active()) throw std::logic_error("reentrant deposit context operation");
   state.operation_active() = true;
  }
  ~Operation() { state.operation_active() = false; }
 };

 void fence(const Checkpoint& check) const {
  checkpoint(check);
  require(!closed.load(), "deposit context closed");
  if (provider_owner) return;
  const auto fd = stat_fd(directory_fd.fd), path = stat_path(directory);
  require(same_directory_identity(fd, directory_material) && fd == path &&
          S_ISDIR(fd.mode) && (fd.mode & 07777) == 0700 && fd.uid == ::geteuid(),
          "deposit context directory changed");
  require(stat_fd(marker_fd.fd) == marker_material &&
          stat_path(directory / "owner.json") == marker_material,
          "deposit context marker changed");
  require(read_marker(marker_fd.fd, marker.size()) == marker,
          "deposit context marker bytes changed");
  checkpoint(check);
  require(stat_fd(marker_fd.fd) == marker_material &&
          stat_path(directory / "owner.json") == marker_material,
          "deposit context marker changed during read");
 }
 std::optional<std::string> fetch_cas(const ConsensusRoot& id, size_t maximum,
                                     const Checkpoint& check) const {
  checkpoint(check);
  require(provider_owner && bool(fetch) && maximum && !closed.load(),
          "deposit context unavailable CAS reader");
  const auto bytes = Bytes(id.begin(), id.end());
  auto value = fetch(bytes, maximum);
  checkpoint(check);
  require(!closed.load(), "deposit context closed during CAS read");
  if (value) require(!value->empty() && value->size() <= maximum && digest(*value) == id,
                     "deposit context CAS capacity or digest");
  return value;
 }
 std::optional<Locator> hint(CanonicalDepositLookup kind, const Bytes& key,
                             const Checkpoint& check) const {
  checkpoint(check);
  require(provider_owner && bool(fetch_locator) && valid_lookup_kind(kind) &&
          key.size() == 32 && !closed.load(),
          "deposit context unavailable locator reader");
  const auto value = fetch_locator(kind, key);
  checkpoint(check);
  require(!closed.load(), "deposit context closed during locator read");
  if (!value) return std::nullopt;
  return Locator{value->ordinal, value->digest};
 }
 void stage_cas(const ConsensusRoot& id, const std::string& payload,
                 const Checkpoint& check) const {
  checkpoint(check);
  require(provider_owner && bool(stage) && bool(stage_locator) && !closed.load(),
          "deposit context is read-only");
  stage(Bytes(id.begin(), id.end()), payload);
  checkpoint(check);
  require(!closed.load(), "deposit context closed during stage");
 }
 void stage_hint(CanonicalDepositLookup kind, const Bytes& key,
                 const Locator& location, const Checkpoint& check) const {
  checkpoint(check);
  require(provider_owner && bool(stage) && bool(stage_locator) &&
          valid_lookup_kind(kind) && key.size() == 32 && !closed.load(),
          "deposit context is read-only");
  stage_locator(kind, key, {location.ordinal, location.digest});
  checkpoint(check);
  require(!closed.load(), "deposit context closed during locator stage");
 }
 uint64_t durable_tuple_charge(const CapellaDepositAncestor& tuple, size_t size) const {
  const auto beacon = consensus_beacon_header_root(tuple.beacon);
  const auto bkey = CanonicalDepositContext::locator_storage_key(profile, network_root,
    CanonicalDepositLookup::BeaconRoot, Bytes(beacon.begin(), beacon.end()));
  const auto ekey = CanonicalDepositContext::locator_storage_key(profile, network_root,
    CanonicalDepositLookup::ExecutionHash, tuple.execution.hash());
  return add(add(size, 19 + 64), add(add(bkey.size(), ekey.size()), 2 * locator_bytes));
 }
 std::optional<std::string> get(const std::string& key, size_t maximum,
                               const Checkpoint& check) const {
  checkpoint(check);
  if (provider_owner) {
   if (key == "metadata") return metadata;
   require(key.size() == 34 && key[1] == '/' &&
           (key[0] == 'b' || key[0] == 'r' || key[0] == 'h'),
           "deposit context unsupported durable key");
   ConsensusRoot id{};std::copy(key.begin() + 2, key.end(), id.begin());
   if (key[0] == 'b') return fetch_cas(id, maximum, check);
   const auto location = hint(key[0] == 'r' ? CanonicalDepositLookup::BeaconRoot :
     CanonicalDepositLookup::ExecutionHash, Bytes(id.begin(), id.end()), check);
   if (!location) return std::nullopt;
   return encode_hint_locator(*location);
  }
  rocksdb::PinnableSlice value;
  const auto found = db->Get(rocksdb::ReadOptions{}, db->DefaultColumnFamily(), key, &value);
  checkpoint(check);
  if (found.IsNotFound()) return std::nullopt;
  status(found);
  require(value.size() <= maximum, "deposit context value capacity");
  std::string copy(value.data(), value.size());
  checkpoint(check);
  return copy;
 }
 std::string next_metadata(uint64_t t, uint64_t tb, uint64_t b, uint64_t bb,
                            const ConsensusRoot& tip) const {
  return Json::array({"nano-canonical-deposit-context-v1", hex(profile),
                     root_hex(anchor_digest), integer(t), integer(tb), integer(b),
                     integer(bb), root_hex(tip)}).dump();
 }
 void check_metadata(const Checkpoint& check) const {
  fence(check);
  if (provider_owner) return;
  const auto current = get("metadata", metadata_bytes, check);
  require(current && *current == metadata, "deposit context metadata changed");
 }
 Row row(const Locator& location, const Checkpoint& check) const {
  require(location.ordinal < tuples, "deposit context ordinal outside history");
  const auto bytes = provider_owner ? fetch_cas(location.digest, limits.maximum_tuple_record_bytes, check) :
    get(tuple_key(location.ordinal), limits.maximum_tuple_record_bytes, check);
  require(bytes && digest(*bytes) == location.digest, "deposit context tuple digest");
  const auto j = Json::parse(*bytes);
  require(j.is_array() && j.size() == 9 && j.dump() == *bytes &&
          j[0] == "nano-canonical-deposit-tuple-v1" && j[1] == hex(profile) &&
          number(j[2]) == location.ordinal, "deposit context tuple schema");
  const auto& h = j[4];
  require(h.is_array() && h.size() == 5, "deposit context beacon schema");
  ConsensusBeaconHeader header{number(h[0]), number(h[1]), root(h[2]), root(h[3]), root(h[4])};
  Row out{location.ordinal, std::nullopt,
          {header, Header::from_json(j[5]), number(j[6]), root(j[7])}, location.digest, {}};
  if (location.ordinal == 0) {
   require(j[3].is_null() && location.digest == anchor_digest,
           "deposit context original anchor changed");
  } else {
   out.parent_digest = root(j[3]);
  }
  require(j[8].is_string(), "deposit context jump encoding");
  const auto& jump_text = j[8].get_ref<const std::string&>();
  require(jump_text.size() <= 2 + 64 * locator_bytes * 2,
          "deposit context jump capacity");
  const auto jump_bytes = unhex(jump_text);
  require(hex(jump_bytes) == jump_text && jump_bytes.size() % locator_bytes == 0,
          "deposit context jump encoding");
  uint64_t distance = 1;
  for (size_t at = 0; at < jump_bytes.size(); at += locator_bytes) {
   require(out.jumps.size() < 64 && distance <= out.ordinal,
           "deposit context jump ordinal");
   const auto part = std::string(jump_bytes.begin() + at,
                                 jump_bytes.begin() + at + locator_bytes);
   const auto jump = decode_hint_locator(part);
   require(jump.ordinal == out.ordinal - distance,
           "deposit context jump distance");
   out.jumps.push_back(jump);
   if (out.jumps.size() < 64) distance <<= 1;
  }
  size_t expected = 0;
  for (auto n = out.ordinal; n; n >>= 1) ++expected;
  require(out.jumps.size() == expected &&
          (!out.ordinal || out.jumps.front().digest == *out.parent_digest),
          "deposit context incomplete jumps");
  require(encode_row(out.ordinal, out.parent_digest, out.tuple, profile, out.jumps) == *bytes,
          "deposit context noncanonical tuple");
  checkpoint(check);
  return out;
 }
 Row cursor_row(const CanonicalDepositContext::Cursor& cursor,
                 const Checkpoint& check) const {
  require(bool(cursor), "deposit context missing cursor");
  const auto owner = cursor->owner_.lock();
  require(owner.get() == this, "deposit context foreign cursor");
  return row({cursor->ordinal_, cursor->digest_}, check);
 }
 Row predecessor(const Row& child, const Checkpoint& check) const {
  require(child.ordinal && child.parent_digest.has_value(), "deposit context no predecessor");
  auto parent = row({child.ordinal - 1, *child.parent_digest}, check);
  validate_link(parent.tuple, child.tuple);
  return parent;
 }
 Row jump(const Row& child, size_t power, const Checkpoint& check) const {
  require(power < child.jumps.size(), "deposit context missing jump");
  auto parent = row(child.jumps[power], check);
  require(parent.tuple.beacon.slot < child.tuple.beacon.slot,
          "deposit context nondecreasing jump slot");
  if (power == 0) validate_link(parent.tuple, child.tuple);
  return parent;
 }
 // Secondary indexes are hints. Authentication always walks a committed digest
 // chain from a private live cursor/tip, never from a caller's ordinal alone.
 Row ancestor(Row from, const Locator& target, const Checkpoint& check) const {
  require(target.ordinal <= from.ordinal, "deposit context future tuple");
  while (from.ordinal > target.ordinal) {
   checkpoint(check);
   const auto distance = from.ordinal - target.ordinal;
   size_t power = 0;
   for (auto n = distance; n > 1; n >>= 1) ++power;
   from = jump(from, power, check);
  }
  require(from.digest == target.digest, "deposit context index ancestry mismatch");
  return from;
 }
 void write(rocksdb::WriteBatch& batch, const Checkpoint& check) {
  require(!provider_owner, "durable context cannot write a database directly");
  fence(check);
  rocksdb::WriteOptions options;
  options.sync = true;
  options.disableWAL = false;
  // Caller updates its retained metadata immediately after Write returns. There
  // is no callback in between: cancellation cannot hide an already committed row.
  status(db->Write(options, &batch));
 }
};

CanonicalDepositCursor::CanonicalDepositCursor(
 std::shared_ptr<const CanonicalDepositContextState> owner, uint64_t ordinal,
 ConsensusRoot digest):owner_(std::move(owner)),ordinal_(ordinal),digest_(digest) {}
CanonicalDepositContext::CanonicalDepositContext(
 std::shared_ptr<CanonicalDepositContextState> state):state_(std::move(state)) {}
CanonicalDepositContext::~CanonicalDepositContext() { try { close(); } catch (...) {} }
CanonicalDepositContext::Cursor CanonicalDepositContext::make_cursor(
 uint64_t ordinal, const ConsensusRoot& digest) const {
 return Cursor(new CanonicalDepositCursor(state_, ordinal, digest));
}

std::shared_ptr<CanonicalDepositContext> CanonicalDepositContext::create(
 const std::filesystem::path& directory, const CapellaDepositAncestor& anchor,
 const Bytes& profile, const CanonicalDepositContextLimits& limits,
 const Checkpoint& check) {
 checkpoint(check);
 require(profile.size() == 32 && limits.maximum_tuples && limits.maximum_tuple_bytes &&
         limits.maximum_tuple_record_bytes &&
         limits.maximum_tuple_record_bytes <= limits.maximum_tuple_bytes &&
         limits.maximum_blob_records && limits.maximum_blob_bytes &&
         limits.maximum_cached_blob_records && limits.maximum_cached_blob_bytes,
         "deposit context invalid limits");
 require(directory.is_absolute() && directory == directory.lexically_normal() &&
         !directory.filename().empty() && directory.filename() != "." &&
         directory.filename() != ".." &&
         std::filesystem::canonical(directory.parent_path()) == directory.parent_path(),
         "deposit context requires canonical new directory");
 auto state = std::make_shared<CanonicalDepositContextState>();
 state->directory = directory;
 state->profile = profile;
 state->limits = limits;
 const auto encoded = encode_row(0, std::nullopt, anchor, profile, {});
 const auto charged = add(encoded.size(), tuple_index_bytes);
 if (charged > limits.maximum_tuple_record_bytes || charged > limits.maximum_tuple_bytes)
  throw std::length_error("deposit context anchor capacity");
 state->anchor_digest = state->tip_digest = digest(encoded);
 state->marker = Json::array({"nano-canonical-deposit-owner-v1", hex(profile),
                              root_hex(consensus_beacon_header_root(anchor.beacon))}).dump();
 checkpoint(check);
 if (::mkdir(directory.c_str(), 0700) != 0)
  throw std::runtime_error("deposit context requires new directory");
 // Every failure retains its new scratch. Never unlink a file whose creation
 // may have failed or whose identity has not been established by this owner.
 state->directory_fd.fd = ::open(directory.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
 require(state->directory_fd.fd >= 0, "deposit context directory open");
 state->directory_material = stat_fd(state->directory_fd.fd);
 require(S_ISDIR(state->directory_material.mode) &&
         (state->directory_material.mode & 07777) == 0700 &&
         state->directory_material.uid == ::geteuid() &&
         state->directory_material == stat_path(directory), "deposit context directory ownership");
 state->marker_fd.fd = ::openat(state->directory_fd.fd, "owner.json",
                               O_RDWR | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
 require(state->marker_fd.fd >= 0, "deposit context exclusive marker create");
 write_all(state->marker_fd.fd, state->marker);
 state->marker_material = stat_fd(state->marker_fd.fd);
 require(S_ISREG(state->marker_material.mode) && state->marker_material.nlink == 1 &&
         (state->marker_material.mode & 07777) == 0600 &&
         state->marker_material.uid == ::geteuid(), "deposit context marker ownership");
 if (::fsync(state->directory_fd.fd) != 0)
  throw std::runtime_error("deposit context directory fsync");
 state->fence(check);
 {
  ScopedChainStoreMemoryPolicy policy(limits.database_memory);
  rocksdb::Options options;
  options.create_if_missing = true;
  options.error_if_exists = true;
  apply_chain_store_memory_policy(options);
  state->db = open_database(options, (directory / "store").string());
 }
 state->fence(check);
 const auto next = state->next_metadata(1, charged, 0, 0, state->tip_digest);
 rocksdb::WriteBatch batch;
 const Locator location{0, state->tip_digest};
 batch.Put(tuple_key(0), encoded);
 batch.Put(root_key(consensus_beacon_header_root(anchor.beacon)), encode_hint_locator(location));
 batch.Put(hash_key(anchor.execution.hash()), encode_hint_locator(location));
 batch.Put("metadata", next);
 state->write(batch, check);
 state->tuples = 1;
 state->tuple_bytes = charged;
 state->metadata = next;
 state->check_metadata(check);
 state->fence(check);
 return std::shared_ptr<CanonicalDepositContext>(new CanonicalDepositContext(std::move(state)));
}

std::string CanonicalDepositContext::locator_storage_key(
 const Bytes& profile, const ConsensusRoot& network,
 CanonicalDepositLookup kind, const Bytes& key) {
 require(profile.size() == 32 && key.size() == 32,
         "deposit context locator key width");
 const char* domain = nullptr;
 switch (kind) {
  case CanonicalDepositLookup::BeaconRoot: domain = "/b/"; break;
  case CanonicalDepositLookup::ExecutionHash: domain = "/e/"; break;
  default: throw std::invalid_argument("deposit context locator kind");
 }
 return "checkpoint-deposit-locator/v4/" + hex(profile).substr(2) + "/" +
   root_hex(network).substr(2) + domain + hex(key).substr(2);
}
std::string CanonicalDepositContext::encode_locator(
 const CanonicalDepositLocator& location) {
 std::string out(locator_bytes, '\0');
 for (unsigned i = 0; i < 8; ++i) out[i] = char(location.ordinal >> (8 * i));
 std::copy(location.digest.begin(), location.digest.end(), out.begin() + 8);
 return out;
}
CanonicalDepositLocator CanonicalDepositContext::decode_locator(const std::string& bytes) {
 require(bytes.size() == locator_bytes, "deposit context durable locator width");
 CanonicalDepositLocator out{};
 for (unsigned i = 0; i < 8; ++i) out.ordinal |= uint64_t(uint8_t(bytes[i])) << (8 * i);
 std::copy(bytes.begin() + 8, bytes.end(), out.digest.begin());
 return out;
}
std::shared_ptr<CanonicalDepositContext> CanonicalDepositContext::create_staged(
 const CapellaDepositAncestor& anchor, const Bytes& profile,
 const CanonicalDepositContextLimits& limits, Fetch fetch, FetchLocator lookup,
 Stage stage, StageLocator stage_locator, const Checkpoint& check) {
 checkpoint(check);
 validate_durable_limits(profile, limits);
 require(bool(fetch) && bool(lookup) && bool(stage) && bool(stage_locator),
         "deposit context requires explicit candidate reader and stage callbacks");
 auto state = std::make_shared<CanonicalDepositContextState>();
 state->provider_owner = std::make_shared<DepositProviderOwner>();
 state->fetch = std::move(fetch);state->fetch_locator = std::move(lookup);
 state->stage = std::move(stage);state->stage_locator = std::move(stage_locator);
 state->writable = true;
 state->profile = profile;state->limits = limits;
 state->network_root = consensus_beacon_header_root(anchor.beacon);
 const auto encoded = encode_row(0, std::nullopt, anchor, profile, {});
 const auto charged = state->durable_tuple_charge(anchor, encoded.size());
 if (charged > limits.maximum_tuple_record_bytes || charged > limits.maximum_tuple_bytes)
  throw std::length_error("deposit context durable anchor capacity");
 state->anchor_digest = state->tip_digest = digest(encoded);
 state->anchor_bytes = state->tip_bytes = encoded.size();
 const Locator first{0, state->tip_digest};
 state->stage_cas(first.digest, encoded, check);
 state->stage_hint(CanonicalDepositLookup::BeaconRoot,
   Bytes(state->network_root.begin(), state->network_root.end()), first, check);
 state->stage_hint(CanonicalDepositLookup::ExecutionHash, anchor.execution.hash(), first, check);
 state->tuples = 1;state->tuple_bytes = charged;
 state->metadata = state->next_metadata(1, charged, 0, 0, state->tip_digest);
 auto context = std::shared_ptr<CanonicalDepositContext>(new CanonicalDepositContext(state));
 context->visit_checkpoint_closure([](const CanonicalDepositBlobRef&) {}, check);
 return context;
}
std::shared_ptr<CanonicalDepositContext> CanonicalDepositContext::from_checkpoint(
 const Json& descriptor, const Bytes& profile, const ConsensusRoot& network,
 const CanonicalDepositContextLimits& limits, Fetch fetch, FetchLocator lookup,
 Stage stage, StageLocator stage_locator, const Checkpoint& check) {
 checkpoint(check);
 validate_durable_limits(profile, limits);
 require(bool(fetch) && bool(lookup) && bool(stage) == bool(stage_locator),
         "deposit context explicit reader and paired stage callbacks required");
 require(descriptor.is_object() && descriptor.size() == 9 &&
   descriptor.contains("format") && descriptor.contains("profile") &&
   descriptor.contains("networkAnchor") && descriptor.contains("anchorBlob") &&
   descriptor.contains("tipBlob") && descriptor.contains("tupleCount") &&
   descriptor.contains("tupleBytes") && descriptor.contains("emittedBlobCount") &&
   descriptor.contains("emittedBlobBytes") &&
   descriptor.at("format") == "nano-canonical-deposit-context-v2" &&
   descriptor.at("profile") == hex(profile) && root(descriptor.at("networkAnchor")) == network,
   "deposit context descriptor scope/schema");
 const auto& a = descriptor.at("anchorBlob");const auto& t = descriptor.at("tipBlob");
 require(a.is_object() && a.size() == 2 && a.contains("digest") && a.contains("bytes") &&
   t.is_object() && t.size() == 3 && t.contains("ordinal") && t.contains("digest") && t.contains("bytes"),
   "deposit context descriptor references");
 const auto anchor_bytes = number(a.at("bytes")), tip_bytes = number(t.at("bytes"));
 const auto tuple_count = number(descriptor.at("tupleCount"));
 const auto tuple_bytes = number(descriptor.at("tupleBytes"));
 const auto blob_count = number(descriptor.at("emittedBlobCount"));
 const auto blob_bytes = number(descriptor.at("emittedBlobBytes"));
 require(anchor_bytes && anchor_bytes <= limits.maximum_tuple_record_bytes &&
   tip_bytes && tip_bytes <= limits.maximum_tuple_record_bytes &&
   tuple_count && tuple_count <= limits.maximum_tuples &&
   number(t.at("ordinal")) == tuple_count - 1 && tuple_bytes <= limits.maximum_tuple_bytes &&
   blob_count <= limits.maximum_blob_records && blob_bytes <= limits.maximum_blob_bytes &&
   (blob_count == 0) == (blob_bytes == 0) && blob_count <= blob_bytes &&
   (blob_bytes / maximum_node_bytes < blob_count ||
    (blob_bytes / maximum_node_bytes == blob_count && blob_bytes % maximum_node_bytes == 0)),
   "deposit context descriptor exceeds external limits");
 auto state = std::make_shared<CanonicalDepositContextState>();
 state->provider_owner = std::make_shared<DepositProviderOwner>();
 state->profile = profile;state->network_root = network;state->limits = limits;
 state->fetch = std::move(fetch);state->fetch_locator = std::move(lookup);
 state->stage = std::move(stage);state->stage_locator = std::move(stage_locator);
 state->writable = bool(state->stage);
 state->anchor_digest = root(a.at("digest"));state->tip_digest = root(t.at("digest"));
 state->anchor_bytes = size_t(anchor_bytes);state->tip_bytes = size_t(tip_bytes);
 state->tuples = tuple_count;state->tuple_bytes = tuple_bytes;
 state->blob_records = blob_count;state->blob_bytes = blob_bytes;
 state->metadata = state->next_metadata(tuple_count, tuple_bytes, blob_count, blob_bytes,
                                       state->tip_digest);
 auto context = std::shared_ptr<CanonicalDepositContext>(new CanonicalDepositContext(state));
 // This authenticates every canonical row, parent/jump and both locator rows.
 // Node-tree closure validation is separately mandatory in History restore.
 context->visit_checkpoint_closure([](const CanonicalDepositBlobRef&) {}, check);
 return context;
}
Json CanonicalDepositContext::checkpoint_descriptor(const Checkpoint& check) const {
 CanonicalDepositContextState::Operation operation(*state_, check);
 require(bool(state_->provider_owner), "scratch context has no durable descriptor");
 state_->check_metadata(check);
 const auto anchor = state_->row({0, state_->anchor_digest}, check);
 const auto tip = state_->row({state_->tuples - 1, state_->tip_digest}, check);
 require(consensus_beacon_header_root(anchor.tuple.beacon) == state_->network_root &&
   encode_row(0, anchor.parent_digest, anchor.tuple, state_->profile, anchor.jumps).size() == state_->anchor_bytes &&
   encode_row(tip.ordinal, tip.parent_digest, tip.tuple, state_->profile, tip.jumps).size() == state_->tip_bytes,
   "deposit context descriptor anchor/tip references changed");
 auto out = Json{{"format", "nano-canonical-deposit-context-v2"},
   {"profile", hex(state_->profile)}, {"networkAnchor", root_hex(state_->network_root)},
   {"anchorBlob", Json{{"digest", root_hex(state_->anchor_digest)}, {"bytes", integer(state_->anchor_bytes)}}},
   {"tipBlob", Json{{"ordinal", integer(state_->tuples - 1)}, {"digest", root_hex(state_->tip_digest)}, {"bytes", integer(state_->tip_bytes)}}},
   {"tupleCount", integer(state_->tuples)}, {"tupleBytes", integer(state_->tuple_bytes)},
   {"emittedBlobCount", integer(state_->blob_records)}, {"emittedBlobBytes", integer(state_->blob_bytes)}};
 state_->fence(check);
 return out;
}
std::shared_ptr<CanonicalDepositContext> CanonicalDepositContext::fork_staged(
 Fetch fetch, FetchLocator lookup, Stage stage, StageLocator stage_locator,
 const Checkpoint& check) const {
 CanonicalDepositContextState::Operation operation(*state_, check);
 state_->check_metadata(check);
 require(state_->provider_owner && bool(fetch) && bool(lookup) &&
   bool(stage) == bool(stage_locator), "deposit context requires explicit same-namespace fork readers");
 auto next = std::make_shared<CanonicalDepositContextState>();
 next->provider_owner = state_->provider_owner;next->blob_cache = state_->blob_cache;
 next->fetch = std::move(fetch);next->fetch_locator = std::move(lookup);
 next->stage = std::move(stage);next->stage_locator = std::move(stage_locator);
 next->writable = bool(next->stage);
 next->profile = state_->profile;next->limits = state_->limits;
 next->network_root = state_->network_root;
 next->anchor_digest = state_->anchor_digest;next->tip_digest = state_->tip_digest;
 next->anchor_bytes = state_->anchor_bytes;next->tip_bytes = state_->tip_bytes;
 next->tuples = state_->tuples;next->tuple_bytes = state_->tuple_bytes;
 next->blob_records = state_->blob_records;next->blob_bytes = state_->blob_bytes;
 next->metadata = state_->metadata;
 const auto anchor = next->row({0, next->anchor_digest}, check);
 const auto tip = next->row({next->tuples - 1, next->tip_digest}, check);
 require(consensus_beacon_header_root(anchor.tuple.beacon) == next->network_root &&
   encode_row(0, anchor.parent_digest, anchor.tuple, next->profile, anchor.jumps).size() == next->anchor_bytes &&
   encode_row(tip.ordinal, tip.parent_digest, tip.tuple, next->profile, tip.jumps).size() == next->tip_bytes,
   "deposit context fork reader cannot resolve the committed prefix");
 state_->fence(check);
 return std::shared_ptr<CanonicalDepositContext>(new CanonicalDepositContext(std::move(next)));
}
bool CanonicalDepositContext::shares_blob_storage(const CanonicalDepositContext& target) const {
 ensure_open({});target.ensure_open({});
 if (state_.get() == target.state_.get()) return true;
 if (!state_->provider_owner || state_->provider_owner != target.state_->provider_owner) return false;
 require(!state_->closed.load() && !target.state_->closed.load(), "deposit context closed");
 return !state_->writable;
}
CanonicalDepositContextLimits CanonicalDepositContext::limits() const {
 ensure_open({});return state_->limits;
}
Bytes CanonicalDepositContext::profile_digest() const {
 ensure_open({});return state_->profile;
}
CanonicalDepositContext::Cursor CanonicalDepositContext::tip_cursor(const Checkpoint& check) const {
 CanonicalDepositContextState::Operation operation(*state_, check);
 state_->check_metadata(check);
 const auto tip = state_->row({state_->tuples - 1, state_->tip_digest}, check);
 state_->fence(check);
 return make_cursor(tip.ordinal, tip.digest);
}
void CanonicalDepositContext::visit_checkpoint_closure(const ClosureVisitor& visit,
 const Checkpoint& check) const {
 CanonicalDepositContextState::Operation operation(*state_, check);
 require(bool(state_->provider_owner), "scratch context has no durable closure");
 state_->check_metadata(check);
 auto current = state_->row({state_->tuples - 1, state_->tip_digest}, check);
 uint64_t seen = 0, total = 0;
 while (true) {
  checkpoint(check);
  const auto encoded = encode_row(current.ordinal, current.parent_digest, current.tuple,
                                  state_->profile, current.jumps);
  const auto charged = state_->durable_tuple_charge(current.tuple, encoded.size());
  total = add(total, charged);++seen;
  require(charged <= state_->limits.maximum_tuple_record_bytes &&
    total <= state_->limits.maximum_tuple_bytes && seen <= state_->limits.maximum_tuples,
    "deposit context closure capacity");
  const auto beacon = consensus_beacon_header_root(current.tuple.beacon);
  const auto b = state_->hint(CanonicalDepositLookup::BeaconRoot, Bytes(beacon.begin(), beacon.end()), check);
  const auto e = state_->hint(CanonicalDepositLookup::ExecutionHash, current.tuple.execution.hash(), check);
  require(b && e && b->ordinal == current.ordinal && e->ordinal == current.ordinal &&
    b->digest == current.digest && e->digest == current.digest,
    "deposit context closure locator missing or conflicting");
  for (size_t power = 1; power < current.jumps.size(); ++power) {
   const auto middle = state_->row(current.jumps[power - 1], check);
   require(power - 1 < middle.jumps.size() &&
     middle.jumps[power - 1].ordinal == current.jumps[power].ordinal &&
     middle.jumps[power - 1].digest == current.jumps[power].digest,
     "deposit context closure inconsistent jump");
  }
  if (current.ordinal == state_->tuples - 1)
   require(encoded.size() == state_->tip_bytes, "deposit context tip byte commitment");
  if (visit) { visit({current.digest, uint64_t(encoded.size())});checkpoint(check); }
  if (!current.ordinal) {
   require(current.digest == state_->anchor_digest && encoded.size() == state_->anchor_bytes &&
     beacon == state_->network_root, "deposit context closure network anchor");
   break;
  }
  current = state_->predecessor(current, check);
 }
 require(seen == state_->tuples && total == state_->tuple_bytes,
         "deposit context closure accounting mismatch");
 state_->fence(check);
}

void CanonicalDepositContext::ensure_open(const Checkpoint& check) const {
 checkpoint(check);
 require(!state_->closed.load(), "deposit context closed");
 checkpoint(check);
 require(!state_->closed.load(), "deposit context closed");
}
CanonicalDepositContext::Cursor CanonicalDepositContext::anchor_cursor(const Checkpoint& check) const {
 CanonicalDepositContextState::Operation operation(*state_, check);
 state_->check_metadata(check);
 (void)state_->row({0, state_->anchor_digest}, check);
 state_->fence(check);
 return make_cursor(0, state_->anchor_digest);
}
CanonicalDepositContext::Cursor CanonicalDepositContext::append_admitted(
 const CapellaDepositAncestor& tuple, const Checkpoint& check) {
 CanonicalDepositContextState::Operation operation(*state_, check);
 state_->check_metadata(check);
 require(!state_->provider_owner || (bool(state_->stage) && bool(state_->stage_locator)),
         "deposit context is read-only");
 const auto id = consensus_beacon_header_root(tuple.beacon);
 if (const auto existing = state_->get(root_key(id), locator_bytes, check)) {
  const auto location = decode_hint_locator(*existing);
  const auto old = state_->ancestor(state_->row({state_->tuples - 1, state_->tip_digest}, check),
                                     location, check);
  require(same_tuple(old.tuple, tuple), "deposit context conflicting duplicate");
  state_->fence(check);
  return make_cursor(old.ordinal, old.digest);
 }
 const auto parent = state_->row({state_->tuples - 1, state_->tip_digest}, check);
 validate_link(parent.tuple, tuple);
 require(!state_->get(hash_key(tuple.execution.hash()), locator_bytes, check),
         "deposit context repeated execution hash");
 std::vector<Locator> jumps{{parent.ordinal, parent.digest}};
 // The next 2^k ancestor is the committed midpoint's own 2^(k-1)
 // ancestor. No locator from a mutable secondary index participates here.
 for (size_t power = 1; power < 64; ++power) {
  const auto middle = state_->row(jumps.back(), check);
  if (power - 1 >= middle.jumps.size()) break;
  jumps.push_back(middle.jumps[power - 1]);
 }
 const auto encoded = encode_row(state_->tuples, state_->tip_digest, tuple, state_->profile, jumps);
 const auto charged = state_->provider_owner ? state_->durable_tuple_charge(tuple, encoded.size()) :
   add(encoded.size(), tuple_index_bytes);
 if (state_->tuples >= state_->limits.maximum_tuples ||
     charged > state_->limits.maximum_tuple_record_bytes ||
     charged > state_->limits.maximum_tuple_bytes - state_->tuple_bytes)
  throw std::length_error("deposit context tuple capacity");
 const Locator location{state_->tuples, digest(encoded)};
 auto next = state_->next_metadata(state_->tuples + 1, state_->tuple_bytes + charged,
                                         state_->blob_records, state_->blob_bytes, location.digest);
 if (state_->provider_owner) {
  state_->stage_cas(location.digest, encoded, check);
  state_->stage_hint(CanonicalDepositLookup::BeaconRoot, Bytes(id.begin(), id.end()), location, check);
  state_->stage_hint(CanonicalDepositLookup::ExecutionHash, tuple.execution.hash(), location, check);
 } else {
  rocksdb::WriteBatch batch;
  batch.Put(tuple_key(location.ordinal), encoded);
  batch.Put(root_key(id), encode_hint_locator(location));
  batch.Put(hash_key(tuple.execution.hash()), encode_hint_locator(location));
  batch.Put("metadata", next);
  state_->write(batch, check);
 }
 ++state_->tuples;
 state_->tuple_bytes += charged;
 state_->tip_digest = location.digest;
 state_->tip_bytes = encoded.size();
 if (state_->provider_owner) state_->metadata.swap(next); else state_->metadata = next;
 state_->check_metadata(check);
 state_->fence(check);
 return make_cursor(location.ordinal, location.digest);
}
CanonicalDepositContext::Cursor CanonicalDepositContext::cursor_for_root(
 const ConsensusRoot& id, const Checkpoint& check) const {
 CanonicalDepositContextState::Operation operation(*state_, check);
 state_->check_metadata(check);
 const auto hint = state_->get(root_key(id), locator_bytes, check);
 require(hint.has_value(), "deposit context unknown root");
 const auto target = state_->ancestor(state_->row({state_->tuples - 1, state_->tip_digest}, check),
                                      decode_hint_locator(*hint), check);
 require(consensus_beacon_header_root(target.tuple.beacon) == id, "deposit context root index mismatch");
 state_->fence(check);
 return make_cursor(target.ordinal, target.digest);
}
CapellaDepositAncestor CanonicalDepositContext::at(const Cursor& cursor, const Checkpoint& check) const {
 CanonicalDepositContextState::Operation operation(*state_, check);
 state_->check_metadata(check);
 const auto row = state_->cursor_row(cursor, check);
 state_->fence(check);
 return row.tuple;
}
void CanonicalDepositContext::validate_cursor(const Cursor& cursor,
 const ConsensusBeaconHeader& beacon, const Header& execution, const Checkpoint& check) const {
 const auto tuple = at(cursor, check);
 require(tuple.beacon == beacon && tuple.execution.to_json() == execution.to_json(),
         "deposit context hot anchor mismatch");
 checkpoint(check);
}
std::optional<CapellaDepositAncestor> CanonicalDepositContext::find_before(
 const Cursor& cursor, const Bytes& hash, uint64_t count, const ConsensusRoot& deposit_root,
 const Checkpoint& check) const {
 CanonicalDepositContextState::Operation operation(*state_, check);
 state_->check_metadata(check);
 auto anchor = state_->cursor_row(cursor, check);
 const auto hint = state_->get(hash_key(hash), locator_bytes, check);
 if (!hint) {
  state_->fence(check);
  if (state_->provider_owner) throw std::runtime_error("deposit context locator unavailable");
  return std::nullopt;
 }
 const auto target = decode_hint_locator(*hint);
 if (state_->provider_owner) {
  // Even an excluded/future hit is only a hint: prove its membership in the
  // held canonical tip and its requested execution key before returning None.
  const auto authenticated = state_->ancestor(
    state_->row({state_->tuples - 1, state_->tip_digest}, check), target, check);
  require(authenticated.tuple.execution.hash() == hash,
          "deposit context durable hash index mismatch");
  if (target.ordinal >= anchor.ordinal) { state_->fence(check); return std::nullopt; }
 }
 if (target.ordinal >= anchor.ordinal) { state_->fence(check); return std::nullopt; }
 const auto found = state_->ancestor(std::move(anchor), target, check);
 require(found.tuple.execution.hash() == hash, "deposit context hash index mismatch");
 state_->fence(check);
 if (found.tuple.count != count || found.tuple.root != deposit_root) return std::nullopt;
 return found.tuple;
}
CapellaDepositAncestor CanonicalDepositContext::last_before(
 const Cursor& cursor, uint64_t start_slot, const Checkpoint& check) const {
 CanonicalDepositContextState::Operation operation(*state_, check);
 state_->check_metadata(check);
 auto current = state_->cursor_row(cursor, check);
 if (current.ordinal) current = state_->predecessor(current, check);
 if (current.tuple.beacon.slot < start_slot) { state_->fence(check); return current.tuple; }
 // Slots are strictly increasing at admission. Skip whole authenticated ranges
 // whose oldest endpoint is still too recent, then inspect the predecessor.
 for (size_t power = current.jumps.size(); power > 0; --power) {
  if (power - 1 >= current.jumps.size()) continue;
  auto candidate = state_->jump(current, power - 1, check);
  if (candidate.tuple.beacon.slot >= start_slot) current = std::move(candidate);
 }
 if (current.ordinal) current = state_->predecessor(current, check);
 state_->fence(check);
 return current.tuple; // Explicit original-anchor fallback, even before period 0.
}

CanonicalDepositBlobRef CanonicalDepositContext::put_node_blob(
 const Bytes& bytes, const Checkpoint& check) {
 checkpoint(check);
 require(!bytes.empty() && bytes.size() <= maximum_node_bytes, "deposit context node size");
 const auto id = digest(std::string_view(reinterpret_cast<const char*>(bytes.data()), bytes.size()));
 CanonicalDepositContextState::Operation operation(*state_, check);
 state_->check_metadata(check);
 const auto key = blob_key(id);
 require(!state_->provider_owner || (bool(state_->stage) && bool(state_->stage_locator)),
         "deposit context is read-only");
 if (const auto old = state_->get(key, maximum_node_bytes, check)) {
  require(old->size() == bytes.size() && std::equal(old->begin(), old->end(), bytes.begin(),
          [](char a, uint8_t b) { return uint8_t(a) == b; }) && digest(*old) == id,
          "deposit context conflicting blob");
  state_->fence(check);
  return {id, uint64_t(bytes.size())};
 }
 if (state_->blob_records >= state_->limits.maximum_blob_records ||
     bytes.size() > state_->limits.maximum_blob_bytes - state_->blob_bytes)
  throw std::length_error("deposit context blob capacity");
 auto next = state_->next_metadata(state_->tuples, state_->tuple_bytes,
                                         state_->blob_records + 1,
                                         state_->blob_bytes + bytes.size(), state_->tip_digest);
 if (state_->provider_owner) {
  state_->stage_cas(id, std::string(bytes.begin(), bytes.end()), check);
 } else {
  rocksdb::WriteBatch batch;
  batch.Put(key, rocksdb::Slice(reinterpret_cast<const char*>(bytes.data()), bytes.size()));
  batch.Put("metadata", next);
  state_->write(batch, check);
 }
 ++state_->blob_records;
 state_->blob_bytes += bytes.size();
 if (state_->provider_owner) state_->metadata.swap(next); else state_->metadata = next;
 state_->check_metadata(check);
 state_->fence(check);
 return {id, uint64_t(bytes.size())};
}
std::shared_ptr<const Bytes> CanonicalDepositContext::lease_node_blob(
 const CanonicalDepositBlobRef& reference, const Checkpoint& check) {
 CanonicalDepositContextState::Operation operation(*state_, check);
 state_->check_metadata(check);
 require(reference.bytes && reference.bytes <= maximum_node_bytes &&
         reference.bytes <= state_->limits.maximum_cached_blob_bytes,
         "deposit context blob reference size");
 // A shared cache is never proof that a different candidate's overlay/primary
 // contains a blob. In particular it must not resurrect an aborted stage.
 std::optional<std::string> visible;
 if (state_->provider_owner) {
  visible = state_->fetch_cas(reference.digest, maximum_node_bytes, check);
  require(visible && visible->size() == reference.bytes,
          "deposit context blob unavailable in this candidate reader");
 }
 if (state_->blob_cache->cache_clock == UINT64_MAX) throw std::overflow_error("deposit context cache clock");
 if (auto known = state_->blob_cache->cache.find(reference.digest); known != state_->blob_cache->cache.end()) {
  require(known->second.bytes->size() == reference.bytes, "deposit context cached reference mismatch");
  state_->fence(check);
  known->second.used = ++state_->blob_cache->cache_clock;
  return known->second.bytes;
 }
 std::vector<std::pair<uint64_t, ConsensusRoot>> victims;
 for (const auto& [id, entry] : state_->blob_cache->cache) {
  checkpoint(check);
  if (entry.bytes.use_count() == 1) victims.emplace_back(entry.used, id);
 }
 std::sort(victims.begin(), victims.end());
 auto [count, size] = state_->blob_cache->charges->read();
 size_t used = 0;
 while (count >= state_->limits.maximum_cached_blob_records ||
        reference.bytes > state_->limits.maximum_cached_blob_bytes - size) {
  if (used == victims.size()) throw std::length_error("deposit context cache records pinned");
  const auto& victim = state_->blob_cache->cache.at(victims[used++].second);
  --count;
  size -= victim.bytes->size();
 }
 const auto bytes = state_->provider_owner ? std::move(visible) :
   state_->get(blob_key(reference.digest), maximum_node_bytes, check);
 require(bytes && bytes->size() == reference.bytes && digest(*bytes) == reference.digest,
         "deposit context missing or changed blob");
 checkpoint(check);
 // A weak lease may become strong at any point. Removing a candidate never
 // releases its charge until its final immutable byte owner actually dies.
 for (size_t i = 0; i < used; ++i) {
  auto victim = state_->blob_cache->cache.find(victims[i].second);
  if (victim->second.bytes.use_count() == 1) state_->blob_cache->cache.erase(victim);
 }
 state_->blob_cache->charges->reserve(size_t(reference.bytes),
                          state_->limits.maximum_cached_blob_records,
                          state_->limits.maximum_cached_blob_bytes);
 Bytes* candidate = nullptr;
 try { candidate = new Bytes(bytes->begin(), bytes->end()); }
 catch (...) { state_->blob_cache->charges->release(size_t(reference.bytes)); throw; }
 const auto charges = state_->blob_cache->charges;
 // shared_ptr calls the deleter itself if its control-block allocation fails.
 auto lease = std::shared_ptr<const Bytes>(candidate,
   [charges, amount = size_t(reference.bytes)](const Bytes* owned) {
    delete owned;
    charges->release(amount);
   });
 checkpoint(check);
 state_->blob_cache->cache.emplace(reference.digest,
                        CanonicalDepositContextState::CacheEntry{lease, state_->blob_cache->cache_clock + 1});
 ++state_->blob_cache->cache_clock;
 state_->fence(check);
 return lease;
}
CanonicalDepositContextUsage CanonicalDepositContext::usage() const {
 std::lock_guard lock(state_->operation_mutex());
 size_t unpinned = 0;
 for (const auto& [id, entry] : state_->blob_cache->cache) if (entry.bytes.use_count() == 1) ++unpinned;
 const auto [records, bytes] = state_->blob_cache->charges->read();
 return {state_->tuples, state_->tuple_bytes, state_->blob_records, state_->blob_bytes,
         records, bytes, records - unpinned};
}
void CanonicalDepositContext::close() {
 std::lock_guard lock(state_->operation_mutex());
 if (state_->operation_active()) throw std::logic_error("reentrant deposit context close");
 if (state_->closed.exchange(true)) return;
 state_->db.reset();
 state_->fetch = {};state_->fetch_locator = {};state_->stage = {};state_->stage_locator = {};
 state_->blob_cache->cache.clear(); // Retire lookup owners; external leases remain charged.
 // Leases remain valid immutable bytes. Scratch is deliberately retained; no
 // child filename grants deletion authority. FD RAII releases owner handles.
}
}
