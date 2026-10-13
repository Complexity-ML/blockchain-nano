#include "nano/consensus_capella_bootstrap_input.hpp"
#include "nano/consensus_capella_runtime.hpp"
#include <rocksdb/db.h>
#include <chrono>
#include <cerrno>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <fcntl.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
using namespace nano;
namespace fs=std::filesystem;
namespace {
const auto began=std::chrono::steady_clock::now();
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void checkpoint(){rusage r{};require(::getrusage(RUSAGE_SELF,&r)==0,"resource observation");
#ifdef __APPLE__
 const auto memory=r.ru_maxrss/1024;
#else
 const auto memory=r.ru_maxrss;
#endif
 require(std::chrono::steady_clock::now()-began<std::chrono::seconds(60)&&memory<=131072,"startup test resource bound");}
struct Temp{fs::path path;Temp(){auto p=(fs::temp_directory_path()/"nano-bootstrap-startup-XXXXXX").string();require(::mkdtemp(p.data()),"owned test directory");path=fs::canonical(p);}~Temp(){std::error_code e;fs::remove_all(path,e);}};
std::string read(const fs::path& p){std::ifstream f(p,std::ios::binary);require(bool(f),"public fixture read");std::string s((std::istreambuf_iterator<char>(f)),{});require(s.size()<=32*1024*1024,"fixture bound");return s;}
Bytes raw(const std::string& s){return {s.begin(),s.end()};}
void save(const fs::path& p,const Bytes& b){require(!fs::exists(p),"new test artifact");std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),std::streamsize(b.size()));f.close();require(bool(f),"test artifact write");}
Json pin(const ForkCheckpointToken& t){return {{"sequence",quantity(Int(std::to_string(t.sequence)))},{"digest",hex(t.digest)}};}
template<class F>void refuses(F action,const char* why){bool rejected=false;try{action();}catch(const std::exception&){rejected=true;}require(rejected,why);}
// Detect both admitted RocksDB APIs without release-number assumptions.
template<class Database=rocksdb::DB>
std::unique_ptr<Database> open_read_only(const rocksdb::Options& o,const std::string& path){
 std::unique_ptr<Database> d;
 if constexpr(requires { Database::OpenForReadOnly(o,path,&d); })require(Database::OpenForReadOnly(o,path,&d).ok(),"logical RO read");
 else{Database* x=nullptr;const auto status=Database::OpenForReadOnly(o,path,&x);d.reset(x);require(status.ok(),"logical RO read");}return d;
}
template<class Database=rocksdb::DB>
std::unique_ptr<Database> open_write(const rocksdb::Options& o,const std::string& path){
 std::unique_ptr<Database> d;
 if constexpr(requires { Database::Open(o,path,&d); })require(Database::Open(o,path,&d).ok(),"owned adverse open");
 else{Database* x=nullptr;const auto status=Database::Open(o,path,&x);d.reset(x);require(status.ok(),"owned adverse open");}return d;
}
using Rows=std::map<std::string,std::string>;
Rows rows(const fs::path& p){rocksdb::Options o;o.create_if_missing=false;auto d=open_read_only(o,p.string());std::unique_ptr<rocksdb::Iterator> it(d->NewIterator({}));Rows out;size_t bytes=0;for(it->SeekToFirst();it->Valid();it->Next()){checkpoint();auto k=it->key().ToString(),v=it->value().ToString();bytes+=k.size()+v.size();require(out.size()<1024&&bytes<=32*1024*1024,"logical inventory bound");out.emplace(std::move(k),std::move(v));}require(it->status().ok(),"complete logical read");return out;}
void erase(const fs::path& p,const std::string& key){rocksdb::Options o;o.create_if_missing=false;auto d=open_write(o,p.string());rocksdb::WriteOptions w;w.sync=true;require(d->Delete(w,key).ok(),"owned adverse delete");}
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
void emit(const char* name){std::cout<<Json{{"scenario",name},{"result","PASS"},{"runtimeDutiesResumed",false},{"privateInputsRead",false},{"productionQualified",false}}.dump()<<std::endl;}
void inputs(const Fixture& f,const fs::path& p){
 auto d=descriptor(p,f);auto proof=load_capella_signed_bootstrap_proof(f.genesis,f.rules,f.limits,d,checkpoint);require(proof.signed_wire()==f.wire&&proof.header().slot==1,"valid general proof loader");
 for(unsigned i=0;i<7;++i){auto bad=d;if(i==0)bad["extra"]=true;else if(i==1)bad["genesisState"]["bytes"]=0;else if(i==2)bad["signedBlock"]["bytes"]=f.wire.size()+1;else if(i==3)bad["signedBlock"]["sha256"]=std::string(64,'0');else if(i==4)bad["genesisState"]["sha256"]="0x"+hex(sha256(f.pre)).substr(2);else if(i==5)bad["genesisState"]["path"]="relative.ssz";else bad["format"]="trusted-certificate";refuses([&]{load_capella_signed_bootstrap_proof(f.genesis,f.rules,f.limits,bad,checkpoint);},"bad artifact descriptor accepted");}
 fs::create_symlink(p/"signed-block.ssz",p/"wire-link");auto bad=d;bad["signedBlock"]["path"]=(p/"wire-link").string();refuses([&]{load_capella_signed_bootstrap_proof(f.genesis,f.rules,f.limits,bad,checkpoint);},"symlink leaf accepted");
 fs::create_directory(p/"real-parent");save(p/"real-parent/wire.ssz",f.wire);fs::create_directory_symlink(p/"real-parent",p/"alias-parent");bad=d;bad["signedBlock"]["path"]=(p/"alias-parent/wire.ssz").string();refuses([&]{load_capella_signed_bootstrap_proof(f.genesis,f.rules,f.limits,bad,checkpoint);},"symlink ancestor accepted");
 require(::mkfifo((p/"pipe").c_str(),0600)==0,"owned FIFO");bad=d;bad["signedBlock"]["path"]=(p/"pipe").string();refuses([&]{load_capella_signed_bootstrap_proof(f.genesis,f.rules,f.limits,bad,checkpoint);},"FIFO accepted");
 struct Cancel{};bool cancelled=false;try{load_capella_signed_bootstrap_proof(f.genesis,f.rules,f.limits,d,[]{throw Cancel{};});}catch(const Cancel&){cancelled=true;}require(cancelled,"typed caller cancellation lost");
 auto external=d;bool changed=false;auto owned=load_capella_signed_bootstrap_proof(f.genesis,f.rules,f.limits,external,[&]{if(!changed){changed=true;external["signedBlock"]["sha256"]=std::string(64,'0');}checkpoint();});require(changed&&owned.signed_wire()==f.wire,"caller descriptor aliases loader authority");emit("pinned-proof-inputs-and-refusals");
 size_t callbacks=0;load_capella_signed_bootstrap_proof(f.genesis,f.rules,f.limits,d,[&]{++callbacks;checkpoint();});require(callbacks>0,"loader callback control");
 size_t observed=0;bool injected=false;refuses([&]{load_capella_signed_bootstrap_proof(f.genesis,f.rules,f.limits,d,[&]{if(++observed==callbacks){injected=true;auto corrupt=f.wire;corrupt[4]^=1;std::ofstream file(p/"signed-block.ssz",std::ios::binary);file.write(reinterpret_cast<const char*>(corrupt.data()),std::streamsize(corrupt.size()));file.close();require(bool(file),"owned artifact replacement after validation");}checkpoint();});},"post-validation artifact replacement accepted");require(injected,"post-validation fault boundary not reached");
 {std::ofstream file(p/"signed-block.ssz",std::ios::binary);file.write(reinterpret_cast<const char*>(f.wire.data()),std::streamsize(f.wire.size()));file.close();require(bool(file),"restore owned public wire fixture");}
 emit("post-validation-artifact-rebinding-refused");
}
void existing(const Fixture& f){
 Temp t;const auto absent=t.path/"absent/primary";refuses([&]{Blockchain c(absent,f.genesis,ChainStore::OpenMode::ReadWriteExisting);},"strict owner created absent DB");require(!fs::exists(t.path/"absent"),"strict owner created parent");
 for(const auto& key:{"meta/identity","meta/tip","block/0x0000000000000000","state/0x0000000000000000"}){Temp s;{Blockchain c(s.path/"primary",f.genesis);}erase(s.path/"primary",key);const auto before=rows(s.path/"primary");refuses([&]{Blockchain c(s.path/"primary",f.genesis,ChainStore::OpenMode::ReadWriteExisting);},"strict owner repaired missing binding");require(rows(s.path/"primary")==before,"strict owner initialized logical state on refusal");}
 {Temp s;{Blockchain c(s.path/"primary",f.genesis);}const auto before=rows(s.path/"primary");{Blockchain c(s.path/"primary",f.genesis,ChainStore::OpenMode::ReadWriteExisting);const auto tip=c.store().latest();require(tip&&tip->header.hash()==f.genesis.block.header.hash(),"strict owner canonical tip");}require(rows(s.path/"primary")==before,"strict valid owner changed logical state");}
 {Temp s;refuses([&]{Blockchain c(s.path/"primary",f.genesis,ChainStore::OpenMode::ReadOnly);},"readonly Blockchain owner accepted");require(!fs::exists(s.path/"primary"),"invalid owner mode opened DB");}
 emit("strict-existing-owner-never-initializes");
}
struct ClockForbidden{};
void before_clock(Blockchain& chain,Json cfg){bool reached=false,mutated=false;try{start_capella_runtime(chain,cfg,[&]{if(!mutated){mutated=true;cfg["recoveryWitness"]="/nonexistent/caller-replaced-witness";cfg["unexpectedCallerField"]=true;}checkpoint();},[&]()->uint64_t{reached=true;throw ClockForbidden{};});}catch(const ClockForbidden&){}require(reached&&mutated,"typed startup did not own config before real-clock boundary");}
void startup(const Fixture& f,const fs::path& input){
 Temp t;const auto primary=t.path/"primary",witness=t.path/"witness";auto proof=load_capella_signed_bootstrap_proof(f.genesis,f.rules,f.limits,descriptor(input,f),checkpoint);ForkCheckpointToken np{},nw{};
 {Blockchain c(primary,f.genesis);ChainStore w(witness);CapellaDurableReactor r(c,f.rules,f.limits,f.initial,checkpoint,&w);const auto wp=w.fork_checkpoint(f.genesis.chain_id,f.genesis.block.header.hash(),2048+8*f.limits.envelope_bytes);require(bool(wp),"witness pin");auto plan=r.prepare_bootstrap_archive(proof,r.current().token,wp->token,checkpoint);np=plan.next_primary();nw=plan.next_witness();r.apply_bootstrap_archive(plan,checkpoint);}
 auto cfg=f.config;cfg["recoveryWitness"]=witness.string();cfg["bootstrapArchiveRecovery"]={{"authority",descriptor(input,f)},{"nextPrimary",pin(np)},{"nextWitness",pin(nw)}};const auto a=rows(primary),b=rows(witness);
 {Blockchain c(primary,f.genesis,ChainStore::OpenMode::ReadWriteExisting);before_clock(c,cfg);}require(rows(primary)==a&&rows(witness)==b,"typed completed startup changed protocol state before clock");
 for(unsigned i=0;i<5;++i){auto bad=cfg;if(i==0)bad["automaticRecovery"]=true;else if(i==1)bad["recoveryCommitment"]=pin(np);else if(i==2)bad["bootstrapArchive"]=descriptor(input,f);else if(i==3)bad["bootstrapArchiveRecovery"]["nextWitness"]["digest"]=hex(Bytes(32));else bad["bootstrapArchiveRecovery"]["extra"]=true;bool clock=false;{Blockchain c(primary,f.genesis,ChainStore::OpenMode::ReadWriteExisting);refuses([&]{start_capella_runtime(c,bad,checkpoint,[&]()->uint64_t{clock=true;throw ClockForbidden{};});},"invalid startup mode accepted");}require(!clock&&rows(primary)==a&&rows(witness)==b,"invalid startup reached clock or changed protocol state");}
 emit("explicit-two-pin-runtime-startup-before-clock");
 {Temp fresh;auto fresh_cfg=f.config;fresh_cfg["initial"]=f.initial_descriptor;fresh_cfg["recoveryWitness"]=(fresh.path/"witness").string();fresh_cfg["bootstrapArchive"]=descriptor(input,f);
  {Blockchain c(fresh.path/"primary",f.genesis);before_clock(c,fresh_cfg);}
  ChainStore p(fresh.path/"primary",ChainStore::OpenMode::ReadOnly);const auto root=consensus_beacon_header_root(proof.header());const auto saved=p.checkpoint_archive(Bytes(root.begin(),root.end()),f.limits.envelope_bytes);require(saved&&saved->slot==1&&saved->payload==f.wire,"fresh runtime failed to publish authenticated archive");
 }
 emit("fresh-typed-bootstrap-archives-before-clock");
}
void main_existing_guard(){
 Temp t;const auto config=t.path/"invalid-recovery.json",primary=t.path/"absent-parent/primary";
 save(config,raw(Json{{"capella",{{"bootstrapArchiveRecovery",Json::object()}}}}.dump()));
 const auto genesis=(fs::path(NANO_TEST_SOURCE)/"tests/fixtures/capella-bootstrap-anchor-cb532/genesis.json").string();
 const auto binary=fs::canonical(NANO_TEST_NANO_BINARY).string(),db=primary.string(),cfg=config.string();
 struct Child{pid_t pid=-1;~Child(){if(pid>0){::kill(pid,SIGKILL);while(::waitpid(pid,nullptr,0)<0&&errno==EINTR){}}}} child;
 child.pid=::fork();require(child.pid>=0,"owned CLI child");
 if(child.pid==0){
  const int output=::open("/dev/null",O_WRONLY);if(output<0||::dup2(output,STDOUT_FILENO)<0||::dup2(output,STDERR_FILENO)<0)_exit(125);::close(output);
  char* const args[]={const_cast<char*>(binary.c_str()),const_cast<char*>("serve"),const_cast<char*>(genesis.c_str()),const_cast<char*>(db.c_str()),const_cast<char*>("0"),const_cast<char*>("127.0.0.1"),const_cast<char*>(cfg.c_str()),nullptr};
  // Missing network token also prevents a listener if the main guard regresses.
  char* const environment[]={const_cast<char*>("PATH=/usr/bin:/bin"),nullptr};::execve(binary.c_str(),args,environment);_exit(126);
 }
 int state=0;bool joined=false;const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
 while(!joined&&std::chrono::steady_clock::now()<deadline){checkpoint();const auto got=::waitpid(child.pid,&state,WNOHANG);if(got<0){require(errno==EINTR,"CLI child wait");continue;}joined=got==child.pid;if(!joined)::usleep(10000);}
 require(joined,"CLI child timeout");child.pid=-1;require(WIFEXITED(state)&&WEXITSTATUS(state)==1,"CLI refused existing-only startup");
 require(!fs::exists(t.path/"absent-parent"),"main created PRIMARY before typed recovery validation");emit("main-cli-typed-recovery-refuses-absent-primary");
}
}
int main(int argc,char** argv){try{require(argc==2&&std::string(argv[1])=="--self-test","usage --self-test");Temp input;const auto f=fixture(input.path);inputs(f,input.path);existing(f);startup(f,input.path);main_existing_guard();checkpoint();std::cout<<Json{{"scenario","bootstrap-startup-self-test"},{"result","PASS"},{"newSignaturesCreated",0},{"hostClockRead",false},{"clockRebased",false},{"workersStarted",false},{"networkUsed",false},{"privateInputsRead",false},{"productionQualified",false}}.dump()<<std::endl;return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
