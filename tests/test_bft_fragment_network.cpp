#include "support/process.hpp"
#include "nano/http.hpp"
using namespace test;
struct Running {HttpServer& server;std::thread thread;explicit Running(HttpServer& s):server(s),thread([&s]{s.run();}){}~Running(){server.stop();thread.join();}};
int main(int argc,char** argv){try{
 if(argc==7&&std::string(argv[1])=="--core"){
  Blockchain chain(argv[3],Genesis::from_json(load(argv[2])));for(const auto& raw:load(argv[4]))check(chain.submit(SignedTransaction::decode(unhex(raw.get<std::string>()))),"child preloads signed transaction");
  Rpc rpc(chain,argv[6]);rpc.load_wallets({{"bft",{{"key_env","NANO_BFT_FRAGMENT_NETWORK_KEY"},{"journal",argv[5]}}}},"127.0.0.1");HttpServer server(rpc,"127.0.0.1",0);
  std::cout<<"Nano fixture RPC listening on 127.0.0.1:"<<server.port()<<std::endl;std::cout<<Json{{"event","core-process"},{"pid",getpid()}}.dump()<<std::endl;server.run();return 0;
 }
 check(argc==2||(argc==3&&(std::string(argv[2])=="--processes"||std::string(argv[2])=="--crash-download")),"relay binary and optional --processes required");const bool processes=argc==3;const bool crash_download=processes&&std::string(argv[2])=="--crash-download";Temp root;
 Json staking;std::ifstream(std::string(NANO_TEST_SOURCE)+"/contracts/NanoStaking.json")>>staking;
 std::vector<Bytes> keys;std::vector<std::string> addresses;Json allocations=Json::object();
 Int minimum=ProofOfStake::minimum_stake;
 for(unsigned i=1;i<=4;++i){keys.push_back(bytes(Int(i),32));addresses.push_back(public_address(public_key(keys.back())));allocations[addresses.back()]={{"balance",quantity(Int(minimum*10))}};}
 auto registry=create_address(addresses[0],0);
 Json config={{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bootstrapValidator",addresses[0]},{"stakingContract",registry}};
 std::string fork="NanoExperimental";if(const char* configured=std::getenv("NANO_TEST_EVM_FORK")){fork=configured;check(fork=="Paris"||fork=="Shanghai","unsupported network fixture fork");config["evmFork"]=fork;}
 auto seed=Genesis::from_json({{"config",config},{"alloc",allocations}});State registered;
 {Blockchain setup(root/"setup",seed);
  for(unsigned i=0;i<4u;++i){Transaction tx;tx.chain_id=1337;tx.value=minimum;tx.gas=3000000;tx.gas_price=2000000000;
   if(i==0)tx.data=unhex(staking.at("bytecode"));else{tx.to=registry;tx.data=unhex(staking.at("methodIdentifiers").at("registerValidator()"));}
   auto signed_tx=SignedTransaction::sign(tx,keys[i]);check(setup.submit(signed_tx),"seed validator registration submitted");
   auto selected=setup.next_validator();auto index=std::find_if(addresses.begin(),addresses.end(),[&](const auto& candidate){return address(candidate)==address(*selected);})-addresses.begin();check(index<4,"known setup proposer");setup.validate_signed(keys[index],Int((i+1)*12));
   check(setup.store().receipt(signed_tx.hash())->success,"seed validator registered by executed contract");
  }registered=setup.state();
 }


 config["bftVersion"]=1;auto genesis=Genesis::from_json({{"gasLimit","0x2625a00"},{"config",config},{"alloc",registered.to_json()}});
 std::vector<SignedTransaction> transactions;for(unsigned n=0;n<10;++n){Transaction tx;tx.chain_id=1337;tx.nonce=registered.get(addresses[0]).nonce+n;tx.to="0x00000000000000000000000000000000000000b0";tx.value=1;tx.gas=4000000;tx.gas_price=2000000000;tx.data=Bytes(900000,0);transactions.push_back(SignedTransaction::sign(tx,keys[0]));}
 auto genesis_path=root/"genesis.json",transactions_path=root/"transactions.json";save(genesis_path,{{"gasLimit","0x2625a00"},{"config",config},{"alloc",registered.to_json()}});Json raws=Json::array();for(const auto& tx:transactions)raws.push_back(hex(tx.raw()));save(transactions_path,raws);
 std::vector<std::unique_ptr<Process>> cores;std::set<int> core_pids;
 std::vector<std::unique_ptr<Blockchain>> chains;std::vector<std::unique_ptr<Rpc>> rpcs;std::vector<std::unique_ptr<HttpServer>> servers;std::vector<std::unique_ptr<Running>> runners;std::vector<std::unique_ptr<Process>> relays;std::vector<std::string> seeds;std::vector<int> ports;
 for(unsigned i=0;i<4;++i){
  if(processes){cores.push_back(std::make_unique<Process>(std::vector<std::string>{argv[0],"--core",genesis_path.string(),(root/("node-"+std::to_string(i))).string(),transactions_path.string(),(root/("journal-"+std::to_string(i))).string(),"fragment-token-"+std::to_string(i)},Env{{"NANO_BFT_FRAGMENT_NETWORK_KEY",hex(keys[i])}}));ports.push_back(cores.back()->port());core_pids.insert(cores.back()->event("core-process").at("pid").get<int>());continue;}
  chains.push_back(std::make_unique<Blockchain>(root/("node-"+std::to_string(i)),genesis));for(const auto& tx:transactions)check(chains.back()->submit(tx),"preload large transaction");
  rpcs.push_back(std::make_unique<Rpc>(*chains.back(),"fragment-token-"+std::to_string(i)));setenv("NANO_BFT_FRAGMENT_NETWORK_KEY",hex(keys[i]).c_str(),1);rpcs.back()->load_wallets({{"bft",{{"key_env","NANO_BFT_FRAGMENT_NETWORK_KEY"},{"journal",(root/("journal-"+std::to_string(i))).string()}}}},"127.0.0.1");servers.push_back(std::make_unique<HttpServer>(*rpcs.back(),"127.0.0.1",0));ports.push_back(servers.back()->port());
 }
 for(auto& server:servers)runners.push_back(std::make_unique<Running>(*server));
 for(unsigned i=0;i<4;++i){std::vector<std::string> args={argv[1],std::to_string(ports[i]),(root/("peer-"+std::to_string(i))).string(),"/ip4/127.0.0.1/tcp/0"};args.insert(args.end(),seeds.begin(),seeds.end());relays.push_back(std::make_unique<Process>(args,Env{{"NANO_NETWORK_TOKEN","fragment-token-"+std::to_string(i)},{"NANO_TEST_NETWORK_TRACE","1"}}));seeds.push_back(relays.back()->event("listen").at("address").get<std::string>());relays.back()->event("ready");}
 check(!processes||core_pids.size()==4,"four distinct core process identities");auto began=std::chrono::steady_clock::now();
 Json crash_evidence=nullptr;
 if(crash_download){
  unsigned victim=4;Json progress;
  until([&]{for(unsigned i=0;i<4;++i){check(cores[i]->running()&&relays[i]->running(),"all processes alive before targeted crash");std::istringstream lines(relays[i]->output());std::string line;while(std::getline(lines,line)){auto event=Json::parse(line,nullptr,false);if(event.is_object()&&event.value("event","")=="test-bft-fragment-progress"&&event.at("fragments")==1){victim=i;progress=event;return true;}}}return false;},"Receiver acknowledged first BFT fragment over libp2p",40);
  relays[victim]->signal(SIGSTOP);
  auto before=rpc(ports[victim],"nano_bftManifest");check(before.is_object()&&before.at("proposal").is_null(),"receiver has not validated incomplete proposal");
  check(rpc(ports[victim],"eth_blockNumber")=="0x0","victim remains at genesis before crash");
  cores[victim]->signal(SIGKILL);check(cores[victim]->wait().code==-SIGKILL,"core SIGKILL observed");relays[victim]->signal(SIGKILL);check(relays[victim]->wait().code==-SIGKILL,"relay SIGKILL observed");
  {Blockchain reopened(root/("node-"+std::to_string(victim)),genesis);check(reopened.tip().header.number==0&&reopened.state().root()==genesis.state.root()&&reopened.audit().at("verified")==true,"crashed receiver reopens at audited genesis");}
  cores[victim]=std::make_unique<Process>(std::vector<std::string>{argv[0],"--core",genesis_path.string(),(root/("node-"+std::to_string(victim))).string(),transactions_path.string(),(root/("journal-"+std::to_string(victim))).string(),"fragment-token-"+std::to_string(victim)},Env{{"NANO_BFT_FRAGMENT_NETWORK_KEY",hex(keys[victim])}});ports[victim]=cores[victim]->port();auto restarted_pid=cores[victim]->event("core-process").at("pid");check(!core_pids.contains(restarted_pid.get<int>()),"fresh process identity after core death");
  bool stale_rejected=false;try{rpc(ports[victim],"nano_finishBftImport",Json::array({"fragment-token-"+std::to_string(victim),progress.at("session")}));}catch(const RpcError& e){stale_rejected=e.code==-32602&&std::string(e.what()).find("unknown or expired block import")!=std::string::npos;}check(stale_rejected,"old assembly cannot finish after restart");check(rpc(ports[victim],"eth_blockNumber")=="0x0","RPC responsive after stale session refusal");
  std::vector<std::string> args={argv[1],std::to_string(ports[victim]),(root/("peer-"+std::to_string(victim))).string(),"/ip4/127.0.0.1/tcp/0"};for(unsigned i=0;i<4;++i)if(i!=victim)args.push_back(seeds[i]);
  relays[victim]=std::make_unique<Process>(args,Env{{"NANO_NETWORK_TOKEN","fragment-token-"+std::to_string(victim)},{"NANO_TEST_NETWORK_TRACE","1"}});seeds[victim]=relays[victim]->event("listen").at("address").get<std::string>();relays[victim]->event("ready");
  crash_evidence={{"victim",victim},{"progress",progress},{"observedCoreSignal","SIGKILL"},{"observedRelaySignal","SIGKILL"},{"preCommitAudit",true},{"staleSessionRejected",true},{"restartedCorePid",restarted_pid}};
  std::cout<<Json{{"stage","bft-download-crash-restarted"},{"fork",fork},{"crash",crash_evidence}}.dump()<<std::endl;
 }

 try{until([&]{for(unsigned i=0;i<4;++i){check(relays[i]->running()&&(!processes||cores[i]->running()),"core and relay remain alive");relays[i]->pump(0);if(rpc(ports[i],"eth_getTransactionReceipt",Json::array({hex(transactions.back().hash())})).is_null())return false;}return true;},"Four validators finalize oversized proposal over libp2p",crash_download?160:100);}
 catch(...){for(unsigned i=0;i<4;++i){std::cout<<Json{{"stage","failed-network-state"},{"peer",i},{"manifest",rpc(ports[i],"nano_bftManifest")},{"status",rpc(ports[i],"nano_networkStatus")}}.dump()<<std::endl;std::cout<<relays[i]->output();}throw;}
 auto receipt=rpc(ports[0],"eth_getTransactionReceipt",Json::array({hex(transactions.front().hash())}));auto height=quantity(receipt.at("blockNumber"));auto expected=rpc(ports[0],"eth_getBlockByNumber",Json::array({quantity(height),false}));check(expected.at("transactions").size()==10,"all ten transactions share one block");
 for(auto port:ports){auto actual=rpc(port,"eth_getBlockByNumber",Json::array({quantity(height),false}));check(actual==expected,"all peers agree on block");for(const auto& tx:transactions){auto value=rpc(port,"eth_getTransactionReceipt",Json::array({hex(tx.hash())}));check(value.at("status")=="0x1"&&value.at("blockHash")==expected.at("hash")&&value==rpc(ports[0],"eth_getTransactionReceipt",Json::array({hex(tx.hash())})),"all signed receipts agree");}check(rpc(port,"eth_getBalance",Json::array({"0x00000000000000000000000000000000000000b0","latest"}))=="0xa","exactly-once transfers");}
 size_t successful=0,failed=0;std::set<unsigned> receivers;unsigned receiver_index=0;for(auto& relay:relays){check(relay->stop().code==0,"relay stops cleanly");std::istringstream lines(relay->output());std::string line;while(std::getline(lines,line)){auto row=Json::parse(line,nullptr,false);if(!row.is_object())continue;if(row.value("event","")=="test-bft-fragments-received"){row["receiver"]=receiver_index;if(row.at("blockHash")==expected.at("hash")){check(row.at("fragments")==40&&row.at("height")==u64(height),"target proposal trace must contain every fragment");++successful;receivers.insert(receiver_index);}std::cout<<row.dump()<<std::endl;}if(row.value("event","")=="test-bft-fragments-failed"){++failed;std::cout<<row.dump()<<std::endl;}}++receiver_index;}runners.clear();servers.clear();rpcs.clear();chains.clear();for(auto& core:cores)check(core->stop().code==0,"core process stops cleanly");
 check(successful>=2&&receivers.size()>=2,"distinct quorum validators receive every target proposal fragment");size_t compact=0;uint64_t round=0;
 for(unsigned i=0;i<4;++i){Blockchain reopened(root/("node-"+std::to_string(i)),genesis);auto block=reopened.store().block_by_number(height);check(block&&hex(block->header.hash())==expected.at("hash").get<std::string>()&&reopened.audit().at("verified")==true,"reopened history audits");compact=block->to_transport_json().dump().size();round=reopened.store().certificate_at(height)->round;}
 check(compact>16*1024*1024,"certified proposal exceeds legacy frame limit");
 std::cout<<Json{{"scenario",crash_download?"bft-fragment-download-crash":"bft-fragment-network"},{"crash",crash_evidence},{"fork",fork},{"validators",4},{"successfulDownloads",successful},{"distinctTargetReceivers",receivers.size()},{"failedDownloads",failed},{"compactBytes",compact},{"certificateRound",round},{"reopenedAudits",4},{"blockHash",expected.at("hash")},{"durationSeconds",std::chrono::duration<double>(std::chrono::steady_clock::now()-began).count()},{"coreProcesses",processes?4:1},{"corePids",core_pids},{"relayProcesses",4}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
