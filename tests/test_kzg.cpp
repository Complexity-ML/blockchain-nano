#include "nano/precompiles.hpp"
#include <future>
#include <iostream>
using namespace nano;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try{
 // The zero polynomial has infinity commitments and proofs; this is a valid case.
 Bytes infinity(48,0);infinity[0]=0xc0;auto hash=sha256(infinity);hash[0]=1;
 Bytes input=hash;input.resize(96);input=concat(concat(input,infinity),infinity);
 std::vector<std::future<ExecutionResult>> jobs;
 for(unsigned i=0;i<4;++i)jobs.push_back(std::async(std::launch::async,[input]{return precompile(10,input,50000);}));
 const Int modulus("52435875175126190479447740508185965837690552500527637822603658699938581184513");
 auto expected=concat(bytes(Int(4096),32),bytes(modulus,32));
 for(auto& job:jobs){auto result=job.get();check(result.success&&result.gas_used==50000&&result.return_data==expected,"Concurrent KZG proof verification failed");}
 auto failed=[&](Bytes invalid,uint64_t gas){auto result=precompile(10,invalid,gas);check(!result.success&&result.gas_used==gas&&result.return_data.empty(),"Invalid KZG proof did not consume its call budget");};
 failed(input,49999);auto invalid=input;invalid.pop_back();failed(invalid,60000);
 invalid=input;invalid[0]=2;failed(invalid,60000);
 invalid=input;auto noncanonical=bytes(modulus,32);std::copy(noncanonical.begin(),noncanonical.end(),invalid.begin()+32);failed(invalid,60000);
 invalid=input;invalid.back()=1;failed(invalid,60000);
 for(auto revision:{EvmRevision::Paris,EvmRevision::Shanghai,EvmRevision::Cancun,EvmRevision::NanoExperimental}){
  State state;ExecutionContext ctx;ctx.revision=revision;ctx.address=address("0x000000000000000000000000000000000000000a");ctx.data=input;ctx.gas=60000;
  auto result=EVM(state).execute(ctx);
  if(revision==EvmRevision::Cancun)check(result.success&&result.return_data==expected&&result.gas_used==50000,"Cancun KZG dispatch failed");
  else if(revision==EvmRevision::NanoExperimental)check(!result.success&&result.gas_used==60000,"Experimental precompile behavior changed");
  else check(result.success&&result.return_data.empty()&&result.gas_used==0,"KZG activated before Cancun");
 }
 std::cout<<"KZG concurrent initialization, proof validation, gas and fork activation passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
