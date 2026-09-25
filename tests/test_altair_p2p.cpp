#include "support/altair_transition_rules.hpp"
#include "nano/consensus_altair_runtime.hpp"
#include "../network/altair_rpc_service.hpp"
#include "nano/http.hpp"
using namespace test;using namespace test::phase0_minimal;
struct RunningAltair {HttpServer& server;std::thread thread;explicit RunningAltair(HttpServer& s):server(s),thread([&s]{s.run();}){}~RunningAltair(){server.stop();thread.join();}};
int main(int argc,char** argv){try{
 check(argc==3,"daemon and qualification peer binaries required");
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures",corpus=fixtures/"consensus-altair-fork-head-official",dir=corpus/"chain_no_attestations";
 const auto manifest=load(corpus/"manifest.json");for(const auto& row:manifest.at("cases"))if(row.at("name")=="chain_no_attestations")for(const auto& [path,info]:row.at("files").items())check(hex(sha256(binary(corpus/path))).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");
 const auto transition=altair_transition_rules(fixtures);const auto config=read(corpus/"minimal-config-v1.6.0-beta.0.yaml");const auto duration=parameter(config,"SECONDS_PER_SLOT")*1000;
 const AltairForkBlockRules rules{transition,duration,duration/3,parameter(config,"PROPOSER_SCORE_BOOST"),AltairProposerBoostProfile::Release160Beta0};
 const auto initial=initialize_altair_fork_store({binary(dir/"anchor_state.ssz"),std::nullopt},binary(dir/"anchor_block.ssz"),rules);
 const AltairStoreSnapshotLimits limits{32*1024*1024,100,10000,10000,1024*1024,16*1024*1024,4096,100000};
 Temp root;const auto genesis=Genesis::from_json(Json{{"alloc",Json::object()}});const auto genesis_hash=genesis.block.header.hash();
 ChainStore disk(root/"consensus");disk.bind_chain(1337,genesis_hash,"altair-qualification");AltairDurableReactor reactor(disk,1337,genesis_hash,rules,limits,initial);
 const auto& anchor=initial.blocks.at(initial.timed.view.anchor);ConsensusBytes4 version{};std::copy_n(anchor.state.ssz.begin()+52,4,version.begin());const auto validators=root_at(anchor.state.ssz,8);const auto fork=consensus_fork_data_root(version,validators);
 const auto identity=hex(altair_consensus_identity(rules,{initial.timed.view.anchor,anchor.header.state_root,validators,version},1337,genesis_hash));
 const AltairWireProfile profile{"0x539",hex(genesis_hash),hex(Bytes(fork.begin(),fork.begin()+4)),2*transition.maximum_block_bytes+1024,transition.maximum_block_bytes,512,40000};
 std::atomic<bool> healthy=true;
 // Fixture time is advanced explicitly from official steps. This test validates
 // transport/commit integration, not host UTC synchronisation or live finality.
 auto endpoint=std::make_shared<AltairRpcService>(reactor,profile,4,2*transition.maximum_block_bytes,10s,[&](const auto& check){if(check)check();if(!healthy.load())throw std::runtime_error("fixture local clock unavailable");},[&]{return healthy.load();},identity,true);
 Blockchain execution(root/"execution",genesis);Rpc rpc_service(execution,"fixture-token",AdmissionBudget::Clock::now,RpcSimulationLimits{},endpoint);HttpServer server(rpc_service,"127.0.0.1",0);RunningAltair running(server);
 Process daemon({argv[1],std::to_string(server.port()),(root/"peer-key").string(),"/ip4/127.0.0.1/tcp/0"},{{"NANO_NETWORK_TOKEN","fixture-token"}});
 const auto address=daemon.event("listen").at("address").get<std::string>();daemon.event("ready");check(daemon.event("altair-ingress-ready").at("consensusIdentity")==identity,"daemon enabled different profile");
 auto status=rpc(server.port(),"nano_networkStatus");status["altair"]={{"protocol","/nano/altair/1.0.0"},{"consensusIdentity",identity},{"forkDigest",profile.fork_digest}};
 unsigned probes=0;std::set<std::string> peer_ids;
 auto probe=[&](const Json& advertised,const std::vector<std::string>& frames){
  const auto file=root/("probe-"+std::to_string(++probes)+".json");save(file,{{"status",advertised},{"frames",frames}});
  Process peer({argv[2],address,file.string()});const auto result=peer.event("altair-probe-result");check(peer.wait().code==0,"qualification peer failed");check(peer_ids.insert(result.at("peerId").get<std::string>()).second,"qualification peers reused an identity");return result.at("results");
 };
 auto frame=[&](const std::string& kind,const Bytes& payload){return Json{{"chainId",profile.chain_id},{"genesisHash",profile.genesis_hash},{"forkDigest",profile.fork_digest},{"kind",kind},{"ssz",hex(payload)}}.dump();};
 const auto invalid=frame("attestation",{0,1});const auto start=reactor.current().token;const auto watermark=endpoint->jobs().at("highWater");
 check(probe(nullptr,{invalid})==Json::array({nullptr}),"unhandshaken peer reached admission");
 auto legacy=status;legacy.erase("altair");check(probe(legacy,{invalid})==Json::array({nullptr}),"legacy peer reached Altair admission");
 auto other=status;other["altair"]["consensusIdentity"]="0x"+std::string(64,'f');check(probe(other,{invalid})==Json::array({nullptr}),"same fork digest with foreign rules reached admission");
 auto bad=Json::parse(invalid);bad["forkDigest"]="0x00000000";
 const auto duplicate=std::string("{\"chainId\":\"0x539\",")+invalid.substr(1);
 check(probe(status,{bad.dump(),duplicate,frame("attestation",Bytes(513,0))})==Json::array({nullptr,nullptr,nullptr}),"invalid frame reached consensus");
 check(endpoint->jobs().at("highWater")==watermark&&reactor.current().token==start,"rejected peer/frame changed core admission");
 check(probe(status,{invalid})==Json::array({Json{{"accepted",false}}}),"core rejection not returned to compatible peer");
 check(probe(status,{frame("block",Bytes(transition.maximum_block_bytes,0))})==Json::array({Json{{"accepted",false}}}),"full configured block bound failed before core validation");
 unsigned blocks=0;
 for(const auto& step:load(dir/"steps.json")){
  if(step.contains("tick"))reactor.tick(step.at("tick").get<uint64_t>()*1000);
  else if(step.contains("block")){
   const auto before=reactor.current().token;const auto packet=binary(dir/(step.at("block").get<std::string>()+".ssz"));
   check(probe(status,{frame("block",packet)})==Json::array({Json{{"accepted",true}}}),"official block rejected through real libp2p");
   const auto after=reactor.current();check(after.token.sequence==before.sequence+1&&disk.fork_checkpoint(1337,genesis_hash,limits.envelope_bytes,after.token).has_value(),"P2P reply before durable commit");++blocks;
  }else if(step.contains("checks")&&step.at("checks").contains("head")){
   const auto head=phase0_fork_choice_head(reactor.current().state->timed.view,{transition.epoch.ffg.slots_per_epoch,transition.attestation.duties.effective_balance_increment,rules.proposer_score_boost}).head;
   check(hex(Bytes(head.begin(),head.end()))==step.at("checks").at("head").at("root").get<std::string>(),"P2P head differs from official expected head");
  }
 }
 const auto archived=reactor.current();std::set<std::string> exported;std::string cursor="0x";
 do{const auto page=rpc(server.port(),"nano_altairInventory",Json::array({"fixture-token",cursor,"0x1"}));check(page.at("blocks").size()==1&&page.at("missingSignedBlocks")==0,"invalid inventory page");const auto id=page.at("blocks")[0].at("root").get<std::string>();check(exported.insert(id).second,"duplicate inventory page");
  const auto payload=rpc(server.port(),"nano_altairBlock",Json::array({"fixture-token",id}));check(payload.at("available")==true,"durable block unavailable over RPC");ConsensusRoot key{};const auto decoded=unhex(id);std::copy(decoded.begin(),decoded.end(),key.begin());check(altair_unbase64(payload.at("envelope").at("sszBase64").get<std::string>(),transition.maximum_block_bytes)==*archived.signed_blocks->at(key),"export changed signed block bytes");
  if(page.at("next").is_null())break;cursor=page.at("next").get<std::string>();
 }while(true);check(exported.size()==blocks,"inventory omitted archived block");
 for(const auto* method:{"nano_altairInventory","nano_altairBlock"}){bool denied=false;try{auto p=Json::array({"wrong",method==std::string("nano_altairBlock")?*exported.begin():"0x"});if(method==std::string("nano_altairInventory"))p.push_back("0x1");rpc(server.port(),method,p);}catch(const RpcError& e){denied=e.code==-32001;}check(denied,"archive read bypassed authentication");}
 healthy=false;const auto stable=reactor.current().token;check(probe(status,{invalid})==Json::array({nullptr})&&reactor.current().token==stable,"local failure returned consensus acceptance/rejection");
 check(endpoint->jobs().at("jobs").empty(),"daemon did not retire finished tickets");
 check(rpc(server.port(),"eth_chainId")=="0x539","EVM RPC unavailable after P2P tests");
 check(daemon.stop().code==0,"daemon failed shutdown");endpoint->stop();endpoint->join();
 std::cout<<Json{{"scenario","altair-libp2p-ingress"},{"maximumPayloadBytes",transition.maximum_block_bytes},{"officialBlocks",blocks},{"distinctAuthenticatedPeers",peer_ids.size()},{"realDaemon",true},{"realCoreHttp",true},{"fixtureClock",true},{"executionBound",false},{"distributedFinality",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
