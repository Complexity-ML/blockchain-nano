#pragma once
#include "receipt.hpp"
#include "finality.hpp"
#include <filesystem>
#include <mutex>
namespace rocksdb { class DB; }
namespace nano {
struct ForkCheckpointToken {uint64_t sequence;Bytes digest;bool operator==(const ForkCheckpointToken&)const=default;};
struct ForkCheckpointRecord {ForkCheckpointToken token;std::string snapshot;};
class ChainStore {
 friend class BeaconSigningJournal;
 // Dedicated signing databases serialize all journal users through this gate,
 // including callbacks, independently of the short RocksDB operation mutex.
 mutable std::mutex beacon_signing_mutex_;
 rocksdb::DB* db_=nullptr;
 mutable std::mutex mutex_;
 bool fork_checkpoint_write_uncertain_=false;
 std::optional<ForkCheckpointRecord> fork_checkpoint_impl(const Int&,const Bytes&,size_t) const;
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
 explicit ChainStore(const std::filesystem::path& path);
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
 // expected, when supplied, is an independently retained exact commitment. It
 // detects a restored old DB. Without it, local disk rollback is not detectable.
 std::optional<ForkCheckpointRecord> fork_checkpoint(const Int& chain_id,const Bytes& genesis_hash,
  size_t maximum_bytes,const std::optional<ForkCheckpointToken>& expected=std::nullopt) const;
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
