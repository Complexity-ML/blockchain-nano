#pragma once
#include "nano/consensus_ffg.hpp"
#include "nano/consensus_block_operations.hpp"
namespace nano {
struct Phase0SignedAttestation {
 ConsensusAttestationData data;
 std::vector<bool> aggregation_bits;
 Bytes signature;
};
struct Phase0AttestationInclusionRules {
 Phase0FfgRules duties;
 uint64_t minimum_inclusion_delay,maximum_pending_attestations;
};
// Authenticated, funded registry must establish PoP for every key (including
// genesis validators) before FastAggregateVerify. Keys/weights/committees are
// never supplied by the attestation. Caller supplies immutable consensus context.
// Phase0 source, epoch, inclusion window, committee bits and BLS admission.
// Appends only after all checks; false/exception leaves state unchanged. Slashed
// participants may be included, but FFG excludes their weight. Head/target roots
// are NOT required to match history by Phase0 admission; FFG filters targets.
// Inclusion metadata retains the selected proposer and delay for epoch rewards.
// Does not authenticate the enclosing block or commit a full state transition.
bool process_phase0_attestation(Phase0FfgState& state,
 const ConsensusSlashingState& registry,const ConsensusRandaoState& randao,
 const Phase0AttestationInclusionRules& rules,const Phase0StateFork& fork,
 const ConsensusRoot& genesis_root,const Phase0SignedAttestation& attestation,
 const std::function<void()>& checkpoint={});
}
