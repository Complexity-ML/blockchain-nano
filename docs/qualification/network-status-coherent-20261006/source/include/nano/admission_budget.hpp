#pragma once
#include <chrono>
#include <algorithm>
#include <cstddef>
namespace nano {
// Access serialized by Rpc::mutex_. Units count submitted transactions, not
// connections or JSON-RPC envelopes. Admission is local policy, never consensus.
class AdmissionBudget {
public:
 using Clock=std::chrono::steady_clock;
private:
 double tokens_=128;
 Clock::time_point updated_;
public:
 explicit AdmissionBudget(Clock::time_point now):updated_(now){}
 bool consume(size_t count,Clock::time_point now){
  if(now>updated_){tokens_=std::min(128.0,tokens_+std::chrono::duration<double>(now-updated_).count()*64);updated_=now;}
  if(count>128||tokens_<count)return false;
  tokens_-=count;return true;
 }
};
}
