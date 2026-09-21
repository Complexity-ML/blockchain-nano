#include "nano/bft_reactor.hpp"
#include "nano/block_auth.hpp"
namespace nano {
BftReactor::BftReactor(Blockchain& chain,const std::filesystem::path& journal,Bytes key)
 :chain_(chain),journal_(journal),key_(std::move(key)),signer_(public_address(public_key(key_))){
 if(chain.genesis().bft_version!=1)throw std::invalid_argument("reactor requires BFT genesis version 1");
}
void BftReactor::ensure_round(ConsensusRound::Clock::time_point now){
 if(round_&&parent_==chain_.tip().header.hash())return;
 round_.reset();outbound_.clear();previous_=nullptr;parent_=chain_.tip().header.hash();
 // Registration/removal is derived from certified state. A configured key
 // outside that set is an observer, not a failing or unauthorized voter.
 auto members=chain_.voting_set();
 if(std::none_of(members.begin(),members.end(),[&](const auto& member){return address(member.first)==address(signer_);}))return;
 uint64_t number=0;auto saved=journal_.latest_consensus_vote(signer_,chain_.genesis().chain_id,chain_.genesis().block.header.hash());
 if(saved&&saved->height==u64(Int(chain_.tip().header.number+1)))number=saved->round;
 auto timeout=std::chrono::seconds(5);
 round_=std::make_unique<ConsensusRound>(chain_,journal_,key_,number,now,timeout,timeout,timeout);
}
void BftReactor::tick(ConsensusRound::Clock::time_point now,const Int& wall){
 if(wall<=chain_.tip().header.timestamp||wall<chain_.tip().header.timestamp+chain_.genesis().minimum_block_interval)return;
 ensure_round(now);
 if(!round_)return;
 auto selected=chain_.next_validator(round_->round());
 if(!round_->proposal()&&selected&&address(*selected)==address(signer_)){
  auto proposal=chain_.block_template(signer_,wall,round_->round());
  proposal=chain_.resume_signing_proposal(proposal,signer_);
  chain_.reserve_signature(proposal,signer_);sign_block(proposal,key_,chain_.genesis().chain_id,chain_.genesis().block.header.hash());
  round_->propose(proposal,wall);
 }
 auto result=round_->advance(now,wall);
 for(auto& vote:result.broadcast){bool found=false;for(const auto& old:outbound_)if(old.step==vote.step&&address(old.validator)==address(vote.validator))found=true;if(!found)outbound_.push_back(std::move(vote));}
 if(result.next_round){auto previous=snapshot();previous.erase("previous");round_->begin_next_round(now);previous_=std::move(previous);outbound_.clear();}
}
Json BftReactor::snapshot()const{
 if(!round_||parent_!=chain_.tip().header.hash())return nullptr;
 Json votes=Json::array();for(const auto& vote:outbound_)votes.push_back(encode_vote_message(vote,chain_.genesis().chain_id,chain_.genesis().block.header.hash()));
 return {{"version",1},{"height",quantity(Int(chain_.tip().header.number+1))},{"round",quantity(Int(std::to_string(round_->round())))},{"proposal",round_->proposal()?round_->proposal()->to_json(true):Json(nullptr)},{"votes",votes},{"previous",previous_}};
}
void BftReactor::receive(const Json& envelope,ConsensusRound::Clock::time_point now,const Int& wall){
 if(!envelope.is_object()||envelope.size()!=6||!envelope.contains("previous"))throw std::invalid_argument("invalid reactor envelope");
 ensure_round(now);
 if(!round_)return; // Certified block synchronization remains available via RPC.
 const Json* selected=&envelope;
 if(envelope.at("round")!=Json(quantity(Int(std::to_string(round_->round()))))&&envelope.at("previous").is_object())selected=&envelope.at("previous");
 const auto& message=*selected;
 if(!message.is_object()||message.size()!=(selected==&envelope?6:5)||!message.at("version").is_number_integer()||message.at("version")!=1||!message.at("votes").is_array()||message.at("votes").size()>2*chain_.voting_set().size())throw std::invalid_argument("invalid reactor message");
 // Reject stale/future messages before proposal execution or signature work.
 if(!message.at("height").is_string()||message.at("height").get_ref<const std::string&>()!=quantity(Int(chain_.tip().header.number+1)))throw std::invalid_argument("reactor height mismatch");
 ensure_round(now);
 if(!message.at("round").is_string()||message.at("round").get_ref<const std::string&>()!=quantity(Int(std::to_string(round_->round()))))throw std::invalid_argument("reactor round mismatch");
 for(const auto& wire:message.at("votes"))if(!wire.is_string()||wire.get_ref<const std::string&>().size()>2048)throw std::invalid_argument("invalid reactor vote length");
 if(!message.at("proposal").is_null()){
  auto proposal=Block::from_json(message.at("proposal"));
  if(!round_->proposal())round_->propose(proposal,wall);
  else if(round_->proposal()->header.hash()!=proposal.header.hash()){
   // An equivocating proposer can leave honest peers holding different valid
   // proposals. Keep our local proposal/lock, but do not discard their votes:
   // participation is needed to time out and leave the split round.
   chain_.verify_authentication(proposal);chain_.verify_proposal(proposal,wall);
  }
 }
 for(const auto& wire:message.at("votes")){
  const auto& text=wire.get_ref<const std::string&>();
  auto vote=decode_vote_message(text,chain_.voting_set(),chain_.genesis().chain_id,chain_.genesis().block.header.hash(),u64(Int(chain_.tip().header.number+1)),round_->round());
  round_->receive(text,now);
  // Retain only the first validated vote for each member/step. Equivocation
  // evidence stays in the collector and never expands gossip memory.
  bool found=false;for(const auto& old:outbound_)if(old.step==vote.step&&address(old.validator)==address(vote.validator))found=true;
  if(!found)outbound_.push_back(std::move(vote));
 }
}
}
