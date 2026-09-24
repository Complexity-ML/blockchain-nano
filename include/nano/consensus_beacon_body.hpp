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
}
