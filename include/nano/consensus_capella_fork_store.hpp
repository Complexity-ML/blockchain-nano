#pragma once
#include "nano/consensus_fork_clock.hpp"
#include "nano/consensus_deposit_history.hpp"
#include "nano/consensus_capella_fork_state.hpp"
#include "nano/consensus_capella_transition.hpp"
namespace nano {
struct CapellaForkBlock {ConsensusBeaconHeader header;CapellaSlotState state;Block execution_block;State execution_state;std::vector<Receipt> receipts;std::shared_ptr<const ExecutionDepositHistory> deposit_history;};
// Stable immutable lease used by transitions; a cold cache may evict only after
// all leases are released. Metadata remains available without reading full state.
struct CapellaColdReference {
 ConsensusRoot root;Bytes digest;size_t bytes;bool anchor;
 bool operator==(const CapellaColdReference&)const=default;
};
class CapellaColdRecordCache;
class CapellaForkRecord {
 std::shared_ptr<CapellaForkBlock> hot_;
 std::optional<CapellaColdReference> cold_;
 ConsensusBeaconHeader header_{};Header execution_header_;
 std::shared_ptr<CapellaColdRecordCache> cache_;
 std::function<std::optional<std::string>(const Bytes&,size_t)> fetch_;
public:
 CapellaForkRecord()=default;
 CapellaForkRecord(CapellaForkBlock);
 CapellaForkRecord(CapellaColdReference,ConsensusBeaconHeader,Header,
  std::shared_ptr<CapellaColdRecordCache>,std::function<std::optional<std::string>(const Bytes&,size_t)>);
 std::shared_ptr<const CapellaForkBlock> lease(const std::function<void()>& checkpoint={})const;
 // Local candidate mutation only; detach first so snapshots/leases stay intact.
 CapellaForkBlock& writable(const std::function<void()>& checkpoint={});
 const ConsensusBeaconHeader& header()const;
 const Header& execution_header()const;
 const std::optional<CapellaColdReference>& cold_reference()const{return cold_;}
};
struct CapellaDepositAncestor {ConsensusBeaconHeader beacon;Header execution;uint64_t count;ConsensusRoot root;};
struct CapellaForkStore {
 Phase0ForkTimedView timed;
 // Original trusted anchor; storage pruning must not change network identity.
 std::optional<ConsensusRoot> network_anchor;
 // Authenticated newest-first execution ancestors of the anchor (up to 256).
 std::vector<Header> anchor_execution_ancestors;
 // Canonical ancestors before the storage anchor, newest first; no EVM states.
 std::vector<CapellaDepositAncestor> deposit_ancestors;
 std::map<ConsensusRoot,CapellaForkRecord> blocks;
 std::map<ConsensusRoot,bool> timely;
 std::map<ConsensusRoot,ConsensusCheckpoint> unrealized_justifications;
};
enum class CapellaProposerBoostProfile { Release160Beta0, ShufflingDependent };
struct CapellaForkBlockRules {
 CapellaTransitionRules transition;
 uint64_t slot_duration_ms,attestation_due_ms,proposer_score_boost;
 // Explicit spec revision: the 2025 fixtures predate the dependent-root gate.
 CapellaProposerBoostProfile boost_profile=CapellaProposerBoostProfile::ShufflingDependent;
 // Explicit network profile extension; empty retains the legacy profile bytes.
 // Pins the immutable Ethereum-format deposit contract on Nano execution.
 Bytes deposit_contract_code_hash;
 bool native_deposit_votes=false;
};
// Ephemeral target-epoch continuation bound to a real admitted block and the
// complete local profile. Contexts never come from gossip or serialized weights.
class CapellaTargetPreparation {
 CapellaForkBlockRules rules_;
 ConsensusCheckpoint target_;
 CapellaSlotPreparation slots_;
 // Local SHA256 certificate of the fully authenticated, pre-advance source.
 // Never imported, persisted, or inferred from a HOT lease pointer identity.
 ConsensusRoot source_certificate_{};
public:
 CapellaTargetPreparation(const CapellaForkStore&,const CapellaForkBlockRules&,
  const ConsensusCheckpoint&,const std::function<void()>& checkpoint={});
 const ConsensusCheckpoint& target()const{return target_;}
 bool matches(const CapellaForkStore&,const CapellaForkBlockRules&,
  const ConsensusCheckpoint&,const std::function<void()>& checkpoint={})const;
 bool retarget(const CapellaForkStore&,const CapellaForkBlockRules&,const ConsensusCheckpoint&,const std::function<void()>& checkpoint={});
 bool advance(uint64_t maximum_slots,uint64_t trials,const std::function<void()>& checkpoint={});
 bool ready()const;
 Phase0ForkVoteContext context(const CapellaForkStore&,const CapellaForkBlockRules&,
  const std::function<void()>& checkpoint={})const;
};
// Explicit local restoration/validation path. Each completed chunk remains
// bounded; caller's cancellation is checked between chunks. No signatures/I/O.
Phase0ForkVoteContext prepare_capella_resumed_target_context(const CapellaForkStore&,
 const CapellaForkBlockRules&,const ConsensusCheckpoint&,uint64_t maximum_slots,
 uint64_t trials,const std::function<void()>& checkpoint={});
std::optional<CapellaForkStore> prepare_capella_prepared_store_vote(const CapellaForkStore&,
 const Phase0SignedAttestation&,const CapellaForkBlockRules&,const CapellaTargetPreparation&,
 const std::function<void()>& checkpoint={});
// Capella on_block on a trusted, internally consistent in-memory Store. Full
// signature/operation/state-root validation precedes candidate publication.
// Unknown parent/future/incompatible-finality/invalid blocks return nullopt;
// invalid trusted context, cancellation and resource errors may throw.
// Duplicate block roots return the unchanged Store (as in the reference).
// Caller initializes the authenticated anchor and publishes/persists all fields
// together. Both Beacon and EVM parents are selected from the same branch.
// Not a P2P adapter, durable Store or startup trust policy.
std::optional<CapellaForkStore> prepare_capella_fork_block(const CapellaForkStore&,
 std::span<const uint8_t> signed_block,const CapellaForkBlockRules&,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});
// Explicit local archive-validation path. Complete authenticated slot chunks
// with cancellation between chunks, then run full signed on_block processing.
// Does not replay standalone/included fork votes, persist, sign or admit gossip.
std::optional<CapellaForkStore> prepare_capella_resumed_fork_block(const CapellaForkStore&,
 std::span<const uint8_t>,const CapellaForkBlockRules&,uint64_t maximum_slots,
 uint64_t maximum_sync_trials,const std::function<void()>& checkpoint={});
CapellaForkStore prepare_capella_resumed_store_tick(const CapellaForkStore&,uint64_t time_ms,
 const CapellaForkBlockRules&,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});
// Receive a block as one private candidate: on_block first, then its embedded
// attestations (verified-block time window) and attester slashing evidence.
// The relaxed vote window is never exposed to gossip callers. Votes whose fork
// context/ancestry is unavailable or invalid do not invalidate the valid block;
// they are not queued here. Caller owns retry/retrieval policy and persistence.
std::optional<CapellaForkStore> prepare_capella_received_block(const CapellaForkStore&,
 std::span<const uint8_t> signed_block,const CapellaForkBlockRules&,uint64_t maximum_slots,
 uint64_t maximum_sync_trials,const std::function<void()>& checkpoint={});
// Same Store admission/publication candidate semantics using a prepared parent.
// Full profile and admitted parent must match; included-vote processing remains
// bounded by maximum_slots. No unverified state or peer checkpoint is installed.
std::optional<CapellaForkStore> prepare_capella_prepared_received_block(
 const CapellaForkStore&,std::span<const uint8_t>,const CapellaForkBlockRules&,
 const CapellaBlockPreparation&,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});
// Internal continuation signal: a fully validated signed block needs a
// retained checkpoint context beyond this call's slot budget. No candidate
// Store has been published. Ordinary admission APIs keep their old exception.
class CapellaTargetContextRequired : public std::length_error {
 ConsensusCheckpoint target_;
public:
 explicit CapellaTargetContextRequired(const ConsensusCheckpoint& target)
  :std::length_error("Capella target continuation required"),target_(target){}
 const ConsensusCheckpoint& target()const{return target_;}
};
// Completed local target frames may be shared with embedded votes and FFG
// updates. Every used frame rechecks the admitted source and complete profile.
// Missing targets retain the original bounded replay semantics.
std::optional<CapellaForkStore> prepare_capella_contextual_received_block(
 const CapellaForkStore&,std::span<const uint8_t>,const CapellaForkBlockRules&,
 const CapellaBlockPreparation&,std::span<const CapellaTargetPreparation* const>,
 uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});
CapellaForkStore prepare_capella_contextual_store_tick(const CapellaForkStore&,uint64_t time_ms,
 const CapellaForkBlockRules&,std::span<const CapellaTargetPreparation* const>,
 uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});
// Store-bound gossip admission; the sender cannot supply committee/context or
// bypass the gossip time window with an "already included" flag.
std::optional<CapellaForkStore> prepare_capella_store_vote(const CapellaForkStore&,
 const Phase0SignedAttestation&,const CapellaForkBlockRules&,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});
std::optional<CapellaForkStore> prepare_capella_store_slashing(const CapellaForkStore&,
 const IndexedConsensusAttestation&,const IndexedConsensusAttestation&,
 const CapellaForkBlockRules&,const std::function<void()>& checkpoint={});
CapellaForkStore prepare_capella_store_tick(const CapellaForkStore&,uint64_t time_ms,
 const CapellaForkBlockRules&,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});
}

namespace nano {
// Initialize from an externally trusted anchor (not peer-supplied snapshot
// admission). The unsigned block/state binding is checked, not its trust origin.
CapellaForkStore initialize_capella_fork_store(const CapellaSlotState& anchor,
 std::span<const uint8_t> unsigned_block,const Block& execution_block,const State& execution_state,const std::vector<Receipt>& execution_receipts,
 const std::vector<Header>& execution_ancestors,const CapellaForkBlockRules&,
 const std::function<void()>& checkpoint={},std::shared_ptr<const ExecutionDepositHistory> anchor_deposits={});
}

namespace nano {
// Returns the paired record at the LMD-GHOST head; no canonical DB publication.
std::shared_ptr<const CapellaForkBlock> capella_fork_head(const CapellaForkStore&,
 const CapellaForkBlockRules&,const std::function<void()>& checkpoint={});
}

namespace nano {
// Validate a trusted record's Beacon/native execution roots and receipt binding.
// Does not authenticate its source or replay its state transition.
void validate_capella_execution_record(const CapellaForkBlock&,const CapellaForkBlockRules&,
 const std::function<void()>& checkpoint={});
}

namespace nano {
// Branch-bound history for local execution; Store and parent must be authenticated.
Bytes capella_execution_history(const CapellaForkStore&,const ConsensusRoot& parent,
 const Int& height,const std::function<void()>& checkpoint={});
}

namespace nano {
ConsensusRoot capella_network_anchor(const CapellaForkStore&);
// Pure candidate transformation. Does not publish or delete persisted history.
// Native deposit ancestry is retained as authenticated commitments.
CapellaForkStore prepare_capella_pruned_store(const CapellaForkStore&,
 const ConsensusRoot& new_anchor,const CapellaForkBlockRules&,
 const std::function<void()>& checkpoint={});
}
