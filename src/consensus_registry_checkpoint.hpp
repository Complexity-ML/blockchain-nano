#pragma once
#include "nano/consensus_slashing.hpp"
namespace nano::consensus_registry_detail {
// Private staging only. Keep the old fast copy when cancellation is absent;
// otherwise visit each fixed-size row/bucket before making it part of the copy.
inline ConsensusSlashingState copy(const ConsensusSlashingState& state,const std::function<void()>& checkpoint){
 if(!checkpoint)return state;
 checkpoint();ConsensusSlashingState next;next.epoch=state.epoch;
 for(const auto& [index,validator]:state.validators){checkpoint();next.validators.emplace_hint(next.validators.end(),index,validator);}
 next.slashings.reserve(state.slashings.size());
 for(const auto amount:state.slashings){checkpoint();next.slashings.push_back(amount);}
 checkpoint();return next;
}
}
