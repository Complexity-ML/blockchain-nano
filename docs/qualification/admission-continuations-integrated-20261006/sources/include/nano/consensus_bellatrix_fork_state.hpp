#pragma once
#include "nano/consensus_bellatrix_slots.hpp"
#include "nano/consensus_fork_state.hpp"
namespace nano {
struct BellatrixForkStateContexts {Phase0ForkSlashingContext block;Phase0ForkVoteContext target;BellatrixSlotState target_state;};
Phase0ForkSlashingContext bellatrix_fork_block_context(const BellatrixSlotState&,const ConsensusBeaconHeader&,
 const BellatrixEpochRules&,const AltairStateLimits&,const std::function<void()>& checkpoint={});
Phase0ForkCheckpoints bellatrix_fork_checkpoints(const BellatrixSlotState&,const BellatrixEpochRules&,
 const AltairStateLimits&,const std::function<void()>& checkpoint={});
// Same authenticated-block precondition as the Phase0 helper; target context
// advances with Bellatrix slots and epochs. No snapshot admission or durable store.
BellatrixForkStateContexts prepare_bellatrix_fork_state_contexts(const BellatrixSlotState&,
 const ConsensusBeaconHeader&,const ConsensusCheckpoint&,const BellatrixEpochRules&,
 const AltairStateLimits&,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});
struct BellatrixProposerIdentity {
 uint64_t index;
 ConsensusBytes4 fork_version;
 ConsensusRoot genesis_root;
 Bytes public_key;
};
// Authenticate the parent, advance to the proposed slot and derive the selected
// proposer from that state. Slashed selection is refused, never rerolled.
std::optional<BellatrixProposerIdentity> prepare_bellatrix_proposer_identity(
 const BellatrixSlotState&,const ConsensusBeaconHeader&,uint64_t slot,
 const BellatrixEpochRules&,const AltairStateLimits&,const Phase0FfgRules&,
 uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});

}
