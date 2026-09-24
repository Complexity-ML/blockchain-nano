#include "nano/engine.hpp"
#include <pthread.h>
#include <fstream>
#include <iostream>
using namespace nano;
struct Job { Json fixture; std::exception_ptr error; };
void* execute(void* opaque){
 auto& job=*static_cast<Job*>(opaque);
 try{
  // Use the complete official fixture, including its signed transaction and root.
  for(const auto& [name,test]:job.fixture.items()){
   const auto& env=test.at("env");Header header;
   header.coinbase=address(env.at("currentCoinbase"));header.gas_limit=quantity(env.at("currentGasLimit"));
   header.number=quantity(env.at("currentNumber"));header.timestamp=quantity(env.at("currentTimestamp"));
   header.base_fee=quantity(env.at("currentBaseFee"));header.difficulty=quantity(env.at("currentRandom"));
   for(const auto& expected:test.at("post").at("Cancun")){
    // A second execution checks that coroutine ownership survives full teardown.
    for(int repeat=0;repeat<2;++repeat){
     State state=State::from_json(test.at("pre"));
     auto tx=SignedTransaction::decode(unhex(expected.at("txbytes")));
     auto receipt=ExecutionEngine(Int(1),{},EvmRevision::Cancun,ReplayPolicy::AllowLegacyUnprotected).apply_transaction(state,tx,header,0,0);
     if(hex(state.root())!=expected.at("hash").get<std::string>()||hex(receipt.encode())!=expected.at("receipt").at("rlp").get<std::string>())throw std::runtime_error("Deep-call fixture diverged");
    }
   }
  }
 }catch(...){job.error=std::current_exception();}
 return nullptr;
}
int main(int argc,char** argv){
 try{
  if(argc!=2)throw std::runtime_error("Expected official fixture path");
  Job job;std::ifstream file(argv[1]);file>>job.fixture;
  pthread_attr_t attr;if(pthread_attr_init(&attr))throw std::runtime_error("pthread_attr_init failed");
  if(pthread_attr_setstacksize(&attr,256*1024)){pthread_attr_destroy(&attr);throw std::runtime_error("Cannot set 256 KiB stack");}
  pthread_t worker;auto error=pthread_create(&worker,&attr,execute,&job);pthread_attr_destroy(&attr);
  if(error)throw std::runtime_error("pthread_create failed");
  if(pthread_join(worker,nullptr))throw std::runtime_error("pthread_join failed");
  if(job.error)std::rethrow_exception(job.error);
  std::cout<<"Official deep-call fixture matches twice on a 256 KiB native stack\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
