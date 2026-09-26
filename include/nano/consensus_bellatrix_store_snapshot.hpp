#pragma once
#include "nano/consensus_bellatrix_fork_store.hpp"
#include "nano/consensus_altair_snapshot.hpp"
namespace nano {
// Canonical local consensus/execution configuration commitment.
Json bellatrix_fork_profile(const BellatrixForkBlockRules&);
// Explicit canonical profile, authenticated before parsing; no inferred defaults.
BellatrixForkBlockRules load_bellatrix_fork_profile(std::string_view,const Bytes& expected_digest,size_t maximum_bytes);
struct BellatrixStoreSnapshotLimits {
 size_t envelope_bytes,blocks,messages,equivocations,state_bytes,total_state_bytes;
 uint64_t replay_slots,sync_trials;
};
// Canonical LOCAL checkpoint of an already authenticated Store, including votes,
// branches, clock and all checkpoints. Not peer admission. Caller atomically
// persists bytes and an independently trusted digest/sequence; this API does not
// prevent rollback of that external commitment and does not perform disk I/O.
std::string bellatrix_store_snapshot(const BellatrixForkStore&,const BellatrixForkBlockRules&,
 const BellatrixStoreSnapshotLimits&,const std::function<void()>& checkpoint={});
// Verify external digest before JSON parsing. Rebuild the justified registry
// from authenticated SSZ rather than accepting duplicate serialized weights.
BellatrixForkStore bellatrix_store_from_snapshot(std::string_view,const Bytes& expected_digest,
 const BellatrixForkBlockRules&,const BellatrixStoreSnapshotLimits&,
 const std::function<void()>& checkpoint={});
}
