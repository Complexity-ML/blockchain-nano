#include "nano/consensus_capella_slots.hpp"
#include "consensus_capella_slot_merkle.hpp"
#include "consensus_capella_projection.hpp"
#include "consensus_epoch_common.hpp"
namespace nano {namespace {
void require(bool ok){if(!ok)throw std::invalid_argument("incoherent Capella epoch/slot state");}
uint64_t number(const Bytes& b,size_t at,size_t width=8){require(at<=b.size()&&width<=b.size()-at);uint64_t n=0;for(size_t i=0;i<width;++i)n|=uint64_t(b[at+i])<<(8*i);return n;}
ConsensusRoot root_at(const Bytes& b,size_t at){require(at<=b.size()&&32<=b.size()-at);ConsensusRoot r{};std::copy_n(b.begin()+at,32,r.begin());return r;}
// Used only after complete schema validation, which bounds every vector and
// offset. Common Altair fields precede the Capella header, cursors and summaries.
struct Layout {
 std::array<size_t,7> lists;
 size_t execution;
 explicit Layout(const AltairStateLimits& l){
  const size_t historical=176+64*l.slots_per_historical_root;
  const size_t previous=historical+96+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector;
  lists={historical,historical+76,historical+88,historical+92,previous,previous+4,previous+129};
  execution=previous+229+96*l.sync_committee_size;
 }
};
void config(const CapellaEpochRules& r,const AltairStateLimits& l){
 validate_altair_epoch_rules(r.operations,l);
 require(r.operations.ffg.slots_per_epoch&&l.slots_per_historical_root>=r.operations.ffg.slots_per_epoch&&l.slots_per_historical_root%r.operations.ffg.slots_per_epoch==0);
 require(r.execution.extra_data_bytes&&r.execution.transaction_bytes&&r.execution.transactions&&r.execution.transactions<=UINT32_MAX/4);
}
void coherent(const CapellaSlotState& state,const AltairStateLimits& l){
 // Called only after complete SSZ schema validation has bounded this layout.
 const Layout layout(l);const auto& b=state.ssz;
 const auto v=number(b,layout.lists[2],4),balances=number(b,layout.lists[3],4),previous=number(b,layout.lists[4],4),current=number(b,layout.lists[5],4),scores=number(b,layout.lists[6],4),end=number(b,layout.execution,4),count=(balances-v)/121;
 require((previous-balances)/8==count&&current-previous==count&&scores-current==count&&(end-scores)/8==count);
}
ConsensusRoot validate(const CapellaSlotState& state,const CapellaEpochRules& r,
 const AltairStateLimits& l,const std::function<void()>& check){
 config(r,l);const auto root=capella_beacon_state_root(state.ssz,l,r.execution,r.maximum_state_bytes,check);
 coherent(state,l);
 return root;
}
}
CapellaSlotState prepare_capella_epoch(const CapellaSlotState& input,const CapellaEpochRules& r,
 const AltairStateLimits& l,uint64_t expected,uint64_t trials,const std::function<void()>& check){
 validate(input,r,l,check);
 const auto projection=capella_detail::project(input.ssz,l,r.execution,r.maximum_state_bytes,check);
 // Common epoch operations do not calculate slot state roots.
 auto updated=detail::prepare_common_epoch({projection.common,input.last_processed_epoch},r.operations,l,expected,trials,check,false);
 auto summaries=projection.summaries;
 if((expected+1)%(l.slots_per_historical_root/r.operations.ffg.slots_per_epoch)==0){
  if(summaries.size()/64>=l.historical_roots_limit)throw std::length_error("Capella historical summary limit");
  for(size_t base:{size_t(176),size_t(176+32*l.slots_per_historical_root)}){
   std::vector<ConsensusRoot> nodes;for(size_t i=0;i<l.slots_per_historical_root;++i){if(check)check();nodes.push_back(root_at(input.ssz,base+32*i));}
   size_t width=1;while(width<nodes.size())width*=2;nodes.resize(width);
   while(nodes.size()>1){for(size_t i=0;i<nodes.size()/2;++i){if(check)check();nodes[i]=consensus_signing_root(nodes[2*i],nodes[2*i+1]);}nodes.resize(nodes.size()/2);}
   summaries.insert(summaries.end(),nodes[0].begin(),nodes[0].end());
  }
 }
 auto post=capella_detail::restore(updated.ssz,projection.execution_header,projection.cursors,summaries,l,r.execution,r.maximum_state_bytes,check);
 CapellaSlotState out{std::move(post),updated.last_processed_epoch};validate(out,r,l,check);if(check)check();return out;
}
CapellaSlotState prepare_capella_slots(const CapellaSlotState& input,const CapellaEpochRules& r,
 const AltairStateLimits& l,uint64_t target,uint64_t maximum_slots,uint64_t trials,const std::function<void()>& check){
 return capella_detail::prepare_capella_slots_observed(input,r,l,target,maximum_slots,trials,check,nullptr);
}
CapellaSlotState capella_detail::prepare_capella_slots_observed(const CapellaSlotState& input,
 const CapellaEpochRules& rules,const AltairStateLimits& limits,uint64_t target,
 uint64_t maximum_slots,uint64_t trials,const std::function<void()>& checkpoint,SlotMerkleWork* output){
 SlotMerkleCounter work(output);
 try{
  // Reject an excessive copy before allocation. No callback runs until bytes,
  // metadata, rules, limits and the callback itself have detached ownership.
  require(input.ssz.size()<=rules.maximum_state_bytes&&input.ssz.size()<=UINT32_MAX);
  const auto r=rules;const auto l=limits;const auto check=checkpoint;auto owned=input;
  config(r,l);
  OwnedCapellaSlotMerkle out(std::move(owned),l,r.execution,r.maximum_state_bytes,check,work);
  coherent(out.state(),l);const auto slots=r.operations.ffg.slots_per_epoch;
  const auto start=number(out.state().ssz,40),epoch=start/slots;
  require(target>start&&number(out.state().ssz,64)<=start&&(!out.state().last_processed_epoch||(epoch&&*out.state().last_processed_epoch==epoch-1)));
  if(target-start>maximum_slots)throw std::length_error("Capella slot work budget exceeded");
  for(uint64_t slot=start;slot<target;++slot){
   if(check)check();
   // Exact process_slot order: pre-slot root, state ring, zero header root,
   // block ring, unchanged process_epoch at its real boundary, then slot++.
   out.process_slot_roots(slot);
   if((slot+1)%slots==0){
    work.increment(work.value.epochs_started);
    auto post=prepare_capella_epoch(out.state(),r,l,slot/slots,trials,check);
    work.increment(work.value.epochs_completed);out.reset_epoch(std::move(post));
   }
   out.set_slot(slot+1);work.increment(work.value.slots_completed);
  }
  coherent(out.state(),l);auto result=out.finish();work.publish();return result;
 }catch(...){work.failed();throw;}
}
}
