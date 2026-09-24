#include "nano/precompiles.hpp"
#include <iostream>
using namespace nano;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try{
 struct Case{unsigned address;size_t length;uint64_t gas;size_t output;};
 for(const auto& test:{Case{11,256,375,128},Case{12,160,12000,128},Case{13,512,600,256},Case{14,288,22500,256},Case{15,384,70300,32},Case{16,64,5500,128},Case{17,128,23800,256}}){
  Bytes input(test.length);auto valid=precompile(test.address,input,test.gas);
  check(valid.success&&valid.gas_used==test.gas&&valid.return_data.size()==test.output,"BLS zero/infinity valid input failed");
  if(test.address==15)check(integer(valid.return_data)==1,"Infinity pairing must be true");
  else if(test.address<16)check(integer(valid.return_data)==0,"Infinity result differs");
  auto insufficient=precompile(test.address,input,test.gas-1);
  check(!insufficient.success&&insufficient.gas_used==test.gas-1&&insufficient.return_data.empty(),"BLS insufficient gas failed");
  auto invalid=input;invalid[0]=1;auto noncanonical=precompile(test.address,invalid,test.gas+123);
  check(!noncanonical.success&&noncanonical.gas_used==test.gas+123&&noncanonical.return_data.empty(),"BLS noncanonical field did not burn call budget");
  invalid=input;invalid.pop_back();auto malformed=precompile(test.address,invalid,test.gas+123);
  check(!malformed.success&&malformed.gas_used==test.gas+123,"BLS malformed length accepted");
  State state;ExecutionContext context;context.address=address(hex(bytes(Int(test.address),20)));context.data=input;context.gas=test.gas;context.revision=EvmRevision::Cancun;
  auto old=EVM(state).execute(context);check(old.success&&old.return_data.empty()&&old.gas_used==0,"BLS activated before Prague");
  context.revision=EvmRevision::Prague;auto active=EVM(state).execute(context);
  check(active.success&&active.return_data==valid.return_data&&active.gas_used==test.gas,"BLS Prague dispatch differs");
 }
 for(unsigned number:{12u,14u,15u}){auto empty=precompile(number,{},100000);check(!empty.success&&empty.gas_used==100000,"Empty variable BLS input accepted");}
 std::cout<<"BLS encodings, infinity, exact gas, malformed inputs and fork activation passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
