#pragma once
#include "core.hpp"
#include <boost/asio.hpp>
#include <atomic>
namespace nano {
// Event-loop confined accounting, shared by HTTP and WebSocket. Reserve every
// simulation in a batch before running any item; notifications count too.
struct SimulationQueue {
 static constexpr size_t worker_count=2;
 boost::asio::thread_pool workers{worker_count};std::atomic<bool> stopping{false};size_t pending=0;static constexpr size_t limit=8;
 std::chrono::milliseconds maximum_wait;
 explicit SimulationQueue(std::chrono::milliseconds wait=std::chrono::seconds(4)):maximum_wait(wait){if(wait<=std::chrono::milliseconds::zero())throw std::invalid_argument("RPC queue wait must be positive");}
 // A timed-out job keeps its capacity reservation until the worker discards it.
 // Otherwise repeated expirations could grow an unbounded worker backlog.
 struct Job:std::enable_shared_from_this<Job> {
  enum class State {waiting,running,expired};
  std::atomic<State> state{State::waiting};
  boost::asio::steady_timer timer;
  std::chrono::steady_clock::time_point deadline;
  std::function<Json()> work;
  std::function<void(std::optional<Json>)> complete;
  std::function<void()> drained;
  bool delivered=false; // event-loop confined
  Job(boost::asio::any_io_executor executor,std::chrono::milliseconds wait,std::function<Json()> run,std::function<void(std::optional<Json>)> done,std::function<void()> cleanup):timer(executor),deadline(std::chrono::steady_clock::now()+wait),work(std::move(run)),complete(std::move(done)),drained(std::move(cleanup)){}
  void deliver(std::optional<Json> result){if(delivered)return;delivered=true;complete(std::move(result));}
  void start(boost::asio::thread_pool& workers){
   timer.expires_at(deadline);
   timer.async_wait([self=shared_from_this()](boost::system::error_code ec){
    if(ec)return;auto expected=State::waiting;self->state.compare_exchange_strong(expected,State::expired);
    if(self->state.load()==State::expired)self->deliver(std::nullopt);
   });
   boost::asio::post(workers,[self=shared_from_this()]{
    auto expected=State::waiting;
    if(std::chrono::steady_clock::now()>=self->deadline)self->state.compare_exchange_strong(expected,State::expired);
    expected=State::waiting;std::optional<Json> result;
    if(self->state.compare_exchange_strong(expected,State::running))result=self->work();
    boost::asio::post(self->timer.get_executor(),[self,result=std::move(result)]()mutable{
     self->timer.cancel();self->deliver(std::move(result));if(self->drained)self->drained();
    });
   });
  }
 };
 void dispatch(boost::asio::any_io_executor executor,std::function<Json()> work,std::function<void(std::optional<Json>)> complete,std::function<void()> drained={}){
  std::make_shared<Job>(executor,maximum_wait,std::move(work),std::move(complete),std::move(drained))->start(workers);
 }
 bool reserve(size_t count){if(stopping.load()||count>limit||count>limit-pending)return false;pending+=count;return true;}
 void release(size_t count){pending-=count;}
};
inline bool simulation_item(const Json& value){
 if(!value.is_object())return false;auto method=value.value("method",Json());return method=="nano_altairReceive"||method=="nano_altairStatus"||method=="nano_altairSubmit"||method=="nano_altairJob"||method=="nano_altairCancel"||method=="nano_altairForget"||method=="nano_altairBlock"||method=="nano_altairInventory"||method=="nano_altairVote"||method=="nano_altairVoteInventory"||method=="nano_altairRotateSession"||method=="nano_altairJobs"||method=="nano_bellatrixReceive"||method=="nano_bellatrixStatus"||method=="nano_bellatrixSubmit"||method=="nano_bellatrixJob"||method=="nano_bellatrixCancel"||method=="nano_bellatrixForget"||method=="nano_bellatrixBlock"||method=="nano_bellatrixInventory"||method=="nano_bellatrixVote"||method=="nano_bellatrixVoteInventory"||method=="nano_bellatrixRotateSession"||method=="nano_bellatrixJobs"||method=="eth_call"||method=="eth_estimateGas"||method=="eth_signTransaction"||method=="eth_sendTransaction"||
  method=="nno_registerValidator"||method=="nno_addStake"||method=="nno_withdrawStake";
}
inline size_t simulation_count(const Json& value){
 if(!value.is_array())return simulation_item(value)?1:0;
 size_t count=0;for(const auto& item:value)if(simulation_item(item)&&++count>SimulationQueue::limit)return SimulationQueue::limit+1;
 return count;
}
inline bool contains_simulation(const Json& value){return simulation_count(value)!=0;}
}
