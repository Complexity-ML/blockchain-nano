#pragma once
#include "nano/consensus_slots.hpp"
#include "nano/consensus_registry_operations.hpp"
namespace nano {
struct Phase0TransitionRules {
 Phase0EpochRules epoch;
 Phase0StateLimits state;
 Phase0BodyLimits body;
 Phase0DepositRules deposit;
 Phase0ExitRules exit;
 uint64_t minimum_attestation_inclusion_delay;
 size_t maximum_block_bytes;
};
// Full Phase0 signed block state transition on an authenticated starting state:
// slots/epochs, parent/proposer/signatures, RANDAO, Eth1 voting, all operations,
// and final advertised state_root. No skip-validation mode. Returns a private
// candidate, never commits it. Resource budget/configuration errors, arithmetic
// overflow and cancellation may throw; invalid signed blocks return nullopt.
// Not snapshot admission, fork choice, durable commit or network activation.
std::optional<Phase0SlotState> prepare_phase0_state_transition(const Phase0SlotState&,
 std::span<const uint8_t> signed_block,const Phase0TransitionRules&,uint64_t maximum_slots,
 const std::function<void()>& checkpoint={});
}
