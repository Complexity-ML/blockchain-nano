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
}
