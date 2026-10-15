#pragma once
// ROOT-run synthetic lifecycle qualification support; no signing/publication.
#include "nano/consensus_capella_store_snapshot.hpp"
#include "nano/storage.hpp"
#include <filesystem>
#include <functional>

namespace nano::test::lifecycle {
inline constexpr uint64_t canonical_stream_index_record_bytes=112;
struct CanonicalStreamLimits {
 // Live replay window only; these cannot exceed the caller's existing snapshot
 // limits. Total history has its separate authority-derived traversal budget.
 size_t maximum_live_blocks=256;
 size_t maximum_live_state_bytes=0;
 uint64_t maximum_total_blocks=0; // signed blocks AFTER the trusted anchor
 uint64_t maximum_total_archive_bytes=0;
 uint64_t maximum_private_index_bytes=0;
};
struct CanonicalStreamSource {
 // Caller owns stopped RO handles. Must enforce the supplied per-entry bound.
 std::function<std::optional<CheckpointArchiveEntry>(const ConsensusRoot&,size_t,
  const std::function<void()>&)> archive;
 // Mandatory RO canonical execution block/index/receipt validation, called for
 // anchor and every fully verified transition. Never grants consensus authority.
 std::function<void(const CapellaForkBlock&,const std::function<void()>&)> verify_execution;
};
struct CanonicalStreamBoundary {
 ConsensusBeaconHeader stopped_head;
 // Both come from independently authenticated stopped/observed commitments,
 // never inferred from the canonical replayer's off-chain fork-choice view.
 ConsensusCheckpoint stopped_finalized;
 ConsensusCheckpoint required_finalized;
};
struct CanonicalStreamVisit {
 const CapellaForkBlock* parent; // null only for ordinal0 trusted anchor
 const CapellaForkBlock& block;
 std::span<const uint8_t> signed_wire; // empty only for trusted anchor
 ConsensusRoot root;
 uint64_t ordinal;
};
using CanonicalStreamVisitor=std::function<void(const CanonicalStreamVisit&,
 const std::function<void()>&)>;
struct CanonicalStreamResult {
 std::shared_ptr<const CapellaForkBlock> head;
 uint64_t verified_signed_blocks=0,archive_bytes=0,index_bytes=0,prunes=0;
 size_t peak_live_blocks=0,peak_live_state_ssz_bytes=0;
 // Counts include the caller-retained original anchor plus the independently
 // revalidated replay records. Reservation adds one maximum_state_bytes
 // candidate; neither metric is a process RSS claim.
 size_t peak_live_state_ssz_plus_next_reserve_bytes=0;
 bool required_finalized_ancestor_verified=false;
 bool historical_fork_choice_verified=false;
};
// Creates ONLY the non-existing private index file below a canonical owner0700
// parent (O_EXCL/0600); retains it on success/refusal. Source callbacks remain RO.
// Visitor references/wire expire on return; it must keep only bounded counters
// or event commitments, never whole-history states/wires. No mutable Store leaks.
inline CanonicalStreamResult stream_capella_canonical_history(
 const CapellaForkStore& trusted_anchor,const CapellaForkBlockRules&,
 const CapellaStoreSnapshotLimits&,const CanonicalStreamLimits&,
 const CanonicalStreamBoundary&,const CanonicalStreamSource&,
 const std::filesystem::path& new_private_index,
 const CanonicalStreamVisitor&,const std::function<void()>& checkpoint={});
}
#include "capella_canonical_stream.ipp"
