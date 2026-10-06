#pragma once
#include "nano/consensus_capella_reactor.hpp"
#include <condition_variable>
#include <atomic>
#include <thread>
namespace nano {
// Single local clock authority. No peer timestamps. Backwards/oversized jumps
// or local failures latch an error until restart; cancellation alone does not.
class CapellaLocalClock {
 CapellaDurableReactor& reactor_;
 uint64_t maximum_step_;std::optional<uint64_t> last_observed_;
 std::function<uint64_t()> now_;
 std::mutex mutex_,wait_mutex_;std::condition_variable wake_;
 std::thread thread_;bool stopping_=false;std::atomic<bool> failed_{false};
public:
 CapellaLocalClock(CapellaDurableReactor&,uint64_t maximum_forward_step_ms,std::function<uint64_t()> now={});
 ~CapellaLocalClock();
 // Explicit startup-only recovery, before poll/duties. Each durable tick uses
 // at most the ordinary step bound; total downtime and step count are bounded.
 Json startup_catch_up(uint64_t maximum_downtime_ms,uint64_t maximum_steps,const std::function<void()>& checkpoint={});
 bool poll(const std::function<void()>& checkpoint={});
 // Health reports the latched fault, without waiting for a poll blocked on
 // admission. Poll ordering / last_observed_ remain protected by mutex_.
 bool healthy()const noexcept{return !failed_.load(std::memory_order_acquire);}
 void start(std::chrono::milliseconds interval);
 void stop();
};
}
