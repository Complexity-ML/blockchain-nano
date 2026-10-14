#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
namespace nano::capella_detail {
// Runtime work budget, never a consensus/UTC clock or an administrative grant.
struct CapellaStartupDeadlineExpired : std::runtime_error {
 uint64_t maximum_ms,elapsed_ms;
 CapellaStartupDeadlineExpired(uint64_t maximum,uint64_t elapsed)
  :std::runtime_error("Capella startup deadline exceeded: maximumMs="+std::to_string(maximum)+", elapsedMs="+std::to_string(elapsed)),maximum_ms(maximum),elapsed_ms(elapsed){}
};
class CapellaStartupBudget {
public:
 using Clock=std::chrono::steady_clock;
 using SteadySource=std::function<Clock::time_point()>;
private:
 struct State {
  std::optional<uint64_t> maximum;
  Clock::time_point started,last;
  Clock::duration duration{};
  const SteadySource now;
  const std::function<void()> caller;
  std::atomic<bool> active{true};
  std::mutex observations;
  State(std::optional<uint64_t> limit,Clock::time_point start,SteadySource source,std::function<void()> check)
   :maximum(limit),started(start),last(start),now(std::move(source)),caller(std::move(check)){
   if(maximum){
    const auto bound=std::chrono::duration_cast<std::chrono::milliseconds>(Clock::duration::max()/2).count();
    if(!*maximum||bound<=0||*maximum>uint64_t(bound)||!now)throw std::invalid_argument("Capella startup elapsed budget range");
    duration=std::chrono::duration_cast<Clock::duration>(std::chrono::milliseconds(*maximum));
   }
  }
  void observe(){
   if(!active.load(std::memory_order_acquire))return;
   const auto at=now();
   std::lock_guard lock(observations);
   if(!active.load(std::memory_order_acquire))return;
   if(at<started||at<last)throw std::runtime_error("Capella startup steady clock regressed");
   using Rep=Clock::duration::rep;using Unsigned=std::make_unsigned_t<Rep>;
   static_assert(std::is_integral_v<Rep> && sizeof(Rep)<=sizeof(uint64_t));
   // Unsigned difference avoids overflowing signed time_point subtraction.
   const auto ticks=Unsigned(at.time_since_epoch().count())-Unsigned(started.time_since_epoch().count());
   if(ticks>Unsigned(Clock::duration::max().count()))throw std::overflow_error("Capella startup steady elapsed range");
   const Clock::duration elapsed{Rep(ticks)};last=at;
   if(elapsed>=duration){const auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();throw CapellaStartupDeadlineExpired(*maximum,uint64_t(ms));}
  }
  void check(){
   if(!active.load(std::memory_order_acquire))return;
   if(maximum)observe();
   if(!active.load(std::memory_order_acquire))return;
   if(caller)caller();
   if(maximum&&active.load(std::memory_order_acquire))observe();
  }
 };
 std::shared_ptr<State> state_;
public:
 // The injectable source is private tooling for deterministic unit tests.
 // Production runtime passes steady_clock::now explicitly; no public/env hook.
 CapellaStartupBudget(std::optional<uint64_t> maximum,std::function<void()> caller,
  Clock::time_point started,SteadySource source)
  :state_(std::make_shared<State>(maximum,started,std::move(source),std::move(caller))){}
 CapellaStartupBudget(const CapellaStartupBudget&)=delete;
 CapellaStartupBudget& operator=(const CapellaStartupBudget&)=delete;
 ~CapellaStartupBudget(){close();}
 std::function<void()> checkpoint()const{return [state=state_]{state->check();};}
 void close()noexcept{state_->active.store(false,std::memory_order_release);}
 // Complete the final check while startup is still synchronous. Any retained
 // copy becomes fully inert before clock/duty workers start, including caller.
 void finish(){if(state_->maximum)state_->check();close();}
};
}
