#include "nano/consensus_capella_bootstrap_input.hpp"
#include "nano/consensus_capella_runtime.hpp"
#include "../src/consensus_capella_startup_budget.hpp"
#include <rocksdb/db.h>
#include <chrono>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <sys/resource.h>
#include <unistd.h>
using namespace nano;
namespace fs=std::filesystem;
using Budget=nano::capella_detail::CapellaStartupBudget;
using Deadline=nano::capella_detail::CapellaStartupDeadlineExpired;
namespace {
const auto began=std::chrono::steady_clock::now();
unsigned completed_test_runtime_starts=0;
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
uint64_t rss(){rusage r{};require(::getrusage(RUSAGE_SELF,&r)==0,"resource observation");
#ifdef __APPLE__
 return uint64_t(r.ru_maxrss)/1024;
#else
 return uint64_t(r.ru_maxrss);
#endif
}
void checkpoint(){require(std::chrono::steady_clock::now()-began<std::chrono::seconds(60)&&rss()<=131072,"deadline test resource bound");}
struct Temp{fs::path path;Temp(){auto p=(fs::temp_directory_path()/"nano-startup-deadline-XXXXXX").string();require(::mkdtemp(p.data()),"owned temporary test directory");path=fs::canonical(p);}~Temp(){std::error_code e;fs::remove_all(path,e);}};
std::string read(const fs::path& p){std::ifstream f(p,std::ios::binary);require(bool(f),"public fixture read");std::string out((std::istreambuf_iterator<char>(f)),{});require(out.size()<=32*1024*1024,"fixture bound");return out;}
Bytes raw(const std::string& s){return {s.begin(),s.end()};}
void save(const fs::path& p,const Bytes& b){require(!fs::exists(p),"new public test artifact");std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),std::streamsize(b.size()));f.close();require(bool(f),"public test write");}
std::string q(uint64_t n){return quantity(Int(std::to_string(n)));}
template<class Exception,class Action>bool refused_as(Action action){try{action();}catch(const Exception&){return true;}return false;}
template<class Database=rocksdb::DB>
std::unique_ptr<Database> open_read_only(const rocksdb::Options& o,const std::string& p){std::unique_ptr<Database> d;
 if constexpr(requires { Database::OpenForReadOnly(o,p,&d); })require(Database::OpenForReadOnly(o,p,&d).ok(),"logical RO open");
 else{Database* value=nullptr;const auto s=Database::OpenForReadOnly(o,p,&value);d.reset(value);require(s.ok(),"logical RO open");}return d;
}
using Rows=std::map<std::string,std::string>;
Rows rows(const fs::path& p){rocksdb::Options o;o.create_if_missing=false;auto db=open_read_only(o,p.string());std::unique_ptr<rocksdb::Iterator> it(db->NewIterator({}));Rows out;size_t bytes=0;for(it->SeekToFirst();it->Valid();it->Next()){checkpoint();const auto k=it->key().ToString();const auto v=it->value().ToString();bytes+=k.size()+v.size();require(out.size()<1024&&bytes<=32*1024*1024,"logical inventory bound");out.emplace(k,v);}require(it->status().ok(),"complete logical inventory");return out;}
void emit(const char* name,const Json& details=Json::object()){auto j=details;j["scenario"]=name;j["result"]="PASS";j["newSignaturesCreated"]=0;j["privateInputsRead"]=false;j["runtimeUTCQualified"]=false;j["runtimeDutiesResumed"]=false;j["productionQualified"]=false;std::cout<<j.dump()<<std::endl;}
struct Fixture{Genesis genesis;CapellaForkBlockRules rules;CapellaStoreSnapshotLimits limits;CapellaForkStore initial;Bytes pre,wire;Json config,initial_descriptor;};
Fixture fixture(const fs::path& output){
 const auto dir=fs::path(NANO_TEST_SOURCE)/"tests/fixtures/capella-bootstrap-anchor-cb532";
 const auto manifest=read(dir/"manifest.json");require(hex(sha256(raw(manifest)))=="0xec150c28c8c187e6728cea7ef717acc6383e0b75242302b0a481fc2a9c425fc6","literal public fixture pin");const auto m=Json::parse(manifest);
 auto file=[&](const char* kind){const auto& p=m.at("files").at(kind);const auto text=read(dir/p.at("path").get<std::string>());require(p.at("bytes").get<size_t>()==text.size()&&p.at("sha256")==hex(sha256(raw(text))).substr(2),"public fixture file pin");return text;};
 const auto profile=file("profile"),initial=file("initial");auto rules=load_capella_fork_profile(profile,sha256(raw(profile)),1024*1024);auto g=Genesis::from_json(Json::parse(file("genesis")));const auto state_max=rules.transition.maximum_state_bytes;
 const CapellaStoreSnapshotLimits limits{16*1024*1024,4,64,64,state_max,4*state_max,64,100000};
 auto store=capella_store_from_snapshot(initial,sha256(raw(initial)),rules,limits,checkpoint);auto pre=raw(file("genesisState"));auto wire=unhex(m.at("bootstrapSigningHistory").at("block").at("ssz").get<std::string>());
 require(hex(sha256(wire))=="0x9575c3a186b101bd4a35bf56072bc9eba91d1d912e4ec462714eae0a60b08995","literal public wire pin");save(output/"genesis-state.ssz",pre);save(output/"signed-block.ssz",wire);
 const auto q=[](size_t n){return quantity(Int(std::to_string(n)));};
 Json config{{"profile",{{"file",(dir/"profile.json").string()},{"sha256",hex(sha256(raw(profile)))},{"maximumBytes","0x100000"}}},{"limits",{{"envelopeBytes",q(limits.envelope_bytes)},{"blocks",q(limits.blocks)},{"messages",q(limits.messages)},{"equivocations",q(limits.equivocations)},{"stateBytes",q(limits.state_bytes)},{"totalStateBytes",q(limits.total_state_bytes)},{"replaySlots",q(limits.replay_slots)},{"syncTrials",q(limits.sync_trials)}}},{"initial",nullptr},{"restart",nullptr},{"clock",{{"intervalMs","0x1"},{"maximumForwardStepMs",q(rules.slot_duration_ms)}}},{"automaticPruning",false},{"historicalStatePruning",false}};
 Json initial_descriptor{{"file",(dir/"initial.json").string()},{"sha256",hex(sha256(raw(initial)))}};
 return {std::move(g),std::move(rules),limits,std::move(store),std::move(pre),std::move(wire),std::move(config),std::move(initial_descriptor)};
}
Json descriptor(const fs::path& p,const Fixture& f){return {{"format","nano-capella-signed-bootstrap-proof-inputs-v1"},{"genesisState",{{"path",(p/"genesis-state.ssz").string()},{"sha256",hex(sha256(f.pre)).substr(2)},{"bytes",f.pre.size()}}},{"signedBlock",{{"path",(p/"signed-block.ssz").string()},{"sha256",hex(sha256(f.wire)).substr(2)},{"bytes",f.wire.size()}}}};}

Json fresh_config(const Fixture& f,const fs::path& inputs,const fs::path& witness){auto c=f.config;c["initial"]=f.initial_descriptor;c["recoveryWitness"]=witness.string();c["bootstrapArchive"]=descriptor(inputs,f);return c;}
void boundaries(const Fixture& f){
 using C=Budget::Clock;const auto origin=C::time_point{std::chrono::duration_cast<C::duration>(std::chrono::seconds(1))};auto at=origin;
 unsigned caller_calls=0;Budget b(10,[&]{++caller_calls;},origin,[&]{return at;});const auto check=b.checkpoint();check();at=origin+std::chrono::milliseconds(9);check();at=origin+std::chrono::milliseconds(10);require(refused_as<Deadline>(check),"exact elapsed boundary accepted");at=origin+std::chrono::milliseconds(11);require(refused_as<Deadline>(check),"after elapsed boundary accepted");
 at=origin;Budget backward(10,{},origin,[&]{return at;});const auto back=backward.checkpoint();at=origin+std::chrono::milliseconds(5);back();at=origin+std::chrono::milliseconds(4);require(refused_as<std::runtime_error>(back),"steady regression accepted");
 require(refused_as<std::invalid_argument>([&]{Budget x(0,{},origin,[&]{return at;});}),"zero helper budget accepted");
 require(refused_as<std::invalid_argument>([&]{Budget x(UINT64_MAX,{},origin,[&]{return at;});}),"unrepresentable budget accepted");
 Budget extreme(10,{},C::time_point::min(),[]{return C::time_point::max();});require(refused_as<std::overflow_error>(extreme.checkpoint()),"steady subtraction overflow accepted");
 unsigned legacy_calls=0;Budget legacy(std::nullopt,[&]{++legacy_calls;},origin,[]{throw std::runtime_error("legacy read steady");return C::time_point{};});const auto lc=legacy.checkpoint();lc();require(legacy_calls>0,"absent budget lost caller cancellation");const auto before=legacy_calls;legacy.finish();lc();require(legacy_calls==before,"finished absent budget called caller");
 for(const auto& invalid:{Json("0x0"),Json("0x00"),Json("0x01"),Json("0xA"),Json(UINT64_MAX),Json(nullptr),Json("0xffffffffffffffff")}){
  Temp t;{Blockchain initialize(t.path/"primary",f.genesis);}const auto logical_before=rows(t.path/"primary");auto cfg=f.config;cfg["clock"]["maximumStartupElapsedMs"]=invalid;cfg["profile"]["file"]=(t.path/"absent-profile").string();cfg["recoveryWitness"]=(t.path/"absent-witness").string();
  bool callback_reached=false;{Blockchain owner(t.path/"primary",f.genesis,ChainStore::OpenMode::ReadWriteExisting);require(refused_as<std::invalid_argument>([&]{start_capella_runtime(owner,cfg,[&]{callback_reached=true;checkpoint();});}),"invalid elapsed config accepted");}require(!callback_reached&&!fs::exists(t.path/"absent-witness")&&rows(t.path/"primary")==logical_before,"invalid elapsed config reached startup callback or changed owner/witness");
 }
 emit("startup-deadline-boundaries-and-compatibility",{{"exactBoundaryRefused",true},{"invalidBeforeProfileRead",true},{"legacyNoSteadyRead",true}});
}
struct CallerCancelled{};struct BeforeClock{};
void inputs_and_categories(const Fixture& f,const fs::path& inputs){
 {Temp t;Blockchain owner(t.path/"primary",f.genesis);auto cfg=fresh_config(f,inputs,t.path/"witness");cfg["clock"]["maximumStartupElapsedMs"]=q(5000);bool changed=false;bool reached=false;
  try{start_capella_runtime(owner,cfg,[&]{if(!changed){changed=true;cfg["clock"]["maximumStartupElapsedMs"]="0x0";cfg["unexpectedCallerField"]=true;}checkpoint();},[&]()->uint64_t{reached=true;throw BeforeClock{};});}catch(const BeforeClock&){}
  require(changed&&reached,"caller alias changed owned runtime budget/config");}
 {Temp t;{Blockchain initialize(t.path/"primary",f.genesis);}const auto before=rows(t.path/"primary");auto cfg=fresh_config(f,inputs,t.path/"witness");cfg["clock"]["maximumStartupElapsedMs"]=q(5000);
  {Blockchain owner(t.path/"primary",f.genesis,ChainStore::OpenMode::ReadWriteExisting);require(refused_as<CallerCancelled>([&]{start_capella_runtime(owner,cfg,[]{throw CallerCancelled{};});}),"caller cancellation type rewritten");}require(!fs::exists(t.path/"witness")&&rows(t.path/"primary")==before,"caller cancellation before IO changed stores");}
 {Temp t;Blockchain owner(t.path/"primary",f.genesis);auto cfg=fresh_config(f,inputs,t.path/"witness");cfg["clock"]["maximumStartupElapsedMs"]=q(5000);cfg["profile"]["sha256"]=hex(Bytes(32));
  require(refused_as<std::invalid_argument>([&]{start_capella_runtime(owner,cfg,checkpoint);}),"bad public profile masked as deadline");require(!fs::exists(t.path/"witness"),"bad public profile opened witness");}
 emit("startup-deadline-owned-inputs-and-error-types",{{"copiedBudgetAndConfiguration",true},{"callerCancellationTypePreserved",true},{"invalidProfileStillRefused",true}});
}
void actual_runtime_expiration(const Fixture& f,const fs::path& inputs){
 Temp t;{Blockchain initialize(t.path/"primary",f.genesis);}const auto before=rows(t.path/"primary");auto cfg=fresh_config(f,inputs,t.path/"witness");cfg["clock"]["maximumStartupElapsedMs"]="0x1";bool caller_waited=false;bool expired=false;
 {Blockchain owner(t.path/"primary",f.genesis,ChainStore::OpenMode::ReadWriteExisting);expired=refused_as<Deadline>([&]{start_capella_runtime(owner,cfg,[&]{caller_waited=true;std::this_thread::sleep_for(std::chrono::milliseconds(20));checkpoint();});});}
 require(expired&&!fs::exists(t.path/"witness")&&rows(t.path/"primary")==before,"real steady expiry failed before runtime file publication");
 // Expiry can precede the caller if the real process is already delayed.
 emit("startup-deadline-real-runtime-expiration",{{"realSteadyUsed",true},{"deadlineTypeObserved",expired},{"callerWaitObserved",caller_waited},{"noRuntimeHandleReturned",true},{"noPublicationObserved",true}});
}
void durable_prefix(const Fixture& f,const fs::path& inputs){
 Temp t;ForkCheckpointToken old_primary{},retained_primary{},retained_witness{};std::string retained_snapshot;uint64_t expected_time=0;
 {Blockchain owner(t.path/"primary",f.genesis);ChainStore witness(t.path/"witness");auto proof=load_capella_signed_bootstrap_proof(f.genesis,f.rules,f.limits,descriptor(inputs,f),checkpoint);CapellaDurableReactor reactor(owner,f.rules,f.limits,f.initial,std::move(proof),witness,checkpoint);old_primary=reactor.current().token;
  const auto base=reactor.current().state->timed.clock.time_ms;const auto step=f.rules.slot_duration_ms;require(step&&step<=(UINT64_MAX-base)/3,"fixture clock bound");const auto target=base+3*step;expected_time=base+step;
  using C=Budget::Clock;const auto origin=C::time_point{};auto at=origin;Budget budget(10,checkpoint,origin,[&]{return at;});const auto bounded=budget.checkpoint();
  CapellaLocalClock clock(reactor,step,[&]{const auto current=owner.store().fork_checkpoint(f.genesis.chain_id,f.genesis.block.header.hash(),f.limits.envelope_bytes);require(bool(current),"fixture persisted checkpoint missing");if(current->token.sequence>old_primary.sequence)at=origin+std::chrono::milliseconds(10);return target;});
  require(refused_as<Deadline>([&]{clock.startup_catch_up(3*step,3,bounded);}),"deadline not observed after genuine published prefix");require(clock.healthy(),"cooperative budget expiry latched corrupt clock");const auto current=reactor.current();require(current.token.sequence==old_primary.sequence+1&&current.state->timed.clock.time_ms==expected_time,"deadline rolled back or fabricated bounded prefix");
  const auto p=owner.store().fork_checkpoint(f.genesis.chain_id,f.genesis.block.header.hash(),f.limits.envelope_bytes,current.token);const auto w=witness.fork_checkpoint(f.genesis.chain_id,f.genesis.block.header.hash(),2048+8*f.limits.envelope_bytes);require(p&&w,"published prefix missing primary/witness");const auto intent=Json::parse(w->snapshot);require(intent.at("snapshot").get<std::string>()==p->snapshot&&intent.at("next").at("digest").get<std::string>()==hex(p->token.digest),"witness prefix differs from primary");
  const auto oracle=prepare_capella_store_tick(f.initial,expected_time,f.rules,f.limits.replay_slots,f.limits.sync_trials,checkpoint);require(capella_store_snapshot(*current.state,f.rules,f.limits,checkpoint)==capella_store_snapshot(oracle,f.rules,f.limits,checkpoint),"published prefix differs from real transition oracle");require(reactor.signed_votes().empty(),"catch-up fabricated signatures/votes");retained_primary=p->token;retained_witness=w->token;retained_snapshot=p->snapshot;
 }
 {Blockchain owner(t.path/"primary",f.genesis,ChainStore::OpenMode::ReadWriteExisting);ChainStore witness(t.path/"witness",ChainStore::OpenMode::ReadWriteExisting);
  require(refused_as<std::exception>([&]{owner.store().fork_checkpoint(f.genesis.chain_id,f.genesis.block.header.hash(),f.limits.envelope_bytes,old_primary);}),"old primary commitment accepted after partial catch-up");const auto wi=witness.fork_checkpoint(f.genesis.chain_id,f.genesis.block.header.hash(),2048+8*f.limits.envelope_bytes,retained_witness);require(bool(wi),"external witness commitment refused");CapellaDurableReactor restored(owner,f.rules,f.limits,witness,checkpoint,retained_primary);require(restored.current().token==retained_primary&&restored.current().state->timed.clock.time_ms==expected_time,"current prefix lost at cold reopen");const auto p=owner.store().fork_checkpoint(f.genesis.chain_id,f.genesis.block.header.hash(),f.limits.envelope_bytes,retained_primary);require(p&&p->snapshot==retained_snapshot,"reopen changed retained raw snapshot");
 }
 emit("startup-deadline-durable-prefix-reopen",{{"actualClockPublications",1},{"currentPrimaryAndWitnessRetained",true},{"oldPrimaryPinRefused",true},{"rawSnapshotPreserved",true},{"privateJournalFloorsQualified",false}});
}
void handoff_lifetime(const Fixture& f,const fs::path& inputs){
 using C=Budget::Clock;const auto at=std::make_shared<C::time_point>();const auto calls=std::make_shared<unsigned>(0);std::function<void()> retained;
 {Budget b(10,[calls]{++*calls;},C::time_point{},[at]{return *at;});retained=b.checkpoint();retained();b.finish();}const auto before=*calls;*at+=std::chrono::seconds(10);retained();require(*calls==before,"retained finished callback invoked expired budget/caller");
 std::function<void()> unwound;{Budget b(10,[calls]{++*calls;},C::time_point{},[at]{return *at;});unwound=b.checkpoint();}unwound();require(*calls==before,"retained RAII-closed callback invoked caller");
 {Temp t;Blockchain owner(t.path/"primary",f.genesis);auto cfg=fresh_config(f,inputs,t.path/"witness");const auto now=f.initial.timed.clock.time_ms;auto runtime=start_capella_runtime(owner,cfg,checkpoint,[now]{return now;});++completed_test_runtime_starts;require(!runtime.validator&&runtime.clock&&runtime.clock->healthy(),"legacy valid test runtime failed");stop_capella_runtime(runtime);}
 {Temp t;Blockchain owner(t.path/"primary",f.genesis);auto cfg=fresh_config(f,inputs,t.path/"witness");const uint64_t maximum=5000;cfg["clock"]["maximumStartupElapsedMs"]=q(maximum);cfg["coldCheckpoint"]={{"cacheRecords","0x1"},{"cacheEncodedBytes",q(f.limits.envelope_bytes)}};const auto after_handoff=std::make_shared<std::atomic<bool>>(false);const auto now=f.initial.timed.clock.time_ms;
  auto runtime=start_capella_runtime(owner,cfg,[after_handoff]{require(!after_handoff->load(std::memory_order_acquire),"startup callback retained by runtime after handoff");checkpoint();},[now]{return now;});++completed_test_runtime_starts;after_handoff->store(true,std::memory_order_release);
  // Real time has crossed the original deadline. The cold reader/clock/RPC must
  // retain their own normal callbacks, not a startup budget or borrowed locals.
  std::this_thread::sleep_for(std::chrono::milliseconds(maximum+20));checkpoint();require(runtime.clock->healthy()&&!runtime.validator,"finished startup deadline stopped test runtime");runtime.clock->poll(checkpoint);const auto published=runtime.reactor->current();const auto anchor=published.state->blocks.at(published.state->timed.view.anchor).lease(checkpoint);require(bool(anchor),"post-handoff cold lease failed");const auto response=runtime.beacon_read("/eth/v1/beacon/genesis",checkpoint);require(response.status==200,"post-handoff public read failed");const auto stopped=stop_capella_runtime(runtime);require(stopped.at("clockHealthy").get<bool>(),"normal stop sees expired startup budget");
 }
 emit("startup-deadline-handoff-and-retained-callback-lifetime",{{"retainedCheckSafeAfterScope",true},{"deadlineInactiveBeforeWorkers",true},{"realTimePastStartupBudget",true},{"legacyAndBudgetedRuntimeReturnsObserved",2},{"coldReadAfterHandoff",true}});
}
}
int main(int argc,char** argv){try{require(argc==2&&std::string(argv[1])=="--self-test","usage --self-test");Temp input;const auto f=fixture(input.path);boundaries(f);inputs_and_categories(f,input.path);actual_runtime_expiration(f,input.path);durable_prefix(f,input.path);handoff_lifetime(f,input.path);checkpoint();const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-began).count();std::cout<<Json{{"scenario","capella-startup-deadline-self-test"},{"result","PASS"},{"groups",5},{"elapsedMs",elapsed},{"maxRssKiB",rss()},{"completedTestRuntimeStarts",completed_test_runtime_starts},{"newSignaturesCreated",0},{"runtimeUTCQualified",false},{"runtimeDutiesResumed",false},{"privateInputsRead",false},{"networkUsed",false},{"powerLossQualified",false},{"productionQualified",false}}.dump()<<std::endl;return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
