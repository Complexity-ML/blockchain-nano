#pragma once
#include "nano/consensus_capella_fork_store.hpp"
#include "nano/consensus_altair_snapshot.hpp"
namespace nano {
// Canonical local consensus/execution configuration commitment.
Json capella_fork_profile(const CapellaForkBlockRules&);
// Explicit canonical profile, authenticated before parsing; no inferred defaults.
CapellaForkBlockRules load_capella_fork_profile(std::string_view,const Bytes& expected_digest,size_t maximum_bytes);
struct CapellaStoreSnapshotLimits {
 size_t envelope_bytes,blocks,messages,equivocations,state_bytes,total_state_bytes;
 uint64_t replay_slots,sync_trials;
};
// Process-local validation memo, never serialized or accepted from peers. Entries
// are reused only for byte-equivalent complete records under identical rules.
class CapellaStoreSnapshotCache {
 Json profile_;
 std::map<ConsensusRoot,Json> records_;
 friend std::string capella_store_snapshot_impl(const CapellaForkStore&,const CapellaForkBlockRules&,
  const CapellaStoreSnapshotLimits&,const std::function<void()>&,CapellaStoreSnapshotCache*,
  const std::function<void(const CapellaColdReference&,std::string_view)>*);
 friend std::string capella_store_snapshot(const CapellaForkStore&,const CapellaForkBlockRules&,
  const CapellaStoreSnapshotLimits&,const std::function<void()>&,CapellaStoreSnapshotCache*);
};
// Canonical LOCAL checkpoint of an already authenticated Store, including votes,
// branches, clock and all checkpoints. Not peer admission. Caller atomically
// persists bytes and an independently trusted digest/sequence; this API does not
// prevent rollback of that external commitment and does not perform disk I/O.
std::string capella_store_snapshot(const CapellaForkStore&,const CapellaForkBlockRules&,
 const CapellaStoreSnapshotLimits&,const std::function<void()>& checkpoint={},CapellaStoreSnapshotCache* cache=nullptr);
// LOCAL reference-only manifest. Validate one complete row at a time, emit new
// hot rows synchronously, reuse authenticated cold rows without emitting them.
// The sink must stage only: callbacks can precede a later validation failure.
// Per-row/manifest bounds apply; no total flat-state budget or JSON memo is used.
// Caller must commit every referenced blob and manifest atomically. Not runtime
// activation; legacy restoration intentionally does not accept this format.
std::string capella_store_reference_snapshot(const CapellaForkStore&,const CapellaForkBlockRules&,
 const CapellaStoreSnapshotLimits&,
 const std::function<void(const CapellaColdReference&,std::string_view)>& stage,
 const std::function<void()>& checkpoint={});
// Verify external digest before JSON parsing. Rebuild the justified registry
// from authenticated SSZ rather than accepting duplicate serialized weights.
CapellaForkStore capella_store_from_snapshot(std::string_view,const Bytes& expected_digest,
 const CapellaForkBlockRules&,const CapellaStoreSnapshotLimits&,
 const std::function<void()>& checkpoint={});
}
