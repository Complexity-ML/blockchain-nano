#include "nano/consensus_signing.hpp"
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <algorithm>
#include <atomic>
#include <barrier>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>
using namespace nano;
ConsensusRoot reference(const ConsensusRoot& object,const ConsensusRoot& domain){
 std::array<unsigned char,64> data{};std::copy(object.begin(),object.end(),data.begin());std::copy(domain.begin(),domain.end(),data.begin()+32);
 ConsensusRoot expected{};if(!SHA256(data.data(),data.size(),expected.data()))throw std::runtime_error("reference SHA256 failed");return expected;
}
int main(int argc,char** argv){try{
 if(argc==2&&std::string(argv[1])=="--provider-unavailable"){
  if(EVP_set_default_properties(nullptr,"provider=nano-unavailable")!=1)throw std::runtime_error("cannot set failure fixture");
  bool refused=false;try{consensus_signing_root({},{});}catch(const std::runtime_error&){refused=true;}
  if(EVP_set_default_properties(nullptr,"")!=1||!refused)throw std::runtime_error("unavailable provider was not refused");
  if(consensus_signing_root({},{})!=reference({},{}))throw std::runtime_error("initialization did not recover after failure");
  std::cout<<"providerFailureRejected=true initializationRetried=true\n";return 0;
 }
 if(argc!=1)throw std::runtime_error("unknown arguments");
 constexpr unsigned workers=16,per_worker=256;std::barrier start(workers);std::atomic<unsigned> failures=0;std::vector<std::jthread> threads;
 for(unsigned worker=0;worker<workers;++worker)threads.emplace_back([&,worker]{start.arrive_and_wait();try{
  for(unsigned n=0;n<per_worker;++n){ConsensusRoot a{},b{};const auto seed=worker*per_worker+n;
   for(unsigned i=0;i<32;++i){a[i]=static_cast<unsigned char>((seed*37+i*131)^(seed>>3));b[i]=static_cast<unsigned char>((seed*113+i*17)^(seed>>5));}
   if(consensus_signing_root(a,b)!=reference(a,b))++failures;
  }
 }catch(...){++failures;}});
 threads.clear();if(failures)throw std::runtime_error("concurrent signing roots differ");
 ConsensusRoot ones{};ones.fill(255);
 if(consensus_signing_root({},{})!=reference({},{})||consensus_signing_root(ones,ones)!=reference(ones,ones))throw std::runtime_error("boundary roots differ");
 std::cout<<"parallelWorkers=16 independentDigestComparisons=4098 concurrentFirstUse=true\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
