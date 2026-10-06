#pragma once
#include "nano/consensus_beacon_state.hpp"
// Internal representation adapter, never a consensus root or public admission
// API. Only Capella roots authenticate the surrounding state and slots.
namespace nano::capella_detail {
struct Projection {Bytes common,execution_header,cursors,summaries;};
inline size_t execution_field(const AltairStateLimits& l){return 272+64*l.slots_per_historical_root+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector+229+96*l.sync_committee_size;}
inline std::array<size_t,7> offsets(const AltairStateLimits& l){const size_t h=176+64*l.slots_per_historical_root,p=h+96+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector;return {h,h+76,h+88,h+92,p,p+4,p+129};}
inline uint32_t offset(const Bytes& b,size_t at){uint32_t n=0;for(unsigned i=0;i<4;++i)n|=uint32_t(b.at(at+i))<<(8*i);return n;}
inline void put_offset(Bytes& b,size_t at,size_t n){if(n>UINT32_MAX)throw std::length_error("Capella SSZ offset overflow");for(unsigned i=0;i<4;++i)b.at(at+i)=uint8_t(n>>(8*i));}
inline Projection project(const Bytes& b,const AltairStateLimits& l,const ExecutionPayloadLimits& e,size_t budget,const std::function<void()>& check){
 capella_beacon_state_root(b,l,e,budget,check);
 const size_t field=execution_field(l),start=offset(b,field),summaries=offset(b,field+20);
 Projection out{Bytes(b.begin(),b.begin()+field),Bytes(b.begin()+start,b.begin()+summaries),Bytes(b.begin()+field+4,b.begin()+field+20),Bytes(b.begin()+summaries,b.end())};
 out.common.insert(out.common.end(),b.begin()+field+24,b.begin()+start);
 for(auto at:offsets(l))put_offset(out.common,at,offset(b,at)-24);
 return out;
}
inline Bytes restore(const Bytes& common,const Bytes& header,const Bytes& cursors,const Bytes& summaries,const AltairStateLimits& l,const ExecutionPayloadLimits& e,size_t budget,const std::function<void()>& check){
 altair_beacon_state_root(common,l,check);
 const uint64_t total=uint64_t(common.size())+24+header.size()+summaries.size();
 if(total>budget||total>UINT32_MAX)throw std::length_error("Capella state budget exceeded");
 const size_t field=execution_field(l);Bytes out(common.begin(),common.begin()+field);out.resize(out.size()+24);
 if(cursors.size()!=16)throw std::invalid_argument("Capella cursor width");
 std::copy(cursors.begin(),cursors.end(),out.begin()+field+4);
 out.insert(out.end(),common.begin()+field,common.end());for(auto at:offsets(l))put_offset(out,at,offset(common,at)+24);
 put_offset(out,field,out.size());out.insert(out.end(),header.begin(),header.end());put_offset(out,field+20,out.size());out.insert(out.end(),summaries.begin(),summaries.end());capella_beacon_state_root(out,l,e,budget,check);return out;
}
}
