#include "support/process.hpp"
#include "nano/rpc.hpp"
using namespace test;
int main(int argc,char** argv){try{
 check(argc==3,"nano and relay binaries required");Temp root;
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
 std::vector<std::unique_ptr<Process>> nodes;std::vector<int> ports,relay_pids;std::vector<std::string> seeds;std::vector<fs::path> configs;
 auto start_node=[&](unsigned i){return std::make_unique<Process>(std::vector<std::string>{argv[1],"run",genesis_path.string(),(root/("node-"+std::to_string(i))).string(),configs[i].string()},Env{{"NANO_MANAGED_LARGE_KEY",hex(keys[i])},{"NANO_TEST_NETWORK_TRACE","1"}});};
 for(unsigned i=0;i<4;++i){
  auto file=root/("config-"+std::to_string(i)+".json");configs.push_back(file);
  save(file,{{"rpc_host","127.0.0.1"},{"rpc_port",0},{"p2p_host","127.0.0.1"},{"p2p_port",0},{"p2p_binary",argv[2]},{"bootnodes",seeds},{"bft",{{"key_env","NANO_MANAGED_LARGE_KEY"},{"journal",(root/("journal-"+std::to_string(i))).string()}}}});
  nodes.push_back(start_node(i));ports.push_back(nodes.back()->port());relay_pids.push_back(nodes.back()->event("p2p-process").at("pid"));seeds.push_back(nodes.back()->event("listen").at("address"));nodes.back()->event("ready");
 }
 check(std::set<int>(relay_pids.begin(),relay_pids.end()).size()==4,"four separately managed relays");
 size_t duplicate_submissions=0;
 auto submit=[&](int port,const SignedTransaction& tx){try{check(rpc(port,"eth_sendRawTransaction",Json::array({hex(tx.raw())}))==hex(tx.hash()),"signed transaction admitted through public RPC");}catch(const RpcError& e){if(e.code!=-32000||std::string(e.what()).find("Transaction rejected by pool")==std::string::npos)throw;auto known=rpc(port,"eth_getTransactionByHash",Json::array({hex(tx.hash())}));check(known.is_object()&&known.at("hash")==hex(tx.hash()),"duplicate rejection must identify exact already-gossiped signed transaction");++duplicate_submissions;}};
 // Admit the nonce gap first, through the production HTTP RPC. Releasing the
 // missing first nonce makes all ten transactions eligible together.
 for(size_t n=1;n<transactions.size();++n)for(auto port:ports)submit(port,transactions[n]);
 for(auto port:ports)check(rpc(port,"eth_getTransactionReceipt",Json::array({hex(transactions.back().hash())})).is_null(),"nonce gap prevents premature execution");
 for(auto port:ports)submit(port,transactions[0]);
 unsigned victim=4;Json progress;
 try{until([&]{for(unsigned i=0;i<4;++i){check(nodes[i]->running(),"managed node alive before crash");std::istringstream lines(nodes[i]->output());std::string line;while(std::getline(lines,line)){auto event=Json::parse(line,nullptr,false);if(event.is_object()&&event.value("event","")=="test-bft-fragment-progress"&&event.at("fragments")==1){victim=i;progress=event;return true;}}}return false;},"Managed receiver acknowledged first proposal fragment",90);}
 catch(...){for(auto& node:nodes)std::cout<<node->output();throw;}
 auto manifest=rpc(ports[victim],"nano_bftManifest");check(manifest.is_object()&&manifest.at("proposal").is_null(),"incomplete proposal not accepted before crash");
 auto before=rpc(ports[victim],"eth_getBlockByNumber",Json::array({"latest",false}));check(rpc(ports[victim],"eth_getTransactionReceipt",Json::array({hex(transactions[0].hash())})).is_null(),"no target transaction committed before crash");
 nodes[victim]->signal(SIGKILL);check(nodes[victim]->wait().code==-SIGKILL,"managed core SIGKILL observed");until([&]{return kill(relay_pids[victim],0)<0&&errno==ESRCH;},"Orphan relay exits after managed core death",6);
 auto interrupted=nodes[victim]->output();std::cout<<Json{{"stage","managed-large-crash"},{"fork",fork},{"victim",victim},{"progress",progress},{"parentSignal","SIGKILL"},{"orphanExited",true},{"priorBlockHash",before.at("hash")}}.dump()<<std::endl;
 {Blockchain reopened(root/("node-"+std::to_string(victim)),genesis);check(hex(reopened.tip().header.hash())==before.at("hash").get<std::string>()&&reopened.audit().at("verified")==true,"crashed managed node reopens at exact audited prior block");for(const auto& tx:transactions)check(!reopened.store().receipt(tx.hash()),"partial proposal left no receipt");}
 auto settings=load(configs[victim]);settings["bootnodes"]=Json::array();for(unsigned i=0;i<4;++i)if(i!=victim)settings["bootnodes"].push_back(seeds[i]);save(configs[victim],settings);
 auto previous_identity=seeds[victim].substr(seeds[victim].rfind("/p2p/"));auto old_pid=relay_pids[victim];nodes[victim]=start_node(victim);ports[victim]=nodes[victim]->port();relay_pids[victim]=nodes[victim]->event("p2p-process").at("pid");seeds[victim]=nodes[victim]->event("listen").at("address");nodes[victim]->event("ready");check(relay_pids[victim]!=old_pid&&seeds[victim].substr(seeds[victim].rfind("/p2p/"))==previous_identity,"fresh relay preserves managed identity");
 // Do not inject the lost mempool into the restarted node. It must recover
 // through its ordinary libp2p proposal/block/transaction synchronization.
 try{until([&]{for(unsigned i=0;i<4;++i){check(nodes[i]->running(),"managed node remains alive after recovery");if(rpc(ports[i],"eth_getTransactionReceipt",Json::array({hex(transactions.back().hash())})).is_null())return false;}return true;},"Managed quorum finalizes and recovered peer catches up",180);}
 catch(...){for(auto& node:nodes)std::cout<<node->output();throw;}
 auto receipt=rpc(ports[0],"eth_getTransactionReceipt",Json::array({hex(transactions[0].hash())}));auto height=quantity(receipt.at("blockNumber"));auto expected=rpc(ports[0],"eth_getBlockByNumber",Json::array({quantity(height),false}));check(expected.at("transactions").size()==10,"all ten RPC-admitted transactions share one block");
 for(auto port:ports){check(rpc(port,"eth_getBlockByNumber",Json::array({quantity(height),false}))==expected,"managed peers agree on certified block");for(const auto& tx:transactions){auto r=rpc(port,"eth_getTransactionReceipt",Json::array({hex(tx.hash())}));check(r.at("status")=="0x1"&&r.at("blockHash")==expected.at("hash")&&r==rpc(ports[0],"eth_getTransactionReceipt",Json::array({hex(tx.hash())})),"managed receipts agree");}check(rpc(port,"eth_getBalance",Json::array({"0x00000000000000000000000000000000000000b0","latest"}))=="0xa","exactly-once managed transfers");}
 std::set<unsigned> receivers;size_t downloads=0,failed=0,delta_messages=0,votes_suppressed=0;
 for(unsigned i=0;i<4;++i){check(nodes[i]->stop().code==0,"managed node stops cleanly");until([&]{return kill(relay_pids[i],0)<0&&errno==ESRCH;},"Managed relay reaped");std::istringstream lines(nodes[i]->output());std::string line;while(std::getline(lines,line)){auto row=Json::parse(line,nullptr,false);if(!row.is_object())continue;auto event=row.value("event","");if(event=="test-bft-vote-delta"){check(row.at("votesBefore").get<size_t>()>row.at("votesSent").get<size_t>(),"observed vote reduction");++delta_messages;votes_suppressed+=row.at("votesBefore").get<size_t>()-row.at("votesSent").get<size_t>();std::cout<<row.dump()<<std::endl;}else if(event=="test-bft-fragments-received"){row["receiver"]=i;if(row.at("blockHash")==expected.at("hash")){check(row.at("fragments")==40&&row.at("height")==u64(height),"complete target proposal download");receivers.insert(i);++downloads;}std::cout<<row.dump()<<std::endl;}else if(event=="test-bft-fragments-failed"){++failed;std::cout<<row.dump()<<std::endl;}}}
 check(delta_messages>0&&votes_suppressed>0,"real managed traffic used vote deltas");
 check(receivers.size()>=2,"distinct quorum receivers downloaded target proposal");size_t compact=0;
 for(unsigned i=0;i<4;++i){Blockchain reopened(root/("node-"+std::to_string(i)),genesis);auto block=reopened.store().block_by_number(height);check(block&&hex(block->header.hash())==expected.at("hash").get<std::string>()&&reopened.store().certificate_at(height)&&reopened.audit().at("verified")==true,"managed reopened certified history audits");compact=block->to_transport_json().dump().size();}
 check(compact>16*1024*1024,"managed proposal exceeds legacy frame size");
 std::cout<<Json{{"scenario","bft-managed-large-crash"},{"fork",fork},{"managedNodes",4},{"rpcSubmissions",40},{"knownDuplicates",duplicate_submissions},{"newAdmissions",40-duplicate_submissions},{"crashAfterFragment",1},{"orphanExited",true},{"identityPreserved",true},{"mempoolReinjected",false},{"preCommitAudit",true},{"reopenedAudits",4},{"receiptCount",10},{"distinctTargetReceivers",receivers.size()},{"voteDeltaMessages",delta_messages},{"votesSuppressed",votes_suppressed},{"successfulDownloads",downloads},{"failedDownloads",failed},{"compactBytes",compact},{"blockHash",expected.at("hash")},{"blockNumber",quantity(height)}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
