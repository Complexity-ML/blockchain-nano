#pragma once
#include <array>
#include <cstddef>

// One catalog per peer still owns the existing relay budget and send pacer.
// Interleave time-sensitive attestations with every background category;
// keep head delivery, sync, slashing, credential changes and exits non-starving.
// State is bounded (no ever-growing counter) and belongs to one peer session.
class CapellaRelaySchedule {
 std::size_t background_=0;
 bool attestation_=false;
public:
 unsigned next(){
  if(attestation_){attestation_=false;return 1;}
  static constexpr std::array<unsigned,5> background{0,2,3,4,5};
  const auto kind=background[background_];
  background_=(background_+1)%background.size();attestation_=true;return kind;
 }
};
