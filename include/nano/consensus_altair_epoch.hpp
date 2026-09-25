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
}
