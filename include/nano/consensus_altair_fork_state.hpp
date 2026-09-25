#pragma once
#include "nano/consensus_altair_slots.hpp"
#include "nano/consensus_fork_state.hpp"
namespace nano {
struct AltairForkStateContexts {Phase0ForkSlashingContext block;Phase0ForkVoteContext target;AltairSlotState target_state;};
Phase0ForkSlashingContext altair_fork_block_context(const AltairSlotState&,const ConsensusBeaconHeader&,
 const AltairEpochRules&,const AltairStateLimits&,const std::function<void()>& checkpoint={});
Phase0ForkCheckpoints altair_fork_checkpoints(const AltairSlotState&,const AltairEpochRules&,
 const AltairStateLimits&,const std::function<void()>& checkpoint={});
// Same authenticated-block precondition as the Phase0 helper; target context
// advances with Altair slots and epochs. No snapshot admission or durable store.
AltairForkStateContexts prepare_altair_fork_state_contexts(const AltairSlotState&,
 const ConsensusBeaconHeader&,const ConsensusCheckpoint&,const AltairEpochRules&,
 const AltairStateLimits&,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});
}
