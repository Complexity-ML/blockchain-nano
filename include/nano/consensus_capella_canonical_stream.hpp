#pragma once
#include "nano/consensus_capella_store_snapshot.hpp"
#include "nano/storage.hpp"
#include "nano/consensus_canonical_deposit_context.hpp"
#include <filesystem>
#include <functional>

namespace nano {
inline constexpr uint64_t canonical_stream_index_record_bytes=112;
struct CanonicalStreamLimits {
 // Explicit live replay window; cannot exceed the existing snapshot limits.
 // Total history and the private disk index have separate traversal budgets.
 size_t maximum_live_blocks=0;
 size_t maximum_live_state_bytes=0;
 uint64_t maximum_total_blocks=0; // signed blocks AFTER the trusted anchor
 uint64_t maximum_total_archive_bytes=0;
 uint64_t maximum_private_index_bytes=0;
 // Optional explicit local scratch policy. Absent uses a documented policy
 // derived from the traversal/window budgets; this never changes admission.
 std::optional<CanonicalDepositContextLimits> deposit_context_limits{};
};
struct CanonicalStreamSource {
 // Caller owns stopped RO handles and enforces the supplied per-entry bound.
 std::function<std::optional<CheckpointArchiveEntry>(const ConsensusRoot&,size_t,
  const std::function<void()>&)> archive;
 // Mandatory canonical execution block/index/receipt validation for anchor and
 // every fully verified transition. Never grants consensus authority.
 std::function<void(const CapellaForkBlock&,const std::function<void()>&)> verify_execution;
};
struct CanonicalStreamBoundary {
 ConsensusBeaconHeader stopped_head;
 // Independently authenticated stopped/observed commitments; never inferred
 // from the canonical replayer's incomplete historical fork-choice view.
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
 // Includes caller-retained original anchor plus revalidated replay records.
 // Adds one maximum_state_bytes candidate reservation, not all temporary copies.
 // None of these metrics is a process RSS or total heap bound.
 size_t peak_live_state_ssz_plus_next_reserve_bytes=0;
 // Logical auxiliary maxima, deliberately NOT included in the SSZ byte metric.
 // History count is the largest cumulative deposit count on any retained record,
 // not a count of physical shared tree nodes, unique allocations or bytes.
 size_t peak_deposit_ancestor_records=0;
 uint64_t largest_deposit_history_count=0;
 // Diagnostic final warm anchor, not an independently authenticated checkpoint.
 ConsensusRoot replay_anchor{};
 uint64_t replay_anchor_slot=0;
 // Logical scratch/cache metrics only. Input/checkpoint/OS memory is separate.
 Json deposit_context=Json(nullptr);
 bool required_finalized_ancestor_verified=false;
 bool historical_fork_choice_verified=false;
};
// Creates only a NEW private index (O_EXCL/0600) under canonical owner0700 parent.
// Retains that scratch file on success/refusal; source callbacks remain RO.
// Visitor references/wire expire on return: retain bounded counters/commitments,
// never whole-history states/wires. No mutable store escapes. An explicit
// non-empty cancellation callback is required, including for non-cancelling users.
CanonicalStreamResult stream_capella_canonical_history(
 const CapellaForkStore& trusted_anchor,const CapellaForkBlockRules&,
 const CapellaStoreSnapshotLimits&,const CanonicalStreamLimits&,
 const CanonicalStreamBoundary&,const CanonicalStreamSource&,
 const std::filesystem::path& new_private_index,
 const CanonicalStreamVisitor&,const std::function<void()>& checkpoint={});
}
