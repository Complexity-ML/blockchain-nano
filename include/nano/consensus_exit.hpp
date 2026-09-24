#pragma once
#include "nano/consensus_slashing.hpp"
namespace nano {
struct Phase0ExitRules {
 uint64_t minimum_churn_limit,churn_limit_quotient,max_seed_lookahead;
 uint64_t minimum_withdrawability_delay,shard_committee_period;
};
struct SignedConsensusVoluntaryExit {
 uint64_t epoch,validator_index;
 Bytes signature;
};
ConsensusRoot consensus_voluntary_exit_root(uint64_t epoch,uint64_t validator_index);
// Phase0 voluntary exit: caller authenticates the registry and fork version for
// the signed exit epoch (not the processing epoch). This is NOT EIP-7044's
// permanently valid Capella-domain rule or EIP-7002 withdrawal-key exits.
// Validates signature, active tenure and absence of a previous exit; schedules
// through the same queue as slashing. No balance debit or payout.
// Invalid/replayed requests return false unchanged; bad state/configuration or
// arithmetic overflow throws unchanged. Caller owns full block/durable commit.
bool process_phase0_voluntary_exit(ConsensusSlashingState& state,const Phase0ExitRules& rules,
 const SignedConsensusVoluntaryExit& exit,const ConsensusBytes4& fork_version,const ConsensusRoot& genesis_root);
}
