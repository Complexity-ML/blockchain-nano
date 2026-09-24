#pragma once
#include "nano/consensus_randao.hpp"
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
}
