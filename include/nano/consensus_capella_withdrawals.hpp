#pragma once
#include "nano/consensus_slashing.hpp"
#include "nano/engine.hpp"
namespace nano {
struct CapellaWithdrawal {
 uint64_t index,validator_index;std::array<uint8_t,20> address;uint64_t amount_gwei;
 bool operator==(const CapellaWithdrawal&)const=default;
 Json to_json()const;
};
struct CapellaWithdrawalRules {uint64_t maximum_effective_balance,maximum_per_payload,maximum_validators_per_sweep;};
struct CapellaWithdrawalState {
 ConsensusSlashingState registry;uint64_t next_withdrawal_index=0,next_validator_index=0;
 bool operator==(const CapellaWithdrawalState&)const=default;
};
// Authenticated Capella registry/cursors only, not Electra compounding rules.
std::vector<CapellaWithdrawal> expected_capella_withdrawals(const CapellaWithdrawalState&,
 const CapellaWithdrawalRules&,const std::function<void()>& checkpoint={});
std::optional<CapellaWithdrawalState> prepare_capella_withdrawals(const CapellaWithdrawalState&,
 const CapellaWithdrawalRules&,const std::vector<CapellaWithdrawal>& advertised,
 const std::function<void()>& checkpoint={});
struct CapellaWithdrawalExecutionCandidate {
 CapellaWithdrawalState consensus;State execution;std::vector<Receipt> receipts;
};
// Paired private candidate only. Caller authenticates block signature, slot,
// RANDAO and both parents; this is not full Capella block admission or persistence.
// Select withdrawals from consensus, then verify the complete Shanghai EVM block.
// Neither input is modified, including after a late execution failure.
CapellaWithdrawalExecutionCandidate verify_capella_withdrawal_execution(
 const CapellaWithdrawalState&,const CapellaWithdrawalRules&,const Header& execution_parent,
 const State& execution_state,const Block&,const Int& chain_id,ReplayPolicy,
 const std::function<Bytes(const Int&)>& block_hash={},const std::function<void()>& checkpoint={});
}
