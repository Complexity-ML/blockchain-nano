#pragma once
#include "nano/consensus_beacon_state.hpp"
#include "nano/consensus_execution_bridge.hpp"
namespace nano {
struct BellatrixPayloadStateCandidate {
 Bytes consensus_state;
 ConsensusRoot consensus_root;
 BellatrixExecutionCandidate execution;
};
// The caller must authenticate the state after process_slots and process_header,
// BEFORE process_randao, as required by Bellatrix process_block ordering.
// This is the execution-payload operation, NOT a full signed block transition.
// Post-merge only. Both results are private candidates; no database publication.
BellatrixPayloadStateCandidate prepare_bellatrix_payload_state(
 std::span<const uint8_t> state,const ConsensusRoot& authenticated_state_root,
 const AltairStateLimits&,const ExecutionPayloadLimits&,size_t maximum_state_bytes,
 uint64_t slots_per_epoch,uint64_t seconds_per_slot,
 std::span<const uint8_t> payload,size_t maximum_payload_bytes,
 const Header& execution_parent,const State& execution_state,const Int& chain_id,
 ReplayPolicy,const std::function<Bytes(const Int&)>& authenticated_block_hash,
 const std::function<void()>& checkpoint={});
}
