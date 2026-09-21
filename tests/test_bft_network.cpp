#include "support/process.hpp"
#include "support/persistent_rpc.hpp"
#include "nano/chain.hpp"
#include <atomic>
#include <array>
using namespace test;
int main(int argc,char** argv){try{
 check(argc==3||argc==4,"core and relay binaries required");Temp root;
 bool joining=argc==4&&std::string(argv[3])=="--joining";
 Json staking;std::ifstream(std::string(NANO_TEST_SOURCE)+"/contracts/NanoStaking.json")>>staking;
 std::vector<Bytes> keys;std::vector<std::string> addresses;Json allocations=Json::object();
 Int minimum=ProofOfStake::minimum_stake;
 for(unsigned i=1;i<=4;++i){keys.push_back(bytes(Int(i),32));addresses.push_back(public_address(public_key(keys.back())));allocations[addresses.back()]={{"balance",quantity(Int(minimum*10))}};}
 auto registry=create_address(addresses[0],0);
 Json config={{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bootstrapValidator",addresses[0]},{"stakingContract",registry}};
 auto seed=Genesis::from_json({{"config",config},{"alloc",allocations}});State registered;
 {Blockchain setup(root/"setup",seed);
  for(unsigned i=0;i<(joining?3u:4u);++i){Transaction tx;tx.chain_id=1337;tx.value=minimum;tx.gas=3000000;tx.gas_price=2000000000;
   if(i==0)tx.data=unhex(staking.at("bytecode"));else{tx.to=registry;tx.data=unhex(staking.at("methodIdentifiers").at("registerValidator()"));}
   auto signed_tx=SignedTransaction::sign(tx,keys[i]);check(setup.submit(signed_tx),"seed validator registration submitted");
   auto selected=setup.next_validator();auto index=std::find_if(addresses.begin(),addresses.end(),[&](const auto& candidate){return address(candidate)==address(*selected);})-addresses.begin();check(index<4,"known setup proposer");setup.validate_signed(keys[index],Int((i+1)*12));
   check(setup.store().receipt(signed_tx.hash())->success,"seed validator registered by executed contract");
  }registered=setup.state();
 }

 config["bftVersion"]=1;auto genesis=root/"genesis.json";save(genesis,{{"config",config},{"alloc",registered.to_json()}});
 std::optional<unsigned> silent;
 bool flood=argc==4&&std::string(argv[3])=="--restart-flood";
 bool lagging=argc==4&&std::string(argv[3])=="--lagging";
 bool star=argc==4&&std::string(argv[3])=="--star",line=argc==4&&std::string(argv[3])=="--line",restart=flood||(argc==4&&std::string(argv[3])=="--restart"),partition=restart||(argc==4&&std::string(argv[3])=="--partition");
 if(argc==4&&!star&&!line&&!partition&&!lagging&&!joining){Blockchain selector(root/"selector",Genesis::from_json(load(genesis)));auto chosen=selector.next_validator();for(unsigned i=0;i<4;++i)if(address(addresses[i])==address(*chosen))silent=i;check(silent.has_value(),"silent proposer located");}
 std::vector<std::unique_ptr<Process>> cores,relays;std::vector<int> ports;std::vector<std::string> seeds;
 for(unsigned i=0;i<4;++i){if(silent&&i==*silent)continue;auto cfg=root/("node-"+std::to_string(i)+".json");save(cfg,{{"bft",{{"key_env","TEST_BFT_KEY"},{"journal",(root/("journal-"+std::to_string(i))).string()}}}});
  cores.push_back(std::make_unique<Process>(std::vector<std::string>{argv[1],"serve",genesis.string(),(root/("node-"+std::to_string(i))).string(),"0","127.0.0.1",cfg.string()},Env{{"TEST_BFT_KEY",hex(keys[i])}}));ports.push_back(cores.back()->port());
 }
 for(unsigned i=0;i<ports.size();++i){std::vector<std::string> args{argv[2],std::to_string(ports[i]),(root/("peer-"+std::to_string(i))).string(),"/ip4/127.0.0.1/tcp/0"};if(lagging){if(i>1)args.insert(args.end(),seeds.begin()+1,seeds.end());}else if(partition){if(i%2==1)args.push_back(seeds.back());}else if(star||line){if(!seeds.empty())args.push_back(star?seeds.front():seeds.back());}else args.insert(args.end(),seeds.begin(),seeds.end());relays.push_back(std::make_unique<Process>(args));seeds.push_back(relays.back()->event("listen")["address"].get<std::string>());relays.back()->event("ready");}
 if(lagging){
  Transaction tx;tx.nonce=registered.get(addresses[1]).nonce;tx.to="0x00000000000000000000000000000000000000ac";tx.value=1;tx.gas_price=2000000000;
  auto signed_tx=SignedTransaction::sign(tx,keys[1]);auto hash=hex(signed_tx.hash());
  check(rpc(ports[1],"eth_sendRawTransaction",{hex(signed_tx.raw())})==hash,"majority accepts transfer during isolation");
  until([&]{
   check(cores[0]->running()&&relays[0]->running(),"isolated node must stay alive");
   check(rpc(ports[0],"eth_blockNumber")=="0x0","isolated quarter cannot finalize alone");
   for(unsigned i=1;i<4;++i)if(quantity(rpc(ports[i],"eth_blockNumber"))<4||rpc(ports[i],"eth_getTransactionReceipt",{hash}).is_null())return false;
   return true;
  },"Majority advances four heights while one validator is isolated",90);
  auto expected_receipt=rpc(ports[1],"eth_getTransactionReceipt",{hash});check(expected_receipt["status"]=="0x1","isolated-period transfer succeeds");
  std::string stale_vote;auto stale_snapshot=rpc(ports[0],"nano_bftExchange");
  for(const auto& encoded:stale_snapshot.at("votes")){auto vote=Json::parse(encoded.get<std::string>());if(address(vote.at("validator").get<std::string>())==address(addresses[0]))stale_vote=encoded.get<std::string>();}
  check(!stale_vote.empty(),"isolated validator reserved a vote before falling behind");
  // Restart with the stale height-one vote journal before reconnecting transport.
  relays[0]->stop();cores[0]->signal(SIGKILL);check(cores[0]->wait(5).code==-SIGKILL,"isolated core killed abruptly");
  cores[0]=std::make_unique<Process>(std::vector<std::string>{argv[1],"serve",genesis.string(),(root/"node-0").string(),"0","127.0.0.1",(root/"node-0.json").string()},Env{{"TEST_BFT_KEY",hex(keys[0])}});ports[0]=cores[0]->port();
  until([&]{auto snapshot=rpc(ports[0],"nano_bftExchange");if(snapshot.is_null())return false;for(const auto& encoded:snapshot.at("votes"))if(encoded==Json(stale_vote))return true;return false;},"Stale journal resumes exact vote before catch-up",5);
  std::vector<std::string> args{argv[2],std::to_string(ports[0]),(root/"peer-0").string(),"/ip4/127.0.0.1/tcp/0"};args.insert(args.end(),seeds.begin()+1,seeds.end());
  relays[0]=std::make_unique<Process>(args);relays[0]->event("listen");relays[0]->event("ready");
  until([&]{return quantity(rpc(ports[0],"eth_blockNumber"))>=4&&!rpc(ports[0],"eth_getTransactionReceipt",{hash}).is_null();},"Restarted validator imports missed certified blocks",45);
  for(unsigned height=1;height<=4;++height){
   auto number=quantity(Int(height));auto reference=rpc(ports[1],"eth_getBlockByNumber",{number,false});auto recovered=rpc(ports[0],"eth_getBlockByNumber",{number,false});
   check(recovered["hash"]==reference["hash"]&&recovered["stateRoot"]==reference["stateRoot"],"recovered history differs from majority");
  }
  auto receipt=rpc(ports[0],"eth_getTransactionReceipt",{hash});check(receipt["status"]=="0x1"&&receipt["blockHash"]==expected_receipt["blockHash"],"recovered receipt differs");
  check(rpc(ports[0],"eth_getBalance",{*tx.to,"latest"})=="0x1","catch-up applies transfer exactly once");
  // Remove a different validator: subsequent finality now requires the returnee.
  relays[1]->stop();tx.nonce+=1;tx.value=2;signed_tx=SignedTransaction::sign(tx,keys[1]);hash=hex(signed_tx.hash());
  check(rpc(ports[0],"eth_sendRawTransaction",{hex(signed_tx.raw())})==hash,"recovered validator accepts new transaction");
  until([&]{for(unsigned i:{0u,2u,3u})if(rpc(ports[i],"eth_getTransactionReceipt",{hash}).is_null())return false;return true;},"Recovered validator participates in new finality",45);
  receipt=rpc(ports[0],"eth_getTransactionReceipt",{hash});check(receipt["status"]=="0x1","post-recovery transfer succeeds");
  auto certified=rpc(ports[0],"nano_networkBlocks",{quantity(Int(quantity(receipt["blockNumber"])-1))}).at("blocks").at(0);
  bool returnee_signed=false;for(const auto& encoded:certified.at("bftCertificate").at("votes")){
   auto vote=Json::parse(encoded.get<std::string>());if(address(vote.at("validator").get<std::string>())==address(addresses[0]))returnee_signed=true;
  }
  check(returnee_signed,"new certificate includes the recovered validator's vote");
  for(unsigned i:{0u,2u,3u}){auto other=rpc(ports[i],"eth_getTransactionReceipt",{hash});check(other["blockHash"]==receipt["blockHash"]&&other["status"]=="0x1","post-recovery finality differs");check(rpc(ports[i],"eth_getBalance",{*tx.to,"latest"})=="0x3","post-recovery transfer lost or duplicated");}
  std::cout<<"Four-height catch-up after SIGKILL and renewed quorum participation passed"<<std::endl;
 }
 std::array<std::atomic<int>,4> attack_ports{};
 std::atomic<bool> crashing=false;std::atomic<unsigned> foreign_rejections=0,bft_rejections=0,healthy_reads=0,transport_failures=0,unexpected=0;
 std::vector<std::jthread> attackers;
 if(flood){
  Transaction foreign;foreign.chain_id=7331;foreign.to=addresses[1];auto raw=hex(SignedTransaction::sign(foreign,keys[0]).raw());
  for(unsigned i=0;i<4;++i){attack_ports[i]=ports[i];attackers.emplace_back([&,i,raw](std::stop_token stop){
   unsigned iteration=0;int connected_port=0;std::unique_ptr<PersistentRpc> client;
   while(!stop.stop_requested()){
    auto port=attack_ports[i].load();bool during_crash=crashing.load();
    if(!port){std::this_thread::yield();continue;}
    try{
     if(!client||connected_port!=port){client=std::make_unique<PersistentRpc>(port);connected_port=port;}
     bool malformed=(iteration++%2)==0;
     try{client->call(malformed?"nano_bftExchange":"eth_sendRawTransaction",malformed?Json::array({Json{{"invalid",true}}}):Json::array({raw}));++unexpected;}
     catch(const std::exception& error){std::string message=error.what();
      if(malformed&&message.find("invalid reactor envelope")!=std::string::npos)++bft_rejections;
      else if(!malformed&&message.find("wrong chain ID")!=std::string::npos)++foreign_rejections;
      else throw;
     }
     if(client->call("eth_chainId")!="0x539")++unexpected;else ++healthy_reads;
    }catch(const std::exception&){client.reset();if(during_crash||crashing.load()||port!=attack_ports[i].load())++transport_failures;else ++unexpected;}
   }
  });}
 }
 if(partition){
  auto deadline=std::chrono::steady_clock::now()+12s;
  while(std::chrono::steady_clock::now()<deadline){for(unsigned i=0;i<4;++i){check(cores[i]->running()&&relays[i]->running(),"partitioned process exited");check(rpc(ports[i],"eth_blockNumber")=="0x0","half voting power finalized during partition");}std::this_thread::sleep_for(100ms);}
  std::cout<<"Two isolated halves cannot finalize"<<std::endl;
  for(auto& relay:relays)relay->stop();relays.clear();seeds.clear();
  if(restart){
   std::vector<std::string> reserved;
   for(unsigned i=0;i<4;++i){auto snapshot=rpc(ports[i],"nano_bftExchange");std::string last;unsigned last_step=0;
    for(const auto& encoded:snapshot.at("votes")){auto vote=Json::parse(encoded.get<std::string>());unsigned step=vote.at("step")=="precommit"?2:1;if(address(vote.at("validator").get<std::string>())==address(addresses[i])&&step>last_step){last=encoded.get<std::string>();last_step=step;}}
    check(!last.empty(),"validator has a durable vote before crash");reserved.push_back(last);
   }
   if(flood){crashing=true;for(auto& port:attack_ports)port=0;}
   for(auto& core:cores)core->signal(SIGKILL);
   for(auto& core:cores)check(core->wait(5).code==-SIGKILL,"core killed abruptly");cores.clear();ports.clear();
   for(unsigned i=0;i<4;++i){auto cfg=root/("node-"+std::to_string(i)+".json");cores.push_back(std::make_unique<Process>(std::vector<std::string>{argv[1],"serve",genesis.string(),(root/("node-"+std::to_string(i))).string(),"0","127.0.0.1",cfg.string()},Env{{"TEST_BFT_KEY",hex(keys[i])}}));ports.push_back(cores.back()->port());}
   if(flood){for(unsigned i=0;i<4;++i)attack_ports[i]=ports[i];crashing=false;}
   for(unsigned i=0;i<4;++i)until([&]{auto snapshot=rpc(ports[i],"nano_bftExchange");if(snapshot.is_null())return false;for(const auto& encoded:snapshot.at("votes"))if(encoded==Json(reserved[i]))return true;return false;},"Restart reproduces exact reserved vote",5);
  }
  for(unsigned i=0;i<ports.size();++i){std::vector<std::string> args{argv[2],std::to_string(ports[i]),(root/("peer-"+std::to_string(i))).string(),"/ip4/127.0.0.1/tcp/0"};args.insert(args.end(),seeds.begin(),seeds.end());relays.push_back(std::make_unique<Process>(args));seeds.push_back(relays.back()->event("listen")["address"].get<std::string>());relays.back()->event("ready");}
 }
 until([&]{for(auto port:ports)if(quantity(rpc(port,"eth_blockNumber"))<1)return false;return true;},"Four networked validators finalize a block",35);
 auto expected=rpc(ports[0],"eth_getBlockByNumber",{"0x1",false});
 for(auto port:ports){auto block=rpc(port,"eth_getBlockByNumber",{"0x1",false});check(block["hash"]==expected["hash"]&&block["stateRoot"]==expected["stateRoot"],"distributed BFT commitments differ");auto batch=rpc(port,"nano_networkBlocks",{0});check(batch["blocks"][0]["bftCertificate"]["votes"].size()>=3,"distributed certificate lacks quorum");}
 if(joining){
  check(rpc(ports[3],"nano_bftExchange").is_null(),"unregistered node follows certified blocks without voting");
  Transaction registration;registration.chain_id=1337;registration.to=registry;registration.value=minimum;registration.gas=500000;registration.gas_price=2000000000;registration.data=unhex(staking.at("methodIdentifiers").at("registerValidator()"));
  auto signed_registration=SignedTransaction::sign(registration,keys[3]);auto registration_hash=hex(signed_registration.hash());
  check(rpc(ports[3],"eth_sendRawTransaction",{hex(signed_registration.raw())})==registration_hash,"observer relays its signed registration");
  until([&]{for(auto port:ports)if(rpc(port,"eth_getTransactionReceipt",{registration_hash}).is_null())return false;return true;},"Registration finalizes through libp2p on every node",40);
  auto receipt=rpc(ports[3],"eth_getTransactionReceipt",{registration_hash});check(receipt["status"]=="0x1","network registration succeeds");
  auto activation_height=quantity(receipt["blockNumber"]);
  for(auto port:ports){
   auto other=rpc(port,"eth_getTransactionReceipt",{registration_hash});check(other["status"]=="0x1"&&other["blockHash"]==receipt["blockHash"],"registration receipt agrees on all peers");
   auto block=rpc(port,"nano_networkBlocks",{quantity(Int(activation_height-1))}).at("blocks").at(0);
   check(block.at("bftCertificate").at("votes").size()==3,"activation certified by all three former validators");
   for(const auto& encoded:block.at("bftCertificate").at("votes")){auto vote=Json::parse(encoded.get<std::string>());check(address(vote.at("validator").get<std::string>())!=address(addresses[3]),"newcomer cannot certify its own registration");}
  }
  // Three of four are now required: remove one old signer so the newcomer is
  // essential to the next transaction's certificate, not just an observer.
  relays[0]->stop();
  Transaction tx;tx.chain_id=1337;tx.nonce=1;tx.to="0x00000000000000000000000000000000000000ad";tx.value=1;tx.gas_price=2000000000;
  auto signed_tx=SignedTransaction::sign(tx,keys[3]);auto hash=hex(signed_tx.hash());
  check(rpc(ports[3],"eth_sendRawTransaction",{hex(signed_tx.raw())})==hash,"new member submits transfer without process restart");
  until([&]{for(unsigned i=1;i<4;++i)if(rpc(ports[i],"eth_getTransactionReceipt",{hash}).is_null())return false;return true;},"New validator supplies live quorum after old validator disconnects",40);
  receipt=rpc(ports[3],"eth_getTransactionReceipt",{hash});check(receipt["status"]=="0x1"&&quantity(receipt["blockNumber"])>activation_height,"new member transaction commits after activation");
  for(unsigned i=1;i<4;++i){
   auto other=rpc(ports[i],"eth_getTransactionReceipt",{hash});check(other["status"]=="0x1"&&other["blockHash"]==receipt["blockHash"],"live peers agree after validator admission");
   check(rpc(ports[i],"eth_getBalance",{*tx.to,"latest"})=="0x1","post-admission transfer applied exactly once");
   auto block=rpc(ports[i],"nano_networkBlocks",{quantity(Int(quantity(receipt["blockNumber"])-1))}).at("blocks").at(0);bool found=false;
   for(const auto& encoded:block.at("bftCertificate").at("votes")){auto vote=Json::parse(encoded.get<std::string>());if(address(vote.at("validator").get<std::string>())==address(addresses[3]))found=true;}
   check(found,"certified block contains newly registered validator signature");
  }
  std::cout<<"Live libp2p validator admission and required new-member signature passed"<<std::endl;
 }
 if(flood){
  Transaction tx;tx.nonce=registered.get(addresses[0]).nonce;tx.to="0x00000000000000000000000000000000000000ab";tx.value=1;tx.gas_price=2000000000;
  auto signed_tx=SignedTransaction::sign(tx,keys[0]);auto hash=hex(signed_tx.hash());check(rpc(ports[0],"eth_sendRawTransaction",{hex(signed_tx.raw())})==hash,"healthy transaction accepted under flood");
  until([&]{for(auto port:ports)if(rpc(port,"eth_getTransactionReceipt",{hash}).is_null())return false;return true;},"Healthy transaction finalized on every validator under continuing flood",35);
  auto receipt=rpc(ports[0],"eth_getTransactionReceipt",{hash});
  for(auto port:ports){auto other=rpc(port,"eth_getTransactionReceipt",{hash});check(other["status"]=="0x1"&&other["blockHash"]==receipt["blockHash"],"flood changed receipt outcome or consensus");check(rpc(port,"eth_getBalance",{*tx.to,"latest"})=="0x1","healthy transfer lost or duplicated under flood");}
  for(auto& worker:attackers)worker.request_stop();attackers.clear();
  check(unexpected==0&&foreign_rejections>=20&&bft_rejections>=20&&healthy_reads>=40,"flood rejection/read thresholds or unexpected responses");
  std::cout<<Json{{"foreignRejections",foreign_rejections.load()},{"malformedBftRejections",bft_rejections.load()},{"healthyReads",healthy_reads.load()},{"crashTransportFailures",transport_failures.load()},{"unexpectedResponses",unexpected.load()}}.dump()<<std::endl;
 }
 if(silent){auto batch=rpc(ports[0],"nano_networkBlocks",{0});check(quantity(batch["blocks"][0]["bftCertificate"]["round"])>0,"silent initial proposer requires later round");}
 std::cout<<"Distributed BFT proposal and vote exchange passed\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
