#pragma once
#include "nano/consensus_beacon_state.hpp"
#include "nano/consensus_epoch.hpp"
namespace nano {
struct Phase0SlotState {
 Bytes ssz;
 std::optional<uint64_t> last_processed_epoch;
 bool operator==(const Phase0SlotState&)const=default;
};
// Complete Phase0 process_slots on an AUTHENTICATED starting BeaconState.
// Updates cached SSZ roots, the latest header and ordered epoch components.
// target_slot must advance; maximum_slots is a caller resource budget, NOT a
// consensus fork rule. Candidate bytes and replay metadata must commit together.
// This does not admit a genesis/snapshot, authenticate blocks or persist state.
Phase0SlotState prepare_phase0_slots(const Phase0SlotState&,const Phase0EpochRules&,
 const Phase0StateLimits&,uint64_t target_slot,uint64_t maximum_slots,
 const std::function<void()>& checkpoint={});
}
