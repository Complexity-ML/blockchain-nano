#pragma once
#include "consensus.hpp"
#include "staking_rules.hpp"
#include "engine.hpp"
#include "storage.hpp"
#include "txpool.hpp"
#include <deque>
namespace nano {
struct ChainEvent { uint64_t sequence; std::string type; Json value; };
struct Genesis {
 Int chain_id=1337;
 EvmRevision evm_revision=EvmRevision::NanoExperimental;
 std::string consensus="pow";
 std::string staking_contract;
 unsigned pos_signature_version=0;
 unsigned consensus_rules_version=0;
 unsigned bft_version=0;
 unsigned staking_query_version=0;
 StakingQueryLimits staking_query_limits;
 Int minimum_block_interval=1,maximum_future_drift=15,minimum_gas_limit=5000;
 std::string bootstrap_validator;
 State state;
 Block block;
 static Genesis from_json(const Json& data);
};
// Owns canonical state; durable commit precedes publication to readers.
class Blockchain {
 friend class BellatrixDurableReactor;
 friend class CapellaDurableReactor;
 mutable std::mutex mutex_;
 bool beacon_ready_=false;
 std::optional<Block> beacon_finalized_; // populated only by authenticated coordinator
 void require_execution_ready_locked() const;
 // Process-local proof: unknown on reopen; extended only by verified durable certificates.
 mutable std::optional<Bytes> audited_certified_tip_;
 Json audit_locked() const;
 BlockCertificate import_certified_locked(const Block& block,uint64_t round,const std::vector<FinalityVote>& certificate,const Int& now);
 Genesis genesis_;
 ChainStore store_;
 ExecutionEngine engine_;
 State state_;
 Block tip_;
 TxPool pool_;
 ProofOfStake pos_;
 uint64_t event_sequence_=0;
 std::string event_epoch_;
 std::deque<ChainEvent> events_;
 ProofOfStake sync_consensus(const State& state,const Header& header,const ProofOfStake& previous) const;
 VotingSet voting_set(const ProofOfStake& registry) const;
 std::optional<std::string> selected_validator(uint64_t round=0) const;
 void validate_uncles(const Block& block,const Int& now) const;
 void emit(const std::string& type,Json value);
 struct Prepared {Block block;State state;std::vector<Receipt> receipts;};
 Prepared verify_contents(const Block& block,const Int& now) const;
 Header prepare_header(const std::string& coinbase,const Int& timestamp,uint64_t round) const;
 static Prepared execute_candidate(Block block,const State& initial,std::vector<TxPool::Candidate> pending,const ExecutionEngine& engine,const std::function<void()>& checkpoint,bool reward=true);
 Prepared prepare(const std::string& coinbase,const Int& timestamp,uint64_t round=0,const std::function<void()>& checkpoint={}) const;
 Block produce(const std::string& coinbase,const Int& timestamp,bool pow,std::chrono::milliseconds timeout);
 // Reserved for the authenticated Beacon coordinators: validated candidate, same database.
 // Memory/pool/event preparation precedes the atomic write; readers remain
 // excluded until the canonical head and state have been published.
 ForkCheckpointToken commit_beacon_checkpoint(const std::optional<ForkCheckpointToken>& expected,
  const Bytes& expected_tip,const Int& common_height,const Bytes& common_hash,
  const std::vector<ExecutionCheckpointBlock>& suffix,Block finalized,std::string_view snapshot,
  size_t maximum_bytes,size_t maximum_blocks,const std::vector<CheckpointArchiveEntry>& archive,
  const std::optional<ExecutionStateRetentionAnchor>& retention=std::nullopt);
 ForkCheckpointToken commit_beacon_checkpoint_with_blobs(const std::optional<ForkCheckpointToken>& expected,
  const Bytes& expected_tip,const Int& common_height,const Bytes& common_hash,
  const std::vector<ExecutionCheckpointBlock>& suffix,Block finalized,std::string_view snapshot,
  size_t maximum_bytes,size_t maximum_blocks,const std::vector<CheckpointArchiveEntry>& archive,
  const std::optional<ExecutionStateRetentionAnchor>& retention,const std::vector<CheckpointBlob>& blobs);
public:
 Blockchain(const std::filesystem::path& database,Genesis genesis);
 // Existing owner for stopped-copy maintenance: no schema, identity or Genesis
 // initialization. ReadOnly/unknown modes are refused before opening the DB.
 Blockchain(const std::filesystem::path& database,Genesis genesis,ChainStore::OpenMode mode);
 // RPC must not serve a Beacon database until its checkpoint has been verified.
 void require_execution_ready() const;
 uint64_t event_cursor() const;
 std::string event_epoch() const{return event_epoch_;}
 Json gossip_page(uint64_t cursor,const std::string& epoch,const std::string& after,bool snapshot) const;
 std::vector<ChainEvent> events_since(uint64_t cursor) const;
 Json block_reference_page(uint64_t cursor,const std::string& epoch) const;
 bool restore(const Int& number,const Bytes& expected_hash);
 Block tip() const;
 Header tip_header() const;
 std::optional<Block> finalized_block() const;
 Json audit() const;
 void recover_voting_progress(ChainStore& journal,const std::string& signer) const;
 State state() const;
 bool submit(const SignedTransaction& tx);
 size_t pending_count() const;
 Int pending_nonce(const std::string& addr) const;
 std::optional<SignedTransaction> pending_transaction(const Bytes& hash) const;
 Json pool_info(bool content) const;
 void import_block(const Block& block,const Int& now);
 void commit_certified_round(ChainStore& journal,const std::string& signer,const Block& block,uint64_t round,const std::vector<FinalityVote>& certificate,const Int& now);
 void import_certified_block(const Block& block,uint64_t round,const std::vector<FinalityVote>& certificate,const Int& now);
 void verify_proposal(const Block& block,const Int& now) const;
 void verify_authentication(const Block& block) const;
 void reserve_signature(const Block& block,const std::string& signer);
 Block resume_signing_proposal(const Block& candidate,const std::string& signer) const;
 Block validate_signed(const Bytes& key,const Int& timestamp);
 VotingSet voting_set() const;
 // Re-sign only the latest exact durable reservation after domain/lock/replay checks.
 std::optional<FinalityVote> resume_consensus_vote(ChainStore& journal,const Bytes& key,const Int& now) const;
 FinalityVote vote_nil(ChainStore& journal,const Bytes& key,uint64_t round,VoteStep step,const std::vector<FinalityVote>& justification={}) const;
 FinalityVote vote_for_block(ChainStore& journal,const Bytes& key,const Block& block,uint64_t round,VoteStep step,const std::vector<FinalityVote>& justification,const Int& now) const;
 VotingSet certify_voting_progress(ChainStore& journal,const std::string& signer,const Block& block,uint64_t round,const std::vector<FinalityVote>& certificate,const Int& now) const;
 Json validators() const;
 Int stake(const std::string& addr) const;
 std::optional<std::string> next_validator(uint64_t round=0) const;
 // Read-only candidate after executing eligible pool transactions; not a finality promise.
 struct PendingSnapshot {Header header;std::map<Int,Bytes> ancestors;std::function<std::pair<Header,State>()> execute;};
 PendingSnapshot pending_snapshot(const std::string& coinbase,const Int& now,const std::function<void()>& checkpoint={}) const;
 std::pair<Header,State> pending_execution(const std::string& coinbase,const Int& now,const std::function<void()>& checkpoint={}) const;
 Block block_template(const std::string& coinbase,const Int& timestamp,uint64_t round=0) const;
 std::vector<SignedTransaction> pending_transactions(const Int& gas_limit) const;
 Block validate(const std::string& coinbase,const Int& timestamp);
 Block mine(const std::string& coinbase,const Int& timestamp,std::chrono::milliseconds timeout=std::chrono::seconds(10));
 const ChainStore& store() const {return store_;}
 const Genesis& genesis() const {return genesis_;}
};
}
