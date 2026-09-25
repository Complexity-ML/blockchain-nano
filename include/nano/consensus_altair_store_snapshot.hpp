#pragma once
#include "nano/consensus_altair_fork_store.hpp"
#include "nano/consensus_altair_snapshot.hpp"
namespace nano {
struct AltairStoreSnapshotLimits {
 size_t envelope_bytes,blocks,messages,equivocations,state_bytes,total_state_bytes;
 uint64_t replay_slots,sync_trials;
};
// Canonical LOCAL checkpoint of an already authenticated Store, including votes,
// branches, clock and all checkpoints. Not peer admission. Caller atomically
// persists bytes and an independently trusted digest/sequence; this API does not
// prevent rollback of that external commitment and does not perform disk I/O.
std::string altair_store_snapshot(const AltairForkStore&,const AltairForkBlockRules&,
 const AltairStoreSnapshotLimits&,const std::function<void()>& checkpoint={});
// Verify external digest before JSON parsing. Rebuild the justified registry
// from authenticated SSZ rather than accepting duplicate serialized weights.
AltairForkStore altair_store_from_snapshot(std::string_view,const Bytes& expected_digest,
 const AltairForkBlockRules&,const AltairStoreSnapshotLimits&,
 const std::function<void()>& checkpoint={});
}
