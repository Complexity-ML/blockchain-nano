#pragma once
#include <chrono>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
namespace nano_profile {
struct Counter {uint64_t calls=0,total=0,maximum=0;};
inline std::map<std::string,Counter>& counters(){static std::map<std::string,Counter> value;return value;}
inline std::mutex& mutex(){static std::mutex value;return value;}
struct Timer {const char* name;std::chrono::steady_clock::time_point start=std::chrono::steady_clock::now();
 ~Timer() noexcept {try {auto us=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start).count();std::lock_guard lock(mutex());auto& c=counters()[name];++c.calls;c.total+=us;c.maximum=std::max(c.maximum,uint64_t(us));if(c.calls%64==0)std::cerr<<"{\"event\":\"reactor-inner-profile\",\"phase\":\""<<name<<"\",\"calls\":"<<c.calls<<",\"totalUs\":"<<c.total<<",\"maximumUs\":"<<c.maximum<<"}"<<std::endl;}catch(...) {}}
};
}
