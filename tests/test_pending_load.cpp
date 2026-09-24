#include "nano/http.hpp"
#include "support/persistent_rpc.hpp"
#include <thread>
#include <atomic>
#include <algorithm>
#include <iostream>
#include <set>
#include <unistd.h>
using namespace nano;
static void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
struct Running {HttpServer& server;std::thread thread;explicit Running(HttpServer& s):server(s),thread([&s]{s.run();}){}~Running(){server.stop();thread.join();}};
int main(){try{
 const std::string fork=std::getenv("NANO_TEST_EVM_FORK")?std::getenv("NANO_TEST_EVM_FORK"):"Shanghai";
 require(fork=="Paris"||fork=="Shanghai","unsupported fork");
 const auto key=bytes(Int(1),32);const auto validator=public_address(public_key(key));
 const auto recipient=public_address(public_key(bytes(Int(500),32)));
 const std::string probe="0x00000000000000000000000000000000000040ad";
 auto path=std::filesystem::temp_directory_path()/("nano-pending-load-"+std::to_string(getpid()));
 struct Cleanup{std::filesystem::path path;~Cleanup(){std::filesystem::remove_all(path);}}cleanup{path};
 Json alloc={{validator,{{"balance",quantity(Int("1000000000000000000000000"))}}},{probe,{{"code","0x4360005260206000f3"}}}};
 std::vector<Bytes> keys;for(unsigned i=10;i<110;++i){keys.push_back(bytes(Int(i),32));alloc[public_address(public_key(keys.back()))]={{"balance",quantity(Int("1000000000000000000000000"))}};}
 auto genesis=Genesis::from_json({{"gasLimit",quantity(Int(2100000))},{"config",{{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bftVersion",1},{"bootstrapValidator",validator},{"evmFork",fork}}},{"alloc",alloc}});
 Bytes final_root;Json report;
 {Blockchain chain(path/"node",genesis);std::set<std::string> admitted;
  for(const auto& sender_key:keys)for(unsigned nonce=0;nonce<100;++nonce){Transaction tx;tx.to=recipient;tx.nonce=nonce;tx.value=1;tx.gas_price=2000000000;auto signed_tx=SignedTransaction::sign(tx,sender_key);require(chain.submit(signed_tx),"full pool admission failed");admitted.insert(hex(signed_tx.hash()));}
  require(chain.pending_count()==10000&&admitted.size()==10000,"full pool fixture missing transactions");
  auto root=chain.state().root(),tip=chain.tip().header.hash();auto cursor=chain.event_cursor();
  Rpc rpc(chain);HttpServer server(rpc,"127.0.0.1",0);Running running(server);
  Json invocation={{"from",validator},{"to",probe}};
  std::vector<double> durations;std::mutex samples;
  auto phase=[&](unsigned rounds,bool immutable){std::vector<std::thread> workers;std::vector<std::exception_ptr> errors(4);
   for(unsigned w=0;w<4;++w)workers.emplace_back([&,w]{try{PersistentRpc client(server.port());for(unsigned i=0;i<rounds;++i){
    auto start=std::chrono::steady_clock::now();auto estimated=client.call("eth_estimateGas",Json::array({invocation,"pending"}));require(quantity(estimated)==21017,"pending estimate disagrees with fixed probe cost");
    auto value=integer(unhex(client.call("eth_call",Json::array({invocation,"pending"})).get<std::string>()));require(immutable?value==1:value>0,"pending call has wrong block number");
    require(client.call("eth_chainId")=="0x539","identity read failed during pending simulations");
    std::lock_guard lock(samples);durations.push_back(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());
   }}catch(...){errors[w]=std::current_exception();}});
   for(auto& worker:workers)worker.join();for(const auto& error:errors)if(error)std::rethrow_exception(error);
  };
  auto began=std::chrono::steady_clock::now();phase(1,true);
  require(chain.pending_count()==10000&&chain.state().root()==root&&chain.tip().header.hash()==tip&&chain.event_cursor()==cursor,"read-only full pool simulations changed canonical state/pool/events");
  auto readonly_seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-began).count();
  // The HTTP server's own timer drives this single-validator BFT reactor.
  setenv("NANO_PENDING_LOAD_BFT_KEY",hex(key).c_str(),1);
  rpc.load_wallets({{"bft",{{"key_env","NANO_PENDING_LOAD_BFT_KEY"},{"journal",(path/"votes").string()}}}},"127.0.0.1");
  phase(2,false);auto last=chain.tip();require(last.header.number>0,"BFT timer made no progress during bounded pending RPC workload");
  // Stop signing before the following exact inventory and audit checks.
  rpc.load_wallets(Json::object(),"127.0.0.1");last=chain.tip();
  std::set<std::string> committed;size_t certificates=0;
  for(Int number=1;number<=last.header.number;++number){auto block=chain.store().block_by_number(number);require(bool(block),"committed block missing");auto cert=chain.store().certificate_at(number);require(cert&&cert->votes.size()==1,"single-bootstrap BFT certificate missing");++certificates;
   for(const auto& tx:block->transactions){auto hash=hex(tx.hash());require(admitted.contains(hash)&&committed.insert(hash).second,"unexpected/duplicate committed transaction");auto receipt=chain.store().receipt(tx.hash());require(receipt&&receipt->success,"committed transaction failed");}}
  require(!committed.empty(),"no mempool transactions committed during simulation workload");
  auto content=chain.pool_info(true);std::set<std::string> pending;
  for(const char* group:{"pending","queued"})for(const auto& [sender,entries]:content.at(group).items())for(const auto& [nonce,tx]:entries.items()){auto hash=tx.at("hash").get<std::string>();require(admitted.contains(hash)&&!committed.contains(hash)&&pending.insert(hash).second,"pool/committed overlap or unexpected hash");}
  require(pending.size()+committed.size()==admitted.size()&&pending.size()==chain.pending_count(),"pending simulations lost or duplicated an admitted transaction");
  require(chain.state().get(recipient).balance==Int(committed.size()),"recipient balance differs from committed transfer count");
  require(chain.audit().at("verified")==true,"post-workload certified history audit failed");final_root=chain.state().root();
  std::sort(durations.begin(),durations.end());report={{"scenario","rpc-pending-full-pool"},{"fork",fork},{"poolInitially",10000},{"workers",4},{"estimateCalls",12},{"pendingCalls",12},{"identityReads",12},{"fullPoolReadonlySeconds",readonly_seconds},{"requestTripletP50Ms",durations[durations.size()/2]},{"requestTripletMaxMs",durations.back()},{"committed",committed.size()},{"remainingPool",pending.size()},{"certifiedBlocks",certificates},{"auditVerified",true},{"singleBootstrapValidator",true},{"distributedBft",false}};
 }
 {Blockchain reopened(path/"node",genesis);require(reopened.state().root()==final_root&&reopened.audit().at("verified")==true,"certified state failed reopen audit");}
 report["reopenVerified"]=true;std::cout<<report.dump(2)<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
