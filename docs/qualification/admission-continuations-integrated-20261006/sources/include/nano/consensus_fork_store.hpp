#pragma once
#include "nano/consensus_fork_clock.hpp"
#include "nano/consensus_fork_state.hpp"
#include "nano/consensus_transition.hpp"
namespace nano {
struct Phase0ForkBlock {ConsensusBeaconHeader header;Phase0SlotState state;};
struct Phase0ForkStore {
 Phase0ForkTimedView timed;
 std::map<ConsensusRoot,Phase0ForkBlock> blocks;
 std::map<ConsensusRoot,bool> timely;
 std::map<ConsensusRoot,ConsensusCheckpoint> unrealized_justifications;
};
struct Phase0ForkBlockRules {
 Phase0TransitionRules transition;
 uint64_t slot_duration_ms,attestation_due_ms,proposer_score_boost;
};
// Phase0 on_block on a trusted, internally consistent in-memory Store. Full
// signature/operation/state-root validation precedes candidate publication.
// Unknown parent/future/incompatible-finality/invalid blocks return nullopt;
// invalid trusted context, cancellation and resource errors may throw.
// Duplicate block roots return the unchanged Store (as in the reference).
// Caller initializes the authenticated anchor and publishes/persists all fields
// together. Not a P2P adapter, durable Store, execution binding or startup policy.
std::optional<Phase0ForkStore> prepare_phase0_fork_block(const Phase0ForkStore&,
 std::span<const uint8_t> signed_block,const Phase0ForkBlockRules&,uint64_t maximum_slots,
 const std::function<void()>& checkpoint={});
// Store-bound gossip admission; the sender cannot supply committee/context or
// bypass the gossip time window with an "already included" flag.
std::optional<Phase0ForkStore> prepare_phase0_store_vote(const Phase0ForkStore&,
 const Phase0SignedAttestation&,const Phase0ForkBlockRules&,uint64_t maximum_slots,
 const std::function<void()>& checkpoint={});
std::optional<Phase0ForkStore> prepare_phase0_store_slashing(const Phase0ForkStore&,
 const IndexedConsensusAttestation&,const IndexedConsensusAttestation&,
 const Phase0ForkBlockRules&,const std::function<void()>& checkpoint={});
Phase0ForkStore prepare_phase0_store_tick(const Phase0ForkStore&,uint64_t time_ms,
 const Phase0ForkBlockRules&,uint64_t maximum_slots,
 const std::function<void()>& checkpoint={});
}
