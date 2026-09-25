#pragma once
#include "nano/consensus_signing.hpp"
#include <span>
namespace nano {
// Consensus SSZ type bounds supplied by the authenticated Phase0 preset.
// These are per-operation/per-committee limits, not a total validator cap.
struct Phase0BodyLimits {
 uint32_t proposer_slashings,attester_slashings,attestations,deposits,voluntary_exits,validators_per_committee;
};
// Strict canonical SSZ decoding and hash_tree_root for the complete Phase0 body.
// Throws std::invalid_argument on malformed data or invalid type bounds.
// Does not verify operation signatures, eligibility or state transitions.
ConsensusRoot phase0_beacon_body_root(std::span<const uint8_t> bytes,const Phase0BodyLimits& limits);
// Altair appends a fixed SyncAggregate to the body; operation SSZ types remain
// unchanged. sync_committee_size comes from the authenticated Altair preset.
ConsensusRoot altair_beacon_body_root(std::span<const uint8_t> bytes,const Phase0BodyLimits& limits,uint64_t sync_committee_size);
}
