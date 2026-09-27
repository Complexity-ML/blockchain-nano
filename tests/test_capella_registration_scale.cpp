#include "support/phase0_minimal_ssz.hpp"
#include "nano/consensus_capella_registration.hpp"
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
 const ExecutionPayloadLimits execution{32,1048576,1024};constexpr uint64_t maximum_balance=32000000000ULL;constexpr size_t budget=256*1024*1024;
 const size_t historical=176+64*limits.slots_per_historical_root,previous=historical+96+32*limits.epochs_per_historical_vector+8*limits.epochs_per_slashings_vector,header=previous+229+96*limits.sync_committee_size;
 const std::vector<size_t> offsets{historical,historical+76,historical+88,historical+92,previous,previous+4,previous+129,header,header+20};
 capella_beacon_state_root(original,limits,execution,budget);
 for(uint64_t count:{100000ULL,1000000ULL}){
  Bytes state(original.begin(),original.begin()+little(original,offsets.front(),4));
  for(size_t f=0;f<offsets.size();++f){put(state,offsets[f],state.size(),4);const size_t start=state.size();
   if(f==2){state.resize(start+121*count);for(uint64_t i=0;i<count;++i){const auto at=start+121*i;put(state,at,i);state.at(at+48)=1;put(state,at+72,i);put(state,at+80,maximum_balance);put(state,at+105,UINT64_MAX);put(state,at+113,UINT64_MAX);}}
   else if(f==3){state.resize(start+8*count);for(uint64_t i=0;i<count;++i)put(state,start+8*i,maximum_balance+1000);}
   else if(f==4||f==5||f==6)state.resize(start+count*(f==6?8:1));
   else{const auto a=little(original,offsets[f],4),b=f+1<offsets.size()?little(original,offsets[f+1],4):original.size();state.insert(state.end(),original.begin()+a,original.begin()+b);}
  }
  put(state,header+4,100);put(state,header+12,count-8);
  auto started=Clock::now();const auto root=capella_beacon_state_root(state,limits,execution,budget);const auto root_us=us(started);
  auto key_at=[&](uint64_t index){const auto at=little(state,historical+88,4)+121*index;return Bytes(state.begin()+at,state.begin()+at+48);};
  const std::set<Bytes> requested{key_at(0),key_at(count/2),key_at(count-1),Bytes(48,255)};
  const std::map<Bytes,uint64_t> expected{{key_at(0),0},{key_at(count/2),count/2},{key_at(count-1),count-1}};
  const auto before=sha256(state);started=Clock::now();const auto selected=capella_validator_indices(state,root,limits,execution,budget,requested);const auto selection_us=us(started);
  check(selected==expected&&sha256(state)==before,"registry lookup differs or mutates SSZ");
  auto bad=root;bad[0]^=1;bool refused=false;try{capella_validator_indices(state,bad,limits,execution,budget,requested);}catch(const std::invalid_argument&){refused=true;}check(refused,"wrong state root accepted");
  bool cancelled=false;try{capella_validator_indices(state,root,limits,execution,budget,requested,[]{throw ExecutionCancelled("registry lookup cancelled");});}catch(const ExecutionCancelled&){cancelled=true;}check(cancelled&&sha256(state)==before,"lookup cancellation failed");
  bool width=false;try{capella_validator_indices(state,root,limits,execution,budget,{Bytes(47)});}catch(const std::invalid_argument&){width=true;}check(width,"wrong public key width accepted");
  const auto begin=little(state,historical+88,4);const auto last=key_at(count-1),first=key_at(0);std::copy(first.begin(),first.end(),state.begin()+begin+121*(count-1));
  const auto duplicate_root=capella_beacon_state_root(state,limits,execution,budget);bool duplicate=false;
  try{capella_validator_indices(state,duplicate_root,limits,execution,budget,requested);}catch(const std::runtime_error& e){duplicate=std::string(e.what())=="Ambiguous validator public key";}check(duplicate,"duplicate public key was silently resolved");
  std::copy(last.begin(),last.end(),state.begin()+begin+121*(count-1));check(sha256(state)==before,"negative-control restoration changed source");
  if(count==100000){
   auto mismatch=state;const auto end_balances=little(mismatch,previous,4);
   mismatch.erase(mismatch.begin()+end_balances-8,mismatch.begin()+end_balances);
   for(size_t field=4;field<offsets.size();++field)put(mismatch,offsets[field],little(mismatch,offsets[field],4)-8,4);
   const auto malformed_root=capella_beacon_state_root(mismatch,limits,execution,budget);bool cardinality=false;
   try{capella_validator_indices(mismatch,malformed_root,limits,execution,budget,requested);}catch(const std::invalid_argument& e){cardinality=std::string(e.what())=="Capella registry cardinality mismatch";}check(cardinality,"root-valid incoherent list cardinality accepted");
  }
  struct rusage usage{};check(getrusage(RUSAGE_SELF,&usage)==0,"resource accounting");
#ifdef __APPLE__
  const uint64_t rss=usage.ru_maxrss;
#else
  const uint64_t rss=uint64_t(usage.ru_maxrss)*1024;
#endif
  std::cout<<Json{{"scenario","capella-registration-ssz-scale"},{"validators",count},{"inputRoot",hex(Bytes(root.begin(),root.end()))},{"stateBytes",state.size()},{"rootMicroseconds",root_us},{"lookupIncludingAuthenticationMicroseconds",selection_us},{"processPeakRssBytes",rss},{"requestedKeys",requested.size()},{"matchedKeys",selected.size()},{"syntheticRegistry",true},{"includesBls",false},{"distributedNodes",0},{"wrongRootRejected",true},{"duplicateRejected",true},{"cancelledWithoutMutation",true}}.dump()<<std::endl;
 }
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
