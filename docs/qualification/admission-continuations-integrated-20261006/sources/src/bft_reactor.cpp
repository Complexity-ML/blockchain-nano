#include "nano/bft_reactor.hpp"
#include "nano/block_auth.hpp"
namespace nano {
namespace {
// Retry rounds must leave time for bounded proposal transfer and validation.
// Saturate before shifting; recovered rounds use the same deterministic delay.
std::chrono::seconds proposal_wait(uint64_t round){return std::chrono::seconds(round>=6?300:5*(uint64_t(1)<<round));}
}
BftReactor::BftReactor(Blockchain& chain,const std::filesystem::path& journal,Bytes key)
 :chain_(chain),journal_(journal),key_(std::move(key)),signer_(public_address(public_key(key_))){
 if(chain.genesis().bft_version!=1)throw std::invalid_argument("reactor requires BFT genesis version 1");
}
void BftReactor::remember_vote(const FinalityVote& vote){
 auto slot=std::make_pair(static_cast<unsigned>(vote.step),address(vote.validator));
 if(outbound_slots_.contains(slot))return;
 // Encode and verify once, after consensus acceptance. Preserve arrival order;
 // duplicate/equivocating votes cannot replace the first validated message.
 auto wire=encode_vote_message(vote,chain_.genesis().chain_id,chain_.genesis().block.header.hash());
 auto [position,inserted]=outbound_slots_.insert(std::move(slot));
 if(!inserted)return;
 try{outbound_.push_back(std::move(wire));}catch(...){outbound_slots_.erase(position);throw;}
}
void BftReactor::ensure_round(ConsensusRound::Clock::time_point now){
 if(round_&&parent_==chain_.tip().header.hash())return;
 round_.reset();outbound_.clear();outbound_slots_.clear();previous_=nullptr;previous_proposal_.reset();parent_=chain_.tip().header.hash();
 // Registration/removal is derived from certified state. A configured key
 // outside that set is an observer, not a failing or unauthorized voter.
 auto members=chain_.voting_set();
 if(std::none_of(members.begin(),members.end(),[&](const auto& member){return address(member.first)==address(signer_);}))return;
 uint64_t number=0;auto saved=journal_.latest_consensus_vote(signer_,chain_.genesis().chain_id,chain_.genesis().block.header.hash());
 if(saved&&saved->height==u64(Int(chain_.tip().header.number+1)))number=saved->round;
 auto timeout=std::chrono::seconds(5);
 round_=std::make_unique<ConsensusRound>(chain_,journal_,key_,number,now,proposal_wait(number),timeout,timeout);
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
 for(const auto& vote:result.broadcast)remember_vote(vote);
 if(result.next_round){auto previous=snapshot_page(false,true);previous.erase("previous");previous["proposal"]=nullptr;auto previous_proposal=round_->proposal();round_->begin_next_round(now,proposal_wait(round_->round()==UINT64_MAX?UINT64_MAX:round_->round()+1));previous_proposal_=std::move(previous_proposal);previous_=std::move(previous);outbound_.clear();outbound_slots_.clear();}
}
Json BftReactor::snapshot()const{
 auto result=snapshot_page();if(result.is_object()&&previous_.is_object()){auto previous=snapshot_page(true);previous.erase("previous");result["previous"]=std::move(previous);}return result;
}
Json BftReactor::snapshot_page(bool previous,bool hashes)const{
 if(!round_||parent_!=chain_.tip().header.hash())return nullptr;
 auto encode=[hashes](const std::optional<Block>& proposal)->Json{
  if(!proposal)return nullptr;
  if(!hashes)return proposal->to_transport_json();
  auto result=proposal->to_json(false);result["transactionEncoding"]="signed-rlp-hashes-v1";
  if(!proposal->uncles.empty()){result["uncleHeaders"]=Json::array();for(const auto& uncle:proposal->uncles)result["uncleHeaders"].push_back(uncle.to_json());}
  return result;
 };
 if(previous&&previous_.is_object()){auto result=previous_;result["proposal"]=encode(previous_proposal_);result["previous"]=nullptr;return result;}
 Json votes=outbound_;
 return {{"version",1},{"height",quantity(Int(chain_.tip().header.number+1))},{"round",quantity(Int(std::to_string(round_->round())))},{"proposal",encode(round_->proposal())},{"votes",votes},{"previous",nullptr}};
}
Json BftReactor::proposal_fragment(const Bytes& block_hash,const Bytes& transaction_hash,uint64_t offset)const{
 constexpr size_t chunk=256*1024;
 if(block_hash.size()!=32||transaction_hash.size()!=32||offset%chunk)throw std::invalid_argument("invalid BFT fragment identity or offset");
 if(!round_||parent_!=chain_.tip().header.hash())throw std::invalid_argument("BFT proposal unavailable");
 const Block* proposal=nullptr;
 for(const auto* candidate:{&round_->proposal(),&previous_proposal_})if(*candidate&&(*candidate)->header.hash()==block_hash){proposal=&**candidate;break;}
 if(!proposal)throw std::invalid_argument("unknown BFT proposal");
 for(const auto& tx:proposal->transactions)if(tx.hash()==transaction_hash){
  auto raw=tx.raw();if(offset>=raw.size())throw std::invalid_argument("BFT fragment offset outside transaction");
  const size_t begin=static_cast<size_t>(offset),end=begin+std::min(chunk,raw.size()-begin);
  return {{"version",1},{"blockHash",hex(block_hash)},{"transactionHash",hex(transaction_hash)},{"offset",quantity(Int(std::to_string(offset)))},{"total",quantity(Int(raw.size()))},{"next",end==raw.size()?Json(nullptr):Json(quantity(Int(end)))},{"data",hex(Bytes(raw.begin()+begin,raw.begin()+end))}};
 }
 throw std::invalid_argument("transaction outside BFT proposal");
}
void BftReactor::validate_manifest(const Json& message,ConsensusRound::Clock::time_point now){
 if(!message.is_object()||message.size()!=6||!message.at("version").is_number_integer()||message.at("version")!=1||!message.at("previous").is_null()||!message.at("votes").is_array()||message.at("votes").size()>2*chain_.voting_set().size())throw std::invalid_argument("invalid BFT proposal manifest");
 if(message.at("height")!=quantity(Int(chain_.tip().header.number+1)))throw std::invalid_argument("BFT manifest height mismatch");
 for(const auto& vote:message.at("votes"))if(!vote.is_string()||vote.get_ref<const std::string&>().size()>2048)throw std::invalid_argument("invalid BFT manifest vote length");
 const auto& proposal=message.at("proposal");if(!proposal.is_object()||proposal.at("transactionEncoding")!="signed-rlp-hashes-v1")throw std::invalid_argument("invalid BFT proposal encoding");
 auto header=Header::from_json(proposal);auto tip=chain_.tip();if(header.number!=tip.header.number+1||header.parent_hash!=tip.header.hash())throw std::invalid_argument("BFT manifest does not extend canonical tip");
 // The producer signature covers full signed transaction bytes, not just the
 // header. Check encoding here; ordinary receive authenticates after assembly.
 auto structure=proposal;structure["transactions"]=Json::array();structure["transactionEncoding"]="signed-rlp-v1";(void)Block::from_json(structure);
 ensure_round(now);if(!round_||message.at("round")!=quantity(Int(std::to_string(round_->round()))))throw std::invalid_argument("BFT manifest round unavailable");
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
  remember_vote(vote);
 }
}
}
