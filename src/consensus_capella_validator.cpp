#include "nano/consensus_capella_validator.hpp"
#include <set>
#include <algorithm>
#include <chrono>
namespace nano {
namespace {
struct DutyWallTiming {
 using Clock=std::chrono::steady_clock;
 Clock::time_point start=Clock::now();uint64_t& maximum;uint64_t* overruns=nullptr;uint64_t threshold=0;
 ~DutyWallTiming(){const auto elapsed=uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-start).count());maximum=std::max(maximum,elapsed);if(overruns&&elapsed>=threshold)++*overruns;}
};
}

CapellaValidatorLoop::CapellaValidatorLoop(CapellaDurableReactor& reactor,CapellaLocalClock& clock,
 uint64_t slot_ms,uint64_t slots_per_epoch,size_t transaction_trials,size_t vote_trials,std::vector<CapellaValidatorKey> keys,std::function<uint64_t()> now):
 reactor_(reactor),clock_(clock),slot_ms_(slot_ms),slots_per_epoch_(slots_per_epoch),transaction_trials_(transaction_trials),vote_trials_(vote_trials),keys_(std::move(keys)),now_(std::move(now)){
 if(reactor.local_duty_timing()!=std::pair{slot_ms_,slots_per_epoch_})throw std::invalid_argument("Capella scheduler timing differs from consensus");
 if(!reactor.execution_bound()||!slot_ms_||!slots_per_epoch_||!transaction_trials_||!vote_trials_||!now_||keys_.empty())throw std::invalid_argument("Capella validator scheduler configuration");
 std::set<uint64_t> indices;std::set<Bytes> public_keys;std::set<const BeaconSigningJournal*> journals;
 for(const auto& key:keys_){if(!key.journal||!key.signer||(key.registration_public_key.empty()?!indices.insert(key.index).second:(key.registration_public_key.size()!=48||!public_keys.insert(key.registration_public_key).second))||!journals.insert(key.journal.get()).second||key.journal->is_retired())throw std::invalid_argument("Duplicate, retired or incomplete Capella validator key");address(key.fee_recipient);}
}
CapellaValidatorLoop::~CapellaValidatorLoop(){stop();}
void CapellaValidatorLoop::poll(const std::function<void()>& checkpoint){
 std::lock_guard lock(mutex_);if(failed_)throw std::runtime_error("Capella validator loop unavailable");
 ++polls_;DutyWallTiming poll_timing{DutyWallTiming::Clock::now(),poll_max_ms_,&polls_over_slot_,slot_ms_};
 bool cancelled=false;auto check=[&]{if(checkpoint)try{checkpoint();}catch(...){cancelled=true;throw;}};
 try{
  check();clock_.poll(check);auto current=reactor_.current();
  const auto genesis=current.state->timed.clock.genesis_time_ms,now=now_();if(now<genesis)throw std::runtime_error("Validator clock precedes genesis");
  const auto slot=(now-genesis)/slot_ms_,epoch=slot/slots_per_epoch_;
  auto same_slot=[&]{const auto time=now_();if(time<genesis)throw std::runtime_error("Validator clock precedes genesis");return (time-genesis)/slot_ms_==slot;};
  std::set<Bytes> requested;
  for(const auto& key:keys_)if(!key.registration_public_key.empty())requested.insert(key.registration_public_key);
  const auto registered=reactor_.current_validator_indices(requested,check);
  std::vector<CapellaValidatorKey> eligible;std::set<uint64_t> indices;awaiting_registration_=0;
  for(const auto& configured:keys_){auto key=configured;
   if(!key.registration_public_key.empty()){const auto found=registered.find(key.registration_public_key);if(found==registered.end()){++awaiting_registration_;continue;}key.index=found->second;}
   if(!indices.insert(key.index).second)throw std::invalid_argument("Local validator keys resolve to duplicate index");
   eligible.push_back(std::move(key));
  }
  resolved_indices_=std::move(indices);
  if(eligible.empty())return;
  const auto proposer=reactor_.current_proposer(slot,check);
  for(const auto& key:eligible){
   if(!proposer||key.index!=*proposer)continue;
   check();if(!same_slot()){++slot_exits_;return;}
   const auto previous=key.journal->latest_proposal_slot();
   if(!previous||*previous<slot){
    ++proposal_attempts_;DutyWallTiming proposal_timing{DutyWallTiming::Clock::now(),proposal_max_ms_};
    const auto signed_block=reactor_.propose_current(key.index,slot,key.fee_recipient,transaction_trials_,vote_trials_,now_,*key.journal,key.signer,check,key.registration_public_key);
    if(signed_block){if(!reactor_.block(*signed_block,check))throw std::runtime_error("Locally signed Capella proposal refused by admission");++proposals_;}else ++proposals_unavailable_;

   }
  }
  // All locally owned proposals are admitted before choosing attestation heads.
  // Final attestation/sync signing paths already enforce their timing guards.
  // Skip premature duty enumeration/context work using the profile deadline.
  // No sleep under a lock. https://github.com/ethereum/consensus-specs/blob/v1.3.0/specs/altair/validator.md#sync-committee-messages
  const auto duty_time=now_();if(duty_time<genesis)throw std::runtime_error("Validator clock precedes genesis");
  if((duty_time-genesis)/slot_ms_!=slot){++slot_exits_;return;}
  if(!reactor_.current_duties_ready(slot,check)){++duty_waits_;return;}
  const auto attesters=reactor_.current_attesters(slot,check);
  for(const auto& key:eligible){
   if(!attesters.contains(key.index))continue;
   check();if(!same_slot()){++slot_exits_;return;}
   const auto previous=key.journal->latest_attestation_target();if(previous&&*previous>=epoch)continue;
   const auto vote=reactor_.attest_current(key.index,slot,now_,*key.journal,key.signer,check,key.registration_public_key);
   if(vote){record_duty(key.index,*vote);if(!reactor_.vote(*vote,check))throw std::runtime_error("Locally signed Capella attestation refused by admission");++votes_;}
  }
  const auto sync_members=reactor_.current_sync_members(slot,check);
  for(const auto& key:eligible){
   if(!sync_members.contains(key.index))continue;
   check();if(!same_slot()){++slot_exits_;return;}
   if(reactor_.sync_current(key.index,slot,now_,*key.journal,key.signer,check,key.registration_public_key))++sync_messages_;
  }
 }catch(const ExpiredCapellaDuty&){
  if(cancelled)throw;
  ++slot_exits_;return;
 }catch(const StaleCapellaDuty&){
  if(cancelled)throw;
  ++stale_retries_;return;
 }catch(const std::exception& error){if(!cancelled){failed_=true;failure_=error.what();}throw;
 }catch(...){if(!cancelled){failed_=true;failure_="Unknown validator failure";}throw;}

}
void CapellaValidatorLoop::set_duty_trace_limit(size_t limit){
 std::lock_guard lock(mutex_);trace_limit_=limit;while(duty_trace_.size()>limit)duty_trace_.pop_front();
 if(!limit)traced_slot_.reset();
}
void CapellaValidatorLoop::record_duty(uint64_t index,const Phase0SignedAttestation& vote){
 if(!trace_limit_||traced_slot_==vote.data.slot)return;
 // Telemetry must not turn a successfully signed duty into a scheduler failure.
 try{
  const auto current=reactor_.current();const auto& view=current.state->timed.view;
  auto root=[](const ConsensusRoot& r){return hex(Bytes(r.begin(),r.end()));};
  auto cp=[&](const ConsensusCheckpoint& c){return Json{{"epoch",c.epoch},{"root",root(c.root)}};};
  const auto& local=resolved_indices_;
  Json messages=Json::array();
  for(const auto& [member,message]:view.latest_messages)messages.push_back({{"index",member},{"local",local.contains(member)},{"epoch",message.epoch},{"root",root(message.root)}});
  Json entry={{"slot",vote.data.slot},{"validator",index},{"signedHead",root(vote.data.beacon_block_root)},
   {"signedSource",cp(vote.data.source)},{"signedTarget",cp(vote.data.target)},
   {"observedSequence",current.token.sequence},{"observedTimeMs",current.state->timed.clock.time_ms},
   {"observedJustified",cp(view.justified)},{"observedFinalized",cp(view.finalized)},
   {"observedBoost",view.proposer_boost?Json(root(*view.proposer_boost)):Json(nullptr)},{"latestMessages",std::move(messages)}};
  duty_trace_.push_back(std::move(entry));while(duty_trace_.size()>trace_limit_)duty_trace_.pop_front();traced_slot_=vote.data.slot;
 }catch(...){++trace_failures_;/* Consensus admission still follows. */}
}
Json CapellaValidatorLoop::status(bool include_trace){std::lock_guard lock(mutex_);Json result={{"healthy",!failed_},{"localKeys",keys_.size()},{"awaitingRegistration",awaiting_registration_},{"proposalsAdmitted",proposals_},{"attestationsAdmitted",votes_},{"syncMessagesAdmitted",sync_messages_},{"staleDutyRetries",stale_retries_},{"timing",{{"polls",polls_},{"maxPollMs",poll_max_ms_},{"pollsOverSlot",polls_over_slot_},{"proposalAttempts",proposal_attempts_},{"maxProposalMs",proposal_max_ms_},{"proposalsUnavailable",proposals_unavailable_},{"slotBoundaryExits",slot_exits_},{"awaitingSlotBlock",duty_waits_}}},{"failure",failure_.empty()?Json(nullptr):Json(failure_)},{"dutyTraceFailures",trace_failures_}};if(include_trace)result["dutyTrace"]=duty_trace_;return result;}
Json CapellaValidatorLoop::restart_commitments(){
 std::lock_guard lock(mutex_);Json out=Json::array();
 for(const auto& key:keys_){const auto token=key.journal->commitment();Json item={{"index",key.registration_public_key.empty()?Json(quantity(Int(std::to_string(key.index)))):Json(nullptr)},{"restart",{{"sequence",quantity(Int(std::to_string(token.sequence)))},{"digest",hex(token.digest)}}}};if(!key.registration_public_key.empty())item["publicKey"]=hex(key.registration_public_key);out.push_back(std::move(item));}
 return out;
}
void CapellaValidatorLoop::start(std::chrono::milliseconds interval){
 if(interval<=std::chrono::milliseconds::zero())throw std::invalid_argument("Validator poll interval required");
 // Even a configured full-slot polling interval must revisit deferred duties.
 interval=std::min(interval,std::chrono::milliseconds(std::max<uint64_t>(1,reactor_.local_duty_due_ms())));
 std::lock_guard lock(wait_mutex_);if(thread_.joinable()||stopping_)throw std::logic_error("Validator loop already started/stopped");
 thread_=std::thread([this,interval]{std::unique_lock lock(wait_mutex_);while(!wake_.wait_for(lock,interval,[&]{return stopping_;})){lock.unlock();try{poll([this]{std::lock_guard guard(wait_mutex_);if(stopping_)throw std::runtime_error("Validator stopping");});}catch(...){return;}lock.lock();}});
}
void CapellaValidatorLoop::stop(){
 {std::lock_guard lock(wait_mutex_);stopping_=true;}wake_.notify_all();if(thread_.joinable())thread_.join();
}
}
