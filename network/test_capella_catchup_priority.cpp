#include "capella_relay_schedule.hpp"
#include <array>
#include <iostream>
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
 CapellaRelaySchedule steady;std::array<unsigned,6> count{};unsigned blocks=0;
 for(unsigned i=0;i<2000;++i){auto kind=next(steady,true);check(kind<6,"invalid category");++count[kind];if(kind==0){check(steady.next_block_history_first()==bool(blocks%2),"catch-up boost starved historical blocks");++blocks;}}
 check(count[0]==1100&&count[1]==500,"preparation did not add bounded block service");for(unsigned i:{2,3,4,5})check(count[i]==100,"mandatory background service lost");
 // Exhaustively test every twenty-opportunity mode pattern at every reachable
 // normal-cycle/insertion phase. Every pair must contain a mandatory normal
 // turn; ten mandatory turns must cover all six categories.
 unsigned windows=0;
 for(unsigned phase=0;phase<20;++phase){CapellaRelaySchedule seed;for(unsigned i=0;i<phase;++i)next(seed,true);
  for(unsigned mask=0;mask<(1u<<20);++mask){auto q=seed;std::array<bool,6> seen{};for(unsigned i=0;i<20;++i)seen[next(q,(mask>>i)&1)]=true;for(bool s:seen)check(s,"adversarial mode changes starved a category");++windows;}
 }
 CapellaRelaySchedule q,plain;for(unsigned i=0;i<1000;++i)check(next(q,false)==plain.next(),"non-catch-up schedule changed");
 CapellaRelaySchedule a,b;for(unsigned i=0;i<100;++i)next(a,true);check(next(b,false)==0&&next(b,false)==1,"another peer changed mandatory schedule");
 std::cout<<"catchupBlocks="<<count[0]<<" attestations="<<count[1]<<" modeWindows="<<windows<<" normalScheduleUnchanged=true; opportunity model only, not WAN qualification\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
