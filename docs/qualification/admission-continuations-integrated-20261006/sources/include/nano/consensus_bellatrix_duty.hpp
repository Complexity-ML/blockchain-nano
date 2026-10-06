#pragma once
#include "nano/consensus_bellatrix_fork_store.hpp"
namespace nano {
struct BellatrixAttestationDuty {
 uint64_t validator_index;
 Bytes public_key;
 ConsensusRoot genesis_root;
 ConsensusBytes4 fork_version;
 Phase0SignedAttestation attestation; // one membership bit, signature absent
};
// Derive the current committee in one state traversal, independent of local key count.
std::map<uint64_t,BellatrixAttestationDuty> prepare_bellatrix_attestation_duties(
 const BellatrixForkStore&,const BellatrixForkBlockRules&,uint64_t expected_slot,
 uint64_t maximum_slots,uint64_t maximum_sync_trials,const std::function<void()>& checkpoint={});
// Derive a current-slot vote from an authenticated Store. Caller authenticates
// the anchor/registry PoPs and serializes the Store. Beacon and execution records remain paired.
// A missing assignment, slashed/equivocating validator, stale slot or premature
// request returns nullopt; invalid trusted context/resource exhaustion throws.
std::optional<BellatrixAttestationDuty> prepare_bellatrix_attestation_duty(
 const BellatrixForkStore&,const BellatrixForkBlockRules&,uint64_t validator_index,
 uint64_t expected_slot,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});
}
