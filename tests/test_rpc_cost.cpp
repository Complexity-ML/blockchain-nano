#include "nano/http.hpp"
#include "support/persistent_rpc.hpp"
#include <thread>
#include <barrier>
#include <iostream>
#include <unistd.h>
using namespace nano;
static void require(bool v,const char* message){if(!v)throw std::runtime_error(message);}
struct Running {HttpServer& server;std::thread thread;explicit Running(HttpServer& s):server(s),thread([&s]{s.run();}){}~Running(){server.stop();thread.join();}};
int main(){try{
 std::string fork=std::getenv("NANO_TEST_EVM_FORK")?std::getenv("NANO_TEST_EVM_FORK"):"Shanghai";require(fork=="Paris"||fork=="Shanghai","unsupported fork");
 auto key=bytes(Int(1),32),sender_key=bytes(Int(10),32);auto validator=public_address(public_key(key)),sender=public_address(public_key(sender_key)),recipient=public_address(public_key(bytes(Int(99),32)));
 const unsigned depth=std::getenv("NANO_RPC_COST_DEPTH")?std::stoul(std::getenv("NANO_RPC_COST_DEPTH")):0;require(depth<=4,"bounded nested depth exceeded");
 const bool successful=std::getenv("NANO_RPC_COST_SUCCESS")!=nullptr;
 const std::string probe="0x00000000000000000000000000000000000040ad";
 // Write once, then consume all gas with JUMPDEST/PUSH/JUMP. Simulation must discard the write.
 Json specification={{"config",{{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bftVersion",1},{"bootstrapValidator",validator},{"evmFork",fork}}},{"alloc",{{validator,{{"balance",quantity(Int("1000000000000000000000000"))}}},{sender,{{"balance",quantity(Int("1000000000000000000000000"))}}},{probe,{{"code",successful?"0x60016000556000355b8015601557600190036008565b50602a60005260206000f3":"0x60016000555b600556"}}}}}};
 std::string target=probe;
 for(unsigned level=1;level<=depth;++level){auto outer=hex(bytes(Int(0x40ad+level),20));specification["alloc"][outer]={{"code","0x3660006000376020600060206000600073"+target.substr(2)+"5af1602f5760006000fd5b60206000f3"}};target=outer;}
 auto genesis=Genesis::from_json(specification);
 auto path=std::filesystem::temp_directory_path()/("nano-rpc-cost-"+std::to_string(getpid()));struct Cleanup{std::filesystem::path p;~Cleanup(){std::filesystem::remove_all(p);}}cleanup{path};
 Json report;Bytes final_root;
 {Blockchain chain(path/"node",genesis);Rpc rpc(chain);Json args={{"from",sender},{"to",target},{"gas",quantity(genesis.block.header.gas_limit)}};
  if(successful)args["data"]=hex(bytes(Int(600000),32));
  auto before=chain.state().root();auto start=std::chrono::steady_clock::now();
  auto calibration=rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method","eth_call"},{"params",Json::array({args,"latest"})}});
  double single_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
  require(successful?calibration.value("result",Json())==hex(bytes(Int(42),32)):calibration.contains("error")&&calibration["error"]["code"]==3&&calibration["error"]["message"].get<std::string>().find("Out of gas")!=std::string::npos,"cost probe has unexpected execution result");
  require(chain.state().root()==before&&chain.state().get_storage(probe,0)==0,"calibration simulation wrote canonical state");
  std::cout<<Json{{"stage","calibrated"},{"successfulProbe",successful},{"depth",depth},{"fork",fork},{"gas",quantity(genesis.block.header.gas_limit)},{"singleCallMs",single_ms}}.dump()<<std::endl;
  Transaction tx;tx.to=recipient;tx.value=1;tx.gas_price=2000000000;auto signed_tx=SignedTransaction::sign(tx,sender_key);require(chain.submit(signed_tx),"fixture transfer rejected");
  setenv("NANO_RPC_COST_BFT_KEY",hex(key).c_str(),1);rpc.load_wallets({{"bft",{{"key_env","NANO_RPC_COST_BFT_KEY"},{"journal",(path/"votes").string()}}}},"127.0.0.1");
  HttpServer server(rpc,"127.0.0.1",0);Running running(server);std::barrier gate(5);std::vector<std::thread> workers;std::vector<Json> samples(4);std::vector<std::string> failures(4);
  for(unsigned w=0;w<4;++w)workers.emplace_back([&,w]{gate.arrive_and_wait();try{PersistentRpc client(server.port());const char* method=w%2?"eth_estimateGas":"eth_call";const char* tag=w<2?"latest":"pending";auto begin=std::chrono::steady_clock::now();bool rejected=false;
   Json response;
   try{response=client.call(method,Json::array({args,tag}));}catch(const PersistentRpcError& e){rejected=e.code==3&&std::string(e.what()).find("Out of gas")!=std::string::npos;}
   if(successful){require(!rejected&&!response.is_null(),"successful expensive simulation failed");if(w%2){auto estimate=quantity(response);require(estimate>23000000&&estimate<=30000000,"expensive estimate outside expected gas range");auto trial=args;trial["gas"]=response;require(client.call("eth_call",Json::array({trial,tag}))==hex(bytes(Int(42),32)),"estimated gas cannot execute expensive contract");}else require(response==hex(bytes(Int(42),32)),"expensive call returns wrong value");}
   else require(rejected,"expensive simulation returned wrong result");require(client.call("eth_chainId")=="0x539","identity read failed after expensive simulation");auto height=quantity(client.call("eth_blockNumber"));
   samples[w]={{"method",method},{"tag",tag},{"response",response},{"durationMs",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count()},{"heightDuringWorkload",quantity(height)}};
  }catch(const std::exception& e){failures[w]=e.what();}});
  gate.arrive_and_wait();for(auto& worker:workers)worker.join();
  Json signed_probe=nullptr;
  if(successful&&std::all_of(failures.begin(),failures.end(),[](const auto& v){return v.empty();})){
   PersistentRpc client(server.port());
   require(quantity(client.call("eth_getStorageAt",Json::array({probe,"0x0","latest"})))==0,"expensive simulations persisted a storage write");
   Transaction execution;execution.nonce=1;execution.to=target;execution.data=bytes(Int(600000),32);execution.gas=quantity(samples[3]["response"]);execution.gas_price=2000000000;
   auto signed_execution=SignedTransaction::sign(execution,sender_key);
   auto hash=client.call("eth_sendRawTransaction",Json::array({hex(signed_execution.raw())}));require(hash==hex(signed_execution.hash()),"signed estimate transaction hash mismatch");
   for(unsigned poll=0;poll<200;++poll){signed_probe=client.call("eth_getTransactionReceipt",Json::array({hash}));if(!signed_probe.is_null())break;std::this_thread::sleep_for(std::chrono::milliseconds(100));}
   require(!signed_probe.is_null()&&signed_probe["status"]=="0x1","transaction at pending estimate failed to finalize");
   require(quantity(signed_probe["gasUsed"])<=execution.gas,"receipt gas exceeds estimated allowance");
   require(quantity(client.call("eth_getStorageAt",Json::array({probe,"0x0","latest"})))==1,"signed estimate transaction did not persist storage");
   require(quantity(client.call("eth_getTransactionCount",Json::array({sender,"latest"})))==2,"signed estimate transaction nonce mismatch");
  }
  rpc.load_wallets(Json::object(),"127.0.0.1");auto last=chain.tip();auto receipt=chain.store().receipt(signed_tx.hash());
  bool progressed=false;for(const auto& sample:samples)if(sample.is_object()&&quantity(sample["heightDuringWorkload"])>0)progressed=true;
  bool no_errors=std::all_of(failures.begin(),failures.end(),[](const auto& v){return v.empty();});
  report={{"scenario","rpc-expensive-simulation"},{"successfulProbe",successful},{"depth",depth},{"fork",fork},{"workers",4},{"gasPerSimulation",quantity(genesis.block.header.gas_limit)},{"singleCallMs",single_ms},{"samples",samples},{"failures",failures},{"bftProgressObserved",progressed},{"transferCommitted",receipt&&receipt->success},{"signedEstimatedTransaction",signed_probe},{"probeStorageMatchesExecution",chain.state().get_storage(probe,0)==(successful?1:0)},{"singleBootstrapValidator",true}};
  std::cout<<report.dump()<<std::endl;
  require(no_errors,"expensive RPC workload exceeded transport deadline or returned wrong result");require(progressed&&receipt&&receipt->success,"BFT failed to progress during expensive RPC workload");
  require(chain.state().get_storage(probe,0)==(successful?1:0)&&chain.state().get(recipient).balance==1&&chain.pending_count()==0,"simulation changed canonical state or transfer accounting");
  if(successful)require(chain.store().certificate_at(quantity(signed_probe["blockNumber"])).has_value(),"estimated execution block not certified");
  require(chain.store().certificate_at(receipt->block_number).has_value(),"transfer block not certified");require(chain.audit().at("verified")==true,"history audit failed");final_root=chain.state().root();
 }
 {Blockchain reopened(path/"node",genesis);require(reopened.state().root()==final_root&&reopened.audit().at("verified")==true,"reopen audit failed");}
 std::cout<<"Expensive simulations, single-validator progression and reopen audit passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
