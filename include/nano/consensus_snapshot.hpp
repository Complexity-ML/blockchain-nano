#pragma once
#include "nano/consensus_transition.hpp"
namespace nano {
// Canonical local persistence envelope for a verified Phase0 state. The replay
// marker must correspond to the last completed epoch. Not snapshot admission.
Json phase0_slot_snapshot(const Phase0SlotState&,const Phase0TransitionRules&);
// expected_root MUST be obtained from the authenticated canonical checkpoint,
// not from this envelope. A self-consistent hash is not proof of authenticity.
// Limits decoded SSZ allocation; caller also bounds JSON input before parsing.
Phase0SlotState phase0_slot_from_snapshot(const Json&,const Phase0TransitionRules&,
 const ConsensusRoot& expected_root,size_t maximum_state_bytes);
}
