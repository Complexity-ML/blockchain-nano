#pragma once
#include "nano/consensus_signing.hpp"
namespace nano {
struct ConsensusCheckpoint {uint64_t epoch=0;ConsensusRoot root{};bool operator==(const ConsensusCheckpoint&)const=default;};
struct ConsensusAttestationData {
 uint64_t slot=0;
 uint64_t index=0;
 ConsensusRoot beacon_block_root{};
 ConsensusCheckpoint source,target;
 bool operator==(const ConsensusAttestationData&)const=default;
};
// SSZ object roots only. Slot windows, checkpoint ancestry/justification and
// fork-specific committee index rules must be validated against consensus state.
// Data predicate only: authenticate BOTH signatures and intersect validator
// indices before applying any penalty. Identical repeat delivery is not slashable.
bool is_slashable_attestation_data(const ConsensusAttestationData& first,const ConsensusAttestationData& second);
ConsensusRoot consensus_checkpoint_root(const ConsensusCheckpoint& checkpoint);
ConsensusRoot consensus_attestation_root(const ConsensusAttestationData& data);
}
