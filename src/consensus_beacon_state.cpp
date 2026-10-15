#include "nano/consensus_beacon_state.hpp"
#include "consensus_capella_slot_merkle.hpp"
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
 capella_detail::SlotMerkleCounter* work=nullptr;
 void tick()const{if(cancel)cancel();}
 Root pair(const Root& a,const Root& b)const{
  const auto root=consensus_signing_root(a,b);
  if(work)work->increment(work->value.slot_merkle_pairs);
  return root;
 }
 Root merkle(std::vector<Root> nodes,uint64_t limit)const{
  require(limit&&nodes.size()<=limit);Root zero{};if(nodes.empty())nodes.push_back(zero);
  const auto depth=std::bit_width(limit-1);
  for(unsigned level=0;level<depth;++level){
   if(nodes.size()%2)nodes.push_back(zero);
   for(size_t i=0;i<nodes.size()/2;++i){tick();nodes[i]=pair(nodes[2*i],nodes[2*i+1]);}
   nodes.resize(nodes.size()/2);zero=pair(zero,zero);
  }
  require(nodes.size()==1);return nodes[0];
 }
 Root container(std::initializer_list<Root> fields)const{return merkle(std::vector<Root>(fields),fields.size());}
 Root packed(View b,uint64_t chunks)const{
  std::vector<Root> nodes;nodes.reserve(b.size()/32+(b.size()%32!=0));
  for(size_t i=0;i<b.size();i+=32){tick();nodes.push_back(scalar(b.subspan(i,std::min(size_t(32),b.size()-i))));}
  return merkle(std::move(nodes),chunks);
 }
 Root mix(const Root& root,uint64_t count)const{return pair(root,length_root(count));}
 Root checkpoint(View b)const{require(b.size()==40);return container({scalar(b.first(8)),scalar(b.subspan(8))});}
 Root data(View b)const{require(b.size()==128);return container({scalar(b.first(8)),scalar(b.subspan(8,8)),scalar(b.subspan(16,32)),checkpoint(b.subspan(48,40)),checkpoint(b.subspan(88,40))});}
 Root header(View b)const{require(b.size()==112);return container({scalar(b.first(8)),scalar(b.subspan(8,8)),scalar(b.subspan(16,32)),scalar(b.subspan(48,32)),scalar(b.subspan(80,32))});}
 Root eth1(View b)const{require(b.size()==72);return container({scalar(b.first(32)),scalar(b.subspan(32,8)),scalar(b.subspan(40,32))});}
 Root validator(View b)const{
  require(b.size()==121&&b[88]<=1);
  // Validator has exactly eight fields: no padding subtrees or heap buffers.
  // Its 48-byte public key is two chunks, with the final 16 bytes zero-padded.
  tick();const auto key=pair(scalar(b.first(32)),scalar(b.subspan(32,16)));
  std::array<Root,8> nodes{key,scalar(b.subspan(48,32)),scalar(b.subspan(80,8)),scalar(b.subspan(88,1)),scalar(b.subspan(89,8)),scalar(b.subspan(97,8)),scalar(b.subspan(105,8)),scalar(b.subspan(113,8))};
  for(size_t width=nodes.size();width>1;width/=2)
   for(size_t i=0;i<width/2;++i){tick();nodes[i]=pair(nodes[2*i],nodes[2*i+1]);}
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
struct FieldCache {std::array<View,28> values{};std::array<Root,28> roots{};};
ConsensusRoot beacon_state_root(View b,const Phase0StateLimits& limits,uint64_t sync_size,const std::function<void()>& checkpoint,const ExecutionPayloadLimits* execution=nullptr,bool capella=false,FieldCache* capture=nullptr,const FieldCache* reuse=nullptr,std::array<Root,28>* all_fields=nullptr,capella_detail::SlotMerkleCounter* work=nullptr){
 require(b.size()<=UINT32_MAX&&limits.slots_per_historical_root&&limits.epochs_per_historical_vector&&limits.epochs_per_slashings_vector&&limits.historical_roots_limit&&limits.eth1_votes_limit&&limits.validator_registry_limit&&limits.pending_attestations_limit&&limits.validators_per_committee);
 Hasher h{checkpoint,work};h.tick();size_t cursor=0;
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
  if(reuse&&value.size()==reuse->values[field].size()&&std::equal(value.begin(),value.end(),reuse->values[field].begin())){fields[field]=reuse->roots[field];continue;}
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
 if(capture){for(size_t i=0;i<variable.size();++i){const auto [field,start]=variable[i];const auto end=i+1==variable.size()?b.size():variable[i+1].second;capture->values[field]=b.subspan(start,end-start);capture->roots[field]=fields[field];}}
 if(all_fields){require(capella&&field_count==all_fields->size());std::copy(fields.begin(),fields.end(),all_fields->begin());}
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
std::optional<CapellaSszTransform> transform_capella_beacon_state(View input,const ConsensusRoot& expected,const AltairStateLimits& l,const ExecutionPayloadLimits& e,size_t maximum_bytes,const std::function<std::optional<Bytes>(View)>& transform,const std::function<void()>& checkpoint){
 require(l.sync_committee_size&&input.size()<=maximum_bytes&&input.size()<=UINT32_MAX);
 if(checkpoint)checkpoint();const Bytes owned(input.begin(),input.end());
 const Phase0StateLimits limits{l.slots_per_historical_root,l.epochs_per_historical_vector,l.epochs_per_slashings_vector,l.historical_roots_limit,l.eth1_votes_limit,l.validator_registry_limit,1,1};
 FieldCache cache;
 if(beacon_state_root(owned,limits,l.sync_committee_size,checkpoint,&e,true,&cache)!=expected)throw std::invalid_argument("Capella withdrawal state authentication failed");
 auto post=transform(owned);if(!post)return std::nullopt;
 require(post->size()<=maximum_bytes);
 const auto root=beacon_state_root(*post,limits,l.sync_committee_size,checkpoint,&e,true,nullptr,&cache);
 if(checkpoint)checkpoint();return CapellaSszTransform{std::move(*post),root};
}

namespace capella_detail {
namespace {
Root batch_strict_root(View b,const AltairStateLimits& l,const ExecutionPayloadLimits& e,
 size_t maximum_bytes,const std::function<void()>& check,SlotMerkleCounter& work,
 std::array<Root,28>* fields=nullptr){
 require(l.sync_committee_size&&b.size()<=maximum_bytes&&b.size()<=UINT32_MAX);
 work.increment(work.value.full_state_roots_started);
 const auto root=beacon_state_root(b,{l.slots_per_historical_root,l.epochs_per_historical_vector,l.epochs_per_slashings_vector,l.historical_roots_limit,l.eth1_votes_limit,l.validator_registry_limit,1,1},l.sync_committee_size,check,&e,true,nullptr,nullptr,fields,&work);
 work.increment(work.value.full_state_roots_completed);return root;
}
void batch_put(Bytes& b,size_t at,View value){
 require(at<=b.size()&&value.size()<=b.size()-at);std::copy(value.begin(),value.end(),b.begin()+at);
}
}
ConsensusRoot capella_beacon_state_root_observed(View input,const AltairStateLimits& limits,
 const ExecutionPayloadLimits& execution,size_t maximum_bytes,
 const std::function<void()>& checkpoint,SlotMerkleWork* output){
 SlotMerkleCounter work(output);
 try{
  // Bound the allocation first; detach all values before any external callback.
  require(input.size()<=maximum_bytes&&input.size()<=UINT32_MAX);
  const auto l=limits;const auto e=execution;const auto check=checkpoint;
  const Bytes owned(input.begin(),input.end());
  return batch_strict_root(owned,l,e,maximum_bytes,check,work);
 }catch(...){work.failed();throw;}
}
OwnedCapellaSlotMerkle::OwnedCapellaSlotMerkle(CapellaSlotState state,
 AltairStateLimits limits,ExecutionPayloadLimits execution,size_t maximum_bytes,
 std::function<void()> check,SlotMerkleCounter& work)
 :state_(std::move(state)),limits_(limits),execution_(execution),
  maximum_bytes_(maximum_bytes),check_(std::move(check)),work_(work){rebuild();}
OwnedCapellaSlotMerkle::Ring OwnedCapellaSlotMerkle::ring(size_t base){
 const auto count=limits_.slots_per_historical_root;
 require(count&&count<=UINT32_MAX/32);
 Ring out;out.leaves=std::bit_ceil(size_t(count));
 require(out.leaves<=out.nodes.max_size()/2);
 const auto total=out.leaves*2;Hasher h{check_,&work_};h.tick();
 // Reservation/padding is bounded by the already validated ring byte length,
 // never registry/list maxima. Touch each node under the existing checkpoint.
 out.nodes.reserve(total);
 for(size_t i=0;i<total;++i){h.tick();out.nodes.emplace_back();}
 for(size_t i=0;i<size_t(count);++i){h.tick();out.nodes[out.leaves+i]=scalar(part(state_.ssz,base+32*i,32));}
 for(size_t i=out.leaves;i-- >1;){h.tick();out.nodes[i]=h.pair(out.nodes[2*i],out.nodes[2*i+1]);}
 return out;
}
void OwnedCapellaSlotMerkle::update(Ring& tree,size_t position,const Root& value){
 require(position<limits_.slots_per_historical_root&&tree.leaves+position<tree.nodes.size());
 auto i=tree.leaves+position;tree.nodes[i]=value;Hasher h{check_,&work_};
 while(i>1){i/=2;h.tick();tree.nodes[i]=h.pair(tree.nodes[2*i],tree.nodes[2*i+1]);}
}
Root OwnedCapellaSlotMerkle::compose(){
 Hasher h{check_,&work_};return h.merkle(std::vector<Root>(fields_.begin(),fields_.end()),fields_.size());
}
void OwnedCapellaSlotMerkle::rebuild(){
 root_=batch_strict_root(state_.ssz,limits_,execution_,maximum_bytes_,check_,work_,&fields_);
 // No views from the old epoch buffer survive this reconstruction.
 blocks_=Ring{};states_=Ring{};
 blocks_=ring(176);states_=ring(176+32*size_t(limits_.slots_per_historical_root));
 require(blocks_.nodes[1]==fields_[5]&&states_.nodes[1]==fields_[6]);
}
void OwnedCapellaSlotMerkle::process_slot_roots(uint64_t slot){
 require(little(part(state_.ssz,40,8))==slot);Hasher h{check_,&work_};h.tick();
 const auto before=root_;const auto pos=size_t(slot%limits_.slots_per_historical_root);
 batch_put(state_.ssz,176+32*size_t(limits_.slots_per_historical_root)+32*pos,before);
 update(states_,pos,before);fields_[6]=states_.nodes[1];
 if(scalar(part(state_.ssz,112,32))==Root{})batch_put(state_.ssz,112,before);
 // The same five-field header root as consensus_beacon_header_root; using this
 // Hasher also counts its actual pairs in both the strict and incremental path.
 fields_[4]=h.header(part(state_.ssz,64,112));
 batch_put(state_.ssz,176+32*pos,fields_[4]);update(blocks_,pos,fields_[4]);fields_[5]=blocks_.nodes[1];
 root_=compose();
}
void OwnedCapellaSlotMerkle::reset_epoch(CapellaSlotState post){
 require(post.ssz.size()<=maximum_bytes_&&post.ssz.size()<=UINT32_MAX);
 state_=std::move(post);rebuild();
}
void OwnedCapellaSlotMerkle::set_slot(uint64_t next){
 const auto value=length_root(next);batch_put(state_.ssz,40,View(value).first(8));
 fields_[2]=value;root_=compose();
}
CapellaSlotState OwnedCapellaSlotMerkle::finish(){
 const auto strict=batch_strict_root(state_.ssz,limits_,execution_,maximum_bytes_,check_,work_);
 require(strict==root_);if(check_)check_();return std::move(state_);
}
}

}
