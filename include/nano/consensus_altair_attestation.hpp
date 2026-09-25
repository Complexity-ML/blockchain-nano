#pragma once
#include "nano/consensus_attestation_inclusion.hpp"
#include "nano/consensus_beacon_state.hpp"
namespace nano {
struct AltairAttestationRules {
 Phase0FfgRules duties;
 uint64_t minimum_inclusion_delay,base_reward_factor;
 std::array<uint64_t,3> participation_weights;
 uint64_t proposer_weight,weight_denominator;
};
// Isolated Altair process_attestation on an admitted state at the block slot.
// Registry keys must have established PoPs. Repeated inclusion is valid but
// already credited participation flags cannot earn proposer rewards twice.
// Returns private SSZ or nullopt for invalid attestations; no enclosing block,
// slot advancement, durable commit or network admission is performed here.
std::optional<Bytes> prepare_altair_attestation(std::span<const uint8_t> state,
 const Phase0SignedAttestation&,const AltairStateLimits&,const AltairAttestationRules&,
 const std::function<void()>& checkpoint={});
}
