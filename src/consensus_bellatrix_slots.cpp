#include "nano/consensus_bellatrix_slots.hpp"
namespace nano {namespace {
void require(bool ok){if(!ok)throw std::invalid_argument("incoherent Bellatrix epoch/slot state");}
uint64_t number(const Bytes& b,size_t at,size_t width=8){require(at<=b.size()&&width<=b.size()-at);uint64_t n=0;for(size_t i=0;i<width;++i)n|=uint64_t(b[at+i])<<(8*i);return n;}
void put(Bytes& b,size_t at,uint64_t n,size_t width=8){require(at<=b.size()&&width<=b.size()-at&&(width!=4||n<=UINT32_MAX));for(size_t i=0;i<width;++i)b[at+i]=uint8_t(n>>(8*i));}
ConsensusRoot root_at(const Bytes& b,size_t at){require(at<=b.size()&&32<=b.size()-at);ConsensusRoot r{};std::copy_n(b.begin()+at,32,r.begin());return r;}
void put_root(Bytes& b,size_t at,const ConsensusRoot& r){require(at<=b.size()&&32<=b.size()-at);std::copy(r.begin(),r.end(),b.begin()+at);}
// Used only after complete schema validation, which bounds every vector and
// offset. The first 24 fields are identical to Altair; the 25th is preserved.
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
ConsensusRoot validate(const BellatrixSlotState& state,const BellatrixEpochRules& r,
 const AltairStateLimits& l,const std::function<void()>& check){
 validate_altair_epoch_rules(r.operations,l);
 require(r.execution.extra_data_bytes&&r.execution.transaction_bytes&&r.execution.transactions&&r.execution.transactions<=UINT32_MAX/4);
 const auto root=bellatrix_beacon_state_root(state.ssz,l,r.execution,r.maximum_state_bytes,check);
 const Layout layout(l);const auto& b=state.ssz;
 const auto v=number(b,layout.lists[2],4),balances=number(b,layout.lists[3],4),previous=number(b,layout.lists[4],4),current=number(b,layout.lists[5],4),scores=number(b,layout.lists[6],4),end=number(b,layout.execution,4),count=(balances-v)/121;
 require((previous-balances)/8==count&&current-previous==count&&scores-current==count&&(end-scores)/8==count);
 return root;
}
}
BellatrixSlotState prepare_bellatrix_epoch(const BellatrixSlotState& input,const BellatrixEpochRules& r,
 const AltairStateLimits& l,uint64_t expected,uint64_t trials,const std::function<void()>& check){
 validate(input,r,l,check);const Layout layout(l);const auto& original=input.ssz;
 const size_t execution_start=number(original,layout.execution,4);
 // A private lossless projection reuses epoch operations that do not calculate
 // slot state roots. Never feed this projection into Altair process_slots.
 AltairSlotState projected{Bytes(original.begin(),original.begin()+layout.execution),input.last_processed_epoch};
 projected.ssz.insert(projected.ssz.end(),original.begin()+layout.execution+4,original.begin()+execution_start);
 for(auto at:layout.lists)put(projected.ssz,at,number(original,at,4)-4,4);
 auto updated=prepare_altair_epoch(projected,r.operations,l,expected,trials,check);
 const size_t header_bytes=original.size()-execution_start;
 const size_t total=updated.ssz.size()+4+header_bytes;
 if(total>r.maximum_state_bytes||total>UINT32_MAX)throw std::length_error("Bellatrix epoch state budget exceeded");
 Bytes post(updated.ssz.begin(),updated.ssz.begin()+layout.execution);post.resize(post.size()+4);
 post.insert(post.end(),updated.ssz.begin()+layout.execution,updated.ssz.end());
 for(auto at:layout.lists)put(post,at,number(updated.ssz,at,4)+4,4);
 put(post,layout.execution,post.size(),4);post.insert(post.end(),original.begin()+execution_start,original.end());
 BellatrixSlotState out{std::move(post),updated.last_processed_epoch};validate(out,r,l,check);if(check)check();return out;
}
BellatrixSlotState prepare_bellatrix_slots(const BellatrixSlotState& input,const BellatrixEpochRules& r,
 const AltairStateLimits& l,uint64_t target,uint64_t maximum_slots,uint64_t trials,const std::function<void()>& check){
 auto state_root=validate(input,r,l,check);const auto slots=r.operations.ffg.slots_per_epoch;
 const auto start=number(input.ssz,40),epoch=start/slots;
 require(target>start&&number(input.ssz,64)<=start&&(!input.last_processed_epoch||(epoch&&*input.last_processed_epoch==epoch-1)));
 if(target-start>maximum_slots)throw std::length_error("Bellatrix slot work budget exceeded");
 auto out=input;
 for(uint64_t slot=start;slot<target;++slot){
  if(check)check();
  if(slot!=start)state_root=bellatrix_beacon_state_root(out.ssz,l,r.execution,r.maximum_state_bytes,check);
  const auto pos=slot%l.slots_per_historical_root;
  put_root(out.ssz,176+32*l.slots_per_historical_root+32*pos,state_root);
  if(root_at(out.ssz,112)==ConsensusRoot{})put_root(out.ssz,112,state_root);
  const ConsensusBeaconHeader h{number(out.ssz,64),number(out.ssz,72),root_at(out.ssz,80),root_at(out.ssz,112),root_at(out.ssz,144)};
  put_root(out.ssz,176+32*pos,consensus_beacon_header_root(h));
  if((slot+1)%slots==0)out=prepare_bellatrix_epoch(out,r,l,slot/slots,trials,check);
  put(out.ssz,40,slot+1);
 }
 validate(out,r,l,check);if(check)check();return out;
}
}
