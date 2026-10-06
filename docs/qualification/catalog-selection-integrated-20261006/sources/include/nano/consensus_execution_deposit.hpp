#pragma once
#include "nano/engine.hpp"
#include "nano/consensus_deposit_transition.hpp"
namespace nano {
inline constexpr const char* ethereum_deposit_contract="0x00000000219ab540356cbb839cbe05303d7705fa";
struct ExecutionDepositBatch {
 State state;
 std::vector<Receipt> receipts;
 // Ordered DepositData only; Merkle proofs are not populated here.
 std::vector<Phase0Deposit> deposits;
 uint64_t next_deposit_index;
};
// Replay an independently authenticated execution block against its parent.
// expected_block_hash, pinned_contract_code_hash and next_index must come from
// trusted chain configuration/branch state, never from the received packet.
// No mutation or persistence: caller must bind/persist this result to the branch.
// Does not perform Beacon admission, PoP, activation, or Eth1 voting.
ExecutionDepositBatch replay_execution_deposits(const ExecutionEngine& engine,
 const State& parent_state,const Header& parent,const Block& block,
 const Bytes& expected_block_hash,const Bytes& pinned_contract_code_hash,
 uint64_t next_index,size_t maximum_deposits,const std::function<void()>& checkpoint={});
}
