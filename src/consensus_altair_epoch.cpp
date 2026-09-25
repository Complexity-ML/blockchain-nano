#include "nano/consensus_altair_epoch.hpp"
#include <algorithm>
namespace nano {
Bytes prepare_altair_inactivity_updates(std::span<const uint8_t> b,const AltairStateLimits& l,
 const AltairInactivityRules& r,const std::function<void()>& checkpoint){
 if(!r.slots_per_epoch||!r.score_bias||!r.score_recovery_rate)throw std::invalid_argument("invalid Altair inactivity rules");
 altair_beacon_state_root(b,l,checkpoint);
 auto number=[&](size_t at,size_t width=8){uint64_t n=0;for(size_t i=0;i<width;++i)n|=uint64_t(b[at+i])<<(8*i);return n;};
 const auto epoch=number(40)/r.slots_per_epoch;
 if(!epoch)return Bytes(b.begin(),b.end());
 // These positions follow the validated Altair schema, parameterized by preset.
 const size_t historical=176+64*l.slots_per_historical_root;
 const size_t previous_at=historical+96+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector;
 const auto validators=number(historical+88,4),balances=number(historical+92,4);
 const auto previous=number(previous_at,4),current=number(previous_at+4,4),scores=number(previous_at+129,4);
 const auto count=(balances-validators)/121;
 if((previous-balances)/8!=count||current-previous!=count||scores-current!=count||(b.size()-scores)/8!=count)
  throw std::invalid_argument("incoherent Altair participation or score counts");
 const auto finalized=number(previous_at+89),prior=epoch-1;
 if(finalized>prior)throw std::invalid_argument("Altair finality from future");
 const bool leak=prior-finalized>r.minimum_epochs_to_inactivity_penalty;
 Bytes out(b.begin(),b.end());
 for(size_t i=0;i<count;++i){
  if(checkpoint)checkpoint();const size_t at=validators+121*i;
  const auto activation=number(at+97),exit=number(at+105),withdrawable=number(at+113);const bool slashed=b[at+88];
  const bool active=activation<=prior&&prior<exit;
  if(!active&&!(slashed&&epoch<withdrawable))continue;
  auto score=number(scores+8*i);
  // TIMELY_TARGET_FLAG_INDEX is the Altair spec constant 1 (not a network cap).
  if(active&&!slashed&&(b[previous+i]&2)){if(score)--score;}
  else{if(score>UINT64_MAX-r.score_bias)throw std::overflow_error("Altair inactivity score overflow");score+=r.score_bias;}
  if(!leak)score-=std::min(score,r.score_recovery_rate);
  for(size_t j=0;j<8;++j)out[scores+8*i+j]=uint8_t(score>>(8*j));
 }
 if(checkpoint)checkpoint();return out;
}
}
