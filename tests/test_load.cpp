#include "nano/http.hpp"
#include "nano/client.hpp"
#include "support/persistent_rpc.hpp"
#include <thread>
#include <iostream>
#include <algorithm>
#include <atomic>
#include <unistd.h>
using namespace nano;
struct Running {HttpServer& server;std::thread thread;explicit Running(HttpServer& s):server(s),thread([&s]{s.run();}){}~Running(){server.stop();thread.join();}};
static void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
int main(int argc,char** argv){try{
 unsigned workers=argc>1?std::stoul(argv[1]):4,per_worker=argc>2?std::stoul(argv[2]):25;
 unsigned flood_seconds=argc>3?std::stoul(argv[3]):0;require(flood_seconds<=60,"local flood duration must not exceed 60 seconds");
 require(workers>0&&workers<=32&&per_worker>0&&per_worker<=90,"bounded local load: 1..32 workers, 1..90 transactions each");
 auto root=std::filesystem::temp_directory_path()/("nano-load-"+std::to_string(getpid()));
 struct Cleanup{std::filesystem::path p;~Cleanup(){std::filesystem::remove_all(p);}} cleanup{root};
 Json alloc=Json::object();std::vector<Bytes> keys;std::vector<std::vector<SignedTransaction>> valid(workers),foreign(workers);
 auto recipient=public_address(public_key(bytes(Int(99),32)));
 for(unsigned w=0;w<workers;++w){auto key=bytes(Int(w+1),32);keys.push_back(key);alloc[public_address(public_key(key))]={{"balance",quantity(Int("1000000000000000000000"))}};
  for(unsigned n=0;n<per_worker;++n){Transaction tx;tx.chain_id=1337;tx.type=2;tx.max_fee=2000000000;tx.priority_fee=1000000000;tx.nonce=n;tx.to=recipient;tx.value=1;valid[w].push_back(SignedTransaction::sign(tx,key));tx.chain_id=7331;foreign[w].push_back(SignedTransaction::sign(tx,key));}
 }
 Json specification={{"config",{{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bootstrapValidator",public_address(public_key(keys[0]))}}},{"alloc",alloc}};
 std::string fork="NanoExperimental";if(const char* configured=std::getenv("NANO_TEST_EVM_FORK")){fork=configured;require(fork=="Paris"||fork=="Shanghai","unsupported load fixture fork");specification["config"]["evmFork"]=fork;specification["gasLimit"]="0x200b20";}
 auto genesis=Genesis::from_json(specification);
 std::vector<double> latencies;std::mutex results;std::vector<std::exception_ptr> failures(workers);Bytes final_root;Json report;auto begin=std::chrono::steady_clock::now();
 {Blockchain chain(root/"node",genesis),peer(root/"peer",genesis);Rpc rpc(chain);HttpServer server(rpc,"127.0.0.1",0);Running running(server);
  std::string url="http://127.0.0.1:"+std::to_string(server.port());std::vector<std::thread> clients;
  std::atomic<uint64_t> flood_rejections=0,flood_reads=0,admission_backoffs=0,backoff_reads=0,submission_attempts=0,worker_reads=0;
  std::vector<std::exception_ptr> flood_failures(workers);
  auto flood_deadline=std::chrono::steady_clock::now()+std::chrono::seconds(flood_seconds);
  std::vector<std::jthread> attackers;
  if(flood_seconds)for(unsigned w=0;w<workers;++w)attackers.emplace_back([&,w](std::stop_token stop){try{
   PersistentRpc client(server.port());
   while(!stop.stop_requested()&&std::chrono::steady_clock::now()<flood_deadline){
    bool rejected=false;std::string rejection;try{client.call("eth_sendRawTransaction",Json::array({hex(foreign[w][0].raw())}));}catch(const std::exception& e){rejection=e.what();rejected=rejection.find("wrong chain ID")!=std::string::npos;}
    if(!rejected)throw std::runtime_error("continuous flood unexpected response: "+rejection);++flood_rejections;
    require(client.call("eth_chainId")=="0x539","healthy read starved during continuous flood");++flood_reads;
   }
  }catch(...){flood_failures[w]=std::current_exception();}});
  begin=std::chrono::steady_clock::now();
  for(unsigned w=0;w<workers;++w)clients.emplace_back([&,w]{try{PersistentRpc client(server.port());std::vector<double> samples;
   auto submit=[&](const SignedTransaction& tx){for(;;){try{++submission_attempts;return client.call("eth_sendRawTransaction",Json::array({hex(tx.raw())}));}catch(const PersistentRpcError& error){if(error.code!=-32005)throw;require(std::chrono::steady_clock::now()-begin<std::chrono::seconds(60),"admission backoff deadline exceeded");++admission_backoffs;require(client.call("eth_chainId")=="0x539","identity read unavailable during admission backoff");++backoff_reads;std::this_thread::sleep_for(std::chrono::milliseconds(20+7*w));}}};
   for(unsigned n=0;n<per_worker;++n){auto start=std::chrono::steady_clock::now();
    auto hash=submit(valid[w][n]);require(hash==hex(valid[w][n].hash()),"load transaction hash mismatch");
    samples.push_back(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());
    bool rejected=false;try{submit(foreign[w][n]);}catch(const std::exception& e){rejected=std::string(e.what()).find("wrong chain ID")!=std::string::npos;}require(rejected,"foreign transaction not explicitly rejected under load");
    require(client.call("eth_chainId")=="0x539","RPC identity unavailable during mixed load");++worker_reads;
   }
   std::lock_guard lock(results);latencies.insert(latencies.end(),samples.begin(),samples.end());
  }catch(...){failures[w]=std::current_exception();}});
  for(auto& thread:clients)thread.join();for(const auto& error:failures)if(error)std::rethrow_exception(error);
  auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();unsigned total=workers*per_worker;
  require(submission_attempts==2*total+admission_backoffs,"submission attempt accounting mismatch");
  require(worker_reads==total&&backoff_reads==admission_backoffs,"worker read accounting mismatch");
  require(chain.pending_count()==total,"mixed load corrupts pending transaction count");unsigned committed=0;auto execution_start=std::chrono::steady_clock::now();
  while(chain.pending_count()){auto time=Int(chain.tip().header.timestamp+12);auto block=chain.validate_signed(keys[0],time);require(!block.transactions.empty(),"load queue cannot drain");peer.import_block(block,time);committed+=block.transactions.size();require(chain.state().root()==peer.state().root(),"peer diverges during load drain");}
  auto execution_seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-execution_start).count();
  if(fork!="NanoExperimental")require(chain.tip().header.number>=4,"profile load must span multiple blocks");
  require(committed==total&&chain.state().get(recipient).balance==total,"load lost or duplicated funds");
  for(const auto& group:valid)for(const auto& tx:group){auto receipt=chain.store().receipt(tx.hash());require(receipt&&receipt->success&&peer.store().receipt(tx.hash())->to_json()==receipt->to_json(),"load receipt missing or divergent");}
  require(chain.audit().at("verified")==true&&peer.audit().at("verified")==true,"history audit fails after load");final_root=chain.state().root();
  RpcClient recovery({url});require(recovery.call("eth_blockNumber")==quantity(chain.tip().header.number),"RPC does not recover after load");
  // Keep attack workers active through block execution, replay and audit.
  for(auto& attacker:attackers)attacker.join();
  for(const auto& error:flood_failures)if(error)std::rethrow_exception(error);
  require(chain.pending_count()==0&&chain.state().root()==final_root,"continuous attack changes drained state");
  std::sort(latencies.begin(),latencies.end());report={{"fork",fork},{"mode","concurrent admission then signed block drain and direct peer replay"},{"bftNetworkTest",false},{"workers",workers},{"floodSeconds",flood_seconds},{"floodRejections",flood_rejections.load()},{"healthyReadsDuringFlood",flood_reads.load()},{"admissionBackoffs",admission_backoffs.load()},{"healthyReadsDuringBackoff",backoff_reads.load()},{"validTransactions",total},{"rejectedForeignTransactions",total},{"readRequests",worker_reads.load()+backoff_reads.load()},{"submissionAttempts",submission_attempts.load()},{"workerHttpRequests",submission_attempts.load()+worker_reads.load()+backoff_reads.load()},{"workerHttpRequestsPerSecond",(submission_attempts.load()+worker_reads.load()+backoff_reads.load())/elapsed},{"nominalOperationsPerSecond",3*total/elapsed},{"acceptedTransactionsPerSecond",total/elapsed},{"admissionSeconds",elapsed},{"rateScope","mixed workers only; excludes optional flood, post-load recovery and direct block replay"},{"admissionP50Ms",latencies[latencies.size()/2]},{"admissionP95Ms",latencies[(latencies.size()-1)*95/100]},{"drainAndPeerReplaySeconds",execution_seconds},{"blocks",quantity(chain.tip().header.number)},{"auditVerified",true}};
 }
 {Blockchain reopened(root/"node",genesis);require(reopened.state().root()==final_root,"load state lost after reopen");}
 report["reopenVerified"]=true;std::cout<<report.dump(2)<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
