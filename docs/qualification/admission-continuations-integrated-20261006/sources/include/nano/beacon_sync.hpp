#pragma once
#include <chrono>
#include <cstdint>
#include <utility>
#include <functional>
#include <mutex>
#include <optional>
#include <vector>
namespace nano {
// Local supervisor observations, not consensus authority. Peer slot claims may
// only affect monitoring; they never admit blocks or advance the local clock.
class BeaconSyncTracker {
public:
 using Clock=std::chrono::steady_clock;
 struct Sample {std::vector<uint64_t> peer_slots;};
 explicit BeaconSyncTracker(std::function<Clock::time_point()> now=Clock::now):now_(std::move(now)){}
 void report(std::vector<uint64_t> slots){std::lock_guard lock(mutex_);sample_=Sample{std::move(slots)};seen_=now_();}
 std::optional<Sample> current(){std::lock_guard lock(mutex_);const auto now=now_();if(!sample_||now<seen_||now-seen_>=std::chrono::seconds(5))return {};return sample_;}
private:
 std::mutex mutex_;std::function<Clock::time_point()> now_;Clock::time_point seen_{};std::optional<Sample> sample_;
};
}
