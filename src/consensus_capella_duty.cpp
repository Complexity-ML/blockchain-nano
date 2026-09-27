#include "nano/consensus_capella_duty.hpp"
#include "nano/committee.hpp"
#include <algorithm>
namespace nano {
std::map<uint64_t,CapellaAttestationDuty> prepare_capella_attestation_duties(
 const CapellaForkStore& store,const CapellaForkBlockRules& rules,
 uint64_t expected_slot,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint){
 const auto& t=rules.transition.consensus;validate_altair_transition_rules(t);
 const CapellaEpochRules epoch_rules{t.epoch,rules.transition.execution,rules.transition.maximum_state_bytes};
 const auto slots=t.epoch.ffg.slots_per_epoch;const auto& clock=store.timed.clock;const auto& view=store.timed.view;
 if(!rules.slot_duration_ms||rules.attestation_due_ms>rules.slot_duration_ms||clock.time_ms<clock.genesis_time_ms)throw std::invalid_argument("Invalid attestation duty clock");
 const auto slot=(clock.time_ms-clock.genesis_time_ms)/rules.slot_duration_ms,epoch=slot/slots;
 if(view.current_epoch!=epoch)throw std::invalid_argument("Attestation duty epoch mismatch");
 if(slot!=expected_slot)return {};
 const auto head=phase0_fork_choice_head(view,{slots,t.attestation.duties.effective_balance_increment,rules.proposer_score_boost},checkpoint).head;
 const auto& block=store.blocks.at(head);validate_capella_execution_record(block,rules,checkpoint);
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
 const auto& boundary=store.blocks.at(target);
 const auto context=prepare_capella_fork_state_contexts(boundary.state,boundary.header,{epoch,target},epoch_rules,t.state,maximum_slots,maximum_sync_trials,checkpoint);
 const auto duties=phase0_attestation_duties(context.target.registry,context.target.randao,t.attestation.duties,epoch,checkpoint);
 if(duties.committees_per_slot>UINT64_MAX/slots)throw std::overflow_error("Duty committee count overflow");
 const auto advanced=block.header.slot==slot?block.state:prepare_capella_slots(block.state,epoch_rules,t.state,slot,maximum_slots,maximum_sync_trials,checkpoint);
 const auto source=capella_fork_checkpoints(advanced,epoch_rules,t.state,checkpoint).realized.current_justified;
 if(source.epoch>epoch)throw std::invalid_argument("Future duty source checkpoint");
 const auto& fork=context.target.fork;const auto version=epoch<fork.epoch?fork.previous_version:fork.current_version;
 if(checkpoint)checkpoint();
 std::map<uint64_t,CapellaAttestationDuty> result;
 for(uint64_t c=0;c<duties.committees_per_slot;++c){
  if(checkpoint)checkpoint();
  const auto committee=partition_committee(duties.shuffled,(slot%slots)*duties.committees_per_slot+c,duties.committees_per_slot*slots);
  if(committee.size()>t.body.validators_per_committee)throw std::invalid_argument("Duty committee exceeds profile bound");
  for(size_t position=0;position<committee.size();++position){
   if(checkpoint)checkpoint();const auto index=committee[position];
   const auto member=context.target.registry.validators.find(index),head_member=head_context.registry.validators.find(index);
   if(member==context.target.registry.validators.end()||head_member==head_context.registry.validators.end())continue;
   if(view.equivocating_indices.contains(index)||member->second.slashed||head_member->second.slashed||member->second.activation_epoch>epoch||member->second.exit_epoch<=epoch)continue;
   if(member->second.public_key!=head_member->second.public_key||context.target.genesis_root!=head_context.genesis_root)throw std::invalid_argument("Inconsistent duty validator identity");
   std::vector<bool> bits(committee.size());bits[position]=true;
   result.emplace(index,CapellaAttestationDuty{index,Bytes(member->second.public_key.begin(),member->second.public_key.end()),context.target.genesis_root,version,{{slot,c,head,source,{epoch,target}},std::move(bits),{}}});
  }
 }
 return result;
}
std::optional<CapellaAttestationDuty> prepare_capella_attestation_duty(
 const CapellaForkStore& store,const CapellaForkBlockRules& rules,uint64_t index,
 uint64_t slot,uint64_t maximum_slots,uint64_t maximum_sync_trials,const std::function<void()>& checkpoint){
 auto duties=prepare_capella_attestation_duties(store,rules,slot,maximum_slots,maximum_sync_trials,checkpoint);
 auto found=duties.find(index);if(found==duties.end())return std::nullopt;return std::move(found->second);
}
}
