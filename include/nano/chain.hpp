#pragma once
#include "consensus.hpp"
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
 Int minimum_block_interval=1,maximum_future_drift=15,minimum_gas_limit=5000;
 std::string bootstrap_validator;
 State state;
 Block block;
 static Genesis from_json(const Json& data);
};
// Owns canonical state; durable commit precedes publication to readers.
class Blockchain {
 mutable std::mutex mutex_;
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
 std::deque<ChainEvent> events_;
 ProofOfStake sync_consensus(const State& state,const Header& header,const ProofOfStake& previous) const;
 VotingSet voting_set(const ProofOfStake& registry) const;
 std::optional<std::string> selected_validator(uint64_t round=0) const;
 void validate_uncles(const Block& block,const Int& now) const;
 void emit(const std::string& type,Json value);
 struct Prepared {Block block;State state;std::vector<Receipt> receipts;};
 Prepared verify_contents(const Block& block,const Int& now) const;
 Prepared prepare(const std::string& coinbase,const Int& timestamp,uint64_t round=0) const;
 Block produce(const std::string& coinbase,const Int& timestamp,bool pow,std::chrono::milliseconds timeout);
public:
 Blockchain(const std::filesystem::path& database,Genesis genesis);
 uint64_t event_cursor() const;
 std::vector<ChainEvent> events_since(uint64_t cursor) const;
 bool restore(const Int& number,const Bytes& expected_hash);
 Block tip() const;
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
 Block block_template(const std::string& coinbase,const Int& timestamp,uint64_t round=0) const;
 std::vector<SignedTransaction> pending_transactions(const Int& gas_limit) const;
 Block validate(const std::string& coinbase,const Int& timestamp);
 Block mine(const std::string& coinbase,const Int& timestamp,std::chrono::milliseconds timeout=std::chrono::seconds(10));
 const ChainStore& store() const {return store_;}
 const Genesis& genesis() const {return genesis_;}
};
}
