#pragma once
#include "nano/consensus_signing.hpp"
#include "nano/consensus_execution_payload.hpp"
#include <functional>
#include <span>
namespace nano {
struct Phase0StateLimits {
 uint64_t slots_per_historical_root,epochs_per_historical_vector,epochs_per_slashings_vector;
 uint64_t historical_roots_limit,eth1_votes_limit,validator_registry_limit;
 uint64_t pending_attestations_limit,validators_per_committee;
};
// Complete Phase0 BeaconState SSZ schema and hash_tree_root. Rejects malformed
// offsets, lengths, booleans and bit encodings; sparse Merkle padding never
// allocates the configured list maximum. Limits must come from the fork preset.
// SSZ validity is NOT consensus validity: lifecycle, keys, balance/validator
// count coherence and genesis authentication belong to state admission.
// No slot transition, epoch replay metadata, persistence or network activation.
ConsensusRoot phase0_beacon_state_root(std::span<const uint8_t>,const Phase0StateLimits&,
 const std::function<void()>& checkpoint={});
struct AltairStateLimits {
 uint64_t slots_per_historical_root,epochs_per_historical_vector,epochs_per_slashings_vector;
 uint64_t historical_roots_limit,eth1_votes_limit,validator_registry_limit,sync_committee_size;
};
// Full Altair 24-field SSZ schema: participation byte lists, inactivity scores,
// and both sync committees. Same schema-only trust boundary as Phase0 above.
ConsensusRoot altair_beacon_state_root(std::span<const uint8_t>,const AltairStateLimits&,
 const std::function<void()>& checkpoint={});
// Bellatrix appends a variable ExecutionPayloadHeader (25 fields). Format/root
// validation only; no lifecycle or execution acceptance is implied.
ConsensusRoot bellatrix_beacon_state_root(std::span<const uint8_t>,const AltairStateLimits&,
 const ExecutionPayloadLimits&,size_t maximum_bytes,const std::function<void()>& checkpoint={});
}
