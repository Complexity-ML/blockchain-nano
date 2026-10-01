#include "nano/consensus_capella_clock.hpp"
namespace nano {
CapellaLocalClock::CapellaLocalClock(CapellaDurableReactor& reactor,uint64_t maximum,std::function<uint64_t()> now):reactor_(reactor),maximum_step_(maximum),now_(std::move(now)){
 if(!maximum_step_)throw std::invalid_argument("Capella clock step bound required");
 if(!now_)now_=[](){const auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();if(ms<0)throw std::runtime_error("negative system clock");return uint64_t(ms);};
}
CapellaLocalClock::~CapellaLocalClock(){stop();}
bool CapellaLocalClock::poll(const std::function<void()>& checkpoint){
 std::lock_guard lock(mutex_);if(failed_)throw std::runtime_error("Capella local clock unavailable; restart required");
 bool cancelled=false;auto check=[&]{if(checkpoint)try{checkpoint();}catch(...){cancelled=true;throw;}};
 try{
  check();const auto now=now_();const auto previous=reactor_.current().state->timed.clock.time_ms;
  const auto observed=last_observed_.value_or(previous);
  const bool before_durable=now<previous,before_observed=now<observed;
  const bool forward_exceeded=now>=observed&&now-observed>maximum_step_;
  if(before_durable||before_observed||forward_exceeded)
   throw std::runtime_error("Capella local clock discontinuity: nowMs="+std::to_string(now)+
    ", durableMs="+std::to_string(previous)+", observedMs="+std::to_string(observed)+
    ", maximumForwardStepMs="+std::to_string(maximum_step_)+
    ", beforeDurable="+std::to_string(before_durable)+", beforeObserved="+std::to_string(before_observed)+
    ", forwardStepExceeded="+std::to_string(forward_exceeded));
  const bool changed=reactor_.tick_if_needed(now,check);
  // Still observe every poll: coalescing must not hide backward/oversized steps
  // within an otherwise unchanged consensus phase.
  last_observed_=now;return changed;
 }catch(...){if(!cancelled){failed_=true;reactor_.invalidate();}throw;}
}
void CapellaLocalClock::start(std::chrono::milliseconds interval){
 if(interval<=std::chrono::milliseconds::zero())throw std::invalid_argument("Capella clock interval required");
 std::lock_guard lock(wait_mutex_);if(thread_.joinable()||stopping_)throw std::logic_error("Capella clock already started/stopped");
 thread_=std::thread([this,interval]{
  std::unique_lock lock(wait_mutex_);
  while(!wake_.wait_for(lock,interval,[&]{return stopping_;})){
   lock.unlock();try{poll();}catch(...){return;}lock.lock();
  }
 });
}
void CapellaLocalClock::stop(){
 {std::lock_guard lock(wait_mutex_);stopping_=true;}wake_.notify_all();
 if(thread_.joinable())thread_.join();
}
}
