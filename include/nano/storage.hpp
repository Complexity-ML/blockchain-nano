#pragma once
#include "receipt.hpp"
#include "finality.hpp"
#include <filesystem>
#include <mutex>
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
struct ForkCheckpointRecord {ForkCheckpointToken token;std::string snapshot;};
class ChainStore {
 friend class BeaconSigningJournal;
 friend class AltairDurableReactor;
 friend class BellatrixDurableReactor;
 friend class CapellaDurableReactor;
 mutable std::mutex altair_reactor_mutex_;
 // Dedicated signing databases serialize all journal users through this gate,
 // including callbacks, independently of the short RocksDB operation mutex.
 mutable std::mutex beacon_signing_mutex_;
 rocksdb::DB* db_=nullptr;
 mutable std::mutex mutex_;
 bool fork_checkpoint_write_uncertain_=false;
 void append_checkpoint_blobs(rocksdb::WriteBatch&,const std::vector<CheckpointBlob>&,size_t maximum_records,size_t maximum_bytes);
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
 void reserve_vote_impl(const FinalityVote& vote,const Int& chain_id,const Bytes& genesis_hash,const VotingSet* validators,const std::vector<FinalityVote>& justification,const Block* proposal=nullptr);
public:
 enum class OpenMode { ReadWrite, ReadOnly };
 // Offline inspection only: no creation, migration, WAL flush or writes.
 explicit ChainStore(const std::filesystem::path& path,OpenMode mode=OpenMode::ReadWrite);
 ~ChainStore();
 ChainStore(const ChainStore&)=delete;
 ChainStore& operator=(const ChainStore&)=delete;
 void bind_chain(const Int& chain_id,const Bytes& genesis_hash,const std::string& consensus,const std::string& staking_contract="");
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
 std::optional<State> state_at(const Int& number) const;
 std::optional<Json> transaction(const Bytes& hash) const;
 std::optional<Receipt> receipt(const Bytes& hash) const;
 std::optional<Json> consensus_at(const Int& number) const;
 Int total_difficulty() const;
 size_t count() const;
};
}
