#pragma once
#include "nano/consensus_bellatrix_runtime.hpp"
#include "../../network/bellatrix_rpc_service.hpp"
#include <fstream>
#include <cstdlib>
namespace test {
// Synthetic fixture keys only. Exercises the production configuration loader,
// ownership and shutdown path; this is not a CLI or distributed-network test.
inline nano::Json bellatrix_runtime_config(const Temp& location,
 const nano::BellatrixForkBlockRules& rules,const nano::BellatrixStoreSnapshotLimits& limits,
 const nano::BellatrixForkStore& initial){
 using namespace nano;
 auto q=[](uint64_t n){return quantity(Int(std::to_string(n)));};
 auto save=[&](const char* name,const std::string& value){const auto path=location/name;std::ofstream f(path,std::ios::binary);f<<value;f.close();check(bool(f),"runtime fixture write");return path.string();};
 const auto profile=bellatrix_fork_profile(rules).dump();
 const auto snapshot=bellatrix_store_snapshot(initial,rules,limits);
 const auto committee=rules.transition.consensus.body.validators_per_committee;
 const auto block=rules.transition.consensus.maximum_block_bytes;
 return {
  {"profile",{{"file",save("profile.json",profile)},{"sha256",hex(sha256(Bytes(profile.begin(),profile.end())))},{"maximumBytes",q(profile.size())}}},
  {"initial",{{"file",save("snapshot.json",snapshot)},{"sha256",hex(sha256(Bytes(snapshot.begin(),snapshot.end())))}}},{"restart",nullptr},
  {"limits",{{"envelopeBytes",q(limits.envelope_bytes)},{"blocks",q(limits.blocks)},{"messages",q(limits.messages)},{"equivocations",q(limits.equivocations)},{"stateBytes",q(limits.state_bytes)},{"totalStateBytes",q(limits.total_state_bytes)},{"replaySlots",q(limits.replay_slots)},{"syncTrials",q(limits.sync_trials)}}},
  {"clock",{{"intervalMs",q(rules.slot_duration_ms)},{"maximumForwardStepMs",q(rules.slot_duration_ms)}}},
  {"wire",{{"frameBytes",q(2*block+1024)},{"blockBytes",q(block)},{"attestationBytes",q(228+(committee+8)/8)},{"slashingBytes",q(464+16*committee)}}},
  {"worker",{{"jobs","0x2"},{"payloadBytes",q(2*block)},{"timeoutMs","0x2710"}}}
 };
}
inline void bellatrix_runtime_scenario(const nano::Genesis& genesis,
 const nano::BellatrixForkBlockRules& rules,const nano::BellatrixStoreSnapshotLimits& limits,
 const nano::BellatrixForkStore& initial,const std::string& recipient){
 using namespace nano;Temp location;
 auto q=[](uint64_t n){return quantity(Int(std::to_string(n)));};
 auto config=bellatrix_runtime_config(location,rules,limits,initial);
 std::atomic<uint64_t> wall=initial.timed.clock.time_ms;
 auto now=[&]{return wall.load();};
 auto reject=[&](Json invalid,const char* label){
  Blockchain chain(location/label,genesis);bool refused=false;
  try{auto runtime=start_bellatrix_runtime(chain,invalid,{},now);}catch(const std::exception&){refused=true;}
  check(refused,label);bool gated=false;try{chain.require_execution_ready();}catch(const std::exception&){gated=true;}check(gated,"failed runtime left execution available");
 };
 auto invalid=config;invalid["profile"]["sha256"]=hex(Bytes(32,0));reject(invalid,"profile-digest");
 invalid=config;invalid["wire"]["blockBytes"]="0x1";reject(invalid,"wire-capacity");
 invalid=config;invalid["restart"]={{"sequence","0x1"},{"digest",hex(Bytes(32,1))}};reject(invalid,"ambiguous-trust");
 // Restore the caller's environment even when a check throws.
 struct EnvScope {
  std::vector<std::pair<std::string,std::optional<std::string>>> old;
  void set(const std::string& name,const std::string& value){const auto* p=std::getenv(name.c_str());old.push_back({name,p?std::optional<std::string>(p):std::nullopt});if(setenv(name.c_str(),value.c_str(),1)!=0)throw std::runtime_error("fixture environment");}
  ~EnvScope(){for(const auto& [name,value]:old){if(value)setenv(name.c_str(),value->c_str(),1);else unsetenv(name.c_str());}}
 } env;
 Json keys=Json::array();
 for(uint64_t i=0;i<64;++i){
  const auto name="NANO_RUNTIME_FIXTURE_BLS_"+std::to_string(i);env.set(name,hex(bytes(Int(std::to_string(i+1)),32)));
  keys.push_back({{"index",q(i)},{"feeRecipient",recipient},{"keyEnv",name},{"journal",(location/("key-"+std::to_string(i))).string()},{"restart",nullptr}});
 }
 config["validator"]={{"pollIntervalMs",q(rules.slot_duration_ms)},{"maximumVoteTrials","0x80"},{"keys",keys}};
 invalid=config;invalid["validator"]["keys"]=Json::array({keys[0]});invalid["validator"]["keys"][0]["index"]="0x1";reject(invalid,"key-registry-mismatch");
 ForkCheckpointToken saved;Json commitments;Bytes head,state;
 auto stop=[](BellatrixRuntime& runtime){(void)stop_bellatrix_runtime(runtime);};
 {
  Blockchain chain(location/"chain",genesis);
  auto runtime=start_bellatrix_runtime(chain,config,{},now);
  check(runtime.rpc->status().at("executionBound")==true&&runtime.validator->status().at("localKeys")==64,"runtime services not bound");
  // Drive one real duty using the configured providers, not injected Signers.
  wall+=rules.slot_duration_ms;runtime.validator->poll();stop(runtime);
  check(runtime.validator->status().at("healthy")==true&&runtime.validator->status().at("proposalsAdmitted")==1,"configured providers failed proposal");
  saved=runtime.reactor->current().token;commitments=runtime.validator->restart_commitments();head=chain.tip().header.hash();state=chain.state().root();
 }
 config["initial"]=nullptr;config["restart"]={{"sequence",q(saved.sequence)},{"digest",hex(saved.digest)}};
 for(size_t i=0;i<keys.size();++i)config["validator"]["keys"][i]["restart"]=commitments.at(i).at("restart");
 {
  Blockchain chain(location/"chain",genesis);
  auto stale=config;stale["restart"]["digest"]=hex(Bytes(32,0));bool refused=false;
  try{auto bad=start_bellatrix_runtime(chain,stale,{},now);}catch(const std::exception&){refused=true;}
  check(refused,"runtime accepted stale external checkpoint");
  auto runtime=start_bellatrix_runtime(chain,config,{},now);
  check(chain.tip().header.hash()==head&&chain.state().root()==state,"configured restart lost execution");
  runtime.validator->poll();
  check(runtime.validator->status().at("proposalsAdmitted")==0&&runtime.validator->restart_commitments()==commitments,"restart repeated reserved duties");
  wall+=rules.slot_duration_ms;runtime.validator->poll();stop(runtime);
  check(runtime.validator->status().at("proposalsAdmitted")==1&&chain.tip().header.number==3,"restarted validator did not resume next slot");
  // Clock regression closes execution admission, rather than serving stale state.
  wall-=rules.slot_duration_ms;bool refused_clock=false;try{runtime.clock->poll();}catch(const std::exception&){refused_clock=true;}
  bool gated=false;try{chain.require_execution_ready();}catch(const std::exception&){gated=true;}
  check(refused_clock&&gated&&!runtime.clock->healthy(),"clock regression failed open");
 }
 std::cout<<Json{{"scenario","bellatrix-runtime-config-restart"},{"configuredKeys",64},{"proposalsAcrossRestart",2},{"repeatedDutySuppressed",true},{"invalidConfigCases",4},{"staleCheckpointRejected",true},{"clockRegressionGated",true},{"independentProcesses",false}}.dump()<<'\n';
}
inline void bellatrix_managed_runtime_scenario(const nano::Json& genesis,
 const nano::BellatrixForkBlockRules& rules,const nano::BellatrixStoreSnapshotLimits& limits,
 const nano::BellatrixForkStore& initial,const std::string& executable,const std::string& daemon,bool distributed=false,bool partition=false){
 using namespace nano;Temp location;
 auto bounded=limits;
 if(distributed){bounded.blocks=128;bounded.messages=2048;bounded.total_state_bytes=64*1024*1024;bounded.envelope_bytes=128*1024*1024;}
 auto config=bellatrix_runtime_config(location,rules,bounded,initial);
 auto q=[](uint64_t n){return quantity(Int(std::to_string(n)));};
 config["clock"]={{"intervalMs","0xfa"},{"maximumForwardStepMs",q(30*rules.slot_duration_ms)}};
 Json keys=Json::array(),other_keys=Json::array();Env environment,other_environment;
 for(uint64_t i=0;i<64;++i){
  const auto name="NANO_MANAGED_FIXTURE_KEY_"+std::to_string(i);(distributed&&i>=32?other_environment:environment)[name]=hex(bytes(Int(std::to_string(i+1)),32));
  (distributed&&i>=32?other_keys:keys).push_back({{"index",q(i)},{"feeRecipient","0x0000000000000000000000000000000000001234"},{"keyEnv",name},{"journal",(location/("key-"+std::to_string(i))).string()},{"restart",nullptr}});
 }
 if(partition){environment["NANO_TEST_NETWORK_TRACE"]="1";other_environment["NANO_TEST_NETWORK_TRACE"]="1";}
 Json node={{"rpc_host","127.0.0.1"},{"rpc_port",0},{"p2p_binary",daemon},{"listen_addr","/ip4/127.0.0.1/tcp/0"},{"bootnodes",Json::array()},{"bellatrix",config}};
 if(distributed)node["bellatrix"]["validator"]={{"pollIntervalMs","0xfa"},{"maximumVoteTrials","0x80"},{"keys",other_keys}};
 auto validator=node;validator["bellatrix"]["validator"]={{"pollIntervalMs","0xfa"},{"maximumVoteTrials","0x80"},{"keys",keys}};
 save(location/"genesis.json",genesis);save(location/"validator.json",validator);
 auto launch=[&](const char* name,const Env& env=Env{}){return std::make_unique<Process>(std::vector<std::string>{executable,"run",(location/"genesis.json").string(),(location/name).string(),(location/(std::string(name)+".json")).string()},env);};
 auto source=launch("validator",environment);source->event("bellatrix-runtime-ready");const auto source_port=source->port();
 const auto listen=source->event("listen").at("address").get<std::string>();source->event("altair-ingress-ready");
 node["bootnodes"]=Json::array({listen});save(location/"observer.json",node);
 auto observer=launch("observer",other_environment);observer->event("bellatrix-runtime-ready");const auto observer_port=observer->port();observer->event("altair-ingress-ready");
 struct Diagnostics {
  std::unique_ptr<Process>& a;std::unique_ptr<Process>& b;fs::path directory;
  int exceptions=std::uncaught_exceptions();
  ~Diagnostics(){if(std::uncaught_exceptions()>exceptions)try{
   const auto ar=a->stop(),br=b->stop();
   std::cerr<<"source process output:\n"<<ar.out<<ar.err<<"peer process output:\n"<<br.out<<br.err;
   // Only this synthetic-key test opts into retention. Both processes have
   // closed RocksDB before copying; never overwrite an earlier failure.
   if(const auto* output=std::getenv("NANO_TEST_PARTITION_EVIDENCE")){
    const fs::path destination(output);
    check(!fs::exists(destination),"partition evidence destination already exists");
    fs::copy(directory,destination,fs::copy_options::recursive);
    save(destination/"source-output.txt",ar.out+ar.err);
    save(destination/"observer-output.txt",br.out+br.err);
    std::cerr<<Json{{"event","partition-evidence-retained"},{"path",destination.string()}}.dump()<<std::endl;
   }
  }catch(const std::exception& error){std::cerr<<"partition diagnostics: "<<error.what()<<std::endl;}}
 } diagnostics{source,observer,location.path};
 auto latest=[&](int port){source->pump(0);observer->pump(0);return rpc(port,"eth_getBlockByNumber",Json::array({"latest",false}));};
 until([&]{const auto a=latest(source_port),b=latest(observer_port);return quantity(a.at("number"))>=3&&a.at("hash")==b.at("hash");},"independent Bellatrix cores synchronized",40);
 const auto before=latest(source_port);
 auto result=source->stop();check(result.code==0,"managed validator shutdown: "+result.err);
 Json stopped;std::istringstream lines(result.out);std::string line;
 while(std::getline(lines,line)){auto j=Json::parse(line,nullptr,false);if(j.is_object()&&j.value("event","")=="bellatrix-runtime-stopped")stopped=std::move(j);}
 check(stopped.is_object()&&stopped.at("validatorKeys").size()==keys.size(),"managed shutdown missing journal commitments");
 validator["bellatrix"]["initial"]=nullptr;validator["bellatrix"]["restart"]=stopped.at("restart");
 for(size_t i=0;i<keys.size();++i)validator["bellatrix"]["validator"]["keys"][i]["restart"]=stopped.at("validatorKeys")[i].at("restart");
 // Reconnect to the observer's live port; the source uses an ephemeral listener.
 validator["bootnodes"]=Json::array({observer->event("listen").at("address")});save(location/"validator.json",validator);
 source=launch("validator",environment);source->event("bellatrix-runtime-ready");const auto resumed_port=source->port();source->event("altair-ingress-ready");
 until([&]{const auto a=latest(resumed_port),b=latest(observer_port);return quantity(a.at("number"))>quantity(before.at("number"))&&a.at("hash")==b.at("hash");},"restarted Bellatrix validator propagated next block",40);
 if(distributed){
  uint64_t last_epoch=0;const auto start=u64(quantity(genesis.at("timestamp"))),seconds=rules.transition.seconds_per_slot,slots=rules.transition.consensus.epoch.ffg.slots_per_epoch;
  until([&]{const auto a=latest(resumed_port),b=latest(observer_port);const auto slot=(u64(quantity(a.at("timestamp")))-start)/seconds;
   if(slot/slots>last_epoch){last_epoch=slot/slots;std::cout<<Json{{"event","distributed-progress"},{"slot",slot},{"epoch",last_epoch},{"sameHead",a.at("hash")==b.at("hash")}}.dump()<<std::endl;}
   return slot>=4*slots&&a.at("hash")==b.at("hash");},"distributed validators completed four epochs",int(5*slots*seconds));
 }
 bool partition_recovered=false;
 if(partition){
  auto finalized=[](int port){return rpc(port,"eth_getBlockByNumber",Json::array({"finalized",false}));};
  const auto initial_finalized=finalized(resumed_port);check(initial_finalized.at("hash")==finalized(observer_port).at("hash"),"partition starts without common finality");
  const auto a_start=latest(resumed_port),b_start=latest(observer_port);
  struct PausedDaemons {
   std::vector<pid_t> pids;
   void pause(pid_t pid){check(pid>0&&kill(pid,SIGSTOP)==0,"pause owned P2P daemon");pids.push_back(pid);}
   void resume(){for(auto pid:pids)check(kill(pid,SIGCONT)==0,"resume owned P2P daemon");pids.clear();}
   ~PausedDaemons(){for(auto pid:pids)kill(pid,SIGCONT);}
  } paused;
  paused.pause(source->event("p2p-process").at("pid").get<pid_t>());
  paused.pause(observer->event("p2p-process").at("pid").get<pid_t>());
  std::map<std::string,std::string> before_cut;
  for(Int height=quantity(initial_finalized.at("number"));height<=quantity(a_start.at("number"));++height){const auto block=rpc(resumed_port,"eth_getBlockByNumber",Json::array({quantity(height),false}));before_cut.emplace(block.at("number").get<std::string>(),block.at("hash").get<std::string>());}
  auto summary=[](const Json& block){return Json{{"number",block.at("number")},{"hash",block.at("hash")},{"timestamp",block.at("timestamp")}};};
  std::cout<<Json{{"event","partition-baseline"},{"finalized",summary(initial_finalized)},{"aHead",summary(a_start)},{"bHead",summary(b_start)},{"preCutPrefix",before_cut}}.dump()<<std::endl;
  const auto epoch_seconds=rules.transition.seconds_per_slot*rules.transition.consensus.epoch.ffg.slots_per_epoch;
  const auto cut=std::chrono::steady_clock::now(),end=cut+std::chrono::seconds(2*epoch_seconds);bool divergent=false;unsigned reports=0;
  std::cout<<Json{{"event","partition-start"},{"seconds",2*epoch_seconds},{"finalizedHash",initial_finalized.at("hash")}}.dump()<<std::endl;
  while(std::chrono::steady_clock::now()<end){
   const auto af=finalized(resumed_port),bf=finalized(observer_port);
   if(af.at("hash")!=initial_finalized.at("hash")||bf.at("hash")!=initial_finalized.at("hash")){
    auto known=[&](const Json& b){const auto it=before_cut.find(b.at("number").get<std::string>());return it!=before_cut.end()&&it->second==b.at("hash").get<std::string>();};
    std::cout<<Json{{"event","partition-finality-change"},{"a",summary(af)},{"b",summary(bf)},{"aInPreCutPrefix",known(af)},{"bInPreCutPrefix",known(bf)}}.dump()<<std::endl;
    check(false,"isolated half-stake advanced finality");
   }
   const auto a=latest(resumed_port),b=latest(observer_port);divergent|=a.at("hash")!=b.at("hash");
   auto elapsed=std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now()-cut).count();
   if(uint64_t(elapsed)/epoch_seconds>=reports){++reports;std::cout<<Json{{"event","partition-progress"},{"elapsedSeconds",elapsed},{"a",a.at("number")},{"b",b.at("number")},{"finalityUnchanged",true}}.dump()<<std::endl;}
   std::this_thread::sleep_for(250ms);
  }
  check(divergent&&quantity(latest(resumed_port).at("number"))>quantity(a_start.at("number"))&&quantity(latest(observer_port).at("number"))>quantity(b_start.at("number")),"partition did not exercise competing live branches");
  paused.resume();std::cout<<Json{{"event","partition-healed"}}.dump()<<std::endl;
  const auto recovery_start=std::chrono::steady_clock::now();unsigned recovery_report=0;
  until([&]{const auto a=finalized(resumed_port),b=finalized(observer_port);const auto elapsed=std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now()-recovery_start).count();
   if(uint64_t(elapsed)/epoch_seconds>=recovery_report){++recovery_report;std::cout<<Json{{"event","recovery-progress"},{"elapsedSeconds",elapsed},{"aFinalized",a.at("number")},{"bFinalized",b.at("number")}}.dump()<<std::endl;}
   return quantity(a.at("number"))>quantity(initial_finalized.at("number"))&&a.at("hash")==b.at("hash")&&latest(resumed_port).at("hash")==latest(observer_port).at("hash");},"partition recovered common advancing finality",int(5*epoch_seconds));
  partition_recovered=true;
 }
 const auto after=latest(resumed_port);const auto source_end=source->stop(),observer_end=observer->stop();check(source_end.code==0&&observer_end.code==0,"managed peers failed shutdown");
 Json finality;
 if(distributed){
  auto shutdown=[](const Result& result){Json found;std::istringstream lines(result.out);std::string line;while(std::getline(lines,line)){auto j=Json::parse(line,nullptr,false);if(j.is_object()&&j.value("event","")=="bellatrix-runtime-stopped")found=std::move(j);}return found;};
  const auto a=shutdown(source_end),b=shutdown(observer_end);check(a.is_object()&&b.is_object(),"missing distributed shutdown evidence");
  std::cout<<Json{{"event","distributed-checkpoints"},{"a",a.at("finalized")},{"b",b.at("finalized")},{"justifiedA",a.at("justifiedEpoch")},{"justifiedB",b.at("justifiedEpoch")}}.dump()<<std::endl;
  check(quantity(a.at("finalized").at("epoch"))>=2&&a.at("finalized")==b.at("finalized"),"distributed validators did not finalize a common checkpoint");finality=a.at("finalized");
 }

 std::cout<<Json{{"scenario","bellatrix-managed-cores-restart"},{"independentCoreProcesses",2},{"p2pDaemons",2},{"wallClock",true},{"validatorNodes",distributed?2:1},{"physicalHosts",1},{"localKeys",64},{"blockBeforeRestart",before.at("number")},{"blockAfterRestart",after.at("number")},{"observerSynchronized",true},{"distributedStake",distributed},{"partitionRecovered",partition_recovered},{"finalized",finality}}.dump()<<std::endl;
}

}
