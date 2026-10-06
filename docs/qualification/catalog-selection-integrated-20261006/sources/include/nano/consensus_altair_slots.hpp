#pragma once
#include "nano/consensus_altair_epoch.hpp"
#include "nano/consensus_epoch.hpp"
namespace nano {
struct AltairEpochRules {
 AltairFfgRules ffg;
 AltairInactivityRules inactivity;
 AltairRewardRules rewards;
 AltairSyncRules sync;
 Phase0RegistryUpdateRules registry;
 Phase0SlashingRules slashing;
 Phase0EffectiveBalanceRules effective;
 Phase0HistoryRules history;
 uint64_t epochs_per_eth1_voting_period;
};
struct AltairSlotState {Bytes ssz;std::optional<uint64_t> last_processed_epoch;bool operator==(const AltairSlotState&)const=default;};
void validate_altair_epoch_rules(const AltairEpochRules&,const AltairStateLimits&);
// Complete ordered Altair process_epoch on an authenticated end-of-epoch state
// whose current-slot roots have already been cached by process_slot.
// Leaves slot unchanged; sets the marker. Caller commits bytes/marker together.
AltairSlotState prepare_altair_epoch(const AltairSlotState&,const AltairEpochRules&,
 const AltairStateLimits&,uint64_t expected_epoch,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});
// Complete process_slots using Altair roots and epoch operations. Starting
// state must be authenticated. Budgets are local, not consensus validator caps.
AltairSlotState prepare_altair_slots(const AltairSlotState&,const AltairEpochRules&,
 const AltairStateLimits&,uint64_t target_slot,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});
}
