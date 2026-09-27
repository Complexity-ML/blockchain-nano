#include "support/phase0_minimal_ssz.hpp"
#include "nano/consensus_capella_withdrawals.hpp"
#include <chrono>
#include <sys/resource.h>
using namespace test;using namespace test::phase0_minimal;
namespace {
void put(Bytes& b,size_t at,uint64_t value,size_t width=8){for(size_t i=0;i<width;++i)b.at(at+i)=uint8_t(value>>(8*i));}
using Clock=std::chrono::steady_clock;
uint64_t us(Clock::time_point t){return std::chrono::duration_cast<std::chrono::microseconds>(Clock::now()-t).count();}
}
int main(){try{
 const auto fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures",folder=fixtures/"consensus-capella-withdrawals-official";
 const auto manifest=load(folder/"manifest.json");check(manifest.at("commit")=="194a91fd9e6f97118156421aac4c80e6e47a9a7f","fixture pin");
 const auto original=binary(folder/"all_withdrawal/pre.ssz");bool verified=false;
 for(const auto& row:manifest.at("cases"))if(row.at("name")=="all_withdrawal"){const auto info=row.at("files").at("pre.ssz");check(original.size()==info.at("bytes")&&hex(sha256(original)).substr(2)==info.at("sha256").get<std::string>(),"fixture integrity");verified=true;}check(verified,"missing seed fixture");
 const auto phase0=read(fixtures/"consensus-bellatrix-containers-official/minimal-phase0.yaml"),altair=read(fixtures/"consensus-bellatrix-containers-official/minimal-altair.yaml");auto p=[&](const char* name){return parameter(phase0,name);};
 const AltairStateLimits limits{p("SLOTS_PER_HISTORICAL_ROOT"),p("EPOCHS_PER_HISTORICAL_VECTOR"),p("EPOCHS_PER_SLASHINGS_VECTOR"),p("HISTORICAL_ROOTS_LIMIT"),p("EPOCHS_PER_ETH1_VOTING_PERIOD")*p("SLOTS_PER_EPOCH"),p("VALIDATOR_REGISTRY_LIMIT"),parameter(altair,"SYNC_COMMITTEE_SIZE")};
 const ExecutionPayloadLimits execution{32,1048576,1024};const CapellaWithdrawalRules rules{32000000000ULL,16,16384};constexpr size_t budget=256*1024*1024;
 const size_t historical=176+64*limits.slots_per_historical_root,previous=historical+96+32*limits.epochs_per_historical_vector+8*limits.epochs_per_slashings_vector,header=previous+229+96*limits.sync_committee_size;
 const std::vector<size_t> offsets{historical,historical+76,historical+88,historical+92,previous,previous+4,previous+129,header,header+20};
 capella_beacon_state_root(original,limits,execution,budget);
 for(uint64_t count:{100000ULL,1000000ULL}){
  Bytes state(original.begin(),original.begin()+little(original,offsets.front(),4));
  for(size_t f=0;f<offsets.size();++f){put(state,offsets[f],state.size(),4);const size_t start=state.size();
   if(f==2){state.resize(start+121*count);for(uint64_t i=0;i<count;++i){const auto at=start+121*i;put(state,at,i);state.at(at+48)=1;put(state,at+72,i);put(state,at+80,rules.maximum_effective_balance);put(state,at+105,UINT64_MAX);put(state,at+113,UINT64_MAX);}}
   else if(f==3){state.resize(start+8*count);for(uint64_t i=0;i<count;++i)put(state,start+8*i,rules.maximum_effective_balance+1000);}
   else if(f==4||f==5||f==6)state.resize(start+count*(f==6?8:1));
   else{const auto a=little(original,offsets[f],4),b=f+1<offsets.size()?little(original,offsets[f+1],4):original.size();state.insert(state.end(),original.begin()+a,original.begin()+b);}
  }
  put(state,header+4,100);put(state,header+12,count-8);
  auto started=Clock::now();const auto root=capella_beacon_state_root(state,limits,execution,budget);const auto root_us=us(started);
  std::vector<CapellaWithdrawal> advertised;for(uint64_t j=0;j<16;++j){const auto i=(count-8+j)%count;CapellaWithdrawal w{100+j,i,{},1000};for(unsigned k=0;k<8;++k)w.address[12+k]=uint8_t(i>>(8*k));advertised.push_back(w);}
  const auto before=sha256(state);started=Clock::now();const auto result=prepare_capella_withdrawal_state(state,root,limits,execution,budget,p("SLOTS_PER_EPOCH"),rules,advertised);const auto preparation_us=us(started);check(bool(result),"scaled SSZ preparation rejected");
  auto expected=state;const auto balances=little(state,historical+92,4);for(const auto& w:advertised)put(expected,balances+8*w.validator_index,rules.maximum_effective_balance);put(expected,header+4,116);put(expected,header+12,8);
  check(result->state==expected&&sha256(state)==before,"unexpected SSZ mutation or source write");check(capella_beacon_state_root(expected,limits,execution,budget)==result->root,"post-root mismatch");
  auto bad=root;bad[0]^=1;bool refused=false;try{read_capella_withdrawal_state(state,bad,limits,execution,budget,p("SLOTS_PER_EPOCH"));}catch(const std::invalid_argument&){refused=true;}check(refused,"unauthenticated large state accepted");
  struct rusage usage{};check(getrusage(RUSAGE_SELF,&usage)==0,"resource accounting");
#ifdef __APPLE__
  const uint64_t rss=usage.ru_maxrss;
#else
  const uint64_t rss=uint64_t(usage.ru_maxrss)*1024;
#endif
  std::cout<<Json{{"scenario","capella-withdrawal-ssz-scale"},{"validators",count},{"inputRoot",hex(Bytes(root.begin(),root.end()))},{"outputRoot",hex(Bytes(result->root.begin(),result->root.end()))},{"stateBytes",state.size()},{"rootMicroseconds",root_us},{"prepareMicroseconds",preparation_us},{"processPeakRssBytes",rss},{"syntheticRegistry",true},{"includesSszAuthentication",true},{"includesBls",false},{"distributedNodes",0},{"fullStateCompared",true},{"wrongRootRejected",true}}.dump()<<std::endl;
 }
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
