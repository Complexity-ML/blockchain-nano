#pragma once
#include "nano/consensus_bellatrix_slots.hpp"
#include "nano/consensus_capella_slots.hpp"
namespace nano {
// Isolated upgrade_to_capella: authenticates and transforms the state, but does
// not decide activation. Runtime callers must use the scheduled boundary API.
CapellaSlotState upgrade_capella_state(const BellatrixSlotState&,const ConsensusRoot&,
 const AltairStateLimits&,const ExecutionPayloadLimits&,size_t maximum_state_bytes,
 uint64_t slots_per_epoch,const ConsensusBytes4& capella_version,
 const std::function<void()>& checkpoint={});
struct CapellaForkSchedule {uint64_t epoch;ConsensusBytes4 bellatrix_version,capella_version;};
// Advance through exactly one authenticated Bellatrix->Capella boundary, even
// when no block exists at that slot. No durable commit or network activation.
CapellaSlotState prepare_capella_fork_slots(const BellatrixSlotState&,const ConsensusRoot&,
 const BellatrixEpochRules&,const AltairStateLimits&,const CapellaForkSchedule&,
 uint64_t target_slot,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});
}
