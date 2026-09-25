#pragma once
#include "nano/consensus_fork_clock.hpp"
#include "nano/consensus_altair_fork_state.hpp"
#include "nano/consensus_altair_transition.hpp"
namespace nano {
struct AltairForkBlock {ConsensusBeaconHeader header;AltairSlotState state;};
struct AltairForkStore {
 Phase0ForkTimedView timed;
 std::map<ConsensusRoot,AltairForkBlock> blocks;
 std::map<ConsensusRoot,bool> timely;
 std::map<ConsensusRoot,ConsensusCheckpoint> unrealized_justifications;
};
enum class AltairProposerBoostProfile { Release160Beta0, ShufflingDependent };
struct AltairForkBlockRules {
 AltairTransitionRules transition;
 uint64_t slot_duration_ms,attestation_due_ms,proposer_score_boost;
 // Explicit spec revision: the 2025 fixtures predate the dependent-root gate.
 AltairProposerBoostProfile boost_profile=AltairProposerBoostProfile::ShufflingDependent;
};
// Altair on_block on a trusted, internally consistent in-memory Store. Full
// signature/operation/state-root validation precedes candidate publication.
// Unknown parent/future/incompatible-finality/invalid blocks return nullopt;
// invalid trusted context, cancellation and resource errors may throw.
// Duplicate block roots return the unchanged Store (as in the reference).
// Caller initializes the authenticated anchor and publishes/persists all fields
// together. Not a P2P adapter, durable Store, execution binding or startup policy.
std::optional<AltairForkStore> prepare_altair_fork_block(const AltairForkStore&,
 std::span<const uint8_t> signed_block,const AltairForkBlockRules&,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});
// Receive a block as one private candidate: on_block first, then its embedded
// attestations (verified-block time window) and attester slashing evidence.
// The relaxed vote window is never exposed to gossip callers. Votes whose fork
// context/ancestry is unavailable or invalid do not invalidate the valid block;
// they are not queued here. Caller owns retry/retrieval policy and persistence.
std::optional<AltairForkStore> prepare_altair_received_block(const AltairForkStore&,
 std::span<const uint8_t> signed_block,const AltairForkBlockRules&,uint64_t maximum_slots,
 uint64_t maximum_sync_trials,const std::function<void()>& checkpoint={});
// Store-bound gossip admission; the sender cannot supply committee/context or
// bypass the gossip time window with an "already included" flag.
std::optional<AltairForkStore> prepare_altair_store_vote(const AltairForkStore&,
 const Phase0SignedAttestation&,const AltairForkBlockRules&,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});
std::optional<AltairForkStore> prepare_altair_store_slashing(const AltairForkStore&,
 const IndexedConsensusAttestation&,const IndexedConsensusAttestation&,
 const AltairForkBlockRules&,const std::function<void()>& checkpoint={});
AltairForkStore prepare_altair_store_tick(const AltairForkStore&,uint64_t time_ms,
 const AltairForkBlockRules&,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});
}

namespace nano {
// Initialize from an externally trusted anchor (not peer-supplied snapshot
// admission). The unsigned block/state binding is checked, not its trust origin.
AltairForkStore initialize_altair_fork_store(const AltairSlotState& anchor,
 std::span<const uint8_t> unsigned_block,const AltairForkBlockRules&,
 const std::function<void()>& checkpoint={});
}
