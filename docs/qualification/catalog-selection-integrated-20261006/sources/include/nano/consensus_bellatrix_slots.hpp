#pragma once
#include "nano/consensus_altair_slots.hpp"
namespace nano {
struct BellatrixEpochRules {
 // Same operation formulas as Altair, with Bellatrix penalty parameters from
 // the authenticated fork preset. Slot roots always use the Bellatrix schema.
 AltairEpochRules operations;
 ExecutionPayloadLimits execution;
 size_t maximum_state_bytes;
};
struct BellatrixSlotState {
 Bytes ssz;
 std::optional<uint64_t> last_processed_epoch;
 bool operator==(const BellatrixSlotState&)const=default;
};
// Ordered process_epoch: caller supplies an authenticated end-of-epoch state
// with this slot's roots cached. Neither the slot nor execution header changes.
BellatrixSlotState prepare_bellatrix_epoch(const BellatrixSlotState&,
 const BellatrixEpochRules&,const AltairStateLimits&,uint64_t expected_epoch,
 uint64_t maximum_sync_trials,const std::function<void()>& checkpoint={});
// Advance an authenticated Bellatrix state, cache Bellatrix roots and process
// each crossed epoch. Local work/byte budgets are not total validator caps.
BellatrixSlotState prepare_bellatrix_slots(const BellatrixSlotState&,
 const BellatrixEpochRules&,const AltairStateLimits&,uint64_t target_slot,
 uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});
}
