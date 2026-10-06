#pragma once
#include "nano/consensus_block_operations.hpp"
#include "nano/consensus_deposit_transition.hpp"
#include "nano/consensus_exit.hpp"
#include <variant>
#include "nano/consensus_attestation_inclusion.hpp"
namespace nano {
struct InvalidPhase0RegistryBlock {};
struct UnsupportedPhase0RegistryOperations {
 size_t attester_slashings,attestations;
};
struct Phase0AttestationStage {
 const Phase0FfgState& state;
 const Phase0AttestationInclusionRules& rules;
};
struct Phase0RegistryOperationsCandidate {
 AuthenticatedPhase0Block authenticated;
 Phase0DepositState deposits_after;
 size_t proposer_slashings_processed,deposits_processed,exits_processed;
 std::optional<Phase0FfgState> attestations_after;
 size_t attestations_processed=0;
 size_t attester_slashings_processed=0;
};
using Phase0RegistryOperationsResult=std::variant<InvalidPhase0RegistryBlock,
 UnsupportedPhase0RegistryOperations,Phase0RegistryOperationsCandidate>;
// Partial Phase0 block pipeline: authentication, proposer/attester slashings, deposits,
// then voluntary exits, all on private state. With attestation_stage, validates
// and stages attestations before deposits. Without it, nonempty attestations are
// Unsupported. All supplied slashing evidence is verified against registry keys.
// Checks the mandatory
// number of deposits against authenticated root/count after Eth1 voting.
// Caller supplies immutable, epoch-advanced state and authenticated Eth1 data.
// Does NOT implement Eth1 voting, slots/epochs, state_root or durable commit;
// even a candidate with all operations processed is NOT full block validation.
Phase0RegistryOperationsResult prepare_phase0_registry_operations(
 std::span<const uint8_t> signed_block,const Phase0BlockContext& context,
 const Phase0BodyLimits& body_limits,const ConsensusRandaoRules& randao_rules,
 const Phase0SlashingRules& slashing_rules,const Phase0DepositRules& deposit_rules,
 const Phase0ExitRules& exit_rules,const Phase0StateFork& fork,
 const Phase0DepositState& state,const ConsensusRandaoState& randao,
 const std::function<void()>& checkpoint={},
 const Phase0AttestationStage* attestation_stage=nullptr);
}
