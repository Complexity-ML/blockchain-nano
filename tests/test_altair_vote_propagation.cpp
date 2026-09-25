#include "support/altair_transition_rules.hpp"
#include "nano/consensus_altair_runtime.hpp"
#include "../network/altair_rpc_service.hpp"
#include "nano/http.hpp"
using namespace test;using namespace test::phase0_minimal;
struct RunningAltair {HttpServer& server;std::thread thread;explicit RunningAltair(HttpServer& s):server(s),thread([&s]{s.run();}){}~RunningAltair(){server.stop();thread.join();}};
int main(int argc,char** argv){try{
 check(argc==2,"daemon binary required");
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures",corpus=fixtures/"consensus-altair-fork-head-official",dir=corpus/"shorter_chain_but_heavier_weight";
 const auto manifest=load(corpus/"manifest.json");for(const auto& row:manifest.at("cases"))if(row.at("name")=="shorter_chain_but_heavier_weight")for(const auto& [path,info]:row.at("files").items())check(hex(sha256(binary(corpus/path))).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");
 const auto transition=altair_transition_rules(fixtures);const auto config=read(corpus/"minimal-config-v1.6.0-beta.0.yaml");const auto duration=parameter(config,"SECONDS_PER_SLOT")*1000;
 const AltairForkBlockRules rules{transition,duration,duration/3,parameter(config,"PROPOSER_SCORE_BOOST"),AltairProposerBoostProfile::Release160Beta0};
 const auto initial=initialize_altair_fork_store({binary(dir/"anchor_state.ssz"),std::nullopt},binary(dir/"anchor_block.ssz"),rules);
 const AltairStoreSnapshotLimits limits{32*1024*1024,100,10000,10000,1024*1024,16*1024*1024,4096,100000};
 Temp root;const auto genesis=Genesis::from_json(Json{{"alloc",Json::object()}});const auto genesis_hash=genesis.block.header.hash();

 const auto& anchor=initial.blocks.at(initial.timed.view.anchor);ConsensusBytes4 version{};std::copy_n(anchor.state.ssz.begin()+52,4,version.begin());const auto validators=root_at(anchor.state.ssz,8);const auto fork=consensus_fork_data_root(version,validators);
 const auto identity=hex(altair_consensus_identity(rules,{initial.timed.view.anchor,anchor.header.state_root,validators,version},1337,genesis_hash));
 const AltairWireProfile profile{"0x539",hex(genesis_hash),hex(Bytes(fork.begin(),fork.begin()+4)),2*transition.maximum_block_bytes+1024,transition.maximum_block_bytes,512,40000};
 ChainStore da(root/"a-consensus"),db(root/"b-consensus");da.bind_chain(1337,genesis_hash,"altair-qualification");db.bind_chain(1337,genesis_hash,"altair-qualification");
 AltairDurableReactor a(da,1337,genesis_hash,rules,limits,initial),b(db,1337,genesis_hash,rules,limits,initial);
 auto endpoint=[&](auto& reactor){return std::make_shared<AltairRpcService>(reactor,profile,4,2*transition.maximum_block_bytes,10s,[](const auto& check){if(check)check();},[]{return true;},identity,true);};
 auto ea=endpoint(a),eb=endpoint(b);
 Blockchain xa(root/"a-execution",genesis),xb(root/"b-execution",genesis);
 Rpc ra(xa,"fixture-token",AdmissionBudget::Clock::now,RpcSimulationLimits{},ea),rb(xb,"fixture-token",AdmissionBudget::Clock::now,RpcSimulationLimits{},eb);
 HttpServer sa(ra,"127.0.0.1",0),sb(rb,"127.0.0.1",0);RunningAltair ha(sa),hb(sb);

 Bytes vote;std::string expected;const auto steps=load(dir/"steps.json");
 for(const auto& step:steps){
  if(step.contains("tick"))a.tick(step.at("tick").get<uint64_t>()*1000);
  else if(step.contains("block"))check(a.block(binary(dir/(step.at("block").get<std::string>()+".ssz"))),"source official block rejected");
  else if(step.contains("attestation")){vote=binary(dir/(step.at("attestation").get<std::string>()+".ssz"));break;}
 }
 check(!vote.empty()&&a.current().signed_blocks->size()==4,"fixture prerequisites changed");
 for(const auto& step:steps)if(step.contains("checks")&&step.at("checks").contains("head"))expected=step.at("checks").at("head").at("root");
 b.tick(a.current().state->timed.clock.time_ms);
 Process receiver({argv[1],std::to_string(sb.port()),(root/"b-key").string(),"/ip4/127.0.0.1/tcp/0"},{{"NANO_NETWORK_TOKEN","fixture-token"},{"NANO_TEST_NETWORK_TRACE","1"},{"NANO_TEST_DROP_ALTAIR_VOTE_REPLY_ONCE","1"}});
 const auto address=receiver.event("listen").at("address").get<std::string>();receiver.event("altair-ingress-ready");
 Process sender({argv[1],std::to_string(sa.port()),(root/"a-key").string(),"/ip4/127.0.0.1/tcp/0",address},{{"NANO_NETWORK_TOKEN","fixture-token"},{"NANO_TEST_NETWORK_TRACE","1"}});sender.event("altair-ingress-ready");
 until([&]{return b.current().signed_blocks->size()==4;},"receiver obtained vote dependencies",45);
 auto head=[&](const AltairPublishedStore& state){const auto id=phase0_fork_choice_head(state.state->timed.view,{transition.epoch.ffg.slots_per_epoch,transition.attestation.duties.effective_balance_increment,rules.proposer_score_boost}).head;return hex(Bytes(id.begin(),id.end()));};
 check(b.current().state->timed.view.latest_messages.empty()&&head(b.current())!=expected,"fixture does not distinguish block propagation from vote effect");
 check(a.vote_ssz(vote)&&head(a.current())==expected,"source vote differs from official expected head");
 const auto id=hex(sha256(vote));const auto page=rpc(sa.port(),"nano_altairVoteInventory",Json::array({"fixture-token","0x","0x1"}));check(page.at("votes").size()==1&&page.at("votes")[0].at("root")==id,"vote inventory root mismatch");
 for(const auto* method:{"nano_altairVote","nano_altairVoteInventory"}){bool denied=false;try{auto args=Json::array({"wrong",method==std::string("nano_altairVote")?id:"0x"});if(method==std::string("nano_altairVoteInventory"))args.push_back("0x1");rpc(sa.port(),method,args);}catch(const RpcError& error){denied=error.code==-32001;}check(denied,"vote read bypassed authentication");}
 receiver.event("test-altair-vote-reply-dropped");check(sender.event("test-altair-vote-relayed").at("root")==id,"vote lost ACK was not retried");
 const auto received=b.current();check(head(received)==expected&&received.state->timed.view.latest_messages==a.current().state->timed.view.latest_messages,"P2P vote did not update fork choice");
 check(received.signed_votes->size()==1&&*received.signed_votes->begin()->second==vote,"vote payload changed or duplicated");
 const auto exported=rpc(sb.port(),"nano_altairVote",Json::array({"fixture-token",id}));check(exported.at("available")==true&&altair_unbase64(exported.at("envelope").at("sszBase64").get<std::string>(),profile.attestation_bytes)==vote,"receiver export changed vote");
 check(sender.stop().code==0&&receiver.stop().code==0,"daemon shutdown failed");
 // Pruning changes the catalogue epoch; an old cursor cannot silently skip
 // votes in a later catalogue, and an expired payload cannot be re-exported.
 a.tick(2*duration*transition.epoch.ffg.slots_per_epoch);
 const auto expired=rpc(sa.port(),"nano_altairVoteInventory",Json::array({"fixture-token","0x","0x1"}));check(expired.at("votes").empty()&&expired.at("catalogEpoch")!=page.at("catalogEpoch"),"pruning did not reset vote catalogue");
 check(rpc(sa.port(),"nano_altairVote",Json::array({"fixture-token",id})).at("available")==false,"expired vote still exportable");
 bool stale=false;try{rpc(sa.port(),"nano_altairVoteInventory",Json::array({"fixture-token",id,"0x1"}));}catch(const RpcError& error){stale=error.code==-32602;}check(stale,"expired cursor accepted");
 ea->stop();eb->stop();ea->join();eb->join();
 std::cout<<Json{{"scenario","altair-vote-propagation"},{"officialBlocks",4},{"officialVotes",1},{"realDaemons",2},{"lostVoteAckRetried",true},{"remoteHeadChangedByVote",true},{"expiryResetsCatalogue",true},{"fixtureClock",true},{"executionBound",false},{"distributedFinality",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
