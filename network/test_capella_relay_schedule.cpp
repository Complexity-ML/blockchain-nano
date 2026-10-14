#include "capella_relay_schedule.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){try{
 // Every background catalog is followed by an attestation opportunity. Large
 // histories or failed/empty categories cannot queue four other kinds first.
 CapellaRelaySchedule schedule;std::array<unsigned,CapellaRelaySchedule::category_count> count{};
 for(unsigned i=0;i<12000;++i){const auto kind=schedule.next();check(kind<count.size(),"unknown category");++count[kind];if(i%2)check(kind==1,"attestation starved behind background categories");else check(kind!=1,"background category starved");}
 check(count[1]==6000,"attestation service share");for(unsigned i:CapellaRelaySchedule::background_categories)check(count[i]==1000,"background service share");
 // Check the starvation bound across all starting phases, including wrap.
 for(unsigned phase=0;phase<CapellaRelaySchedule::normal_opportunities;++phase){CapellaRelaySchedule q;for(unsigned i=0;i<phase;++i)q.next();std::array<bool,CapellaRelaySchedule::category_count> seen{};for(unsigned i=0;i<CapellaRelaySchedule::normal_opportunities;++i)seen[q.next()]=true;for(bool s:seen)check(s,"category missing from bounded service window");}
 // Different peers/sessions advance independently, even after one stalls.
 CapellaRelaySchedule slow,fast;check(slow.next()==0,"fresh session head first");for(unsigned i=0;i<93;++i)fast.next();check(slow.next()==1,"another peer changed schedule");CapellaRelaySchedule replacement;check(replacement.next()==0&&replacement.next()==1,"replacement session inherited old phase");
 // Block order alternates independently of other categories and peers. Every
 // 24 category opportunities contain a head turn and a historical turn.
 CapellaRelaySchedule order;unsigned blocks=0;for(unsigned i=0;i<12000;++i){if(order.next()==0){check(order.next_block_history_first()==bool(blocks%2),"moving head starved historical block turn");++blocks;}}check(blocks==1000,"missing block opportunities");
 CapellaRelaySchedule independent;check(!independent.next_block_history_first(),"peer inherited historical phase");check(!CapellaRelaySchedule{}.next_block_history_first(),"replacement inherited historical phase");
 // A simulated catalog completion delay does not accumulate delay from all
 // other categories before the next attestation. This is a scheduling model,
 // not a measurement of transport or a claim about WAN finality.
 CapellaRelaySchedule model;unsigned clock=0,last_vote=0,votes=0;constexpr std::array<unsigned,CapellaRelaySchedule::category_count> cost{3000,1500,5000,4000,2000,6000,5500};
 for(unsigned i=0;i<100;++i){auto kind=model.next();clock+=cost[kind];if(kind==1){if(votes)check(clock-last_vote<=7500,"multiple background catalogs delayed next vote");last_vote=clock;++votes;}}
 check(votes==50,"modeled attestation opportunities");
 std::cout<<"5 scheduler scenarios passed: bounded fairness, every starting phase, peer/session independence, modeled catalog delays; not WAN qualification\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
