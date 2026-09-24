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
 // Exhaust bulk admission, then prove control can use the reserved capacity.
 using Traffic=InboundBudget::Traffic;
 InboundBudget reserved(now);leases.clear();
 for(unsigned i=0;i<2;++i)leases.push_back(reserved.acquire("busy",now,Traffic::bulk));
 check(leases[0]&&leases[1]&&!reserved.acquire("busy",now,Traffic::bulk),"bulk concurrency bounded at two");
 for(unsigned i=0;i<2;++i){auto lease=reserved.acquire("busy",now,Traffic::control);check(bool(lease),"control survives held bulk leases");leases.push_back(lease);}
 check(!reserved.acquire("busy",now,Traffic::control)&&reserved.active()==4,"combined peer concurrency unchanged");leases.clear();
 for(unsigned i=0;i<6;++i)check(bool(reserved.acquire("busy",now,Traffic::bulk)),"remaining bulk tokens");
 check(!reserved.acquire("busy",now,Traffic::bulk),"bulk rate exhausted");
 for(unsigned i=0;i<6;++i)check(bool(reserved.acquire("busy",now,Traffic::control)),"reserved peer tokens available to control");
 check(!reserved.acquire("busy",now,Traffic::control),"combined peer burst unchanged");
 check(!reserved.acquire("busy",now+std::chrono::milliseconds(125),Traffic::bulk),"bulk cannot borrow reserved refill");
 check(bool(reserved.acquire("busy",now+std::chrono::milliseconds(125),Traffic::control)),"control gets reserved refill");
 InboundBudget rotated(now);
 for(unsigned i=0;i<8;++i){auto lease=rotated.acquire("bulk"+std::to_string(i),now,Traffic::bulk);check(bool(lease),"bulk global admission");leases.push_back(lease);}
 check(!rotated.acquire("extra",now,Traffic::bulk),"bulk identity rotation cannot exhaust global concurrency");
 for(unsigned i=0;i<8;++i){auto lease=rotated.acquire("control"+std::to_string(i),now,Traffic::control);check(bool(lease),"global control reserve");leases.push_back(lease);}
 check(!rotated.acquire("more",now,Traffic::control)&&rotated.active()==16,"combined global concurrency unchanged");leases.clear();
 for(unsigned i=0;i<56;++i)check(bool(rotated.acquire("b"+std::to_string(i),now,Traffic::bulk)),"remaining global bulk burst");
 check(!rotated.acquire("rotated",now,Traffic::bulk),"bulk identity rotation cannot steal reserved tokens");
 for(unsigned i=0;i<56;++i)check(bool(rotated.acquire("c"+std::to_string(i),now,Traffic::control)),"remaining global control burst");
 check(!rotated.acquire("last",now,Traffic::control),"combined global burst unchanged");
 InboundBudget work(now);
 check(!work.admit_transactions("unknown",1,now),"unadmitted peer cannot consume work budget");
 auto first=work.acquire("first",now,Traffic::bulk);check(bool(first),"first work stream");
 check(!work.admit_transactions("first",0,now)&&!work.admit_transactions("first",51,now),"empty and oversized batches refused before work");
 check(work.admit_transactions("first",50,now)&&work.admit_transactions("first",14,now),"every transaction charges peer burst");
 check(!work.admit_transactions("first",1,now),"small repeated batches cannot bypass work limit");
 auto second=work.acquire("second",now,Traffic::bulk);check(bool(second),"second work stream");
 check(work.admit_transactions("second",50,now)&&work.admit_transactions("second",14,now),"peer rejection does not consume global work tokens");
 auto rotated_work=work.acquire("rotated",now,Traffic::bulk);check(bool(rotated_work)&&!work.admit_transactions("rotated",1,now),"identity rotation cannot bypass global work limit");
 check(!work.admit_transactions("first",1,now-std::chrono::seconds(1)),"backwards clock cannot refill work budget");
 check(work.admit_transactions("first",1,now+std::chrono::milliseconds(125)),"peer work refills at eight transactions per second");
 first.reset();check(!work.admit_transactions("first",1,now+std::chrono::seconds(1)),"closed stream cannot forward work");
 first=work.acquire("first",now+std::chrono::seconds(1),Traffic::bulk);check(bool(first)&&!work.admit_transactions("first",8,now+std::chrono::seconds(1))&&work.admit_transactions("first",7,now+std::chrono::seconds(1)),"reopening stream preserves consumed transaction budget");
 std::cout<<"Inbound concurrency, rates, identity bounds and lease lifetime passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
