#pragma once
#include "nano/consensus_ffg.hpp"
namespace nano {
struct Phase0RewardRules {
 Phase0FfgRules duties;
 uint64_t base_reward_factor,base_rewards_per_epoch,proposer_reward_quotient;
 uint64_t minimum_epochs_to_inactivity_penalty,inactivity_penalty_quotient;
};
struct Phase0RewardCandidate {
 ConsensusSlashingState registry_after;
 std::vector<uint64_t> rewards,penalties;
 bool inactivity_leak=false;
};
// Phase0 process_rewards_and_penalties, after justification/finalization and
// before registry/effective-balance updates. Pending records MUST come from
// authenticated block inclusion; this is not a signed-vote admission API.
// Current-epoch active balance determines rewards for previous-epoch duties.
// Private candidate only: no epoch advancement, replay marker or durable commit.
Phase0RewardCandidate prepare_phase0_rewards(const Phase0FfgState&,
 const ConsensusSlashingState&,const ConsensusRandaoState&,const Phase0RewardRules&,
 const std::function<void()>& checkpoint={});
}
