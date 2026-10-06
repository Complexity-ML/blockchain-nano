#pragma once
#include "nano/consensus_beacon_state.hpp"
#include "nano/types.hpp"
namespace nano {
struct AltairInactivityRules {
 uint64_t slots_per_epoch,score_bias,score_recovery_rate,minimum_epochs_to_inactivity_penalty;
};
// Isolated Altair process_inactivity_updates component. SSZ/count coherence is
// checked here; lifecycle/genesis/authentication must be checked at admission.
// Official component fixtures include synthetic impossible lifecycle histories.
// Returns private SSZ bytes; only inactivity scores change. Does not advance
// slots, execute other epoch components, provide replay protection or persist.
Bytes prepare_altair_inactivity_updates(std::span<const uint8_t>,const AltairStateLimits&,
 const AltairInactivityRules&,const std::function<void()>& checkpoint={});
struct AltairRewardRules {
 uint64_t slots_per_epoch,effective_balance_increment,base_reward_factor;
 uint64_t score_bias,inactivity_penalty_quotient,minimum_epochs_to_inactivity_penalty;
 std::array<uint64_t,3> participation_weights;
 uint64_t weight_denominator;
};
// Isolated process_rewards_and_penalties, on the state AFTER inactivity updates
// in a full epoch pipeline. Source/target/head deltas are applied in that order,
// then inactivity deltas. Saturation is per delta, not after a net sum.
// Same admission and replay preconditions as the inactivity component above.
Bytes prepare_altair_rewards(std::span<const uint8_t>,const AltairStateLimits&,
 const AltairRewardRules&,const std::function<void()>& checkpoint={});
struct AltairFfgRules {uint64_t slots_per_epoch,effective_balance_increment;};
// Isolated Altair justification/finalization from authenticated participation
// flags. Changes only justification bits/checkpoints; not gossip admission.
Bytes prepare_altair_justification_finalization(std::span<const uint8_t>,const AltairStateLimits&,
 const AltairFfgRules&,const std::function<void()>& checkpoint={});
struct AltairSyncRules {
 uint64_t slots_per_epoch,epochs_per_sync_committee_period,min_seed_lookahead,maximum_effective_balance;
 uint16_t shuffle_rounds;
};
// Isolated sync committee rotation. Rejection-sampling budget is a local work
// limit, not a consensus validator cap. Keeps repeated selected public keys.
Bytes prepare_altair_sync_updates(std::span<const uint8_t>,const AltairStateLimits&,
 const AltairSyncRules&,uint64_t maximum_trials,const std::function<void()>& checkpoint={});
// Isolated participation rotation on a coherent authenticated state. Call once
// at the prescribed point of epoch processing; no replay guard is supplied here.
Bytes prepare_altair_participation_updates(std::span<const uint8_t>,const AltairStateLimits&,
 const std::function<void()>& checkpoint={});
}
