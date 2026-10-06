#pragma once
#include "nano/consensus_deposit.hpp"
#include "nano/consensus_slashing.hpp"
namespace nano {
struct Phase0Deposit {
 ConsensusDepositMessage message;
 std::array<uint8_t,96> signature{};
 std::array<ConsensusRoot,33> proof{};
};
struct Phase0DepositRules {
 uint64_t effective_balance_increment,maximum_effective_balance,validator_registry_limit;
 ConsensusBytes4 genesis_fork_version;
};
struct Phase0DepositState {
 ConsensusSlashingState registry;
 ConsensusRoot deposit_root;
 uint64_t deposit_count,next_deposit_index;
 bool operator==(const Phase0DepositState&)const=default;
};
enum class Phase0DepositOutcome { Admitted, ToppedUp, IgnoredInvalidSignature };
ConsensusRoot consensus_deposit_data_root(const Phase0Deposit& deposit);
// Requires an authenticated Eth1 deposit root/count and registry. Merkle proof
// checks the next ordered index. New keys require BLS PoP; existing keys may be
// topped up without a new signature and cannot change withdrawal credentials.
// An invalid new-key signature consumes the proven deposit index without adding
// a validator, as prescribed by Phase0. nullopt means invalid proof/order.
// Atomic in-memory only; caller persists root/index/registry with the full block.
std::optional<Phase0DepositOutcome> process_phase0_deposit(Phase0DepositState& state,
 const Phase0DepositRules& rules,const Phase0Deposit& deposit);
Json phase0_deposit_snapshot(const Phase0DepositState& state,const Phase0DepositRules& rules,const Phase0SlashingRules& registry_rules);
Phase0DepositState phase0_deposit_from_snapshot(const Json& snapshot,const Phase0DepositRules& rules,const Phase0SlashingRules& registry_rules);
}
