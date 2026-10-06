#pragma once
#include <exception>
#include <atomic>
#include "nano/consensus_capella_store_snapshot.hpp"
#include "nano/storage.hpp"
#include "nano/consensus_capella_duty.hpp"
#include "nano/consensus_beacon_signer.hpp"
#include "nano/consensus_capella_bls_change.hpp"
#include "nano/consensus_sync_message.hpp"
namespace nano {
class Blockchain;
struct BeaconReadResponse;
class StaleCapellaDuty : public std::invalid_argument {public: using std::invalid_argument::invalid_argument;};
class ExpiredCapellaDuty : public std::runtime_error {public: using std::runtime_error::runtime_error;};
using CapellaSignedVotes=std::map<Bytes,Bytes>;
struct CapellaPendingBlsChange {ConsensusRoot admission_root;Bytes wire;};
using CapellaPendingBlsChanges=std::map<uint64_t,CapellaPendingBlsChange>;
struct CapellaPendingExit {ConsensusRoot admission_root;std::array<uint8_t,48> public_key;Bytes wire;};
using CapellaPendingExits=std::map<Bytes,CapellaPendingExit>;
enum class CapellaBlockAdmission {accepted,rejected,deferred};
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
 // Coherent immutable state/token, installed only after durable publication.
 // Use shared_ptr atomic free functions for Apple libc++ compatibility.
 // Every access must remain atomic, including invalidation and restoration.
 std::shared_ptr<const CapellaPublishedStore> beacon_publication_;
 CapellaPublishedStore beacon_snapshot()const;
 std::optional<Bytes> beacon_signed_block(const CapellaPublishedStore&,const ConsensusRoot&,const std::function<void()>&)const;
 CapellaSignedVotes votes_,slashings_;CapellaPendingBlsChanges bls_changes_;CapellaPendingExits exits_;CapellaStoreSnapshotCache cache_;bool unavailable_=false;bool automatic_pruning_=false;bool historical_state_pruning_=false;
 std::exception_ptr failure_;
 // Bounded ephemeral continuations keyed by admitted parent and block slot.
 // Alternating parents or target slots never reset another retained prefix.
 // No peer state, signing or durable writes while incomplete; eviction only
 // discards recomputable work. All entries remain owned under mutex_.
 std::vector<std::pair<uint64_t,CapellaBlockPreparation>> incoming_preparations_;
 // Bounded shared target continuations, bound to admitted root/epoch/profile.
 // Required contexts come from checked block processing, never peer state.
 // Blocks and votes share this cache; request identity never discards prefixes.
 ConsensusRoot incoming_target_block_{};
 std::vector<CapellaTargetPreparation> incoming_targets_;
 std::optional<CapellaDutyPreparation> local_preparation_;
 std::array<std::optional<CapellaTargetPreparation>,2> vote_preparations_;
 CapellaTargetPreparation* prepare_vote_target_locked(const CapellaForkStore&,const Phase0SignedAttestation&,
  bool complete,const std::function<void()>&);
 bool prepare_local_duties_locked(uint64_t,const std::function<void()>&);
 bool block_locked(std::span<const uint8_t>,const std::function<void()>&,bool* deferred=nullptr);
 std::optional<Bytes> propose_current_locked(uint64_t,uint64_t,const std::string&,size_t,size_t,const std::function<uint64_t()>&,BeaconSigningJournal&,const BeaconSigningJournal::Signer&,const std::function<void()>&,const Bytes&);
 // Only populated from this reactor's immutable, admitted head under mutex_.
 // Single-entry cache: no retained state snapshots or old-branch registry maps.
 struct ValidatorIndexCache {ConsensusRoot root;std::set<Bytes> keys;std::map<Bytes,uint64_t> indices;};
 std::optional<ValidatorIndexCache> validator_index_cache_;
 void check_registered_index_locked(uint64_t,const Bytes&,const BeaconSigningJournal&,const std::function<void()>&);
 // Bounded header-only proof cache, derived from finalized hash-linked
 // ancestry under mutex_. No states, peer claims or unverified ACKs retained.
 mutable std::map<ConsensusRoot,ConsensusBeaconHeader> historical_headers_;
 std::optional<Bytes> signed_block_locked(const ConsensusRoot&,const std::function<void()>&)const;
 Json network_identity_locked()const;
 Json network_identity_for_store(const CapellaForkStore&)const;
 struct PendingSyncAggregate {uint64_t slot;ConsensusRoot parent;size_t participants;Bytes ssz;};
 std::map<uint64_t,PendingSyncAggregate> pending_sync_;
 struct PendingSyncMessages { uint64_t slot; ConsensusRoot parent; std::map<uint64_t,Bytes> wires; };
 std::map<uint64_t,PendingSyncMessages> pending_sync_messages_;
 enum class SyncOutcome {accepted,invalid_width,slot_overflow,expired_slot,future_slot,head_at_or_after_inclusion,unknown_validator,root_mismatch,not_in_committee,invalid_signature,aggregation_failed,count};
 std::array<uint64_t,static_cast<size_t>(SyncOutcome::count)> sync_outcomes_{};
 bool record_sync_outcome(SyncOutcome)noexcept;
 // Derived only from an admitted immutable parent under mutex_. One entry,
 // keyed by parent root and inclusion slot; never persisted or peer supplied.
 struct SyncAdmissionContext {uint64_t slot;ConsensusRoot parent;Bytes ssz,common;};
 std::optional<SyncAdmissionContext> sync_admission_context_;
 uint64_t sync_context_builds_=0,sync_context_hits_=0;
 const SyncAdmissionContext& sync_admission_context_locked(const ConsensusRoot&,uint64_t,const std::function<void()>&);
 struct SyncSigningContext {uint64_t slot;ConsensusRoot parent,block_root,genesis_root;ConsensusBytes4 version;std::map<uint64_t,Bytes> members;};
 std::optional<SyncSigningContext> sync_signing_context_;
 const SyncSigningContext& sync_signing_context_locked(uint64_t,const std::function<void()>&);
 bool sync_messages_current_locked(const PendingSyncMessages&,const std::function<void()>&)const;
 bool submit_sync_message_locked(const CapellaSyncMessage&,const ForkCheckpointToken&,const std::function<void()>&);
 bool submit_sync_aggregate_locked(uint64_t,std::span<const uint8_t>,const ForkCheckpointToken&,const std::function<void()>&);
 void require_available()const;
 ChainStore* witness_=nullptr;std::optional<ForkCheckpointToken> witness_token_;
 size_t witness_limit()const;
 void witness_prepare(std::string_view,const std::vector<CheckpointArchiveEntry>&);
 void validate_execution_binding()const;
 void fail_closed();
 std::optional<Bytes> prepare_local_body_locked(uint64_t,uint64_t,const Bytes&,const std::string&,const ForkCheckpointToken&,std::span<const SignedTransaction>,size_t,size_t,const std::function<void()>&,std::span<const SignedCapellaBlsChange> = {},size_t maximum_change_trials = 0,std::span<const SignedConsensusVoluntaryExit> = {},size_t maximum_exit_trials = 0);
 std::vector<ConsensusRoot> exit_roots_locked(const std::function<void()>&)const;
 std::vector<ConsensusRoot> bls_change_roots_locked(const std::function<void()>&)const;
 std::vector<SignedCapellaBlsChange> pending_bls_changes_locked(size_t,const std::function<void()>&)const;
 std::vector<SignedConsensusVoluntaryExit> pending_exits_locked(size_t,const std::function<void()>&)const;
 void prune_if_ready_locked(const std::function<void()>&);
 void tick_locked(uint64_t,const std::function<void()>&);
 std::optional<Phase0SignedAttestation> sign_attestation_locked(uint64_t,uint64_t,const std::function<uint64_t()>&,const ForkCheckpointToken&,BeaconSigningJournal&,const BeaconSigningJournal::Signer&,const std::function<void()>&);
 std::optional<Bytes> sign_randao_locked(uint64_t,uint64_t,const std::function<uint64_t()>&,const ForkCheckpointToken&,BeaconSigningJournal&,const BeaconSigningJournal::Signer&,const std::function<void()>&);
 std::optional<Bytes> sign_proposal_locked(uint64_t,uint64_t,std::span<const uint8_t>,const std::function<uint64_t()>&,const ForkCheckpointToken&,BeaconSigningJournal&,const BeaconSigningJournal::Signer&,const std::function<void()>&);
 std::optional<Bytes> prepare_local_body_from_pool_locked(uint64_t,uint64_t,const Bytes&,const std::string&,const ForkCheckpointToken&,size_t,size_t,const std::function<void()>&);
 void publish(CapellaForkStore,const std::function<void()>&,const std::vector<CheckpointArchiveEntry>& archive={},std::optional<CapellaSignedVotes> votes=std::nullopt,std::optional<CapellaSignedVotes> slashings=std::nullopt,std::optional<CapellaPendingBlsChanges> changes=std::nullopt,std::optional<CapellaPendingExits> exits=std::nullopt);
public:
 bool prepare_current_duties(uint64_t,const std::function<void()>& checkpoint={});
 bool voluntary_exit_ssz(std::span<const uint8_t>,const std::function<void()>& checkpoint={});
 std::vector<SignedConsensusVoluntaryExit> pending_exits()const;
 // Ephemeral bounded diagnostic counters; never part of consensus state.
 Json sync_admission_stats()const;
 // Local duties sign during the signed slot and enter normal admission.
 // Returns only committee members whose current message is not already held.
 std::set<uint64_t> current_sync_members(uint64_t signed_slot,const std::function<void()>& checkpoint={});
 std::optional<CapellaSyncMessage> sync_current(uint64_t validator_index,uint64_t signed_slot,
  const std::function<uint64_t()>& local_clock,BeaconSigningJournal&,const BeaconSigningJournal::Signer&,
  const std::function<void()>& checkpoint={},const Bytes& registration_public_key={});
 // Ephemeral current/next proposal contexts, each bounded to one committee.
 // Incoming messages verify membership/domain/signature without local signing.
 // Disjoint aggregate positions merge; overlap is never double-counted.
 bool sync_message_ssz(std::span<const uint8_t>,const std::function<void()>& checkpoint={});
 bool submit_sync_message(const CapellaSyncMessage&,const ForkCheckpointToken&,
  const std::function<void()>& checkpoint={});
 bool submit_sync_aggregate(uint64_t proposal_slot,std::span<const uint8_t>,
  const ForkCheckpointToken&,const std::function<void()>& checkpoint={});


 CapellaDurableReactor(ChainStore&,Int,Bytes,CapellaForkBlockRules,CapellaStoreSnapshotLimits,CapellaForkStore,const std::function<void()>& checkpoint={},Blockchain* owner=nullptr,ChainStore* witness=nullptr);
 CapellaDurableReactor(ChainStore&,Int,Bytes,CapellaForkBlockRules,CapellaStoreSnapshotLimits,const ForkCheckpointToken&,const std::function<void()>& checkpoint={},Blockchain* owner=nullptr);
 CapellaDurableReactor(Blockchain&,CapellaForkBlockRules,CapellaStoreSnapshotLimits,CapellaForkStore,const std::function<void()>& checkpoint={},ChainStore* witness=nullptr);
 CapellaDurableReactor(Blockchain&,CapellaForkBlockRules,CapellaStoreSnapshotLimits,const ForkCheckpointToken&,const std::function<void()>& checkpoint={});
 CapellaDurableReactor(const CapellaDurableReactor&)=delete;
 CapellaDurableReactor& operator=(const CapellaDurableReactor&)=delete;
 // The separate witness is an explicitly trusted rollback boundary.
 CapellaDurableReactor(Blockchain&,CapellaForkBlockRules,CapellaStoreSnapshotLimits,ChainStore& witness,const std::function<void()>& checkpoint={},std::optional<ForkCheckpointToken> recovery_commitment=std::nullopt);
 ~CapellaDurableReactor();
 void invalidate();
 std::optional<Bytes> prepare_local_body_from_pool(uint64_t validator_index,uint64_t slot,
  const Bytes& randao,const std::string& fee_recipient,const ForkCheckpointToken& expected,
  size_t maximum_transaction_trials,size_t maximum_vote_trials,const std::function<void()>& checkpoint={});
 // Head hint from one authenticated immutable publication; never remote input.
 Json relay_head(const std::function<void()>& checkpoint={})const;
 bool execution_bound()const{return execution_!=nullptr;}
 std::pair<uint64_t,uint64_t> local_duty_timing()const{return {rules_.slot_duration_ms,rules_.transition.consensus.epoch.ffg.slots_per_epoch};}
 bool tick_if_needed(uint64_t,const std::function<void()>& checkpoint={});
 std::optional<uint64_t> current_proposer(uint64_t,const std::function<void()>& checkpoint={});
 std::map<Bytes,uint64_t> current_validator_indices(const std::set<Bytes>&,const std::function<void()>& checkpoint={});
 // Scheduler eligibility: admitted slot block or authenticated profile deadline.
 uint64_t local_duty_due_ms()const{return rules_.attestation_due_ms;}
 bool current_duties_ready(uint64_t,const std::function<void()>& checkpoint={});
 std::set<uint64_t> current_attesters(uint64_t,const std::function<void()>& checkpoint={});
 std::optional<Phase0SignedAttestation> attest_current(uint64_t,uint64_t,const std::function<uint64_t()>&,BeaconSigningJournal&,const BeaconSigningJournal::Signer&,const std::function<void()>& checkpoint={},const Bytes& registration_public_key={});
 std::optional<Bytes> propose_current(uint64_t,uint64_t,const std::string&,size_t maximum_transaction_trials,size_t maximum_vote_trials,const std::function<uint64_t()>&,BeaconSigningJournal&,const BeaconSigningJournal::Signer&,const std::function<void()>& checkpoint={},const Bytes& registration_public_key={});
 // Scheduler path: hold the owner lock through signing and durable admission.
 // Return a wire only after ordinary local admission has committed it.
 std::optional<Bytes> propose_and_admit_current(uint64_t,uint64_t,const std::string&,size_t maximum_transaction_trials,size_t maximum_vote_trials,const std::function<uint64_t()>&,BeaconSigningJournal&,const BeaconSigningJournal::Signer&,const std::function<void()>& checkpoint={},const Bytes& registration_public_key={});
 // Local durable maintenance; preserves the selected head and authenticated archives.
 // Scheduling and native deposit-history pinning remain separate responsibilities.
 bool prune_finalized(const std::function<void()>& checkpoint={});
 // Local maintenance policy, not a consensus/fork parameter. Runtime enables it.
 void set_automatic_pruning(bool enabled);
 void set_historical_state_pruning(bool enabled);
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
 // Untrusted voluntary exits are selected by full block transition, in order.
 // Trial and body limits apply independently; no queue is persisted by this API.
 std::optional<Bytes> prepare_local_body_with_exits(uint64_t validator_index,uint64_t slot,
  const Bytes& randao,const std::string& fee_recipient,const ForkCheckpointToken& expected,
  std::span<const SignedTransaction> pending,size_t maximum_transaction_trials,size_t maximum_vote_trials,
  std::span<const SignedConsensusVoluntaryExit> exits,size_t maximum_exit_trials,const std::function<void()>& checkpoint={});
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
 CapellaBlockAdmission block_admission(std::span<const uint8_t>,const std::function<void()>& checkpoint={});
 // Current-slot votes are durable before acknowledgment, effective next slot.
 bool vote(const Phase0SignedAttestation&,const std::function<void()>& checkpoint={});
 bool slashing(const IndexedConsensusAttestation&,const IndexedConsensusAttestation&,const std::function<void()>& checkpoint={});
};
}
