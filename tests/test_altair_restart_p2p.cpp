#include "support/altair_transition_rules.hpp"
#include "nano/consensus_altair_runtime.hpp"
#include "../network/altair_rpc_service.hpp"
#include "nano/http.hpp"
using namespace test;using namespace test::phase0_minimal;
struct RunningAltair {HttpServer& server;std::thread thread;explicit RunningAltair(HttpServer& s):server(s),thread([&s]{s.run();}){}~RunningAltair(){server.stop();thread.join();}};
int main(int argc,char** argv){try{
 check(argc==2||argc==5,"daemon binary or receiver restart arguments required");
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures",corpus=fixtures/"consensus-altair-fork-head-official",dir=corpus/"chain_no_attestations";
 const auto manifest=load(corpus/"manifest.json");for(const auto& row:manifest.at("cases"))if(row.at("name")=="chain_no_attestations")for(const auto& [path,info]:row.at("files").items())check(hex(sha256(binary(corpus/path))).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");
 const auto transition=altair_transition_rules(fixtures);const auto config=read(corpus/"minimal-config-v1.6.0-beta.0.yaml");const auto duration=parameter(config,"SECONDS_PER_SLOT")*1000;
 const AltairForkBlockRules rules{transition,duration,duration/3,parameter(config,"PROPOSER_SCORE_BOOST"),AltairProposerBoostProfile::Release160Beta0};
 const auto initial=initialize_altair_fork_store({binary(dir/"anchor_state.ssz"),std::nullopt},binary(dir/"anchor_block.ssz"),rules);
 const AltairStoreSnapshotLimits limits{32*1024*1024,100,10000,10000,1024*1024,16*1024*1024,4096,100000};

 const auto genesis=Genesis::from_json(Json{{"alloc",Json::object()}});const auto genesis_hash=genesis.block.header.hash();
 const auto& anchor=initial.blocks.at(initial.timed.view.anchor);ConsensusBytes4 version{};std::copy_n(anchor.state.ssz.begin()+52,4,version.begin());const auto validators=root_at(anchor.state.ssz,8);const auto fork=consensus_fork_data_root(version,validators);
 const auto identity=hex(altair_consensus_identity(rules,{initial.timed.view.anchor,anchor.header.state_root,validators,version},1337,genesis_hash));
 const AltairWireProfile profile{"0x539",hex(genesis_hash),hex(Bytes(fork.begin(),fork.begin()+4)),2*transition.maximum_block_bytes+1024,transition.maximum_block_bytes,512,40000};

 std::vector<Bytes> packets;std::string expected;uint64_t final_time=0;
 for(const auto& step:load(dir/"steps.json")){if(step.contains("block"))packets.push_back(binary(dir/(step.at("block").get<std::string>()+".ssz")));if(step.contains("tick"))final_time=step.at("tick").get<uint64_t>()*1000;if(step.contains("checks")&&step.at("checks").contains("head"))expected=step.at("checks").at("head").at("root");}
 check(packets.size()==2,"fixture block count changed");
 auto endpoint=[&](auto& reactor){return std::make_shared<AltairRpcService>(reactor,profile,4,2*transition.maximum_block_bytes,10s,[](const auto& check){if(check)check();},[]{return true;},identity,true);};
 if(argc==5){
  check(std::string(argv[1])=="--receiver","unknown child mode");
  // A separate core process, killed without destructors by the parent test.
  const fs::path directory=argv[2];ChainStore disk(directory/"consensus");disk.bind_chain(1337,genesis_hash,"altair-qualification");
  std::unique_ptr<AltairDurableReactor> reactor;
  if(std::string(argv[3])=="new"){reactor=std::make_unique<AltairDurableReactor>(disk,1337,genesis_hash,rules,limits,initial);reactor->tick(final_time);}
  else{ForkCheckpointToken token;token.sequence=std::stoull(argv[3]);token.digest=unhex(argv[4]);reactor=std::make_unique<AltairDurableReactor>(disk,1337,genesis_hash,rules,limits,token);}
  auto service=endpoint(*reactor);Blockchain execution(directory/"execution",genesis);Rpc rpc_service(execution,"fixture-token",AdmissionBudget::Clock::now,RpcSimulationLimits{},service);HttpServer server(rpc_service,"127.0.0.1",0);
  const auto current=reactor->current();const auto head=phase0_fork_choice_head(current.state->timed.view,{transition.epoch.ffg.slots_per_epoch,transition.attestation.duties.effective_balance_increment,rules.proposer_score_boost}).head;
  std::cout<<Json{{"event","receiver-core-ready"},{"port",server.port()},{"head",hex(Bytes(head.begin(),head.end()))},{"signedBlocks",current.signed_blocks->size()},{"sequence",current.token.sequence},{"digest",hex(current.token.digest)}}.dump()<<std::endl;
  server.run();return 0;
 }
 Temp root;ChainStore disk(root/"a-consensus");disk.bind_chain(1337,genesis_hash,"altair-qualification");AltairDurableReactor source_state(disk,1337,genesis_hash,rules,limits,initial);source_state.tick(final_time);check(source_state.block(packets[0]),"source first block rejected");auto service=endpoint(source_state);
 Blockchain execution(root/"a-execution",genesis);Rpc rpc_service(execution,"fixture-token",AdmissionBudget::Clock::now,RpcSimulationLimits{},service);HttpServer server(rpc_service,"127.0.0.1",0);RunningAltair running(server);
 auto child=[&](const std::string& sequence,const std::string& digest){return std::make_unique<Process>(std::vector<std::string>{argv[0],"--receiver",(root/"receiver").string(),sequence,digest});};
 auto core=child("new","0x");auto ready=core->event("receiver-core-ready");int port=ready.at("port");
 Process sender({argv[1],std::to_string(server.port()),(root/"a-key").string(),"/ip4/127.0.0.1/tcp/0"},{{"NANO_NETWORK_TOKEN","fixture-token"},{"NANO_TEST_NETWORK_TRACE","1"}});
 const auto source_address=sender.event("listen").at("address").get<std::string>();sender.event("altair-ingress-ready");
 auto receiver_daemon=[&]{return std::make_unique<Process>(std::vector<std::string>{argv[1],std::to_string(port),(root/"b-key").string(),"/ip4/127.0.0.1/tcp/0",source_address},Env{{"NANO_NETWORK_TOKEN","fixture-token"},{"NANO_TEST_NETWORK_TRACE","1"}});};
 auto receiver=receiver_daemon();receiver->event("altair-ingress-ready");
 const auto first=service->inventory("0x",1).at("blocks")[0].at("root");
 check(sender.event("test-altair-block-relayed").at("root")==first,"first block was not acknowledged");
 const auto old_status=rpc(port,"nano_networkStatus");
 // Stop the relay before taking the externally retained restart commitment;
 // kill the core itself, retaining its WAL without graceful destruction.
 check(receiver->stop().code==0,"receiver relay shutdown failed");receiver.reset();
 auto token=rpc(port,"nano_altairStatus",Json::array({"fixture-token"}));
 check(rpc(port,"nano_altairInventory",Json::array({"fixture-token","0x","0xa"})).at("blocks").size()==1,"receiver did not persist first block");
 core->signal(SIGKILL);check(core->wait().code==-SIGKILL,"receiver core did not crash");core.reset();
 check(source_state.block(packets[1]),"source second block rejected while receiver offline");
 // Decimal sequence is a test-process argument; the RPC quantity remains canonical hex.
 auto sequence=std::stoull(token.at("sequence").get<std::string>(),nullptr,16);
 core=child(std::to_string(sequence),token.at("digest"));ready=core->event("receiver-core-ready");port=ready.at("port");
 check(ready.at("signedBlocks")==1&&ready.at("head")==first&&ready.at("sequence")==sequence&&ready.at("digest")==token.at("digest"),"crash restart changed authenticated checkpoint");
 check(rpc(port,"nano_networkStatus").at("gossip_epoch")!=old_status.at("gossip_epoch"),"receiver restart reused gossip session");
 receiver=receiver_daemon();receiver->event("altair-ingress-ready");
 until([&]{return rpc(port,"nano_altairInventory",Json::array({"fixture-token","0x","0xa"})).at("blocks").size()==2;},"receiver caught up after core SIGKILL",45);
 for(const auto& [id,bytes]:*source_state.current().signed_blocks){const auto item=rpc(port,"nano_altairBlock",Json::array({"fixture-token",hex(Bytes(id.begin(),id.end()))}));check(altair_unbase64(item.at("envelope").at("sszBase64").get<std::string>(),transition.maximum_block_bytes)==*bytes,"restart relay altered signed block bytes");}
 check(receiver->stop().code==0,"receiver relay final shutdown failed");receiver.reset();token=rpc(port,"nano_altairStatus",Json::array({"fixture-token"}));
 core->signal(SIGKILL);check(core->wait().code==-SIGKILL,"second core crash failed");core.reset();
 sequence=std::stoull(token.at("sequence").get<std::string>(),nullptr,16);core=child(std::to_string(sequence),token.at("digest"));ready=core->event("receiver-core-ready");
 check(sender.running(),"sender exited during receiver recovery");
 check(ready.at("signedBlocks")==2&&ready.at("head")==expected&&ready.at("sequence")==sequence&&ready.at("digest")==token.at("digest"),"second crash lost received block or fork-choice head");
 core->signal(SIGKILL);check(core->wait().code==-SIGKILL,"test core cleanup failed");check(sender.stop().code==0,"sender daemon stopped unexpectedly");service->stop();service->join();
 std::cout<<Json{{"scenario","altair-core-crash-catchup"},{"receiverCoreRestarts",2},{"officialBlocks",2},{"senderKeptRunning",true},{"newGossipSession",true},{"fixtureClock",true},{"executionBound",false},{"powerLossTested",false},{"distributedFinality",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
