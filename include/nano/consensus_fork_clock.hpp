#pragma once
#include "nano/consensus_fork_vote.hpp"
namespace nano {
struct Phase0ForkClock {
 uint64_t genesis_time_ms,time_ms;
 ConsensusCheckpoint unrealized_justified,unrealized_finalized;
};
struct Phase0ForkTimedView {Phase0ForkChoiceView view;Phase0ForkClock clock;};
// Local clock event on an authenticated Store. Time must not move backwards.
// If justification is promoted, supply the authenticated target-state context
// for that checkpoint (e.g. derived by prepare_phase0_fork_state_contexts).
// Candidate clock/view must be published together. No wall-clock source, block
// reception, unrealized FFG calculation, persistence or networking is provided.
// Catch-up is O(1) in skipped slots: no events interleave inside this operation,
// so the first crossed epoch boundary performs the only effective promotion.
Phase0ForkTimedView prepare_phase0_fork_tick(const Phase0ForkTimedView&,
 uint64_t time_ms,uint64_t slot_duration_ms,uint64_t slots_per_epoch,
 const Phase0ForkVoteContext* promoted_context=nullptr,
 const std::function<void()>& checkpoint={});
}
