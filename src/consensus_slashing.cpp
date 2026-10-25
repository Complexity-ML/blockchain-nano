#include "nano/consensus_slashing.hpp"
#include "nano/consensus_exit.hpp"
#include "nano/consensus_registry_epoch.hpp"
#include "consensus_registry_checkpoint.hpp"
#include <algorithm>
#include "nano/consensus_bls.hpp"
#include <set>
#include <type_traits>
namespace nano {
std::optional<std::vector<uint64_t>> verify_attester_slashing(
 const IndexedConsensusAttestation& first,const ConsensusCommittee& first_context,
 const IndexedConsensusAttestation& second,const ConsensusCommittee& second_context){
 return verify_attester_slashing(first,first_context,second,second_context,{});
}
std::optional<std::vector<uint64_t>> verify_attester_slashing(
 const IndexedConsensusAttestation& first,const ConsensusCommittee& first_context,
 const IndexedConsensusAttestation& second,const ConsensusCommittee& second_context,
 const std::function<void()>& checkpoint){
 if(checkpoint)checkpoint();
 if(!is_slashable_attestation_data(first.data,second.data))return std::nullopt;
 if(!first_context.verify(first.indices,consensus_attestation_root(first.data),first.signature,checkpoint)||
    !second_context.verify(second.indices,consensus_attestation_root(second.data),second.signature,checkpoint))return std::nullopt;
 std::vector<uint64_t> common;size_t i=0,j=0;
 while(i<first.indices.size()&&j<second.indices.size()){
  if(checkpoint)checkpoint();
  if(first.indices[i]<second.indices[j])++i;
  else if(second.indices[j]<first.indices[i])++j;
  else{
   if(!first_context.same_validator_key(first.indices[i],second_context))return std::nullopt;
   common.push_back(first.indices[i]);++i;++j;
  }
 }
 if(checkpoint)checkpoint();if(common.empty())return std::nullopt;
 return common;
}

namespace {
uint64_t checked_epoch_add(uint64_t a,uint64_t b){
 if(b>UINT64_MAX-a)throw std::overflow_error("consensus slashing uint64 overflow");
 return a+b;
}
struct ExitQueue {
 uint64_t active=0,latest_exit=0;
 std::map<uint64_t,uint64_t> counts;
};
ExitQueue inspect_exit_registry(const ConsensusSlashingState& state,bool check_lifecycle=true,const std::function<void()>& checkpoint={}){
 ExitQueue queue;std::set<std::array<uint8_t,48>> keys;
 for(const auto& [index,v]:state.validators){
  if(checkpoint)checkpoint();
  if(!keys.insert(v.public_key).second||(check_lifecycle&&(v.activation_epoch>v.exit_epoch||v.exit_epoch>v.withdrawable_epoch)))
   throw std::invalid_argument("invalid slashing registry");
  if(v.activation_epoch<=state.epoch&&state.epoch<v.exit_epoch)++queue.active;
  if(v.exit_epoch!=UINT64_MAX){queue.latest_exit=std::max(queue.latest_exit,v.exit_epoch);++queue.counts[v.exit_epoch];}
 }
 if(checkpoint)checkpoint();return queue;
}
template<class Rules>
void schedule_exit(ConsensusSlashingValidator& v,uint64_t epoch,const Rules& rules,ExitQueue& queue){
 if(v.exit_epoch!=UINT64_MAX)return;
 const auto churn=std::max(rules.minimum_churn_limit,queue.active/rules.churn_limit_quotient);
 auto exit_epoch=std::max(checked_epoch_add(checked_epoch_add(epoch,1),rules.max_seed_lookahead),queue.latest_exit);
 if(queue.counts[exit_epoch]>=churn)exit_epoch=checked_epoch_add(exit_epoch,1);
 if(exit_epoch==UINT64_MAX)throw std::overflow_error("slashing exit epoch sentinel");
 queue.latest_exit=exit_epoch;++queue.counts[exit_epoch];
 v.exit_epoch=exit_epoch;
 v.withdrawable_epoch=checked_epoch_add(exit_epoch,rules.minimum_withdrawability_delay);
}
template<class Verify>
std::optional<std::vector<uint64_t>> process_verified_slashing(
 ConsensusSlashingState& state,const Phase0SlashingRules& rules,
 uint64_t proposer_index,uint64_t whistleblower_index,Verify verify,const std::function<void()>& checkpoint={},bool check_lifecycle=true){
 if(!rules.minimum_churn_limit||!rules.churn_limit_quotient||!rules.epochs_per_slashings_vector||
    !rules.minimum_penalty_quotient||!rules.whistleblower_reward_quotient||!rules.proposer_reward_quotient||
    !rules.effective_balance_increment||!rules.proportional_slashing_multiplier||
    state.slashings.size()!=rules.epochs_per_slashings_vector)
  throw std::invalid_argument("invalid Phase0 slashing configuration");
 if(!state.validators.contains(proposer_index)||!state.validators.contains(whistleblower_index))
  throw std::invalid_argument("missing slashing reward recipient");
 if(checkpoint)checkpoint();
 auto exit_queue=inspect_exit_registry(state,check_lifecycle,checkpoint);
 auto common=verify();if(checkpoint)checkpoint();
 if(!common)return std::nullopt;
 std::vector<uint64_t> eligible;
 for(auto index:*common){
  if(checkpoint)checkpoint();
  auto found=state.validators.find(index);
  if(found==state.validators.end())return std::nullopt;
  const auto& v=found->second;
  if(!v.slashed&&v.activation_epoch<=state.epoch&&state.epoch<v.withdrawable_epoch)eligible.push_back(index);
 }
 if(eligible.empty())return std::nullopt;
 // Stage all changes, including credits, before publishing. Overflow or allocation
 // failure cannot partially punish a subset of the authenticated participants.
 auto next=consensus_registry_detail::copy(state,checkpoint);
 for(auto index:eligible){
  if(checkpoint)checkpoint();
  auto& v=next.validators.at(index);
  schedule_exit(v,state.epoch,rules,exit_queue);
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
 if(checkpoint)checkpoint();
 static_assert(std::is_nothrow_swappable_v<ConsensusSlashingState>);
 using std::swap;swap(state,next);
 return eligible;
}

} // namespace
Phase0RegistryUpdateCandidate prepare_phase0_registry_updates(
 const ConsensusSlashingState& state,const Phase0RegistryUpdateRules& rules,
 uint64_t finalized_epoch,const std::function<void()>& checkpoint){
 if(!rules.minimum_churn_limit||!rules.churn_limit_quotient||!rules.maximum_effective_balance||
    rules.ejection_balance>rules.maximum_effective_balance||finalized_epoch>state.epoch||state.epoch==UINT64_MAX)
  throw std::invalid_argument("invalid Phase0 registry update context");
 if(checkpoint)checkpoint();
 auto queue=inspect_exit_registry(state,true,checkpoint);
 Phase0RegistryUpdateCandidate out{consensus_registry_detail::copy(state,checkpoint),{},{},{}};
 std::vector<std::pair<uint64_t,uint64_t>> activation_queue;
 uint64_t expected_index=0;
 for(auto& [index,v]:out.registry_after.validators){
  if(checkpoint)checkpoint();
  if(index!=expected_index++||v.effective_balance>rules.maximum_effective_balance)
   throw std::invalid_argument("invalid Phase0 registry update validator");
  if(v.activation_eligibility_epoch==UINT64_MAX&&v.effective_balance==rules.maximum_effective_balance){
   const auto epoch=checked_epoch_add(state.epoch,1);
   if(epoch==UINT64_MAX)throw std::overflow_error("activation eligibility epoch sentinel");
   v.activation_eligibility_epoch=epoch;out.newly_eligible.push_back(index);
  }
  if(v.activation_epoch<=state.epoch&&state.epoch<v.exit_epoch&&v.effective_balance<=rules.ejection_balance&&v.exit_epoch==UINT64_MAX){
   schedule_exit(v,state.epoch,rules,queue);out.ejected.push_back(index);
  }
  if(v.activation_eligibility_epoch<=finalized_epoch&&v.activation_epoch==UINT64_MAX)
   activation_queue.emplace_back(v.activation_eligibility_epoch,index);
 }
 const auto churn=std::max(rules.minimum_churn_limit,queue.active/rules.churn_limit_quotient);
 const size_t count=std::min<uint64_t>(activation_queue.size(),churn);
 // Only the first churn entries need sorting; no arbitrary validator cap.
 std::partial_sort(activation_queue.begin(),activation_queue.begin()+count,activation_queue.end(),[&](const auto& a,const auto& b){if(checkpoint)checkpoint();return a<b;});
 if(checkpoint)checkpoint();
 for(size_t i=0;i<count;++i){
  if(checkpoint)checkpoint();
  const auto epoch=checked_epoch_add(checked_epoch_add(state.epoch,1),rules.max_seed_lookahead);
  if(epoch==UINT64_MAX)throw std::overflow_error("activation epoch sentinel");
  const auto index=activation_queue[i].second;
  out.registry_after.validators.at(index).activation_epoch=epoch;out.activated.push_back(index);
 }
 if(checkpoint)checkpoint();
 return out;
}
std::optional<std::vector<uint64_t>> process_phase0_attester_slashing(
 ConsensusSlashingState& state,const Phase0SlashingRules& rules,
 const IndexedConsensusAttestation& first,const ConsensusCommittee& first_context,
 const IndexedConsensusAttestation& second,const ConsensusCommittee& second_context,
 uint64_t proposer_index,uint64_t whistleblower_index){
 return process_phase0_attester_slashing(state,rules,first,first_context,second,second_context,proposer_index,whistleblower_index,{});
}
std::optional<std::vector<uint64_t>> process_phase0_attester_slashing(
 ConsensusSlashingState& state,const Phase0SlashingRules& rules,
 const IndexedConsensusAttestation& first,const ConsensusCommittee& first_context,
 const IndexedConsensusAttestation& second,const ConsensusCommittee& second_context,
 uint64_t proposer_index,uint64_t whistleblower_index,const std::function<void()>& checkpoint){
 return process_verified_slashing(state,rules,proposer_index,whistleblower_index,[&]()->std::optional<std::vector<uint64_t>>{
  auto common=verify_attester_slashing(first,first_context,second,second_context,checkpoint);
  if(!common)return std::nullopt;
  for(auto index:*common){if(checkpoint)checkpoint();auto found=state.validators.find(index);if(found==state.validators.end()||!first_context.matches_validator_key(index,found->second.public_key))return std::nullopt;}
  return common;
 },checkpoint);
}
namespace {
std::optional<std::vector<uint64_t>> registry_attester_slashing_impl(
 ConsensusSlashingState& state,const Phase0SlashingRules& rules,
 const IndexedConsensusAttestation& first,const IndexedConsensusAttestation& second,
 uint64_t maximum_indices,const ConsensusBytes4& first_version,const ConsensusBytes4& second_version,
 const ConsensusRoot& genesis,uint64_t proposer_index,uint64_t whistleblower_index,
 const std::function<void()>& checkpoint,bool check_lifecycle){
 if(!maximum_indices)throw std::invalid_argument("invalid indexed attestation limit");
 return process_verified_slashing(state,rules,proposer_index,whistleblower_index,[&]()->std::optional<std::vector<uint64_t>>{
  if(!is_slashable_attestation_data(first.data,second.data))return std::nullopt;
  auto verify=[&](const IndexedConsensusAttestation& vote,const ConsensusBytes4& version){
   if(vote.indices.empty()||vote.indices.size()>maximum_indices)return false;
   std::vector<Bytes> keys;keys.reserve(vote.indices.size());
   for(size_t i=0;i<vote.indices.size();++i){if(checkpoint)checkpoint();const auto index=vote.indices[i];if(i&&index<=vote.indices[i-1])return false;const auto found=state.validators.find(index);if(found==state.validators.end())return false;const auto& key=found->second.public_key;keys.emplace_back(key.begin(),key.end());}
   const auto root=consensus_signing_root(consensus_attestation_root(vote.data),consensus_domain({1,0,0,0},version,genesis));
   if(checkpoint)checkpoint();
   const auto valid=verify_consensus_bls_fast_aggregate(keys,root,vote.signature);if(checkpoint)checkpoint();return valid;
  };
  if(!verify(first,first_version)||!verify(second,second_version))return std::nullopt;
  std::vector<uint64_t> common;size_t i=0,j=0;
  while(i<first.indices.size()&&j<second.indices.size()){if(checkpoint)checkpoint();if(first.indices[i]<second.indices[j])++i;else if(second.indices[j]<first.indices[i])++j;else{common.push_back(first.indices[i]);++i;++j;}}
  return common;
 },checkpoint,check_lifecycle);
}
} // namespace
std::optional<std::vector<uint64_t>> process_phase0_registry_attester_slashing(
 ConsensusSlashingState& state,const Phase0SlashingRules& rules,
 const IndexedConsensusAttestation& first,const IndexedConsensusAttestation& second,
 uint64_t maximum_indices,const ConsensusBytes4& first_version,const ConsensusBytes4& second_version,
 const ConsensusRoot& genesis,uint64_t proposer_index,uint64_t whistleblower_index,const std::function<void()>& checkpoint){
 return registry_attester_slashing_impl(state,rules,first,second,maximum_indices,first_version,second_version,genesis,proposer_index,whistleblower_index,checkpoint,true);
}
std::optional<Phase0AttesterSlashingCandidate> phase0_attester_slashing_component(
 const ConsensusSlashingState& state,const Phase0SlashingRules& rules,
 const IndexedConsensusAttestation& first,const IndexedConsensusAttestation& second,
 uint64_t maximum_indices,const ConsensusBytes4& first_version,const ConsensusBytes4& second_version,
 const ConsensusRoot& genesis,uint64_t proposer_index,uint64_t whistleblower_index){
 auto next=state;auto indices=registry_attester_slashing_impl(next,rules,first,second,maximum_indices,first_version,second_version,genesis,proposer_index,whistleblower_index,{},false);
 if(!indices)return std::nullopt;return Phase0AttesterSlashingCandidate{std::move(next),std::move(*indices)};
}
bool verify_proposer_slashing(const SignedConsensusBeaconHeader& first,
 const SignedConsensusBeaconHeader& second,std::span<const uint8_t> key,
 const ConsensusBytes4& version,const ConsensusRoot& genesis){
 if(first.message.slot!=second.message.slot||first.message.proposer_index!=second.message.proposer_index||first.message==second.message)return false;
 return verify_consensus_beacon_header(first.message,version,genesis,key,first.signature)&&verify_consensus_beacon_header(second.message,version,genesis,key,second.signature);
}
std::optional<std::vector<uint64_t>> process_phase0_proposer_slashing(
 ConsensusSlashingState& state,const Phase0SlashingRules& rules,
 const SignedConsensusBeaconHeader& first,const SignedConsensusBeaconHeader& second,
 const ConsensusBytes4& version,const ConsensusRoot& genesis,
 uint64_t proposer_index,uint64_t whistleblower_index){
 return process_phase0_proposer_slashing(state,rules,first,second,version,genesis,proposer_index,whistleblower_index,{});
}
std::optional<std::vector<uint64_t>> process_phase0_proposer_slashing(
 ConsensusSlashingState& state,const Phase0SlashingRules& rules,
 const SignedConsensusBeaconHeader& first,const SignedConsensusBeaconHeader& second,
 const ConsensusBytes4& version,const ConsensusRoot& genesis,
 uint64_t proposer_index,uint64_t whistleblower_index,const std::function<void()>& checkpoint){
 return process_verified_slashing(state,rules,proposer_index,whistleblower_index,[&]()->std::optional<std::vector<uint64_t>>{
  const auto found=state.validators.find(first.message.proposer_index);
  if(found==state.validators.end())return std::nullopt;
  if(checkpoint)checkpoint();const auto valid=verify_proposer_slashing(first,second,found->second.public_key,version,genesis);if(checkpoint)checkpoint();if(!valid)return std::nullopt;
  return std::vector<uint64_t>{first.message.proposer_index};
 },checkpoint);
}

ConsensusRoot consensus_voluntary_exit_root(uint64_t epoch,uint64_t index){
 ConsensusRoot epoch_root{},index_root{};
 for(unsigned i=0;i<8;++i){epoch_root[i]=uint8_t(epoch>>(8*i));index_root[i]=uint8_t(index>>(8*i));}
 return consensus_signing_root(epoch_root,index_root);
}
bool process_phase0_voluntary_exit(ConsensusSlashingState& state,const Phase0ExitRules& rules,
 const SignedConsensusVoluntaryExit& exit,const ConsensusBytes4& version,const ConsensusRoot& genesis){
 return process_phase0_voluntary_exit(state,rules,exit,version,genesis,{});
}
bool process_phase0_voluntary_exit(ConsensusSlashingState& state,const Phase0ExitRules& rules,
 const SignedConsensusVoluntaryExit& exit,const ConsensusBytes4& version,const ConsensusRoot& genesis,
 const std::function<void()>& checkpoint){
 if(!rules.minimum_churn_limit||!rules.churn_limit_quotient||!rules.shard_committee_period)
  throw std::invalid_argument("invalid Phase0 voluntary exit configuration");
 if(checkpoint)checkpoint();auto queue=inspect_exit_registry(state,true,checkpoint);
 const auto found=state.validators.find(exit.validator_index);
 if(found==state.validators.end())return false;
 const auto& validator=found->second;
 if(validator.activation_epoch>state.epoch||state.epoch>=validator.exit_epoch||validator.exit_epoch!=UINT64_MAX||
    exit.epoch>state.epoch||state.epoch-validator.activation_epoch<rules.shard_committee_period)return false;
 const auto signing_root=consensus_signing_root(consensus_voluntary_exit_root(exit.epoch,exit.validator_index),consensus_domain({4,0,0,0},version,genesis));
 if(checkpoint)checkpoint();const auto valid=verify_consensus_bls(validator.public_key,signing_root,exit.signature);if(checkpoint)checkpoint();if(!valid)return false;
 auto next=consensus_registry_detail::copy(state,checkpoint);schedule_exit(next.validators.at(exit.validator_index),state.epoch,rules,queue);
 if(checkpoint)checkpoint();
 static_assert(std::is_nothrow_swappable_v<ConsensusSlashingState>);
 using std::swap;swap(state,next);return true;
}

bool process_consensus_voluntary_exit(ConsensusSlashingState& state,const Phase0ExitRules& rules,
 const SignedConsensusVoluntaryExit& exit,const VoluntaryExitDomainContext& context){
 return process_consensus_voluntary_exit(state,rules,exit,context,{});
}
bool process_consensus_voluntary_exit(ConsensusSlashingState& state,const Phase0ExitRules& rules,
 const SignedConsensusVoluntaryExit& exit,const VoluntaryExitDomainContext& context,
 const std::function<void()>& checkpoint){
 if(checkpoint)checkpoint();
 if(context.fork_epoch>state.epoch)throw std::invalid_argument("exit fork state is in the future");
 ConsensusBytes4 version{};
 switch(context.profile){
  case VoluntaryExitProfile::Phase0:version=exit.epoch<context.fork_epoch?context.previous_version:context.current_version;break;
  case VoluntaryExitProfile::Deneb:version=context.capella_version;break;
  default:throw std::invalid_argument("unsupported voluntary exit profile");
 }
 return process_phase0_voluntary_exit(state,rules,exit,version,context.genesis_root,checkpoint);
}

namespace {
Json slashing_rules_json(const Phase0SlashingRules& r){
 if(!r.minimum_churn_limit||!r.churn_limit_quotient||!r.epochs_per_slashings_vector||
    !r.minimum_penalty_quotient||!r.whistleblower_reward_quotient||!r.proposer_reward_quotient||
    !r.effective_balance_increment||!r.proportional_slashing_multiplier)
  throw std::invalid_argument("invalid Phase0 snapshot rules");
 Json values=Json::array();
 for(auto n:{r.minimum_churn_limit,r.churn_limit_quotient,r.max_seed_lookahead,r.minimum_withdrawability_delay,
             r.epochs_per_slashings_vector,r.minimum_penalty_quotient,r.whistleblower_reward_quotient,r.proposer_reward_quotient,
             r.effective_balance_increment,r.proportional_slashing_multiplier})
  values.push_back(quantity(Int(std::to_string(n))));
 return values;
}
uint64_t snapshot_uint64(const Json& value){
 if(!value.is_string())throw std::invalid_argument("snapshot quantity must be a string");
 const auto n=quantity(value);
 const auto result=u64(n);
 if(quantity(n)!=value.get<std::string>())throw std::invalid_argument("noncanonical snapshot quantity");
 return result;
}
}
Json phase0_slashing_snapshot(const ConsensusSlashingState& state,const Phase0SlashingRules& rules){
 const auto encoded_rules=slashing_rules_json(rules);
 if(state.slashings.size()!=rules.epochs_per_slashings_vector)throw std::invalid_argument("snapshot slashing vector length");
 Json entries=Json::array(),slashings=Json::array();std::set<std::array<uint8_t,48>> keys;
 auto q=[](uint64_t n){return quantity(Int(std::to_string(n)));};
 for(const auto& [index,v]:state.validators){
  if(!keys.insert(v.public_key).second||v.activation_epoch>v.exit_epoch||v.exit_epoch>v.withdrawable_epoch)
   throw std::invalid_argument("invalid snapshot validator");
  entries.push_back({{"index",q(index)},{"publicKey",hex(Bytes(v.public_key.begin(),v.public_key.end()))},
   {"balance",q(v.balance)},{"effectiveBalance",q(v.effective_balance)},{"activationEpoch",q(v.activation_epoch)},
   {"withdrawalCredentials",hex(Bytes(v.withdrawal_credentials.begin(),v.withdrawal_credentials.end()))},{"activationEligibilityEpoch",q(v.activation_eligibility_epoch)},
   {"exitEpoch",q(v.exit_epoch)},{"withdrawableEpoch",q(v.withdrawable_epoch)},{"slashed",v.slashed}});
 }
 for(auto amount:state.slashings)slashings.push_back(q(amount));
 return {{"format","nano-phase0-slashing-v3"},{"rules",encoded_rules},{"epoch",q(state.epoch)},{"validators",entries},{"slashings",slashings}};
}
ConsensusSlashingState phase0_slashing_from_snapshot(const Json& snapshot,const Phase0SlashingRules& expected_rules){
 if(!snapshot.is_object()||snapshot.size()!=5||snapshot.at("format")!="nano-phase0-slashing-v3"||
    snapshot.at("rules")!=slashing_rules_json(expected_rules)||!snapshot.at("validators").is_array()||
    !snapshot.at("slashings").is_array()||snapshot.at("slashings").size()!=expected_rules.epochs_per_slashings_vector)
  throw std::invalid_argument("invalid Phase0 slashing snapshot envelope");
 ConsensusSlashingState state;state.epoch=snapshot_uint64(snapshot.at("epoch"));
 std::optional<uint64_t> previous;
 for(const auto& entry:snapshot.at("validators")){
  if(!entry.is_object()||entry.size()!=10||!entry.at("publicKey").is_string()||!entry.at("slashed").is_boolean())
   throw std::invalid_argument("invalid snapshot validator fields");
  auto index=snapshot_uint64(entry.at("index"));
  if(previous&&index<=*previous)throw std::invalid_argument("unordered snapshot validator indices");
  previous=index;const auto key=unhex(entry.at("publicKey"));
  if(key.size()!=48||hex(key)!=entry.at("publicKey").get<std::string>())throw std::invalid_argument("invalid snapshot public key");
  ConsensusSlashingValidator v;std::copy(key.begin(),key.end(),v.public_key.begin());
  const auto credentials=unhex(entry.at("withdrawalCredentials"));
  if(credentials.size()!=32||hex(credentials)!=entry.at("withdrawalCredentials").get<std::string>())throw std::invalid_argument("invalid snapshot withdrawal credentials");
  std::copy(credentials.begin(),credentials.end(),v.withdrawal_credentials.begin());v.activation_eligibility_epoch=snapshot_uint64(entry.at("activationEligibilityEpoch"));
  v.balance=snapshot_uint64(entry.at("balance"));v.effective_balance=snapshot_uint64(entry.at("effectiveBalance"));
  v.activation_epoch=snapshot_uint64(entry.at("activationEpoch"));v.exit_epoch=snapshot_uint64(entry.at("exitEpoch"));
  v.withdrawable_epoch=snapshot_uint64(entry.at("withdrawableEpoch"));v.slashed=entry.at("slashed").get<bool>();
  state.validators.emplace(index,v);
 }
 for(const auto& value:snapshot.at("slashings"))state.slashings.push_back(snapshot_uint64(value));
 // Reuse encoder validation and require the exact canonical representation.
 if(phase0_slashing_snapshot(state,expected_rules)!=snapshot)throw std::invalid_argument("noncanonical slashing snapshot");
 return state;
}

std::map<uint64_t,uint64_t> phase0_slashing_penalties(
 const ConsensusSlashingState& state,const Phase0SlashingRules& rules){
 return phase0_slashing_penalties(state,rules,{});
}
std::map<uint64_t,uint64_t> phase0_slashing_penalties(
 const ConsensusSlashingState& state,const Phase0SlashingRules& rules,const std::function<void()>& checkpoint){
 if(checkpoint)checkpoint();
 slashing_rules_json(rules);
 if(state.slashings.size()!=rules.epochs_per_slashings_vector)
  throw std::invalid_argument("slashing vector mismatch");
 const auto penalty_epoch=checked_epoch_add(state.epoch,rules.epochs_per_slashings_vector/2);
 Int total_active=0,total_slashed=0;
 for(const auto& [index,v]:state.validators){if(checkpoint)checkpoint();
  if(v.activation_epoch<=state.epoch&&state.epoch<v.exit_epoch)total_active+=Int(std::to_string(v.effective_balance));}
 const Int increment(std::to_string(rules.effective_balance_increment));
 if(total_active<increment)total_active=increment;
 for(auto amount:state.slashings){if(checkpoint)checkpoint();total_slashed+=Int(std::to_string(amount));}
 Int adjusted=total_slashed*Int(std::to_string(rules.proportional_slashing_multiplier));
 if(adjusted>total_active)adjusted=total_active;
 std::map<uint64_t,uint64_t> debits;
 for(const auto& [index,v]:state.validators){
  if(checkpoint)checkpoint();
  if(!v.slashed||v.withdrawable_epoch!=penalty_epoch)continue;
  // Keep the prescribed division order; aggregate arithmetic must not wrap.
  const Int units=Int(std::to_string(v.effective_balance))/increment;
  const Int penalty=(units*adjusted/total_active)*increment;
  debits.emplace(index,std::min(v.balance,u64(penalty)));
 }
 if(checkpoint)checkpoint();return debits;
}
std::map<uint64_t,uint64_t> advance_phase0_slashing_epoch(
 ConsensusSlashingState& state,const Phase0SlashingRules& rules,uint64_t expected_epoch){
 return advance_phase0_slashing_epoch(state,rules,expected_epoch,{});
}
std::map<uint64_t,uint64_t> advance_phase0_slashing_epoch(
 ConsensusSlashingState& state,const Phase0SlashingRules& rules,uint64_t expected_epoch,const std::function<void()>& checkpoint){
 if(checkpoint)checkpoint();
 if(state.epoch!=expected_epoch)throw std::invalid_argument("slashing epoch sequence mismatch");
 const auto next_epoch=checked_epoch_add(state.epoch,1);
 std::set<std::array<uint8_t,48>> keys;
 for(const auto& [index,v]:state.validators){if(checkpoint)checkpoint();
  if(!keys.insert(v.public_key).second||v.activation_epoch>v.exit_epoch||v.exit_epoch>v.withdrawable_epoch)
   throw std::invalid_argument("invalid epoch slashing registry");}
 auto debits=phase0_slashing_penalties(state,rules,checkpoint);
 auto next=consensus_registry_detail::copy(state,checkpoint);
 for(const auto& [index,debit]:debits){if(checkpoint)checkpoint();next.validators.at(index).balance-=debit;}
 next.slashings[next_epoch%rules.epochs_per_slashings_vector]=0;
 next.epoch=next_epoch;
 if(checkpoint)checkpoint();
 static_assert(std::is_nothrow_swappable_v<ConsensusSlashingState>);
 using std::swap;swap(state,next);
 return debits;
}
}
