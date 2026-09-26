#include "nano/consensus_bellatrix_validator.hpp"
#include <set>
namespace nano {
BellatrixValidatorLoop::BellatrixValidatorLoop(BellatrixDurableReactor& reactor,BellatrixLocalClock& clock,
 uint64_t slot_ms,uint64_t slots_per_epoch,size_t vote_trials,std::vector<BellatrixValidatorKey> keys,std::function<uint64_t()> now):
 reactor_(reactor),clock_(clock),slot_ms_(slot_ms),slots_per_epoch_(slots_per_epoch),vote_trials_(vote_trials),keys_(std::move(keys)),now_(std::move(now)){
 if(!reactor.execution_bound()||!slot_ms_||!slots_per_epoch_||!vote_trials_||!now_||keys_.empty())throw std::invalid_argument("Bellatrix validator scheduler configuration");
 std::set<uint64_t> indices;std::set<const BeaconSigningJournal*> journals;
 for(const auto& key:keys_){if(!key.journal||!key.signer||!indices.insert(key.index).second||!journals.insert(key.journal.get()).second||key.journal->is_retired())throw std::invalid_argument("Duplicate, retired or incomplete Bellatrix validator key");address(key.fee_recipient);}
}
BellatrixValidatorLoop::~BellatrixValidatorLoop(){stop();}
void BellatrixValidatorLoop::poll(const std::function<void()>& checkpoint){
 std::lock_guard lock(mutex_);if(failed_)throw std::runtime_error("Bellatrix validator loop unavailable");
 bool cancelled=false;auto check=[&]{if(checkpoint)try{checkpoint();}catch(...){cancelled=true;throw;}};
 try{
  check();clock_.poll(check);auto current=reactor_.current();
  const auto genesis=current.state->timed.clock.genesis_time_ms,now=now_();if(now<genesis)throw std::runtime_error("Validator clock precedes genesis");
  const auto slot=(now-genesis)/slot_ms_,epoch=slot/slots_per_epoch_;
  const auto proposer=reactor_.current_proposer(slot,check);
  for(const auto& key:keys_){
   if(!proposer||key.index!=*proposer)continue;
   check();if((now_()-genesis)/slot_ms_!=slot)return;
   const auto previous=key.journal->latest_proposal_slot();
   if(!previous||*previous<slot){
    const auto signed_block=reactor_.propose_current(key.index,slot,key.fee_recipient,vote_trials_,now_,*key.journal,key.signer,check);
    if(signed_block){if(!reactor_.block(*signed_block,check))throw std::runtime_error("Locally signed Bellatrix proposal refused by admission");++proposals_;}

   }
  }
  // All locally owned proposals are admitted before choosing attestation heads.
  const auto attesters=reactor_.current_attesters(slot,check);
  for(const auto& key:keys_){
   if(!attesters.contains(key.index))continue;
   check();if((now_()-genesis)/slot_ms_!=slot)return;
   const auto previous=key.journal->latest_attestation_target();if(previous&&*previous>=epoch)continue;
   const auto vote=reactor_.attest_current(key.index,slot,now_,*key.journal,key.signer,check);
   if(vote){if(!reactor_.vote(*vote,check))throw std::runtime_error("Locally signed Bellatrix attestation refused by admission");++votes_;}
  }
 }catch(const StaleBellatrixDuty&){
  if(cancelled)throw;
  ++stale_retries_;return;
 }catch(const std::exception& error){if(!cancelled){failed_=true;failure_=error.what();}throw;
 }catch(...){if(!cancelled){failed_=true;failure_="Unknown validator failure";}throw;}

}
Json BellatrixValidatorLoop::status(){std::lock_guard lock(mutex_);return {{"healthy",!failed_},{"localKeys",keys_.size()},{"proposalsAdmitted",proposals_},{"attestationsAdmitted",votes_},{"staleDutyRetries",stale_retries_},{"failure",failure_.empty()?Json(nullptr):Json(failure_)}};}
Json BellatrixValidatorLoop::restart_commitments(){
 std::lock_guard lock(mutex_);Json out=Json::array();
 for(const auto& key:keys_){const auto token=key.journal->commitment();out.push_back({{"index",quantity(Int(std::to_string(key.index)))},{"restart",{{"sequence",quantity(Int(std::to_string(token.sequence)))},{"digest",hex(token.digest)}}}});}
 return out;
}
void BellatrixValidatorLoop::start(std::chrono::milliseconds interval){
 if(interval<=std::chrono::milliseconds::zero())throw std::invalid_argument("Validator poll interval required");
 std::lock_guard lock(wait_mutex_);if(thread_.joinable()||stopping_)throw std::logic_error("Validator loop already started/stopped");
 thread_=std::thread([this,interval]{std::unique_lock lock(wait_mutex_);while(!wake_.wait_for(lock,interval,[&]{return stopping_;})){lock.unlock();try{poll([this]{std::lock_guard guard(wait_mutex_);if(stopping_)throw std::runtime_error("Validator stopping");});}catch(...){return;}lock.lock();}});
}
void BellatrixValidatorLoop::stop(){
 {std::lock_guard lock(wait_mutex_);stopping_=true;}wake_.notify_all();if(thread_.joinable())thread_.join();
}
}
