#pragma once
#include "receipt.hpp"
#include "finality.hpp"
#include <filesystem>
#include <mutex>
#include <functional>
namespace rocksdb { class DB; class WriteBatch; }
namespace nano {
struct ForkCheckpointToken {uint64_t sequence;Bytes digest;bool operator==(const ForkCheckpointToken&)const=default;};
struct ExecutionStateRetentionAnchor {Int number;Bytes hash;};
struct ExecutionStatePruningInfo {Int before;ExecutionStateRetentionAnchor anchor;};
struct ExecutionCheckpointBlock {Block block;State state;std::vector<Receipt> receipts;};
// Opaque authenticated consensus record; validation belongs to the reactor.
struct CheckpointArchiveEntry {Bytes root;uint64_t slot;Bytes payload;};
// Local opaque immutable fragments. The caller validates consensus semantics;
// the storage layer verifies the SHA256 commitment and synchronous atomicity.
struct CheckpointBlob {Bytes digest;std::string payload;};
// Non-authoritative immutable local lookup hints. Their row digests MUST be
// authenticated against a held canonical cursor by the caller. No peer API.
struct CheckpointDepositLocatorKey {
 Bytes profile_digest,network_anchor,key;
 bool execution_hash=false;
};
struct CheckpointDepositLocatorRecord {
 CheckpointDepositLocatorKey lookup;
 uint64_t ordinal;
 Bytes digest;
};
// A request-owned serialized canonical state; no proof/finality authority.
struct SerializedExecutionState {bool canonical=false;std::optional<std::string> encoded;};
struct ForkCheckpointRecord {ForkCheckpointToken token;std::string snapshot;};
class ChainStore {
 friend class BeaconSigningJournal;
 friend class AltairDurableReactor;
 friend class BellatrixDurableReactor;
 friend class CapellaDurableReactor;
 friend class CheckpointBlobGcCopies;
 mutable std::mutex altair_reactor_mutex_;
 // Dedicated signing databases serialize all journal users through this gate,
 // including callbacks, independently of the short RocksDB operation mutex.
 mutable std::mutex beacon_signing_mutex_;
 rocksdb::DB* db_=nullptr;
 mutable std::mutex mutex_;
 bool fork_checkpoint_write_uncertain_=false;
 void append_checkpoint_blobs(rocksdb::WriteBatch&,const std::vector<CheckpointBlob>&,size_t maximum_records,size_t maximum_bytes);
 ForkCheckpointToken commit_fork_checkpoint_parts(const Int&,const Bytes&,
  const std::optional<ForkCheckpointToken>&,std::string_view,size_t,
  const std::vector<CheckpointBlob>&,size_t,
  const std::vector<CheckpointDepositLocatorRecord>&,size_t,bool);
 void append_checkpoint_deposit_locators(rocksdb::WriteBatch&,
  const std::vector<CheckpointDepositLocatorRecord>&,size_t maximum_records,size_t maximum_bytes);
 std::optional<ForkCheckpointRecord> fork_checkpoint_impl(const Int&,const Bytes&,size_t) const;
 std::optional<ExecutionStatePruningInfo> state_pruning_impl() const;
 std::optional<std::string> get(const std::string& key) const;
 std::optional<Json> read(const std::string& key) const;
 std::optional<std::string> proposal_data_impl(const std::string& key) const;
 std::optional<std::string> certified_tip_impl() const;
 std::optional<Block> signing_proposal_impl(const std::string& key,const Int& chain_id,const Bytes& genesis_hash,const std::string& producer,const Int& height,bool required) const;
 std::optional<Json> voting_watermark_impl(const std::string& prefix) const;
 std::optional<VoteLock> voting_lock_impl(const std::string& prefix) const;
 std::optional<BlockCertificate> certificate_impl(const Int& height) const;
 void commit_impl(const Block& block,const State& state,const std::vector<Receipt>& receipts,const std::optional<Json>& consensus,const BlockCertificate* certificate);
 enum class HeaderSelection { Number,Hash,UncertifiedTip };
 std::optional<Header> header_impl(HeaderSelection,const Int&,const Bytes&,size_t,const std::function<void()>&) const;
 void reserve_vote_impl(const FinalityVote& vote,const Int& chain_id,const Bytes& genesis_hash,const VotingSet* validators,const std::vector<FinalityVote>& justification,const Block* proposal=nullptr);
public:
 enum class OpenMode { ReadWrite, ReadOnly, ReadWriteExisting };
 // ReadOnly: offline inspection, no creation, migration, WAL flush or writes.
 // ReadWriteExisting: exclusive maintenance open, no creation or initialization.
 explicit ChainStore(const std::filesystem::path& path,OpenMode mode=OpenMode::ReadWrite);
 ~ChainStore();
 // Authoritative opened database locator; read-only, no layout or DB mutation.
 std::filesystem::path directory() const;
 ChainStore(const ChainStore&)=delete;
 ChainStore& operator=(const ChainStore&)=delete;
 void bind_chain(const Int& chain_id,const Bytes& genesis_hash,const std::string& consensus,const std::string& staking_contract="");
 // Authenticate an existing identity on this handle; never initializes it.
 void require_existing_chain_binding(const Int& chain_id,const Bytes& genesis_hash,const std::string& consensus,const std::string& staking_contract="")const;
 // Opaque checkpoint transport: caller validates the full consensus candidate
 // before committing. Synchronous atomic payload/token replacement with CAS.
 // A failed/uncertain disk write requires reopen and inspection before signing.
 // Separate from execution-block commit; no EVM/consensus atomic binding yet.
 ForkCheckpointToken commit_fork_checkpoint(const Int& chain_id,const Bytes& genesis_hash,
  const std::optional<ForkCheckpointToken>& expected,std::string_view snapshot,size_t maximum_bytes);
 // All fragments and the checkpoint/token share one synchronous CAS batch.
 // Existing content is compared, never overwritten; duplicates are idempotent.
 // maximum_bytes also bounds the entire serialized write batch, not just each
 // fragment. This API does not authorize a peer record or prune graph branches.
 ForkCheckpointToken commit_fork_checkpoint_with_blobs(const Int& chain_id,const Bytes& genesis_hash,
  const std::optional<ForkCheckpointToken>& expected,std::string_view snapshot,size_t maximum_bytes,
  const std::vector<CheckpointBlob>& blobs,size_t maximum_records);
 // Local v4 staging primitive: checkpoint, CAS blobs and locator hints share
 // one synchronous primary batch. The record and aggregate byte budgets are
 // independent; this does not validate consensus or activate a runtime format.
 ForkCheckpointToken commit_fork_checkpoint_with_deposit_blobs(const Int& chain_id,const Bytes& genesis_hash,
  const std::optional<ForkCheckpointToken>& expected,std::string_view snapshot,size_t maximum_bytes,
  const std::vector<CheckpointBlob>& blobs,size_t maximum_records,
  const std::vector<CheckpointDepositLocatorRecord>& locators,size_t maximum_locators);
 // Returns exactly 40 bytes (LE64 ordinal + SHA256 row digest), or nullopt.
 // A well-shaped hint is not ancestry authority. RocksDB may allocate internally.
 std::optional<std::string> checkpoint_deposit_locator(const Int& chain_id,const Bytes& genesis_hash,
  const CheckpointDepositLocatorKey&) const;
 // Digest comes from the independently authenticated manifest, not from disk.
 // Missing fragments return nullopt; corrupt or oversized records fail closed.
 std::optional<std::string> checkpoint_blob(const Int& chain_id,const Bytes& genesis_hash,
  const Bytes& digest,size_t maximum_bytes) const;
 // expected, when supplied, is an independently retained exact commitment. It
 // detects a restored old DB. Without it, local disk rollback is not detectable.
 std::optional<ForkCheckpointRecord> fork_checkpoint(const Int& chain_id,const Bytes& genesis_hash,
  size_t maximum_bytes,const std::optional<ForkCheckpointToken>& expected=std::nullopt) const;
 // Trusted candidate only: caller validates consensus/finality and snapshot.
 // Replace the canonical suffix after common_height and checkpoint in one sync
 // batch. Empty suffix permits votes/ticks or rewind to the common ancestor.
 // Both old token and old execution tip are CAS guards. Genesis must exist.
 // Once coupled, ordinary commit/rewind/checkpoint writers are rejected.
 ForkCheckpointToken commit_execution_checkpoint(const Int& chain_id,const Bytes& genesis_hash,
  const std::optional<ForkCheckpointToken>& expected,const Bytes& expected_tip,
  const Int& common_height,const Bytes& common_hash,
  const std::vector<ExecutionCheckpointBlock>& suffix,std::string_view snapshot,
  size_t maximum_bytes,size_t maximum_blocks,
  const std::vector<CheckpointArchiveEntry>& archive={},
  const std::optional<ExecutionStateRetentionAnchor>& retention=std::nullopt);
 // Atomic execution/checkpoint/immutable-fragment publication. Same execution
 // CAS and retention rules as commit_execution_checkpoint; no new consensus rule.
 ForkCheckpointToken commit_execution_checkpoint_with_blobs(const Int& chain_id,const Bytes& genesis_hash,
  const std::optional<ForkCheckpointToken>& expected,const Bytes& expected_tip,
  const Int& common_height,const Bytes& common_hash,
  const std::vector<ExecutionCheckpointBlock>& suffix,std::string_view snapshot,
  size_t maximum_bytes,size_t maximum_blocks,const std::vector<CheckpointBlob>& blobs,
  size_t maximum_records,const std::vector<CheckpointArchiveEntry>& archive={},
  const std::optional<ExecutionStateRetentionAnchor>& retention=std::nullopt);
 // Same execution/tip/token CAS guards, with typed immutable locator hints
 // included in the execution/checkpoint/blob batch. Legacy API stays unchanged.
 ForkCheckpointToken commit_execution_checkpoint_with_deposit_blobs(const Int& chain_id,const Bytes& genesis_hash,
  const std::optional<ForkCheckpointToken>& expected,const Bytes& expected_tip,
  const Int& common_height,const Bytes& common_hash,const std::vector<ExecutionCheckpointBlock>& suffix,
  std::string_view snapshot,size_t maximum_bytes,size_t maximum_blocks,
  const std::vector<CheckpointBlob>& blobs,size_t maximum_records,
  const std::vector<CheckpointDepositLocatorRecord>& locators,size_t maximum_locators,
  const std::vector<CheckpointArchiveEntry>& archive={},
  const std::optional<ExecutionStateRetentionAnchor>& retention=std::nullopt);
 // Missing historical state is intentional only below this durable boundary.
 std::optional<ExecutionStatePruningInfo> state_pruning() const;
 // Immutable records committed in the same batch as the execution checkpoint.
 // The digest detects corruption, not consensus validity or external rollback.
 std::optional<CheckpointArchiveEntry> checkpoint_archive(const Bytes& root,size_t maximum_bytes) const;
 void rewind(const Int& number);
 // Returns unsigned fields already reserved durably; never authorizes a new vote.
 std::optional<FinalityVote> latest_consensus_vote(const std::string& validator,const Int& chain_id,const Bytes& genesis_hash) const;
 std::optional<Block> voting_proposal(const std::string& validator,uint64_t height,const Bytes& block_hash,const Int& chain_id,const Bytes& genesis_hash) const;
 std::optional<VoteLock> voting_lock(const std::string& validator,const Int& chain_id,const Bytes& genesis_hash) const;
 std::optional<VotingPosition> voting_position(const std::string& validator,const Int& chain_id,const Bytes& genesis_hash) const;
 void advance_voting_height(const std::string& validator,uint64_t height,uint64_t round,const Bytes& block_hash,const std::vector<FinalityVote>& certificate,const VotingSet& current_set,const VotingSet& next_set,const Int& chain_id,const Bytes& genesis_hash);
 void reserve_consensus_vote(const FinalityVote& vote,const VotingSet& validators,const std::vector<FinalityVote>& justification,const Int& chain_id,const Bytes& genesis_hash,const Block* proposal=nullptr);
 void reserve_vote(const FinalityVote& vote,const Int& chain_id,const Bytes& genesis_hash);
 void reserve_signature(const Int& chain_id,const Bytes& genesis_hash,const std::string& producer,const Int& height,const Bytes& digest,const std::optional<Block>& proposal=std::nullopt,std::optional<uint64_t> round=std::nullopt);
 std::optional<Block> signing_proposal(const Int& chain_id,const Bytes& genesis_hash,const std::string& producer,const Int& height,std::optional<uint64_t> round=std::nullopt) const;
 void commit(const Block& block,const State& state,const std::vector<Receipt>& receipts,const std::optional<Json>& consensus=std::nullopt);
 void commit_certified(const Block& block,const State& state,const std::vector<Receipt>& receipts,const Json& consensus,const BlockCertificate& certificate);
 std::optional<BlockCertificate> certificate_at(const Int& height) const;
 std::optional<Block> block_by_number(const Int& number) const;
 std::optional<Block> block_by_hash(const Bytes& hash) const;
 std::optional<Block> latest() const;
 // Derived canonical header index, not body/certificate authentication. A legacy
 // missing index uses a bounded projection; it never writes or repairs records.
 // Snapshot reads, parsing and callbacks run outside the writer mutex. The cap
 // bounds application input, not RocksDB's internal I/O/cache allocation.
 std::optional<Header> header_by_number(const Int&,size_t maximum_serialized_bytes,const std::function<void()>& checkpoint={}) const;
 std::optional<Header> header_by_hash(const Bytes&,size_t maximum_serialized_bytes,const std::function<void()>& checkpoint={}) const;
 // Beacon-only readiness check: reject every legacy certified-chain marker or
 // record. This deliberately does not replace latest()/certificate_at().
 std::optional<Header> uncertified_tip_header(size_t maximum_serialized_bytes,const std::function<void()>& checkpoint={}) const;
 std::optional<State> state_at(const Int& number) const;
 // Pin state and its hash-index membership in one writer-serialized read. Copy
 // and caller checkpoints run after unlocking; no JSON/trie work under mutex.
 // The cap bounds the returned bytes, not RocksDB's internal I/O/cache buffers.
 SerializedExecutionState serialized_state_at(const Int& number,const Bytes& expected_hash,
  size_t maximum_bytes,const std::function<void()>& checkpoint={}) const;
 std::optional<Json> transaction(const Bytes& hash) const;
 std::optional<Receipt> receipt(const Bytes& hash) const;
 std::optional<Json> consensus_at(const Int& number) const;
 Int total_difficulty() const;
 size_t count() const;
};
}
