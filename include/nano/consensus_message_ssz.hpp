#pragma once
#include "nano/consensus_attestation_inclusion.hpp"
#include "nano/consensus_exit.hpp"
#include "nano/consensus_beacon_header.hpp"
namespace nano {
// Fixed SSZ ProposerSlashing: two SignedBeaconBlockHeader values, 416 bytes.
// Own inputs before callbacks. Authentication belongs to admission/transition.
Bytes encode_consensus_proposer_slashing_ssz(SignedConsensusBeaconHeader,
 SignedConsensusBeaconHeader,const std::function<void()>& checkpoint={});
std::optional<std::pair<SignedConsensusBeaconHeader,SignedConsensusBeaconHeader>>
 decode_consensus_proposer_slashing_ssz(std::span<const uint8_t>,const std::function<void()>& checkpoint={});
Bytes encode_consensus_voluntary_exit_ssz(const SignedConsensusVoluntaryExit&,const std::function<void()>& checkpoint={});
std::optional<SignedConsensusVoluntaryExit> decode_consensus_voluntary_exit_ssz(std::span<const uint8_t>,const std::function<void()>& checkpoint={});
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
Bytes encode_consensus_attester_slashing_ssz(const IndexedConsensusAttestation&,const IndexedConsensusAttestation&,
 uint32_t validators_per_committee,const std::function<void()>& checkpoint={});
std::optional<std::pair<IndexedConsensusAttestation,IndexedConsensusAttestation>>
 decode_consensus_attester_slashing_ssz(std::span<const uint8_t>,uint32_t validators_per_committee,
 const std::function<void()>& checkpoint={});
}
