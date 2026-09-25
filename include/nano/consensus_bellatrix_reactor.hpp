#pragma once
#include "nano/consensus_bellatrix_store_snapshot.hpp"
#include "nano/storage.hpp"
#include <memory>
#include "nano/consensus_beacon_signer.hpp"
namespace nano {
using BellatrixSignedBlockArchive=std::map<ConsensusRoot,std::shared_ptr<const Bytes>>;
using BellatrixSignedVoteArchive=std::map<ConsensusRoot,std::shared_ptr<const Bytes>>;
struct BellatrixPublishedStore {std::shared_ptr<const BellatrixForkStore> state;ForkCheckpointToken token;std::shared_ptr<const BellatrixSignedBlockArchive> signed_blocks;std::shared_ptr<const BellatrixSignedVoteArchive> signed_votes;};
// Serialized durable event coordinator, NOT a P2P service or EVM activation.
// The caller owns ChainStore for this object's lifetime and binds its chain.
// Initial Store / restart commitment must be independently authenticated.
// Cancellation callbacks must not re-enter this serialized coordinator.
// Full snapshots per event are a correctness baseline, not a throughput claim.
class BellatrixDurableReactor {
 ChainStore& disk_;
 const Int chain_id_;const Bytes genesis_;
 const BellatrixForkBlockRules rules_;const BellatrixStoreSnapshotLimits limits_;
 std::mutex& mutex_;
 std::shared_ptr<const BellatrixForkStore> state_;
 std::optional<ForkCheckpointToken> token_;
 std::shared_ptr<const BellatrixSignedBlockArchive> signed_blocks_=std::make_shared<const BellatrixSignedBlockArchive>();
 std::shared_ptr<const BellatrixSignedVoteArchive> signed_votes_=std::make_shared<const BellatrixSignedVoteArchive>();
 bool unavailable_=false;
 void require_available() const;
 void publish(BellatrixForkStore,std::shared_ptr<const BellatrixSignedBlockArchive>,std::shared_ptr<const BellatrixSignedVoteArchive>,const std::function<void()>&);
public:
 BellatrixDurableReactor(ChainStore&,Int,Bytes,BellatrixForkBlockRules,BellatrixStoreSnapshotLimits,
  BellatrixForkStore trusted_initial,const std::function<void()>& checkpoint={});
 BellatrixDurableReactor(ChainStore&,Int,Bytes,BellatrixForkBlockRules,BellatrixStoreSnapshotLimits,
  const ForkCheckpointToken& expected,const std::function<void()>& checkpoint={});
 BellatrixDurableReactor(const BellatrixDurableReactor&)=delete;
 BellatrixDurableReactor& operator=(const BellatrixDurableReactor&)=delete;
 BellatrixPublishedStore current() const;
 // Current-slot local duty; no caller-supplied vote fields or fork domain.
 // local_clock must come from the validator service's independent local clock,
 // never a peer. Holds Store/journal gates through the bounded signing callback.
 // Samples must not regress within an attempt; time and cancellation are
 // checked again after journal verification, before returning a signature.
 // Callback must not re-enter either coordinator. No autonomous duty or outbox.
 std::optional<Phase0SignedAttestation> sign_attestation(uint64_t validator_index,
  uint64_t expected_slot,const std::function<uint64_t()>& local_clock,const ForkCheckpointToken& expected_store,
  BeaconSigningJournal&,const BeaconSigningJournal::Signer&,
  const std::function<void()>& checkpoint={});
 // Construct against the current fork-choice head and sign only after full
 // body validation. Candidate body includes an already signed RANDAO reveal.
 // No automatic publication: returned wire block must use normal admission.
 std::optional<Bytes> sign_proposal(uint64_t validator_index,uint64_t expected_slot,
  std::span<const uint8_t> body,const std::function<uint64_t()>& local_clock,
  const ForkCheckpointToken& expected_store,BeaconSigningJournal&,
  const BeaconSigningJournal::Signer&,const std::function<void()>& checkpoint={});
 // Epoch RANDAO reveal for the selected current-slot proposer only. Domain
 // and key derived locally. Uses the journal gate/retirement check; RANDAO is
 // not a slashable proposal/vote and does not advance those reservations.
 std::optional<Bytes> sign_randao(uint64_t validator_index,uint64_t expected_slot,
  const std::function<uint64_t()>& local_clock,const ForkCheckpointToken& expected_store,
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
