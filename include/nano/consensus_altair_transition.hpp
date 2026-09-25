#pragma once
#include "nano/consensus_altair_slots.hpp"
#include "nano/consensus_altair_attestation.hpp"
#include "nano/consensus_altair_sync_aggregate.hpp"
#include "nano/consensus_altair_deposit.hpp"
#include "nano/consensus_altair_slashing.hpp"
#include "nano/consensus_beacon_body.hpp"
#include "nano/consensus_exit.hpp"
namespace nano {
struct AltairTransitionRules {
 AltairEpochRules epoch;
 AltairStateLimits state;
 Phase0BodyLimits body;
 Phase0DepositRules deposit;
 Phase0ExitRules exit;
 AltairAttestationRules attestation;
 AltairSyncAggregateRules sync;
 size_t maximum_block_bytes;
};
// Cross-component consistency checks, also used when loading local profiles.
void validate_altair_transition_rules(const AltairTransitionRules&);
// Full Altair signed block transition on an authenticated starting state:
// slots/epochs, block signature, proposer/parent, RANDAO, Eth1 vote, ordered
// operations, SyncAggregate, final state root. No skip-validation option.
// Returns a private SSZ/epoch-marker candidate. Invalid block -> nullopt;
// bad configuration/state, budget exhaustion or cancellation may throw.
// Does not admit snapshots, implement fork choice, persist or activate a network.
std::optional<AltairSlotState> prepare_altair_state_transition(const AltairSlotState&,
 std::span<const uint8_t> signed_block,const AltairTransitionRules&,
 uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});
}
