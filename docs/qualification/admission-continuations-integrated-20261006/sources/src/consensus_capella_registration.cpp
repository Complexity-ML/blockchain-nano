#include "nano/consensus_capella_registration.hpp"
namespace nano {
std::map<Bytes,uint64_t> capella_validator_indices(std::span<const uint8_t> state,
 const ConsensusRoot& authenticated_root,const AltairStateLimits& l,const ExecutionPayloadLimits& execution,
 size_t budget,const std::set<Bytes>& keys,const std::function<void()>& check){
 std::map<std::array<uint8_t,48>,const Bytes*> requested;
 for(const auto& key:keys){if(key.size()!=48)throw std::invalid_argument("Validator public key width");std::array<uint8_t,48> bytes;std::copy(key.begin(),key.end(),bytes.begin());requested.emplace(bytes,&key);}
 if(capella_beacon_state_root(state,l,execution,budget,check)!=authenticated_root)throw std::invalid_argument("Validator registry authentication failed");
 // Root validation above has checked all offsets, list widths and bounds.
 auto offset=[&](size_t at){uint32_t value=0;for(unsigned i=0;i<4;++i)value|=uint32_t(state[at+i])<<(8*i);return size_t(value);};
 const size_t historical=176+64*l.slots_per_historical_root;
 const size_t previous=historical+96+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector;
 const auto begin=offset(historical+88),balances=offset(historical+92),prior=offset(previous),current=offset(previous+4),scores=offset(previous+129),header=offset(previous+229+96*l.sync_committee_size);
 const size_t count=(balances-begin)/121;
 if((prior-balances)/8!=count||current-prior!=count||scores-current!=count||(header-scores)/8!=count)throw std::invalid_argument("Capella registry cardinality mismatch");
 std::map<Bytes,uint64_t> result;
 for(size_t index=0;index<count;++index){
  if(check)check();std::array<uint8_t,48> key;std::copy_n(state.begin()+begin+121*index,48,key.begin());
  const auto found=requested.find(key);
  if(found!=requested.end()&&!result.emplace(*found->second,index).second)throw std::runtime_error("Ambiguous validator public key");
 }
 return result;
}
}
