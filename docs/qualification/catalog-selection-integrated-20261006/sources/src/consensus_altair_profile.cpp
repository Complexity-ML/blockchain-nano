#include "nano/consensus_altair_snapshot.hpp"
#include <limits>
#include <type_traits>
#include <set>
namespace nano {
namespace {
template<class T> void number(const Json& j,T& destination){
 static_assert(std::is_unsigned_v<T>);
 if(!j.is_string())throw std::invalid_argument("Altair profile requires canonical quantities");
 const auto value=quantity(j);
 if(value<0||value>Int(std::to_string(std::numeric_limits<T>::max()))||quantity(value)!=j.get<std::string>())throw std::invalid_argument("Altair profile quantity range or encoding");
 destination=static_cast<T>(u64(value));
}
template<class... T> void fields(const Json& j,const char* key,T&... destinations){
 const auto& values=j.at(key);
 if(!values.is_array()||values.size()!=sizeof...(T))throw std::invalid_argument("Altair profile field count");
 size_t index=0;(number(values.at(index++),destinations),...);
}
}
AltairTransitionRules altair_transition_from_profile(const Json& j){
 if(!j.is_object())throw std::invalid_argument("Altair profile object required");
 AltairTransitionRules r{};
 auto& e=r.epoch;auto& d=r.attestation.duties;auto& q=e.registry;auto& s=e.slashing;auto& h=e.history;auto& l=r.state;auto& b=r.body;
 fields(j,"ffg",e.ffg.slots_per_epoch,e.ffg.effective_balance_increment);
 fields(j,"inactivity",e.inactivity.slots_per_epoch,e.inactivity.score_bias,e.inactivity.score_recovery_rate,e.inactivity.minimum_epochs_to_inactivity_penalty);
 fields(j,"epochSync",e.sync.slots_per_epoch,e.sync.epochs_per_sync_committee_period,e.sync.min_seed_lookahead,e.sync.maximum_effective_balance,e.sync.shuffle_rounds);
 fields(j,"rewardWeights",e.rewards.participation_weights[0],e.rewards.participation_weights[1],e.rewards.participation_weights[2],e.rewards.weight_denominator);
 fields(j,"attestation",r.attestation.minimum_inclusion_delay,r.attestation.base_reward_factor,r.attestation.participation_weights[0],r.attestation.participation_weights[1],r.attestation.participation_weights[2],r.attestation.proposer_weight,r.attestation.weight_denominator);
 fields(j,"syncAggregate",r.sync.slots_per_epoch,r.sync.effective_balance_increment,r.sync.base_reward_factor,r.sync.sync_reward_weight,r.sync.proposer_weight,r.sync.weight_denominator,r.sync.min_seed_lookahead,r.sync.maximum_effective_balance,r.sync.shuffle_rounds);
 fields(j,"duties",d.slots_per_epoch,d.min_seed_lookahead,d.target_committee_size,d.max_committees_per_slot,d.effective_balance_increment,d.maximum_effective_balance,d.shuffle_rounds);
 fields(j,"rewards",e.rewards.base_reward_factor,e.rewards.slots_per_epoch,e.rewards.effective_balance_increment,e.rewards.score_bias,e.rewards.minimum_epochs_to_inactivity_penalty,e.rewards.inactivity_penalty_quotient);
 fields(j,"registry",q.minimum_churn_limit,q.churn_limit_quotient,q.max_seed_lookahead,q.minimum_withdrawability_delay,q.maximum_effective_balance,q.ejection_balance);
 fields(j,"slashing",s.minimum_churn_limit,s.churn_limit_quotient,s.max_seed_lookahead,s.minimum_withdrawability_delay,s.epochs_per_slashings_vector,s.minimum_penalty_quotient,s.whistleblower_reward_quotient,s.proposer_reward_quotient,s.effective_balance_increment,s.proportional_slashing_multiplier);
 fields(j,"effective",e.effective.increment,e.effective.maximum,e.effective.hysteresis_quotient,e.effective.downward_multiplier,e.effective.upward_multiplier);
 fields(j,"history",h.slots_per_epoch,h.slots_per_historical_root,h.historical_roots_limit,e.epochs_per_eth1_voting_period);
 fields(j,"stateLimits",l.slots_per_historical_root,l.epochs_per_historical_vector,l.epochs_per_slashings_vector,l.historical_roots_limit,l.eth1_votes_limit,l.validator_registry_limit,l.sync_committee_size);
 fields(j,"bodyLimits",b.proposer_slashings,b.attester_slashings,b.attestations,b.deposits,b.voluntary_exits,b.validators_per_committee);
 fields(j,"deposit",r.deposit.effective_balance_increment,r.deposit.maximum_effective_balance,r.deposit.validator_registry_limit);
 fields(j,"exit",r.exit.minimum_churn_limit,r.exit.churn_limit_quotient,r.exit.max_seed_lookahead,r.exit.minimum_withdrawability_delay,r.exit.shard_committee_period);
 fields(j,"executionLimits",r.maximum_block_bytes);
 const auto& encoded=j.at("genesisForkVersion");
 if(!encoded.is_string()||encoded.get_ref<const std::string&>().size()!=10)throw std::invalid_argument("Altair genesis fork version width");
 const auto version=unhex(encoded.get<std::string>());
 if(version.size()!=4)throw std::invalid_argument("Altair genesis fork version encoding");
 std::copy(version.begin(),version.end(),r.deposit.genesis_fork_version.begin());
 if(altair_transition_profile(r)!=j)throw std::invalid_argument("Noncanonical or unknown Altair profile fields");
 validate_altair_transition_rules(r);
 return r;
}
AltairTransitionRules load_altair_transition_profile(std::string_view bytes,const Bytes& expected,size_t maximum_bytes){
 if(!maximum_bytes||bytes.size()>maximum_bytes||expected.size()!=32)throw std::invalid_argument("Altair profile file bound or digest width");
 if(sha256(Bytes(bytes.begin(),bytes.end()))!=expected)throw std::invalid_argument("Altair profile digest mismatch");
 std::set<std::string> keys;bool duplicate=false;
 const auto json=Json::parse(bytes,[&](int depth,Json::parse_event_t event,Json& value){if(depth==1&&event==Json::parse_event_t::key&&!keys.insert(value.get<std::string>()).second)duplicate=true;return true;});
 if(duplicate)throw std::invalid_argument("duplicate Altair profile field");
 return altair_transition_from_profile(json);
}

}
