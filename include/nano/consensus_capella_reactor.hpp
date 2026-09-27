#pragma once
#include "nano/consensus_capella_store_snapshot.hpp"
#include "nano/storage.hpp"
namespace nano {
struct CapellaPublishedStore {std::shared_ptr<const CapellaForkStore> state;ForkCheckpointToken token;};
// Serialized local event coordinator. Initial anchor or restart token must be
// independently trusted. No local signing, P2P, RPC cache or fork activation.
// Own ChainStore for this object's lifetime; callbacks must not re-enter it.
class CapellaDurableReactor {
 ChainStore& disk_;Int chain_;Bytes genesis_;CapellaForkBlockRules rules_;CapellaStoreSnapshotLimits limits_;
 mutable std::mutex mutex_;std::shared_ptr<const CapellaForkStore> state_;std::optional<ForkCheckpointToken> token_;
 CapellaStoreSnapshotCache cache_;bool unavailable_=false;
 void require_available()const;
 void publish(CapellaForkStore,const std::function<void()>&,const std::vector<CheckpointArchiveEntry>& archive={});
public:
 CapellaDurableReactor(ChainStore&,Int,Bytes,CapellaForkBlockRules,CapellaStoreSnapshotLimits,CapellaForkStore,const std::function<void()>& checkpoint={});
 CapellaDurableReactor(ChainStore&,Int,Bytes,CapellaForkBlockRules,CapellaStoreSnapshotLimits,const ForkCheckpointToken&,const std::function<void()>& checkpoint={});
 CapellaPublishedStore current()const;
 // Exact admitted wire bytes, authenticated against the current Store. Anchors
 // and legacy checkpoints may lack archives; unknown/pruned roots return empty.
 std::optional<Bytes> signed_block(const ConsensusRoot&,const std::function<void()>& checkpoint={})const;
 void tick(uint64_t time_ms,const std::function<void()>& checkpoint={});
 bool block(std::span<const uint8_t>,const std::function<void()>& checkpoint={});
 bool vote(const Phase0SignedAttestation&,const std::function<void()>& checkpoint={});
 bool slashing(const IndexedConsensusAttestation&,const IndexedConsensusAttestation&,const std::function<void()>& checkpoint={});
};
}
