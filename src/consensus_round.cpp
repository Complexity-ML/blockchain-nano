#include "nano/consensus_round.hpp"
#include <limits>
#include "nano/block_auth.hpp"
namespace nano {
ConsensusRound::ConsensusRound(Blockchain& chain,ChainStore& journal,Bytes key,uint64_t round,Clock::time_point start,
 Clock::duration proposal_timeout,Clock::duration prevote_timeout,Clock::duration precommit_timeout)
 :chain_(chain),journal_(journal),key_(std::move(key)),parent_(chain.tip().header.hash()),signer_(public_address(public_key(key_))),round_(round),height_(u64(Int(chain.tip().header.number+1))),
 wait_(chain.voting_set(),chain.genesis().chain_id,chain.genesis().block.header.hash(),height_,round,start,proposal_timeout,prevote_timeout,precommit_timeout),proposal_timeout_(proposal_timeout),prevote_timeout_(prevote_timeout),precommit_timeout_(precommit_timeout){
 auto members=chain.voting_set();if(std::none_of(members.begin(),members.end(),[&](const auto& member){return address(member.first)==address(signer_);}))throw std::invalid_argument("round signer not in parent set");
 auto saved=journal.latest_consensus_vote(signer_,chain.genesis().chain_id,chain.genesis().block.header.hash());
 if(saved&&saved->height>=height_){
  if(saved->height!=height_||saved->round!=round_)throw std::runtime_error("cross-round recovery required for existing local votes");
  resume_pending_=true;
  if(saved->block_hash){proposal_=journal.voting_proposal(signer_,height_,*saved->block_hash,chain.genesis().chain_id,chain.genesis().block.header.hash());if(!proposal_)throw std::runtime_error("reserved proposal unavailable");}
 }
 chain.recover_voting_progress(journal,signer_);
}
void ConsensusRound::check_parent()const{if(chain_.tip().header.hash()!=parent_)throw std::runtime_error("round parent changed");}
void ConsensusRound::propose(const Block& block,const Int& wall_time){
 check_parent();if(committed_)throw std::runtime_error("round already committed");
 chain_.verify_authentication(block);chain_.verify_proposal(block,wall_time);
 if(chain_.genesis().bft_version&&!proposal_&&bft_proposal_round(block)!=round_)throw std::invalid_argument("proposal created outside current round");
 if(proposal_&&proposal_->to_json(true)!=block.to_json(true))throw std::invalid_argument("conflicting proposal in round");
 proposal_=block;
}
VoteReception ConsensusRound::receive(const std::string& wire,Clock::time_point now){check_parent();return wait_.receive(wire,now);}
void ConsensusRound::begin_next_round(Clock::time_point now){
 check_parent();if(committed_||resume_pending_||!wait_.next_round_ready(now))throw std::runtime_error("round transition lacks precommit participation or recovery");
 if(round_==std::numeric_limits<uint64_t>::max())throw std::overflow_error("consensus round exhausted");
 auto locked=journal_.voting_lock(signer_,chain_.genesis().chain_id,chain_.genesis().block.header.hash());
 if(locked&&(locked->height!=height_||locked->validator_set_hash!=voting_set_hash(chain_.voting_set())))throw std::runtime_error("round transition lock domain mismatch");
 std::optional<Block> retained;
 if(locked&&locked->block_hash){retained=journal_.voting_proposal(signer_,height_,*locked->block_hash,chain_.genesis().chain_id,chain_.genesis().block.header.hash());if(!retained)throw std::runtime_error("locked proposal unavailable for next round");}
 auto proof=wait_.prevote_quorum();std::vector<FinalityVote> justification=proof?proof->votes:std::vector<FinalityVote>{};
 RoundWait next(chain_.voting_set(),chain_.genesis().chain_id,chain_.genesis().block.header.hash(),height_,round_+1,now,proposal_timeout_,prevote_timeout_,precommit_timeout_);
 wait_=std::move(next);++round_;proposal_=std::move(retained);justification_=std::move(justification);prevoted_=false;precommitted_=false;
}
ConsensusRound::Progress ConsensusRound::advance(Clock::time_point now,const Int& wall_time){
 Progress result;if(committed_){result.committed=true;return result;}check_parent();
 // Check monotonic time even if an already collected commit can be applied.
 bool expired=wait_.proposal_expired(now);
 auto try_commit=[&](){auto quorum=wait_.commit_quorum();if(!quorum||!proposal_||quorum->block_hash!=proposal_->header.hash())return false;
  chain_.commit_certified_round(journal_,signer_,*proposal_,round_,quorum->votes,wall_time);committed_=true;result.committed=true;return true;};
 if(resume_pending_){
  auto saved=chain_.resume_consensus_vote(journal_,key_,wall_time);
  if(!saved||saved->height!=height_||saved->round!=round_)throw std::runtime_error("reserved vote changed during recovery");
  wait_.receive(encode_vote_message(*saved,chain_.genesis().chain_id,chain_.genesis().block.header.hash()),now);
  prevoted_=true;precommitted_=saved->step==VoteStep::precommit;result.broadcast.push_back(*saved);resume_pending_=false;
 }
 if(try_commit())return result;
 auto publish=[&](FinalityVote vote){wait_.receive(encode_vote_message(vote,chain_.genesis().chain_id,chain_.genesis().block.header.hash()),now);result.broadcast.push_back(std::move(vote));};
 if(!prevoted_&&(proposal_||expired)){
  auto vote=proposal_?chain_.vote_for_block(journal_,key_,*proposal_,round_,VoteStep::prevote,justification_,wall_time):chain_.vote_nil(journal_,key_,round_,VoteStep::prevote,justification_);
  publish(std::move(vote));prevoted_=true;
 }
 if(prevoted_&&!precommitted_&&wait_.prevote_ready(now)){
  auto quorum=wait_.prevote_quorum();
  if(quorum&&quorum->block_hash){
   // A certificate alone cannot authorize a signature without the block data.
   if(proposal_&&quorum->block_hash==proposal_->header.hash()){
    publish(chain_.vote_for_block(journal_,key_,*proposal_,round_,VoteStep::precommit,quorum->votes,wall_time));precommitted_=true;
   }
  }else{
   publish(chain_.vote_nil(journal_,key_,round_,VoteStep::precommit,quorum?quorum->votes:std::vector<FinalityVote>{}));precommitted_=true;
  }
 }
 if(!try_commit())result.next_round=wait_.next_round_ready(now);
 return result;
}
}
