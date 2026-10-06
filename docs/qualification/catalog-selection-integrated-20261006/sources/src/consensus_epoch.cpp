#include "nano/consensus_epoch.hpp"
#include <stdexcept>
namespace nano {
Phase0EpochState prepare_phase0_epoch(const Phase0EpochState& state,const Phase0EpochRules& rules,
 uint64_t expected_epoch,const std::function<void()>& checkpoint){
 const auto& duties=rules.rewards.duties;const auto epoch=state.registry.epoch;
 const auto& queue=rules.registry;const auto& slash=rules.slashing;
 if(!duties.slots_per_epoch||state.ffg.slot==UINT64_MAX||
    (state.ffg.slot+1)%duties.slots_per_epoch||state.ffg.slot/duties.slots_per_epoch!=epoch||
    epoch!=expected_epoch||state.randao.epoch!=epoch||
    (state.last_processed_epoch&&(epoch==0||*state.last_processed_epoch!=epoch-1))||
    rules.history.slots_per_epoch!=duties.slots_per_epoch||state.ffg.block_roots.size()!=rules.history.slots_per_historical_root||
    state.state_roots.size()!=rules.history.slots_per_historical_root||
    state.randao.mixes.size()!=rules.epochs_per_randao_vector||
    rules.rewards.proposer_reward_quotient!=slash.proposer_reward_quotient||
    queue.minimum_churn_limit!=slash.minimum_churn_limit||queue.churn_limit_quotient!=slash.churn_limit_quotient||
    queue.max_seed_lookahead!=slash.max_seed_lookahead||queue.minimum_withdrawability_delay!=slash.minimum_withdrawability_delay||
    queue.maximum_effective_balance!=duties.maximum_effective_balance||rules.effective.maximum!=duties.maximum_effective_balance||
    rules.effective.increment!=duties.effective_balance_increment||slash.effective_balance_increment!=duties.effective_balance_increment||
    !rules.epochs_per_eth1_voting_period||rules.epochs_per_eth1_voting_period>UINT64_MAX/duties.slots_per_epoch||
    state.eth1_votes.size()>rules.epochs_per_eth1_voting_period*duties.slots_per_epoch||
    !rules.maximum_pending_attestations||state.ffg.previous_attestations.size()>rules.maximum_pending_attestations||
    state.ffg.current_attestations.size()>rules.maximum_pending_attestations)
  throw std::invalid_argument("incoherent Phase0 epoch context");
 if(checkpoint)checkpoint();
 auto out=state;
 out.ffg.checkpoints=prepare_phase0_justification_finalization(out.ffg,out.registry,out.randao,duties,checkpoint).checkpoints;
 out.registry=prepare_phase0_rewards(out.ffg,out.registry,out.randao,rules.rewards,checkpoint).registry_after;
 out.registry=prepare_phase0_registry_updates(out.registry,queue,out.ffg.checkpoints.finalized.epoch,checkpoint).registry_after;
 const auto debits=phase0_slashing_penalties(out.registry,slash);
 for(const auto& [index,debit]:debits){if(checkpoint)checkpoint();out.registry.validators.at(index).balance-=debit;}
 if(checkpoint)checkpoint();
 out.eth1_votes=phase0_eth1_votes_after_reset(epoch,out.eth1_votes,rules.epochs_per_eth1_voting_period);
 out.registry=prepare_phase0_effective_balances(out.registry,rules.effective,checkpoint);
 out.registry.slashings=phase0_slashings_after_reset(epoch,out.registry.slashings,slash.epochs_per_slashings_vector);
 out.randao=phase0_randao_after_reset(out.randao,{duties.slots_per_epoch,duties.min_seed_lookahead});
 out.historical_roots=phase0_history_after_update(epoch,out.ffg.block_roots,out.state_roots,out.historical_roots,rules.history,checkpoint);
 out.ffg=phase0_participation_after_rotation(out.ffg);
 out.last_processed_epoch=epoch;
 if(checkpoint)checkpoint();
 return out;
}
}
