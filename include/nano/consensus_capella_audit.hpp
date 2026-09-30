#pragma once
#include "nano/consensus_capella_reactor.hpp"
namespace nano {
struct Genesis;
Json audit_capella_database(const Genesis&,const std::filesystem::path&,const Json& request);
// Offline, read-only canonical block/state/index replay from an externally
// authenticated anchor. Does not reconstruct historical off-chain votes or
// certify historical fork choice. Caller must stop the writer or use a snapshot.
Json audit_capella_canonical_history(ChainStore&,const Int& chain,const Bytes& genesis,
 const CapellaForkBlockRules&,const CapellaStoreSnapshotLimits&,
 const CapellaForkStore& trusted_anchor,const ForkCheckpointToken& expected,
 size_t maximum_blocks,size_t maximum_archive_bytes,
 const std::function<void()>& checkpoint={});
}
