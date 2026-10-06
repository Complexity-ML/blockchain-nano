#pragma once
#include "phase0_transition_rules.hpp"
#include "nano/consensus_altair_transition.hpp"
namespace test::phase0_minimal {
inline AltairTransitionRules altair_transition_rules(const fs::path& fixtures){
 const auto refs=fixtures/"consensus-slashing-official",altair=fixtures/"consensus-altair-state-roots-official";
 const auto p0=transition_rules(refs);const auto& common=p0.epoch;const auto config=read(refs/"minimal-config.yaml"),ap=read(altair/"minimal-altair.yaml");const auto am=load(altair/"manifest.json");check(hex(sha256(binary(altair/"minimal-altair.yaml"))).substr(2)==am.at("referenceFiles").at("minimal-altair.yaml").at("sha256").get<std::string>(),"Altair preset digest");auto a=[&](const char* k){return parameter(ap,k);};
 const auto slots=common.history.slots_per_epoch,inc=common.effective.increment;
 const AltairStateLimits l{common.history.slots_per_historical_root,common.epochs_per_randao_vector,common.slashing.epochs_per_slashings_vector,common.history.historical_roots_limit,common.epochs_per_eth1_voting_period*slots,p0.deposit.validator_registry_limit,a("SYNC_COMMITTEE_SIZE")};
 AltairEpochRules r;r.ffg={slots,inc};r.inactivity={slots,parameter(config,"INACTIVITY_SCORE_BIAS"),parameter(config,"INACTIVITY_SCORE_RECOVERY_RATE"),common.rewards.minimum_epochs_to_inactivity_penalty};r.rewards={slots,inc,common.rewards.base_reward_factor,r.inactivity.score_bias,a("INACTIVITY_PENALTY_QUOTIENT_ALTAIR"),r.inactivity.minimum_epochs_to_inactivity_penalty,{14,26,14},64};r.sync={slots,a("EPOCHS_PER_SYNC_COMMITTEE_PERIOD"),common.rewards.duties.min_seed_lookahead,common.effective.maximum,common.rewards.duties.shuffle_rounds};r.registry=common.registry;r.slashing=common.slashing;r.slashing.minimum_penalty_quotient=a("MIN_SLASHING_PENALTY_QUOTIENT_ALTAIR");r.slashing.proportional_slashing_multiplier=a("PROPORTIONAL_SLASHING_MULTIPLIER_ALTAIR");r.slashing.proposer_reward_quotient=64/8;r.effective=common.effective;r.history=common.history;r.epochs_per_eth1_voting_period=common.epochs_per_eth1_voting_period;
 return {r,l,p0.body,p0.deposit,p0.exit,{common.rewards.duties,p0.minimum_attestation_inclusion_delay,common.rewards.base_reward_factor,{14,26,14},8,64},{slots,inc,common.rewards.base_reward_factor,2,8,64,r.sync.min_seed_lookahead,r.sync.maximum_effective_balance,r.sync.shuffle_rounds},p0.maximum_block_bytes};
}
}
