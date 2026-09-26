#pragma once
#include "nano/consensus_bellatrix_runtime.hpp"
#include "../../network/bellatrix_rpc_service.hpp"
#include <fstream>
#include <boost/beast.hpp>
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
// Export a fresh, isolated devnet. All 64 keys are PUBLIC synthetic fixture keys.
inline void bellatrix_kurtosis_export(const nano::Json& genesis,
 const nano::BellatrixForkBlockRules& rules,nano::BellatrixStoreSnapshotLimits limits,
 const nano::BellatrixForkStore& initial,const fs::path& destination){
 using namespace nano;Temp location;
 check(!fs::exists(destination),"Kurtosis output already exists");
 limits.blocks=128;limits.messages=2048;limits.total_state_bytes=64*1024*1024;limits.envelope_bytes=128*1024*1024;
 auto config=bellatrix_runtime_config(location,rules,limits,initial);
 config["profile"]["file"]="/config/profile.json";config["initial"]["file"]="/config/snapshot.json";
 config["clock"]={{"intervalMs","0xfa"},{"maximumForwardStepMs",quantity(Int(std::to_string(300*rules.slot_duration_ms)))}};
 fs::create_directories(destination);
 fs::copy_file(location/"profile.json",destination/"profile.json");
 fs::copy_file(location/"snapshot.json",destination/"snapshot.json");
 save(destination/"genesis.json",genesis);
 for(uint64_t n=0;n<2;++n){
  Json keys=Json::array(),env=Json::object();
  for(uint64_t i=n*32;i<(n+1)*32;++i){
   const auto name="NANO_KURTOSIS_FIXTURE_"+std::to_string(i);
   env[name]=hex(bytes(Int(std::to_string(i+1)),32));
   keys.push_back({{"index",quantity(Int(std::to_string(i)))},{"feeRecipient","0x0000000000000000000000000000000000001234"},{"keyEnv",name},{"journal","/var/lib/nano/key-"+std::to_string(i)},{"restart",nullptr}});
  }
  auto node=Json{{"rpc_host","0.0.0.0"},{"rpc_port",8545},{"p2p_binary","/usr/local/bin/nano-p2p"},{"listen_addr","/ip4/0.0.0.0/tcp/30303"},{"bootnodes",Json::array()},{"bellatrix",config}};
  node["bellatrix"]["validator"]={{"pollIntervalMs","0xfa"},{"maximumVoteTrials","0x80"},{"keys",keys}};
  save(destination/("node-"+std::to_string(n)+".json"),node);
  save(destination/("env-"+std::to_string(n)+".json"),env);
 }
 std::cout<<Json{{"exported",destination.string()},{"syntheticValidators",64},{"nodes",2},{"fork","Bellatrix/Paris"}}.dump()<<std::endl;
}
inline void bellatrix_runtime_scenario(const nano::Genesis& genesis,
 const nano::BellatrixForkBlockRules& rules,const nano::BellatrixStoreSnapshotLimits& limits,
 const nano::BellatrixForkStore& initial,const std::string& recipient,bool witnessed=false,const std::function<Bytes(uint64_t,const nano::ConsensusRoot&)>& malicious_signer={}){
 using namespace nano;Temp location;
 auto q=[](uint64_t n){return quantity(Int(std::to_string(n)));};
 auto config=bellatrix_runtime_config(location,rules,limits,initial);
 if(witnessed)config["recoveryWitness"]=(location/"consensus-witness").string();
 std::atomic<uint64_t> wall=initial.timed.clock.time_ms;
 auto now=[&]{return wall.load();};
 auto reject=[&](Json invalid,const char* label){
  if(witnessed)invalid["recoveryWitness"]=(location/(std::string(label)+"-witness")).string();
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
 if(witnessed)for(size_t i=0;i<keys.size();++i)keys[i]["recoveryWitness"]=(location/("witness-key-"+std::to_string(i))).string();
 config["validator"]={{"pollIntervalMs",q(rules.slot_duration_ms)},{"maximumVoteTrials","0x80"},{"keys",keys}};
 invalid=config;invalid["validator"]["keys"]=Json::array({keys[0]});invalid["validator"]["keys"][0]["index"]="0x1";reject(invalid,"key-registry-mismatch");
 ForkCheckpointToken saved;Json commitments;Bytes head,state;std::optional<uint64_t> offender;uint64_t balance_before=0;
 auto stop=[](BellatrixRuntime& runtime){(void)stop_bellatrix_runtime(runtime);};
 {
  Blockchain chain(location/"chain",genesis);
  auto runtime=start_bellatrix_runtime(chain,config,{},now);
  check(runtime.rpc->status().at("executionBound")==true&&runtime.validator->status().at("localKeys")==64,"runtime services not bound");
  // Drive one real duty using the configured providers, not injected Signers.
  wall+=rules.slot_duration_ms;runtime.validator->poll();
  if(malicious_signer){
   const auto current=runtime.reactor->current();const auto& parent=bellatrix_fork_head(*current.state,rules);
   const auto& t=rules.transition.consensus;const BellatrixEpochRules epoch_rules{t.epoch,rules.transition.execution,rules.transition.maximum_state_bytes};
   const auto context=bellatrix_fork_block_context(parent.state,parent.header,epoch_rules,t.state);
   const auto proposer=prepare_bellatrix_proposer_identity(parent.state,parent.header,parent.header.slot+1,epoch_rules,t.state,t.attestation.duties,limits.replay_slots,limits.sync_trials);check(bool(proposer),"missing next proposer");
   offender=(proposer->index+1)%64;balance_before=context.registry.validators.at(*offender).balance;
   IndexedConsensusAttestation a{{parent.header.slot,0,{}, {context.registry.epoch,{}},{context.registry.epoch,{}}},{*offender},{}},b=a;b.data.beacon_block_root[0]=1;
   const auto version=context.registry.epoch<context.fork.epoch?context.fork.previous_version:context.fork.current_version;
   for(auto* vote:{&a,&b})vote->signature=malicious_signer(*offender,consensus_signing_root(consensus_attestation_root(vote->data),consensus_domain({1,0,0,0},version,context.genesis_root)));
   auto invalid=b;invalid.signature[0]^=1;check(!runtime.reactor->slashing(a,invalid),"invalid evidence admitted");check(runtime.reactor->current().token==current.token,"invalid evidence published");
   const auto wire=encode_consensus_attester_slashing_ssz(a,b,t.body.validators_per_committee);check(runtime.reactor->slashing_ssz(wire),"valid evidence refused");
   const auto accepted=runtime.reactor->current();check(accepted.signed_slashings->size()==1&&accepted.state->timed.view.equivocating_indices.contains(*offender),"evidence not retained/excluded");
   check(runtime.reactor->slashing_ssz(wire)&&runtime.reactor->current().token==accepted.token,"duplicate evidence amplified checkpoint");
   auto alternate=b;alternate.data.beacon_block_root[0]=2;
   alternate.signature=malicious_signer(*offender,consensus_signing_root(consensus_attestation_root(alternate.data),consensus_domain({1,0,0,0},version,context.genesis_root)));
   check(runtime.reactor->slashing(a,alternate)&&runtime.reactor->current().token==accepted.token&&runtime.reactor->current().signed_slashings->size()==1,"distinct proof for covered offender amplified archive");
   auto disjoint=b;disjoint.indices={(*offender+1)%64};
   disjoint.signature=malicious_signer(disjoint.indices[0],consensus_signing_root(consensus_attestation_root(disjoint.data),consensus_domain({1,0,0,0},version,context.genesis_root)));
   check(runtime.reactor->slashing(a,disjoint)&&runtime.reactor->current().token==accepted.token,"proof without common offender amplified archive");

   Rpc endpoint(chain,"fixture-proof-token",AdmissionBudget::Clock::now,RpcSimulationLimits{},nullptr,runtime.rpc);
   auto request=[&](const char* method,Json params){return endpoint.handle({{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",std::move(params)}});};
   check(request("nano_bellatrixSlashingInventory",Json::array({"wrong-token","0x","0x1"})).at("error").at("code")==-32001,"proof inventory exposed without authorization");
   const auto page=request("nano_bellatrixSlashingInventory",Json::array({"fixture-proof-token","0x","0x1"})).at("result");
   check(page.at("slashings").size()==1&&page.at("missingSignedBlocks")==0,"proof inventory missing");
   const auto exported=request("nano_bellatrixSlashing",Json::array({"fixture-proof-token",page.at("slashings")[0].at("root")})).at("result");
   check(exported.at("available")==true&&exported.at("envelope").at("kind")=="attester_slashing"&&exported.at("envelope").at("sszBase64")==altair_base64(wire),"proof export changed bytes or kind");

  }
  stop(runtime);
  check(runtime.validator->status().at("healthy")==true&&runtime.validator->status().at("proposalsAdmitted")==1,"configured providers failed proposal");
  saved=runtime.reactor->current().token;commitments=runtime.validator->restart_commitments();head=chain.tip().header.hash();state=chain.state().root();
 }
 config["initial"]=nullptr;if(!witnessed){config["restart"]={{"sequence",q(saved.sequence)},{"digest",hex(saved.digest)}};
 for(size_t i=0;i<keys.size();++i)config["validator"]["keys"][i]["restart"]=commitments.at(i).at("restart");}
 {
  Blockchain chain(location/"chain",genesis);
  auto stale=config;stale["restart"]["digest"]=hex(Bytes(32,0));bool refused=false;
  try{auto bad=start_bellatrix_runtime(chain,stale,{},now);}catch(const std::exception&){refused=true;}
  check(refused,"runtime accepted stale external checkpoint");
  auto runtime=start_bellatrix_runtime(chain,config,{},now);
  check(chain.tip().header.hash()==head&&chain.state().root()==state,"configured restart lost execution");
  if(offender)check(runtime.reactor->current().signed_slashings->size()==1,"restart lost pending proof");
  runtime.validator->poll();
  check(runtime.validator->status().at("proposalsAdmitted")==0&&runtime.validator->restart_commitments()==commitments,"restart repeated reserved duties");
  wall+=rules.slot_duration_ms;runtime.validator->poll();stop(runtime);
  check(runtime.validator->status().at("proposalsAdmitted")==1&&chain.tip().header.number==3,"restarted validator did not resume next slot");
  if(offender){const auto current=runtime.reactor->current();const auto& block=bellatrix_fork_head(*current.state,rules);const auto& t=rules.transition.consensus;const auto context=bellatrix_fork_block_context(block.state,block.header,{t.epoch,rules.transition.execution,rules.transition.maximum_state_bytes},t.state);const auto& member=context.registry.validators.at(*offender);check(member.slashed&&member.exit_epoch!=UINT64_MAX&&member.balance<balance_before,"locally proposed proof did not slash, exit and debit offender");}

  // Clock regression closes execution admission, rather than serving stale state.
  wall-=rules.slot_duration_ms;bool refused_clock=false;try{runtime.clock->poll();}catch(const std::exception&){refused_clock=true;}
  bool gated=false;try{chain.require_execution_ready();}catch(const std::exception&){gated=true;}
  check(refused_clock&&gated&&!runtime.clock->healthy(),"clock regression failed open");
 }
 std::cout<<Json{{"scenario","bellatrix-runtime-config-restart"},{"witnessed",witnessed},{"slashingIncludedAfterRestart",bool(offender)},{"configuredKeys",64},{"proposalsAcrossRestart",2},{"repeatedDutySuppressed",true},{"invalidConfigCases",4},{"staleCheckpointRejected",true},{"clockRegressionGated",true},{"independentProcesses",false}}.dump()<<'\n';
}
// One pending core publication with the primary restored to its predecessor.
inline void bellatrix_core_recovery_scenario(const nano::Genesis& genesis,
 const nano::BellatrixForkBlockRules& rules,const nano::BellatrixStoreSnapshotLimits& limits,
 const nano::BellatrixForkStore& initial){
 using namespace nano;Temp location;auto config=bellatrix_runtime_config(location,rules,limits,initial);
 config["recoveryWitness"]=(location/"witness").string();std::atomic<uint64_t> wall=initial.timed.clock.time_ms;
 auto now=[&]{return wall.load();};ForkCheckpointToken first,next;Bytes head,state;
 {Blockchain genesis_only(location/"primary",genesis);}
 fs::copy(location/"primary",location/"genesis-only",fs::copy_options::recursive);
 {Blockchain chain(location/"primary",genesis);auto runtime=start_bellatrix_runtime(chain,config,{},now);(void)stop_bellatrix_runtime(runtime);first=runtime.reactor->current().token;head=chain.tip().header.hash();state=chain.state().root();}
 fs::copy(location/"primary",location/"previous",fs::copy_options::recursive);
 config["initial"]=nullptr;
 // Lose the entire first execution/checkpoint publication, retain its witness.
 // This replays a real block/state suffix from genesis, not only a clock tick.
 check(first.sequence==1,"initial recovery fixture wrote multiple checkpoints");
 fs::remove_all(location/"primary");fs::copy(location/"genesis-only",location/"primary",fs::copy_options::recursive);
 {Blockchain chain(location/"primary",genesis);auto runtime=start_bellatrix_runtime(chain,config,{},now);(void)stop_bellatrix_runtime(runtime);check(runtime.reactor->current().token==first&&chain.tip().header.hash()==head&&chain.state().root()==state,"witness failed lost execution publication recovery");}

 // Force one explicit durable publication: an unchanged clock phase now coalesces.
 {Blockchain chain(location/"primary",genesis);auto runtime=start_bellatrix_runtime(chain,config,{},now);runtime.clock->stop();++wall;runtime.reactor->tick(wall);(void)stop_bellatrix_runtime(runtime);next=runtime.reactor->current().token;check(next.sequence==first.sequence+1,"recovery fixture advanced multiple publications");}
 fs::remove_all(location/"primary");fs::copy(location/"previous",location/"primary",fs::copy_options::recursive);
 {Blockchain chain(location/"primary",genesis);auto runtime=start_bellatrix_runtime(chain,config,{},now);runtime.clock->stop();check(runtime.reactor->current().token==next&&chain.tip().header.hash()==head&&chain.state().root()==state,"witness failed coupled checkpoint roll-forward");++wall;runtime.reactor->tick(wall);(void)stop_bellatrix_runtime(runtime);}
 fs::remove_all(location/"primary");fs::copy(location/"previous",location/"primary",fs::copy_options::recursive);
 {Blockchain chain(location/"primary",genesis);bool refused=false;try{auto runtime=start_bellatrix_runtime(chain,config,{},now);}catch(const std::exception&){refused=true;}check(refused,"core accepted rollback beyond witnessed predecessor");bool gated=false;try{chain.tip();}catch(const std::exception&){gated=true;}check(gated,"failed recovery exposed execution");}
 std::cout<<"Bellatrix witnessed core roll-forward and rollback admission checks passed\n";
}
inline void bellatrix_managed_runtime_scenario(const nano::Json& genesis,
 const nano::BellatrixForkBlockRules& rules,const nano::BellatrixStoreSnapshotLimits& limits,
 const nano::BellatrixForkStore& initial,const std::string& executable,const std::string& daemon,bool distributed=false,bool partition=false,bool crash=false){
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
 if(partition||std::getenv("NANO_TEST_ASSERTOOR_FINALITY")){environment["NANO_TEST_NETWORK_TRACE"]="1";other_environment["NANO_TEST_NETWORK_TRACE"]="1";}
 Json node={{"rpc_host","127.0.0.1"},{"rpc_port",0},{"p2p_binary",daemon},{"listen_addr","/ip4/127.0.0.1/tcp/0"},{"bootnodes",Json::array()},{"bellatrix",config}};
 if(distributed)node["bellatrix"]["validator"]={{"pollIntervalMs","0xfa"},{"maximumVoteTrials","0x80"},{"keys",other_keys}};
 auto validator=node;validator["bellatrix"]["validator"]={{"pollIntervalMs","0xfa"},{"maximumVoteTrials","0x80"},{"keys",keys}};
 if(crash){
  validator["bellatrix"]["recoveryWitness"]=(location/"core-witness").string();
  for(size_t i=0;i<keys.size();++i)validator["bellatrix"]["validator"]["keys"][i]["recoveryWitness"]=(location/("key-witness-"+std::to_string(i))).string();
 }
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
   std::optional<fs::path> retain;
   if(const auto* output=std::getenv("NANO_TEST_PARTITION_EVIDENCE"))retain=fs::path(output);
   else if(std::getenv("NANO_TEST_ASSERTOOR_FINALITY")||std::getenv("NANO_TEST_POST_RECOVERY_PROBE")){
    std::string pattern=(fs::temp_directory_path()/"nano-assertoor-core-failure-XXXXXX").string();
    check(mkdtemp(pattern.data()),"create diagnostic directory");retain=fs::path(pattern)/"snapshot";
   }
   if(retain){
    const fs::path destination=*retain;
    check(!fs::exists(destination),"partition evidence destination already exists");
    fs::copy(directory,destination,fs::copy_options::recursive);
    save(destination/"source-output.txt",ar.out+ar.err);
    save(destination/"observer-output.txt",br.out+br.err);
    std::cerr<<Json{{"event","partition-evidence-retained"},{"path",destination.string()}}.dump()<<std::endl;
   }
  }catch(const std::exception& error){std::cerr<<"partition diagnostics: "<<error.what()<<std::endl;}}
 } diagnostics{source,observer,location.path};
 auto beacon_get=[](int port,const char* path){
  namespace asio=boost::asio;namespace http=boost::beast::http;using tcp=asio::ip::tcp;
  asio::io_context context;boost::beast::tcp_stream stream(context);stream.expires_after(std::chrono::seconds(5));stream.connect(tcp::endpoint(asio::ip::make_address("127.0.0.1"),uint16_t(port)));
  http::request<http::string_body> request(http::verb::get,path,11);request.set(http::field::host,"localhost");request.set(http::field::accept,"application/json");request.prepare_payload();http::write(stream,request);
  boost::beast::flat_buffer buffer;http::response<http::string_body> response;http::read(stream,buffer,response);return std::pair<unsigned,Json>{response.result_int(),Json::parse(response.body())};
 };
 auto beacon_observed=[&](int port){
  const auto [code,value]=beacon_get(port,"/eth/v1/node/syncing");if(code!=200)return false;
  const auto& data=value.at("data");return data.at("is_syncing")==false&&data.at("sync_distance")=="0"&&std::stoull(data.at("head_slot").get<std::string>())>=3;
 };
 auto latest=[&](int port){source->pump(0);observer->pump(0);return rpc(port,"eth_getBlockByNumber",Json::array({"latest",false}));};
 until([&]{const auto a=latest(source_port),b=latest(observer_port);return quantity(a.at("number"))>=3&&a.at("hash")==b.at("hash");},"independent Bellatrix cores synchronized",40);
 until([&]{return beacon_observed(source_port)&&beacon_observed(observer_port);},"Beacon slots observed through both P2P daemons",20);
 const auto [header_code,header]=beacon_get(source_port,"/eth/v1/beacon/headers/head");check(header_code==200&&header.at("data").at("canonical")==true,"native signed header HTTP unavailable");
 if(const auto* probe=std::getenv("NANO_TEST_BEACON_SDK_PROBE")){
  Process process({probe,url(source_port)});const auto result=process.wait(20);check(result.code==0,"Beacon SDK probe failed: "+result.err+result.out);std::cout<<result.out;
 }
 auto run_assertoor=[&](int port){
  if(const auto* probe=std::getenv("NANO_TEST_ASSERTOOR_PROBE")){
   Process process({probe,url(port),url(observer_port)});
   const auto seconds=std::getenv("NANO_TEST_ASSERTOOR_STABILITY_EPOCHS")?800:std::getenv("NANO_TEST_ASSERTOOR_FINALITY")?340:120;
   const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(seconds);
   while(process.running()){
    check(std::chrono::steady_clock::now()<deadline,"Assertoor process deadline");
    source->pump(0);observer->pump(0);process.pump();
   }
   const auto result=process.wait(1);std::cout<<result.out;check(result.code==0,"Assertoor probe failed: "+result.err+result.out);
  }
 };
 run_assertoor(source_port);
 const auto before=latest(source_port);
 if(crash){
  source->signal(SIGKILL);const auto killed=source->wait();check(killed.code==-SIGKILL,"validator did not receive SIGKILL");
  check(killed.out.find("bellatrix-runtime-stopped")==std::string::npos,"crash produced an orderly restart receipt");
  validator["bellatrix"]["initial"]=nullptr;
  std::cout<<Json{{"event","validator-killed"},{"signal","SIGKILL"},{"orderlyReceipt",false},{"headBefore",before.at("hash")}}.dump()<<std::endl;
 }else{
  auto result=source->stop();check(result.code==0,"managed validator shutdown: "+result.err);
  Json stopped;std::istringstream lines(result.out);std::string line;
  while(std::getline(lines,line)){auto j=Json::parse(line,nullptr,false);if(j.is_object()&&j.value("event"," ")=="bellatrix-runtime-stopped")stopped=std::move(j);}
  check(stopped.is_object()&&stopped.at("validatorKeys").size()==keys.size(),"managed shutdown missing journal commitments");
  validator["bellatrix"]["initial"]=nullptr;validator["bellatrix"]["restart"]=stopped.at("restart");
  for(size_t i=0;i<keys.size();++i)validator["bellatrix"]["validator"]["keys"][i]["restart"]=stopped.at("validatorKeys")[i].at("restart");
 }
 // Reconnect to the observer's live port; the source uses an ephemeral listener.
 validator["bootnodes"]=Json::array({observer->event("listen").at("address")});save(location/"validator.json",validator);
 source=launch("validator",environment);source->event("bellatrix-runtime-ready");const auto resumed_port=source->port();source->event("altair-ingress-ready");
 until([&]{const auto a=latest(resumed_port),b=latest(observer_port);return quantity(a.at("number"))>quantity(before.at("number"))&&a.at("hash")==b.at("hash");},"restarted Bellatrix validator propagated next block",40);
 until([&]{return beacon_observed(resumed_port)&&beacon_observed(observer_port);},"Beacon slot observations restored after restart",20);
 if(std::getenv("NANO_TEST_ASSERTOOR_STABILITY_EPOCHS"))run_assertoor(resumed_port);
 if(crash)check(rpc(resumed_port,"eth_getBlockByNumber",Json::array({before.at("number"),false})).at("hash")==before.at("hash"),"crash recovery replaced acknowledged canonical block");
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
   if(!(quantity(a.at("number"))>quantity(initial_finalized.at("number"))&&a.at("hash")==b.at("hash")&&latest(resumed_port).at("hash")==latest(observer_port).at("hash")))return false;
   // The Store can pull up finality before it appears in the head's post-state.
   // Start the sustained Beacon window only after BOTH projections have caught
   // up, within the same existing recovery deadline. Do not relax its thresholds.
   const auto [ac,av]=beacon_get(resumed_port,"/eth/v1/beacon/states/head/finality_checkpoints");
   const auto [bc,bv]=beacon_get(observer_port,"/eth/v1/beacon/states/head/finality_checkpoints");
   if(ac!=200||bc!=200||av.at("data").at("finalized")!=bv.at("data").at("finalized"))return false;
   const auto cp=av.at("data").at("finalized");const auto path="/eth/v2/beacon/blocks/"+cp.at("root").get<std::string>();
   const auto [abc,ab]=beacon_get(resumed_port,path.c_str());const auto [bbc,bb]=beacon_get(observer_port,path.c_str());
   if(abc!=200||bbc!=200||ab.at("data").at("message").at("body").at("execution_payload").at("block_hash")!=a.at("hash")||bb.at("data").at("message").at("body").at("execution_payload").at("block_hash")!=b.at("hash"))return false;
   std::cout<<Json{{"event","partition-recovery-bound"},{"beaconFinalized",cp},{"executionHash",a.at("hash")},{"elapsedSeconds",elapsed}}.dump()<<std::endl;
   return true;},"partition recovered common advancing finality",int(5*epoch_seconds));
  partition_recovered=true;
 }
 if(const auto* probe=std::getenv("NANO_TEST_POST_RECOVERY_PROBE")){
  Process process({probe,url(resumed_port),url(observer_port)});
  const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(360);
  while(process.running()){
   check(std::chrono::steady_clock::now()<deadline,"post-recovery stability deadline");
   source->pump(0);observer->pump(0);process.pump();
  }
  const auto result=process.wait(1);std::cout<<result.out;
  check(result.code==0,"post-recovery stability failed: "+result.err+result.out);
 }
 const auto after=latest(resumed_port);const auto source_end=source->stop(),observer_end=observer->stop();check(source_end.code==0&&observer_end.code==0,"managed peers failed shutdown");
 Json finality;
 if(distributed){
  auto shutdown=[](const Result& result){Json found;std::istringstream lines(result.out);std::string line;while(std::getline(lines,line)){auto j=Json::parse(line,nullptr,false);if(j.is_object()&&j.value("event","")=="bellatrix-runtime-stopped")found=std::move(j);}return found;};
  const auto a=shutdown(source_end),b=shutdown(observer_end);check(a.is_object()&&b.is_object(),"missing distributed shutdown evidence");
  std::cout<<Json{{"event","distributed-checkpoints"},{"a",a.at("finalized")},{"b",b.at("finalized")},{"justifiedA",a.at("justifiedEpoch")},{"justifiedB",b.at("justifiedEpoch")}}.dump()<<std::endl;
  check(quantity(a.at("finalized").at("epoch"))>=2&&a.at("finalized")==b.at("finalized"),"distributed validators did not finalize a common checkpoint");finality=a.at("finalized");
 }

 std::cout<<Json{{"scenario","bellatrix-managed-cores-restart"},{"independentCoreProcesses",2},{"p2pDaemons",2},{"wallClock",true},{"validatorNodes",distributed?2:1},{"physicalHosts",1},{"localKeys",64},{"blockBeforeRestart",before.at("number")},{"blockAfterRestart",after.at("number")},{"observerSynchronized",true},{"beaconPeerObservations",true},{"distributedStake",distributed},{"partitionRecovered",partition_recovered},{"crashRecovered",crash},{"finalized",finality}}.dump()<<std::endl;
}

}
