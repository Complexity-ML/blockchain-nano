#pragma once
#include <array>
#include <cstddef>

// One catalog per peer still owns the existing relay budget and send pacer.
// Interleave time-sensitive attestations with every background category;
// keep head delivery, sync, slashing, credential changes and exits non-starving.
// State is bounded (no ever-growing counter) and belongs to one peer session.
class CapellaRelaySchedule {
 std::size_t background_=0;
 bool attestation_=false,history_first_=false,catchup_insert_=true;
public:
 // Called only when next() grants a block turn. A moving/unresponsive head
 // cannot consume every historical catch-up opportunity on this peer.
 bool next_block_history_first(){const bool selected=history_first_;history_first_=!history_first_;return selected;}
 // At most one extra block opportunity precedes each mandatory normal turn.
 // Mode changes never reset that normal cycle: all categories remain served
 // within twenty opportunities even if catch-up toggles adversarially.
 unsigned next(bool preparing){
  if(preparing&&catchup_insert_){catchup_insert_=false;return 0;}
  catchup_insert_=true;return next();
 }
 unsigned next(){
  if(attestation_){attestation_=false;return 1;}
  static constexpr std::array<unsigned,5> background{0,2,3,4,5};
  const auto kind=background[background_];
  background_=(background_+1)%background.size();attestation_=true;return kind;
 }
};
