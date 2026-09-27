#include "nano/consensus_capella_withdrawals.hpp"
#include <chrono>
#include <iostream>
#include <sys/resource.h>
using namespace nano;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
using Clock=std::chrono::steady_clock;
uint64_t micros(Clock::time_point start){return std::chrono::duration_cast<std::chrono::microseconds>(Clock::now()-start).count();}
}
int main(){try{
 const CapellaWithdrawalRules rules{32000000000ULL,16,16384};
 for(const uint64_t count:{100000ULL,1000000ULL}){
  CapellaWithdrawalState state;state.registry.epoch=10;state.next_withdrawal_index=100;state.next_validator_index=count-8;
  for(uint64_t i=0;i<count;++i){ConsensusSlashingValidator v;v.balance=rules.maximum_effective_balance+1000;v.effective_balance=rules.maximum_effective_balance;
   for(unsigned j=0;j<8;++j){v.public_key[j]=uint8_t(i>>(j*8));v.withdrawal_credentials[24+j]=uint8_t(i>>(j*8));}
   state.registry.validators.emplace_hint(state.registry.validators.end(),i,v);
  }
  for(unsigned mode=0;mode<3;++mode){
   for(auto& [i,v]:state.registry.validators)v.withdrawal_credentials[0]=(mode==0||(mode==1&&i==0))?1:0;
   std::vector<CapellaWithdrawal> oracle;const uint64_t expected_count=mode==0?16:mode==1?1:0;
   for(uint64_t j=0;j<expected_count;++j){const auto index=mode==0?(count-8+j)%count:0;CapellaWithdrawal w{100+j,index,{},1000};for(unsigned k=0;k<8;++k)w.address[12+k]=uint8_t(index>>(8*k));oracle.push_back(w);}
   uint64_t checkpoints=0;auto start=Clock::now();const auto selected=expected_capella_withdrawals(state,rules,[&]{++checkpoints;});const auto selection_us=micros(start);
   check(selected==oracle,"large withdrawal selection/address/order mismatch");check(checkpoints==(mode==0?17:16385),"selection did not obey bounded sweep");
   start=Clock::now();const auto candidate=prepare_capella_withdrawals(state,rules,oracle);const auto preparation_us=micros(start);check(bool(candidate),"large withdrawal candidate rejected");
   check(candidate->next_withdrawal_index==100+expected_count,"withdrawal index mismatch");check(candidate->next_validator_index==(mode==0?8:16376),"wraparound cursor mismatch");
   uint64_t debited=0;
   for(const auto& [i,v]:state.registry.validators){check(v.balance==rules.maximum_effective_balance+1000,"source balance mutated");auto expected=v;const bool paid=mode==0?(i>=count-8||i<8):mode==1&&i==0;if(paid){expected.balance-=1000;debited+=1000;}check(candidate->registry.validators.at(i)==expected,"unexpected validator mutation");}
   check(debited==expected_count*1000&&state.next_withdrawal_index==100&&state.next_validator_index==count-8,"debit conservation/source cursor");
   auto altered=oracle;if(altered.empty())altered.push_back({100,0,{},1000});else ++altered[0].amount_gwei;
   check(!prepare_capella_withdrawals(state,rules,altered),"altered large withdrawal list accepted");
   struct rusage usage{};check(getrusage(RUSAGE_SELF,&usage)==0,"resource accounting");
#ifdef __APPLE__
   const uint64_t rss=usage.ru_maxrss;
#else
   const uint64_t rss=uint64_t(usage.ru_maxrss)*1024;
#endif
   std::cout<<Json{{"scenario","capella-withdrawal-scale"},{"validators",count},{"distribution",mode==0?"dense":mode==1?"sparse":"empty"},{"withdrawals",expected_count},{"selectionMicroseconds",selection_us},{"candidateMicroseconds",preparation_us},{"selectionCheckpoints",checkpoints},{"processPeakRssBytes",rss},{"syntheticRegistry",true},{"distributedNodes",0},{"includesSszAuthentication",false},{"includesBls",false}}.dump()<<std::endl;
  }
 }
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
