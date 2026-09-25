#include "nano/consensus_altair_snapshot.hpp"
namespace nano {
namespace {
Json numbers(std::initializer_list<uint64_t> values){Json a=Json::array();for(auto n:values)a.push_back(quantity(Int(std::to_string(n))));return a;}
}
Json altair_transition_profile(const AltairTransitionRules& r){
 const auto& e=r.epoch;const auto& d=r.attestation.duties;const auto& q=e.registry;const auto& s=e.slashing;const auto& h=e.history;const auto& l=r.state;const auto& b=r.body;
 return Json{
  {"ffg",numbers({e.ffg.slots_per_epoch,e.ffg.effective_balance_increment})},
  {"inactivity",numbers({e.inactivity.slots_per_epoch,e.inactivity.score_bias,e.inactivity.score_recovery_rate,e.inactivity.minimum_epochs_to_inactivity_penalty})},
  {"epochSync",numbers({e.sync.slots_per_epoch,e.sync.epochs_per_sync_committee_period,e.sync.min_seed_lookahead,e.sync.maximum_effective_balance,e.sync.shuffle_rounds})},
  {"rewardWeights",numbers({e.rewards.participation_weights[0],e.rewards.participation_weights[1],e.rewards.participation_weights[2],e.rewards.weight_denominator})},
  {"attestation",numbers({r.attestation.minimum_inclusion_delay,r.attestation.base_reward_factor,r.attestation.participation_weights[0],r.attestation.participation_weights[1],r.attestation.participation_weights[2],r.attestation.proposer_weight,r.attestation.weight_denominator})},
  {"syncAggregate",numbers({r.sync.slots_per_epoch,r.sync.effective_balance_increment,r.sync.base_reward_factor,r.sync.sync_reward_weight,r.sync.proposer_weight,r.sync.weight_denominator,r.sync.min_seed_lookahead,r.sync.maximum_effective_balance,r.sync.shuffle_rounds})},
  {"duties",numbers({d.slots_per_epoch,d.min_seed_lookahead,d.target_committee_size,d.max_committees_per_slot,d.effective_balance_increment,d.maximum_effective_balance,d.shuffle_rounds})},
  {"rewards",numbers({e.rewards.base_reward_factor,e.rewards.slots_per_epoch,e.rewards.effective_balance_increment,e.rewards.score_bias,e.rewards.minimum_epochs_to_inactivity_penalty,e.rewards.inactivity_penalty_quotient})},
  {"registry",numbers({q.minimum_churn_limit,q.churn_limit_quotient,q.max_seed_lookahead,q.minimum_withdrawability_delay,q.maximum_effective_balance,q.ejection_balance})},
  {"slashing",numbers({s.minimum_churn_limit,s.churn_limit_quotient,s.max_seed_lookahead,s.minimum_withdrawability_delay,s.epochs_per_slashings_vector,s.minimum_penalty_quotient,s.whistleblower_reward_quotient,s.proposer_reward_quotient,s.effective_balance_increment,s.proportional_slashing_multiplier})},
  {"effective",numbers({e.effective.increment,e.effective.maximum,e.effective.hysteresis_quotient,e.effective.downward_multiplier,e.effective.upward_multiplier})},
  {"history",numbers({h.slots_per_epoch,h.slots_per_historical_root,h.historical_roots_limit,e.epochs_per_eth1_voting_period})},
  {"stateLimits",numbers({l.slots_per_historical_root,l.epochs_per_historical_vector,l.epochs_per_slashings_vector,l.historical_roots_limit,l.eth1_votes_limit,l.validator_registry_limit,l.sync_committee_size})},
  {"bodyLimits",numbers({b.proposer_slashings,b.attester_slashings,b.attestations,b.deposits,b.voluntary_exits,b.validators_per_committee})},
  {"deposit",numbers({r.deposit.effective_balance_increment,r.deposit.maximum_effective_balance,r.deposit.validator_registry_limit})},
  {"genesisForkVersion",hex(Bytes(r.deposit.genesis_fork_version.begin(),r.deposit.genesis_fork_version.end()))},
  {"exit",numbers({r.exit.minimum_churn_limit,r.exit.churn_limit_quotient,r.exit.max_seed_lookahead,r.exit.minimum_withdrawability_delay,r.exit.shard_committee_period})},
  {"executionLimits",numbers({r.maximum_block_bytes})}};
}
Json altair_slot_snapshot(const AltairSlotState& state,const AltairTransitionRules& rules){
 if(!rules.epoch.history.slots_per_epoch||state.ssz.size()<48)throw std::invalid_argument("invalid snapshot slot context");
 const auto root=altair_beacon_state_root(state.ssz,rules.state);uint64_t slot=0;for(unsigned i=0;i<8;++i)slot|=uint64_t(state.ssz[40+i])<<(8*i);
 const auto epoch=slot/rules.epoch.history.slots_per_epoch;
 if((epoch==0&&state.last_processed_epoch)||(epoch>0&&(!state.last_processed_epoch||*state.last_processed_epoch!=epoch-1)))throw std::invalid_argument("snapshot epoch marker mismatch");
 return Json{{"format","nano-altair-slot-v1"},{"profile",altair_transition_profile(rules)},{"stateRoot",hex(Bytes(root.begin(),root.end()))},{"ssz",hex(state.ssz)},
  {"lastProcessedEpoch",state.last_processed_epoch?Json(quantity(Int(std::to_string(*state.last_processed_epoch)))):Json(nullptr)}};
}
AltairSlotState altair_slot_from_snapshot(const Json& snapshot,const AltairTransitionRules& rules,
 const ConsensusRoot& expected_root,size_t maximum_state_bytes){
 if(!snapshot.is_object()||snapshot.size()!=5||snapshot.at("format")!="nano-altair-slot-v1"||snapshot.at("profile")!=altair_transition_profile(rules)||snapshot.at("stateRoot")!=hex(Bytes(expected_root.begin(),expected_root.end()))||!snapshot.at("ssz").is_string())throw std::invalid_argument("snapshot identity/profile mismatch");
 const auto& encoded=snapshot.at("ssz").get_ref<const std::string&>();
 if(encoded.size()<2||encoded.size()%2||!encoded.starts_with("0x")||(encoded.size()-2)/2>maximum_state_bytes)throw std::invalid_argument("snapshot state size/encoding");
 AltairSlotState state{unhex(encoded),std::nullopt};
 if(!snapshot.at("lastProcessedEpoch").is_null())state.last_processed_epoch=u64(quantity(snapshot.at("lastProcessedEpoch")));
 if(altair_slot_snapshot(state,rules)!=snapshot)throw std::invalid_argument("noncanonical or corrupted slot snapshot");
 return state;
}
}
