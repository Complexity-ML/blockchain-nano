#include "nano/consensus_bellatrix_duty.hpp"
#include "nano/committee.hpp"
#include <algorithm>
namespace nano {
std::optional<BellatrixAttestationDuty> prepare_bellatrix_attestation_duty(
 const BellatrixForkStore& store,const BellatrixForkBlockRules& rules,uint64_t index,
 uint64_t expected_slot,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint){
 const auto& t=rules.transition.consensus;validate_altair_transition_rules(t);
 const BellatrixEpochRules epoch_rules{t.epoch,rules.transition.execution,rules.transition.maximum_state_bytes};
 const auto slots=t.epoch.ffg.slots_per_epoch;const auto& clock=store.timed.clock;const auto& view=store.timed.view;
 if(!rules.slot_duration_ms||rules.attestation_due_ms>rules.slot_duration_ms||clock.time_ms<clock.genesis_time_ms)throw std::invalid_argument("Invalid attestation duty clock");
 const auto slot=(clock.time_ms-clock.genesis_time_ms)/rules.slot_duration_ms,epoch=slot/slots;
 if(view.current_epoch!=epoch)throw std::invalid_argument("Attestation duty epoch mismatch");
 if(slot!=expected_slot||view.equivocating_indices.contains(index))return std::nullopt;
 const auto head=phase0_fork_choice_head(view,{slots,t.attestation.duties.effective_balance_increment,rules.proposer_score_boost},checkpoint).head;
 const auto& block=store.blocks.at(head);validate_bellatrix_execution_record(block,rules,checkpoint);
 if(block.header.slot>slot)throw std::invalid_argument("Future attestation head");
 if(block.header.slot!=slot&&(clock.time_ms-clock.genesis_time_ms)%rules.slot_duration_ms<rules.attestation_due_ms)return std::nullopt;
 const auto head_context=bellatrix_fork_block_context(block.state,block.header,epoch_rules,t.state,checkpoint);
 if(head_context.block_root!=head)throw std::invalid_argument("Attestation head binding mismatch");
 const auto epoch_start=epoch*slots;auto target=head;
 while(view.nodes.at(target).slot>epoch_start){
  if(checkpoint)checkpoint();const auto& node=view.nodes.at(target);
  if(target==view.anchor||!view.nodes.contains(node.parent)||view.nodes.at(node.parent).slot>=node.slot)throw std::invalid_argument("Invalid duty checkpoint ancestry");
  target=node.parent;
 }
 const auto& boundary=store.blocks.at(target);
 const auto context=prepare_bellatrix_fork_state_contexts(boundary.state,boundary.header,{epoch,target},epoch_rules,t.state,maximum_slots,maximum_sync_trials,checkpoint);
 const auto member=context.target.registry.validators.find(index),head_member=head_context.registry.validators.find(index);
 if(member==context.target.registry.validators.end()||head_member==head_context.registry.validators.end())return std::nullopt;
 if(member->second.slashed||head_member->second.slashed||member->second.activation_epoch>epoch||member->second.exit_epoch<=epoch)return std::nullopt;
 if(member->second.public_key!=head_member->second.public_key||context.target.genesis_root!=head_context.genesis_root)throw std::invalid_argument("Inconsistent duty validator identity");
 const auto duties=phase0_attestation_duties(context.target.registry,context.target.randao,t.attestation.duties,epoch,checkpoint);
 if(duties.committees_per_slot>UINT64_MAX/slots)throw std::overflow_error("Duty committee count overflow");
 std::vector<bool> bits;uint64_t committee_index=0;
 for(uint64_t c=0;c<duties.committees_per_slot;++c){
  if(checkpoint)checkpoint();const auto committee=partition_committee(duties.shuffled,(slot%slots)*duties.committees_per_slot+c,duties.committees_per_slot*slots);
  const auto found=std::find(committee.begin(),committee.end(),index);if(found==committee.end())continue;
  if(committee.size()>t.body.validators_per_committee)throw std::invalid_argument("Duty committee exceeds profile bound");
  bits.resize(committee.size());bits[size_t(found-committee.begin())]=true;committee_index=c;break;
 }
 if(bits.empty())return std::nullopt;
 const auto advanced=block.header.slot==slot?block.state:prepare_bellatrix_slots(block.state,epoch_rules,t.state,slot,maximum_slots,maximum_sync_trials,checkpoint);
 const auto source=bellatrix_fork_checkpoints(advanced,epoch_rules,t.state,checkpoint).realized.current_justified;
 if(source.epoch>epoch)throw std::invalid_argument("Future duty source checkpoint");
 const auto& fork=context.target.fork;const auto version=epoch<fork.epoch?fork.previous_version:fork.current_version;
 if(checkpoint)checkpoint();
 return BellatrixAttestationDuty{index,Bytes(member->second.public_key.begin(),member->second.public_key.end()),context.target.genesis_root,version,{{slot,committee_index,head,source,{epoch,target}},std::move(bits),{}}};
}
}
