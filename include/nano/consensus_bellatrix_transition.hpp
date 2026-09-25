#pragma once
#include "nano/consensus_altair_transition.hpp"
#include "nano/consensus_bellatrix_slots.hpp"
#include "nano/consensus_bellatrix_execution.hpp"
namespace nano {
struct BellatrixTransitionRules {
 AltairTransitionRules consensus; // common operations, Bellatrix penalty values
 ExecutionPayloadLimits execution;
 size_t maximum_state_bytes;
 uint64_t seconds_per_slot;
 Int chain_id;
 ReplayPolicy replay_policy;
};
struct BellatrixBlockCandidate {
 BellatrixSlotState consensus;
 BellatrixExecutionCandidate execution;
};
// Full signed post-merge Bellatrix block, starting from authenticated parents.
// Includes slots/epochs, BLS/proposer/parent, EVM, RANDAO, all body operations,
// sync aggregate and advertised post-state root. Never publishes either state.
// Invalid envelope/consensus operations -> nullopt. Invalid execution, invalid
// configuration, budget exhaustion, unavailable history or cancellation throw.
// Terminal PoW and fork activation are outside this post-merge API.
std::optional<BellatrixBlockCandidate> prepare_bellatrix_state_transition(
 const BellatrixSlotState&,std::span<const uint8_t> signed_block,
 const BellatrixTransitionRules&,const Header& execution_parent,const State& execution_state,
 const std::function<Bytes(const Int&)>& authenticated_block_hash,
 uint64_t maximum_slots,uint64_t maximum_sync_trials,const std::function<void()>& checkpoint={});
struct BellatrixUnsignedBlockCandidate {Bytes message_ssz;BellatrixBlockCandidate post;};
// Construction only; NOT admission or authorization to sign. The signing
// reactor must bind duty/head/time and reserve the anti-slashing journal.
std::optional<BellatrixUnsignedBlockCandidate> prepare_bellatrix_unsigned_block(
 const BellatrixSlotState&,uint64_t slot,uint64_t proposer_index,
 const ConsensusRoot& parent_root,std::span<const uint8_t> body,
 const BellatrixTransitionRules&,const Header& execution_parent,const State& execution_state,
 const std::function<Bytes(const Int&)>& authenticated_block_hash,
 uint64_t maximum_slots,uint64_t maximum_sync_trials,const std::function<void()>& checkpoint={});
}
