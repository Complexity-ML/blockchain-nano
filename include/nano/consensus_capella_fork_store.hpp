#pragma once
#include "nano/consensus_fork_clock.hpp"
#include "nano/consensus_deposit_history.hpp"
#include "nano/consensus_capella_fork_state.hpp"
#include "nano/consensus_capella_transition.hpp"
namespace nano {
struct CapellaForkBlock {ConsensusBeaconHeader header;CapellaSlotState state;Block execution_block;State execution_state;std::vector<Receipt> receipts;std::shared_ptr<const ExecutionDepositHistory> deposit_history;};
struct CapellaForkStore {
 Phase0ForkTimedView timed;
 // Authenticated newest-first execution ancestors of the anchor (up to 256).
 std::vector<Header> anchor_execution_ancestors;
 std::map<ConsensusRoot,CapellaForkBlock> blocks;
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
// Receive a block as one private candidate: on_block first, then its embedded
// attestations (verified-block time window) and attester slashing evidence.
// The relaxed vote window is never exposed to gossip callers. Votes whose fork
// context/ancestry is unavailable or invalid do not invalidate the valid block;
// they are not queued here. Caller owns retry/retrieval policy and persistence.
std::optional<CapellaForkStore> prepare_capella_received_block(const CapellaForkStore&,
 std::span<const uint8_t> signed_block,const CapellaForkBlockRules&,uint64_t maximum_slots,
 uint64_t maximum_sync_trials,const std::function<void()>& checkpoint={});
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
const CapellaForkBlock& capella_fork_head(const CapellaForkStore&,
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
