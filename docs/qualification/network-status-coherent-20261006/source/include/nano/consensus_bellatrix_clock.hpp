#pragma once
#include "nano/consensus_bellatrix_reactor.hpp"
#include <condition_variable>
#include <thread>
namespace nano {
// Single local clock authority. No peer timestamps. Backwards/oversized jumps
// or local failures latch an error until restart; cancellation alone does not.
class BellatrixLocalClock {
 BellatrixDurableReactor& reactor_;
 uint64_t maximum_step_;std::optional<uint64_t> last_observed_;
 std::function<uint64_t()> now_;
 std::mutex mutex_,wait_mutex_;std::condition_variable wake_;
 std::thread thread_;bool stopping_=false,failed_=false;
public:
 BellatrixLocalClock(BellatrixDurableReactor&,uint64_t maximum_forward_step_ms,std::function<uint64_t()> now={});
 ~BellatrixLocalClock();
 bool poll(const std::function<void()>& checkpoint={});
 bool healthy(){std::lock_guard lock(mutex_);return !failed_;}
 void start(std::chrono::milliseconds interval);
 void stop();
};
}
