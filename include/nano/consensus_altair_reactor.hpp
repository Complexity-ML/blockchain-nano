#pragma once
#include "nano/consensus_altair_store_snapshot.hpp"
#include "nano/storage.hpp"
#include <memory>
namespace nano {
struct AltairPublishedStore {std::shared_ptr<const AltairForkStore> state;ForkCheckpointToken token;};
// Serialized durable event coordinator, NOT a P2P service or EVM activation.
// The caller owns ChainStore for this object's lifetime and binds its chain.
// Initial Store / restart commitment must be independently authenticated.
// Cancellation callbacks must not re-enter this serialized coordinator.
// Full snapshots per event are a correctness baseline, not a throughput claim.
class AltairDurableReactor {
 ChainStore& disk_;
 const Int chain_id_;const Bytes genesis_;
 const AltairForkBlockRules rules_;const AltairStoreSnapshotLimits limits_;
 mutable std::mutex mutex_;
 std::shared_ptr<const AltairForkStore> state_;
 std::optional<ForkCheckpointToken> token_;
 bool unavailable_=false;
 void require_available() const;
 void publish(AltairForkStore,const std::function<void()>&);
public:
 AltairDurableReactor(ChainStore&,Int,Bytes,AltairForkBlockRules,AltairStoreSnapshotLimits,
  AltairForkStore trusted_initial,const std::function<void()>& checkpoint={});
 AltairDurableReactor(ChainStore&,Int,Bytes,AltairForkBlockRules,AltairStoreSnapshotLimits,
  const ForkCheckpointToken& expected,const std::function<void()>& checkpoint={});
 AltairPublishedStore current() const;
 void tick(uint64_t time_ms,const std::function<void()>& checkpoint={});
 bool block(std::span<const uint8_t>,const std::function<void()>& checkpoint={});
 // Wire admission uses the configured fork schema/committee limit. There is
 // no sender-controlled "already included in a block" or time-window bypass.
 bool vote_ssz(std::span<const uint8_t>,const std::function<void()>& checkpoint={});
 bool slashing_ssz(std::span<const uint8_t>,const std::function<void()>& checkpoint={});
 bool vote(const Phase0SignedAttestation&,const std::function<void()>& checkpoint={});
 bool slashing(const IndexedConsensusAttestation&,const IndexedConsensusAttestation&,
  const std::function<void()>& checkpoint={});
};
}
