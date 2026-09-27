#pragma once
#include "../../network/capella_inventory_sessions.hpp"
#include <barrier>
#include <thread>

namespace test {
// Real snapshots, fixed synthetic clock, bounded workers and requests. This
// probes registry synchronization; it is not a distributed-node/load benchmark.
inline nano::Json qualify_capella_catalog_concurrency(nano::CapellaDurableReactor& reactor){
 using Clock=std::chrono::steady_clock;
 constexpr size_t workers=32,rounds=32;
 const auto id=[](uint64_t n){auto s=nano::quantity(nano::Int(std::to_string(n)));return "0x"+std::string(66-s.size(),'0')+s.substr(2);};
 const auto baseline=reactor.current().token;
 const auto bytes=reactor.capture_inventory("block",1024*1024)->root_bytes();
 if(!bytes)throw std::runtime_error("Concurrent catalog test needs nonempty inventory");
 nano::Json results=nano::Json::array();
 for(const size_t capacity:{size_t(2),size_t(4)}){
  // maximum=4 in both runs: capacity=2 independently exercises byte quota.
  CapellaInventorySessions sessions(reactor,4,bytes*capacity,std::chrono::seconds(1),[]{return Clock::time_point{};});
  std::barrier phase(static_cast<std::ptrdiff_t>(workers));
  std::array<size_t,workers> admitted{},limited{},recovered{};
  std::atomic<size_t> live=0,peak=0,errors=0;
  auto error=[&]{++errors;};
  std::vector<std::thread> threads;
  for(size_t worker=0;worker<workers;++worker)threads.emplace_back([&,worker]{
   for(size_t round=0;round<rounds;++round){
    std::string generation,handle;const auto request=id(1+round*workers+worker);
    try{generation=sessions.generation();}catch(...){error();}
    phase.arrive_and_wait();
    try{
     handle=sessions.acquire("shared-daemon",generation,request,"block");
     ++admitted[worker];const auto n=++live;auto old=peak.load();while(old<n&&!peak.compare_exchange_weak(old,n)){}
     if(sessions.page("shared-daemon",handle,{},2,512).at("entries").empty())error();
    }catch(const std::length_error&){++limited[worker];}catch(...){error();}
    phase.arrive_and_wait(); // all attempts finish before any snapshot is released
    if(live!=capacity)error();
    phase.arrive_and_wait();
    try{sessions.release("shared-daemon",generation,request);}catch(...){error();}
    if(!handle.empty())--live;
    phase.arrive_and_wait();
    if(live!=0)error();
   }
   // Under a quiet, explicitly serialized schedule every worker can recover.
   // This does NOT claim mutex fairness during unscheduled contention.
   for(size_t turn=0;turn<workers;++turn){
    phase.arrive_and_wait();
    if(turn==worker)try{const auto gen=sessions.generation(),request=id(100000+worker);const auto h=sessions.acquire("shared-daemon",gen,request,"block");if(sessions.page("shared-daemon",h,{},2,512).at("entries").empty())error();sessions.release("shared-daemon",gen,request);++recovered[worker];}catch(...){error();}
    phase.arrive_and_wait();
   }
  });
  for(auto& thread:threads)thread.join();
  size_t total=0,rejected=0;for(size_t w=0;w<workers;++w){total+=admitted[w];rejected+=limited[w];if(recovered[w]!=1)error();}
  if(errors||total!=rounds*capacity||rejected!=rounds*(workers-capacity)||peak!=capacity)throw std::runtime_error("Concurrent catalog quota/recovery invariant failed");
  results.push_back({{"rootBudgetBytes",bytes*capacity},{"maximumHandles",4},{"admitted",total},{"capacityRejected",rejected},{"peakLive",peak.load()},{"perWorkerAdmitted",admitted},{"quietRecoveryWorkers",workers}});
 }
 // Identical concurrent requests must all observe exactly one snapshot.
 {
  CapellaInventorySessions sessions(reactor,1,bytes,std::chrono::seconds(1),[]{return Clock::time_point{};});
  const auto gen=sessions.generation(),request=id(200000);std::barrier phase(static_cast<std::ptrdiff_t>(workers));
  std::array<std::string,workers> handles;std::atomic<size_t> errors=0;std::vector<std::thread> threads;
  for(size_t w=0;w<workers;++w)threads.emplace_back([&,w]{phase.arrive_and_wait();try{handles[w]=sessions.acquire("daemon",gen,request,"block");}catch(...){++errors;}});
  for(auto& t:threads)t.join();for(const auto& h:handles)if(h.empty()||h!=handles[0])++errors;
  sessions.release("daemon",gen,request);if(errors)throw std::runtime_error("Concurrent duplicate acquire allocated multiple snapshots");
 }
 // Cancellation and acquisition race under the same generation. Whichever
 // linearizes first, no handle may remain readable after both calls return.
 size_t won=0,cancelled=0;
 for(size_t round=0;round<128;++round){
  CapellaInventorySessions sessions(reactor,1,bytes,std::chrono::seconds(1),[]{return Clock::time_point{};});
  const auto gen=sessions.generation(),request=id(300000+round);std::barrier phase(2);std::string handle;std::exception_ptr failure;
  std::thread opener([&]{phase.arrive_and_wait();try{handle=sessions.acquire("daemon",gen,request,"block");}catch(const std::invalid_argument&){}catch(...){failure=std::current_exception();}});
  phase.arrive_and_wait();sessions.release("daemon",gen,request);opener.join();if(failure)std::rethrow_exception(failure);
  if(handle.empty())++cancelled;else{++won;bool missing=false;try{sessions.page("daemon",handle,{},1,512);}catch(const std::invalid_argument&){missing=true;}if(!missing)throw std::runtime_error("Cancel/acquire race leaked readable handle");}
  bool fenced=false;try{sessions.acquire("daemon",gen,request,"block");}catch(const std::invalid_argument&){fenced=true;}if(!fenced)throw std::runtime_error("Cancel/acquire race resurrected request");
  const auto h=sessions.open("capacity-probe","block");sessions.close("capacity-probe",h);
 }
 if(reactor.current().token!=baseline)throw std::runtime_error("Catalog contention changed consensus checkpoint");
 return {{"scenario","capella-catalog-concurrency"},{"workers",workers},{"rounds",rounds},{"quotaRuns",results},{"duplicateWorkers",workers},{"raceAcquiredBeforeCancel",won},{"raceCancelledBeforeAcquire",cancelled},{"consensusUnchanged",true},{"distributed",false}};
}
}
