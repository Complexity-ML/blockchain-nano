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

namespace nano {
Bytes prepare_altair_rewards(std::span<const uint8_t> b,const AltairStateLimits& l,
 const AltairRewardRules& r,const std::function<void()>& checkpoint){
 if(!r.slots_per_epoch||!r.effective_balance_increment||!r.base_reward_factor||!r.score_bias||!r.inactivity_penalty_quotient||!r.weight_denominator)
  throw std::invalid_argument("invalid Altair reward rules");
 for(auto w:r.participation_weights)if(w>r.weight_denominator)throw std::invalid_argument("invalid Altair participation weight");
 altair_beacon_state_root(b,l,checkpoint);
 auto number=[&](size_t at,size_t width=8){uint64_t n=0;for(size_t j=0;j<width;++j)n|=uint64_t(b[at+j])<<(8*j);return n;};
 const auto epoch=number(40)/r.slots_per_epoch;if(!epoch)return Bytes(b.begin(),b.end());
 const size_t historical=176+64*l.slots_per_historical_root,previous_at=historical+96+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector;
 const auto validators=number(historical+88,4),balances=number(historical+92,4),previous=number(previous_at,4),current=number(previous_at+4,4),scores=number(previous_at+129,4);
 const auto count=(balances-validators)/121;
 if((previous-balances)/8!=count||current-previous!=count||scores-current!=count||(b.size()-scores)/8!=count)throw std::invalid_argument("incoherent Altair reward lengths");
 const auto prior=epoch-1,finalized=number(previous_at+89);if(finalized>prior)throw std::invalid_argument("Altair reward finality from future");
 const bool leak=prior-finalized>r.minimum_epochs_to_inactivity_penalty;
 auto amount=[](uint64_t n){return Int(std::to_string(n));};const auto increment=amount(r.effective_balance_increment);
 std::vector<bool> eligible(count);std::array<std::vector<bool>,3> participating{std::vector<bool>(count),std::vector<bool>(count),std::vector<bool>(count)};
 std::vector<Int> effective(count),base(count);std::array<Int,3> participating_balance{};Int total=0;
 for(size_t i=0;i<count;++i){
  if(checkpoint)checkpoint();const size_t at=validators+121*i;const auto activation=number(at+97),exit=number(at+105),withdrawable=number(at+113);const bool slashed=b[at+88],active_prior=activation<=prior&&prior<exit;
  effective[i]=amount(number(at+80));if(activation<=epoch&&epoch<exit)total+=effective[i];
  eligible[i]=active_prior||(slashed&&epoch<withdrawable);
  for(unsigned f=0;f<3;++f)if(active_prior&&!slashed&&(b[previous+i]&(1U<<f))){participating[f][i]=true;participating_balance[f]+=effective[i];}
 }
 total=std::max(total,increment);Int square_root;mpz_sqrt(square_root.get_mpz_t(),total.get_mpz_t());
 const Int base_per_increment=increment*amount(r.base_reward_factor)/square_root,active_increments=total/increment;
 for(size_t i=0;i<count;++i)base[i]=(effective[i]/increment)*base_per_increment;
 Bytes out(b.begin(),b.end());std::vector<uint64_t> live(count);for(size_t i=0;i<count;++i)live[i]=number(balances+8*i);
 auto checked=[&](const Int& n){if(n<0||n>amount(UINT64_MAX))throw std::overflow_error("Altair reward uint64 overflow");return u64(n);};
 auto apply=[&](size_t i,const Int& reward,const Int& penalty){const auto debit=checked(penalty);live[i]=checked(amount(live[i])+reward);live[i]-=std::min(live[i],debit);};
 for(unsigned f=0;f<3;++f){
  const Int participating_increments=std::max(participating_balance[f],increment)/increment;
  for(size_t i=0;i<count;++i){if(checkpoint)checkpoint();if(!eligible[i])continue;Int reward=0,penalty=0;
   if(participating[f][i]){if(!leak)reward=base[i]*amount(r.participation_weights[f])*participating_increments/(active_increments*amount(r.weight_denominator));}
   else if(f!=2)penalty=base[i]*amount(r.participation_weights[f])/amount(r.weight_denominator);
   apply(i,reward,penalty);
  }
 }
 for(size_t i=0;i<count;++i){if(checkpoint)checkpoint();if(eligible[i]&&!participating[1][i])apply(i,0,effective[i]*amount(number(scores+8*i))/(amount(r.score_bias)*amount(r.inactivity_penalty_quotient)));for(size_t j=0;j<8;++j)out[balances+8*i+j]=uint8_t(live[i]>>(8*j));}
 if(checkpoint)checkpoint();return out;
}
}
