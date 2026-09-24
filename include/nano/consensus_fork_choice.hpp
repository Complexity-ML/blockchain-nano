#pragma once
#include "nano/consensus_slashing.hpp"
#include <set>
namespace nano {
struct Phase0ForkNode {
 uint64_t slot;
 ConsensusRoot parent;
 uint64_t realized_justified_epoch,unrealized_justified_epoch;
};
struct Phase0LatestMessage {uint64_t epoch;ConsensusRoot root;};
struct Phase0ForkChoiceView {
 ConsensusRoot anchor;
 ConsensusCheckpoint justified,finalized;
 uint64_t current_epoch;
 ConsensusSlashingState justified_registry;
 std::map<ConsensusRoot,Phase0ForkNode> nodes;
 std::map<uint64_t,Phase0LatestMessage> latest_messages;
 std::set<uint64_t> equivocating_indices;
 std::optional<ConsensusRoot> proposer_boost;
};
struct Phase0ForkChoiceRules {uint64_t slots_per_epoch,effective_balance_increment,proposer_score_boost;};
struct Phase0ForkChoiceResult {
 ConsensusRoot head;
 std::map<ConsensusRoot,Int> weights;
 std::set<ConsensusRoot> viable;
};
// Pure get_head/get_weight/filter_node_tree on AUTHENTICATED fork-choice data.
// Nodes must have passed block/state-root validation. Justified registry must
// be the checkpoint state; latest messages must have passed on_attestation.
// Equivocations, pulled-up checkpoints and timely boost come from the caller's
// validated event store. This function does NOT authenticate/update those data,
// advance time, implement fork-choice event handlers, persist or activate them.
// Uses iterative tree passes; weight sums cannot wrap uint64_t.
Phase0ForkChoiceResult phase0_fork_choice_head(const Phase0ForkChoiceView&,
 const Phase0ForkChoiceRules&,const std::function<void()>& checkpoint={});
}
