#pragma once
// Private implementation/test interface. No caller-provided roots or cache.
#include "nano/consensus_capella_slots.hpp"
#include <array>
#include <limits>
#include <vector>
namespace nano::capella_detail {
// Counts actual direct BeaconState/batch Merkle pairs, not hashes inside the
// unchanged epoch/execution-header helpers. This is work evidence, not a quota.
// On failure no state is returned; counters retain completed operations and
// mark the unfinished operation unknown. Callback mutations of this output
// never influence preparation: the authoritative counters are private copies.
struct SlotMerkleWork {
 uint64_t slots_completed=0,full_state_roots_started=0,full_state_roots_completed=0;
 uint64_t slot_merkle_pairs=0,epochs_started=0,epochs_completed=0;
 bool incomplete_work_unknown=false;
};
struct SlotMerkleCounter {
 SlotMerkleWork value{};SlotMerkleWork* output;
 explicit SlotMerkleCounter(SlotMerkleWork* p):output(p){publish();}
 void publish()const noexcept{if(output)*output=value;}
 void increment(uint64_t& n)noexcept{
  if(n==std::numeric_limits<uint64_t>::max())value.incomplete_work_unknown=true;
  else ++n;
  publish();
 }
 void failed()noexcept{value.incomplete_work_unknown=true;publish();}
};
ConsensusRoot capella_beacon_state_root_observed(std::span<const uint8_t>,
 const AltairStateLimits&,const ExecutionPayloadLimits&,size_t maximum_bytes,
 const std::function<void()>&,SlotMerkleWork*);
CapellaSlotState prepare_capella_slots_observed(const CapellaSlotState&,
 const CapellaEpochRules&,const AltairStateLimits&,uint64_t target_slot,
 uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>&,SlotMerkleWork*);

// Only this owned local object can update its byte snapshot and field roots.
// Mutation methods correspond to process_slot, slot++, and a complete epoch
// replacement. No mutable view of cached bytes/roots escapes this interface.
class OwnedCapellaSlotMerkle {
 struct Ring {size_t leaves=0;std::vector<ConsensusRoot> nodes;};
 CapellaSlotState state_;
 AltairStateLimits limits_;
 ExecutionPayloadLimits execution_;
 size_t maximum_bytes_;
 std::function<void()> check_;
 SlotMerkleCounter& work_;
 std::array<ConsensusRoot,28> fields_{};
 Ring blocks_,states_;
 ConsensusRoot root_{};
 void rebuild();
 Ring ring(size_t base);
 void update(Ring&,size_t position,const ConsensusRoot&);
 ConsensusRoot compose();
public:
 OwnedCapellaSlotMerkle(CapellaSlotState,AltairStateLimits,ExecutionPayloadLimits,
  size_t maximum_bytes,std::function<void()>,SlotMerkleCounter&);
 const CapellaSlotState& state()const noexcept{return state_;}
 void process_slot_roots(uint64_t slot);
 void reset_epoch(CapellaSlotState);
 void set_slot(uint64_t next_slot);
 CapellaSlotState finish();
};
}
