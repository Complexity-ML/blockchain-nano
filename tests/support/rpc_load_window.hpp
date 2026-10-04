#pragma once
#include <chrono>
#include <stdexcept>
// Keep issuing real requests until the observer has evidence, bounded by both
// time and rounds. Fixed-round overload campaigns retain their original policy.
inline bool continue_rpc_load(unsigned round,unsigned minimum,bool bounded_observation,
                              bool observed,std::chrono::milliseconds elapsed,
                              std::chrono::milliseconds deadline){
 if(!bounded_observation)return round<minimum;
 if(elapsed>=deadline)throw std::runtime_error("RPC observation workload deadline exceeded");
 const bool needed=round<minimum||!observed;
 if(needed&&round>=1024)throw std::runtime_error("RPC observation workload round cap exceeded");
 return needed;
}
