#pragma once
#include "nano/consensus_rewards.hpp"
#include "nano/consensus_registry_epoch.hpp"
#include "nano/consensus_effective_balance.hpp"
#include "nano/consensus_epoch_maintenance.hpp"
namespace nano {
struct Phase0EpochState {
 ConsensusSlashingState registry;
 ConsensusRandaoState randao;
 Phase0FfgState ffg;
 std::vector<Phase0Eth1Data> eth1_votes;
 std::vector<ConsensusRoot> state_roots,historical_roots;
 std::optional<uint64_t> last_processed_epoch;
 bool operator==(const Phase0EpochState&)const=default;
};
struct Phase0EpochRules {
 Phase0RewardRules rewards;
 Phase0RegistryUpdateRules registry;
 Phase0SlashingRules slashing;
 Phase0EffectiveBalanceRules effective;
 Phase0HistoryRules history;
 uint64_t epochs_per_randao_vector,epochs_per_eth1_voting_period,maximum_pending_attestations;
};
// Ordered Phase0 process_epoch on an authenticated end-of-slot state. The caller
// MUST have performed process_slot (cached block/state roots) first. Does not
// perform process_slot, advance slot, authenticate blocks or hash BeaconState.
// Candidate and replay marker must be committed together with the complete
// state. An absent marker is allowed for an authenticated imported state only.
// No persistence/network wiring here; invalid input/cancellation is atomic.
Phase0EpochState prepare_phase0_epoch(const Phase0EpochState&,const Phase0EpochRules&,
 uint64_t expected_epoch,const std::function<void()>& checkpoint={});
}
