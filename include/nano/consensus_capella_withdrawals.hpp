#pragma once
#include "nano/consensus_slashing.hpp"
#include "nano/engine.hpp"
#include "nano/consensus_beacon_state.hpp"
namespace nano {
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
struct CapellaWithdrawalSszCandidate {Bytes state;ConsensusRoot root;};
// Isolated process_withdrawals on an independently authenticated Capella state.
// Returns updated balances/cursors and their SSZ root; all other bytes survive.
// Caller enforces block ordering, replay protection and durable publication.
std::optional<CapellaWithdrawalSszCandidate> prepare_capella_withdrawal_state(
 std::span<const uint8_t>,const ConsensusRoot& authenticated_root,
 const AltairStateLimits&,const ExecutionPayloadLimits&,size_t maximum_state_bytes,
 uint64_t slots_per_epoch,const CapellaWithdrawalRules&,
 const std::vector<CapellaWithdrawal>& advertised,const std::function<void()>& checkpoint={});
}
