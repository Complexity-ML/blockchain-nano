#pragma once
#include "nano/consensus_signing.hpp"
namespace nano {
struct ConsensusCheckpoint {uint64_t epoch=0;ConsensusRoot root{};};
struct ConsensusAttestationData {
 uint64_t slot=0;
 uint64_t index=0;
 ConsensusRoot beacon_block_root{};
 ConsensusCheckpoint source,target;
};
// SSZ object roots only. Slot windows, checkpoint ancestry/justification and
// fork-specific committee index rules must be validated against consensus state.
ConsensusRoot consensus_checkpoint_root(const ConsensusCheckpoint& checkpoint);
ConsensusRoot consensus_attestation_root(const ConsensusAttestationData& data);
}
