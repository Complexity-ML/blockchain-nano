#include "nano/consensus_altair_proposer.hpp"
namespace nano {
ConsensusRoot altair_proposer_head(const AltairForkStore& store,const ConsensusRoot& head,
 uint64_t slot,const AltairForkBlockRules& rules,const AltairProposerRules& policy,
 const std::function<void()>& checkpoint){
 auto require=[](bool ok){if(!ok)throw std::invalid_argument("invalid Altair proposer context");};
 const auto& view=store.timed.view;const auto& clock=store.timed.clock;
 const auto slots=rules.transition.epoch.ffg.slots_per_epoch;
 require(slots&&rules.slot_duration_ms&&policy.reorg_cutoff_ms<rules.slot_duration_ms&&
         clock.time_ms>=clock.genesis_time_ms&&view.nodes.contains(head));
 const auto& node=view.nodes.at(head);
 require(view.nodes.contains(node.parent)&&store.timely.contains(head)&&
         store.unrealized_justifications.contains(head)&&store.unrealized_justifications.contains(node.parent)&&
         view.proposer_boost.value_or(ConsensusRoot{})!=head&&slot/slots>=view.finalized.epoch);
 const auto weights=phase0_fork_choice_head(view,{slots,rules.transition.attestation.duties.effective_balance_increment,rules.proposer_score_boost},checkpoint);
 Int total=0;
 for(const auto& [index,v]:view.justified_registry.validators){
  if(checkpoint)checkpoint();
  if(v.activation_epoch<=view.justified_registry.epoch&&view.justified_registry.epoch<v.exit_epoch)
   total+=Int(std::to_string(v.effective_balance));
 }
 total=std::max(total,Int(std::to_string(rules.transition.attestation.duties.effective_balance_increment)));
 const Int committee=total/Int(std::to_string(slots));
 const Int head_threshold=committee*Int(std::to_string(policy.head_weight_threshold))/100;
 const Int parent_threshold=committee*Int(std::to_string(policy.parent_weight_threshold))/100;
 const auto& parent=view.nodes.at(node.parent);
 const bool single_slot=parent.slot<UINT64_MAX&&node.slot<UINT64_MAX&&parent.slot+1==node.slot&&node.slot+1==slot;
 const bool reorg=!store.timely.at(head)&&slot%slots!=0&&
  store.unrealized_justifications.at(head)==store.unrealized_justifications.at(node.parent)&&
  slot/slots-view.finalized.epoch<=policy.maximum_epochs_since_finalization&&
  (clock.time_ms-clock.genesis_time_ms)%rules.slot_duration_ms<=policy.reorg_cutoff_ms&&single_slot&&
  weights.weights.at(head)<head_threshold&&weights.weights.at(node.parent)>parent_threshold;
 return reorg?node.parent:head;
}
}
