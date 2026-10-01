#include "nano/types.hpp"
#include <chrono>
#include <iostream>
int main(){nano::Int n=600000;uint64_t sum=0;auto start=std::chrono::steady_clock::now();for(unsigned i=0;i<5000000;++i)sum+=nano::u64(n);std::cout<<nano::Json{{"conversions",5000000},{"checksum",sum},{"milliseconds",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()}}.dump()<<'\n';return sum==3000000000000ULL?0:1;}
