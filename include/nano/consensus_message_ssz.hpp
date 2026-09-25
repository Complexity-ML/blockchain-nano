#pragma once
#include "nano/consensus_attestation_inclusion.hpp"
namespace nano {
// Canonical pre-Electra attestation / indexed-attestation schemas (Phase0 through
// Deneb). Encoding validation only: signatures, committee membership and gossip
// time windows are validated by the event admission layer, never by this codec.
// Malformed / oversized input returns nullopt, zero configured limit throws.
// Allocation is proportional to actual input, bounded by validators_per_committee.
// Cancellation/resource exceptions propagate; they are not invalid peer messages.
// Canonical encoding; rejects an oversized bitlist or a non-96-byte signature.
Bytes encode_consensus_attestation_ssz(const Phase0SignedAttestation&,
 uint32_t validators_per_committee,const std::function<void()>& checkpoint={});
std::optional<Phase0SignedAttestation> decode_consensus_attestation_ssz(
 std::span<const uint8_t>,uint32_t validators_per_committee,const std::function<void()>& checkpoint={});
std::optional<std::pair<IndexedConsensusAttestation,IndexedConsensusAttestation>>
 decode_consensus_attester_slashing_ssz(std::span<const uint8_t>,uint32_t validators_per_committee,
 const std::function<void()>& checkpoint={});
}
