#pragma once
#include "nano/consensus_slashing.hpp"
#include <functional>
namespace nano {
struct Phase0EffectiveBalanceRules {
 uint64_t increment,maximum, hysteresis_quotient,downward_multiplier,upward_multiplier;
};
// Isolated Phase0 epoch component on an authenticated registry. Changes only
// effective balances; no epoch advance, rewards, state root or durable commit.
// Configuration/state errors and cancellation leave the input unchanged.
ConsensusSlashingState prepare_phase0_effective_balances(
 const ConsensusSlashingState&,const Phase0EffectiveBalanceRules&,
 const std::function<void()>& checkpoint={});
}
