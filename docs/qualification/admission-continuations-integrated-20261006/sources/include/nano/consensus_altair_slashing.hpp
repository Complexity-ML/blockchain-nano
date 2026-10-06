#pragma once
#include "nano/consensus_beacon_state.hpp"
#include "nano/consensus_ffg.hpp"
#include "nano/consensus_slashing.hpp"
namespace nano {
struct AltairSlashingRules {
 Phase0FfgRules duties;
 // Shared exit/penalty engine with Altair preset values. The Altair proposer
 // split 8/64 is exactly division by 8; no Phase0 penalty default is inherited.
 Phase0SlashingRules economics;
 uint64_t maximum_attesting_indices;
};
struct AltairSlashingCandidate {Bytes ssz;std::vector<uint64_t> slashed_indices;};
// On an admitted Altair state, derive proposer and proof domains from state,
// authenticate evidence and apply penalties to a private SSZ candidate. Registry
// keys require established PoPs; lifecycle validation remains strict. nullopt
// rejects an invalid/replayed proof. No enclosing block or durable commit.
std::optional<AltairSlashingCandidate> prepare_altair_proposer_slashing(
 std::span<const uint8_t>,const SignedConsensusBeaconHeader&,const SignedConsensusBeaconHeader&,
 const AltairStateLimits&,const AltairSlashingRules&,const std::function<void()>& checkpoint={});
std::optional<AltairSlashingCandidate> prepare_altair_attester_slashing(
 std::span<const uint8_t>,const IndexedConsensusAttestation&,const IndexedConsensusAttestation&,
 const AltairStateLimits&,const AltairSlashingRules&,const std::function<void()>& checkpoint={});
}
