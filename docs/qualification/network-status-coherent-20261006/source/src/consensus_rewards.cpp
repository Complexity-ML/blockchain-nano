#include "nano/consensus_rewards.hpp"
#include "nano/committee.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>
namespace nano {
namespace {
Int amount(uint64_t x){return Int(std::to_string(x));}
uint64_t checked(const Int& x){
 if(x<0||x>amount(UINT64_MAX))throw std::overflow_error("Phase0 reward overflow");
 return u64(x);
}
}
Phase0RewardCandidate prepare_phase0_rewards(const Phase0FfgState& state,
 const ConsensusSlashingState& registry,const ConsensusRandaoState& randao,
 const Phase0RewardRules& rules,const std::function<void()>& checkpoint){
 const auto& d=rules.duties;
 if(!d.slots_per_epoch||!d.effective_balance_increment||!d.maximum_effective_balance||
    d.maximum_effective_balance<d.effective_balance_increment||
    !rules.base_reward_factor||!rules.base_rewards_per_epoch||!rules.proposer_reward_quotient||
    !rules.inactivity_penalty_quotient||state.slot/d.slots_per_epoch!=registry.epoch||randao.epoch!=registry.epoch)
  throw std::invalid_argument("invalid Phase0 reward context");
 if(checkpoint)checkpoint();
 Phase0RewardCandidate out{registry,std::vector<uint64_t>(registry.validators.size()),std::vector<uint64_t>(registry.validators.size()),false};
 if(registry.epoch==0)return out;
 const auto previous=registry.epoch-1;
 if(state.checkpoints.finalized.epoch>previous)throw std::invalid_argument("reward finality from future");
 const auto delay=previous-state.checkpoints.finalized.epoch;
 out.inactivity_leak=delay>rules.minimum_epochs_to_inactivity_penalty;
 const auto duties=phase0_attestation_duties(registry,randao,d,previous,checkpoint);
 const auto count=registry.validators.size();
 Int total=0;
 for(const auto& [i,v]:registry.validators){
  if(checkpoint)checkpoint();
  if(v.effective_balance>d.maximum_effective_balance)throw std::invalid_argument("invalid reward effective balance");
  if(v.activation_epoch<=registry.epoch&&registry.epoch<v.exit_epoch)total+=amount(v.effective_balance);
 }
 total=std::max(total,amount(d.effective_balance_increment));
 Int square_root;mpz_sqrt(square_root.get_mpz_t(),total.get_mpz_t());
 std::vector<Int> base(count),reward(count),penalty(count);
 for(const auto& [i,v]:registry.validators)base[i]=amount(v.effective_balance)*amount(rules.base_reward_factor)/square_root/amount(rules.base_rewards_per_epoch);
 std::array<std::vector<bool>,3> participants{std::vector<bool>(count),std::vector<bool>(count),std::vector<bool>(count)};
 std::vector<const Phase0PendingAttestation*> earliest(count,nullptr);
 auto root=[&](uint64_t slot)->const ConsensusRoot&{
  if(slot>=state.slot||state.slot-slot>state.block_roots.size())throw std::invalid_argument("reward block outside history");
  return state.block_roots[slot%state.block_roots.size()];
 };
 for(const auto& vote:state.previous_attestations){
  if(checkpoint)checkpoint();
  if(vote.data.slot/d.slots_per_epoch!=previous||vote.data.target.epoch!=previous||vote.data.index>=duties.committees_per_slot||
     !vote.inclusion_delay||vote.proposer_index>=count)
   throw std::invalid_argument("invalid reward pending attestation");
  const auto committee=partition_committee(duties.shuffled,(vote.data.slot%d.slots_per_epoch)*duties.committees_per_slot+vote.data.index,d.slots_per_epoch*duties.committees_per_slot);
  if(vote.aggregation_bits.size()!=committee.size())throw std::invalid_argument("reward pending bitlist mismatch");
  const bool target=vote.data.target.root==root(previous*d.slots_per_epoch);
  const bool head=target&&vote.data.beacon_block_root==root(vote.data.slot);
  for(size_t j=0;j<committee.size();++j){
   const auto i=committee[j];if(!vote.aggregation_bits[j]||registry.validators.at(i).slashed)continue;
   participants[0][i]=true;participants[1][i]=participants[1][i]||target;participants[2][i]=participants[2][i]||head;
   // Stable tie: Python min selects the first record at the minimum delay.
   if(!earliest[i]||vote.inclusion_delay<earliest[i]->inclusion_delay)earliest[i]=&vote;
  }
 }
 std::array<Int,3> participating_balance{0,0,0};
 for(const auto& [i,v]:registry.validators)for(size_t part=0;part<3;++part)if(participants[part][i])participating_balance[part]+=amount(v.effective_balance);
 for(auto& balance:participating_balance)balance=std::max(balance,amount(d.effective_balance_increment));
 for(const auto& [i,v]:registry.validators){
  if(checkpoint)checkpoint();
  const Int proposer=base[i]/amount(rules.proposer_reward_quotient);
  if((v.activation_epoch<=previous&&previous<v.exit_epoch)||(v.slashed&&registry.epoch<v.withdrawable_epoch)){
   for(size_t part=0;part<3;++part){
    if(!participants[part][i])penalty[i]+=base[i];
    else if(out.inactivity_leak)reward[i]+=base[i];
    else reward[i]+=base[i]*(participating_balance[part]/amount(d.effective_balance_increment))/(total/amount(d.effective_balance_increment));
   }
   if(out.inactivity_leak){
    penalty[i]+=amount(rules.base_rewards_per_epoch)*base[i]-proposer;
    if(!participants[1][i])penalty[i]+=amount(v.effective_balance)*amount(delay)/amount(rules.inactivity_penalty_quotient);
   }
  }
  if(earliest[i]){
   reward[earliest[i]->proposer_index]+=proposer;
   reward[i]+=(base[i]-proposer)/amount(earliest[i]->inclusion_delay);
  }
 }
 for(auto& [i,v]:out.registry_after.validators){
  if(checkpoint)checkpoint();
  out.rewards[i]=checked(reward[i]);out.penalties[i]=checked(penalty[i]);
  const auto increased=checked(amount(v.balance)+reward[i]);
  v.balance=increased-std::min(increased,out.penalties[i]);
 }
 if(checkpoint)checkpoint();
 return out;
}
}
