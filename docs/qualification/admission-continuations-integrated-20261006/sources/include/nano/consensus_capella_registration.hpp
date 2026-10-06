#pragma once
#include "nano/consensus_beacon_state.hpp"
#include <map>
#include <set>
namespace nano {
// The caller supplies the root of an already admitted consensus state.
// Authenticate SSZ and list cardinalities, then select only requested keys.
// Does not validate deposits/PoP, activation, fork choice or signing duties.
std::map<Bytes,uint64_t> capella_validator_indices(std::span<const uint8_t> state,
 const ConsensusRoot& authenticated_root,const AltairStateLimits&,const ExecutionPayloadLimits&,
 size_t maximum_bytes,const std::set<Bytes>& keys,const std::function<void()>& checkpoint={});
}
