#pragma once
#include "phase0_epoch_ssz.hpp"
#include "nano/consensus_transition.hpp"
namespace test::phase0_minimal {
inline Phase0TransitionRules transition_rules(const fs::path& refs){
 const auto e=epoch_rules(refs);const auto preset=read(refs/"minimal-phase0.yaml"),config=read(refs/"minimal-config.yaml");auto p=[&](const char* k){return parameter(preset,k);};
 return {e,{e.history.slots_per_historical_root,e.epochs_per_randao_vector,e.slashing.epochs_per_slashings_vector,e.history.historical_roots_limit,e.epochs_per_eth1_voting_period*e.history.slots_per_epoch,p("VALIDATOR_REGISTRY_LIMIT"),e.maximum_pending_attestations,p("MAX_VALIDATORS_PER_COMMITTEE")},
 {uint32_t(p("MAX_PROPOSER_SLASHINGS")),uint32_t(p("MAX_ATTESTER_SLASHINGS")),uint32_t(p("MAX_ATTESTATIONS")),uint32_t(p("MAX_DEPOSITS")),uint32_t(p("MAX_VOLUNTARY_EXITS")),uint32_t(p("MAX_VALIDATORS_PER_COMMITTEE"))},
 {p("EFFECTIVE_BALANCE_INCREMENT"),p("MAX_EFFECTIVE_BALANCE"),p("VALIDATOR_REGISTRY_LIMIT"),{0,0,0,1}},
 {e.registry.minimum_churn_limit,e.registry.churn_limit_quotient,e.registry.max_seed_lookahead,e.registry.minimum_withdrawability_delay,parameter(config,"SHARD_COMMITTEE_PERIOD")},p("MIN_ATTESTATION_INCLUSION_DELAY"),8*1024*1024};
}
}
