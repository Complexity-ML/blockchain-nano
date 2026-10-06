#pragma once
#include "nano/consensus_bellatrix_store_snapshot.hpp"
#include "nano/storage.hpp"
#include "nano/beacon_events.hpp"
#include <memory>
#include <exception>
#include "nano/consensus_beacon_signer.hpp"
namespace nano {
class Blockchain;
// Only this explicitly classified race is retryable by the local scheduler.
class StaleBellatrixDuty : public std::invalid_argument {
public: using std::invalid_argument::invalid_argument;
};
// A monotonic clock advanced beyond the duty slot; signing reservations remain.
class ExpiredBellatrixDuty : public std::runtime_error {
public: using std::runtime_error::runtime_error;
};
using BellatrixSignedBlockArchive=std::map<ConsensusRoot,std::shared_ptr<const Bytes>>;
using BellatrixSignedVoteArchive=std::map<ConsensusRoot,std::shared_ptr<const Bytes>>;
struct BellatrixExitIdentity {
 uint64_t index;std::array<uint8_t,48> public_key;
 auto operator<=>(const BellatrixExitIdentity&)const=default;
};
struct BellatrixSignedExitRecord {
 ConsensusRoot admission_root;Bytes ssz;
 // Derived from the BLS-verified admission context; reconstructed on restore,
 // never supplied by the peer or accepted as separate snapshot authority.
 std::array<uint8_t,48> public_key;
 BellatrixExitIdentity identity()const;
};
enum class BellatrixExitMembership {Unknown, Matching, DifferentKey, Scheduled};
BellatrixExitMembership bellatrix_exit_membership(const BellatrixSignedExitRecord&,const ConsensusSlashingState&);
using BellatrixSignedExitArchive=std::map<ConsensusRoot,std::shared_ptr<const BellatrixSignedExitRecord>>;
struct BellatrixPublishedStore {std::shared_ptr<const BellatrixForkStore> state;ForkCheckpointToken token;std::shared_ptr<const BellatrixSignedBlockArchive> signed_blocks;std::shared_ptr<const BellatrixSignedVoteArchive> signed_votes;std::shared_ptr<const BellatrixSignedVoteArchive> signed_slashings;std::shared_ptr<const BellatrixSignedExitArchive> signed_exits;};
// Serialized durable event coordinator, NOT a P2P service or EVM activation.
// The caller owns ChainStore for this object's lifetime and binds its chain.
// Initial Store / restart commitment must be independently authenticated.
// Cancellation callbacks must not re-enter this serialized coordinator.
// Optional execution owner must own the exact same store and outlive this
// reactor. Its canonical cache/pool/events publish in the same commit path.
// Full snapshots per event are a correctness baseline, not a throughput claim.
class BellatrixDurableReactor {
 ChainStore& disk_;
 Blockchain* execution_=nullptr;
 ChainStore* witness_=nullptr;std::optional<ForkCheckpointToken> witness_token_;
 size_t witness_limit()const;
 void witness_prepare(std::string_view);
 void validate_execution_binding() const;
 const Int chain_id_;const Bytes genesis_;
 const BellatrixForkBlockRules rules_;const BellatrixStoreSnapshotLimits limits_;
 std::mutex& mutex_;
 std::shared_ptr<const BellatrixForkStore> state_;
 std::optional<ForkCheckpointToken> token_;
 BellatrixStoreSnapshotCache snapshot_cache_;
 // Exact bytes paired with token_ only after a successful durable publication.
 // Empty after reopen: retain the existing digest comparison as a cold fallback.
 std::string published_snapshot_;
 std::shared_ptr<const BellatrixSignedBlockArchive> signed_blocks_=std::make_shared<const BellatrixSignedBlockArchive>();
 std::shared_ptr<const BellatrixSignedVoteArchive> signed_votes_=std::make_shared<const BellatrixSignedVoteArchive>();
 std::shared_ptr<const BellatrixSignedVoteArchive> signed_slashings_=std::make_shared<const BellatrixSignedVoteArchive>();
 std::shared_ptr<const BellatrixSignedExitArchive> signed_exits_=std::make_shared<const BellatrixSignedExitArchive>();
 std::shared_ptr<const BeaconEventJournal> events_=std::make_shared<const BeaconEventJournal>();
 bool unavailable_=false;
 std::exception_ptr failure_;
 void fail_closed();
 void require_available() const;
 void publish(BellatrixForkStore,std::shared_ptr<const BellatrixSignedBlockArchive>,std::shared_ptr<const BellatrixSignedVoteArchive>,const std::function<void()>&,std::shared_ptr<const BellatrixSignedVoteArchive> slashings={},std::shared_ptr<const BellatrixSignedExitArchive> exits={});
std::optional<Bytes> prepare_local_body_locked(uint64_t index,uint64_t slot,
 const Bytes& reveal,const std::string& recipient,const ForkCheckpointToken& expected,size_t maximum_vote_trials,const std::function<void()>& check);
std::optional<Bytes> sign_randao_locked(uint64_t index,uint64_t slot,
 const std::function<uint64_t()>& local_clock,const ForkCheckpointToken& expected,
 BeaconSigningJournal& journal,const BeaconSigningJournal::Signer& signer,const std::function<void()>& checkpoint);
std::optional<Bytes> sign_proposal_locked(uint64_t index,uint64_t slot,
 std::span<const uint8_t> body,const std::function<uint64_t()>& local_clock,
 const ForkCheckpointToken& expected,BeaconSigningJournal& journal,
 const BeaconSigningJournal::Signer& signer,const std::function<void()>& checkpoint);
std::optional<Phase0SignedAttestation> sign_attestation_locked(uint64_t index,
 uint64_t slot,const std::function<uint64_t()>& local_clock,const ForkCheckpointToken& expected,BeaconSigningJournal& journal,
 const BeaconSigningJournal::Signer& signer,const std::function<void()>& checkpoint);
std::optional<uint64_t> local_proposer_locked(uint64_t slot,const ForkCheckpointToken& expected,const std::function<void()>& checkpoint);
std::set<uint64_t> local_attesters_locked(uint64_t slot,const ForkCheckpointToken& expected,const std::function<void()>& checkpoint);
std::optional<Bytes> propose_local_locked(uint64_t index,uint64_t slot,const std::string& recipient,
 size_t maximum_vote_trials,const std::function<uint64_t()>& local_clock,const ForkCheckpointToken& expected,
 BeaconSigningJournal& journal,const BeaconSigningJournal::Signer& signer,const std::function<void()>& checkpoint);
 void tick_locked(uint64_t,const std::function<void()>&);
public:
 // Persist clock advancement only when slot or attestation eligibility changes.
 // Explicit tick() remains an unconditional durable checkpoint operation.
 bool tick_if_needed(uint64_t,const std::function<void()>& checkpoint={});
 Json beacon_events(std::optional<uint64_t> cursor)const;
 uint64_t beacon_head_slot(const std::function<void()>& checkpoint={}) const;
 BellatrixDurableReactor(Blockchain&,BellatrixForkBlockRules,BellatrixStoreSnapshotLimits,
  BellatrixForkStore trusted_initial,const std::function<void()>& checkpoint={},ChainStore* witness=nullptr);
 BellatrixDurableReactor(Blockchain&,BellatrixForkBlockRules,BellatrixStoreSnapshotLimits,
  const ForkCheckpointToken& expected,const std::function<void()>& checkpoint={});
 // Explicitly trusted separate witness; finish its exact pending publication.
 BellatrixDurableReactor(Blockchain&,BellatrixForkBlockRules,BellatrixStoreSnapshotLimits,
  ChainStore& witness,const std::function<void()>& checkpoint={});
 BellatrixDurableReactor(ChainStore&,Int,Bytes,BellatrixForkBlockRules,BellatrixStoreSnapshotLimits,
  BellatrixForkStore trusted_initial,const std::function<void()>& checkpoint={},Blockchain* execution=nullptr,ChainStore* witness=nullptr);
 BellatrixDurableReactor(ChainStore&,Int,Bytes,BellatrixForkBlockRules,BellatrixStoreSnapshotLimits,
  const ForkCheckpointToken& expected,const std::function<void()>& checkpoint={},Blockchain* execution=nullptr);
 ~BellatrixDurableReactor();
 // Fail closed on runtime/clock shutdown; requires construction and verification anew.
 void invalidate();
 BellatrixDurableReactor(const BellatrixDurableReactor&)=delete;
 BellatrixDurableReactor& operator=(const BellatrixDurableReactor&)=delete;
 BellatrixPublishedStore current() const;
 bool voluntary_exit_ssz(std::span<const uint8_t>,const std::function<void()>& checkpoint={});
 // Compatibility identity, not proof of peer honesty or finality.
 Json network_identity() const;
 bool execution_bound() const{return execution_!=nullptr;}
 // Build on the authenticated head using its state/history and a pool snapshot.
 // Candidate votes are individually inclusion-validated, bounded by local work
 // policy. No signature is issued here. The native deposit profile supplies
 // votes and mandatory proofs; other profiles fail if operations are missing.
 std::optional<Bytes> prepare_local_body(uint64_t validator_index,uint64_t slot,
  const Bytes& randao_reveal,const std::string& fee_recipient,
  const ForkCheckpointToken& expected,size_t maximum_vote_trials,
  const std::function<void()>& checkpoint={});
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
 // One coherent local proposal attempt; still uses normal admission afterwards.
 std::optional<Bytes> propose_local(uint64_t index,uint64_t slot,const std::string& recipient,
  size_t maximum_vote_trials,const std::function<uint64_t()>& local_clock,
  const ForkCheckpointToken&,BeaconSigningJournal&,const BeaconSigningJournal::Signer&,
  const std::function<void()>& checkpoint={});
 // Epoch RANDAO reveal for the selected current-slot proposer only. Domain
 // and key derived locally. Uses the journal gate/retirement check; RANDAO is
 // not a slashable proposal/vote and does not advance those reservations.
 std::optional<Bytes> sign_randao(uint64_t validator_index,uint64_t expected_slot,
  const std::function<uint64_t()>& local_clock,const ForkCheckpointToken& expected_store,
  BeaconSigningJournal&,const BeaconSigningJournal::Signer&,
  const std::function<void()>& checkpoint={});
 // Scheduling hints from the authenticated current state; signing revalidates them.
 std::optional<uint64_t> local_proposer(uint64_t slot,const ForkCheckpointToken&,const std::function<void()>& checkpoint={});
 std::set<uint64_t> local_attesters(uint64_t slot,const ForkCheckpointToken&,const std::function<void()>& checkpoint={});
 // Local intent-only operations: derive all signing data from the latest
 // authenticated state, keeping disk/journal/deadline checks before signatures.
 std::optional<uint64_t> current_proposer(uint64_t slot,const std::function<void()>& checkpoint={});
 std::set<uint64_t> current_attesters(uint64_t slot,const std::function<void()>& checkpoint={});
 std::optional<Phase0SignedAttestation> attest_current(uint64_t index,uint64_t slot,
  const std::function<uint64_t()>&,BeaconSigningJournal&,const BeaconSigningJournal::Signer&,const std::function<void()>& checkpoint={});
 std::optional<Bytes> propose_current(uint64_t index,uint64_t slot,const std::string& recipient,size_t trials,
  const std::function<uint64_t()>&,BeaconSigningJournal&,const BeaconSigningJournal::Signer&,const std::function<void()>& checkpoint={});
 void tick(uint64_t time_ms,const std::function<void()>& checkpoint={});
 bool block(std::span<const uint8_t>,const std::function<void()>& checkpoint={});
 // Wire admission uses the configured fork schema/committee limit. There is
 // no sender-controlled "already included in a block" or time-window bypass.
 bool vote_ssz(std::span<const uint8_t>,const std::function<void()>& checkpoint={});
 bool slashing_ssz(std::span<const uint8_t>,const std::function<void()>& checkpoint={});
 // Current-slot votes are durably archived and applied by the next slot tick.
 bool vote(const Phase0SignedAttestation&,const std::function<void()>& checkpoint={});
 bool slashing(const IndexedConsensusAttestation&,const IndexedConsensusAttestation&,
  const std::function<void()>& checkpoint={});
};
}

namespace nano {
// Revalidate a retained signed block against its authenticated store record,
// including SSZ body commitment and BLS proposer signature.
void validate_bellatrix_archived_block(const ConsensusRoot&,const Bytes&,
 const BellatrixForkStore&,const BellatrixForkBlockRules&,const std::function<void()>& checkpoint={});
}
