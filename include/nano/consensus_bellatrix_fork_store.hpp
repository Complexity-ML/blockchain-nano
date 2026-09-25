#pragma once
#include "nano/consensus_fork_clock.hpp"
#include "nano/consensus_bellatrix_fork_state.hpp"
#include "nano/consensus_bellatrix_transition.hpp"
namespace nano {
struct BellatrixForkBlock {ConsensusBeaconHeader header;BellatrixSlotState state;Block execution_block;State execution_state;std::vector<Receipt> receipts;};
struct BellatrixForkStore {
 Phase0ForkTimedView timed;
 // Authenticated newest-first execution ancestors of the anchor (up to 256).
 std::vector<Header> anchor_execution_ancestors;
 std::map<ConsensusRoot,BellatrixForkBlock> blocks;
 std::map<ConsensusRoot,bool> timely;
 std::map<ConsensusRoot,ConsensusCheckpoint> unrealized_justifications;
};
enum class BellatrixProposerBoostProfile { Release160Beta0, ShufflingDependent };
struct BellatrixForkBlockRules {
 BellatrixTransitionRules transition;
 uint64_t slot_duration_ms,attestation_due_ms,proposer_score_boost;
 // Explicit spec revision: the 2025 fixtures predate the dependent-root gate.
 BellatrixProposerBoostProfile boost_profile=BellatrixProposerBoostProfile::ShufflingDependent;
};
// Bellatrix on_block on a trusted, internally consistent in-memory Store. Full
// signature/operation/state-root validation precedes candidate publication.
// Unknown parent/future/incompatible-finality/invalid blocks return nullopt;
// invalid trusted context, cancellation and resource errors may throw.
// Duplicate block roots return the unchanged Store (as in the reference).
// Caller initializes the authenticated anchor and publishes/persists all fields
// together. Both Beacon and EVM parents are selected from the same branch.
// Not a P2P adapter, durable Store or startup trust policy.
std::optional<BellatrixForkStore> prepare_bellatrix_fork_block(const BellatrixForkStore&,
 std::span<const uint8_t> signed_block,const BellatrixForkBlockRules&,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});
// Receive a block as one private candidate: on_block first, then its embedded
// attestations (verified-block time window) and attester slashing evidence.
// The relaxed vote window is never exposed to gossip callers. Votes whose fork
// context/ancestry is unavailable or invalid do not invalidate the valid block;
// they are not queued here. Caller owns retry/retrieval policy and persistence.
std::optional<BellatrixForkStore> prepare_bellatrix_received_block(const BellatrixForkStore&,
 std::span<const uint8_t> signed_block,const BellatrixForkBlockRules&,uint64_t maximum_slots,
 uint64_t maximum_sync_trials,const std::function<void()>& checkpoint={});
// Store-bound gossip admission; the sender cannot supply committee/context or
// bypass the gossip time window with an "already included" flag.
std::optional<BellatrixForkStore> prepare_bellatrix_store_vote(const BellatrixForkStore&,
 const Phase0SignedAttestation&,const BellatrixForkBlockRules&,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});
std::optional<BellatrixForkStore> prepare_bellatrix_store_slashing(const BellatrixForkStore&,
 const IndexedConsensusAttestation&,const IndexedConsensusAttestation&,
 const BellatrixForkBlockRules&,const std::function<void()>& checkpoint={});
BellatrixForkStore prepare_bellatrix_store_tick(const BellatrixForkStore&,uint64_t time_ms,
 const BellatrixForkBlockRules&,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});
}

namespace nano {
// Initialize from an externally trusted anchor (not peer-supplied snapshot
// admission). The unsigned block/state binding is checked, not its trust origin.
BellatrixForkStore initialize_bellatrix_fork_store(const BellatrixSlotState& anchor,
 std::span<const uint8_t> unsigned_block,const Block& execution_block,const State& execution_state,const std::vector<Receipt>& execution_receipts,
 const std::vector<Header>& execution_ancestors,const BellatrixForkBlockRules&,
 const std::function<void()>& checkpoint={});
}

namespace nano {
// Returns the paired record at the LMD-GHOST head; no canonical DB publication.
const BellatrixForkBlock& bellatrix_fork_head(const BellatrixForkStore&,
 const BellatrixForkBlockRules&,const std::function<void()>& checkpoint={});
}
