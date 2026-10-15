#include "nano/consensus_capella_duty.hpp"
#include "nano/committee.hpp"
#include "nano/consensus_capella_store_snapshot.hpp"
#include <algorithm>
namespace nano {
static std::map<uint64_t,CapellaAttestationDuty> attestation_duties_impl(
 const CapellaForkStore& store,const CapellaForkBlockRules& rules,
 uint64_t expected_slot,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint,const CapellaDutyPreparation* prepared){
 const auto& t=rules.transition.consensus;validate_altair_transition_rules(t);
 const CapellaEpochRules epoch_rules{t.epoch,rules.transition.execution,rules.transition.maximum_state_bytes};
 const auto slots=t.epoch.ffg.slots_per_epoch;const auto& clock=store.timed.clock;const auto& view=store.timed.view;
 if(!rules.slot_duration_ms||rules.attestation_due_ms>rules.slot_duration_ms||clock.time_ms<clock.genesis_time_ms)throw std::invalid_argument("Invalid attestation duty clock");
 const auto slot=(clock.time_ms-clock.genesis_time_ms)/rules.slot_duration_ms,epoch=slot/slots;
 if(view.current_epoch!=epoch)throw std::invalid_argument("Attestation duty epoch mismatch");
 if(slot!=expected_slot)return {};
 const auto head=phase0_fork_choice_head(view,{slots,t.attestation.duties.effective_balance_increment,rules.proposer_score_boost},checkpoint).head;
 const auto block_pin=store.blocks.at(head).lease(checkpoint);const auto& block=*block_pin;validate_capella_execution_record(block,rules,checkpoint);
 if(block.header.slot>slot)throw std::invalid_argument("Future attestation head");
 if(block.header.slot!=slot&&(clock.time_ms-clock.genesis_time_ms)%rules.slot_duration_ms<rules.attestation_due_ms)return {};
 const auto head_context=capella_fork_block_context(block.state,block.header,epoch_rules,t.state,checkpoint);
 if(head_context.block_root!=head)throw std::invalid_argument("Attestation head binding mismatch");
 const auto epoch_start=epoch*slots;auto target=head;
 while(view.nodes.at(target).slot>epoch_start){
  if(checkpoint)checkpoint();const auto& node=view.nodes.at(target);
  // A trusted recovery anchor may be later than this epoch's boundary. We
  // cannot invent the missing target root or attest to an unauthenticated
  // ancestor. Defer this duty until a boundary exists in the retained graph.
  if(target==view.anchor)return {};
  if(!view.nodes.contains(node.parent)||view.nodes.at(node.parent).slot>=node.slot)throw std::invalid_argument("Invalid duty checkpoint ancestry");
  target=node.parent;
 }
 const auto boundary_pin=store.blocks.at(target).lease(checkpoint);const auto& boundary=*boundary_pin;
 const auto prepared_target=prepared?prepared->target_context(store,{epoch,target},checkpoint):std::nullopt;
 if(prepared&&!prepared_target)return {};
 const auto context=prepared_target?*prepared_target:prepare_capella_fork_state_contexts(boundary.state,boundary.header,{epoch,target},epoch_rules,t.state,maximum_slots,maximum_sync_trials,checkpoint).target;
 const auto duties=phase0_attestation_duties(context.registry,context.randao,t.attestation.duties,epoch,checkpoint);
 if(duties.committees_per_slot>UINT64_MAX/slots)throw std::overflow_error("Duty committee count overflow");
 const auto advanced=prepared?prepared->head_state():(block.header.slot==slot?block.state:prepare_capella_slots(block.state,epoch_rules,t.state,slot,maximum_slots,maximum_sync_trials,checkpoint));
 const auto source=capella_fork_checkpoints(advanced,epoch_rules,t.state,checkpoint).realized.current_justified;
 if(source.epoch>epoch)throw std::invalid_argument("Future duty source checkpoint");
 const auto& fork=context.fork;const auto version=epoch<fork.epoch?fork.previous_version:fork.current_version;
 if(checkpoint)checkpoint();
 std::map<uint64_t,CapellaAttestationDuty> result;
 for(uint64_t c=0;c<duties.committees_per_slot;++c){
  if(checkpoint)checkpoint();
  const auto committee=partition_committee(duties.shuffled,(slot%slots)*duties.committees_per_slot+c,duties.committees_per_slot*slots);
  if(committee.size()>t.body.validators_per_committee)throw std::invalid_argument("Duty committee exceeds profile bound");
  for(size_t position=0;position<committee.size();++position){
   if(checkpoint)checkpoint();const auto index=committee[position];
   const auto member=context.registry.validators.find(index),head_member=head_context.registry.validators.find(index);
   if(member==context.registry.validators.end()||head_member==head_context.registry.validators.end())continue;
   if(view.equivocating_indices.contains(index)||member->second.slashed||head_member->second.slashed||member->second.activation_epoch>epoch||member->second.exit_epoch<=epoch)continue;
   if(member->second.public_key!=head_member->second.public_key||context.genesis_root!=head_context.genesis_root)throw std::invalid_argument("Inconsistent duty validator identity");
   std::vector<bool> bits(committee.size());bits[position]=true;
   result.emplace(index,CapellaAttestationDuty{index,Bytes(member->second.public_key.begin(),member->second.public_key.end()),context.genesis_root,version,{{slot,c,head,source,{epoch,target}},std::move(bits),{}}});
  }
 }
 return result;
}
std::map<uint64_t,CapellaAttestationDuty> prepare_capella_attestation_duties(
 const CapellaForkStore& store,const CapellaForkBlockRules& rules,uint64_t slot,
 uint64_t maximum_slots,uint64_t trials,const std::function<void()>& check){
 return attestation_duties_impl(store,rules,slot,maximum_slots,trials,check,nullptr);
}
CapellaDutyPreparation::CapellaDutyPreparation(const CapellaForkStore& store,
 const CapellaForkBlockRules& rules,uint64_t slot,const std::function<void()>& check)
 :rules_(rules),slot_(slot),head_(capella_fork_head(store,rules,check)->state,
 capella_fork_head(store,rules,check)->header,
 {rules.transition.consensus.epoch,rules.transition.execution,rules.transition.maximum_state_bytes},rules.transition.consensus.state,check){
 const auto& t=rules_.transition;const auto slots=t.consensus.epoch.ffg.slots_per_epoch;
 if(!slots||slot_==UINT64_MAX||head_.parent().slot>slot_)throw std::invalid_argument("Invalid prepared local duty slot");
 const auto head_pin=store.blocks.at(consensus_beacon_header_root(head_.parent())).lease(check);const auto& head=*head_pin;
 validate_capella_execution_record(head,rules_,check);
 if(head.header.slot<slot_)block_.emplace(head.state,head.header,t,check);
 auto root=consensus_beacon_header_root(head.header);const auto start=(slot_/slots)*slots;
 while(store.timed.view.nodes.at(root).slot>start){
  if(check)check();if(root==store.timed.view.anchor)return;
  const auto& n=store.timed.view.nodes.at(root);
  if(!store.timed.view.nodes.contains(n.parent)||store.timed.view.nodes.at(n.parent).slot>=n.slot)throw std::invalid_argument("Invalid prepared duty ancestry");
  root=n.parent;
 }
 const auto boundary_pin=store.blocks.at(root).lease(check);const auto& boundary=*boundary_pin;
 target_.emplace(store,rules_,ConsensusCheckpoint{slot_/slots,root},check);
}
bool CapellaDutyPreparation::matches(const CapellaForkStore& store,const CapellaForkBlockRules& rules,
 uint64_t slot,const std::function<void()>& check)const{
 if(slot!=slot_||capella_fork_profile(rules)!=capella_fork_profile(rules_))return false;
 if(capella_fork_head(store,rules,check)->header!=head_.parent())return false;
 if(target_&&!target_->matches(store,rules_,target_->target(),check))return false;
 return true;
}
bool CapellaDutyPreparation::retarget(const CapellaForkStore& store,const CapellaForkBlockRules& rules,
 uint64_t slot,const std::function<void()>& check){
 if(slot<slot_||slot==UINT64_MAX||capella_fork_profile(rules)!=capella_fork_profile(rules_))return false;
 const auto head_pin=capella_fork_head(store,rules_,check);const auto& head=*head_pin;if(head.header!=head_.parent())return false;
 const auto& t=rules_.transition;const auto slots=t.consensus.epoch.ffg.slots_per_epoch;
 capella_fork_block_context(head.state,head.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state,check);
 if(projection_blocked_){if(check)check();slot_=slot;return true;}
 auto root=consensus_beacon_header_root(head.header);const auto start=(slot/slots)*slots;
 bool available=true;
 while(store.timed.view.nodes.at(root).slot>start){
  if(check)check();if(root==store.timed.view.anchor){available=false;break;}
  const auto& n=store.timed.view.nodes.at(root);
  if(!store.timed.view.nodes.contains(n.parent)||store.timed.view.nodes.at(n.parent).slot>=n.slot)throw std::invalid_argument("Invalid retarget duty ancestry");
  root=n.parent;
 }
 auto candidate=target_;
 if(!available)candidate.reset();
 else {
  const ConsensusCheckpoint target{slot/slots,root};
  if(!candidate||!candidate->retarget(store,rules_,target,check))candidate.emplace(store,rules_,target,check);
 }
 std::optional<CapellaBlockPreparation> new_block;
 if(!block_&&head.header.slot<slot)new_block.emplace(head.state,head.header,t,check);
 if(check)check();target_=std::move(candidate);if(new_block)block_=std::move(new_block);slot_=slot;return true;
}
void CapellaDutyPreparation::seed_target(const CapellaTargetPreparation& ready,const CapellaForkStore& store,const std::function<void()>& check){
 if(target_&&ready.ready()&&ready.matches(store,rules_,target_->target(),check))target_=ready;
}
bool CapellaDutyPreparation::advance(uint64_t maximum_slots,uint64_t trials,const std::function<void()>& check){
 return advance(maximum_slots,trials,check,nullptr);
}
bool CapellaDutyPreparation::advance(uint64_t maximum_slots,uint64_t trials,const std::function<void()>& check,
 CapellaDutyPreparationWork* actual_work){
 CapellaDutyPreparationWork local;auto& actual=actual_work?*actual_work:local;actual={};
 if(!maximum_slots||!trials)throw std::invalid_argument("invalid Capella duty continuation bounds");
 // A caller's cancellation/error must never masquerade as a missing future
 // committee, even if its callback throws the same exception type.
 bool callback_failed=false;
 auto checkpoint=[&]{if(check)try{check();}catch(...){callback_failed=true;throw;}};
 checkpoint();if(projection_blocked_)return false;
 bool in_slot_work=false;
 try{
  const auto profile=capella_fork_profile(rules_);
  const auto same_slot_profile=[&](const CapellaSlotPreparation& frame){
   auto candidate=rules_;
   candidate.transition.consensus.epoch=frame.rules_.operations;
   candidate.transition.consensus.state=frame.limits_;
   candidate.transition.execution=frame.rules_.execution;
   candidate.transition.maximum_state_bytes=frame.rules_.maximum_state_bytes;
   return capella_fork_profile(candidate)==profile;
  };
  if(!same_slot_profile(head_))throw std::invalid_argument("Capella duty head profile mismatch");
  // Each frame still has its original per-call maximum_slots allowance,
  // including progress adopted rather than recalculated during this call.
  const auto bounded_end=[&](const CapellaSlotPreparation& frame,uint64_t goal){
   const auto start=frame.slot();if(start>goal)throw std::invalid_argument("Backwards Capella duty prefix");
   return start+std::min(maximum_slots,goal-start);
  };
  const auto head_end=bounded_end(head_,slot_);
  uint64_t target_end=0,block_end=0;
  bool share_target=false,share_block=false;
  if(target_){
   const auto slots=rules_.transition.consensus.epoch.ffg.slots_per_epoch;
   if(!slots||target_->target_.epoch>UINT64_MAX/slots)throw std::invalid_argument("Invalid Capella duty target slot");
   target_end=bounded_end(target_->slots_,target_->target_.epoch*slots);
   share_target=target_->slots_.parent()==head_.parent()&&same_slot_profile(target_->slots_)
    &&capella_fork_profile(target_->rules_)==profile;
  }
  if(block_){
   if(!slot_)throw std::invalid_argument("Invalid Capella duty block slot");
   block_end=bounded_end(block_->slots_,slot_-1);
   auto candidate=rules_;candidate.transition=block_->rules_;
   share_block=block_->slots_.parent()==head_.parent()&&same_slot_profile(block_->slots_)
    &&capella_fork_profile(candidate)==profile;
  }
  const auto reuse=[&](CapellaSlotPreparation& destination,bool compatible,uint64_t end,uint64_t& count){
   const auto before=destination.slot(),after=head_.slot();
   if(!compatible||after<=before||after>end)return;
   destination.adopt_same_profile_prefix(head_,end-before,checkpoint);
   count+=after-before;
  };
  const auto share=[&]{
   if(target_)reuse(target_->slots_,share_target,target_end,actual.target_reused_slots);
   if(block_)reuse(block_->slots_,share_block,block_end,actual.block_reused_slots);
  };
  const auto run=[&](CapellaSlotPreparation& frame,uint64_t goal,uint64_t& count){
   const auto before=frame.slot();if(before==goal)return;
   in_slot_work=true;
   const bool reached=frame.advance(goal,goal-before,trials,checkpoint);
   in_slot_work=false;count+=frame.slot()-before;
   if(!reached)throw std::logic_error("Incomplete bounded Capella duty segment");
  };
  // Reuse only actual owned forward states on the identical authenticated
  // parent/profile trajectory. Split at a sibling's boundary before HEAD can
  // pass it: no rewind, invented epoch snapshot or post-state/header constructor.
  share();
  while(head_.slot()<head_end){
   auto end=head_end;const auto start=head_.slot();
   if(target_&&share_target&&target_->slots_.slot()==start&&target_end>start)end=std::min(end,target_end);
   if(block_&&share_block&&block_->slots_.slot()==start&&block_end>start)end=std::min(end,block_end);
   run(head_,end,actual.head_slots);share();
  }
  // Another target ancestor, a seeded frame ahead of HEAD, or progress left
  // behind by an earlier cancellation keeps its independent ordinary path.
  if(target_)run(target_->slots_,target_end,actual.target_slots);
  if(block_)run(block_->slots_,block_end,actual.block_slots);
  return ready();
 }catch(const EmptyAltairSyncActiveSet&){
  actual.incomplete_work_unknown=in_slot_work;
  if(callback_failed)throw;
  projection_blocked_=true;return false;
 }catch(...){
  actual.incomplete_work_unknown=in_slot_work;throw;
 }
}
bool CapellaDutyPreparation::ready()const{
 return !projection_blocked_&&head_.slot()==slot_&&(!target_||target_->ready())&&(!block_||block_->prepared_slot()==slot_-1);
}
const CapellaSlotState& CapellaDutyPreparation::head_state()const{
 if(!ready())throw std::logic_error("Local duty preparation incomplete");return head_.state();
}
CapellaSlotState CapellaDutyPreparation::sync_state(uint64_t trials,const std::function<void()>& check)const{
 if(!ready())throw std::logic_error("Local sync preparation incomplete");auto frame=head_;frame.advance(slot_+1,1,trials,check);return frame.state();
}
std::optional<CapellaSlotState> CapellaDutyPreparation::try_sync_state(uint64_t trials,const std::function<void()>& check)const{
 bool callback_failed=false;
 auto checkpoint=[&]{if(check)try{check();}catch(...){callback_failed=true;throw;}};
 try{return sync_state(trials,checkpoint);}
 catch(const EmptyAltairSyncActiveSet&){if(callback_failed)throw;return std::nullopt;}
}
const CapellaBlockPreparation* CapellaDutyPreparation::block()const{
 if(!ready())throw std::logic_error("Local block preparation incomplete");return block_?&*block_:nullptr;
}
const CapellaTargetPreparation* CapellaDutyPreparation::target_preparation()const{
 if(!ready())throw std::logic_error("Local target preparation incomplete");return target_?&*target_:nullptr;
}
std::optional<Phase0ForkVoteContext> CapellaDutyPreparation::target_context(const CapellaForkStore& store,
 const ConsensusCheckpoint& target,const std::function<void()>& check)const{
 if(!ready()||!target_)return std::nullopt;
 if(target!=target_->target()||!target_->matches(store,rules_,target,check))return std::nullopt;
 return target_->context(store,rules_,check);
}
std::map<uint64_t,CapellaAttestationDuty> CapellaDutyPreparation::duties(const CapellaForkStore& store,const std::function<void()>& check)const{
 if(!ready())return {};
 if(!matches(store,rules_,slot_,check))throw std::invalid_argument("Stale prepared local duties");
 return attestation_duties_impl(store,rules_,slot_,1,1,check,this);
}
std::optional<CapellaAttestationDuty> prepare_capella_attestation_duty(
 const CapellaForkStore& store,const CapellaForkBlockRules& rules,uint64_t index,
 uint64_t slot,uint64_t maximum_slots,uint64_t maximum_sync_trials,const std::function<void()>& checkpoint){
 auto duties=prepare_capella_attestation_duties(store,rules,slot,maximum_slots,maximum_sync_trials,checkpoint);
 auto found=duties.find(index);if(found==duties.end())return std::nullopt;return std::move(found->second);
}
}
