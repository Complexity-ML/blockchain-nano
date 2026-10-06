#include "nano/consensus_effective_balance.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace nano {
ConsensusSlashingState prepare_phase0_effective_balances(
 const ConsensusSlashingState& state,const Phase0EffectiveBalanceRules& rules,
 const std::function<void()>& checkpoint){
 if(!rules.increment||!rules.maximum||rules.maximum%rules.increment||
    !rules.hysteresis_quotient||rules.hysteresis_quotient>rules.increment)
  throw std::invalid_argument("invalid effective balance rules");
 const auto step=rules.increment/rules.hysteresis_quotient;
 auto threshold=[&](uint64_t multiplier){
  if(multiplier>std::numeric_limits<uint64_t>::max()/step)
   throw std::overflow_error("effective balance threshold overflow");
  return step*multiplier;
 };
 const auto down=threshold(rules.downward_multiplier),up=threshold(rules.upward_multiplier);
 if(checkpoint)checkpoint();
 auto candidate=state;
 uint64_t expected=0;
 for(auto& [index,v]:candidate.validators){
  if(checkpoint)checkpoint();
  if(index!=expected++||v.effective_balance>rules.maximum||v.effective_balance%rules.increment)
   throw std::invalid_argument("invalid effective balance registry");
  // Subtractions preserve the strict mathematical comparisons even when
  // balance + threshold would overflow uint64_t.
  if((v.balance<v.effective_balance&&v.effective_balance-v.balance>down)||
     (v.balance>v.effective_balance&&v.balance-v.effective_balance>up))
   v.effective_balance=std::min(v.balance-v.balance%rules.increment,rules.maximum);
 }
 if(checkpoint)checkpoint();
 return candidate;
}
}
