#include "nano/consensus_beacon_state.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <stdexcept>
#include <vector>
namespace nano {
namespace {
using View=std::span<const uint8_t>;using Root=ConsensusRoot;
void require(bool ok){if(!ok)throw std::invalid_argument("invalid BeaconState SSZ");}
View part(View b,size_t at,size_t n){require(at<=b.size()&&n<=b.size()-at);return b.subspan(at,n);}
uint64_t little(View b){require(b.size()<=8);uint64_t x=0;for(size_t i=0;i<b.size();++i)x|=uint64_t(b[i])<<(8*i);return x;}
Root scalar(View b){require(b.size()<=32);Root r{};std::copy(b.begin(),b.end(),r.begin());return r;}
Root length_root(uint64_t n){Root r{};for(unsigned i=0;i<8;++i)r[i]=uint8_t(n>>(8*i));return r;}
struct Hasher {
 const std::function<void()>& cancel;
 void tick()const{if(cancel)cancel();}
 Root merkle(std::vector<Root> nodes,uint64_t limit)const{
  require(limit&&nodes.size()<=limit);Root zero{};if(nodes.empty())nodes.push_back(zero);
  const auto depth=std::bit_width(limit-1);
  for(unsigned level=0;level<depth;++level){
   if(nodes.size()%2)nodes.push_back(zero);
   for(size_t i=0;i<nodes.size()/2;++i){tick();nodes[i]=consensus_signing_root(nodes[2*i],nodes[2*i+1]);}
   nodes.resize(nodes.size()/2);zero=consensus_signing_root(zero,zero);
  }
  require(nodes.size()==1);return nodes[0];
 }
 Root container(std::initializer_list<Root> fields)const{return merkle(std::vector<Root>(fields),fields.size());}
 Root packed(View b,uint64_t chunks)const{
  std::vector<Root> nodes;nodes.reserve(b.size()/32+(b.size()%32!=0));
  for(size_t i=0;i<b.size();i+=32){tick();nodes.push_back(scalar(b.subspan(i,std::min(size_t(32),b.size()-i))));}
  return merkle(std::move(nodes),chunks);
 }
 Root mix(const Root& root,uint64_t count)const{return consensus_signing_root(root,length_root(count));}
 Root checkpoint(View b)const{require(b.size()==40);return container({scalar(b.first(8)),scalar(b.subspan(8))});}
 Root data(View b)const{require(b.size()==128);return container({scalar(b.first(8)),scalar(b.subspan(8,8)),scalar(b.subspan(16,32)),checkpoint(b.subspan(48,40)),checkpoint(b.subspan(88,40))});}
 Root header(View b)const{require(b.size()==112);return container({scalar(b.first(8)),scalar(b.subspan(8,8)),scalar(b.subspan(16,32)),scalar(b.subspan(48,32)),scalar(b.subspan(80,32))});}
 Root eth1(View b)const{require(b.size()==72);return container({scalar(b.first(32)),scalar(b.subspan(32,8)),scalar(b.subspan(40,32))});}
 Root validator(View b)const{
  require(b.size()==121&&b[88]<=1);
  // Validator has exactly eight fields: no padding subtrees or heap buffers.
  // Its 48-byte public key is two chunks, with the final 16 bytes zero-padded.
  tick();const auto key=consensus_signing_root(scalar(b.first(32)),scalar(b.subspan(32,16)));
  std::array<Root,8> nodes{key,scalar(b.subspan(48,32)),scalar(b.subspan(80,8)),scalar(b.subspan(88,1)),scalar(b.subspan(89,8)),scalar(b.subspan(97,8)),scalar(b.subspan(105,8)),scalar(b.subspan(113,8))};
  for(size_t width=nodes.size();width>1;width/=2)
   for(size_t i=0;i<width/2;++i){tick();nodes[i]=consensus_signing_root(nodes[2*i],nodes[2*i+1]);}
  return nodes[0];
 }
 Root bitlist(View b,uint64_t limit)const{
  require(limit&&!b.empty()&&b.back()!=0&&b.size()-1<=limit/8);
  const auto last=std::bit_width(unsigned(b.back()))-1;const uint64_t count=(uint64_t(b.size())-1)*8+last;require(count<=limit);
  std::vector<uint8_t> bits(b.begin(),b.end());bits.back()^=uint8_t(1U<<last);if(!last)bits.pop_back();
  return mix(packed(bits,(limit-1)/256+1),count);
 }
 Root pending(View b,uint64_t bit_limit)const{
  require(b.size()>=149&&little(b.first(4))==148);
  return container({bitlist(b.subspan(148),bit_limit),data(b.subspan(4,128)),scalar(b.subspan(132,8)),scalar(b.subspan(140,8))});
 }
 template<class F> Root fixed_list(View b,size_t width,uint64_t limit,F hash)const{
  require(limit&&b.size()%width==0&&b.size()/width<=limit);std::vector<Root> roots;roots.reserve(b.size()/width);
  for(size_t i=0;i<b.size();i+=width){tick();roots.push_back(hash(b.subspan(i,width)));}
  return mix(merkle(std::move(roots),limit),b.size()/width);
 }
 Root pending_list(View b,uint64_t limit,uint64_t bit_limit)const{
  if(b.empty())return mix(merkle({},limit),0);
  require(b.size()>=4);const auto first=little(b.first(4));require(first>=4&&first%4==0&&first<=b.size()&&first/4<=limit);
  std::vector<Root> nodes;const auto count=first/4;nodes.reserve(count);size_t start=first;
  for(size_t i=0;i<count;++i){tick();const size_t end=i+1==count?b.size():little(part(b,4*(i+1),4));require(start<=end&&end<=b.size());nodes.push_back(pending(b.subspan(start,end-start),bit_limit));start=end;}
  return mix(merkle(std::move(nodes),limit),count);
 }
};
}
namespace {
ConsensusRoot beacon_state_root(View b,const Phase0StateLimits& limits,uint64_t sync_size,const std::function<void()>& checkpoint,const ExecutionPayloadLimits* execution=nullptr,bool capella=false){
 require(b.size()<=UINT32_MAX&&limits.slots_per_historical_root&&limits.epochs_per_historical_vector&&limits.epochs_per_slashings_vector&&limits.historical_roots_limit&&limits.eth1_votes_limit&&limits.validator_registry_limit&&limits.pending_attestations_limit&&limits.validators_per_committee);
 Hasher h{checkpoint};h.tick();size_t cursor=0;
 auto take=[&](size_t n){auto v=part(b,cursor,n);cursor+=n;return v;};
 auto vector_size=[&](uint64_t count,size_t width){require(count<=UINT32_MAX/width);return size_t(count)*width;};
 const size_t field_count=capella?28:execution?25:sync_size?24:21;
 std::vector<Root> fields(field_count);std::vector<std::pair<size_t,size_t>> variable;
 auto offset=[&](size_t field){variable.emplace_back(field,little(take(4)));};
 fields[0]=scalar(take(8));fields[1]=scalar(take(32));fields[2]=scalar(take(8));
 const auto fork=take(16);fields[3]=h.container({scalar(fork.first(4)),scalar(fork.subspan(4,4)),scalar(fork.subspan(8,8))});
 fields[4]=h.header(take(112));
 fields[5]=h.packed(take(vector_size(limits.slots_per_historical_root,32)),limits.slots_per_historical_root);
 fields[6]=h.packed(take(vector_size(limits.slots_per_historical_root,32)),limits.slots_per_historical_root);
 offset(7);fields[8]=h.eth1(take(72));offset(9);fields[10]=scalar(take(8));offset(11);offset(12);
 fields[13]=h.packed(take(vector_size(limits.epochs_per_historical_vector,32)),limits.epochs_per_historical_vector);
 fields[14]=h.packed(take(vector_size(limits.epochs_per_slashings_vector,8)),(limits.epochs_per_slashings_vector-1)/4+1);
 offset(15);offset(16);const auto bits=take(1);require(bits[0]<=15);fields[17]=scalar(bits);
 for(size_t i=18;i<21;++i)fields[i]=h.checkpoint(take(40));
 if(sync_size){
  offset(21);
  for(size_t i=22;i<24;++i){
   const auto keys=take(vector_size(sync_size,48));std::vector<Root> roots;roots.reserve(sync_size);
   for(size_t at=0;at<keys.size();at+=48){h.tick();roots.push_back(h.packed(keys.subspan(at,48),2));}
   fields[i]=h.container({h.merkle(std::move(roots),sync_size),h.packed(take(48),2)});
  }
 }
 if(execution){require(sync_size);offset(24);}
 if(capella){require(execution);fields[25]=scalar(take(8));fields[26]=scalar(take(8));offset(27);}
 require(variable.front().second==cursor);
 for(size_t i=0;i<variable.size();++i){h.tick();const auto [field,start]=variable[i];const auto end=i+1==variable.size()?b.size():variable[i+1].second;require(start<=end&&end<=b.size());const auto value=b.subspan(start,end-start);
  switch(field){
   case 7:require(value.size()%32==0&&value.size()/32<=limits.historical_roots_limit);fields[field]=h.mix(h.packed(value,limits.historical_roots_limit),value.size()/32);break;
   case 9:fields[field]=h.fixed_list(value,72,limits.eth1_votes_limit,[&](View v){return h.eth1(v);});break;
   case 11:fields[field]=h.fixed_list(value,121,limits.validator_registry_limit,[&](View v){return h.validator(v);});break;
   case 12:case 21:require(value.size()%8==0&&value.size()/8<=limits.validator_registry_limit);fields[field]=h.mix(h.packed(value,(limits.validator_registry_limit-1)/4+1),value.size()/8);break;
   case 24:{
    if(capella){const auto header=decode_capella_execution_header(value,execution->extra_data_bytes,value.size(),checkpoint);require(header.has_value());fields[field]=capella_execution_header_root(*header,execution->extra_data_bytes,checkpoint);}
    else{const auto header=decode_bellatrix_execution_header(value,execution->extra_data_bytes,value.size(),checkpoint);require(header.has_value());fields[field]=bellatrix_execution_header_root(*header,execution->extra_data_bytes,checkpoint);}break;
   }
   case 27:fields[field]=h.fixed_list(value,64,limits.historical_roots_limit,[&](View v){return h.container({scalar(v.first(32)),scalar(v.subspan(32))});});break;
   default:
    if(sync_size){require(value.size()<=limits.validator_registry_limit);fields[field]=h.mix(h.packed(value,(limits.validator_registry_limit-1)/32+1),value.size());}
    else fields[field]=h.pending_list(value,limits.pending_attestations_limit,limits.validators_per_committee);
    break;
  }
 }
 const auto result=h.merkle(std::move(fields),field_count);h.tick();return result;
}
}
ConsensusRoot phase0_beacon_state_root(View b,const Phase0StateLimits& limits,const std::function<void()>& checkpoint){
 return beacon_state_root(b,limits,0,checkpoint);
}
ConsensusRoot altair_beacon_state_root(View b,const AltairStateLimits& l,const std::function<void()>& checkpoint){
 require(l.sync_committee_size);
 return beacon_state_root(b,{l.slots_per_historical_root,l.epochs_per_historical_vector,l.epochs_per_slashings_vector,l.historical_roots_limit,l.eth1_votes_limit,l.validator_registry_limit,1,1},l.sync_committee_size,checkpoint);
}
ConsensusRoot bellatrix_beacon_state_root(View b,const AltairStateLimits& l,const ExecutionPayloadLimits& e,size_t maximum_bytes,const std::function<void()>& checkpoint){
 require(l.sync_committee_size&&b.size()<=maximum_bytes);
 return beacon_state_root(b,{l.slots_per_historical_root,l.epochs_per_historical_vector,l.epochs_per_slashings_vector,l.historical_roots_limit,l.eth1_votes_limit,l.validator_registry_limit,1,1},l.sync_committee_size,checkpoint,&e);
}
ConsensusRoot capella_beacon_state_root(View b,const AltairStateLimits& l,const ExecutionPayloadLimits& e,size_t maximum_bytes,const std::function<void()>& checkpoint){
 require(l.sync_committee_size&&b.size()<=maximum_bytes);
 return beacon_state_root(b,{l.slots_per_historical_root,l.epochs_per_historical_vector,l.epochs_per_slashings_vector,l.historical_roots_limit,l.eth1_votes_limit,l.validator_registry_limit,1,1},l.sync_committee_size,checkpoint,&e,true);
}
}
