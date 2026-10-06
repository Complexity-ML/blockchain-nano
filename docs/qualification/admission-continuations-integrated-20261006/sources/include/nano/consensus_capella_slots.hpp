#pragma once
#include "nano/consensus_altair_slots.hpp"
namespace nano {
struct CapellaEpochRules {
 // Same operation formulas as Altair, with Capella penalty parameters from
 // the authenticated fork preset. Slot roots always use the Capella schema.
 AltairEpochRules operations;
 ExecutionPayloadLimits execution;
 size_t maximum_state_bytes;
};
struct CapellaSlotState {
 Bytes ssz;
 std::optional<uint64_t> last_processed_epoch;
 bool operator==(const CapellaSlotState&)const=default;
};
// Ordered process_epoch: caller supplies an authenticated end-of-epoch state
// with this slot's roots cached. Neither the slot nor execution header changes.
CapellaSlotState prepare_capella_epoch(const CapellaSlotState&,
 const CapellaEpochRules&,const AltairStateLimits&,uint64_t expected_epoch,
 uint64_t maximum_sync_trials,const std::function<void()>& checkpoint={});
// Advance an authenticated Capella state, cache Capella roots and process
// each crossed epoch. Local work/byte budgets are not total validator caps.
CapellaSlotState prepare_capella_slots(const CapellaSlotState&,
 const CapellaEpochRules&,const AltairStateLimits&,uint64_t target_slot,
 uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});
}
