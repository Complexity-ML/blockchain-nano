#pragma once
#include "nano/consensus_capella_slots.hpp"
#include "nano/consensus_fork_state.hpp"
namespace nano {
struct CapellaForkStateContexts {Phase0ForkSlashingContext block;Phase0ForkVoteContext target;CapellaSlotState target_state;};
Phase0ForkSlashingContext capella_fork_block_context(const CapellaSlotState&,const ConsensusBeaconHeader&,
 const CapellaEpochRules&,const AltairStateLimits&,const std::function<void()>& checkpoint={});
Phase0ForkCheckpoints capella_fork_checkpoints(const CapellaSlotState&,const CapellaEpochRules&,
 const AltairStateLimits&,const std::function<void()>& checkpoint={});
// Same authenticated-block precondition as the Phase0 helper; target context
// advances with Capella slots and epochs. No snapshot admission or durable store.
CapellaForkStateContexts prepare_capella_fork_state_contexts(const CapellaSlotState&,
 const ConsensusBeaconHeader&,const ConsensusCheckpoint&,const CapellaEpochRules&,
 const AltairStateLimits&,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});
struct CapellaProposerIdentity {
 uint64_t index;
 ConsensusBytes4 fork_version;
 ConsensusRoot genesis_root;
 Bytes public_key;
};
// Authenticate the parent, advance to the proposed slot and derive the selected
// proposer from that state. Slashed selection is refused, never rerolled.
std::optional<CapellaProposerIdentity> prepare_capella_proposer_identity(
 const CapellaSlotState&,const ConsensusBeaconHeader&,uint64_t slot,
 const CapellaEpochRules&,const AltairStateLimits&,const Phase0FfgRules&,
 uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});

}
