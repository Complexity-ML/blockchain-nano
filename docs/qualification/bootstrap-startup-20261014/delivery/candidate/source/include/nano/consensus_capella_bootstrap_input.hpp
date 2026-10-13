#pragma once
#include "nano/consensus_capella_bootstrap_archive.hpp"
#include "nano/consensus_capella_store_snapshot.hpp"
namespace nano {
// Load only externally pinned PUBLIC inputs for an ALREADY signed slot0->slot1
// transition. The descriptor is exactly format/genesisState/signedBlock; each
// artifact is exactly path/sha256/bytes. SHA256 is lowercase hex without 0x.
// Paths must be absolute, lexically normal and have no symlink component.
// Genesis/rules/limits/descriptor are owned before any caller callback. Files
// stay open read-only; owned bytes are hashed before full transition validation
// and checked again through the same descriptors before returning the proof.
// The caller authenticates its Genesis/profile and descriptor externally. This
// function neither supplies that authority nor publishes/imports any state.
// Invalid schema/pins/rejected transitions throw invalid_argument; I/O errors,
// allocation/resource failures and caller cancellation propagate. No signer,
// journal, DB, clock, mutation, validator-count constant or Boolean trust input.
CapellaSignedBootstrapArchive load_capella_signed_bootstrap_proof(
 Genesis,CapellaForkBlockRules,CapellaStoreSnapshotLimits,Json descriptor,
 const std::function<void()>& checkpoint={});
}
