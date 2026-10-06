#pragma once
#include "nano/consensus_altair_fork_store.hpp"
namespace nano {
struct AltairProposerRules {
 uint64_t reorg_cutoff_ms,head_weight_threshold,parent_weight_threshold,
          maximum_epochs_since_finalization;
};
// Optional proposer optimization on an authenticated in-memory Store. All
// weights and timeliness come from admitted events. Does not mutate Store,
// select a proposer, sign a block or enable a network consensus profile.
// Requires a known head with known parent, no remaining boost on that head,
// and a proposed slot not preceding the finalized epoch. Invalid context throws.
ConsensusRoot altair_proposer_head(const AltairForkStore&,const ConsensusRoot& head,
 uint64_t proposed_slot,const AltairForkBlockRules&,const AltairProposerRules&,
 const std::function<void()>& checkpoint={});
}
