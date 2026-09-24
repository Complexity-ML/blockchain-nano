#include "support/process.hpp"
#include "nano/consensus_registry_epoch.hpp"
#include <chrono>
#include <sys/resource.h>
using namespace test;
int main(){try{
 const Phase0RegistryUpdateRules rules{2,32,4,256,32000000000ULL,16000000000ULL};
 for(uint64_t count:{100000ULL,1000000ULL}){
  ConsensusSlashingState state;state.epoch=4;state.slashings.resize(64);
  // Synthetic unique keys, already-authenticated registry model only; this is
  // not a BLS/signature benchmark, network, or distributed-validator test.
  for(uint64_t i=0;i<count;++i){ConsensusSlashingValidator v;for(unsigned j=0;j<8;++j)v.public_key[j]=uint8_t(i>>(j*8));v.balance=v.effective_balance=rules.maximum_effective_balance;
   v.activation_eligibility_epoch=1;if(i<count/2){v.activation_epoch=0;if(i%16==0)v.effective_balance=rules.ejection_balance;}state.validators.emplace_hint(state.validators.end(),i,v);}
  const auto start=std::chrono::steady_clock::now();auto result=prepare_phase0_registry_updates(state,rules,3);const auto elapsed=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start).count();
  const auto churn=std::max(rules.minimum_churn_limit,(count/2)/rules.churn_limit_quotient);
  check(result.activated.size()==churn&&result.ejected.size()==(count/2+15)/16&&result.newly_eligible.empty(),"scaled counts");
  uint64_t ejections=0;
  for(const auto& [index,v]:result.registry_after.validators){const auto& original=state.validators.at(index);check(original.exit_epoch==UINT64_MAX&&original.activation_epoch==(index<count/2?0:UINT64_MAX),"input mutated");
   if(index<count/2&&index%16==0){check(v.exit_epoch==9+ejections/churn&&v.withdrawable_epoch==265+ejections/churn,"scaled ejection churn");check(result.ejected.at(ejections)==index,"ejection ordering");++ejections;}
   else check(v.exit_epoch==UINT64_MAX,"unrequested exit");
   const auto activation=index<count/2?0:(index<count/2+churn?9:UINT64_MAX);check(v.activation_epoch==activation,"scaled activation ordering/churn");check(v.balance==original.balance&&v.effective_balance==original.effective_balance,"registry update changed funds");
  }
  for(uint64_t i=0;i<churn;++i)check(result.activated.at(i)==count/2+i,"activation tie-break");
  struct rusage usage{};check(getrusage(RUSAGE_SELF,&usage)==0,"resource accounting");
#ifdef __APPLE__
  const uint64_t rss=usage.ru_maxrss;
#else
  const uint64_t rss=uint64_t(usage.ru_maxrss)*1024;
#endif
  std::cout<<Json{{"scenario","phase0-registry-epoch-scale"},{"validators",count},{"activated",result.activated.size()},{"ejected",result.ejected.size()},{"transitionMicroseconds",elapsed},{"processPeakRssBytes",rss},{"syntheticRegistry",true},{"distributedNodes",0},{"activeInConsensus",false}}.dump()<<std::endl;
 }
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
