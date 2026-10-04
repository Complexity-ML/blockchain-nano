#include "nano/types.hpp"
#include <chrono>
#include <iostream>
#include <cstdlib>
#include <new>
using namespace nano;
static size_t allocation_count=0;static bool count_allocations=false;
void* operator new(std::size_t n){if(auto p=std::malloc(n?n:1)){if(count_allocations)++allocation_count;return p;}throw std::bad_alloc();}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
static void check(bool ok){if(!ok)throw std::runtime_error("hex encoding differential failed");}
[[gnu::noinline]] static std::string legacy(const Bytes& b){const char* digits="0123456789abcdef";std::string s="0x";for(auto c:b){s+=digits[c>>4];s+=digits[c&15];}return s;}
using Encoder=std::string(*)(const Bytes&);
static size_t allocations(Encoder encode,const Bytes& bytes){allocation_count=0;count_allocations=true;auto encoded=encode(bytes);count_allocations=false;check(encoded.size()==2+2*bytes.size());return allocation_count;}
static double timing(Encoder encode,const Bytes& bytes,unsigned repeats){auto begin=std::chrono::steady_clock::now();size_t sum=0;for(unsigned i=0;i<repeats;++i){auto out=encode(bytes);sum+=out[2+i%(out.size()-2)];}check(sum>0);return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();}
int main(int argc,char** argv){try{
 check(hex({})=="0x");check(hex({0,1,15,16,255})=="0x00010f10ff");unsigned cases=2;
 for(unsigned byte=0;byte<256;++byte)for(size_t length:{size_t(1),size_t(7),size_t(8),size_t(33)}){Bytes input(length,byte);check(hex(input)==legacy(input)&&unhex(hex(input))==input);++cases;}
 Json reports=Json::array();
 for(size_t size:{size_t(32),size_t(4096),size_t(16384),size_t(65536),size_t(1048576)}){
  Bytes input(size);for(size_t i=0;i<size;++i)input[i]=uint8_t(i*37+i/256);
  check(hex(input)==legacy(input)&&unhex(hex(input))==input);++cases;
  const auto old_count=allocations(legacy,input),new_count=allocations(hex,input);check(new_count<=old_count&&new_count<=1);
  if(argc==2&&std::string(argv[1])=="--benchmark"){
   Json windows=Json::array();unsigned repeats=size<100?100000:size<100000?256:16;
   for(unsigned window=0;window<5;++window){double old_ms,new_ms;if(window%2){new_ms=timing(hex,input,repeats);old_ms=timing(legacy,input,repeats);}else{old_ms=timing(legacy,input,repeats);new_ms=timing(hex,input,repeats);}windows.push_back({{"legacyMs",old_ms},{"candidateMs",new_ms}});}
   reports.push_back({{"bytes",size},{"repeats",repeats},{"legacyAllocations",old_count},{"candidateAllocations",new_count},{"windows",windows}});
  }
 }
 std::cout<<Json{{"differentialCases",cases},{"allocationBoundVerified",true},{"microbenchmark",reports},{"endToEndQualification",false}}.dump()<<'\n';return 0;
}catch(const std::exception& e){count_allocations=false;std::cerr<<e.what()<<'\n';return 1;}}
