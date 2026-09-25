#include "support/altair_transition_rules.hpp"
#include "nano/consensus_altair_reactor.hpp"
#include "../network/altair_consensus_service.hpp"
#include <future>
#include "../network/altair_rpc_service.hpp"
#include "nano/http.hpp"
#include "nano/simulation_queue.hpp"
#include "support/persistent_rpc.hpp"
using namespace test;using namespace test::phase0_minimal;
struct RunningAltairHttp {HttpServer& server;std::thread thread;explicit RunningAltairHttp(HttpServer& s):server(s),thread([&s]{s.run();}){}~RunningAltairHttp(){server.stop();thread.join();}};
int main(){try{
 const bool http_mode=std::getenv("NANO_TEST_ALTAIR_HTTP")!=nullptr;uint64_t ticket_sequence=0;std::string admission_session;bool dropped_reply=false;
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures",corpus=fixtures/"consensus-altair-fork-head-official",dir=corpus/"chain_no_attestations";
 const auto manifest=load(corpus/"manifest.json");for(const auto& row:manifest.at("cases"))if(row.at("name")=="chain_no_attestations")for(const auto& [path,info]:row.at("files").items())check(hex(sha256(binary(corpus/path))).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");
 const auto transition=altair_transition_rules(fixtures);const auto config=read(corpus/"minimal-config-v1.6.0-beta.0.yaml");const auto duration=parameter(config,"SECONDS_PER_SLOT")*1000;
 const AltairForkBlockRules rules{transition,duration,duration/3,parameter(config,"PROPOSER_SCORE_BOOST"),AltairProposerBoostProfile::Release160Beta0};
 const AltairStoreSnapshotLimits limits{32*1024*1024,100,10000,10000,1024*1024,16*1024*1024,4096,100000};
 const auto initial=initialize_altair_fork_store({binary(dir/"anchor_state.ssz"),std::nullopt},binary(dir/"anchor_block.ssz"),rules);
 const Bytes genesis(32,9);Temp directory;ChainStore disk(directory/"db");disk.bind_chain(1337,genesis,"altair-qualification");AltairDurableReactor reactor(disk,1337,genesis,rules,limits,initial);
 const auto& anchor=initial.blocks.at(initial.timed.view.anchor).state.ssz;ConsensusBytes4 version{};std::copy_n(anchor.begin()+52,4,version.begin());const auto fork_root=consensus_fork_data_root(version,root_at(anchor,8));
 const AltairWireProfile profile{"0x539",hex(genesis),hex(Bytes(fork_root.begin(),fork_root.begin()+4)),1024*1024,400000,512,40000};
 Temp execution_directory;Blockchain execution(execution_directory/"execution",Genesis::from_json(Json{{"alloc",Json::object()}}));
 std::shared_ptr<AltairRpcService> endpoint;std::unique_ptr<Rpc> rpc;std::unique_ptr<HttpServer> server;std::unique_ptr<RunningAltairHttp> running;std::unique_ptr<PersistentRpc> client;
 if(http_mode){
  endpoint=std::make_shared<AltairRpcService>(reactor,profile,4,2*1024*1024);
  rpc=std::make_unique<Rpc>(execution,"qualification-only-token",AdmissionBudget::Clock::now,RpcSimulationLimits{},endpoint);
  bool public_rejected=false;try{HttpServer unsafe(*rpc,"0.0.0.0",0);}catch(const std::invalid_argument&){public_rejected=true;}check(public_rejected,"private endpoint publicly bindable");
  server=std::make_unique<HttpServer>(*rpc,"127.0.0.1",0);running=std::make_unique<RunningAltairHttp>(*server);client=std::make_unique<PersistentRpc>(server->port());
  const auto token=reactor.current().token;
  const auto status=client->call("nano_altairStatus",Json::array({"qualification-only-token"}));
  admission_session=status.at("admissionSession").get<std::string>();
  check(status.at("digest")==hex(token.digest)&&status.at("clockHealthy").is_null(),"private status differs from checkpoint");
  for(const auto* method:{"nano_altairSubmit","nano_altairJob","nano_altairCancel","nano_altairForget","nano_altairJobs"}){
   auto params=Json::array({"wrong"});if(std::string(method)!="nano_altairJobs"){params.push_back(admission_session);params.push_back("0x1");}if(std::string(method)=="nano_altairSubmit")params.push_back("{}");
   bool refused=false;try{client->call(method,params);}catch(const PersistentRpcError& e){refused=e.code==-32001;}check(refused,"ticket method bypasses authentication");
  }
  bool status_unauthorized=false;try{client->call("nano_altairStatus",Json::array({"wrong"}));}catch(const PersistentRpcError& e){status_unauthorized=e.code==-32001;}check(status_unauthorized,"status bypasses authentication");

  bool unauthorized=false;try{client->call("nano_altairReceive",Json::array({"wrong","{}"}));}catch(const PersistentRpcError& e){unauthorized=e.code==-32001;}
  check(unauthorized&&reactor.current().token==token,"unauthorized request changed consensus");
  bool invalid=false;try{client->call("nano_altairReceive",Json::array({"qualification-only-token","{}"}));}catch(const PersistentRpcError& e){invalid=e.code==-32602;}
  check(invalid&&client->call("eth_chainId")=="0x539","invalid envelope or normal RPC regression");
  Rpc disabled(execution);auto response=disabled.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method","nano_altairReceive"},{"params",Json::array({"qualification-only-token","{}"})}});
  check(response.at("error").at("code")==-32601,"Altair RPC enabled by default");
  check(simulation_count(Json{{"method","nano_altairReceive"}})==1,"Altair bypasses bounded HTTP worker queue");
 }
 auto now=InboundBudget::Clock::time_point{};InboundBudget shared_budget(now);AltairMessageIngress ingress(profile,shared_budget);size_t forwarded=0;
 AltairConsensusService service(reactor,4,2*1024*1024);auto dispatch=service.dispatcher();
 auto deliver=[&](const AltairWireMessage& m){
  if(client){
   const auto kind=m.kind==AltairWireKind::block?"block":m.kind==AltairWireKind::attestation?"attestation":"attester_slashing";
   const auto encoded=Json{{"chainId",profile.chain_id},{"genesisHash",profile.genesis_hash},{"forkDigest",profile.fork_digest},{"kind",kind},{"ssz",hex(m.ssz)}}.dump();
   const auto sequence=quantity(Int(std::to_string(++ticket_sequence)));
   const auto args=Json::array({"qualification-only-token",admission_session,sequence,encoded});
   if(m.kind==AltairWireKind::block&&!dropped_reply){
    // Send once, establish admission independently, then discard the unread reply.
    namespace http=boost::beast::http;boost::asio::io_context io;boost::beast::tcp_stream socket(io);
    socket.connect({boost::asio::ip::make_address("127.0.0.1"),server->port()});
    http::request<http::string_body> request(http::verb::post,"/",11);request.set(http::field::host,"127.0.0.1");
    request.body()=Json{{"jsonrpc","2.0"},{"id",1},{"method","nano_altairSubmit"},{"params",args}}.dump();request.prepare_payload();http::write(socket,request);
    until([&]{return client->call("nano_altairJob",Json::array({"qualification-only-token",admission_session,sequence})).at("state")!="unknown";},"Altair admitted before discarded reply");
    boost::beast::error_code ignored;socket.socket().close(ignored);dropped_reply=true;
   }
   const auto submitted=client->call("nano_altairSubmit",args);check(submitted.at("state")!="limited"&&submitted.at("state")!="retired","HTTP ticket admission failed");
   const auto retry=client->call("nano_altairSubmit",args);check(retry.at("payloadDigest")==submitted.at("payloadDigest"),"retry changed ticket identity");
   Json receipt;until([&]{receipt=client->call("nano_altairJob",Json::array({"qualification-only-token",admission_session,sequence}));return receipt.at("state")=="completed";},"Altair ticket completed");
   client->call("nano_altairForget",Json::array({"qualification-only-token",admission_session,sequence}));
   check(client->call("nano_altairSubmit",args).at("state")=="retired","retired HTTP ticket replayed");
   const auto decision=receipt.at("decision");return decision=="accepted"?AltairWork::Decision::accepted:decision=="rejected"?AltairWork::Decision::rejected:AltairWork::Decision::local_failure;
  }
  auto reply=std::make_shared<std::promise<AltairWork::Decision>>();auto future=reply->get_future();
  check(dispatch(m,std::make_shared<std::atomic<bool>>(false),[reply](auto decision){reply->set_value(decision);}),"service queue refused bounded fixture");
  check(future.wait_for(20s)==std::future_status::ready,"durable worker did not complete");return future.get();
 };
 auto handler=[&](const AltairWireMessage& m){++forwarded;return deliver(m)==AltairWork::Decision::accepted;};
 auto frame=[&](const std::string& kind,const Bytes& bytes){return Json{{"chainId",profile.chain_id},{"genesisHash",profile.genesis_hash},{"forkDigest",profile.fork_digest},{"kind",kind},{"ssz",hex(bytes)}};};
 unsigned rejected=0;auto refuse=[&](const std::string& encoded,bool reaches_core=false){now+=10s;const auto before=reactor.current().token;const auto count=forwarded;check(ingress.receive("peer-a",encoded,handler,now)==AltairMessageIngress::Result::rejected,"wire message accepted");check(reactor.current().token==before&&ingress.active()==0&&forwarded==count+(reaches_core?1:0),"bad wire message changed state or leaked lease");++rejected;};
 const auto base=frame("attestation",Bytes{0,1});
 if(client){
  auto wrong=base;wrong["chainId"]="0x1";auto extra=base;extra["tick"]=1;
  for(const auto& encoded:std::vector<std::string>{wrong.dump(),extra.dump(),std::string("{\"chainId\":\"0x539\",")+base.dump().substr(1)}){
   const auto before=reactor.current().token;bool rejected=false;
   try{client->call("nano_altairReceive",Json::array({"qualification-only-token",encoded}));}catch(const PersistentRpcError& e){rejected=e.code==-32602;}
   check(rejected&&reactor.current().token==before,"core HTTP envelope validation bypassed");
  }
 }

 refuse(std::string("{\"chainId\":\"0x539\",")+base.dump().substr(1));
 for(const std::string field:{"chainId","genesisHash","forkDigest","kind"}){auto j=base;j[field]="wrong";refuse(j.dump());}
 auto j=base;j["fromBlock"]=true;refuse(j.dump());j=base;j["kind"]="tick";refuse(j.dump());j=base;j["ssz"]="0x0";refuse(j.dump());j=base;j["ssz"]="0xFF";refuse(j.dump());j=base;j["ssz"]=hex(Bytes(513,0));refuse(j.dump());refuse(base.dump()+"{}");refuse(std::string(profile.frame_bytes+1,' '));refuse(base.dump(),true);refuse(frame("attester_slashing",Bytes{0,1}).dump(),true);
 // Identical decoder verdict across fragmented reads through the existing frame guard.
 const auto complete=base.dump();for(size_t width:{size_t(1),size_t(7),complete.size()}){JsonFrame f(profile.frame_bytes);bool done=false;for(size_t i=0;i<complete.size();i+=width)done=f.feed(std::string_view(complete).substr(i,std::min(width,complete.size()-i)));check(done,"fragmented envelope never completed");}
 unsigned accepted=0;for(const auto& step:load(dir/"steps.json")){if(step.contains("tick"))reactor.tick(step.at("tick").get<uint64_t>()*1000);else if(step.contains("block")){now+=10s;const auto packet=binary(dir/(step.at("block").get<std::string>()+".ssz"));const auto before=reactor.current().token;check(ingress.receive("peer-a",frame("block",packet).dump(),handler,now)==AltairMessageIngress::Result::accepted,"official wire block rejected");const auto after=reactor.current();check(after.token.sequence==before.sequence+1&&disk.fork_checkpoint(1337,genesis,limits.envelope_bytes,after.token).has_value(),"wire acknowledged before durability");++accepted;}else if(step.contains("checks")&&step.at("checks").contains("head")){const auto view=reactor.current().state;const auto head=phase0_fork_choice_head(view->timed.view,{transition.epoch.ffg.slots_per_epoch,transition.attestation.duties.effective_balance_increment,rules.proposer_score_boost}).head;check(hex(Bytes(head.begin(),head.end()))==step.at("checks").at("head").at("root").get<std::string>(),"wire head differs from official expectation");}}
 // Every invalid frame consumes rate budget; a different peer has its own share.
 now+=10s;unsigned limited=0;for(unsigned i=0;i<32;++i)if(ingress.receive("peer-a","{}",handler,now)==AltairMessageIngress::Result::limited)++limited;
 check(limited>0&&ingress.receive("peer-b","{}",handler,now)==AltairMessageIngress::Result::rejected,"peer admission isolation lost");
 AltairMessageIngress another(profile,shared_budget);check(another.receive("peer-a","{}",handler,now)==AltairMessageIngress::Result::limited,"another protocol instance reset shared budget");
 now+=10s;bool propagated=false;try{ingress.receive("peer-a",base.dump(),[](const auto&)->bool{throw std::runtime_error("disk unavailable");},now);}catch(const std::runtime_error& e){propagated=std::string(e.what())=="disk unavailable";}check(propagated&&ingress.active()==0,"local failure misclassified or leaked lease");
 // Core-side service errors remain local failures, never peer acceptance.
 const auto stable=reactor.current();const auto record=disk.fork_checkpoint(1337,genesis,limits.envelope_bytes,stable.token);
 const auto competing=disk.commit_fork_checkpoint(1337,genesis,stable.token,record->snapshot,limits.envelope_bytes);
 bool stale=false;try{reactor.tick(stable.state->timed.clock.time_ms+1);}catch(const std::exception&){stale=true;}
 check(stale&&deliver({AltairWireKind::attestation,Bytes{0,1}})==AltairWork::Decision::local_failure,"unavailable reactor acknowledged message");
 service.stop();service.join();
 check(!dispatch({AltairWireKind::attestation,Bytes{0,1}},std::make_shared<std::atomic<bool>>(false),[](auto){}),"stopped service admits work");
 AltairDurableReactor restored(disk,1337,genesis,rules,limits,competing);
 AltairConsensusService resumed(restored,1,1024);std::promise<AltairWork::Decision> reply;auto result=reply.get_future();
 check(resumed.dispatcher()({AltairWireKind::attestation,Bytes{0,1}},std::make_shared<std::atomic<bool>>(false),[&](auto d){reply.set_value(d);}),"restarted service admission");
 check(result.wait_for(20s)==std::future_status::ready&&result.get()==AltairWork::Decision::rejected,"restarted service failed validation");resumed.stop();resumed.join();
 if(endpoint){
  endpoint->stop();endpoint->join();bool unavailable=false;
  try{client->call("nano_altairReceive",Json::array({"qualification-only-token",base.dump()}));}catch(const PersistentRpcError& e){unavailable=e.code==-32005;}
  check(unavailable&&client->call("eth_chainId")=="0x539","stopped Altair service blocks ordinary RPC");
 }
 std::cout<<Json{{"discardedAdmissionReply",dropped_reply},{"httpTransport",http_mode},{"durableWorker",true},{"unavailableReactorNotAcknowledged",true},{"scenario","altair-wire-admission"},{"rejected",rejected},{"officialBlocksCommitted",accepted},{"limited",limited},{"realLibp2pNetwork",false},{"networkActive",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
