#include "nano/rpc.hpp"
#include "nano/staking.hpp"
#include <iostream>
#include <unistd.h>
using namespace nano;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try{
 auto key=bytes(Int(1),32);auto sender=public_address(public_key(key));std::string contract="0x0000000000000000000000000000000000002000";
 for(auto revision:{EvmRevision::NanoExperimental,EvmRevision::Berlin,EvmRevision::Paris,EvmRevision::Shanghai,EvmRevision::Cancun,EvmRevision::Prague,EvmRevision::Osaka}){
  State state;state.get(sender).balance=1000000;state.get(contract).code=unhex("4460005500");
  auto query_state=state;query_state.get(contract).code=unhex("4460005260206000f3");
  auto query_before=query_state.to_json();Header query_header;query_header.difficulty=7;query_header.mix_hash=bytes(Int(123),32);
  check(staking_contract_stake(query_state,query_header,contract,sender,1337,{},revision)==(revision>=EvmRevision::Paris?123:7),"Staking query DIFFICULTY/PREVRANDAO differs by fork");
  check(query_state.to_json()==query_before,"Staking randomness query mutated state");
  Transaction tx;tx.chain_id=1337;tx.to=contract;tx.gas=100000;tx.gas_price=1;
  Header h;h.base_fee=0;h.difficulty=7;h.mix_hash=bytes(Int(123),32);
  auto receipt=ExecutionEngine(1337,{},revision).apply_transaction(state,SignedTransaction::sign(tx,key),h,0,0);
  check(receipt.success&&state.get_storage(contract,0)==(revision>=EvmRevision::Paris?123:7),"Transaction DIFFICULTY/PREVRANDAO differs by fork");
 }
 for(const auto& fork:{std::string("NanoExperimental"),std::string("Paris"),std::string("Shanghai")}){
  Json config={{"chainId",1337}};if(fork!="NanoExperimental")config["evmFork"]=fork;
  auto genesis=Genesis::from_json({{"config",config},{"alloc",{{sender,{{"balance","0x100000000000000000"}}},{contract,{{"code","0x4460005260206000f3"}}}}}});
  genesis.block.header.difficulty=7;genesis.block.header.mix_hash=bytes(Int(123),32);
  auto path=std::filesystem::temp_directory_path()/("nano-prevrandao-"+std::to_string(getpid())+"-"+fork);
  {Blockchain chain(path,genesis);Rpc rpc(chain);auto before=chain.state().to_json();auto result=rpc.handle({{"jsonrpc","2.0"},{"id",1},{"method","eth_call"},{"params",Json::array({Json{{"from",sender},{"to",contract}},"latest"})}});
   check(!result.contains("error")&&result.at("result").get<std::string>()==hex(bytes(Int(fork=="NanoExperimental"?7:123),32)),"RPC DIFFICULTY/PREVRANDAO differs by fork");check(chain.state().to_json()==before,"RPC randomness probe mutated state");}
  std::filesystem::remove_all(path);
 }
 std::cout<<"PREVRANDAO uses mixHash after Paris, legacy difficulty before; signed execution and RPC agree\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
