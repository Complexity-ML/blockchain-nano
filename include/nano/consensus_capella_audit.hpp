#pragma once
#include "nano/consensus_capella_reactor.hpp"
namespace nano {
struct Genesis;
Json audit_capella_database(const Genesis&,const std::filesystem::path&,const Json& request);
// Same offline operation with caller-cooperative cancellation. The old symbol
// above remains available and delegates here; a request may add replayWindow.
Json audit_capella_database(const Genesis&,const std::filesystem::path&,const Json& request,
 const std::function<void()>& checkpoint);
struct CapellaCanonicalAuditReplayWindow {
 size_t maximum_live_blocks;
 size_t maximum_live_state_bytes;
 // Absolute NEW index under an existing canonical owner0700 directory, outside
 // the audited DB. Retained on success/refusal; caller owns its disposition.
 std::filesystem::path new_private_index;
};
// Offline, read-only canonical block/state/index replay from an externally
// authenticated anchor. Does not reconstruct historical off-chain votes or
// certify historical fork choice. Caller must stop the writer or use a snapshot.
// Optional externally captured finalized checkpoint must match the canonical
// ancestor at its epoch boundary, including empty slots; never derive it from DB.
Json audit_capella_canonical_history(ChainStore&,const Int& chain,const Bytes& genesis,
 const CapellaForkBlockRules&,const CapellaStoreSnapshotLimits&,
 const CapellaForkStore& trusted_anchor,const ForkCheckpointToken& expected,
 size_t maximum_blocks,size_t maximum_archive_bytes,
 const std::function<void()>& checkpoint={},
 const std::optional<ConsensusCheckpoint>& required_finalized=std::nullopt);
// Explicit bounded window; the total-history budgets above remain mandatory.
// No default arguments: old source/ABI calls retain their exact overload.
Json audit_capella_canonical_history(ChainStore&,const Int& chain,const Bytes& genesis,
 const CapellaForkBlockRules&,const CapellaStoreSnapshotLimits&,
 const CapellaForkStore& trusted_anchor,const ForkCheckpointToken& expected,
 size_t maximum_blocks,size_t maximum_archive_bytes,
 const CapellaCanonicalAuditReplayWindow&,
 const std::function<void()>& checkpoint,
 const std::optional<ConsensusCheckpoint>& required_finalized);
}
