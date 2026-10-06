#pragma once
#include <chrono>
#include <cstdint>
#include <functional>
#include <limits>
#include <mutex>
#include <utility>
#include <stdexcept>
namespace nano {
// Local-only time. Wall adjustments within the explicit budget never change
// consensus time; larger divergence latches failure. Not a peer time oracle.
class LocalMonotonicTime {
 std::function<uint64_t()> wall_,steady_;
 uint64_t budget_,wall_anchor_,steady_anchor_,last_steady_;
 std::mutex mutex_;bool failed_=false;
 static uint64_t wall_ms(){
  const auto value=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  if(value<0)throw std::runtime_error("Negative local wall time");return uint64_t(value);
 }
 static uint64_t steady_ms(){
  const auto value=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
  if(value<0)throw std::runtime_error("Negative local steady time");return uint64_t(value);
 }
public:
 explicit LocalMonotonicTime(uint64_t maximum_wall_drift_ms,std::function<uint64_t()> wall={},std::function<uint64_t()> steady={}):
  wall_(wall?std::move(wall):wall_ms),steady_(steady?std::move(steady):steady_ms),budget_(maximum_wall_drift_ms){
  if(!budget_)throw std::invalid_argument("Local wall drift budget required");
  steady_anchor_=last_steady_=steady_();wall_anchor_=wall_();
 }
 uint64_t now(){
  std::lock_guard lock(mutex_);if(failed_)throw std::runtime_error("Local monotonic time unavailable; restart required");
  try{
   const auto elapsed_time=steady_();
   if(elapsed_time<last_steady_||elapsed_time<steady_anchor_)throw std::runtime_error("Local steady time regressed");
   const auto elapsed=elapsed_time-steady_anchor_;
   if(elapsed>std::numeric_limits<uint64_t>::max()-wall_anchor_)throw std::overflow_error("Local monotonic time overflow");
   const auto projected=wall_anchor_+elapsed,wall=wall_();
   const auto drift=wall>=projected?wall-projected:projected-wall;
   if(drift>budget_)throw std::runtime_error("Local wall time drift exceeds configured budget");
   last_steady_=elapsed_time;return projected;
  }catch(...){failed_=true;throw;}
 }
};
}
