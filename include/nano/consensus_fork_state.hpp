#pragma once
#include "nano/consensus_slots.hpp"
#include "nano/consensus_fork_vote.hpp"
namespace nano {
struct Phase0ForkStateContexts {
 Phase0ForkSlashingContext block;
 Phase0ForkVoteContext target;
 Phase0SlotState target_state;
};
// Input is the immediate post-state of a PREVIOUSLY AUTHENTICATED block/header.
// Checks SSZ root, latest header, slot and checkpoint block-root binding, then
// advances skipped slots to target.epoch's start and derives both contexts.
// This does not authenticate a header obtained from the network, establish key
// possession at genesis, persist a checkpoint or admit an arbitrary snapshot.
Phase0ForkStateContexts prepare_phase0_fork_state_contexts(
 const Phase0SlotState&,const ConsensusBeaconHeader& authenticated_header,
 const ConsensusCheckpoint& target,const Phase0EpochRules&,const Phase0StateLimits&,
 uint64_t maximum_slots,const std::function<void()>& checkpoint={});
}
