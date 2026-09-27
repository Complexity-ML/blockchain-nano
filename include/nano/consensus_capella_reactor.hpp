#pragma once
#include "nano/consensus_capella_store_snapshot.hpp"
#include "nano/storage.hpp"
#include "nano/consensus_beacon_signer.hpp"
#include "nano/consensus_capella_bls_change.hpp"
namespace nano {
class Blockchain;
struct BeaconReadResponse;
class StaleCapellaDuty : public std::invalid_argument {public: using std::invalid_argument::invalid_argument;};
class ExpiredCapellaDuty : public std::runtime_error {public: using std::runtime_error::runtime_error;};
using CapellaSignedVotes=std::map<Bytes,Bytes>;
struct CapellaPendingBlsChange {ConsensusRoot admission_root;Bytes wire;};
using CapellaPendingBlsChanges=std::map<uint64_t,CapellaPendingBlsChange>;
struct CapellaPublishedStore {std::shared_ptr<const CapellaForkStore> state;ForkCheckpointToken token;};
// Immutable root-only inventory; does not pin EVM state or payload archives.
class CapellaInventorySnapshot {
 friend class CapellaDurableReactor;
 ForkCheckpointToken token_;std::string kind_;std::vector<ConsensusRoot> roots_;
public:
 size_t root_bytes()const{return roots_.capacity()*sizeof(ConsensusRoot);}
 Json page(const std::optional<ConsensusRoot>& after,size_t maximum_entries,size_t maximum_json_bytes,const std::function<void()>& checkpoint={})const;
};
// Serialized local event coordinator. Initial anchor or restart token must be
// independently trusted. Optional execution-cache binding; no scheduler, P2P or fork activation.
// Own ChainStore for this object's lifetime; callbacks must not re-enter it.
class CapellaDurableReactor {
 ChainStore& disk_;Blockchain* execution_=nullptr;Int chain_;Bytes genesis_;CapellaForkBlockRules rules_;CapellaStoreSnapshotLimits limits_;
 std::mutex& mutex_;std::shared_ptr<const CapellaForkStore> state_;std::optional<ForkCheckpointToken> token_;
 CapellaSignedVotes votes_,slashings_;CapellaPendingBlsChanges bls_changes_;CapellaStoreSnapshotCache cache_;bool unavailable_=false;
 std::optional<Bytes> signed_block_locked(const ConsensusRoot&,const std::function<void()>&)const;
 Json network_identity_locked()const;
 void require_available()const;
 ChainStore* witness_=nullptr;std::optional<ForkCheckpointToken> witness_token_;
 size_t witness_limit()const;
 void witness_prepare(std::string_view,const std::vector<CheckpointArchiveEntry>&);
 void validate_execution_binding()const;
 void fail_closed();
 std::optional<Bytes> prepare_local_body_locked(uint64_t,uint64_t,const Bytes&,const std::string&,const ForkCheckpointToken&,std::span<const SignedTransaction>,size_t,size_t,const std::function<void()>&,std::span<const SignedCapellaBlsChange> = {},size_t maximum_change_trials = 0);
 std::vector<ConsensusRoot> bls_change_roots_locked(const std::function<void()>&)const;
 std::vector<SignedCapellaBlsChange> pending_bls_changes_locked(size_t,const std::function<void()>&)const;
 void tick_locked(uint64_t,const std::function<void()>&);
 std::optional<Phase0SignedAttestation> sign_attestation_locked(uint64_t,uint64_t,const std::function<uint64_t()>&,const ForkCheckpointToken&,BeaconSigningJournal&,const BeaconSigningJournal::Signer&,const std::function<void()>&);
 std::optional<Bytes> sign_randao_locked(uint64_t,uint64_t,const std::function<uint64_t()>&,const ForkCheckpointToken&,BeaconSigningJournal&,const BeaconSigningJournal::Signer&,const std::function<void()>&);
 std::optional<Bytes> sign_proposal_locked(uint64_t,uint64_t,std::span<const uint8_t>,const std::function<uint64_t()>&,const ForkCheckpointToken&,BeaconSigningJournal&,const BeaconSigningJournal::Signer&,const std::function<void()>&);
 std::optional<Bytes> prepare_local_body_from_pool_locked(uint64_t,uint64_t,const Bytes&,const std::string&,const ForkCheckpointToken&,size_t,size_t,const std::function<void()>&);
 void publish(CapellaForkStore,const std::function<void()>&,const std::vector<CheckpointArchiveEntry>& archive={},std::optional<CapellaSignedVotes> votes=std::nullopt,std::optional<CapellaSignedVotes> slashings=std::nullopt,std::optional<CapellaPendingBlsChanges> changes=std::nullopt);
public:
 CapellaDurableReactor(ChainStore&,Int,Bytes,CapellaForkBlockRules,CapellaStoreSnapshotLimits,CapellaForkStore,const std::function<void()>& checkpoint={},Blockchain* owner=nullptr,ChainStore* witness=nullptr);
 CapellaDurableReactor(ChainStore&,Int,Bytes,CapellaForkBlockRules,CapellaStoreSnapshotLimits,const ForkCheckpointToken&,const std::function<void()>& checkpoint={},Blockchain* owner=nullptr);
 CapellaDurableReactor(Blockchain&,CapellaForkBlockRules,CapellaStoreSnapshotLimits,CapellaForkStore,const std::function<void()>& checkpoint={},ChainStore* witness=nullptr);
 CapellaDurableReactor(Blockchain&,CapellaForkBlockRules,CapellaStoreSnapshotLimits,const ForkCheckpointToken&,const std::function<void()>& checkpoint={});
 CapellaDurableReactor(const CapellaDurableReactor&)=delete;
 CapellaDurableReactor& operator=(const CapellaDurableReactor&)=delete;
 // The separate witness is an explicitly trusted rollback boundary.
 CapellaDurableReactor(Blockchain&,CapellaForkBlockRules,CapellaStoreSnapshotLimits,ChainStore& witness,const std::function<void()>& checkpoint={});
 ~CapellaDurableReactor();
 void invalidate();
 std::optional<Bytes> prepare_local_body_from_pool(uint64_t validator_index,uint64_t slot,
  const Bytes& randao,const std::string& fee_recipient,const ForkCheckpointToken& expected,
  size_t maximum_transaction_trials,size_t maximum_vote_trials,const std::function<void()>& checkpoint={});
 bool execution_bound()const{return execution_!=nullptr;}
 std::pair<uint64_t,uint64_t> local_duty_timing()const{return {rules_.slot_duration_ms,rules_.transition.consensus.epoch.ffg.slots_per_epoch};}
 bool tick_if_needed(uint64_t,const std::function<void()>& checkpoint={});
 std::optional<uint64_t> current_proposer(uint64_t,const std::function<void()>& checkpoint={});
 std::set<uint64_t> current_attesters(uint64_t,const std::function<void()>& checkpoint={});
 std::optional<Phase0SignedAttestation> attest_current(uint64_t,uint64_t,const std::function<uint64_t()>&,BeaconSigningJournal&,const BeaconSigningJournal::Signer&,const std::function<void()>& checkpoint={});
 std::optional<Bytes> propose_current(uint64_t,uint64_t,const std::string&,size_t maximum_transaction_trials,size_t maximum_vote_trials,const std::function<uint64_t()>&,BeaconSigningJournal&,const BeaconSigningJournal::Signer&,const std::function<void()>& checkpoint={});
 CapellaPublishedStore current()const;
 Json network_identity()const;
 // Atomic archive/checkpoint projection. maximum_bytes bounds raw SSZ; caller
 // must separately budget JSON/hex framing. An anchor has no fabricated signature.
 // Root-ordered candidates; export_message determines signed availability.
 std::shared_ptr<const CapellaInventorySnapshot> capture_inventory(std::string_view kind,size_t maximum_root_bytes,const std::function<void()>& checkpoint={})const;
 Json inventory(std::string_view kind,const std::optional<ConsensusRoot>& after,size_t maximum_entries,size_t maximum_json_bytes,const std::optional<ForkCheckpointToken>& expected={},const std::function<void()>& checkpoint={})const;
 Json export_message(std::string_view kind,const ConsensusRoot&,size_t maximum_bytes,const std::optional<ForkCheckpointToken>& expected={},const std::function<void()>& checkpoint={})const;
 bool bls_change_ssz(std::span<const uint8_t>,const std::function<void()>& checkpoint={});
 // Candidates applicable to the selected head; archive entries survive until finality.
 std::vector<SignedCapellaBlsChange> pending_bls_changes()const;
 bool vote_ssz(std::span<const uint8_t>,const std::function<void()>& checkpoint={});
 bool slashing_ssz(std::span<const uint8_t>,const std::function<void()>& checkpoint={});
 BeaconReadResponse beacon_read(std::string_view,const std::function<void()>& checkpoint={})const;
 // Caller supplies identity and an independent clock, never unsigned vote data.
 // Signature reservations live in a separate durable journal and survive expiry.
 std::optional<Phase0SignedAttestation> sign_attestation(uint64_t validator_index,
  uint64_t slot,const std::function<uint64_t()>& local_clock,const ForkCheckpointToken& expected,
  BeaconSigningJournal&,const BeaconSigningJournal::Signer&,const std::function<void()>& checkpoint={});
 std::optional<Bytes> sign_randao(uint64_t validator_index,uint64_t slot,
  const std::function<uint64_t()>& local_clock,const ForkCheckpointToken& expected,
  BeaconSigningJournal&,const BeaconSigningJournal::Signer&,const std::function<void()>& checkpoint={});
 // Body is fully executed/validated on the current parent before reservation.
 std::optional<Bytes> sign_proposal(uint64_t validator_index,uint64_t slot,
  std::span<const uint8_t> body,const std::function<uint64_t()>& local_clock,const ForkCheckpointToken& expected,
  BeaconSigningJournal&,const BeaconSigningJournal::Signer&,const std::function<void()>& checkpoint={});
 // Bounded transaction candidates; sender identities are recovered internally.
 std::optional<Bytes> prepare_local_body(uint64_t validator_index,uint64_t slot,
  const Bytes& randao,const std::string& fee_recipient,const ForkCheckpointToken& expected,
  std::span<const SignedTransaction> pending,size_t maximum_transaction_trials,size_t maximum_vote_trials,
  const std::function<void()>& checkpoint={});
 // Candidate operations remain untrusted: each accepted addition is checked by
 // a complete block transition. This API does not persist a pending queue.
 std::optional<Bytes> prepare_local_body_with_bls_changes(uint64_t validator_index,uint64_t slot,
  const Bytes& randao,const std::string& fee_recipient,const ForkCheckpointToken& expected,
  std::span<const SignedTransaction> pending,size_t maximum_transaction_trials,size_t maximum_vote_trials,
  std::span<const SignedCapellaBlsChange> changes,size_t maximum_change_trials,const std::function<void()>& checkpoint={});
 // Authenticated recent gossip votes, including deferred current-slot votes.
 // Retained for current/previous epoch; copy is bounded by snapshot limits.
 CapellaSignedVotes signed_votes()const;
 // Canonical attester-slashing proofs; retention bounded by equivocations.
 CapellaSignedVotes signed_slashings()const;
 // Exact admitted wire bytes, authenticated against the current Store. Anchors
 // and legacy checkpoints may lack archives; unknown/pruned roots return empty.
 std::optional<Bytes> signed_block(const ConsensusRoot&,const std::function<void()>& checkpoint={})const;
 void tick(uint64_t time_ms,const std::function<void()>& checkpoint={});
 bool block(std::span<const uint8_t>,const std::function<void()>& checkpoint={});
 // Current-slot votes are durable before acknowledgment, effective next slot.
 bool vote(const Phase0SignedAttestation&,const std::function<void()>& checkpoint={});
 bool slashing(const IndexedConsensusAttestation&,const IndexedConsensusAttestation&,const std::function<void()>& checkpoint={});
};
}
