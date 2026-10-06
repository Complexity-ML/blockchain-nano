#pragma once
#include <algorithm>
#include <chrono>

// Serial use, one active catalog per peer. Four starts may burst after idle;
// subsequent starts are spaced at the receiver's four bulk requests/second.
// Half of its eight-token burst remains available to other bulk traffic.
// Reserves time only; caller retains the existing bounded relay ownership.
class CapellaSendPacer {
public:
 using Clock=std::chrono::steady_clock;
private:
 Clock::time_point next_{};bool initialized_=false;
public:
 Clock::time_point reserve(Clock::time_point now){
  const auto floor=now-std::chrono::milliseconds(750);
  if(!initialized_||next_<floor){next_=floor;initialized_=true;}
  auto due=std::max(now,next_);next_+=std::chrono::milliseconds(250);return due;
 }
};
