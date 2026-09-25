#include "nano/consensus_beacon_body.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <vector>
namespace nano {
namespace {
using View=std::span<const uint8_t>;
using Root=ConsensusRoot;
void require(bool condition){if(!condition)throw std::invalid_argument("invalid Phase0 body SSZ");}
View part(View bytes,size_t start,size_t count){require(start<=bytes.size()&&count<=bytes.size()-start);return bytes.subspan(start,count);}
uint32_t offset(View bytes,size_t start){auto v=part(bytes,start,4);return uint32_t(v[0])|(uint32_t(v[1])<<8)|(uint32_t(v[2])<<16)|(uint32_t(v[3])<<24);}
Root scalar(View bytes){require(bytes.size()<=32);Root root{};std::copy(bytes.begin(),bytes.end(),root.begin());return root;}
Root length_root(uint64_t count){Root root{};for(unsigned i=0;i<8;++i)root[i]=static_cast<uint8_t>(count>>(i*8));return root;}
Root merkle(std::vector<Root> nodes,uint64_t limit){
 require(limit>0&&nodes.size()<=limit);
 // Sparse zero padding: never allocate the configured maximum list size.
 Root zero{};uint64_t capacity=1;
 if(nodes.empty())nodes.push_back(zero);
 while(nodes.size()>1){
  if(nodes.size()%2)nodes.push_back(zero);
  for(size_t i=0;i<nodes.size()/2;++i)nodes[i]=consensus_signing_root(nodes[2*i],nodes[2*i+1]);
  nodes.resize(nodes.size()/2);zero=consensus_signing_root(zero,zero);capacity*=2;
 }
 while(capacity<limit){nodes[0]=consensus_signing_root(nodes[0],zero);zero=consensus_signing_root(zero,zero);capacity*=2;}
 return nodes[0];
}
Root packed(View bytes,uint64_t chunk_limit){std::vector<Root> nodes;for(size_t i=0;i<bytes.size();i+=32)nodes.push_back(scalar(bytes.subspan(i,std::min(size_t(32),bytes.size()-i))));return merkle(std::move(nodes),chunk_limit);}
Root container(std::initializer_list<Root> fields){return merkle(std::vector<Root>(fields),fields.size());}
Root vector_bytes(View bytes,size_t width){require(bytes.size()==width);return packed(bytes,(width+31)/32);}
Root mix(const Root& root,uint64_t count){return consensus_signing_root(root,length_root(count));}
std::vector<View> variable_fields(View bytes,size_t fixed,std::initializer_list<size_t> locations){
 require(bytes.size()>=fixed);std::vector<size_t> offsets;for(auto location:locations)offsets.push_back(offset(bytes,location));
 require(!offsets.empty()&&offsets.front()==fixed);offsets.push_back(bytes.size());std::vector<View> fields;
 for(size_t i=0;i+1<offsets.size();++i){require(offsets[i]<=offsets[i+1]&&offsets[i+1]<=bytes.size());fields.push_back(bytes.subspan(offsets[i],offsets[i+1]-offsets[i]));}return fields;
}
template<class Hash> Root fixed_list(View bytes,size_t width,uint32_t limit,Hash hash){
 require(bytes.size()%width==0&&bytes.size()/width<=limit);std::vector<Root> roots;
 for(size_t i=0;i<bytes.size();i+=width)roots.push_back(hash(bytes.subspan(i,width)));
 return mix(merkle(std::move(roots),limit),bytes.size()/width);
}
template<class Hash> Root variable_list(View bytes,uint32_t limit,Hash hash){
 if(bytes.empty())return mix(merkle({},limit),0);
 const auto first=offset(bytes,0);require(first>=4&&first%4==0&&first<=bytes.size());const size_t count=first/4;require(count<=limit);
 std::vector<Root> roots;size_t start=first;
 for(size_t i=0;i<count;++i){const size_t end=i+1==count?bytes.size():offset(bytes,4*(i+1));require(start<=end&&end<=bytes.size());roots.push_back(hash(bytes.subspan(start,end-start)));start=end;}
 return mix(merkle(std::move(roots),limit),count);
}
Root checkpoint(View b){require(b.size()==40);return container({scalar(b.first(8)),scalar(b.subspan(8))});}
Root attestation_data(View b){require(b.size()==128);return container({scalar(b.first(8)),scalar(b.subspan(8,8)),scalar(b.subspan(16,32)),checkpoint(b.subspan(48,40)),checkpoint(b.subspan(88,40))});}
Root indexed_attestation(View b,uint32_t limit){
 auto fields=variable_fields(b,228,{0});const auto indices=fields[0];require(indices.size()%8==0&&indices.size()/8<=limit);
 return container({mix(packed(indices,(uint64_t(limit)+3)/4),indices.size()/8),attestation_data(b.subspan(4,128)),vector_bytes(b.subspan(132,96),96)});
}
Root attester_slashing(View b,uint32_t limit){auto fields=variable_fields(b,8,{0,4});return container({indexed_attestation(fields[0],limit),indexed_attestation(fields[1],limit)});}
Root bitlist(View b,uint32_t limit){
 require(!b.empty()&&b.size()<=uint64_t(limit)/8+1&&b.back()!=0);
 const auto last=unsigned(std::bit_width(unsigned(b.back()))-1);const uint64_t count=(uint64_t(b.size())-1)*8+last;require(count<=limit);
 std::vector<uint8_t> data(b.begin(),b.end());data.back()^=uint8_t(1U<<last);if(last==0)data.pop_back();
 return mix(packed(data,(uint64_t(limit)+255)/256),count);
}
Root attestation(View b,uint32_t limit){auto fields=variable_fields(b,228,{0});return container({bitlist(fields[0],limit),attestation_data(b.subspan(4,128)),vector_bytes(b.subspan(132,96),96)});}
Root header(View b){require(b.size()==112);return container({scalar(b.first(8)),scalar(b.subspan(8,8)),scalar(b.subspan(16,32)),scalar(b.subspan(48,32)),scalar(b.subspan(80,32))});}
Root signed_header(View b){require(b.size()==208);return container({header(b.first(112)),vector_bytes(b.subspan(112),96)});}
Root proposer_slashing(View b){require(b.size()==416);return container({signed_header(b.first(208)),signed_header(b.subspan(208))});}
Root deposit(View b){
 // Phase0 DEPOSIT_CONTRACT_TREE_DEPTH=32, proof vector has depth+1 roots.
 require(b.size()==1240);const auto data=b.subspan(1056);
 return container({vector_bytes(b.first(1056),1056),container({vector_bytes(data.first(48),48),scalar(data.subspan(48,32)),scalar(data.subspan(80,8)),vector_bytes(data.subspan(88),96)})});
}
Root exit(View b){require(b.size()==112);return container({container({scalar(b.first(8)),scalar(b.subspan(8,8))}),vector_bytes(b.subspan(16),96)});}
}
ConsensusRoot phase0_beacon_body_root(View b,const Phase0BodyLimits& limits){
 require(limits.proposer_slashings&&limits.attester_slashings&&limits.attestations&&limits.deposits&&limits.voluntary_exits&&limits.validators_per_committee);
 require(b.size()<=UINT32_MAX);auto fields=variable_fields(b,220,{200,204,208,212,216});
 return container({vector_bytes(b.first(96),96),container({scalar(b.subspan(96,32)),scalar(b.subspan(128,8)),scalar(b.subspan(136,32))}),scalar(b.subspan(168,32)),
  fixed_list(fields[0],416,limits.proposer_slashings,proposer_slashing),
  variable_list(fields[1],limits.attester_slashings,[&](View v){return attester_slashing(v,limits.validators_per_committee);}),
  variable_list(fields[2],limits.attestations,[&](View v){return attestation(v,limits.validators_per_committee);}),
  fixed_list(fields[3],1240,limits.deposits,deposit),fixed_list(fields[4],112,limits.voluntary_exits,exit)});
}
namespace {
ConsensusRoot sync_beacon_body_root(View b,const Phase0BodyLimits& limits,uint64_t sync_size,const ExecutionPayloadLimits* execution,const std::function<void()>& check){
 if(check)check();
 require(limits.proposer_slashings&&limits.attester_slashings&&limits.attestations&&limits.deposits&&limits.voluntary_exits&&limits.validators_per_committee);
 require(sync_size&&sync_size<=UINT32_MAX&&b.size()<=UINT32_MAX);
 const size_t width=(sync_size+7)/8,fixed=316+width+(execution?4:0);
 auto fields=execution?variable_fields(b,fixed,{200,204,208,212,216,fixed-4}):variable_fields(b,fixed,{200,204,208,212,216});
 const auto bits=b.subspan(220,width);require(sync_size%8==0||(bits.back()>>(sync_size%8))==0);
 const auto sync=container({packed(bits,(sync_size+255)/256),vector_bytes(b.subspan(220+width,96),96)});
 std::vector<Root> roots{vector_bytes(b.first(96),96),container({scalar(b.subspan(96,32)),scalar(b.subspan(128,8)),scalar(b.subspan(136,32))}),scalar(b.subspan(168,32)),
  fixed_list(fields[0],416,limits.proposer_slashings,[&](View v){if(check)check();return proposer_slashing(v);}),
  variable_list(fields[1],limits.attester_slashings,[&](View v){if(check)check();return attester_slashing(v,limits.validators_per_committee);}),
  variable_list(fields[2],limits.attestations,[&](View v){if(check)check();return attestation(v,limits.validators_per_committee);}),
  fixed_list(fields[3],1240,limits.deposits,[&](View v){if(check)check();return deposit(v);}),fixed_list(fields[4],112,limits.voluntary_exits,[&](View v){if(check)check();return exit(v);}),sync};
 if(execution){auto payload=decode_bellatrix_execution_payload(fields[5],*execution,fields[5].size(),check);require(payload.has_value());roots.push_back(bellatrix_execution_payload_root(*payload,*execution,check));}
 const auto root=merkle(std::move(roots),execution?10:9);if(check)check();return root;
}
}
ConsensusRoot altair_beacon_body_root(View b,const Phase0BodyLimits& l,uint64_t sync){return sync_beacon_body_root(b,l,sync,nullptr,{});}
ConsensusRoot bellatrix_beacon_body_root(View b,const Phase0BodyLimits& l,uint64_t sync,const ExecutionPayloadLimits& e,size_t maximum_bytes,const std::function<void()>& check){
 require(b.size()<=maximum_bytes);return sync_beacon_body_root(b,l,sync,&e,check);
}
}
