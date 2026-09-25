#include "support/altair_transition_rules.hpp"
#include "nano/consensus_altair_runtime.hpp"
#include "nano/consensus_altair_clock.hpp"
#include "../network/altair_rpc_service.hpp"
using namespace test;using namespace test::phase0_minimal;
int main(int argc,char** argv){try{
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures",corpus=fixtures/"consensus-altair-fork-head-official",dir=corpus/"chain_no_attestations";
 const auto manifest=load(corpus/"manifest.json");for(const auto& row:manifest.at("cases"))if(row.at("name")=="chain_no_attestations")for(const auto& [path,info]:row.at("files").items())check(hex(sha256(binary(corpus/path))).substr(2)==info.at("sha256").get<std::string>(),"runtime fixture digest");
 const auto transition=altair_transition_rules(fixtures);const auto reference_config=read(corpus/"minimal-config-v1.6.0-beta.0.yaml");const auto duration=parameter(reference_config,"SECONDS_PER_SLOT")*1000;
 const AltairForkBlockRules rules{transition,duration,duration/3,parameter(reference_config,"PROPOSER_SCORE_BOOST"),AltairProposerBoostProfile::Release160Beta0};
 const auto state=binary(dir/"anchor_state.ssz");const auto anchor=initialize_altair_fork_store({state,std::nullopt},binary(dir/"anchor_block.ssz"),rules);
 Temp temp;const auto profile=altair_transition_profile(transition);save(temp/"profile.json",profile);const auto profile_bytes=binary(temp/"profile.json");
 auto q=[](uint64_t n){return quantity(Int(std::to_string(n)));};auto roothex=[](const ConsensusRoot& r){return hex(Bytes(r.begin(),r.end()));};
 const auto genesis_json=Json{{"config",{{"chainId",1337},{"evmFork","Shanghai"}}},{"alloc",Json::object()}};const auto genesis=Genesis::from_json(genesis_json);const auto genesis_hash=genesis.block.header.hash();
 Json config{
 {"profile",{{"file",(temp/"profile.json").string()},{"sha256",hex(sha256(profile_bytes))},{"maximumBytes",q(profile_bytes.size())}}},
 {"forkChoice",{{"slotDurationMs",q(duration)},{"attestationDueMs",q(duration/3)},{"proposerScoreBoost",q(rules.proposer_score_boost)},{"boostProfile","release-1.6.0-beta.0"}}},
 {"anchor",{{"stateFile",(dir/"anchor_state.ssz").string()},{"blockFile",(dir/"anchor_block.ssz").string()},{"blockRoot",roothex(anchor.timed.view.anchor)},{"stateRoot",roothex(altair_beacon_state_root(state,transition.state))},{"genesisValidatorsRoot",roothex(root_at(state,8))},{"forkVersion",hex(Bytes(state.begin()+52,state.begin()+56))}}},
 {"database",(temp/"consensus").string()},
 {"limits",{{"envelopeBytes",q(32*1024*1024)},{"blocks","0x64"},{"messages","0x2710"},{"equivocations","0x2710"},{"stateBytes",q(1024*1024)},{"totalStateBytes",q(16*1024*1024)},{"replaySlots","0x1000"},{"syncTrials","0x186a0"}}},
 {"wire",{{"frameBytes",q(2*transition.maximum_block_bytes+1024)},{"blockBytes",q(transition.maximum_block_bytes)},{"attestationBytes","0x200"},{"slashingBytes","0x9c40"}}},
 {"worker",{{"jobs","0x4"},{"payloadBytes",q(transition.maximum_block_bytes)},{"timeoutMs","0x2710"}}},{"restart",nullptr},{"clock",{{"enabled",false},{"intervalMs","0x64"},{"maximumForwardStepMs","0xea60"}}}};
 const AltairAnchorCommitment commitment{anchor.timed.view.anchor,altair_beacon_state_root(state,transition.state),root_at(state,8),{}};
 auto identity_anchor=commitment;std::copy_n(state.begin()+52,4,identity_anchor.fork_version.begin());
 const auto identity=altair_consensus_identity(rules,identity_anchor,1337,genesis_hash);
 unsigned identity_checks=0;
 auto different=[&](const AltairForkBlockRules& changed,const AltairAnchorCommitment& changed_anchor,const Int& chain,const Bytes& hash){check(altair_consensus_identity(changed,changed_anchor,chain,hash)!=identity,"different consensus profile shared an identity");++identity_checks;};
 auto changed_rules=rules;++changed_rules.slot_duration_ms;different(changed_rules,identity_anchor,1337,genesis_hash);
 changed_rules=rules;++changed_rules.attestation_due_ms;different(changed_rules,identity_anchor,1337,genesis_hash);
 changed_rules=rules;++changed_rules.proposer_score_boost;different(changed_rules,identity_anchor,1337,genesis_hash);
 changed_rules=rules;changed_rules.boost_profile=AltairProposerBoostProfile::ShufflingDependent;different(changed_rules,identity_anchor,1337,genesis_hash);
 changed_rules=rules;++changed_rules.transition.maximum_block_bytes;different(changed_rules,identity_anchor,1337,genesis_hash);
 changed_rules=rules;++changed_rules.transition.epoch.registry.minimum_churn_limit;different(changed_rules,identity_anchor,1337,genesis_hash);
 for(auto member:{&AltairAnchorCommitment::block_root,&AltairAnchorCommitment::state_root,&AltairAnchorCommitment::genesis_validators_root}){auto changed=identity_anchor;(changed.*member)[0]^=1;different(rules,changed,1337,genesis_hash);}
 auto changed_anchor=identity_anchor;changed_anchor.fork_version[0]^=1;different(rules,changed_anchor,1337,genesis_hash);
 different(rules,identity_anchor,1338,genesis_hash);auto changed_genesis=genesis_hash;changed_genesis[0]^=1;different(rules,identity_anchor,1337,changed_genesis);
 unsigned refused=0;
 auto refuse=[&](const Json& input){bool failed=false;try{start_altair_runtime(input,1337,genesis_hash);}catch(const std::exception&){failed=true;}check(failed,"invalid runtime startup accepted");++refused;};
 auto bad=config;bad["worker"]["jobs"]="0x0";refuse(bad);check(!fs::exists(temp/"consensus"),"invalid config opened database");
 bad=config;bad["restart"]={{"sequence","0x1"},{"digest",hex(Bytes(32,0))}};refuse(bad);check(!fs::exists(temp/"consensus"),"missing restart initialized a database");
 bad=config;bad["networkAdmission"]=true;refuse(bad);check(!fs::exists(temp/"consensus"),"network mode without clock touched storage");
 bad=config;bad["networkAdmission"]=true;bad["clock"]["enabled"]=true;bad["wire"]["blockBytes"]=q(16*1024*1024);bad["wire"]["frameBytes"]=q(32*1024*1024+1024);bad["worker"]["payloadBytes"]=q(16*1024*1024);refuse(bad);check(!fs::exists(temp/"consensus"),"oversized IPC payload touched storage");
 {auto network=config;network["networkAdmission"]=true;network["clock"]["enabled"]=true;network["database"]=(temp/"network-config").string();auto runtime=start_altair_runtime(network,1337,genesis_hash,{},[&]{return anchor.timed.clock.time_ms;});
  check(runtime.rpc->status().at("networkAdmissionEnabled")==true,"explicit admission mode lost");bool synchronous=false;try{runtime.rpc->receive("{}");}catch(const RpcError& e){synchronous=e.code==-32601;}check(synchronous,"network mode allowed unfenced synchronous admission");
 }
 ForkCheckpointToken token;
 {auto runtime=start_altair_runtime(config,1337,genesis_hash);token=runtime.reactor->current().token;check(token.sequence==1,"fresh startup sequence");
  const auto advertised=runtime.rpc->status();check(advertised.at("consensusIdentity")==hex(identity),"runtime consensus identity mismatch");
  check(advertised.at("wire").at("blockBytes")==transition.maximum_block_bytes,"runtime wire limits absent");
  auto service=runtime.rpc;runtime={};
  ConsensusBytes4 version{};std::copy_n(state.begin()+52,4,version.begin());const auto fork=consensus_fork_data_root(version,root_at(state,8));
  const auto frame=Json{{"chainId","0x539"},{"genesisHash",hex(genesis_hash)},{"forkDigest",hex(Bytes(fork.begin(),fork.begin()+4))},{"kind","attestation"},{"ssz","0x0001"}}.dump();
  check(service->receive(frame)==Json{{"accepted",false}},"RPC alias did not retain database/reactor lifetime");
 }
 refuse(config); // Must never silently initialize over existing state.
 config["restart"]={{"sequence",q(token.sequence)},{"digest",hex(token.digest)}};
 bad=config;bad["restart"]["sequence"]="0x2";refuse(bad);
 bad=config;bad["anchor"]["blockRoot"]=hex(Bytes(32,0));refuse(bad);
 bad=config;bad["worker"]["payloadBytes"]="0x1";refuse(bad);
 {auto runtime=start_altair_runtime(config,1337,genesis_hash);check(runtime.reactor->current().token==token,"restart changed checkpoint");runtime.reactor->tick(anchor.timed.clock.time_ms+1);token=runtime.reactor->current().token;}
 refuse(config);config["restart"]={{"sequence",q(token.sequence)},{"digest",hex(token.digest)}};
 {auto runtime=start_altair_runtime(config,1337,genesis_hash);check(runtime.reactor->current().token==token,"latest restart failed");check(runtime.rpc->status().at("consensusIdentity")==hex(identity),"restart changed consensus identity");}
 // Serialization formatting and local budgets must not split compatible peers.
 auto local_config=config;local_config["database"]=(temp/"other-local-policy").string();local_config["restart"]=nullptr;local_config["worker"]["jobs"]="0x8";
 {std::ofstream output(temp/"formatted-profile.json");output<<profile.dump(4);}
 local_config["profile"]={{"file",(temp/"formatted-profile.json").string()},{"sha256",hex(sha256(binary(temp/"formatted-profile.json")))},{"maximumBytes",q(binary(temp/"formatted-profile.json").size())}};
 {auto runtime=start_altair_runtime(local_config,1337,genesis_hash);check(runtime.rpc->status().at("consensusIdentity")==hex(identity),"local policy or profile whitespace changed identity");++identity_checks;}
 unsigned clock_guards=0;
 {auto manual=config;manual["database"]=(temp/"manual-clock").string();manual["restart"]=nullptr;
  auto runtime=start_altair_runtime(manual,1337,genesis_hash);auto now=anchor.timed.clock.time_ms;
  AltairLocalClock clock(*runtime.reactor,10,[&]{return now;});
  const auto before=runtime.reactor->current().token;check(!clock.poll()&&runtime.reactor->current().token==before,"duplicate clock tick committed");++clock_guards;
  ++now;bool cancelled=false;try{clock.poll([]{throw std::runtime_error("cancelled");});}catch(const std::runtime_error& e){cancelled=std::string(e.what())=="cancelled";}
  check(cancelled&&clock.healthy()&&runtime.reactor->current().token==before,"clock cancellation poisoned runtime");++clock_guards;
  check(clock.poll()&&runtime.reactor->current().state->timed.clock.time_ms==now,"clock did not advance");++clock_guards;
  --now;bool backwards=false;try{clock.poll();}catch(const std::runtime_error&){backwards=true;}
  check(backwards&&!clock.healthy(),"backwards clock accepted");++clock_guards;
  now+=2;bool latched=false;try{clock.poll();}catch(const std::runtime_error&){latched=true;}check(latched,"clock error not latched");++clock_guards;
 }
 {auto automatic=config;automatic["database"]=(temp/"automatic-clock").string();automatic["restart"]=nullptr;
  automatic["clock"]={{"enabled",true},{"intervalMs","0x1"},{"maximumForwardStepMs","0xa"}};
  std::atomic<uint64_t> now{anchor.timed.clock.time_ms};auto runtime=start_altair_runtime(automatic,1337,genesis_hash,{},[&]{return now.load();});
  now.fetch_add(1);until([&]{return runtime.reactor->current().token.sequence==2;},"periodic Altair clock advanced");++clock_guards;
  const auto status=runtime.rpc->status();check(status.at("clockHealthy")==true&&status.at("sequence")=="0x2","clock status mismatch");
  ConsensusBytes4 version{};std::copy_n(state.begin()+52,4,version.begin());const auto fork=consensus_fork_data_root(version,root_at(state,8));
  const auto frame=Json{{"chainId","0x539"},{"genesisHash",hex(genesis_hash)},{"forkDigest",hex(Bytes(fork.begin(),fork.begin()+4))},{"kind","attestation"},{"ssz","0x0001"}}.dump();
  now.fetch_add(1);check(runtime.rpc->receive(frame)==Json{{"accepted",false}}&&runtime.reactor->current().token.sequence==3,"message processed with stale local time");++clock_guards;
  now.fetch_add(11);bool failed=false;try{runtime.rpc->receive(frame);}catch(const RpcError& e){failed=e.code==-32603;}
  check(failed&&runtime.rpc->status().at("clockHealthy")==false&&runtime.reactor->current().token.sequence==3,"forward jump admitted work or changed clock");++clock_guards;
 }
 bool managed=false;
 if(argc==3){
  save(temp/"genesis.json",genesis_json);
  auto managed_config=config;managed_config["database"]=(temp/"managed-consensus").string();managed_config["restart"]=nullptr;
  Json node{{"rpc_host","127.0.0.1"},{"rpc_port",0},{"p2p_binary",argv[2]},{"listen_addr","/ip4/127.0.0.1/tcp/0"},{"bootnodes",Json::array()},{"altair",managed_config}};save(temp/"node.json",node);
  Process child({argv[1],"run",(temp/"genesis.json").string(),(temp/"execution").string(),(temp/"node.json").string()});
  const auto ready=child.event("altair-runtime-ready");check(ready.at("sequence")=="0x1"&&ready.at("executionBound")==false&&ready.at("networkProtocolActive")==false,"managed status misreports activation");
  const auto port=child.port();child.event("ready");check(child.event("altair-ipc-ready").at("consensusIdentity")==hex(identity),"managed daemon did not verify/drain Altair IPC");check(rpc(port,"eth_chainId")=="0x539","managed RPC unavailable");
  bool unauthorized=false;try{rpc(port,"nano_altairReceive",Json::array({"wrong","{}"}));}catch(const std::exception& e){unauthorized=std::string(e.what()).find("Unauthorized Altair")!=std::string::npos;}check(unauthorized,"managed Altair endpoint not present/authenticated");
  check(child.stop().code==0,"managed shutdown failed");
  node["altair"]["restart"]={{"sequence",ready.at("sequence")},{"digest",ready.at("digest")}};save(temp/"node.json",node);
  Process resumed({argv[1],"run",(temp/"genesis.json").string(),(temp/"execution").string(),(temp/"node.json").string()});
  check(resumed.event("altair-runtime-ready").at("digest")==ready.at("digest"),"managed restart changed checkpoint");resumed.event("ready");check(resumed.event("altair-ipc-ready").at("consensusIdentity")==hex(identity),"resumed daemon did not recover Altair IPC");check(resumed.stop().code==0,"managed restart shutdown failed");managed=true;
 }
 std::cout<<Json{{"identityChecks",identity_checks},{"clockGuards",clock_guards},{"scenario","altair-runtime-start-restart"},{"refused",refused},{"latestSequence",token.sequence},{"ownedLifetime",true},{"managedProcesses",managed},{"networkAltairActive",false},{"executionBound",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
