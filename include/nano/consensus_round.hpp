#pragma once
#include "chain.hpp"
namespace nano {
// Serial, single-round coordinator. Caller owns transport and round scheduling.
// Same-round recovery reproduces the latest durable vote; transport must replay
// peer votes. Cross-round recovery still requires a separate controller.
class ConsensusRound {
 Blockchain& chain_;
 ChainStore& journal_;
 Bytes key_,parent_;
 std::string signer_;
 uint64_t round_,height_;
 RoundWait wait_;
 RoundWait::Clock::duration proposal_timeout_,prevote_timeout_,precommit_timeout_;
 std::vector<FinalityVote> justification_;
 std::optional<Block> proposal_;
 bool resume_pending_=false;
 bool prevoted_=false,precommitted_=false,committed_=false;
 void check_parent() const;
public:
 using Clock=RoundWait::Clock;
 struct Progress {std::vector<FinalityVote> broadcast;bool committed=false,next_round=false;};
 ConsensusRound(Blockchain& chain,ChainStore& journal,Bytes key,uint64_t round,Clock::time_point start,
  Clock::duration proposal_timeout,Clock::duration prevote_timeout,Clock::duration precommit_timeout);
 void propose(const Block& block,const Int& wall_time);
 VoteReception receive(const std::string& wire,Clock::time_point now);
 void begin_next_round(Clock::time_point now);
 const std::optional<Block>& proposal() const {return proposal_;}
 uint64_t round() const {return round_;}
 Progress advance(Clock::time_point now,const Int& wall_time);
};
}
