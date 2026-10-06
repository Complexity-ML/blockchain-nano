#include "nano/consensus_altair_epoch.hpp"
#include "nano/consensus_bls.hpp"
#include "nano/committee.hpp"
#include <algorithm>
namespace nano {
Bytes prepare_altair_sync_updates(std::span<const uint8_t> b,const AltairStateLimits& l,
 const AltairSyncRules& r,uint64_t maximum_trials,const std::function<void()>& checkpoint){
 if(!r.slots_per_epoch||!r.epochs_per_sync_committee_period||!r.maximum_effective_balance||!r.shuffle_rounds||r.shuffle_rounds>256||l.epochs_per_historical_vector<3||r.min_seed_lookahead>=l.epochs_per_historical_vector-2)
  throw std::invalid_argument("invalid Altair sync rules");
 altair_beacon_state_root(b,l,checkpoint);
 auto number=[&](size_t at,size_t width=8){uint64_t n=0;for(size_t j=0;j<width;++j)n|=uint64_t(b[at+j])<<(8*j);return n;};
 const auto current_epoch=number(40)/r.slots_per_epoch;if(current_epoch==UINT64_MAX)throw std::overflow_error("Altair next epoch overflow");const auto epoch=current_epoch+1;
 if(epoch%r.epochs_per_sync_committee_period)return Bytes(b.begin(),b.end());
 const size_t historical=176+64*l.slots_per_historical_root,randao=historical+96,previous_at=randao+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector;
 const auto validators=number(historical+88,4),balances=number(historical+92,4),count=(balances-validators)/121;
 std::vector<uint64_t> active;
 for(size_t i=0;i<count;++i){if(checkpoint)checkpoint();const auto at=validators+121*i;if(number(at+97)<=epoch&&epoch<number(at+105))active.push_back(i);}
 if(active.empty())throw std::invalid_argument("empty Altair sync active set");
 const auto position=epoch%l.epochs_per_historical_vector,back=r.min_seed_lookahead+1;
 const auto mix=position>=back?position-back:l.epochs_per_historical_vector-(back-position);
 Bytes seed_bytes{7,0,0,0};for(size_t j=0;j<8;++j)seed_bytes.push_back(uint8_t(epoch>>(8*j)));seed_bytes.insert(seed_bytes.end(),b.begin()+randao+32*mix,b.begin()+randao+32*mix+32);
 const auto digest=sha256(seed_bytes);ConsensusRoot seed{};std::copy(digest.begin(),digest.end(),seed.begin());
 std::vector<Bytes> keys;Bytes random;
 const Int maximum(std::to_string(r.maximum_effective_balance));
 for(uint64_t i=0;keys.size()<l.sync_committee_size;++i){
  if(checkpoint)checkpoint();if(i>=maximum_trials)throw std::length_error("Altair sync sampling work budget");
  const auto index=active[shuffled_validator_index(i%active.size(),active.size(),seed,r.shuffle_rounds)];
  if(i%32==0){Bytes input(seed.begin(),seed.end());for(size_t j=0;j<8;++j)input.push_back(uint8_t((i/32)>>(8*j)));random=sha256(input);}
  const auto at=validators+121*index;const Int effective(std::to_string(number(at+80)));
  if(effective*255>=maximum*random[i%32])keys.emplace_back(b.begin()+at,b.begin()+at+48);
 }
 const auto aggregate=aggregate_consensus_bls_keys(keys);if(!aggregate)throw std::invalid_argument("invalid sync committee BLS key");
 const size_t current=previous_at+133,width=48*(l.sync_committee_size+1),next=current+width;
 Bytes out(b.begin(),b.end());std::copy_n(b.begin()+next,width,out.begin()+current);size_t at=next;
 for(const auto& key:keys){if(checkpoint)checkpoint();std::copy(key.begin(),key.end(),out.begin()+at);at+=48;}
 std::copy(aggregate->begin(),aggregate->end(),out.begin()+at);if(checkpoint)checkpoint();return out;
}
}
