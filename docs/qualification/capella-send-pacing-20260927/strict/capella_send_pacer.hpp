#pragma once
#include <algorithm>
#include <chrono>

// One catalog owns a peer at a time. This clock reserves send starts, not
// unbounded queued frames. Matches the receiver's four bulk requests/second;
// it does not borrow its initial burst or change admission at the receiver.
class CapellaSendPacer {
public:
 using Clock=std::chrono::steady_clock;
private:
 Clock::time_point next_{};
public:
 Clock::time_point reserve(Clock::time_point now){auto due=std::max(now,next_);next_=due+std::chrono::milliseconds(250);return due;}
};
