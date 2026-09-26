#pragma once
#include "nano/consensus_bellatrix_fork_store.hpp"
namespace nano {
// Nano native execution-deposit profile, not Ethereum's external Eth1 timing
// selection. Vote for the last execution ancestor before this voting period,
// falling back to the authenticated anchor. Stable throughout the period.
Bytes bellatrix_native_deposit_vote(const BellatrixForkStore&,const ConsensusRoot& parent,
 uint64_t slot,const BellatrixForkBlockRules&,const std::function<void()>& checkpoint={});
bool bellatrix_native_deposit_vote_known(const BellatrixForkStore&,const ConsensusRoot& parent,
 std::span<const uint8_t> vote,const std::function<void()>& checkpoint={});
// Takes a validated, slot-advanced Bellatrix state. Predicts the exact Eth1 vote
// update in process_block, then returns the mandatory ordered deposit SSZ list.
Bytes bellatrix_native_deposit_operations(const BellatrixForkStore&,const ConsensusRoot& parent,
 std::span<const uint8_t> advanced_state,std::span<const uint8_t> vote,
 const BellatrixForkBlockRules&,const std::function<void()>& checkpoint={});
}
