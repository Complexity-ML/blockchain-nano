#include "nano/local_monotonic_time.hpp"
#include <iostream>
#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <algorithm>
using nano::LocalMonotonicTime;
void require(bool value){if(!value)throw std::runtime_error("Time assertion failed");}
template<class F>void refuses(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}require(caught);}
int main(){try{
 uint64_t wall=10000,steady=100;
 auto source=[&](uint64_t budget){return std::make_unique<LocalMonotonicTime>(budget,[&]{return wall;},[&]{return steady;});};
 refuses([&]{source(0);});
 auto time=source(250);require(time->now()==10000);
 steady+=100;wall+=57;require(time->now()==10100); // observed43ms reversal never reaches duties
 steady+=100;wall=10212;require(time->now()==10200); // bounded forward adjustment
 steady+=100;wall=10050;require(time->now()==10300); // exact lower bound
 steady+=100;wall=10650;require(time->now()==10400); // exact upper bound
 ++wall;refuses([&]{time->now();});wall=10400;refuses([&]{time->now();}); // fault latched
 wall=20000;steady=200;time=source(250);--wall;require(time->now()==20000);
 --steady;refuses([&]{time->now();});++steady;refuses([&]{time->now();});
 wall=UINT64_MAX-2;steady=100;time=source(250);steady+=3;refuses([&]{time->now();});
 wall=30000;steady=100;time=source(250);wall-=251;refuses([&]{time->now();});wall=30000;refuses([&]{time->now();});
 bool broken=false;LocalMonotonicTime broken_source(250,[&](){if(broken)throw std::runtime_error("source failed");return uint64_t(10);},[]{return uint64_t(10);});broken=true;refuses([&]{broken_source.now();});broken=false;refuses([&]{broken_source.now();});
 std::atomic<uint64_t> ticks{0};LocalMonotonicTime concurrent(10000,[]{return uint64_t(10000);},[&]{return ticks.fetch_add(1);});
 std::vector<std::vector<uint64_t>> values(8);std::vector<std::thread> workers;
 for(size_t i=0;i<values.size();++i)workers.emplace_back([&,i]{for(size_t j=0;j<100;++j)values[i].push_back(concurrent.now());});
 for(auto& worker:workers)worker.join();std::vector<uint64_t> combined;
 for(const auto& row:values){require(std::is_sorted(row.begin(),row.end()));combined.insert(combined.end(),row.begin(),row.end());}
 std::sort(combined.begin(),combined.end());require(combined.size()==800&&combined.front()==10001&&combined.back()==10800&&std::adjacent_find(combined.begin(),combined.end())==combined.end());
 std::cout<<"bounded wall adjustments, latched faults, overflow and800 concurrent observations passed\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
