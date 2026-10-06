#pragma once
#include "nano/consensus_capella_withdrawals.hpp"
namespace nano {
struct SignedCapellaBlsChange {
 uint64_t validator_index=0;
 std::array<uint8_t,48> from_bls_pubkey{};
 std::array<uint8_t,20> to_execution_address{};
 std::array<uint8_t,96> signature{};
};
std::optional<SignedCapellaBlsChange> decode_capella_bls_change(std::span<const uint8_t>);
Bytes encode_capella_bls_change(const SignedCapellaBlsChange&);
ConsensusRoot capella_bls_change_root(const SignedCapellaBlsChange&);
// Atomic isolated operation list on an authenticated Capella SSZ state.
// genesis_version and maximum_changes come from the trusted chain profile,
// never from the signed request. No activation/exit restriction is imposed.
// Caller enforces fork activation, operation ordering and whole-block commit.
// Withdrawal processing precedes these operations in a Capella block.
std::optional<CapellaWithdrawalSszCandidate> prepare_capella_bls_changes(
 std::span<const uint8_t>,const ConsensusRoot& authenticated_root,
 const AltairStateLimits&,const ExecutionPayloadLimits&,size_t maximum_state_bytes,
 const ConsensusBytes4& genesis_version,size_t maximum_changes,
 std::span<const SignedCapellaBlsChange>,const std::function<void()>& checkpoint={});
}
