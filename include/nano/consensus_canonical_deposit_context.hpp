#pragma once
#include "nano/consensus_capella_fork_store.hpp"
#include "nano/storage_memory_policy.hpp"
#include <filesystem>
#include <functional>
#include <memory>
#include <utility>

namespace nano {
struct CanonicalDepositContextState;
namespace deposit_publication_detail { class DepositPublicationFactory; }

// A cursor is created only by its live context. Its ordinal is a locator, not
// ancestry authority; context identity and the committed row digest are checked.
class CanonicalDepositCursor {
 friend class CanonicalDepositContext;
 friend struct CanonicalDepositContextState;
 std::weak_ptr<const CanonicalDepositContextState> owner_;
 uint64_t ordinal_=0;
 ConsensusRoot digest_{};
 CanonicalDepositCursor(std::shared_ptr<const CanonicalDepositContextState>,uint64_t,ConsensusRoot);
public:
 ~CanonicalDepositCursor()=default;
 CanonicalDepositCursor(const CanonicalDepositCursor&)=default;
 uint64_t ordinal()const{return ordinal_;}
};

struct CanonicalDepositBlobRef {
 ConsensusRoot digest; // SHA256 of encoded payload, never its SSZ Merkle root.
 uint64_t bytes;
 bool operator==(const CanonicalDepositBlobRef&)const=default;
};
// A primary-store lookup is a locator hint only. Every hit is authenticated
// from a private cursor/tip's committed tuple digest and exact key fields.
enum class CanonicalDepositLookup { BeaconRoot, ExecutionHash };
struct CanonicalDepositLocator {
 uint64_t ordinal;
 ConsensusRoot digest;
 bool operator==(const CanonicalDepositLocator&)const=default;
};
struct CanonicalDepositContextLimits {
 uint64_t maximum_tuples,maximum_tuple_bytes;
 // One encoded row plus its tuple key and two fixed-width secondary indexes.
 // 8192 is a useful local choice; oversized values refuse, never truncate.
 size_t maximum_tuple_record_bytes;
 // In scratch mode: new unique blobs of the owned scratch DB. In durable
 // staging: successful newly emitted payloads after global-CAS deduplication.
 // These are NOT the reachable History closure or whole-DB/disk bounds.
 // History needs an INDEPENDENT declared traversal budget before checkpoint
 // publication/restore. These emission limits do not supply that budget.
 uint64_t maximum_blob_records,maximum_blob_bytes;
 size_t maximum_cached_blob_records,maximum_cached_blob_bytes;
 ChainStoreMemoryPolicy database_memory;
};
struct CanonicalDepositContextUsage {
 // Durable blob counters describe emissions, not all accessible CAS content.
 uint64_t tuples,tuple_bytes,blob_records,blob_bytes;
 // Counts live allocations, including a lease concurrently retired from the
 // lookup cache. Its charge survives until the last shared byte owner dies.
 size_t cached_blob_records,cached_blob_bytes,pinned_blob_records;
};

// Canonical prefix only, never the hot fork-choice DAG. Callers append ONLY
// after full signed admission AND execution validation; this component verifies
// links and stored commitments, not block admission or an external checkpoint.
// Legacy create() makes one new owner0700 audit scratch outside the audited DB.
// The explicit durable APIs use callback readers/private staging and never write
// a DB; primary atomic publication, witnesses, backups and GC remain the caller's
// responsibility. Logical values/cache budgets do not bound physical SST/WAL/RSS.
class CanonicalDepositContext {
 friend class deposit_publication_detail::DepositPublicationFactory;
 std::shared_ptr<CanonicalDepositContextState> state_;
 explicit CanonicalDepositContext(std::shared_ptr<CanonicalDepositContextState>);
 std::shared_ptr<const CanonicalDepositCursor> make_cursor(uint64_t,const ConsensusRoot&)const;
public:
 using Cursor=std::shared_ptr<const CanonicalDepositCursor>;
 using Fetch=std::function<std::optional<std::string>(const Bytes&,size_t)>;
 using Stage=std::function<void(const Bytes&,const std::string&)>;
 using FetchLocator=std::function<std::optional<CanonicalDepositLocator>(
  CanonicalDepositLookup,const Bytes&)>;
 using StageLocator=std::function<void(CanonicalDepositLookup,const Bytes&,
  const CanonicalDepositLocator&)>;
 using ClosureVisitor=std::function<void(const CanonicalDepositBlobRef&)>;
 static std::shared_ptr<CanonicalDepositContext> create(
  const std::filesystem::path& new_directory,const CapellaDepositAncestor& anchor,
  const Bytes& profile_digest,const CanonicalDepositContextLimits&,
  const std::function<void()>& checkpoint);
 // Staging callbacks MUST write only to the candidate's private pending batch.
 // Fetch/FetchLocator must read that overlay before the durable primary store.
 // They retain their backing provider lifetime. Any exception aborts the whole
 // candidate: pending orphan blobs/locator updates must never be published.
 static std::shared_ptr<CanonicalDepositContext> create_staged(
  const CapellaDepositAncestor& anchor,const Bytes& profile_digest,
  const CanonicalDepositContextLimits&,Fetch,FetchLocator,Stage,StageLocator,
  const std::function<void()>& checkpoint);
 // Descriptor must already belong to an externally authenticated checkpoint.
 // Bounds are supplied externally, never learned from an encoded descriptor.
 // Empty Stage+StageLocator create a read-only published context.
 static std::shared_ptr<CanonicalDepositContext> from_checkpoint(
  const Json& descriptor,const Bytes& expected_profile_digest,
  const ConsensusRoot& expected_network_root,const CanonicalDepositContextLimits&,
  Fetch,FetchLocator,Stage,StageLocator,const std::function<void()>& checkpoint);
 // V2 names emittedBlobCount/emittedBlobBytes explicitly retain that scope.
 // This serializes no tree closure and stages nothing; scratch mode refuses.
 Json checkpoint_descriptor(const std::function<void()>& checkpoint)const;
 // Readers MUST address the same backing CAS namespace and the candidate's
 // own pending overlay. Empty stage callbacks make a read-only fork; publishing
 // that view requires a durable primary reader after the atomic commit.
 std::shared_ptr<CanonicalDepositContext> fork_staged(
  Fetch,FetchLocator,Stage,StageLocator,const std::function<void()>& checkpoint)const;
 // Directional: this SOURCE may share committed references with the target.
 // A writable candidate is not reusable by another context merely because its
 // cache owner matches. This predicate never attests a primary commit.
 bool shares_blob_storage(const CanonicalDepositContext&)const;
 CanonicalDepositContextLimits limits()const;
 Bytes profile_digest()const;
 Cursor tip_cursor(const std::function<void()>& checkpoint)const;
 // Visits/authenticates all canonical tuple CAS rows, including anchor and
 // jump references. History node closures are visited separately by History.
 void visit_checkpoint_closure(const ClosureVisitor&,
  const std::function<void()>& checkpoint)const;
 static std::string locator_storage_key(const Bytes& profile_digest,
  const ConsensusRoot& network_root,CanonicalDepositLookup,const Bytes& key);
 static std::string encode_locator(const CanonicalDepositLocator&);
 static CanonicalDepositLocator decode_locator(const std::string&);
 ~CanonicalDepositContext();
 CanonicalDepositContext(const CanonicalDepositContext&)=delete;
 CanonicalDepositContext& operator=(const CanonicalDepositContext&)=delete;

 Cursor anchor_cursor(const std::function<void()>& checkpoint)const;
 // Exact duplicate is idempotent. A different sibling or non-tip parent refuses;
 // this canonical-audit context never becomes a shared mutable fork-choice head.
 Cursor append_admitted(const CapellaDepositAncestor&,const std::function<void()>& checkpoint);
 Cursor cursor_for_root(const ConsensusRoot&,const std::function<void()>& checkpoint)const;
 CapellaDepositAncestor at(const Cursor&,const std::function<void()>& checkpoint)const;
 void validate_cursor(const Cursor&,const ConsensusBeaconHeader&,const Header&,
  const std::function<void()>& checkpoint)const;
 // The supplied cursor is the hot anchor. Only its STRICT ancestors may match;
 // later rows are never admitted by consulting the context's newest ordinal.
 std::optional<CapellaDepositAncestor> find_before(const Cursor& exclusive_anchor,
  const Bytes& execution_hash,uint64_t count,const ConsensusRoot& deposit_root,
  const std::function<void()>& checkpoint)const;
 CapellaDepositAncestor last_before(const Cursor& exclusive_anchor,uint64_t start_slot,
  const std::function<void()>& checkpoint)const;

 CanonicalDepositBlobRef put_node_blob(const Bytes&,const std::function<void()>& checkpoint);
 std::shared_ptr<const Bytes> lease_node_blob(const CanonicalDepositBlobRef&,
  const std::function<void()>& checkpoint);
 void ensure_open(const std::function<void()>& checkpoint)const;
 CanonicalDepositContextUsage usage()const;
 // Serializes with in-flight operations. Existing immutable byte leases survive;
 // every later operation, including a cache hit, fails closed.
 void close();
private:
 // Only the fixed full-durable preflight factory may transplant certified
 // readonly metadata to a dormant PRIMARY reader. No decoding or fetch occurs;
 // normal public restore/fork APIs still authenticate their complete inputs.
 std::pair<std::shared_ptr<CanonicalDepositContext>,Cursor> clone_preflight_primary(
  Fetch,FetchLocator,const Cursor& source_tip,const std::function<void()>&)const;
};
}
