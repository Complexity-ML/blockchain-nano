#include "altair_worker.hpp"
#include <future>
#include <iostream>
using namespace std::chrono_literals;
using D=AltairWorker::Decision;
static void check(bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);}
static auto token(){return std::make_shared<std::atomic<bool>>(false);}
static AltairWireMessage message(uint8_t id){AltairWireMessage m{AltairWireKind::block,std::vector<uint8_t>(4)};m.ssz[0]=id;return m;}
int main(){try{
 unsigned cases=0;
 {std::promise<void> entered,release,finished;auto gate=release.get_future().share();std::vector<unsigned> order,completions;unsigned first=0;std::atomic<unsigned> done=0;
  AltairWorker queue(2,8,[&](const auto& m,const auto&){order.push_back(m.ssz[0]);if(m.ssz[0]==2)return D::rejected;if(++first==1){entered.set_value();gate.wait();}return first<4?D::deferred:D::accepted;});
  check(queue.submit(message(1),token(),[&](D d){check(d==D::accepted,"deferred job leaked incomplete decision");completions.push_back(1);if(++done==2)finished.set_value();}),"first admission");entered.get_future().wait();
  check(queue.submit(message(2),token(),[&](D d){check(d==D::rejected,"independent job changed decision");completions.push_back(2);if(++done==2)finished.set_value();}),"second admission");check(!queue.submit(message(3),token(),[](D){}),"deferred work released job/byte reservation");release.set_value();check(finished.get_future().wait_for(3s)==std::future_status::ready,"deferred work starved queued job");queue.stop();queue.join();
  check(order==std::vector<unsigned>({1,2,1,1,1})&&completions==std::vector<unsigned>({2,1}),"continuations are not FIFO quanta or complete prematurely");++cases;}
 {std::promise<void> done;auto cancel=token();unsigned calls=0,completions=0;AltairWorker queue(1,4,[&](const auto&,const auto& c){++calls;c->store(true);return D::deferred;});
  check(queue.submit(message(1),cancel,[&](D d){check(d==D::local_failure,"cancelled continuation leaked pending/accepted");++completions;done.set_value();}),"cancel admission");check(done.get_future().wait_for(3s)==std::future_status::ready,"cancelled continuation did not retire");queue.stop();queue.join();check(calls==1&&completions==1,"cancelled continuation reentered or completed twice");++cases;}
 {std::promise<void> entered,release,done;auto gate=release.get_future().share();auto cancel=token();unsigned calls=0;AltairWorker queue(1,4,[&](const auto&,const auto&){++calls;entered.set_value();gate.wait();return D::deferred;});
  check(queue.submit(message(1),cancel,[&](D d){check(d==D::local_failure,"shutdown continuation leaked incomplete decision");done.set_value();}),"stop admission");entered.get_future().wait();queue.stop();check(cancel->load(),"shutdown did not cancel running continuation");release.set_value();check(done.get_future().wait_for(3s)==std::future_status::ready,"shutdown continuation retained reservation");queue.join();check(calls==1,"stopped continuation requeued");++cases;}
 {std::promise<void> done;unsigned calls=0;AltairWorker queue(1,4,[&](const auto&,const auto&){if(++calls==1)return D::deferred;throw std::runtime_error("continuation failure");return D::accepted;});
  check(queue.submit(message(1),token(),[&](D d){check(d==D::local_failure,"exception after deferral classification");done.set_value();}),"exception admission");check(done.get_future().wait_for(3s)==std::future_status::ready,"deferred exception not retired");queue.stop();queue.join();check(calls==2,"exception lost continuation count");++cases;}
 {std::promise<void> entered,release,done;auto gate=release.get_future().share();unsigned calls=0,completions=0;AltairWorker queue(1,4,[&](const auto&,const auto&){if(++calls==1){entered.set_value();gate.wait();}return calls<1000?D::deferred:D::accepted;});
  check(queue.submit(message(1),token(),[&](D d){check(d==D::accepted,"long continuation completed before commit");++completions;done.set_value();}),"long admission");entered.get_future().wait();check(!queue.submit(message(2),token(),[](D){}),"long continuation exceeded reservation bounds");release.set_value();check(done.get_future().wait_for(3s)==std::future_status::ready,"bounded continuation stalled");queue.stop();queue.join();check(calls==1000&&completions==1,"long continuation duplicated completion or lost job");++cases;}
 std::cout<<cases<<" bounded worker continuation scenarios passed; no consensus/network qualification\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
