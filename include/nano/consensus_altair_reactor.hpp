#pragma once
#include "nano/consensus_altair_store_snapshot.hpp"
#include "nano/storage.hpp"
#include "nano/consensus_beacon_signer.hpp"
#include <memory>
namespace nano {
using AltairSignedBlockArchive=std::map<ConsensusRoot,std::shared_ptr<const Bytes>>;
using AltairSignedVoteArchive=std::map<ConsensusRoot,std::shared_ptr<const Bytes>>;
struct AltairPublishedStore {std::shared_ptr<const AltairForkStore> state;ForkCheckpointToken token;std::shared_ptr<const AltairSignedBlockArchive> signed_blocks;std::shared_ptr<const AltairSignedVoteArchive> signed_votes;};
// Serialized durable event coordinator, NOT a P2P service or EVM activation.
// The caller owns ChainStore for this object's lifetime and binds its chain.
// Initial Store / restart commitment must be independently authenticated.
// Cancellation callbacks must not re-enter this serialized coordinator.
// Full snapshots per event are a correctness baseline, not a throughput claim.
class AltairDurableReactor {
 ChainStore& disk_;
 const Int chain_id_;const Bytes genesis_;
 const AltairForkBlockRules rules_;const AltairStoreSnapshotLimits limits_;
 std::mutex& mutex_;
 std::shared_ptr<const AltairForkStore> state_;
 std::optional<ForkCheckpointToken> token_;
 std::shared_ptr<const AltairSignedBlockArchive> signed_blocks_=std::make_shared<const AltairSignedBlockArchive>();
 std::shared_ptr<const AltairSignedVoteArchive> signed_votes_=std::make_shared<const AltairSignedVoteArchive>();
 bool unavailable_=false;
 void require_available() const;
 void publish(AltairForkStore,std::shared_ptr<const AltairSignedBlockArchive>,std::shared_ptr<const AltairSignedVoteArchive>,const std::function<void()>&);
public:
 AltairDurableReactor(ChainStore&,Int,Bytes,AltairForkBlockRules,AltairStoreSnapshotLimits,
  AltairForkStore trusted_initial,const std::function<void()>& checkpoint={});
 AltairDurableReactor(ChainStore&,Int,Bytes,AltairForkBlockRules,AltairStoreSnapshotLimits,
  const ForkCheckpointToken& expected,const std::function<void()>& checkpoint={});
 AltairDurableReactor(const AltairDurableReactor&)=delete;
 AltairDurableReactor& operator=(const AltairDurableReactor&)=delete;
 AltairPublishedStore current() const;
 // Current-slot local duty; no caller-supplied vote fields or fork domain.
 // local_clock must come from the validator service's independent local clock,
 // never a peer. Holds Store/journal gates through the bounded signing callback.
 // Callback must not re-enter either coordinator. No autonomous duty or outbox.
 std::optional<Phase0SignedAttestation> sign_attestation(uint64_t validator_index,
  uint64_t expected_slot,const std::function<uint64_t()>& local_clock,const ForkCheckpointToken& expected_store,
  BeaconSigningJournal&,const BeaconSigningJournal::Signer&,
  const std::function<void()>& checkpoint={});
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
