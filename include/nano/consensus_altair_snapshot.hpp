#pragma once
#include "nano/consensus_altair_transition.hpp"
namespace nano {
// Local persistence envelope of an already verified state. Not peer snapshot
// admission or persistence of the fork-choice Store. All transition rules and
// the epoch replay marker are bound to the canonical envelope.
Json altair_slot_snapshot(const AltairSlotState&,const AltairTransitionRules&);
// expected_root must come from an independently authenticated checkpoint, not
// the envelope itself. Bound JSON input before parsing; SSZ allocation is bounded
// here. Caller commits this envelope atomically with its authenticated checkpoint.
AltairSlotState altair_slot_from_snapshot(const Json&,const AltairTransitionRules&,
 const ConsensusRoot& expected_root,size_t maximum_state_bytes);
}
