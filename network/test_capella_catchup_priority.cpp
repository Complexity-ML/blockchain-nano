#include "capella_relay_schedule.hpp"
#include <array>
#include <iostream>
#include <map>
#include <string>
#include <stdexcept>
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
unsigned next(CapellaRelaySchedule& q,bool preparing){
#ifdef NANO_TEST_PREPARATION_PRIORITY
 return q.next(preparing);
#else
 (void)preparing;return q.next();
#endif
}
int main(){try{
 CapellaRelaySchedule steady;std::array<unsigned,CapellaRelaySchedule::category_count> count{};unsigned blocks=0;
 for(unsigned i=0;i<2400;++i){auto kind=next(steady,true);check(kind<count.size(),"invalid category");++count[kind];if(kind==0){check(steady.next_block_history_first()==bool(blocks%2),"catch-up boost starved historical blocks");++blocks;}}
 check(count[0]==1300&&count[1]==600,"preparation did not add bounded block service");for(unsigned i:{2,3,4,5,6})check(count[i]==100,"mandatory background service lost");
 // Explore both choices at every turn without enumerating 2^24 identical
 // histories. Samples are merged only when every public continuation has
 // the same schedule and the same already-seen categories. Include the 24
 // alternating insertion phases as roots; no implementation fields are read.
 auto signature=[](CapellaRelaySchedule q){std::string s;auto ordinary=q;for(unsigned i=0;i<CapellaRelaySchedule::catchup_opportunities;++i)s.push_back(char('0'+next(ordinary,false)));for(unsigned i=0;i<CapellaRelaySchedule::catchup_opportunities;++i)s.push_back(char('0'+next(q,true)));return s;};
 struct Sample {CapellaRelaySchedule q;unsigned seen;};
 std::map<std::string,Sample> frontier;
 CapellaRelaySchedule seed;
 for(unsigned phase=0;phase<CapellaRelaySchedule::catchup_opportunities;++phase){frontier.emplace(signature(seed)+char(0),Sample{seed,0});next(seed,true);}
 unsigned windows=0;
 for(unsigned turn=0;turn<CapellaRelaySchedule::catchup_opportunities;++turn){
  std::map<std::string,Sample> successors;
  for(const auto& [_,sample]:frontier)for(bool preparing:{false,true}){auto q=sample.q;const auto kind=next(q,preparing);check(kind<CapellaRelaySchedule::category_count,"unknown adversarial category");const auto seen=sample.seen|(1u<<kind);successors.emplace(signature(q)+char(seen),Sample{q,seen});++windows;}
  frontier=std::move(successors);
 }
 for(const auto& [_,sample]:frontier)check(sample.seen==(1u<<CapellaRelaySchedule::category_count)-1,"adversarial mode changes starved a category");
 CapellaRelaySchedule q,plain;for(unsigned i=0;i<1000;++i)check(next(q,false)==plain.next(),"non-catch-up schedule changed");
 CapellaRelaySchedule a,b;for(unsigned i=0;i<100;++i)next(a,true);check(next(b,false)==0&&next(b,false)==1,"another peer changed mandatory schedule");
 std::cout<<"catchupBlocks="<<count[0]<<" attestations="<<count[1]<<" modeWindows="<<windows<<" normalScheduleUnchanged=true; opportunity model only, not WAN qualification\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
