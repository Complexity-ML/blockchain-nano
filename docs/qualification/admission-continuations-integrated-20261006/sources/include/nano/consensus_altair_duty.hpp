#pragma once
#include "nano/consensus_altair_fork_store.hpp"
namespace nano {
struct AltairAttestationDuty {
 uint64_t validator_index;
 Bytes public_key;
 ConsensusRoot genesis_root;
 ConsensusBytes4 fork_version;
 Phase0SignedAttestation attestation; // one membership bit, signature absent
};
// Derive a current-slot vote from an authenticated Store. Caller authenticates
// the anchor/registry PoPs and serializes the Store. No execution binding.
// A missing assignment, slashed/equivocating validator, stale slot or premature
// request returns nullopt; invalid trusted context/resource exhaustion throws.
std::optional<AltairAttestationDuty> prepare_altair_attestation_duty(
 const AltairForkStore&,const AltairForkBlockRules&,uint64_t validator_index,
 uint64_t expected_slot,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});
}
