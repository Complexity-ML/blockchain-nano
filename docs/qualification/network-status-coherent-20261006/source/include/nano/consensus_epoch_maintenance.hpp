#pragma once
#include "nano/consensus_ffg.hpp"
#include <span>
namespace nano {
struct Phase0Eth1Data {
 ConsensusRoot deposit_root{};
 uint64_t deposit_count=0;
 ConsensusRoot block_hash{};
 bool operator==(const Phase0Eth1Data&)const=default;
};
struct Phase0HistoryRules {
 uint64_t slots_per_epoch,slots_per_historical_root,historical_roots_limit;
};
// Isolated epoch components over authenticated state. No slot/epoch increment,
// replay marker, full BeaconState root, durable commit or network admission.
// The epoch orchestrator applies each once, in the prescribed Phase0 order.
std::vector<Phase0Eth1Data> phase0_eth1_votes_after_reset(uint64_t epoch,
 std::span<const Phase0Eth1Data> votes,uint64_t epochs_per_voting_period);
std::vector<uint64_t> phase0_slashings_after_reset(uint64_t epoch,
 std::span<const uint64_t> slashings,uint64_t epochs_per_slashings_vector);
ConsensusRandaoState phase0_randao_after_reset(const ConsensusRandaoState&,
 const ConsensusRandaoRules&);
std::vector<ConsensusRoot> phase0_history_after_update(uint64_t epoch,
 std::span<const ConsensusRoot> block_roots,std::span<const ConsensusRoot> state_roots,
 std::span<const ConsensusRoot> historical_roots,const Phase0HistoryRules&,
 const std::function<void()>& checkpoint={});
Phase0FfgState phase0_participation_after_rotation(const Phase0FfgState&);
}
