#pragma once
#include "nano/consensus_altair_transition.hpp"
namespace nano {
// Explicit local configuration, no preset defaults. Every field must be present
// as a canonical quantity; narrowing and unknown fields are rejected. This is
// not authentication of rules supplied by a peer. Bound and authenticate the
// configuration bytes before JSON parsing; snapshot readers still require an
// independently supplied expected profile/root.
Json altair_transition_profile(const AltairTransitionRules&);
AltairTransitionRules altair_transition_from_profile(const Json&);
// Verify a bounded local profile file against a digest obtained independently.
// Never obtain the expected digest from that same untrusted file or peer.
AltairTransitionRules load_altair_transition_profile(std::string_view,const Bytes& expected_digest,size_t maximum_bytes);

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
