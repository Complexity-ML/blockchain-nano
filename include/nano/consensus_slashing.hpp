#pragma once
#include "nano/consensus_attestation.hpp"
#include "nano/consensus_committee.hpp"
namespace nano {
struct IndexedConsensusAttestation {
 ConsensusAttestationData data;
 std::vector<uint64_t> indices;
 Bytes signature;
};
// Verify a directional double/surround proof against two trusted historical
// snapshots/domains. Returns common authenticated signer indices, or nullopt.
// Caller still checks slashability at the processing epoch, applies penalties
// exactly once, and persists the registry transition atomically.
std::optional<std::vector<uint64_t>> verify_attester_slashing(
 const IndexedConsensusAttestation& first,const ConsensusCommittee& first_context,
 const IndexedConsensusAttestation& second,const ConsensusCommittee& second_context);

// Explicit Phase0 economics profile, NOT the Electra/Pectra slashing rules.
// Values must come from authenticated chain configuration. No network defaults.
struct Phase0SlashingRules {
 uint64_t minimum_churn_limit, churn_limit_quotient, max_seed_lookahead;
 uint64_t minimum_withdrawability_delay, epochs_per_slashings_vector;
 uint64_t minimum_penalty_quotient, whistleblower_reward_quotient, proposer_reward_quotient;
};
struct ConsensusSlashingValidator {
 std::array<uint8_t,48> public_key{};
 uint64_t balance=0, effective_balance=0;
 uint64_t activation_epoch=UINT64_MAX, exit_epoch=UINT64_MAX, withdrawable_epoch=UINT64_MAX;
 bool slashed=false;
 bool operator==(const ConsensusSlashingValidator&)const=default;
};
struct ConsensusSlashingState {
 uint64_t epoch=0;
 std::map<uint64_t,ConsensusSlashingValidator> validators;
 std::vector<uint64_t> slashings;
 bool operator==(const ConsensusSlashingState&)const=default;
};
// Atomic in-memory Phase0 transition: verify proof, check current eligibility,
// queue exits, defer withdrawal, debit penalty and credit proposer/whistleblower.
// nullopt means invalid proof or no slashable signer; bad configuration/state or
// arithmetic overflow throws, always leaving state unchanged. Caller owns locking,
// authenticated historical contexts, epoch processing and durable block commit.
// Does not perform later correlation penalties or activate any network rules.
std::optional<std::vector<uint64_t>> process_phase0_attester_slashing(
 ConsensusSlashingState& state,const Phase0SlashingRules& rules,
 const IndexedConsensusAttestation& first,const ConsensusCommittee& first_context,
 const IndexedConsensusAttestation& second,const ConsensusCommittee& second_context,
 uint64_t proposer_index,uint64_t whistleblower_index);
}
