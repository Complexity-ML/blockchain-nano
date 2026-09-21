#pragma once
#include "core.hpp"
#include <map>
#include <chrono>
namespace nano {
class ChainStore;
// Primitives only: not connected to the live consensus state machine.
using VotingSet=std::vector<std::pair<std::string,Int>>;
enum class VoteStep : unsigned { prevote=1, precommit=2 };
struct FinalityVote {
 uint64_t height=0,round=0;
 VoteStep step=VoteStep::prevote;
 Bytes validator_set_hash;
 std::optional<Bytes> block_hash; // nullopt is a nil vote, never a commit.
 std::string validator;
 Bytes signature;
};
struct VotingPosition { uint64_t height=0; Bytes validator_set_hash; };
struct BlockCertificate {
 uint64_t round=0;
 VotingSet validators,next_validators;
 std::vector<FinalityVote> votes;
};
Bytes voting_set_hash(const VotingSet& validators);
Bytes vote_signing_hash(const FinalityVote& vote,const Int& chain_id,const Bytes& genesis_hash);
// Caller must enforce durable vote/lock rules before using a real validator key.
void sign_vote(FinalityVote& vote,const Bytes& key,const Int& chain_id,const Bytes& genesis_hash);
// Also requires consensus lock checks by the caller; this guards equivocation only.
void sign_vote_durable(ChainStore& journal,FinalityVote& vote,const Bytes& key,const Int& chain_id,const Bytes& genesis_hash);
void sign_consensus_vote(ChainStore& journal,FinalityVote& vote,const Bytes& key,const VotingSet& validators,
 const std::vector<FinalityVote>& justification,const Int& chain_id,const Bytes& genesis_hash,const Block* proposal=nullptr);
// Flat, versioned wire envelope, bounded before JSON parsing. No voting powers
// are accepted from the sender; callers supply the verified parent set.
std::string encode_vote_message(const FinalityVote& vote,const Int& chain_id,const Bytes& genesis_hash);
FinalityVote decode_vote_message(const std::string& message,const VotingSet& validators,const Int& chain_id,const Bytes& genesis_hash,uint64_t height,uint64_t round);
bool verify_vote(const FinalityVote& vote,const Int& chain_id,const Bytes& genesis_hash);
bool verify_commit_certificate(const std::vector<FinalityVote>& votes,const VotingSet& validators,
 uint64_t height,uint64_t round,const Bytes& block_hash,const Int& chain_id,const Bytes& genesis_hash);
bool verify_prevote_certificate(const std::vector<FinalityVote>& votes,const VotingSet& validators,
 uint64_t height,uint64_t round,const std::optional<Bytes>& block_hash,const Int& chain_id,const Bytes& genesis_hash);
struct VoteQuorum {std::optional<Bytes> block_hash;std::vector<FinalityVote> votes;};
enum class VoteReception { added, duplicate, equivocation };
// One height/round, serial use by the consensus event loop. At most two signed
// votes per validator/step (the original and one conflicting evidence item).
class RoundVoteCollector {
 VotingSet validators_;
 Int chain_id_,total_power_=0;
 Bytes genesis_hash_;
 uint64_t height_,round_;
 std::map<std::string,Int> powers_;
 using Slot=std::pair<unsigned,std::string>;
 std::map<Slot,FinalityVote> votes_,conflicts_;
public:
 RoundVoteCollector(VotingSet validators,Int chain_id,Bytes genesis_hash,uint64_t height,uint64_t round);
 VoteReception receive(const std::string& message);
 bool has_participation_quorum(VoteStep step) const;
 std::optional<VoteQuorum> quorum(VoteStep step) const;
 std::vector<FinalityVote> certificate(VoteStep step,const std::optional<Bytes>& block_hash) const;
 std::vector<std::pair<FinalityVote,FinalityVote>> equivocations() const;
 size_t retained_votes() const {return votes_.size()+conflicts_.size();}
};
// Quorum-gated timing for one height/round, not a signer or an event loop.
class RoundWait {
public:
 using Clock=std::chrono::steady_clock;
private:
 RoundVoteCollector votes_;
 Clock::time_point started_;
 mutable Clock::time_point last_observed_;
 Clock::duration proposal_timeout_,prevote_timeout_,precommit_timeout_;
 std::optional<Clock::time_point> prevote_seen_,precommit_seen_;
 void check_time(Clock::time_point now) const;
public:
 RoundWait(VotingSet validators,Int chain_id,Bytes genesis_hash,uint64_t height,uint64_t round,Clock::time_point started,
  Clock::duration proposal_timeout,Clock::duration prevote_timeout,Clock::duration precommit_timeout);
 VoteReception receive(const std::string& message,Clock::time_point now);
 bool proposal_expired(Clock::time_point now) const;
 bool prevote_ready(Clock::time_point now) const;
 bool next_round_ready(Clock::time_point now) const;
 std::optional<VoteQuorum> prevote_quorum() const {return votes_.quorum(VoteStep::prevote);}
 std::optional<VoteQuorum> commit_quorum() const;
};
struct VoteLock {
 uint64_t height=0;
 Bytes validator_set_hash;
 std::optional<uint64_t> round;
 std::optional<Bytes> block_hash;
};
// Pure transition: caller must atomically persist the returned lock with the vote
// reservation before signing. It does not verify execution of the proposed block.
VoteLock advance_vote_lock(const VoteLock& lock,const FinalityVote& proposed,
 const std::vector<FinalityVote>& justification,const VotingSet& validators,
 const Int& chain_id,const Bytes& genesis_hash);

}
