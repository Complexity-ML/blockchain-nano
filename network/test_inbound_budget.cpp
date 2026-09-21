#include "inbound_budget.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
void check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
int main(){try{
 using Clock=InboundBudget::Clock;auto now=Clock::time_point{};InboundBudget budget(now);
 std::vector<std::shared_ptr<InboundBudget::Lease>> leases;
 for(unsigned i=0;i<4;++i){auto lease=budget.acquire("peer",now);check(bool(lease),"per peer admission");leases.push_back(lease);}
 check(!budget.acquire("peer",now)&&budget.active()==4,"per peer concurrency bound");
 auto held=leases.back();leases.clear();check(budget.active()==1,"lease retained by asynchronous owner");held.reset();check(budget.active()==0,"last callback releases admission");
 for(unsigned i=0;i<12;++i)check(bool(budget.acquire("peer",now)),"remaining burst tokens");
 check(!budget.acquire("peer",now),"completed requests still consume rate budget");
 check(!budget.acquire("peer",now-std::chrono::seconds(10)),"backwards clock does not refill");
 check(bool(budget.acquire("peer",now+std::chrono::milliseconds(125))),"monotonic refill permits one request");
 InboundBudget global(now);for(unsigned i=0;i<16;++i){auto lease=global.acquire("peer"+std::to_string(i),now);check(bool(lease),"global concurrent admission");leases.push_back(lease);}
 check(!global.acquire("new",now)&&global.active()==16,"global concurrent bound");leases.clear();check(global.active()==0,"global release");
 for(unsigned i=0;i<112;++i)check(bool(global.acquire("burst"+std::to_string(i),now)),"remaining global burst");
 check(!global.acquire("fresh",now),"identity rotation cannot bypass global rate");
 check(bool(global.acquire("fresh",now+std::chrono::seconds(1))),"global rate refill");
 InboundBudget identities(now);for(unsigned i=0;i<256;++i)check(bool(identities.acquire("id"+std::to_string(i),now+std::chrono::milliseconds(i*20))),"bounded identity table fill");
 check(identities.tracked_peers()==256&&!identities.acquire("overflow",now+std::chrono::seconds(6)),"identity table is bounded");
 check(bool(identities.acquire("later",now+std::chrono::seconds(70)))&&identities.tracked_peers()==1,"idle identities expire");
 std::shared_ptr<InboundBudget::Lease> survivor;{InboundBudget transient(now);survivor=transient.acquire("retained",now);}survivor.reset();
 std::cout<<"Inbound concurrency, rates, identity bounds and lease lifetime passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
