#include "altair_worker.hpp"
#include <future>
#include <iostream>
using D=AltairWorker::Decision;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
auto token(){return std::make_shared<std::atomic<bool>>(false);}
AltairWireMessage message(size_t bytes){return {AltairWireKind::block,std::vector<uint8_t>(bytes)};}
int main(){try{
 std::promise<void> entered,release;auto gate=release.get_future().share();
 std::atomic<unsigned> handlers=0,completions=0;
 auto active=token(),queued=token();
 AltairWorker queue(2,16,[&](const auto&,const auto& cancel){++handlers;entered.set_value();gate.wait();check(cancel->load(),"running cancellation");return D::accepted;});
 auto done=[&](D d){if(++completions==1)check(d==D::accepted,"commit decision preserved after cancellation");else check(d==D::local_failure,"queued cancellation");};
 check(queue.submit(message(8),active,done),"first admitted");entered.get_future().wait();
 check(!queue.submit(message(9),token(),done),"active bytes count toward bound");
 auto spare=message(1);spare.ssz.reserve(32);
 check(!queue.submit(std::move(spare),token(),done),"allocation capacity bounded");
 check(queue.dispatcher()(message(8),queued,done),"dispatcher queues second");
 check(!queue.submit(message(0),token(),done),"active plus pending count bound");
 queue.stop();check(active->load()&&queued->load(),"stop cancels both");
 check(!queue.submit(message(1),token(),done),"shutdown rejects admission");
 check(completions==0,"running handler retains completion");
 release.set_value();queue.join();queue.stop();queue.join();
 check(handlers==1&&completions==2,"pending work skipped and all completions delivered");
 // Exceptions must not kill the worker, and accepted jobs retain FIFO order.
 std::promise<void> finished;std::atomic<unsigned> calls=0,done_calls=0;
 AltairWorker errors(3,16,[&](const auto& m,const auto&){check(++calls==m.ssz[0],"FIFO order");if(m.ssz[0]==1)throw std::runtime_error("local failure");return D::rejected;});
 for(uint8_t i=1;i<=2;++i){auto m=message(1);m.ssz[0]=i;check(errors.submit(std::move(m),token(),[&,i](D d){check(d==(i==1?D::local_failure:D::rejected),"exception classification");if(++done_calls==2)finished.set_value();}),"error sequence admitted");}
 finished.get_future().wait();errors.stop();errors.join();check(calls==2,"worker survives handler exception");
 auto cancelled=token();cancelled->store(true);check(!errors.submit(message(1),cancelled,[](D){}),"cancelled request refused");
 std::cout<<"{\"scenario\":\"altair-bounded-worker\",\"boundedBytesAndJobs\":true,\"cooperativeShutdown\":true,\"networkActive\":false}\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
