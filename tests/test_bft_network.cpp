#include "support/process.hpp"
#include "support/rpc_load_window.hpp"
#include "support/persistent_rpc.hpp"
#include "support/persistent_websocket_rpc.hpp"
#include "nano/chain.hpp"
#include <atomic>
#include <array>
#include <barrier>
using namespace test;
int main(int argc,char** argv){try{
 check(argc==3||argc==4,"core and relay binaries required");Temp root;
 bool default_estimates=argc==4&&std::string(argv[3])=="--rpc-default-overload";
 bool rpc_overload=default_estimates||(argc==4&&std::string(argv[3])=="--rpc-overload");
 bool rpc_cost_websocket=rpc_overload||(argc==4&&std::string(argv[3])=="--rpc-cost-websocket");
 bool rpc_cost_many=rpc_cost_websocket||(argc==4&&std::string(argv[3])=="--rpc-cost-many");
 bool rpc_cost_all=rpc_cost_many||(argc==4&&std::string(argv[3])=="--rpc-cost-all");
 bool rpc_cost_sustained=rpc_cost_all||(argc==4&&std::string(argv[3])=="--rpc-cost-sustained");
 bool rpc_cost_load=rpc_cost_sustained||(argc==4&&std::string(argv[3])=="--rpc-cost");
 bool large_block=argc==4&&std::string(argv[3])=="--large-block";
 bool pool_balance=argc==4&&std::string(argv[3])=="--pool-balance";
 bool short_load=argc==4&&std::string(argv[3])=="--load-short";
 bool load_crash=argc==4&&std::string(argv[3])=="--load-crash";
 bool reconcile_load=argc==4&&std::string(argv[3])=="--load-reconcile";
 bool loading=reconcile_load||load_crash||short_load||(argc==4&&std::string(argv[3])=="--load");
 bool departing=argc==4&&std::string(argv[3])=="--departing";
 bool changing_crash=argc==4&&std::string(argv[3])=="--stake-change-crash";
 bool changing=changing_crash||(argc==4&&std::string(argv[3])=="--stake-change");
 bool joining=argc==4&&std::string(argv[3])=="--joining";
 bool exact_thirds=argc==4&&std::string(argv[3])=="--two-thirds";
 bool weighted=exact_thirds||(argc==4&&std::string(argv[3])=="--weighted");unsigned heavy_weight=exact_thirds?3:4;
 Json staking;std::ifstream(std::string(NANO_TEST_SOURCE)+"/contracts/NanoStaking.json")>>staking;
 std::vector<Bytes> keys;std::vector<std::string> addresses;Json allocations=Json::object();
 Int minimum=ProofOfStake::minimum_stake;
 for(unsigned i=1;i<=4;++i){keys.push_back(bytes(Int(i),32));addresses.push_back(public_address(public_key(keys.back())));allocations[addresses.back()]={{"balance",quantity(Int(minimum*10))}};}
 const Int pool_funds("1000000000000000000000000");
 if(pool_balance||large_block)for(unsigned i=10;i<13;++i)allocations[public_address(public_key(bytes(Int(i),32))) ]={{"balance",quantity(pool_funds)}};
 const std::string cost_leaf="0x00000000000000000000000000000000000040ad";
 std::string cost_target=cost_leaf;
 if(rpc_cost_load){
  allocations[cost_leaf]={{"code","0x60016000556000355b8015601557600190036008565b50602a60005260206000f3"}};
  for(unsigned level=1;level<=4;++level){auto outer=hex(bytes(Int(0x40ad+level),20));allocations[outer]={{"code","0x3660006000376020600060206000600073"+cost_target.substr(2)+"5af1602f5760006000fd5b60206000f3"}};cost_target=outer;}
 }
 auto registry=create_address(addresses[0],0);
 Json config={{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bootstrapValidator",addresses[0]},{"stakingContract",registry}};
 std::string fork="NanoExperimental";if(const char* configured=std::getenv("NANO_TEST_EVM_FORK")){fork=configured;check(fork=="Paris"||fork=="Shanghai","unsupported network fixture fork");config["evmFork"]=fork;}
 auto seed=Genesis::from_json({{"config",config},{"alloc",allocations}});State registered;
 {Blockchain setup(root/"setup",seed);
  for(unsigned i=0;i<(joining?3u:4u);++i){Transaction tx;tx.chain_id=1337;tx.value=minimum*(weighted&&i==0?heavy_weight:1);tx.gas=3000000;tx.gas_price=2000000000;
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
 if(argc==4&&!star&&!line&&!partition&&!lagging&&!joining&&!weighted&&!changing&&!departing&&!loading&&!pool_balance&&!rpc_cost_load&&!large_block){Blockchain selector(root/"selector",Genesis::from_json(load(genesis)));auto chosen=selector.next_validator();for(unsigned i=0;i<4;++i)if(address(addresses[i])==address(*chosen))silent=i;check(silent.has_value(),"silent proposer located");}
 std::vector<std::unique_ptr<Process>> cores,relays;std::vector<int> ports;std::vector<std::string> seeds;
 for(unsigned i=0;i<4;++i){if(silent&&i==*silent)continue;auto cfg=root/("node-"+std::to_string(i)+".json");save(cfg,{{"bft",{{"key_env","TEST_BFT_KEY"},{"journal",(root/("journal-"+std::to_string(i))).string()}}}});
  cores.push_back(std::make_unique<Process>(std::vector<std::string>{argv[1],"serve",genesis.string(),(root/("node-"+std::to_string(i))).string(),"0","127.0.0.1",cfg.string()},Env{{"TEST_BFT_KEY",hex(keys[i])}}));ports.push_back(cores.back()->port());
 }
 // Seed the same nonce-gap state before connecting consensus transport.
 // Recovery is later submitted to one node only and must propagate naturally.
 std::vector<std::vector<SignedTransaction>> pool_cases;
 if(pool_balance)for(unsigned type=0;type<3;++type){
  std::vector<SignedTransaction> txs;auto key=bytes(Int(10+type),32);
  for(const auto& [nonce,percent]:std::vector<std::pair<unsigned,unsigned>>{{1,60},{2,5},{0,60},{1,5}}){
   Transaction tx;tx.chain_id=1337;tx.type=type;tx.nonce=nonce;tx.to=public_address(public_key(bytes(Int(20+type),32)));tx.value=pool_funds*percent/100;tx.gas=21000;tx.gas_price=2000000000;tx.max_fee=2000000000;tx.priority_fee=1000000000;
   txs.push_back(SignedTransaction::sign(tx,key));
  }
  for(auto port:ports)for(unsigned i=0;i<3;++i)check(rpc(port,"eth_sendRawTransaction",{hex(txs[i].raw())})==hex(txs[i].hash()),"preconnect nonce-gap admission");
  pool_cases.push_back(std::move(txs));
 }
 std::vector<SignedTransaction> large_transactions;
 if(large_block){
  for(unsigned nonce=0;nonce<5;++nonce){Transaction tx;tx.chain_id=1337;tx.nonce=nonce;tx.to="0x00000000000000000000000000000000000000b0";tx.value=1;tx.gas=4000000;tx.gas_price=2000000000;tx.data=Bytes(900000,0);large_transactions.push_back(SignedTransaction::sign(tx,bytes(Int(10),32)));}
  // Only majority peers know the transactions; the isolated peer must fetch history.
  for(unsigned i=1;i<4;++i)for(const auto& tx:large_transactions)check(rpc(ports[i],"eth_sendRawTransaction",{hex(tx.raw())})==hex(tx.hash()),"preconnect large transaction admission");
 }
 for(unsigned i=0;i<ports.size();++i){std::vector<std::string> args{argv[2],std::to_string(ports[i]),(root/("peer-"+std::to_string(i))).string(),"/ip4/127.0.0.1/tcp/0"};if(lagging||weighted||large_block){if(i>1)args.insert(args.end(),seeds.begin()+1,seeds.end());}else if(partition){if(i%2==1)args.push_back(seeds.back());}else if(star||line){if(!seeds.empty())args.push_back(star?seeds.front():seeds.back());}else args.insert(args.end(),seeds.begin(),seeds.end());relays.push_back(std::make_unique<Process>(args));seeds.push_back(relays.back()->event("listen")["address"].get<std::string>());relays.back()->event("ready");}
 if(large_block){
  until([&]{
   check(rpc(ports[0],"eth_blockNumber")=="0x0","isolated validator cannot finalize");
   for(unsigned i=1;i<4;++i){check(cores[i]->running()&&relays[i]->running(),"majority process alive");for(const auto& tx:large_transactions)if(rpc(ports[i],"eth_getTransactionReceipt",{hex(tx.hash())}).is_null())return false;}
   return true;
  },"Majority certifies five large transactions",100);
  auto receipt=rpc(ports[1],"eth_getTransactionReceipt",{hex(large_transactions[0].hash())});auto height=quantity(receipt.at("blockNumber"));
  for(const auto& tx:large_transactions){auto other=rpc(ports[1],"eth_getTransactionReceipt",{hex(tx.hash())});check(other.at("blockHash")==receipt.at("blockHash")&&other.at("status")=="0x1","five transactions must execute in one large block");}
  auto page=rpc(ports[1],"nano_networkBlocks",{quantity(Int(height-1))});auto page_bytes=page.dump().size();check(page_bytes>8*1024*1024,"real network page exceeds old eight MiB cap");
  auto block=page.at("blocks").at(0);VotingSet members;Bytes domain;
  {Blockchain inspector(root/"large-inspector",Genesis::from_json(load(genesis)));members=inspector.voting_set();domain=inspector.genesis().block.header.hash();}
  auto cert=block.at("bftCertificate");auto round=u64(quantity(cert.at("round")));std::vector<FinalityVote> votes;
  for(const auto& wire:cert.at("votes"))votes.push_back(decode_vote_message(wire.get<std::string>(),members,1337,domain,u64(height),round));
  check(verify_commit_certificate(votes,members,u64(height),round,unhex(receipt.at("blockHash").get<std::string>()),1337,domain),"large block certificate verifies");
  for(const auto& vote:votes)check(address(vote.validator)!=address(addresses[0]),"isolated validator must not sign majority certificate");
  std::cout<<Json{{"stage","large-block-network-certified"},{"fork",fork},{"height",quantity(height)},{"pageBytes",page_bytes},{"signers",votes.size()}}.dump()<<std::endl;
  relays[0]->stop();std::vector<std::string> args{argv[2],std::to_string(ports[0]),(root/"peer-0").string(),"/ip4/127.0.0.1/tcp/0"};args.insert(args.end(),seeds.begin()+1,seeds.end());relays[0]=std::make_unique<Process>(args);relays[0]->event("listen");relays[0]->event("ready");
  until([&]{return !rpc(ports[0],"eth_getTransactionReceipt",{hex(large_transactions.back().hash())}).is_null();},"Isolated validator catches up over libp2p",60);
  auto expected=rpc(ports[1],"eth_getBlockByNumber",{quantity(height),false});
  for(auto port:ports){auto actual=rpc(port,"eth_getBlockByNumber",{quantity(height),false});check(actual.at("hash")==expected.at("hash")&&actual.at("stateRoot")==expected.at("stateRoot"),"large-block history matches");for(const auto& tx:large_transactions)check(rpc(port,"eth_getTransactionReceipt",{hex(tx.hash())})==rpc(ports[1],"eth_getTransactionReceipt",{hex(tx.hash())}),"large-block receipts match");check(rpc(port,"eth_getBalance",{"0x00000000000000000000000000000000000000b0","latest"})=="0x5","transfers applied exactly once");}
  for(auto& relay:relays)relay->stop();for(auto& core:cores)core->stop();
  for(unsigned i=0;i<4;++i){Blockchain reopened(root/("node-"+std::to_string(i)),Genesis::from_json(load(genesis)));auto state=reopened.store().state_at(height);check(state&&hex(state->root())==expected.at("stateRoot").get<std::string>(),"persisted large-block state matches");check(reopened.audit().at("verified")==true,"large-block reopened audit");}
  std::cout<<Json{{"scenario","large-block-network"},{"fork",fork},{"transactions",5},{"pageBytes",page_bytes},{"blockHash",expected.at("hash")},{"stateRoot",expected.at("stateRoot")},{"reopenedPeers",4},{"isolatedCatchup",true}}.dump()<<std::endl;return 0;
 }
 if(weighted){
  VotingSet members;Bytes genesis_hash;
  {Blockchain inspector(root/"weighted-inspector",Genesis::from_json(load(genesis)));members=inspector.voting_set();genesis_hash=inspector.genesis().block.header.hash();}
  check(members.size()==4,"weighted set has four validators");
  for(const auto& [member,power]:members)check(power==minimum*(address(member)==address(addresses[0])?heavy_weight:1),"executed stake does not match weighted fixture");
  auto deadline=std::chrono::steady_clock::now()+12s;
  while(std::chrono::steady_clock::now()<deadline){for(unsigned i=0;i<4;++i){check(cores[i]->running()&&relays[i]->running(),"weighted partition process exited");check(rpc(ports[i],"eth_blockNumber")=="0x0","sub-quorum stake finalized by validator count");}std::this_thread::sleep_for(100ms);}
  std::cout<<"Initial sub-quorum groups remained without finality"<<std::endl;
  for(auto& relay:relays)relay->stop();relays.clear();seeds.clear();
  for(unsigned i=0;i<4;++i){std::vector<std::string> args{argv[2],std::to_string(ports[i]),(root/("weighted-pair-"+std::to_string(i))).string(),"/ip4/127.0.0.1/tcp/0"};if(i%2==1)args.push_back(seeds.back());relays.push_back(std::make_unique<Process>(args));seeds.push_back(relays.back()->event("listen")["address"].get<std::string>());relays.back()->event("ready");}
  if(exact_thirds){
   for(unsigned i:{0u,1u}){auto peer=relays[i]->event("peer");const auto& remote=seeds[1-i];check(peer.at("peerId")==remote.substr(remote.find_last_of('/')+1),"two-thirds pair did not handshake with intended peer");}
   deadline=std::chrono::steady_clock::now()+12s;
   while(std::chrono::steady_clock::now()<deadline){for(unsigned i=0;i<4;++i){check(cores[i]->running()&&relays[i]->running(),"two-thirds process exited");check(rpc(ports[i],"eth_blockNumber")=="0x0","exactly two thirds finalized");}std::this_thread::sleep_for(100ms);}
   for(unsigned i:{0u,1u}){bool voted=false;auto snapshot=rpc(ports[i],"nano_bftExchange");for(const auto& encoded:snapshot.at("votes")){auto vote=Json::parse(encoded.get<std::string>());if(address(vote.at("validator").get<std::string>())==address(addresses[i]))voted=true;}check(voted,"two-thirds signer did not participate");}
   std::cout<<"Connected participating 4/6 stake stayed below strict quorum"<<std::endl;
   for(auto& relay:relays)relay->stop();relays.clear();seeds.clear();
   for(unsigned i=0;i<4;++i){std::vector<std::string> args{argv[2],std::to_string(ports[i]),(root/("strict-majority-"+std::to_string(i))).string(),"/ip4/127.0.0.1/tcp/0"};if(i<3)args.insert(args.end(),seeds.begin(),seeds.end());relays.push_back(std::make_unique<Process>(args));seeds.push_back(relays.back()->event("listen")["address"].get<std::string>());relays.back()->event("ready");}
  }
  until([&]{for(unsigned i=exact_thirds?3:2;i<4;++i)check(rpc(ports[i],"eth_blockNumber")=="0x0","isolated minority finalized");for(unsigned i=0;i<(exact_thirds?3u:2u);++i)if(quantity(rpc(ports[i],"eth_blockNumber"))<1)return false;return true;},"Stake above two thirds finalizes without isolated minority",45);
  auto block=rpc(ports[0],"eth_getBlockByNumber",{"0x1",false});auto peer=rpc(ports[1],"eth_getBlockByNumber",{"0x1",false});check(block["hash"]==peer["hash"]&&block["stateRoot"]==peer["stateRoot"],"weighted majority disagrees");
  auto certificate=rpc(ports[0],"nano_networkBlocks",{0}).at("blocks").at(0).at("bftCertificate");auto round=u64(quantity(certificate.at("round")));std::vector<FinalityVote> votes;
  for(const auto& encoded:certificate.at("votes"))votes.push_back(decode_vote_message(encoded.get<std::string>(),members,1337,genesis_hash,1,round));
  check(votes.size()==(exact_thirds?3u:2u)&&verify_commit_certificate(votes,members,1,round,unhex(block["hash"]),1337,genesis_hash),"weighted certificate invalid");
  for(const auto& vote:votes)check(address(vote.validator)==address(addresses[0])||address(vote.validator)==address(addresses[1])||(exact_thirds&&address(vote.validator)==address(addresses[2])),"isolated minority entered certificate");
  for(auto& relay:relays)relay->stop();relays.clear();seeds.clear();
  for(unsigned i=0;i<4;++i){std::vector<std::string> args{argv[2],std::to_string(ports[i]),(root/("weighted-joined-"+std::to_string(i))).string(),"/ip4/127.0.0.1/tcp/0"};args.insert(args.end(),seeds.begin(),seeds.end());relays.push_back(std::make_unique<Process>(args));seeds.push_back(relays.back()->event("listen")["address"].get<std::string>());relays.back()->event("ready");}
  until([&]{for(auto port:ports)if(quantity(rpc(port,"eth_blockNumber"))<1)return false;return true;},"Weighted minority catches up after reconnect",30);
  for(auto port:ports){auto recovered=rpc(port,"eth_getBlockByNumber",{"0x1",false});check(recovered["hash"]==block["hash"]&&recovered["stateRoot"]==block["stateRoot"],"weighted minority imported different finalized state");}
  std::cout<<Json{{"scenario",exact_thirds?"two-thirds":"weighted"},{"fork",fork},{"weights",Json::array({heavy_weight,1,1,1})},{"certificateSigners",votes.size()},{"signedWeight",5},{"totalWeight",heavy_weight+3},{"blockHash",block["hash"]},{"stateRoot",block["stateRoot"]},{"reconnectedPeers",4}}.dump()<<std::endl;
  return 0;
 }
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
  // Majority progress can precede the isolated peer's proposal timeout.
  // Synchronize on the vote that this recovery scenario actually requires.
  std::string stale_vote;auto vote_wait_started=std::chrono::steady_clock::now();
  until([&]{
   check(cores[0]->running()&&relays[0]->running(),"isolated node stays alive while awaiting its vote");
   check(rpc(ports[0],"eth_blockNumber")=="0x0","isolated quarter cannot finalize while awaiting its vote");
   auto snapshot=rpc(ports[0],"nano_bftExchange");if(snapshot.is_null())return false;
   check(snapshot.at("height")=="0x1","isolated validator still votes at height one");
   for(const auto& encoded:snapshot.at("votes")){
    auto vote=Json::parse(encoded.get<std::string>());
    if(address(vote.at("validator").get<std::string>())==address(addresses[0])){
     check(vote.at("height")=="0x1","stale vote belongs to height one");stale_vote=encoded.get<std::string>();
    }
   }
   return !stale_vote.empty();
  },"Isolated validator has reserved the vote required for crash recovery",10);
  auto reserved_vote=Json::parse(stale_vote);
  std::cout<<Json{{"stage","lagging-vote-reserved"},{"fork",fork},{"height",reserved_vote.at("height")},{"round",reserved_vote.at("round")},{"step",reserved_vote.at("step")},{"waitMs",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-vote_wait_started).count()}}.dump()<<std::endl;
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
  std::cout<<Json{{"scenario","lagging"},{"fork",fork},{"missedHeightsVerified",4},{"requiredSigner",addresses[0]},{"finalBlockHash",receipt["blockHash"]},{"finalBlockNumber",receipt["blockNumber"]},{"recipientBalance","0x3"}}.dump()<<std::endl;
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
 if(rpc_cost_load){
  const unsigned rpc_workers=rpc_overload?48:rpc_cost_many?16:4;
  const unsigned rpc_rounds=rpc_overload?12:rpc_cost_many?24:rpc_cost_all?96:rpc_cost_sustained?24:3;
  // Only the basic and sustained single-node workloads require this window.
  // Faster execution must not make the traffic end before finality can be read.
  const bool observation_window=!rpc_cost_all;
  const auto workload_deadline=rpc_cost_sustained?80s:45s;
  std::atomic<bool> observation_done=false;
  const std::string recipient="0x00000000000000000000000000000000000000b0";
  Transaction marker;marker.nonce=quantity(rpc(ports[1],"eth_getTransactionCount",{addresses[1],"latest"}));marker.to=recipient;marker.value=7;marker.gas_price=2000000000;
  auto during=SignedTransaction::sign(marker,keys[1]);auto during_hash=hex(during.hash());
  Json invocation={{"from",addresses[0]},{"to",cost_target},{"gas","0x1c9c380"},{"data",hex(bytes(Int(600000),32))}};
  std::atomic<unsigned> completed=0;std::vector<std::string> errors(rpc_workers);std::vector<Json> samples(rpc_workers,Json::array()), requests(rpc_workers,Json::array()),deferred_quotes(rpc_workers,Json::array());
  std::barrier gate(rpc_workers+1);std::vector<std::jthread> clients;auto began=std::chrono::steady_clock::now();
  for(unsigned w=0;w<rpc_workers;++w)clients.emplace_back([&,w]{gate.arrive_and_wait();try{auto port=ports[rpc_cost_many?w/(rpc_overload?12:4):rpc_cost_all?w:0];PersistentRpc http_client(port);std::unique_ptr<PersistentWebSocketRpc> ws_client;if(rpc_cost_websocket)ws_client=std::make_unique<PersistentWebSocketRpc>(port);auto invoke=[&](const std::string& method,Json params=Json::array()){
   auto request_start=std::chrono::steady_clock::now();
   Json trace={{"method",method},{"tag",params.size()>1?params[1]:Json(nullptr)},{"request",requests[w].size()},{"startedMs",std::chrono::duration<double,std::milli>(request_start-began).count()}};
   try{auto result=ws_client?ws_client->call(method,std::move(params)):http_client.call(method,std::move(params));trace["success"]=true;trace["durationMs"]=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-request_start).count();requests[w].push_back(std::move(trace));return result;}
   catch(const std::exception& error){trace["success"]=false;trace["error"]=error.what();if(auto rpc_error=dynamic_cast<const PersistentRpcError*>(&error))trace["rpcCode"]=rpc_error->code;trace["durationMs"]=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-request_start).count();requests[w].push_back(std::move(trace));throw;}
  };for(unsigned round=0;continue_rpc_load(round,rpc_rounds,observation_window,observation_done.load(),
     std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-began),
     std::chrono::duration_cast<std::chrono::milliseconds>(workload_deadline));++round){
   auto start=std::chrono::steady_clock::now();const unsigned mode=rpc_cost_all?(w+round)%4:w;const char* method=mode%2?"eth_estimateGas":"eth_call";const bool omitted_block=default_estimates&&mode==1;const char* tag=omitted_block?"pending":mode<2?"latest":"pending";
   Json result=nullptr;bool overloaded=false;
   try{
    result=invoke(method,omitted_block?Json::array({invocation}):Json::array({invocation,tag}));
    if(mode%2){check(quantity(result)>23000000&&quantity(result)<=30000000,"nested network estimate out of range");auto trial=invocation;trial["gas"]=result;check(invoke("eth_call",Json::array({trial,tag}))==hex(bytes(Int(42),32)),"nested network quote cannot execute");}
    else check(result==hex(bytes(Int(42),32)),"nested network call output mismatch");
   }catch(const PersistentRpcError& error){
    // Only this separately named overload scenario permits explicit admission
    // refusals. Execution failures, transport timeouts and control errors fail.
    const std::string message=error.what();
    if(!rpc_overload||error.code!=-32005||(message!="RPC simulation capacity exceeded"&&message!="RPC simulation queue wait exceeded"))throw;
    overloaded=true;
    // A successfully returned quote must still be validated if its immediate
    // eth_call was refused; do that explicitly after the workload has drained.
    if(mode%2&&!result.is_null())deferred_quotes[w].push_back({{"gas",result},{"tag",tag}});
   }
   check(invoke("eth_chainId")=="0x539","loaded node identity unavailable");
   samples[w].push_back({{"node",rpc_cost_many?w/(rpc_overload?12:4):rpc_cost_all?w:0},{"method",method},{"tag",tag},{"round",round},{"omittedBlock",omitted_block},{"response",result},{"overloaded",overloaded},{"durationMs",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()}});
   std::this_thread::sleep_for(100ms);
  }}catch(const std::exception& e){errors[w]=e.what();}++completed;});
  gate.arrive_and_wait();
  std::string control_error;Json observed=nullptr;unsigned active_when_certified=0;Json progress=Json::array();
  try{
  check(rpc(ports[1],"eth_sendRawTransaction",{hex(during.raw())})==during_hash,"marker admitted during bounded simulation load");
  until([&]{auto value=rpc(ports[1],"eth_getTransactionReceipt",{during_hash});auto done=completed.load();if(!value.is_null()&&done<rpc_workers){observed=value;active_when_certified=rpc_workers-done;return true;}check(done<rpc_workers,"no marker finality observed while RPC workers active");return false;},"Marker finalized while expensive RPC clients remain active",40);
  if(rpc_cost_sustained){
   while(completed.load()<rpc_workers){auto height=rpc(ports[1],"eth_blockNumber");auto active=rpc_workers-completed.load();if(active)progress.push_back({{"height",height},{"activeWorkers",active},{"elapsedSeconds",std::chrono::duration<double>(std::chrono::steady_clock::now()-began).count()}});
    if(observation_window&&progress.size()>=20&&quantity(height)>=quantity(progress.front().at("height"))+5)break;
    std::this_thread::sleep_for(1s);
   }
  }
  }catch(const std::exception& e){control_error=e.what();}
  observation_done=true;
  for(auto& client:clients)client.join();
  std::cout<<Json{{"stage",rpc_overload?"rpc-overload-clients-finished":"rpc-cost-clients-finished"},{"fork",fork},{"defaultEstimates",default_estimates},{"progress",progress},{"controlError",control_error},{"errors",errors},{"samples",samples},{"requests",requests},{"activeWorkersWhenCertified",active_when_certified}}.dump()<<std::endl;
  if(!control_error.empty()){
   Json recovery=Json::array();for(auto port:ports){auto block=rpc(port,"eth_getBlockByNumber",{"latest",false});recovery.push_back({{"height",block.at("number")},{"blockHash",block.at("hash")},{"stateRoot",block.at("stateRoot")},{"probeStorage",rpc(port,"eth_getStorageAt",{cost_leaf,"0x0","latest"})},{"markerReceipt",rpc(port,"eth_getTransactionReceipt",{during_hash})}});}
   std::cout<<Json{{"stage","rpc-cost-after-control-failure"},{"fork",fork},{"peers",recovery}}.dump()<<std::endl;
  }
  check(control_error.empty(),"RPC control failed under load: "+control_error);
  for(const auto& error:errors)check(error.empty(),"bounded distributed RPC client failed: "+error);
  if(rpc_cost_sustained){check(progress.size()>=(rpc_overload?5u:20u),"RPC load did not span the required observations");check(quantity(progress.back().at("height"))>=quantity(progress.front().at("height"))+5,"BFT did not advance five blocks during sustained RPC load");}
  Json actual_rounds=Json::array();size_t actual_estimates=0,actual_calls=0,actual_identity=0;
  for(unsigned w=0;w<rpc_workers;++w){actual_rounds.push_back(samples[w].size());for(const auto& request:requests[w]){
   auto method=request.at("method").get<std::string>();if(method=="eth_estimateGas")++actual_estimates;else if(method=="eth_call")++actual_calls;else if(method=="eth_chainId")++actual_identity;
  }}
  size_t capacity_rejections=0,queue_expirations=0,deferred_verified=0;Json recovery_probes=Json::array();
  if(rpc_overload){
   for(const auto& worker:requests)for(const auto& request:worker)if(request.value("rpcCode",0)==-32005){
    if(request.at("error")=="RPC simulation capacity exceeded")++capacity_rejections;
    else if(request.at("error")=="RPC simulation queue wait exceeded")++queue_expirations;
   }
   check(capacity_rejections>0,"overload scenario did not actually reject admission");
   // No retry loop: each node must immediately serve fresh simulations after
   // clients finish, with the same normal RPC deadlines and exact results.
   const std::vector<std::string> recovery_tags=default_estimates?std::vector<std::string>{"latest","pending","default"}:std::vector<std::string>{"latest","pending"};
   for(unsigned node=0;node<4;++node){PersistentWebSocketRpc client(ports[node]);for(const auto& tag:recovery_tags){
    auto quote=client.call("eth_estimateGas",tag=="default"?Json::array({invocation}):Json::array({invocation,tag}));auto trial=invocation;trial["gas"]=quote;
    check(quantity(quote)>23000000&&quantity(quote)<=30000000,"post-overload estimate out of range");
    check(client.call("eth_call",Json::array({trial,tag=="default"?"pending":tag}))==hex(bytes(Int(42),32)),"post-overload gas quote cannot execute");
    recovery_probes.push_back({{"node",node},{"tag",tag},{"estimate",quote}});
   }}
   for(unsigned w=0;w<rpc_workers;++w){
    check(samples[w].size()==rpc_rounds,"overload worker did not complete every bounded round");
    PersistentWebSocketRpc client(ports[w/12]);
    for(const auto& quote:deferred_quotes[w]){auto trial=invocation;trial["gas"]=quote.at("gas");check(client.call("eth_call",Json::array({trial,quote.at("tag")}))==hex(bytes(Int(42),32)),"deferred gas quote cannot execute after overload");++deferred_verified;}
   }

  }
  check(observed.at("status")=="0x1","during-load marker failed");
  until([&]{for(auto port:ports)if(rpc(port,"eth_getTransactionReceipt",{during_hash})!=observed)return false;return true;},"Loaded validator agrees with certified marker",40);
  marker.nonce+=1;marker.value=11;auto after=SignedTransaction::sign(marker,keys[1]);auto after_hash=hex(after.hash());
  check(rpc(ports[0],"eth_sendRawTransaction",{hex(after.raw())})==after_hash,"loaded node accepts post-load transfer");
  until([&]{for(auto port:ports)if(rpc(port,"eth_getTransactionReceipt",{after_hash}).is_null())return false;return true;},"Post-load transfer certified on all peers",40);
  auto after_receipt=rpc(ports[0],"eth_getTransactionReceipt",{after_hash});check(after_receipt.at("status")=="0x1","post-load marker failed");
  VotingSet members;Bytes domain;{Blockchain inspector(root/"rpc-cost-inspector",Genesis::from_json(load(genesis)));members=inspector.voting_set();domain=inspector.genesis().block.header.hash();}
  auto final_height=quantity(after_receipt.at("blockNumber"));auto final=rpc(ports[0],"eth_getBlockByNumber",{quantity(final_height),false});Json certificates=Json::array();
  for(auto port:ports){
   check(rpc(port,"eth_getTransactionReceipt",{during_hash})==observed&&rpc(port,"eth_getTransactionReceipt",{after_hash})==after_receipt,"marker receipts diverged");
   check(quantity(rpc(port,"eth_getBalance",{recipient,"latest"}))==18,"marker transfers lost or duplicated");check(quantity(rpc(port,"eth_getStorageAt",{cost_leaf,"0x0","latest"}))==0,"RPC computation changed canonical contract storage");
   auto block=rpc(port,"eth_getBlockByNumber",{quantity(final_height),false});check(block.at("hash")==final.at("hash")&&block.at("stateRoot")==final.at("stateRoot"),"post-load canonical state differs");
   for(const auto& receipt:{observed,after_receipt}){auto height=quantity(receipt.at("blockNumber"));auto block=rpc(port,"nano_networkBlocks",{quantity(Int(height-1))}).at("blocks").at(0);auto cert=block.at("bftCertificate");auto round=u64(quantity(cert.at("round")));std::vector<FinalityVote> votes;
    for(const auto& wire:cert.at("votes"))votes.push_back(decode_vote_message(wire.get<std::string>(),members,1337,domain,u64(height),round));check(verify_commit_certificate(votes,members,u64(height),round,unhex(receipt.at("blockHash").get<std::string>()),1337,domain),"RPC-load certificate invalid");
    if(port==ports[0]){Json signers=Json::array();for(const auto& vote:votes)signers.push_back(vote.validator);certificates.push_back({{"height",quantity(height)},{"round",round},{"signers",signers}});}
   }
  }
  for(auto& relay:relays)relay->stop();for(auto& core:cores)core->stop();
  for(unsigned i=0;i<4;++i){Blockchain reopened(root/("node-"+std::to_string(i)),Genesis::from_json(load(genesis)));auto state=reopened.store().state_at(final_height);check(state&&hex(state->root())==final.at("stateRoot").get<std::string>(),"RPC-load persisted snapshot differs");check(reopened.audit().at("verified")==true,"RPC-load reopened history audit failed");}
  std::cout<<Json{{"scenario",rpc_overload?"bft-rpc-overload":"bft-rpc-cost"},{"fork",fork},{"defaultEstimates",default_estimates},{"loadedNodes",rpc_cost_all?Json::array({0,1,2,3}):Json::array({0})},{"loadedValidators",rpc_cost_all?Json(addresses):Json::array({addresses[0]})},{"transport",rpc_cost_websocket?"websocket":"http"},{"workers",rpc_workers},{"sustained",rpc_cost_sustained},{"progress",progress},{"roundsPerWorker",rpc_rounds},{"actualRoundsPerWorker",actual_rounds},{"observationWindow",observation_window},{"maximumWorkloadMs",observation_window?Json(std::chrono::duration_cast<std::chrono::milliseconds>(workload_deadline).count()):Json(nullptr)},{"overloadRejections",capacity_rejections},{"queueExpirations",queue_expirations},{"deferredQuotesVerified",deferred_verified},{"recoveryProbes",recovery_probes},{rpc_overload?"attemptedEstimateRounds":"expensiveEstimates",actual_estimates},{rpc_overload?"maximumExpensiveCallAttempts":"expensiveCalls",actual_calls},{"identityReads",actual_identity},{"activeWorkersWhenCertified",active_when_certified},{"certificates",certificates},{"markerHash",during_hash},{"markerBlockHash",observed.at("blockHash")},{"postLoadHash",after_hash},{"finalHeight",quantity(final_height)},{"stateRoot",final.at("stateRoot")},{"reopenedPeers",4},{"elapsedSeconds",std::chrono::duration<double>(std::chrono::steady_clock::now()-began).count()}}.dump()<<std::endl;return 0;
 }
 if(pool_balance){
  until([&]{for(auto port:ports)for(const auto& txs:pool_cases)if(rpc(port,"eth_getTransactionReceipt",{hex(txs[2].hash())}).is_null())return false;return true;},"All peers execute funded prefix",35);
  for(auto port:ports){
   check(rpc(port,"txpool_status")==Json{{"pending","0x0"},{"queued","0x3"}},"three individually affordable suffixes remain queued");
   auto content=rpc(port,"txpool_content");
   for(unsigned type=0;type<3;++type){const auto& txs=pool_cases[type];auto sender=public_address(public_key(bytes(Int(10+type),32)));auto recipient=public_address(public_key(bytes(Int(20+type),32)));
    check(rpc(port,"eth_getTransactionCount",{sender,"latest"})=="0x1","unfunded nonce must not execute");
    check(quantity(rpc(port,"eth_getBalance",{recipient,"latest"}))==pool_funds*60/100,"prefix transferred once");
    check(rpc(port,"eth_getTransactionReceipt",{hex(txs[0].hash())}).is_null()&&rpc(port,"eth_getTransactionReceipt",{hex(txs[1].hash())}).is_null(),"unexecuted suffix has no receipt");
    bool found=false;for(const auto& group:content.at("queued"))for(const auto& entry:group)if(entry.at("hash")==hex(txs[1].hash()))found=true;
    check(found,"exact nonce-two hash retained");
   }
  }
  for(const auto& txs:pool_cases)check(rpc(ports[0],"eth_sendRawTransaction",{hex(txs[3].raw())})==hex(txs[3].hash()),"cheaper same-fee recovery admitted after cleanup");
  until([&]{for(auto port:ports)for(const auto& txs:pool_cases)if(rpc(port,"eth_getTransactionReceipt",{hex(txs[1].hash())}).is_null())return false;return true;},"Recovery and retained suffix finalized on all peers",45);
  uint64_t final_height=0;std::map<uint64_t,std::string> blocks;
  for(const auto& txs:pool_cases)for(unsigned index:{2u,3u,1u}){auto receipt=rpc(ports[0],"eth_getTransactionReceipt",{hex(txs[index].hash())});check(receipt.at("status")=="0x1","pool recovery transfer succeeds");auto height=u64(quantity(receipt.at("blockNumber")));final_height=std::max(final_height,height);blocks[height]=receipt.at("blockHash");for(auto port:ports)check(rpc(port,"eth_getTransactionReceipt",{hex(txs[index].hash())})==receipt,"full recovery receipt agrees");}
  VotingSet members;Bytes domain;{Blockchain inspector(root/"pool-inspector",Genesis::from_json(load(genesis)));members=inspector.voting_set();domain=inspector.genesis().block.header.hash();}
  auto final=rpc(ports[0],"eth_getBlockByNumber",{quantity(Int(std::to_string(final_height))),false});
  for(auto port:ports){
   check(rpc(port,"txpool_status")==Json{{"pending","0x0"},{"queued","0x0"}},"recovered pools empty");
   for(unsigned type=0;type<3;++type){auto sender=public_address(public_key(bytes(Int(10+type),32)));auto recipient=public_address(public_key(bytes(Int(20+type),32)));
    check(rpc(port,"eth_getTransactionCount",{sender,"latest"})=="0x3","recovered nonce three");check(quantity(rpc(port,"eth_getBalance",{recipient,"latest"}))==pool_funds*70/100,"recovery value conserved");check(rpc(port,"eth_getTransactionReceipt",{hex(pool_cases[type][0].hash())}).is_null(),"unfunded transaction never executed");
   }
   auto other=rpc(port,"eth_getBlockByNumber",{quantity(Int(std::to_string(final_height))),false});check(other.at("hash")==final.at("hash")&&other.at("stateRoot")==final.at("stateRoot"),"pool recovery canonical state agrees");
   for(const auto& [height,hash]:blocks){auto cert=rpc(port,"nano_networkBlocks",{quantity(Int(std::to_string(height-1)))}).at("blocks").at(0).at("bftCertificate");auto round=u64(quantity(cert.at("round")));std::vector<FinalityVote> votes;for(const auto& wire:cert.at("votes"))votes.push_back(decode_vote_message(wire.get<std::string>(),members,1337,domain,height,round));check(verify_commit_certificate(votes,members,height,round,unhex(hash),1337,domain),"pool recovery certificate verified");}
  }
  for(auto& relay:relays)relay->stop();for(auto& core:cores)core->stop();
  for(unsigned i=0;i<4;++i){Blockchain reopened(root/("node-"+std::to_string(i)),Genesis::from_json(load(genesis)));auto state=reopened.store().state_at(Int(std::to_string(final_height)));check(state&&hex(state->root())==final.at("stateRoot").get<std::string>(),"pool recovery persisted state");check(reopened.audit().at("verified")==true,"pool recovery audit");}
  std::cout<<Json{{"scenario","pool-balance"},{"fork",fork},{"transactionTypes",3},{"executedTransfers",9},{"unfundedTransfersNotExecuted",3},{"reopenedPeers",4},{"finalHeight",final_height},{"stateRoot",final.at("stateRoot")}}.dump()<<std::endl;return 0;
 }
 if(loading){
  const bool trace_load=std::getenv("NANO_TEST_BFT_TRACE")!=nullptr;
  const unsigned per_worker=short_load?20:((load_crash||reconcile_load)?90:60);const std::string recipient="0x00000000000000000000000000000000000000b0";
  std::array<std::vector<SignedTransaction>,4> transfers;std::array<std::exception_ptr,4> errors{};std::atomic<unsigned> confirmations_during_send=0,submitted=0,completed=0;std::array<std::atomic<unsigned>,4> sent{};std::atomic<bool> marker_attempted=false;Json crash_evidence=nullptr;std::array<Json,4> traces{Json::array(),Json::array(),Json::array(),Json::array()};
  for(unsigned i=0;i<4;++i){auto nonce=quantity(rpc(ports[i],"eth_getTransactionCount",{addresses[i],"latest"}));for(unsigned n=0;n<per_worker;++n){Transaction tx;tx.chain_id=1337;tx.type=2;tx.nonce=nonce+n;tx.to=recipient;tx.value=1;tx.max_fee=2000000000;tx.priority_fee=1000000000;transfers[i].push_back(SignedTransaction::sign(tx,keys[i]));}}
  auto start=std::chrono::steady_clock::now();std::vector<std::jthread> clients;
  for(unsigned i=0;i<4;++i)clients.emplace_back([&,i]{try{PersistentRpc client(ports[load_crash?1+i%3:i]);for(unsigned n=0;n<per_worker;++n){const auto& tx=transfers[i][n];if(i==1&&n==10)marker_attempted=true;check(client.call("eth_sendRawTransaction",{hex(tx.raw())})==hex(tx.hash()),"concurrent BFT admission hash mismatch");sent[i]=n+1;++submitted;check(client.call("eth_chainId")=="0x539","identity read failed during live load");if(n+1<per_worker&&!client.call("eth_getTransactionReceipt",{hex(transfers[i][0].hash())}).is_null())++confirmations_during_send;if(short_load&&trace_load&&(n%5==0||n+1==per_worker)){auto snapshot=client.call("nano_bftExchange");Json row={{"sent",n+1},{"elapsedMs",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()},{"tip",client.call("eth_blockNumber")}};if(snapshot.is_object()){row["height"]=snapshot.at("height");row["round"]=snapshot.at("round");row["voteCount"]=snapshot.at("votes").size();row["proposalTransactions"]=snapshot.at("proposal").is_object()?snapshot.at("proposal").at("transactions").size():0;Json steps=Json::array();for(const auto& wire:snapshot.at("votes")){auto vote=Json::parse(wire.get<std::string>());steps.push_back(vote.at("step"));}row["voteSteps"]=steps;}traces[i].push_back(row);}std::this_thread::sleep_for(reconcile_load?1000ms:250ms);}}catch(...){errors[i]=std::current_exception();}++completed;});
  if(load_crash){
   until([&]{return submitted.load()>=8;},"Load is active before crash",10);check(sent[1]<=10,"crash marker already submitted");auto crash_height=quantity(rpc(ports[0],"eth_blockNumber"));
   relays[0]->stop();cores[0]->signal(SIGKILL);check(cores[0]->wait(10).code==-SIGKILL,"load validator did not die from SIGKILL");check(!marker_attempted.load(),"marker send began before confirmed validator death");
   const auto marker=hex(transfers[1][10].hash());
   try{
   until([&]{check(completed.load()<4,"senders finished before offline quorum demonstrated");for(unsigned i=1;i<4;++i)if(rpc(ports[i],"eth_getTransactionReceipt",{marker}).is_null())return false;return true;},"Three validators finalize new transfer while fourth is dead",35);
   }catch(const std::exception& failure){
    // Sample only after failure: diagnostics must not extend the acceptance window.
    Json peers=Json::array();
    for(unsigned i=1;i<4;++i){Json row={{"validator",addresses[i]},{"coreAlive",cores[i]->running()},{"relayAlive",relays[i]->running()}};
     try{row["tip"]=rpc(ports[i],"eth_blockNumber");row["markerReceipt"]=rpc(ports[i],"eth_getTransactionReceipt",{marker});auto snapshot=rpc(ports[i],"nano_bftExchange");row["height"]=snapshot.at("height");row["round"]=snapshot.at("round");row["proposalTransactions"]=snapshot.at("proposal").is_object()?snapshot.at("proposal").at("transactions").size():0;row["votes"]=Json::array();for(const auto& encoded:snapshot.at("votes"))row["votes"].push_back(Json::parse(encoded.get<std::string>()));}catch(const std::exception& diagnostic){row["diagnosticError"]=diagnostic.what();}
     if(std::getenv("NANO_TEST_NETWORK_TRACE"))row["relayOutput"]=relays[i]->output();
     peers.push_back(row);
    }
    std::cout<<Json{{"scenario","load-crash-failure-snapshot"},{"fork",fork},{"error",failure.what()},{"elapsedMs",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()},{"submitted",submitted.load()},{"completedSenders",completed.load()},{"peers",peers}}.dump()<<std::endl;
    throw;
   }
   auto receipt=rpc(ports[1],"eth_getTransactionReceipt",{marker});check(receipt.at("status")=="0x1"&&quantity(receipt.at("blockNumber"))>crash_height,"offline transfer not newly finalized");
   auto offline_confirmed_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
   std::cout<<Json{{"scenario","load-crash-offline-quorum"},{"fork",fork},{"elapsedMs",offline_confirmed_ms},{"submitted",submitted.load()},{"completedSenders",completed.load()},{"blockNumber",receipt.at("blockNumber")}}.dump()<<std::endl;
   auto before_restart=submitted.load();check(before_restart<4*per_worker,"no active senders at rejoin");
   auto cfg=root/"node-0.json";cores[0]=std::make_unique<Process>(std::vector<std::string>{argv[1],"serve",genesis.string(),(root/"node-0").string(),"0","127.0.0.1",cfg.string()},Env{{"TEST_BFT_KEY",hex(keys[0])}});ports[0]=cores[0]->port();
   std::vector<std::string> args{argv[2],std::to_string(ports[0]),(root/"load-rejoined-0").string(),"/ip4/127.0.0.1/tcp/0"};args.insert(args.end(),seeds.begin()+1,seeds.end());relays[0]=std::make_unique<Process>(args);relays[0]->event("listen");relays[0]->event("ready");
   check(completed.load()<4,"all senders finished before restarted relay was ready");
   crash_evidence={{"offlineConfirmationMsFromLoadStart",offline_confirmed_ms},{"crashedValidator",addresses[0]},{"heightBeforeCrash",quantity(crash_height)},{"offlineBlockHash",receipt.at("blockHash")},{"offlineBlockNumber",receipt.at("blockNumber")},{"submittedAtRestart",before_restart},{"submittedAtRelayReady",submitted.load()}};
  }
  for(auto& client:clients)client.join();for(const auto& error:errors)if(error)std::rethrow_exception(error);
  auto send_end=std::chrono::steady_clock::now();
  if(short_load&&trace_load)std::cout<<Json{{"scenario","short-load-trace"},{"fork",fork},{"confirmationsObservedDuringSend",confirmations_during_send.load()},{"workers",traces}}.dump()<<std::endl;
  check(confirmations_during_send>0,"no observed finality while senders were still active");
  until([&]{for(auto port:ports)for(const auto& group:transfers)for(const auto& tx:group)if(rpc(port,"eth_getTransactionReceipt",{hex(tx.hash())}).is_null())return false;return true;},"All concurrent transfers finalized on all four live peers",60);
  VotingSet members;Bytes domain;{Blockchain inspector(root/"load-inspector",Genesis::from_json(load(genesis)));members=inspector.voting_set();domain=inspector.genesis().block.header.hash();}
  std::map<uint64_t,std::string> blocks;uint64_t final_height=0;
  for(const auto& group:transfers)for(const auto& tx:group){auto receipt=rpc(ports[0],"eth_getTransactionReceipt",{hex(tx.hash())});check(receipt.at("status")=="0x1","loaded transfer reverted");auto height=u64(quantity(receipt.at("blockNumber")));final_height=std::max(final_height,height);blocks[height]=receipt.at("blockHash");for(unsigned i=1;i<4;++i)check(rpc(ports[i],"eth_getTransactionReceipt",{hex(tx.hash())})==receipt,"loaded receipt diverges across peers");}
  for(auto port:ports){check(quantity(rpc(port,"eth_getBalance",{recipient,"latest"}))==4*per_worker,"live load lost or duplicated transfer");for(const auto& [height,hash]:blocks){auto item=rpc(port,"nano_networkBlocks",{quantity(Int(std::to_string(height-1)))}).at("blocks").at(0).at("bftCertificate");auto round=u64(quantity(item.at("round")));std::vector<FinalityVote> votes;for(const auto& encoded:item.at("votes"))votes.push_back(decode_vote_message(encoded.get<std::string>(),members,1337,domain,height,round));check(verify_commit_certificate(votes,members,height,round,unhex(hash),1337,domain),"invalid loaded block certificate");if(load_crash&&hash==crash_evidence.at("offlineBlockHash").get<std::string>()){check(votes.size()==3,"offline block must be certified by three survivors");for(const auto& vote:votes)check(address(vote.validator)!=address(addresses[0]),"dead validator signed post-crash marker");}}}
  auto last=rpc(ports[0],"eth_getBlockByNumber",{quantity(Int(std::to_string(final_height))),false});for(unsigned i=1;i<4;++i){auto other=rpc(ports[i],"eth_getBlockByNumber",{quantity(Int(std::to_string(final_height))),false});check(other.at("hash")==last.at("hash")&&other.at("stateRoot")==last.at("stateRoot"),"loaded canonical state diverges");}
  for(auto& relay:relays)relay->stop();for(auto& core:cores)core->stop();
  if(reconcile_load){unsigned batches=0,nonempty=0;auto first=std::chrono::duration_cast<std::chrono::milliseconds>(start.time_since_epoch()).count(),last_send=std::chrono::duration_cast<std::chrono::milliseconds>(send_end.time_since_epoch()).count();
   for(const auto& relay:relays){std::istringstream lines(relay->output());std::string line;while(std::getline(lines,line)){auto record=Json::parse(line,nullptr,false);if(!record.is_object()||record.value("event","")!="test-gossip-periodic-batch")continue;auto time=record.at("steadyMs").get<int64_t>();if(time>=first&&time<=last_send){++batches;if(record.at("processed").get<unsigned>()>0)++nonempty;}}}
   std::cout<<Json{{"scenario","bft-periodic-inventory-proof"},{"fork",fork},{"batchesDuringSend",batches},{"nonemptyBatchesDuringSend",nonempty}}.dump()<<std::endl;check(batches>0&&nonempty>0,"no nonempty periodic inventory observed during BFT sending");
  }
  if(std::getenv("NANO_TEST_DROP_GOSSIP_REPLY_ONCE")){
   Json dropped=Json::array(),recovered=Json::array();
   for(unsigned i=1;i<4;++i){auto owner=seeds[i].substr(seeds[i].find_last_of('/')+1);bool injected=false;std::istringstream lines(relays[i]->output());std::string line;
    while(std::getline(lines,line)){auto record=Json::parse(line,nullptr,false);if(!record.is_object())continue;auto event=record.value("event","");if(event=="test-gossip-reply-dropped"){injected=true;record["receiver"]=owner;dropped.push_back(record);}else if(event=="test-gossip-retry-acknowledged"){record["sender"]=owner;recovered.push_back(record);}}
    check(injected,"surviving relay did not inject lost acknowledgement");
   }
   unsigned matched=0;for(const auto& loss:dropped)for(const auto& recovery:recovered)if(loss["sender"]==recovery["sender"]&&loss["receiver"]==recovery["peerId"]&&loss["cursor"]==recovery["failedCursor"]&&recovery["retryFrom"]<=loss["cursor"]&&recovery["acknowledgedCursor"]>=loss["cursor"]){++matched;break;}
   std::cout<<Json{{"scenario","gossip-ack-loss-proof"},{"fork",fork},{"matchedLostAcknowledgements",matched},{"dropped",dropped},{"recovered",recovered}}.dump()<<std::endl;
   check(matched>0,"no sender retry matched an injected lost acknowledgement");
  }
  for(unsigned i=0;i<4;++i){Blockchain reopened(root/("node-"+std::to_string(i)),Genesis::from_json(load(genesis)));auto historical=reopened.store().state_at(Int(std::to_string(final_height)));check(historical&&hex(historical->root())==last.at("stateRoot").get<std::string>(),"loaded state missing after reopen");check(reopened.audit().at("verified")==true,"loaded peer history audit failed");}
  std::cout<<Json{{"scenario",load_crash?"load-crash":"load"},{"crash",crash_evidence},{"fork",fork},{"workers",4},{"transactions",4*per_worker},{"intervalMsPerWorker",reconcile_load?1000:250},{"confirmationsObservedDuringSend",confirmations_during_send.load()},{"certifiedTransactionBlocks",blocks.size()},{"finalHeight",final_height},{"finalBlockHash",last.at("hash")},{"stateRoot",last.at("stateRoot")},{"reopenedPeers",4},{"elapsedSeconds",std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()}}.dump()<<std::endl;return 0;
 }
 if(departing){
  VotingSet before;Bytes domain;
  {Blockchain inspector(root/"departure-inspector",Genesis::from_json(load(genesis)));before=inspector.voting_set();domain=inspector.genesis().block.header.hash();}
  auto after=before;std::erase_if(after,[&](const auto& member){return address(member.first)==address(addresses[3]);});check(after.size()==3,"departure fixture membership");
  auto verify=[&](int port,const Json& receipt,const VotingSet& members){
   auto height=quantity(receipt.at("blockNumber"));auto block=rpc(port,"nano_networkBlocks",{quantity(Int(height-1))}).at("blocks").at(0);auto certificate=block.at("bftCertificate");auto round=u64(quantity(certificate.at("round")));std::vector<FinalityVote> votes;
   for(const auto& encoded:certificate.at("votes"))votes.push_back(decode_vote_message(encoded.get<std::string>(),members,1337,domain,u64(height),round));
   check(verify_commit_certificate(votes,members,u64(height),round,unhex(receipt.at("blockHash")),1337,domain),"departure certificate uses wrong membership");return votes;
  };
  auto selector=staking.at("methodIdentifiers").at("lockedUntil(address)").get<std::string>();
  auto until_block=quantity(rpc(ports[3],"eth_call",{Json{{"to",registry},{"data","0x"+selector+std::string(24,'0')+address(addresses[3]).substr(2)}},"latest"}));
  check(until_block>=100,"real contract lock period retained");
  Transaction withdrawal;withdrawal.chain_id=1337;withdrawal.to=registry;withdrawal.nonce=quantity(rpc(ports[3],"eth_getTransactionCount",{addresses[3],"latest"}));withdrawal.gas=500000;withdrawal.gas_price=2000000000;withdrawal.data=unhex(staking.at("methodIdentifiers").at("withdrawStake()"));
  auto premature=SignedTransaction::sign(withdrawal,keys[3]);auto early_hash=hex(premature.hash());check(rpc(ports[3],"eth_sendRawTransaction",{hex(premature.raw())})==early_hash,"early withdrawal admitted for execution");
  until([&]{for(auto port:ports)if(rpc(port,"eth_getTransactionReceipt",{early_hash}).is_null())return false;return true;},"Early withdrawal receives a receipt",40);
  for(auto port:ports){auto receipt=rpc(port,"eth_getTransactionReceipt",{early_hash});check(receipt.at("status")=="0x0"&&quantity(receipt.at("blockNumber"))<until_block,"early withdrawal must revert before unlock");verify(port,receipt,before);}
  check(!rpc(ports[3],"nano_bftExchange").is_null(),"early withdrawal retains voting role");
  Int reported=0;
  until([&]{Int lowest=quantity(rpc(ports[0],"eth_blockNumber"));for(unsigned i=1;i<4;++i)lowest=std::min(lowest,quantity(rpc(ports[i],"eth_blockNumber")));if(lowest>=reported+20){reported=lowest;std::cout<<"Waiting for actual staking unlock: height "<<lowest<<" / "<<until_block<<std::endl;}return lowest>=until_block;},"Actual 100-block staking lock expires on live network",400);
  withdrawal.nonce+=1;auto signed_withdrawal=SignedTransaction::sign(withdrawal,keys[3]);auto hash=hex(signed_withdrawal.hash());check(rpc(ports[3],"eth_sendRawTransaction",{hex(signed_withdrawal.raw())})==hash,"unlocked withdrawal accepted");
  until([&]{for(auto port:ports)if(rpc(port,"eth_getTransactionReceipt",{hash}).is_null())return false;return true;},"Departure finalizes on every node",45);
  auto activation=rpc(ports[3],"eth_getTransactionReceipt",{hash});check(activation.at("status")=="0x1","unlocked withdrawal succeeds");
  for(auto port:ports){auto receipt=rpc(port,"eth_getTransactionReceipt",{hash});check(receipt.at("status")=="0x1"&&receipt.at("blockHash")==activation.at("blockHash"),"departure receipt disagreement");verify(port,receipt,before);}
  check(rpc(ports[3],"nano_bftExchange").is_null(),"departed validator stops voting without restart");relays[3]->stop();
  Transaction transfer;transfer.chain_id=1337;transfer.nonce=withdrawal.nonce+1;transfer.to="0x00000000000000000000000000000000000000af";transfer.value=1;transfer.gas_price=2000000000;
  auto signed_transfer=SignedTransaction::sign(transfer,keys[3]);auto transfer_hash=hex(signed_transfer.hash());check(rpc(ports[0],"eth_sendRawTransaction",{hex(signed_transfer.raw())})==transfer_hash,"former validator retains ordinary account rights");
  until([&]{for(unsigned i=0;i<3;++i)if(rpc(ports[i],"eth_getTransactionReceipt",{transfer_hash}).is_null())return false;return true;},"Three remaining validators finalize after departure",45);
  auto receipt=rpc(ports[0],"eth_getTransactionReceipt",{transfer_hash});check(receipt.at("status")=="0x1"&&quantity(receipt.at("blockNumber"))>quantity(activation.at("blockNumber")),"post-departure transfer succeeds");
  for(unsigned i=0;i<3;++i){auto other=rpc(ports[i],"eth_getTransactionReceipt",{transfer_hash});check(other.at("blockHash")==receipt.at("blockHash"),"post-departure disagreement");auto votes=verify(ports[i],other,after);check(votes.size()==3,"all remaining validators certify");for(const auto& vote:votes)check(address(vote.validator)!=address(addresses[3]),"removed signer in new certificate");check(rpc(ports[i],"eth_getBalance",{*transfer.to,"latest"})=="0x1","former validator transfer applied once");}
  std::cout<<Json{{"scenario","departing"},{"fork",fork},{"unlockHeight",quantity(until_block)},{"activationHeight",activation.at("blockNumber")},{"activationHash",activation.at("blockHash")},{"finalBlockHash",receipt.at("blockHash")},{"finalBlockNumber",receipt.at("blockNumber")},{"remainingValidators",3},{"earlyWithdrawalReverted",true}}.dump()<<std::endl;return 0;
 }
 if(changing){
  VotingSet before;Bytes domain;
  {Blockchain inspector(root/"changing-inspector",Genesis::from_json(load(genesis)));before=inspector.voting_set();domain=inspector.genesis().block.header.hash();}
  auto after=before;for(auto& [member,power]:after)if(address(member)==address(addresses[0]))power+=minimum*3;
  auto verify=[&](int port,const Json& receipt,const VotingSet& members){
   auto height=quantity(receipt.at("blockNumber"));auto block=rpc(port,"nano_networkBlocks",{quantity(Int(height-1))}).at("blocks").at(0);
   auto certificate=block.at("bftCertificate");auto round=u64(quantity(certificate.at("round")));std::vector<FinalityVote> votes;
   for(const auto& encoded:certificate.at("votes"))votes.push_back(decode_vote_message(encoded.get<std::string>(),members,1337,domain,u64(height),round));
   check(verify_commit_certificate(votes,members,u64(height),round,unhex(receipt.at("blockHash")),1337,domain),"stake transition certificate uses wrong weights");return votes;
  };
  Transaction addition;addition.chain_id=1337;addition.nonce=quantity(rpc(ports[0],"eth_getTransactionCount",{addresses[0],"latest"}));addition.to=registry;addition.value=minimum*3;addition.gas=500000;addition.gas_price=2000000000;addition.data=unhex(staking.at("methodIdentifiers").at("addStake()"));
  auto signed_addition=SignedTransaction::sign(addition,keys[0]);auto addition_hash=hex(signed_addition.hash());
  check(rpc(ports[0],"eth_sendRawTransaction",{hex(signed_addition.raw())})==addition_hash,"stake addition accepted");
  until([&]{for(auto port:ports)if(rpc(port,"eth_getTransactionReceipt",{addition_hash}).is_null())return false;return true;},"Stake addition reaches every live peer",40);
  auto activation=rpc(ports[0],"eth_getTransactionReceipt",{addition_hash});check(activation.at("status")=="0x1","stake addition executed");
  for(auto port:ports){auto receipt=rpc(port,"eth_getTransactionReceipt",{addition_hash});check(receipt.at("blockHash")==activation.at("blockHash")&&receipt.at("status")=="0x1","stake activation disagreement");verify(port,receipt,before);}
  // Rebuild relay topology only: all four cores retain their live journals.
  auto connect=[&](bool pair,const std::string& phase){
   for(auto& relay:relays)relay->stop();relays.clear();seeds.clear();
   for(unsigned i=0;i<4;++i){std::vector<std::string> args{argv[2],std::to_string(ports[i]),(root/(phase+std::to_string(i))).string(),"/ip4/127.0.0.1/tcp/0"};
    if(pair){if(i%2==1)args.push_back(seeds.back());}else if(i>1)args.insert(args.end(),seeds.begin()+1,seeds.end());
    relays.push_back(std::make_unique<Process>(args));seeds.push_back(relays.back()->event("listen")["address"].get<std::string>());relays.back()->event("ready");
   }
  };
  connect(false,"changed-minority-");
  Transaction tx;tx.chain_id=1337;tx.nonce=addition.nonce+1;tx.to="0x00000000000000000000000000000000000000ae";tx.value=1;tx.gas_price=2000000000;
  auto signed_tx=SignedTransaction::sign(tx,keys[0]);auto hash=hex(signed_tx.hash());
  check(rpc(ports[0],"eth_sendRawTransaction",{hex(signed_tx.raw())})==hash,"heavy signer accepts post-change transfer");
  // Any in-flight old certificate may finish, but this new transaction must not.
  auto deadline=std::chrono::steady_clock::now()+12s;
  while(std::chrono::steady_clock::now()<deadline){for(auto port:ports)check(rpc(port,"eth_getTransactionReceipt",{hash}).is_null(),"4/7 signer finalized after stake change");for(unsigned i=0;i<4;++i)check(cores[i]->running()&&relays[i]->running(),"stake change process exited");std::this_thread::sleep_for(100ms);}
  connect(true,"changed-majority-");
  until([&]{return !rpc(ports[0],"eth_getTransactionReceipt",{hash}).is_null()&&!rpc(ports[1],"eth_getTransactionReceipt",{hash}).is_null();},"New 5/7 pair finalizes after live stake change",45);
  auto receipt=rpc(ports[0],"eth_getTransactionReceipt",{hash});check(receipt.at("status")=="0x1"&&quantity(receipt.at("blockNumber"))>quantity(activation.at("blockNumber")),"post-change transfer finalized");
  for(unsigned i=0;i<2;++i){auto other=rpc(ports[i],"eth_getTransactionReceipt",{hash});check(other.at("blockHash")==receipt.at("blockHash"),"changed majority disagrees");auto votes=verify(ports[i],other,after);check(votes.size()==2,"new weighted quorum must contain two signers");for(const auto& vote:votes)check(address(vote.validator)==address(addresses[0])||address(vote.validator)==address(addresses[1]),"unexpected changed quorum signer");check(rpc(ports[i],"eth_getBalance",{*tx.to,"latest"})=="0x1","post-change transfer applied once");}
  // Heal the partition after the new weighted quorum has certified the transfer.
  for(auto& relay:relays)relay->stop();relays.clear();seeds.clear();
  for(unsigned i=0;i<4;++i){
   std::vector<std::string> args{argv[2],std::to_string(ports[i]),(root/("changed-healed-"+std::to_string(i))).string(),"/ip4/127.0.0.1/tcp/0"};args.insert(args.end(),seeds.begin(),seeds.end());
   relays.push_back(std::make_unique<Process>(args));seeds.push_back(relays.back()->event("listen")["address"].get<std::string>());relays.back()->event("ready");
  }
  until([&]{for(auto port:ports){auto found=rpc(port,"eth_getTransactionReceipt",{hash});if(found.is_null()||found.at("blockHash")!=receipt.at("blockHash"))return false;}return true;},"Minority catches up after weighted quorum partition heals",45);
  auto final_height=quantity(receipt.at("blockNumber"));auto canonical=rpc(ports[0],"eth_getBlockByHash",{receipt.at("blockHash"),false});
  for(auto port:ports){auto found=rpc(port,"eth_getTransactionReceipt",{hash});check(found.at("status")=="0x1","healed transfer status differs");verify(port,found,after);
   auto head=rpc(port,"eth_getBlockByNumber",{quantity(final_height),false});check(head.at("hash")==canonical.at("hash")&&head.at("stateRoot")==canonical.at("stateRoot"),"healed peers disagree on transition descendant state");
   check(rpc(port,"eth_getBalance",{*tx.to,quantity(final_height)})=="0x1","healed peer duplicates or loses transfer");
  }
  for(auto& relay:relays)relay->stop();
  if(changing_crash){
   for(auto& core:cores)core->signal(SIGKILL);
   for(auto& core:cores)check(core->wait(5).code==-SIGKILL,"stake-change core did not exit from SIGKILL");
  }else for(auto& core:cores)core->stop();
  for(unsigned i=0;i<4;++i){Blockchain reopened(root/("node-"+std::to_string(i)),Genesis::from_json(load(genesis)));
   auto history=reopened.store().state_at(final_height);check(history&&hex(history->root())==canonical.at("stateRoot").get<std::string>(),"healed historical state lost after reopen");
   auto saved=reopened.store().receipt(signed_tx.hash());check(saved&&saved->success&&hex(saved->block_hash)==receipt.at("blockHash").get<std::string>(),"healed receipt lost after reopen");
   check(voting_set_hash(reopened.voting_set())==voting_set_hash(after),"reopened node reverted to old stake weights");
   check(reopened.audit().at("verified")==true,"stake-change history audit failed after reopen");
  }
  // Restart each core with its original config, chain database and vote journal.
  cores.clear();relays.clear();ports.clear();seeds.clear();
  for(unsigned i=0;i<4;++i){auto cfg=root/("node-"+std::to_string(i)+".json");
   cores.push_back(std::make_unique<Process>(std::vector<std::string>{argv[1],"serve",genesis.string(),(root/("node-"+std::to_string(i))).string(),"0","127.0.0.1",cfg.string()},Env{{"TEST_BFT_KEY",hex(keys[i])}}));ports.push_back(cores.back()->port());
  }
  for(unsigned i=0;i<4;++i){std::vector<std::string> args{argv[2],std::to_string(ports[i]),(root/("changed-restarted-peer-"+std::to_string(i))).string(),"/ip4/127.0.0.1/tcp/0"};args.insert(args.end(),seeds.begin(),seeds.end());
   relays.push_back(std::make_unique<Process>(args));seeds.push_back(relays.back()->event("listen")["address"].get<std::string>());relays.back()->event("ready");
  }
  Transaction resumed_tx;resumed_tx.chain_id=1337;resumed_tx.nonce=quantity(rpc(ports[2],"eth_getTransactionCount",{addresses[2],"pending"}));resumed_tx.to=tx.to;resumed_tx.value=2;resumed_tx.gas_price=2000000000;
  auto resumed_signed=SignedTransaction::sign(resumed_tx,keys[2]);auto resumed_hash=hex(resumed_signed.hash());
  check(rpc(ports[2],"eth_sendRawTransaction",{hex(resumed_signed.raw())})==resumed_hash,"restarted former minority accepts transfer");
  until([&]{for(auto port:ports)if(rpc(port,"eth_getTransactionReceipt",{resumed_hash}).is_null())return false;return true;},"Restarted reactors finalize using persisted new weights",45);
  auto resumed_receipt=rpc(ports[2],"eth_getTransactionReceipt",{resumed_hash});auto resumed_height=quantity(resumed_receipt.at("blockNumber"));check(resumed_height>final_height&&resumed_receipt.at("status")=="0x1","restart transfer did not advance finality");
  auto resumed_block=rpc(ports[2],"eth_getBlockByHash",{resumed_receipt.at("blockHash"),false});
  for(auto port:ports){auto found=rpc(port,"eth_getTransactionReceipt",{resumed_hash});check(found.at("blockHash")==resumed_receipt.at("blockHash")&&found.at("status")=="0x1","restarted peers disagree");verify(port,found,after);
   check(rpc(port,"eth_getBalance",{*tx.to,quantity(resumed_height)})=="0x3","restart lost or duplicated a transfer");
   check(rpc(port,"eth_getTransactionReceipt",{hash}).at("blockHash")==receipt.at("blockHash"),"restart replaced preexisting finalized receipt");
  }
  for(auto& relay:relays)relay->stop();for(auto& core:cores)core->stop();
  Json recovered_journals=Json::array();
  for(unsigned i=0;i<4;++i){Blockchain reopened(root/("node-"+std::to_string(i)),Genesis::from_json(load(genesis)));ChainStore journal(root/("journal-"+std::to_string(i)));
   auto history=reopened.store().state_at(resumed_height);check(history&&hex(history->root())==resumed_block.at("stateRoot").get<std::string>(),"post-restart state lost after reopen");
   // Certified import can finish just before shutdown, before the next reactor
   // round reconciles its separate vote journal. Recovery must verify the full
   // certificate sequence; merely rewriting the expected height is not safe.
   auto before=journal.voting_position(addresses[i],1337,domain);auto tip=reopened.tip().header.hash();auto state_root=reopened.state().root();
   reopened.recover_voting_progress(journal,addresses[i]);
   auto active=journal.voting_position(addresses[i],1337,domain);check(active&&active->height==u64(reopened.tip().header.number)+1&&active->validator_set_hash==voting_set_hash(after),"recovered voting journal not aligned with certified chain and new weights");
   check(reopened.tip().header.hash()==tip&&reopened.state().root()==state_root,"journal recovery modified canonical chain");
   recovered_journals.push_back({{"peer",i},{"tip",quantity(reopened.tip().header.number)},{"before",before?Json(before->height):Json(nullptr)},{"after",active->height},{"beforeSet",before?Json(hex(before->validator_set_hash)):Json(nullptr)},{"afterSet",hex(active->validator_set_hash)}});
   check(reopened.audit().at("verified")==true,"restarted chain history audit failed");
  }
  std::cout<<Json{{"scenario","stake-change"},{"fork",fork},{"restartMode",changing_crash?"SIGKILL":"clean"},{"confirmedSigkillExits",changing_crash?4:0},{"activationHeight",activation.at("blockNumber")},{"activationHash",activation.at("blockHash")},{"finalBlockHash",receipt.at("blockHash")},{"finalBlockNumber",receipt.at("blockNumber")},{"oldWeights",Json::array({1,1,1,1})},{"newWeights",Json::array({4,1,1,1})},{"certificateSigners",2},{"healedPeers",4},{"reopenedPeers",4},{"historicalStateRoot",canonical.at("stateRoot")},{"restartedReactors",4},{"verifiedJournals",4},{"journalRecovery",recovered_journals},{"restartTransactionHash",resumed_hash},{"restartBlockHash",resumed_receipt.at("blockHash")},{"restartBlockNumber",resumed_receipt.at("blockNumber")},{"restartStateRoot",resumed_block.at("stateRoot")},{"restartRecipientBalance","0x3"}}.dump()<<std::endl;
  return 0;
 }
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
  std::cout<<Json{{"scenario","joining"},{"fork",fork},{"activationHeight",quantity(activation_height)},{"requiredSigner",addresses[3]},{"finalBlockHash",receipt["blockHash"]},{"finalBlockNumber",receipt["blockNumber"]},{"recipientBalance","0x1"}}.dump()<<std::endl;
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
 std::cout<<Json{{"fork",fork},{"validators",ports.size()},{"restarted",restart},{"partitioned",partition},{"blockHash",expected["hash"]},{"stateRoot",expected["stateRoot"]},{"heightVerified","0x1"}}.dump()<<std::endl;
 std::cout<<"Distributed BFT proposal and vote exchange passed\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
