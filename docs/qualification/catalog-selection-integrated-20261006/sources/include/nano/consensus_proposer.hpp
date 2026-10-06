#pragma once
#include "nano/consensus_randao.hpp"
#include "nano/consensus_slashing.hpp"
#include <functional>
namespace nano {
struct Phase0ProposerValidator {uint64_t index,effective_balance,activation_epoch,exit_epoch;};
// Caller supplies the authenticated registry in increasing index order and the
// fork's preset. Phase0 algorithm (8-bit rejection sampling), not Electra.
// Slashed validators are not silently removed: Phase0 block validation must
// separately reject a selected slashed proposer. No changes to input state.
// checkpoint may abort work; cancellation is not proof that a block is invalid.
uint64_t phase0_proposer_index(std::span<const Phase0ProposerValidator> registry,
 const ConsensusRandaoState& randao,const ConsensusRandaoRules& rules,uint64_t slot,
 uint64_t maximum_effective_balance,uint16_t shuffle_rounds,const std::function<void()>& checkpoint={});

// Joint proposer/RANDAO gate over the authenticated, funded registry for this
// epoch. Resolves the public key internally; rejects slashed selected proposers
// without rerolling. All failures/cancellation leave randao unchanged.
// This is not full block validation: signature/header/body and state commitment
// must still be checked and the resulting state committed atomically by caller.
bool process_phase0_selected_randao(ConsensusRandaoState& randao,
 const ConsensusSlashingState& registry,const ConsensusRandaoRules& rules,
 uint64_t slot,uint64_t claimed_proposer,uint64_t maximum_effective_balance,uint16_t shuffle_rounds,
 const ConsensusBytes4& fork_version,const ConsensusRoot& genesis_root,
 std::span<const uint8_t> reveal,const std::function<void()>& checkpoint={});
}
