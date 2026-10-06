#pragma once
#include "nano/consensus_capella_fork_store.hpp"
namespace nano {
// Local trusted-store maintenance. Validate against the retained anchor history;
// these commitments are not a peer-admission proof by themselves.
std::vector<CapellaDepositAncestor> capella_deposit_ancestors_before(const CapellaForkStore&,
 const ConsensusRoot& anchor,const std::function<void()>& checkpoint={});
void validate_capella_deposit_ancestors(const CapellaForkStore&,const std::function<void()>& checkpoint={});

// Nano native execution-deposit profile, not Ethereum's external Eth1 timing
// selection. Vote for the last execution ancestor before this voting period,
// falling back to the authenticated anchor. Stable throughout the period.
Bytes capella_native_deposit_vote(const CapellaForkStore&,const ConsensusRoot& parent,
 uint64_t slot,const CapellaForkBlockRules&,const std::function<void()>& checkpoint={});
bool capella_native_deposit_vote_known(const CapellaForkStore&,const ConsensusRoot& parent,
 std::span<const uint8_t> vote,const std::function<void()>& checkpoint={});
// Takes a validated, slot-advanced Capella state. Predicts the exact Eth1 vote
// update in process_block, then returns the mandatory ordered deposit SSZ list.
Bytes capella_native_deposit_operations(const CapellaForkStore&,const ConsensusRoot& parent,
 std::span<const uint8_t> advanced_state,std::span<const uint8_t> vote,
 const CapellaForkBlockRules&,const std::function<void()>& checkpoint={});
}
