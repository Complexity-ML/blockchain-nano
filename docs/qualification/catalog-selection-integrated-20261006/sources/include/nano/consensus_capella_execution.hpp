#pragma once
#include "nano/consensus_bellatrix_execution.hpp"
#include "nano/consensus_capella_withdrawals.hpp"
namespace nano {
// Continuation after Capella activation. Input is authenticated after slots/header,
// before RANDAO and operations. Produces both private states or throws; no commit.
// Withdrawal debit precedes execution; credential changes follow this operation.
BellatrixPayloadStateCandidate prepare_capella_payload_state(
 std::span<const uint8_t>,const ConsensusRoot& authenticated_state_root,
 const AltairStateLimits&,const ExecutionPayloadLimits&,size_t maximum_state_bytes,
 uint64_t slots_per_epoch,uint64_t seconds_per_slot,const CapellaWithdrawalRules&,
 std::span<const uint8_t> payload,size_t maximum_payload_bytes,
 const Header& execution_parent,const State& execution_state,const Int& chain_id,
 ReplayPolicy,const std::function<Bytes(const Int&)>& authenticated_block_hash,
 const std::function<void()>& checkpoint={});
}
