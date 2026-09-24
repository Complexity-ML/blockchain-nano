#pragma once
#include "nano/consensus_fork_choice.hpp"
#include "nano/consensus_attestation_inclusion.hpp"
namespace nano {
// Derived from the authenticated state AT target epoch start, including any
// skipped-slot transitions. The caller must bind this state to checkpoint.root,
// establish registry key PoPs and serialize access to the fork-choice view.
struct Phase0ForkVoteContext {
 ConsensusCheckpoint checkpoint;
 ConsensusSlashingState registry;
 ConsensusRandaoState randao;
 Phase0StateFork fork;
 ConsensusRoot genesis_root;
};
// Admission + latest-message update, not a complete network event Store.
// from_verified_block may ONLY be set by a caller that authenticated the block.
// False means reject/defer (unknown roots or a future slot may become valid).
// No queue, state retrieval, slashing proof processing or persistence is supplied.
// False, exception or cancellation leaves latest_messages unchanged.
bool process_phase0_fork_vote(Phase0ForkChoiceView& view,uint64_t current_slot,
 const Phase0ForkVoteContext&,const Phase0FfgRules&,const Phase0SignedAttestation&,
 bool from_verified_block=false,const std::function<void()>& checkpoint={});
}
