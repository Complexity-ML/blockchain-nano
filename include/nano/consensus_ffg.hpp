#pragma once
#include "nano/consensus_slashing.hpp"
#include "nano/consensus_randao.hpp"
#include <functional>
namespace nano {
struct Phase0PendingAttestation {
 ConsensusAttestationData data;
 std::vector<bool> aggregation_bits;
 bool operator==(const Phase0PendingAttestation&)const=default;
};
struct Phase0FfgCheckpoints {
 uint8_t justification_bits=0;
 ConsensusCheckpoint previous_justified,current_justified,finalized;
 bool operator==(const Phase0FfgCheckpoints&)const=default;
};
struct Phase0FfgState {
 uint64_t slot=0;
 Phase0FfgCheckpoints checkpoints;
 std::vector<ConsensusRoot> block_roots;
 std::vector<Phase0PendingAttestation> previous_attestations,current_attestations;
 bool operator==(const Phase0FfgState&)const=default;
};
struct Phase0FfgRules {
 uint64_t slots_per_epoch,min_seed_lookahead,target_committee_size,max_committees_per_slot;
 uint64_t effective_balance_increment,maximum_effective_balance;
 uint16_t shuffle_rounds;
};
struct Phase0FfgCandidate {
 Phase0FfgCheckpoints checkpoints;
 Int total_active_balance,previous_target_balance,current_target_balance;
};
// Phase0 justification/finalization component over AUTHENTICATED persisted state.
// Pending attestations must already have passed block inclusion, BLS and source
// validation. They deliberately carry no signatures: NEVER accept gossip/wire
// votes here. Membership is rederived from registry/RANDAO, not caller indices.
// Repeated participation is unioned, slashed votes excluded, wrong targets ignored.
// No slot advancement, replay marker, fork choice, full epoch/state_root or commit.
// Caller applies once in epoch orchestration, before rewards and registry updates.
Phase0FfgCandidate prepare_phase0_justification_finalization(
 const Phase0FfgState& state,const ConsensusSlashingState& registry,
 const ConsensusRandaoState& randao,const Phase0FfgRules& rules,
 const std::function<void()>& checkpoint={});
}
