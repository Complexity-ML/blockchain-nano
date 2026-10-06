#pragma once
#include "core.hpp"
#include <atomic>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <unistd.h>
namespace nano {
// Opt-in local diagnostics. Never record RPC parameters, keys or transaction data.
// Logging failure must not affect execution. Each process owns its trace file.
namespace rpc_timing {
using Clock=std::chrono::steady_clock;
inline const std::string& directory(){static const std::string path=[] {auto p=std::getenv("NANO_RPC_TIMING_DIR");return p?std::string(p):std::string{};}();return path;}
inline std::atomic<uint64_t> next{0};
inline thread_local uint64_t request=0;
inline uint64_t allocate(){return directory().empty()?0:++next;}
inline void record(const char* phase,Clock::time_point start,uint64_t id)noexcept{
 if(!id)return;
 const auto end=Clock::now();
 try{static std::mutex output_mutex;std::lock_guard lock(output_mutex);
  std::ofstream file(directory()+"/rpc-"+std::to_string(getpid())+".jsonl",std::ios::app);
  file<<Json{{"request",id},{"phase",phase},{"startNs",std::chrono::duration_cast<std::chrono::nanoseconds>(start.time_since_epoch()).count()},{"durationMs",std::chrono::duration<double,std::milli>(end-start).count()}}.dump()<<'\n';
 }catch(...){}
}
struct Context {uint64_t previous=request;explicit Context(uint64_t id){request=id;}~Context(){request=previous;}};
struct Span {const char* phase;Clock::time_point start=Clock::now();uint64_t id=request;explicit Span(const char* name):phase(name){}~Span(){record(phase,start,id);}};
}
}
