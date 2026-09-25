#pragma once
#include "nano/consensus_execution_payload.hpp"
#include "nano/engine.hpp"
namespace nano {
// Caller supplies authenticated consensus context and canonical execution history.
// This prepares a candidate only: no fork choice, storage commit or activation.
struct BellatrixExecutionCandidate {
 Block block;
 State state;
 std::vector<Receipt> receipts;
};
BellatrixExecutionCandidate prepare_bellatrix_execution(
 std::span<const uint8_t> payload_ssz,const ExecutionPayloadLimits&,size_t maximum_bytes,
 const Header& authenticated_parent,const State& parent_state,
 uint64_t expected_timestamp,const ConsensusRoot& expected_randao,
 const Int& chain_id,ReplayPolicy,
 const std::function<Bytes(const Int&)>& authenticated_block_hash,
 const std::function<void()>& checkpoint={});
}
