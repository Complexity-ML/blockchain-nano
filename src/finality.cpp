#include "nano/finality.hpp"
#include "nano/crypto.hpp"
#include "nano/storage.hpp"
#include <map>
#include <set>
namespace nano {
namespace {
std::map<std::string,Int> normalized(const VotingSet& validators){
 if(validators.empty()||validators.size()>1000)throw std::invalid_argument("invalid voting set size");
 std::map<std::string,Int> result;
 for(const auto& [raw,power]:validators){auto signer=address(raw);
  if(signer==address("0x0000000000000000000000000000000000000000")||power<=0||power>=(Int(1)<<256)||!result.emplace(signer,power).second)throw std::invalid_argument("invalid or duplicate voting member");
 }
 return result;
}
Bytes domain(const char* text){std::string value(text);return Bytes(value.begin(),value.end());}
}
Bytes voting_set_hash(const VotingSet& validators){
 std::vector<Rlp> entries;for(const auto& [signer,power]:normalized(validators))entries.emplace_back(std::vector<Rlp>{Rlp(unhex(signer)),Rlp(power)});
 return keccak(Rlp(std::vector<Rlp>{Rlp(domain("Nano/BFT/Validators")),Rlp(Int(1)),Rlp(entries)}).encode());
}
Bytes vote_signing_hash(const FinalityVote& vote,const Int& chain_id,const Bytes& genesis_hash){
 if(chain_id<=0||chain_id>=(Int(1)<<256)||genesis_hash.size()!=32||vote.height==0||vote.validator_set_hash.size()!=32||
    (vote.step!=VoteStep::prevote&&vote.step!=VoteStep::precommit)||
    (vote.block_hash&&vote.block_hash->size()!=32))throw std::invalid_argument("invalid finality vote domain");
 return keccak(Rlp(std::vector<Rlp>{Rlp(domain("Nano/BFT/Vote")),Rlp(Int(1)),Rlp(chain_id),Rlp(genesis_hash),
  Rlp(Int(std::to_string(vote.height))),Rlp(Int(std::to_string(vote.round))),Rlp(Int(static_cast<unsigned>(vote.step))),
  Rlp(vote.validator_set_hash),Rlp(vote.block_hash.value_or(Bytes())),Rlp(unhex(address(vote.validator)))}).encode());
}
void sign_vote(FinalityVote& vote,const Bytes& key,const Int& chain_id,const Bytes& genesis_hash){
 if(address(public_address(public_key(key)))!=address(vote.validator))throw std::invalid_argument("vote signer differs from validator");
 auto signature=sign_hash(key,vote_signing_hash(vote,chain_id,genesis_hash));
 vote.signature=concat(bytes(signature.r,32),bytes(signature.s,32));vote.signature.push_back(uint8_t(signature.v));
}
void sign_vote_durable(ChainStore& journal,FinalityVote& vote,const Bytes& key,const Int& chain_id,const Bytes& genesis_hash){
 if(address(public_address(public_key(key)))!=address(vote.validator))throw std::invalid_argument("vote signer differs from validator");
 journal.reserve_vote(vote,chain_id,genesis_hash);
 sign_vote(vote,key,chain_id,genesis_hash);
}
void sign_consensus_vote(ChainStore& journal,FinalityVote& vote,const Bytes& key,const VotingSet& validators,
 const std::vector<FinalityVote>& justification,const Int& chain_id,const Bytes& genesis_hash,const Block* proposal){
 if(address(public_address(public_key(key)))!=address(vote.validator))throw std::invalid_argument("vote signer differs from validator");
 journal.reserve_consensus_vote(vote,validators,justification,chain_id,genesis_hash,proposal);
 sign_vote(vote,key,chain_id,genesis_hash);
}
bool verify_vote(const FinalityVote& vote,const Int& chain_id,const Bytes& genesis_hash){
 try{
  const auto& proof=vote.signature;if(proof.size()!=65||proof[64]>1)return false;
  Signature signature{proof[64],integer(Bytes(proof.begin(),proof.begin()+32)),integer(Bytes(proof.begin()+32,proof.begin()+64))};
  const Int order("0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141");
  if(signature.s<=0||signature.s>order/2)return false;
  return address(public_address(recover_public_key(vote_signing_hash(vote,chain_id,genesis_hash),signature)))==address(vote.validator);
 }catch(const std::exception&){return false;}
}
std::string encode_vote_message(const FinalityVote& vote,const Int& chain_id,const Bytes& genesis_hash){
 if(!verify_vote(vote,chain_id,genesis_hash))throw std::invalid_argument("cannot encode invalid vote");
 Json message={{"type","nano/bft/vote"},{"version",1},{"chainId",quantity(chain_id)},{"genesisHash",hex(genesis_hash)},
 {"height",quantity(Int(std::to_string(vote.height)))},{"round",quantity(Int(std::to_string(vote.round)))},
 {"step",vote.step==VoteStep::prevote?"prevote":"precommit"},{"validatorSetHash",hex(vote.validator_set_hash)},
 {"blockHash",vote.block_hash?Json(hex(*vote.block_hash)):Json(nullptr)},{"validator",address(vote.validator)},{"signature",hex(vote.signature)}};
 return message.dump();
}
FinalityVote decode_vote_message(const std::string& message,const VotingSet& validators,const Int& chain_id,const Bytes& genesis_hash,uint64_t height,uint64_t round){
 if(message.empty()||message.size()>2048)throw std::invalid_argument("vote message exceeds size bound");
 std::set<std::string> keys;
 auto flat=[&](int depth,Json::parse_event_t event,Json& value){
  if(event==Json::parse_event_t::array_start||(event==Json::parse_event_t::object_start&&depth!=0))throw std::invalid_argument("vote envelope must be flat");
  if(event==Json::parse_event_t::key&&!keys.insert(value.get<std::string>()).second)throw std::invalid_argument("duplicate vote field");
  return true;
 };
 auto encoded=Json::parse(message,flat);
 const std::set<std::string> expected{"type","version","chainId","genesisHash","height","round","step","validatorSetHash","blockHash","validator","signature"};
 if(!encoded.is_object()||keys!=expected||encoded.at("type")!="nano/bft/vote"||!encoded.at("version").is_number_integer()||encoded.at("version")!=1)throw std::invalid_argument("unsupported vote envelope");
 auto integer_field=[&](const char* name,size_t width){
  const auto& field=encoded.at(name);if(!field.is_string())throw std::invalid_argument("vote quantity must be hex string");
  const auto text=field.get<std::string>();if(text.size()>2+width*2)throw std::invalid_argument("vote quantity exceeds width");
  auto value=quantity(field);if(value<0||quantity(value)!=text)throw std::invalid_argument("noncanonical vote quantity");return value;
 };
 auto byte_field=[&](const char* name,size_t width){
  const auto& field=encoded.at(name);if(!field.is_string())throw std::invalid_argument("vote bytes must be hex string");
  const auto text=field.get<std::string>();if(text.size()!=2+width*2)throw std::invalid_argument("invalid vote byte width");
  auto value=unhex(text);if(hex(value)!=text)throw std::invalid_argument("noncanonical vote hex");return value;
 };
 if(integer_field("chainId",32)!=chain_id||byte_field("genesisHash",32)!=genesis_hash)throw std::invalid_argument("vote belongs to another chain");
 FinalityVote vote;vote.height=u64(integer_field("height",8));vote.round=u64(integer_field("round",8));
 if(vote.height!=height||vote.round!=round)throw std::invalid_argument("vote outside expected height or round");
 if(encoded.at("step")=="prevote")vote.step=VoteStep::prevote;else if(encoded.at("step")=="precommit")vote.step=VoteStep::precommit;else throw std::invalid_argument("invalid vote step");
 vote.validator_set_hash=byte_field("validatorSetHash",32);
 if(!encoded.at("blockHash").is_null())vote.block_hash=byte_field("blockHash",32);
 vote.validator=hex(byte_field("validator",20));vote.signature=byte_field("signature",65);
 auto members=normalized(validators);
 if(vote.validator_set_hash!=voting_set_hash(validators)||!members.contains(address(vote.validator)))throw std::invalid_argument("vote outside verified validator set");
 if(!verify_vote(vote,chain_id,genesis_hash))throw std::invalid_argument("invalid wire vote signature");
 return vote;
}
static bool verify_quorum(const std::vector<FinalityVote>& votes,const VotingSet& validators,
 uint64_t height,uint64_t round,VoteStep step,const std::optional<Bytes>& block_hash,const Int& chain_id,const Bytes& genesis_hash){
 try{
  if(height==0||(block_hash&&block_hash->size()!=32)||votes.empty()||votes.size()>validators.size())return false;
  auto members=normalized(validators);auto set_hash=voting_set_hash(validators);Int total=0,signed_power=0;
  for(const auto& [_,power]:members)total+=power;
  std::set<std::string> seen;
  for(const auto& vote:votes){
   auto signer=address(vote.validator);auto member=members.find(signer);
   if(member==members.end()||!seen.insert(signer).second||vote.height!=height||vote.round!=round||vote.step!=step||
      vote.validator_set_hash!=set_hash||vote.block_hash!=block_hash||!verify_vote(vote,chain_id,genesis_hash))return false;
   signed_power+=member->second;
  }
  return signed_power*3>total*2;
 }catch(const std::exception&){return false;}
}
bool verify_commit_certificate(const std::vector<FinalityVote>& votes,const VotingSet& validators,
 uint64_t height,uint64_t round,const Bytes& block_hash,const Int& chain_id,const Bytes& genesis_hash){
 if(block_hash.size()!=32)return false;
 return verify_quorum(votes,validators,height,round,VoteStep::precommit,block_hash,chain_id,genesis_hash);
}
bool verify_prevote_certificate(const std::vector<FinalityVote>& votes,const VotingSet& validators,
 uint64_t height,uint64_t round,const std::optional<Bytes>& block_hash,const Int& chain_id,const Bytes& genesis_hash){
 return verify_quorum(votes,validators,height,round,VoteStep::prevote,block_hash,chain_id,genesis_hash);
}
RoundVoteCollector::RoundVoteCollector(VotingSet validators,Int chain_id,Bytes genesis_hash,uint64_t height,uint64_t round)
 :validators_(std::move(validators)),chain_id_(std::move(chain_id)),genesis_hash_(std::move(genesis_hash)),height_(height),round_(round),powers_(normalized(validators_)){
 if(height_==0||chain_id_<=0||chain_id_>=(Int(1)<<256)||genesis_hash_.size()!=32)throw std::invalid_argument("invalid vote collector domain");
 for(const auto& [_,power]:powers_)total_power_+=power;
}
VoteReception RoundVoteCollector::receive(const std::string& message){
 auto vote=decode_vote_message(message,validators_,chain_id_,genesis_hash_,height_,round_);
 Slot slot{static_cast<unsigned>(vote.step),address(vote.validator)};
 auto original=votes_.find(slot);
 if(original==votes_.end()){votes_.emplace(slot,std::move(vote));return VoteReception::added;}
 if(original->second.block_hash==vote.block_hash)return VoteReception::duplicate;
 // Keep the first verified vote counted. Conflicting relays never add weight
 // and cannot replace the original or allocate unlimited evidence.
 conflicts_.try_emplace(slot,std::move(vote));return VoteReception::equivocation;
}
bool RoundVoteCollector::has_participation_quorum(VoteStep step)const{
 if(step!=VoteStep::prevote&&step!=VoteStep::precommit)throw std::invalid_argument("invalid vote collection step");
 Int power=0;for(const auto& [slot,vote]:votes_)if(slot.first==static_cast<unsigned>(step))power+=powers_.at(slot.second);
 return power*3>total_power_*2;
}
std::optional<VoteQuorum> RoundVoteCollector::quorum(VoteStep step)const{
 if(step!=VoteStep::prevote&&step!=VoteStep::precommit)throw std::invalid_argument("invalid quorum step");
 std::map<std::optional<Bytes>,Int> powers;
 for(const auto& [slot,vote]:votes_)if(slot.first==static_cast<unsigned>(step))powers[vote.block_hash]+=powers_.at(slot.second);
 for(const auto& [target,power]:powers)if(power*3>total_power_*2){VoteQuorum result{target,{}};for(const auto& [slot,vote]:votes_)if(slot.first==static_cast<unsigned>(step)&&vote.block_hash==target)result.votes.push_back(vote);return result;}
 return std::nullopt;
}
std::vector<FinalityVote> RoundVoteCollector::certificate(VoteStep step,const std::optional<Bytes>& block_hash)const{
 if((step!=VoteStep::prevote&&step!=VoteStep::precommit)||(block_hash&&block_hash->size()!=32))throw std::invalid_argument("invalid certificate target");
 if(step==VoteStep::precommit&&!block_hash)return {};
 Int power=0;std::vector<FinalityVote> result;
 for(const auto& [slot,vote]:votes_)if(slot.first==static_cast<unsigned>(step)&&vote.block_hash==block_hash){power+=powers_.at(slot.second);result.push_back(vote);}
 if(power*3<=total_power_*2)return {};return result;
}
std::vector<std::pair<FinalityVote,FinalityVote>> RoundVoteCollector::equivocations()const{
 std::vector<std::pair<FinalityVote,FinalityVote>> result;
 for(const auto& [slot,vote]:conflicts_)result.emplace_back(votes_.at(slot),vote);return result;
}
RoundWait::RoundWait(VotingSet validators,Int chain_id,Bytes genesis_hash,uint64_t height,uint64_t round,Clock::time_point started,
 Clock::duration proposal_timeout,Clock::duration prevote_timeout,Clock::duration precommit_timeout)
 :votes_(std::move(validators),std::move(chain_id),std::move(genesis_hash),height,round),started_(started),last_observed_(started),proposal_timeout_(proposal_timeout),prevote_timeout_(prevote_timeout),precommit_timeout_(precommit_timeout){
 if(proposal_timeout<=Clock::duration::zero()||prevote_timeout<=Clock::duration::zero()||precommit_timeout<=Clock::duration::zero())throw std::invalid_argument("round deadlines must be positive");
}
void RoundWait::check_time(Clock::time_point now)const{if(now<last_observed_)throw std::invalid_argument("round clock moved backwards");last_observed_=now;}
VoteReception RoundWait::receive(const std::string& message,Clock::time_point now){
 check_time(now);auto result=votes_.receive(message);
 if(!prevote_seen_&&votes_.has_participation_quorum(VoteStep::prevote))prevote_seen_=now;
 if(!precommit_seen_&&votes_.has_participation_quorum(VoteStep::precommit))precommit_seen_=now;
 return result;
}
bool RoundWait::proposal_expired(Clock::time_point now)const{check_time(now);return now-started_>=proposal_timeout_;}
bool RoundWait::prevote_ready(Clock::time_point now)const{
 check_time(now);return votes_.quorum(VoteStep::prevote).has_value()||(prevote_seen_&&now-*prevote_seen_>=prevote_timeout_);
}
std::optional<VoteQuorum> RoundWait::commit_quorum()const{auto result=votes_.quorum(VoteStep::precommit);if(result&&!result->block_hash)return std::nullopt;return result;}
bool RoundWait::next_round_ready(Clock::time_point now)const{
 check_time(now);auto quorum=votes_.quorum(VoteStep::precommit);
 if(quorum)return !quorum->block_hash; // A block quorum takes priority over any timeout.
 return precommit_seen_&&now-*precommit_seen_>=precommit_timeout_;
}
VoteLock advance_vote_lock(const VoteLock& lock,const FinalityVote& proposed,
 const std::vector<FinalityVote>& justification,const VotingSet& validators,
 const Int& chain_id,const Bytes& genesis_hash){
 vote_signing_hash(proposed,chain_id,genesis_hash);
 auto members=normalized(validators);auto set_hash=voting_set_hash(validators);
 if(lock.height!=proposed.height||lock.validator_set_hash!=set_hash||proposed.validator_set_hash!=set_hash||!members.contains(address(proposed.validator)))throw std::invalid_argument("vote lock domain mismatch");
 if(lock.round.has_value()!=lock.block_hash.has_value()||(lock.block_hash&&lock.block_hash->size()!=32)||(lock.round&&*lock.round>proposed.round))throw std::invalid_argument("invalid or future vote lock");
 auto next=lock;
 bool proof=!justification.empty();uint64_t proof_round=proof?justification.front().round:0;
 auto proof_target=proof?justification.front().block_hash:std::nullopt;
 auto expected_target=proposed.step==VoteStep::prevote?proof_target:proposed.block_hash;
 if(proof&&!verify_prevote_certificate(justification,validators,proposed.height,proof_round,expected_target,chain_id,genesis_hash))throw std::invalid_argument("invalid prevote justification");
 if(proposed.step==VoteStep::prevote){
  if(proof&&proof_round>=proposed.round)throw std::invalid_argument("prevote justification must be from an earlier round");
  // A newer PoLC for another value releases the old lock before selecting the
  // current valid proposal. That proposal need not be the PoLC's value.
  if(lock.block_hash&&proof&&proof_round>*lock.round&&proof_target!=lock.block_hash){next.round.reset();next.block_hash.reset();}
  if(next.block_hash&&next.block_hash!=proposed.block_hash)throw std::invalid_argument("conflicting prevote without newer justification");
 }else{
  if(proof&&proof_round!=proposed.round)throw std::invalid_argument("precommit requires current-round justification");
  if(proposed.block_hash){
   if(!proof)throw std::invalid_argument("block precommit requires prevote quorum");
   next.round=proposed.round;next.block_hash=proposed.block_hash;
  }else if(proof){next.round.reset();next.block_hash.reset();}
 }
 return next;
}

}
