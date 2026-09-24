#include "nano/consensus_slashing.hpp"
#include <set>
#include <type_traits>
namespace nano {
std::optional<std::vector<uint64_t>> verify_attester_slashing(
 const IndexedConsensusAttestation& first,const ConsensusCommittee& first_context,
 const IndexedConsensusAttestation& second,const ConsensusCommittee& second_context){
 if(!is_slashable_attestation_data(first.data,second.data))return std::nullopt;
 if(!first_context.verify(first.indices,consensus_attestation_root(first.data),first.signature)||
    !second_context.verify(second.indices,consensus_attestation_root(second.data),second.signature))return std::nullopt;
 std::vector<uint64_t> common;size_t i=0,j=0;
 while(i<first.indices.size()&&j<second.indices.size()){
  if(first.indices[i]<second.indices[j])++i;
  else if(second.indices[j]<first.indices[i])++j;
  else{
   if(!first_context.same_validator_key(first.indices[i],second_context))return std::nullopt;
   common.push_back(first.indices[i]);++i;++j;
  }
 }
 if(common.empty())return std::nullopt;
 return common;
}

namespace {
uint64_t checked_epoch_add(uint64_t a,uint64_t b){
 if(b>UINT64_MAX-a)throw std::overflow_error("consensus slashing uint64 overflow");
 return a+b;
}
}
std::optional<std::vector<uint64_t>> process_phase0_attester_slashing(
 ConsensusSlashingState& state,const Phase0SlashingRules& rules,
 const IndexedConsensusAttestation& first,const ConsensusCommittee& first_context,
 const IndexedConsensusAttestation& second,const ConsensusCommittee& second_context,
 uint64_t proposer_index,uint64_t whistleblower_index){
 if(!rules.minimum_churn_limit||!rules.churn_limit_quotient||!rules.epochs_per_slashings_vector||
    !rules.minimum_penalty_quotient||!rules.whistleblower_reward_quotient||!rules.proposer_reward_quotient||
    state.slashings.size()!=rules.epochs_per_slashings_vector)
  throw std::invalid_argument("invalid Phase0 slashing configuration");
 if(!state.validators.contains(proposer_index)||!state.validators.contains(whistleblower_index))
  throw std::invalid_argument("missing slashing reward recipient");
 std::set<std::array<uint8_t,48>> keys;
 uint64_t active=0,latest_exit=0;
 std::map<uint64_t,uint64_t> exit_counts;
 for(const auto& [index,v]:state.validators){
  if(!keys.insert(v.public_key).second||v.activation_epoch>v.exit_epoch||v.exit_epoch>v.withdrawable_epoch)
   throw std::invalid_argument("invalid slashing registry");
  if(v.activation_epoch<=state.epoch&&state.epoch<v.exit_epoch)++active;
  if(v.exit_epoch!=UINT64_MAX){latest_exit=std::max(latest_exit,v.exit_epoch);++exit_counts[v.exit_epoch];}
 }
 auto common=verify_attester_slashing(first,first_context,second,second_context);
 if(!common)return std::nullopt;
 std::vector<uint64_t> eligible;
 for(auto index:*common){
  auto found=state.validators.find(index);
  if(found==state.validators.end()||!first_context.matches_validator_key(index,found->second.public_key))
   return std::nullopt;
  const auto& v=found->second;
  if(!v.slashed&&v.activation_epoch<=state.epoch&&state.epoch<v.withdrawable_epoch)eligible.push_back(index);
 }
 if(eligible.empty())return std::nullopt;
 // Stage all changes, including credits, before publishing. Overflow or allocation
 // failure cannot partially punish a subset of the authenticated participants.
 auto next=state;
 const auto churn=std::max(rules.minimum_churn_limit,active/rules.churn_limit_quotient);
 for(auto index:eligible){
  auto& v=next.validators.at(index);
  if(v.exit_epoch==UINT64_MAX){
   uint64_t queue=checked_epoch_add(checked_epoch_add(state.epoch,1),rules.max_seed_lookahead);
   queue=std::max(queue,latest_exit);
   if(exit_counts[queue]>=churn)queue=checked_epoch_add(queue,1);
   // UINT64_MAX is the unset sentinel, never a scheduled exit.
   if(queue==UINT64_MAX)throw std::overflow_error("slashing exit epoch sentinel");
   latest_exit=queue;++exit_counts[queue];
   v.exit_epoch=queue;
   v.withdrawable_epoch=checked_epoch_add(queue,rules.minimum_withdrawability_delay);
  }
  v.slashed=true;
  v.withdrawable_epoch=std::max(v.withdrawable_epoch,checked_epoch_add(state.epoch,rules.epochs_per_slashings_vector));
  auto& bucket=next.slashings.at(state.epoch%rules.epochs_per_slashings_vector);
  bucket=checked_epoch_add(bucket,v.effective_balance);
  v.balance-=std::min(v.balance,v.effective_balance/rules.minimum_penalty_quotient);
  const auto reward=v.effective_balance/rules.whistleblower_reward_quotient;
  const auto proposer_reward=reward/rules.proposer_reward_quotient;
  auto& proposer=next.validators.at(proposer_index).balance;
  proposer=checked_epoch_add(proposer,proposer_reward);
  auto& whistleblower=next.validators.at(whistleblower_index).balance;
  whistleblower=checked_epoch_add(whistleblower,reward-proposer_reward);
 }
 static_assert(std::is_nothrow_swappable_v<ConsensusSlashingState>);
 using std::swap;swap(state,next);
 return eligible;
}
}
