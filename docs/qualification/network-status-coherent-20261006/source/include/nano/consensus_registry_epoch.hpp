#pragma once
#include "nano/consensus_slashing.hpp"
#include <functional>
namespace nano {
struct Phase0RegistryUpdateRules {
 uint64_t minimum_churn_limit,churn_limit_quotient,max_seed_lookahead;
 uint64_t minimum_withdrawability_delay,maximum_effective_balance,ejection_balance;
};
struct Phase0RegistryUpdateCandidate {
 ConsensusSlashingState registry_after;
 std::vector<uint64_t> newly_eligible,ejected,activated;
};
// Phase0 process_registry_updates component, on an immutable input snapshot.
// finalized_epoch must come from authenticated consensus, after justification/
// finalization. Eligibility alone cannot activate unfinalized deposits. Shares
// the slashing/voluntary-exit queue and schedules (not immediate) activation.
// No balance/effective-balance updates, epoch increment, durable commit or replay
// marker: full epoch orchestration must apply this component exactly once.
// Not Electra balance churn, EIP-7251 consolidation, or full epoch processing.
// Invalid configuration/state, overflow and cancellation throw without changes.
Phase0RegistryUpdateCandidate prepare_phase0_registry_updates(
 const ConsensusSlashingState& state,const Phase0RegistryUpdateRules& rules,
 uint64_t finalized_epoch,const std::function<void()>& checkpoint={});
}
