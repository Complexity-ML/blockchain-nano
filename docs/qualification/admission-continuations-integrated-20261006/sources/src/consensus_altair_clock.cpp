#include "nano/consensus_altair_clock.hpp"
namespace nano {
AltairLocalClock::AltairLocalClock(AltairDurableReactor& reactor,uint64_t maximum,std::function<uint64_t()> now):reactor_(reactor),maximum_step_(maximum),now_(std::move(now)){
 if(!maximum_step_)throw std::invalid_argument("Altair clock step bound required");
 if(!now_)now_=[](){const auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();if(ms<0)throw std::runtime_error("negative system clock");return uint64_t(ms);};
}
AltairLocalClock::~AltairLocalClock(){stop();}
bool AltairLocalClock::poll(const std::function<void()>& checkpoint){
 std::lock_guard lock(mutex_);if(failed_)throw std::runtime_error("Altair local clock unavailable; restart required");
 bool cancelled=false;auto check=[&]{if(checkpoint)try{checkpoint();}catch(...){cancelled=true;throw;}};
 try{
  check();const auto now=now_();const auto previous=reactor_.current().state->timed.clock.time_ms;
  if(now<previous||now-previous>maximum_step_)throw std::runtime_error("Altair local clock discontinuity");
  if(now==previous)return false;
  reactor_.tick(now,check);return true;
 }catch(...){if(!cancelled)failed_=true;throw;}
}
void AltairLocalClock::start(std::chrono::milliseconds interval){
 if(interval<=std::chrono::milliseconds::zero())throw std::invalid_argument("Altair clock interval required");
 std::lock_guard lock(wait_mutex_);if(thread_.joinable()||stopping_)throw std::logic_error("Altair clock already started/stopped");
 thread_=std::thread([this,interval]{
  std::unique_lock lock(wait_mutex_);
  while(!wake_.wait_for(lock,interval,[&]{return stopping_;})){
   lock.unlock();try{poll();}catch(...){return;}lock.lock();
  }
 });
}
void AltairLocalClock::stop(){
 {std::lock_guard lock(wait_mutex_);stopping_=true;}wake_.notify_all();
 if(thread_.joinable())thread_.join();
}
}
