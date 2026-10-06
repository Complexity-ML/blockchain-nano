#pragma once
#include "nano/consensus_altair_slots.hpp"
namespace nano::detail {
// Internal common operations. Capella supplies its own historical summaries;
// public Altair/Bellatrix entry points always retain the legacy history update.
AltairSlotState prepare_common_epoch(const AltairSlotState&,const AltairEpochRules&,
 const AltairStateLimits&,uint64_t,uint64_t,const std::function<void()>&,bool update_history);
}
