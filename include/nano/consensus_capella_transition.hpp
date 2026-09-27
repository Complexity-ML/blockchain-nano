#pragma once
#include "nano/consensus_altair_transition.hpp"
#include "nano/consensus_capella_slots.hpp"
#include "nano/consensus_capella_execution.hpp"
namespace nano {
struct CapellaTransitionRules {
 AltairTransitionRules consensus; // common operations, Capella penalty values
 ExecutionPayloadLimits execution;
 size_t maximum_state_bytes;
 uint64_t seconds_per_slot;
 CapellaWithdrawalRules withdrawals;
 ConsensusBytes4 genesis_version;
 uint32_t maximum_bls_changes;
 Int chain_id;
 ReplayPolicy replay_policy;
};
struct CapellaBlockCandidate {
 CapellaSlotState consensus;
 BellatrixExecutionCandidate execution;
};
// Full signed post-merge Capella block, starting from authenticated parents.
// Includes slots/epochs, BLS/proposer/parent, EVM, RANDAO, all body operations,
// sync aggregate and advertised post-state root. Never publishes either state.
// Invalid envelope/consensus operations -> nullopt. Invalid execution, invalid
// configuration, budget exhaustion, unavailable history or cancellation throw.
// Terminal PoW and fork activation are outside this post-merge API.
std::optional<CapellaBlockCandidate> prepare_capella_state_transition(
 const CapellaSlotState&,std::span<const uint8_t> signed_block,
 const CapellaTransitionRules&,const Header& execution_parent,const State& execution_state,
 const std::function<Bytes(const Int&)>& authenticated_block_hash,
 uint64_t maximum_slots,uint64_t maximum_sync_trials,const std::function<void()>& checkpoint={});
struct CapellaUnsignedBlockCandidate {Bytes message_ssz;CapellaBlockCandidate post;};
// Construction only; NOT admission or authorization to sign. The signing
// reactor must bind duty/head/time and reserve the anti-slashing journal.
std::optional<CapellaUnsignedBlockCandidate> prepare_capella_unsigned_block(
 const CapellaSlotState&,uint64_t slot,uint64_t proposer_index,
 const ConsensusRoot& parent_root,std::span<const uint8_t> body,
 const CapellaTransitionRules&,const Header& execution_parent,const State& execution_state,
 const std::function<Bytes(const Int&)>& authenticated_block_hash,
 uint64_t maximum_slots,uint64_t maximum_sync_trials,const std::function<void()>& checkpoint={});
}
