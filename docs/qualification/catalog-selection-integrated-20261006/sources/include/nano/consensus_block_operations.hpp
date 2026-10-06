#pragma once
#include "nano/consensus_block_auth.hpp"
namespace nano {
// The authenticated BeaconState Fork used by Phase0 get_domain, including for
// evidence whose slot differs from the block slot. Future evidence uses current.
struct Phase0StateFork {
 ConsensusBytes4 previous_version,current_version;
 uint64_t epoch;
};
struct Phase0ProposerOperationsCandidate {
 AuthenticatedPhase0Block authenticated;
 ConsensusSlashingState registry_after_proposer_slashings;
 size_t proposer_slashings_processed;
 // Attester slashings, attestations, deposits, exits remain to be processed.
 std::array<size_t,4> unprocessed_operations;
};
// Authenticate the signed block and apply its proposer slashings, in order, to
// private candidates. Every rejection/exception leaves both source states intact.
// Does not process Eth1 voting, other operations, slots/epochs or state_root.
// Even all-zero unprocessed_operations counts is NOT complete block validation.
// The caller must hold immutable input snapshots during the entire operation.
std::optional<Phase0ProposerOperationsCandidate> prepare_phase0_proposer_operations(
 std::span<const uint8_t> signed_block,const Phase0BlockContext& context,
 const Phase0BodyLimits& body_limits,const ConsensusRandaoRules& randao_rules,
 const Phase0SlashingRules& slashing_rules,const Phase0StateFork& fork,
 const ConsensusSlashingState& registry,const ConsensusRandaoState& randao,
 const std::function<void()>& checkpoint={});
}
