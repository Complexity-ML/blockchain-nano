#include "support/altair_transition_rules.hpp"
#include "nano/consensus_altair_runtime.hpp"
#include "../network/altair_rpc_service.hpp"
#include "nano/http.hpp"
using namespace test;using namespace test::phase0_minimal;
struct RunningAltair {HttpServer& server;std::thread thread;explicit RunningAltair(HttpServer& s):server(s),thread([&s]{s.run();}){}~RunningAltair(){server.stop();thread.join();}};
int main(int argc,char** argv){try{
 check(argc==2,"daemon binary required");
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures",corpus=fixtures/"consensus-altair-fork-head-official",dir=corpus/"chain_no_attestations";
 const auto manifest=load(corpus/"manifest.json");for(const auto& row:manifest.at("cases"))if(row.at("name")=="chain_no_attestations")for(const auto& [path,info]:row.at("files").items())check(hex(sha256(binary(corpus/path))).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");
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
 std::vector<Bytes> packets;std::string expected;uint64_t final_time=0;
 for(const auto& step:load(dir/"steps.json")){if(step.contains("block"))packets.push_back(binary(dir/(step.at("block").get<std::string>()+".ssz")));if(step.contains("tick"))final_time=step.at("tick").get<uint64_t>()*1000;if(step.contains("checks")&&step.at("checks").contains("head"))expected=step.at("checks").at("head").at("root");}
 check(packets.size()==2,"fixture block count changed");a.tick(final_time);b.tick(final_time);check(a.block(packets[0]),"source first block rejected");
 const auto first=ea->inventory("0x",1);const auto cursor=first.at("blocks")[0].at("root").get<std::string>();
 Process receiver({argv[1],std::to_string(sb.port()),(root/"b-key").string(),"/ip4/127.0.0.1/tcp/0"},{{"NANO_NETWORK_TOKEN","fixture-token"},{"NANO_TEST_NETWORK_TRACE","1"},{"NANO_TEST_DROP_ALTAIR_REPLY_ONCE","1"}});
 const auto address=receiver.event("listen").at("address").get<std::string>();receiver.event("altair-ingress-ready");
 auto sender=[&]{return std::make_unique<Process>(std::vector<std::string>{argv[1],std::to_string(sa.port()),(root/"a-key").string(),"/ip4/127.0.0.1/tcp/0",address},Env{{"NANO_NETWORK_TOKEN","fixture-token"},{"NANO_TEST_NETWORK_TRACE","1"}});};
 auto source=sender();source->event("altair-ingress-ready");receiver.event("test-altair-reply-dropped");
 check(source->event("test-altair-block-relayed").at("root")==cursor,"lost ACK retry failed");
 check(b.current().signed_blocks->size()==1,"first block not durably received");check(source->stop().code==0,"source shutdown failed");
 // A clock-only checkpoint and an append preserve the existing catalogue cursor.
 a.tick(final_time+1);check(ea->inventory(cursor,1).at("catalogEpoch")==first.at("catalogEpoch"),"tick changed catalogue epoch");
 check(a.block(packets[1]),"source second block rejected");const auto next=ea->inventory(cursor,1);
 check(next.at("catalogEpoch")==first.at("catalogEpoch")&&next.at("blocks").size()==1&&next.at("blocks")[0].at("root")==expected,"append invalidated inventory cursor");
 source=sender();source->event("altair-ingress-ready");
 until([&]{return b.current().signed_blocks->size()==2;},"automatic propagation after sender restart",45);
 const auto actual=b.current();for(const auto& [id,bytes]:*a.current().signed_blocks)check(*actual.signed_blocks->at(id)==*bytes,"relay changed durable SSZ bytes");
 const auto head=phase0_fork_choice_head(actual.state->timed.view,{transition.epoch.ffg.slots_per_epoch,transition.attestation.duties.effective_balance_increment,rules.proposer_score_boost}).head;
 check(hex(Bytes(head.begin(),head.end()))==expected,"receiver head differs from official fixture");
 check(db.fork_checkpoint(1337,genesis_hash,limits.envelope_bytes,actual.token).has_value(),"receiver archive not persisted");
 check(source->stop().code==0&&receiver.stop().code==0,"daemon shutdown failed");ea->stop();eb->stop();ea->join();eb->join();
 std::cout<<Json{{"scenario","altair-automatic-propagation"},{"realDaemons",2},{"realCoreHttpServers",2},{"officialBlocks",2},{"lostAckRetried",true},{"senderDaemonRestart",true},{"stableAppendCursor",true},{"fixtureClock",true},{"executionBound",false},{"distributedFinality",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
