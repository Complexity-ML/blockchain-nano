#include "nano/consensus_capella_bootstrap_input.hpp"
#include "nano/consensus_capella_reactor.hpp"
#include "nano/consensus_beacon_header.hpp"
#include "nano/checkpoint_blob_gc.hpp"
#include "rocksdb-owned-copy.hpp"
#include <rocksdb/db.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <poll.h>
#include <signal.h>
#include <string>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
using namespace nano;
namespace fs=std::filesystem;
namespace {
const auto began=std::chrono::steady_clock::now();
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
uint64_t elapsed(){return uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-began).count());}
uint64_t rss(){rusage r{};require(::getrusage(RUSAGE_SELF,&r)==0,"resource observation");
#ifdef __APPLE__
 return uint64_t(r.ru_maxrss)/1024;
#else
 return uint64_t(r.ru_maxrss);
#endif
}
void checkpoint(){require(elapsed()<120000&&rss()<=131072,"maintenance wrapper resource bound");}
struct Temp{fs::path path;Temp(){auto s=(fs::temp_directory_path()/"nano-bootstrap-maintenance-XXXXXX").string();require(::mkdtemp(s.data()),"new owned fixture");path=fs::canonical(s);}~Temp(){std::error_code e;fs::remove_all(path,e);}};
void directory(const fs::path& p){require(!fs::exists(p),"new directory only");require(::mkdir(p.c_str(),0700)==0,"private directory create");}
std::string read(const fs::path& p,size_t max=32*1024*1024){std::ifstream in(p,std::ios::binary);require(bool(in),"public or owned input");std::string s((std::istreambuf_iterator<char>(in)),{});require(s.size()<=max,"input bound");return s;}
Bytes raw(std::string_view s){return {s.begin(),s.end()};}
std::string sha(std::string_view s){return hex(sha256(raw(s))).substr(2);}
std::string sha_file(const fs::path& p){return sha(read(p));}
void save(const fs::path& p,std::string_view s){require(!fs::exists(p),"new file only");const int fd=::open(p.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);require(fd>=0,"owned file create");size_t at=0;while(at<s.size()){const auto n=::write(fd,s.data()+at,s.size()-at);if(n<0&&errno==EINTR)continue;require(n>0,"owned file write");at+=size_t(n);}require(::fsync(fd)==0&&::close(fd)==0,"owned file durable");}
void save(const fs::path& p,const std::string& s){save(p,std::string_view(s));}
void save(const fs::path& p,const char* s){save(p,std::string_view(s));}
void save(const fs::path& p,const Json& j){save(p,j.dump());}
Json token_json(const ForkCheckpointToken& t){return {{"sequence",quantity(Int(std::to_string(t.sequence)))},{"digest",hex(t.digest)}};}
ForkCheckpointToken token(const Json& j){return {u64(quantity(j.at("sequence"))),unhex(j.at("digest").get<std::string>())};}
using Rows=std::map<std::string,std::string>;
Rows rows(const fs::path& p){rocksdb::Options o;o.create_if_missing=false;o.paranoid_checks=true;auto d=maintenance::open_read_only_copy(o,p.string());rocksdb::ReadOptions r;r.verify_checksums=true;r.fill_cache=false;std::unique_ptr<rocksdb::Iterator> it(d->NewIterator(r));Rows v;size_t bytes=0;for(it->SeekToFirst();it->Valid();it->Next()){checkpoint();bytes+=it->key().size()+it->value().size();require(v.size()<4096&&bytes<=64*1024*1024,"logical rows bound");v.emplace(it->key().ToString(),it->value().ToString());}require(it->status().ok(),"complete logical rows");return v;}
void erase_owned(const fs::path& p,const std::string& key){rocksdb::Options o;o.create_if_missing=false;auto d=maintenance::open_copy(o,p.string());rocksdb::WriteOptions w;w.sync=true;require(d->Delete(w,key).ok(),"owned adverse delete");}
std::string inventory(const fs::path& p){return checkpoint_blob_gc_copy_inventory(p,4096,64*1024*1024,checkpoint);}
void clone(const fs::path& from,const fs::path& to){directory(to.parent_path());fs::copy(from,to,fs::copy_options::recursive);require(::chmod(to.c_str(),0700)==0,"private cloned root");require(inventory(from)==inventory(to),"closed independent physical clone");}
void emit(const char* name,Json fields=Json::object()){fields["scenario"]=name;fields["result"]="PASS";fields["newSignaturesCreated"]=0;fields["privateInputsRead"]=false;fields["productionQualified"]=false;std::cout<<fields.dump()<<std::endl;}
struct Fixture{Genesis g;CapellaForkBlockRules rules;CapellaStoreSnapshotLimits limits;CapellaForkStore initial;CapellaSignedBootstrapArchive proof;Bytes wire;Json genesis,profile,authority;};
Fixture fixture(const fs::path& p){
 const auto source=fs::path(NANO_TEST_SOURCE)/"tests/fixtures/capella-bootstrap-anchor-cb532";const auto text=read(source/"manifest.json");require(sha(text)=="ec150c28c8c187e6728cea7ef717acc6383e0b75242302b0a481fc2a9c425fc6","literal public manifest");const auto m=Json::parse(text);
 auto file=[&](const char* role){const auto& pin=m.at("files").at(role);const auto s=read(source/pin.at("path").get<std::string>());require(pin.at("bytes").get<size_t>()==s.size()&&pin.at("sha256").get<std::string>()==sha(s),"public fixture bytes");return s;};
 const auto gs=file("genesis"),ps=file("profile"),ss=file("genesisState"),initial=file("initial");auto g=Genesis::from_json(Json::parse(gs));auto rules=load_capella_fork_profile(ps,sha256(raw(ps)),1024*1024);const auto max=rules.transition.maximum_state_bytes;CapellaStoreSnapshotLimits limits{16*1024*1024,4,64,64,max,4*max,64,100000};auto store=capella_store_from_snapshot(initial,sha256(raw(initial)),rules,limits,checkpoint);auto wire=unhex(m.at("bootstrapSigningHistory").at("block").at("ssz").get<std::string>());require(hex(sha256(wire))=="0x9575c3a186b101bd4a35bf56072bc9eba91d1d912e4ec462714eae0a60b08995","literal already signed public wire");
 auto proof=prepare_capella_signed_bootstrap_archive(g,CapellaSlotState{raw(ss),std::nullopt},wire,rules,64,100000,checkpoint);require(bool(proof),"full signed bootstrap authority");
 save(p/"genesis.json",gs);save(p/"profile.json",ps);save(p/"genesis-state.ssz",ss);save(p/"wire.ssz",std::string_view(reinterpret_cast<const char*>(wire.data()),wire.size()));
 auto artifact=[&](const char* name){const auto bytes=read(p/name);return Json{{"path",(p/name).string()},{"sha256",sha(bytes)},{"bytes",bytes.size()}};};
 Json authority{{"format","nano-capella-signed-bootstrap-proof-inputs-v1"},{"genesisState",artifact("genesis-state.ssz")},{"signedBlock",artifact("wire.ssz")}};
 return {std::move(g),std::move(rules),limits,std::move(store),std::move(*proof),std::move(wire),artifact("genesis.json"),artifact("profile.json"),std::move(authority)};
}
struct Sources{fs::path p,w;std::string pi,wi;ForkCheckpointToken pt,wt;};
Sources sources(const Fixture& f,const fs::path& root,bool archived=false,unsigned format=0){
 directory(root);directory(root/"primary");directory(root/"witness");Sources s{root/"primary/db",root/"witness/db",{}, {},{}, {}};
 {Blockchain owner(s.p,f.g);ChainStore w(s.w);if(archived){CapellaDurableReactor r(owner,f.rules,f.limits,f.initial,f.proof,w,checkpoint);s.pt=r.current().token;}else{CapellaDurableReactor r(owner,f.rules,f.limits,f.initial,checkpoint,&w);if(format==1)require(r.migrate_reference_checkpoint({2,128*1024},checkpoint),"real cold source migration");else if(format==2)require(r.migrate_fragment_checkpoint(checkpoint),"real fragment source migration");s.pt=r.current().token;}const auto pin=w.fork_checkpoint(f.g.chain_id,f.g.block.header.hash(),2048+8*f.limits.envelope_bytes);require(bool(pin),"source witness current");s.wt=pin->token;}
 require(::chmod(s.p.c_str(),0700)==0&&::chmod(s.w.c_str(),0700)==0,"private source roots");s.pi=inventory(s.p);s.wi=inventory(s.w);return s;
}
void unchanged_sources(const Sources& s){require(inventory(s.p)==s.pi&&inventory(s.w)==s.wi,"original source changed");}
struct Child{pid_t pid=-1;~Child(){if(pid>0){::kill(pid,SIGKILL);while(::waitpid(pid,nullptr,0)<0&&errno==EINTR){}}}};
struct Run{int code;Json status;std::string err;};
Run execute(const std::vector<std::string>& arguments,const fs::path& logs,const std::vector<std::string>& additions={}){
 checkpoint();directory(logs);const auto exe=fs::canonical(NANO_TEST_TOOL).string();std::vector<std::string> args{exe};args.insert(args.end(),arguments.begin(),arguments.end());std::vector<char*> argv;for(auto& s:args)argv.push_back(s.data());argv.push_back(nullptr);std::vector<std::string> env{"PATH=/usr/bin:/bin"};env.insert(env.end(),additions.begin(),additions.end());std::vector<char*> ep;for(auto& s:env)ep.push_back(s.data());ep.push_back(nullptr);const auto stdout_path=(logs/"out").string(),stderr_path=(logs/"err").string();const int out=::open(stdout_path.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC,0600),stderr_fd=::open(stderr_path.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC,0600);require(out>=0&&stderr_fd>=0,"new child output files");Child c;c.pid=::fork();require(c.pid>=0,"tool child fork");
 if(c.pid==0){if(::dup2(out,STDOUT_FILENO)<0||::dup2(stderr_fd,STDERR_FILENO)<0)_exit(125);::close(out);::close(stderr_fd);::execve(exe.c_str(),argv.data(),ep.data());_exit(126);}::close(out);::close(stderr_fd);
 int status=0;bool joined=false;const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(20);while(!joined&&std::chrono::steady_clock::now()<end){checkpoint();auto n=::waitpid(c.pid,&status,WNOHANG);if(n<0){require(errno==EINTR,"tool wait");continue;}joined=n==c.pid;if(!joined){for(const auto* name:{"out","err"})if(fs::exists(logs/name))require(fs::file_size(logs/name)<=65536,"tool output bound");::usleep(10000);}}
 if(joined)c.pid=-1;require(joined&&WIFEXITED(status),"tool timeout or signal");const auto output=read(logs/"out",65536),err=read(logs/"err",65536);auto j=Json::parse(output);require(j.is_object()&&j.at("completed").is_boolean()&&j.at("runtimeStarted")==false&&j.at("productionQualified")==false&&!j.contains("primary")&&!j.contains("witness")&&!j.contains("copies")&&!j.contains("digest"),"public output leaked private receipts");return {WEXITSTATUS(status),std::move(j),err};
}
struct Case{
 const Fixture& f;const Sources& s;fs::path root,p,w,request;Json j;unsigned run_id=0;
 Case(const Fixture& fixture,const Sources& source,const fs::path& path):f(fixture),s(source),root(path),p(path/"primary/db"),w(path/"witness/db"){
  directory(root);clone(s.p,p);clone(s.w,w);save(p.parent_path()/".nano-owned-bootstrap-archive-copy","owned-offline-bootstrap-archive-v1");save(w.parent_path()/".nano-owned-bootstrap-archive-copy","owned-offline-bootstrap-archive-v1");
  auto value=[&](const char* role,const fs::path& copy,const fs::path& original,const std::string& hash,const ForkCheckpointToken& pin){return Json{{"path",copy.string()},{"sourcePath",original.string()},{"inventorySha256",hash},{"sourceInventorySha256",hash},{"token",token_json(pin)}};};
  auto q=[](uint64_t n){return quantity(Int(std::to_string(n)));};
  j={{"format","nano-owned-bootstrap-archive-maintenance-v1"},{"genesis",f.genesis},{"profile",f.profile},{"authority",f.authority},{"primary",value("primary",p,s.p,s.pi,s.pt)},{"witness",value("witness",w,s.w,s.wi,s.wt)},{"limits",Json::array({q(f.limits.envelope_bytes),q(f.limits.blocks),q(f.limits.messages),q(f.limits.equivocations),q(f.limits.state_bytes),q(f.limits.total_state_bytes),q(f.limits.replay_slots),q(f.limits.sync_trials)})},{"budget",{{"inventoryFiles","0x1000"},{"inventoryBytes","0x4000000"},{"logicalRecords","0x1000"},{"logicalBytes","0x4000000"},{"durationMs","0x4e20"}}}};
  reseal(true);
 }
 void reseal(bool stopped){const auto name="stop-"+std::to_string(run_id)+".json";Json copies=Json::array();for(const auto* role:{"primary","witness"}){const auto& v=j.at(role);copies.push_back(Json{{"role",role},{"path",v.at("path")},{"inventorySha256",v.at("inventorySha256")},{"sourcePath",v.at("sourcePath")},{"sourceInventorySha256",v.at("sourceInventorySha256")}});}Json stop{{"format","nano-stopped-checkpoint-copy-receipt-v1"},{"scope","isolated-owned-copies"},{"allWritersStopped",stopped},{"allReadersStopped",true},{"stoppedAtMs","0x1"},{"copies",copies}};save(root/name,stop);j["stopReceipt"]={{"path",(root/name).string()},{"sha256",sha(stop.dump())},{"bytes",stop.dump().size()}};request=root/("request-"+std::to_string(run_id)+".json");save(request,j);++run_id;}
 Run call(const char* op,const fs::path& out,const fs::path& next={},const std::vector<std::string>& env={},std::vector<std::string> override_args={}){
  std::vector<std::string> a{op,request.string(),sha_file(request),out.string()};if(override_args.empty()&&std::string(op)!="plan"){a.push_back((next/"primary-next.json").string());a.push_back(sha_file(next/"primary-next.json"));a.push_back((next/"witness-next.json").string());a.push_back(sha_file(next/"witness-next.json"));}if(!override_args.empty())a=std::move(override_args);return execute(a,root/("log-"+std::to_string(run_id++)),env);
 }
};
void refused(Case& c,const Run& r,const Rows& p,const Rows& w,const fs::path& out){require(r.code==1&&r.status.at("completed")==false&&!fs::exists(out/"complete.json"),"tool did not refuse without complete receipt");require(rows(c.p)==p&&rows(c.w)==w,"refusal changed protocol rows");unchanged_sources(c.s);}
void private_file(const fs::path& p){struct stat s{};require(::lstat(p.c_str(),&s)==0&&S_ISREG(s.st_mode)&&s.st_uid==::geteuid()&&s.st_nlink==1&&(s.st_mode&0777)==0600,"private distinct receipt metadata");}
struct Next{ForkCheckpointToken p,w;bool work;};
Next planned(Case& c,const fs::path& out){const auto before=rows(c.p),wit=rows(c.w);const auto r=c.call("plan",out);require(r.code==0&&r.status.at("completed")==true,"real CLI plan failed");require(rows(c.p)==before&&rows(c.w)==wit,"plan published supplement");private_file(out/"primary-next.json");private_file(out/"witness-next.json");private_file(out/"complete.json");const auto p=Json::parse(read(out/"primary-next.json")),w=Json::parse(read(out/"witness-next.json"));require(p.at("role")=="primary"&&w.at("role")=="witness"&&p.at("requestSha256")==sha_file(c.request)&&p.at("currentPrimary")==token_json(c.s.pt)&&p.at("currentWitness")==token_json(c.s.wt),"retained request/current provenance");auto a=p,b=w;a.erase("role");a.erase("next");b.erase("role");b.erase("next");require(a==b&&p.at("copies")[0].at("path")==c.p.string()&&p.at("copies")[1].at("path")==c.w.string(),"independent files differ in authority");const auto np=token(p.at("next")),nw=token(w.at("next"));const bool work=p.at("hasWork").get<bool>();if(work)require(np.sequence==c.s.pt.sequence+1&&np.digest==c.s.pt.digest&&nw.sequence==c.s.wt.sequence+1,"predicted two-token relation");else require(np==c.s.pt&&nw==c.s.wt,"noWork predictions advanced");unchanged_sources(c.s);return {np,nw,work};}
Sources adverse_sources(const Sources& from,const fs::path& root,const fs::path& member,const char* key){directory(root);Sources s{root/"primary/db",root/"witness/db",{}, {},from.pt,from.wt};clone(from.p,s.p);clone(from.w,s.w);const auto target=member=="primary"?s.p:s.w;require(rows(target).contains(key),"adverse source key absent before explicit deletion");erase_owned(target,key);s.pi=inventory(s.p);s.wi=inventory(s.w);return s;}
void existing_refusals(const Fixture& f,const Sources& s,const fs::path& root){
 const auto original=rows(s.p);const auto canonical_state=std::string("state/")+original.at("meta/tip");require(canonical_state!="state/0x0000000000000000"&&original.contains(canonical_state)&&original.contains("state/0x0000000000000000"),"fixture canonical tip and historical state discrimination");
 unsigned i=0;for(const auto& key:std::array<std::string,4>{"meta/identity","meta/tip","block/0x0000000000000000",canonical_state}){auto bad=adverse_sources(s,root/("owner-source-"+std::to_string(i)),"primary",key.c_str());Case c(f,bad,root/("owner-case-"+std::to_string(i++)));const auto p=rows(c.p),w=rows(c.w);const auto out=c.root/"out";refused(c,c.call("plan",out),p,w,out);require(!rows(c.p).contains(key),"existing owner recreated missing binding");}
 {auto pruned=adverse_sources(s,root/"historical-state-source","primary","state/0x0000000000000000");Case c(f,pruned,root/"historical-state-case");const auto p=rows(c.p),w=rows(c.w);require(!p.contains("state/0x0000000000000000")&&p.contains(canonical_state),"historical-only state removal");const auto next=planned(c,c.root/"out");require(next.work&&rows(c.p)==p&&rows(c.w)==w&&!rows(c.p).contains("state/0x0000000000000000"),"valid current-state plan should accept historical-state absence without initialization");unchanged_sources(s);}
 {auto bad=adverse_sources(s,root/"witness-source","witness","meta/identity");Case c(f,bad,root/"witness-case");const auto p=rows(c.p),w=rows(c.w);const auto out=c.root/"out";refused(c,c.call("plan",out),p,w,out);require(!rows(c.w).contains("meta/identity"),"witness recreated identity");}
 {Case c(f,s,root/"missing-path");fs::remove_all(c.p);const auto out=c.root/"out";auto r=c.call("plan",out);require(r.code==1&&!fs::exists(c.p)&&!fs::exists(out/"complete.json"),"missing DB recreated");unchanged_sources(s);}
 {Case c(f,s,root/"missing-current");fs::remove(c.p/"CURRENT");const auto before=inventory(c.p);const auto out=c.root/"out";auto r=c.call("plan",out);require(r.code==1&&inventory(c.p)==before&&!fs::exists(c.p/"CURRENT")&&!fs::exists(out/"complete.json"),"CURRENT absence initialized copy");unchanged_sources(s);}
 emit("maintenance-existing-owner-refuses-initialization",{{"missingPrimaryBindings",4},{"missingWitnessIdentity",true},{"missingDatabaseOrCurrentNeverCreated",true},{"canonicalStateKeyDerivedFromTip",true},{"missingCanonicalStateRefused",true},{"historicalStateAbsentCurrentStatePresentAccepted",true}});
}
int hold_lock(const fs::path& p,int ready){rocksdb::Options o;o.create_if_missing=false;auto db=maintenance::open_copy(o,p.string());require(::write(ready,"L",1)==1,"lock ready");::close(ready);for(;;)::pause();}
void locked_and_stopped(const Fixture& f,const Sources& s,const fs::path& root,const fs::path& own_exe){
 {Case c(f,s,root/"false-stop");c.reseal(false);const auto p=rows(c.p),w=rows(c.w);const auto out=c.root/"out";refused(c,c.call("plan",out),p,w,out);}
 {Case c(f,s,root/"actual-lock");const auto plan=c.root/"plan";planned(c,plan);require(c.call("apply",c.root/"applied",plan).code==0,"lock fixture completed supplement");const auto p=rows(c.p),w=rows(c.w);int signal[2];require(::pipe(signal)==0,"lock readiness pipe");const auto exe=fs::canonical(own_exe).string(),db=c.p.string(),ready_fd=std::to_string(signal[1]);char* const a[]={const_cast<char*>(exe.c_str()),const_cast<char*>("--hold-lock"),const_cast<char*>(db.c_str()),const_cast<char*>(ready_fd.c_str()),nullptr};char* const e[]={const_cast<char*>("PATH=/usr/bin:/bin"),nullptr};Child child;child.pid=::fork();require(child.pid>=0,"lock fresh child");if(child.pid==0){::close(signal[0]);::execve(exe.c_str(),a,e);_exit(126);}::close(signal[1]);pollfd fd{signal[0],POLLIN,0};require(::poll(&fd,1,3000)>0&&(fd.revents&POLLIN),"actual LOCK acquisition not observed");char v=0;require(::read(signal[0],&v,1)==1&&v=='L',"LOCK readiness value");::close(signal[0]);
  // Completed typed recover deliberately has no pre-interruption physical
  // fingerprint gate; this checks the real exclusive owner open under LOCK.
  const auto out=c.root/"out";auto r=c.call("recover",out,plan);require(r.code==1&&r.status.at("refusedStage")=="reconstructed-process-local-plan"&&!fs::exists(out/"complete.json"),"tool did not reach/refuse actual competing LOCK");require(::kill(child.pid,SIGTERM)==0,"stop owned lock holder");int status=0;while(::waitpid(child.pid,&status,0)<0)require(errno==EINTR,"lock holder wait");child.pid=-1;require(WIFSIGNALED(status)&&WTERMSIG(status)==SIGTERM,"lock holder not terminal");require(rows(c.p)==p&&rows(c.w)==w,"lock refusal changed protocol state");unchanged_sources(s);
 }
 emit("maintenance-authenticated-stop-and-actual-exclusive-lock",{{"falseStopReceiptRefused",true},{"freshExecLockAcquisitionObserved",true},{"sourcesNeverOpenedRw",true}});
}
void provenance(const Fixture& f,const Sources& s,const fs::path& root){
 for(unsigned mode=0;mode<8;++mode){Case c(f,s,root/("provenance-"+std::to_string(mode)));const auto p=rows(c.p),w=rows(c.w);if(mode==0)c.j["primary"]["sourceInventorySha256"]=std::string(64,'0');else if(mode==1)c.j["primary"]["path"]=s.p.string();else if(mode==2){fs::create_directory_symlink(c.p,c.root/"alias");c.j["primary"]["path"]=(c.root/"alias").string();}else if(mode==3)fs::create_hard_link(c.p/"CURRENT",c.root/"shared-current");else if(mode==4)c.j["budget"]["logicalRecords"]="0x1";else if(mode==5){auto bad=Json::parse(read(f.genesis.at("path").get<std::string>()));bad["config"]["chainId"]=1338;save(c.root/"foreign-genesis.json",bad);const auto text=bad.dump();c.j["genesis"]={{"path",(c.root/"foreign-genesis.json").string()},{"sha256",sha(text)},{"bytes",text.size()}};}else if(mode==6)c.j["primary"]["sourcePath"]=c.root.string();else c.j["primary"]["sourcePath"]=(c.root/"absent-source").string();c.reseal(true);const auto out=c.root/"out";refused(c,c.call("plan",out),p,w,out);}
 emit("maintenance-provenance-alias-hardlink-and-budget",{{"cases",8},{"sourceHashAliasSymlinkHardlinkScanBudgetForeignGenesisOverlapAndAbsenceRefused",true},{"allProtocolRowsAndOriginalSourcesUnchanged",true}});
}
void retain_refusals(const Fixture& f,const Sources& s,const fs::path& root){
 for(unsigned mode=0;mode<6;++mode){Case c(f,s,root/("retention-"+std::to_string(mode)));const auto plan=c.root/"plan";const auto next=planned(c,plan);require(next.work,"legacy fixture did not need archive");const auto p=rows(c.p),w=rows(c.w);const auto out=c.root/"out";auto pp=plan/"primary-next.json",wp=plan/"witness-next.json";std::vector<std::string> a{"apply",c.request.string(),sha_file(c.request),out.string(),pp.string(),sha_file(pp),wp.string(),sha_file(wp)};
  if(mode==0)a[5]=std::string(64,'0');else if(mode==1){std::swap(a[4],a[6]);std::swap(a[5],a[7]);}else if(mode==2){a[4]=(c.root/"absent-next").string();}else if(mode==3){auto j=Json::parse(read(pp));j["next"]["sequence"]=quantity(Int(std::to_string(next.p.sequence+1)));save(c.root/"wrong-next",j);a[4]=(c.root/"wrong-next").string();a[5]=sha_file(c.root/"wrong-next");}else if(mode==4){require(::chmod(pp.c_str(),0644)==0,"owned external NEXT mode fault");}else{directory(out);save(out/"third-party-sentinel","unchanged");}
  auto r=c.call("apply",out,{}, {},a);refused(c,r,p,w,out);if(mode==5)require(read(out/"third-party-sentinel")=="unchanged","existing output changed");
 }
 {Case c(f,s,root/"inside-output");const auto p=rows(c.p),w=rows(c.w);const auto out=s.p/"out";auto r=c.call("plan",out);require(r.code==1&&!fs::exists(out),"output created inside original source");require(rows(c.p)==p&&rows(c.w)==w,"inside-output refusal wrote protocol");unchanged_sources(s);}
 emit("maintenance-retained-pins-and-protected-output-refusals",{{"cases",7},{"missingSwappedWrongHashWrongNextNonPrivateAndExistingOutputRefused",true},{"noSupplementOnRefusal",true}});
}
void verified_result(Case& c,const Rows& previous,const Rows& previous_w,const Next& n){
 const auto after=rows(c.p),after_w=rows(c.w);const auto anchor=consensus_beacon_header_root(c.f.proof.header());const auto root=hex(Bytes(anchor.begin(),anchor.end()));const auto archive="checkpoint-archive/v1/"+root,index="checkpoint-archive-slot/v1/0x0000000000000001/"+root;
 auto x=previous,y=after;for(const auto* k:{"fork-checkpoint/v1/token"}){x.erase(k);y.erase(k);}x.erase(archive);x.erase(index);y.erase(archive);y.erase(index);require(x==y&&after.at("fork-checkpoint/v1/payload")==previous.at("fork-checkpoint/v1/payload")&&after.contains(archive)&&after.at(index).empty(),"apply modified raw envelope/unrelated namespace or lost index");auto wx=previous_w,wy=after_w;for(const auto* k:{"fork-checkpoint/v1/token","fork-checkpoint/v1/payload"}){wx.erase(k);wy.erase(k);}require(wx==wy,"apply changed unrelated witness namespace");
 {Blockchain owner(c.p,c.f.g,ChainStore::OpenMode::ReadWriteExisting);ChainStore w(c.w,ChainStore::OpenMode::ReadWriteExisting);w.require_existing_chain_binding(c.f.g.chain_id,c.f.g.block.header.hash(),"nano-capella-recovery-v1");CapellaDurableReactor r(owner,c.f.rules,c.f.limits,w,c.f.proof,n.p,n.w,checkpoint);require(r.current().token==n.p&&r.signed_block(r.current().state->timed.view.anchor)==c.f.wire,"independent cold typed readback");const auto wp=w.fork_checkpoint(c.f.g.chain_id,c.f.g.block.header.hash(),2048+8*c.f.limits.envelope_bytes,n.w);require(bool(wp)&&!r.prepare_bootstrap_archive(c.f.proof,n.p,n.w,checkpoint).has_work(),"cold idempotent plan");}
 unchanged_sources(c.s);
}
void apply_and_recover(const Fixture& f,const Sources& s,const fs::path& root,const char* format){
 Case c(f,s,root/"apply");const auto plan=c.root/"plan";auto n=planned(c,plan);require(n.work,"positive requires genuine archive absence");const auto p=rows(c.p),w=rows(c.w);auto out=c.root/"applied";auto r=c.call("apply",out,plan);require(r.code==0&&r.status.at("completed")==true,"real CLI apply failed");for(const auto* name:{"primary-next.json","witness-next.json","complete.json"})private_file(out/name);require(read(out/"primary-next.json")==read(plan/"primary-next.json")&&read(out/"witness-next.json")==read(plan/"witness-next.json"),"apply did not durably re-retain exact two predictions");verified_result(c,p,w,n);
 // Ordinary restart must not authorize the distinct archive-only intent.
 const auto done=rows(c.p),wit=rows(c.w);bool rejected=false;{Blockchain owner(c.p,f.g,ChainStore::OpenMode::ReadWriteExisting);ChainStore ww(c.w,ChainStore::OpenMode::ReadWriteExisting);try{CapellaDurableReactor generic(owner,f.rules,f.limits,ww,checkpoint);}catch(const std::invalid_argument&){rejected=true;}}require(rejected&&rows(c.p)==done&&rows(c.w)==wit,"generic restart bypassed typed intent");
 for(unsigned i=0;i<2;++i){const auto before=rows(c.p),before_w=rows(c.w);const auto recovered=c.root/("recover-"+std::to_string(i));r=c.call("recover",recovered,plan);require(r.code==0&&r.status.at("completed")==true&&rows(c.p)==before&&rows(c.w)==before_w,"completed typed recovery advanced token/changed state");verified_result(c,p,w,n);}
 emit("maintenance-real-apply-cold-readback-and-completed-recovery",{{"actualDurableFormat",format},{"archiveAndIndexRetained",true},{"rawSnapshotAndAllUnrelatedNamespacesExact",true},{"bothNextReretainedBeforeIo",true},{"coldTypedReopen",true},{"ordinaryRecoveryRefused",true},{"completedRecoveryRepetitions",2},{"interruptedRecoveryQualified",false}});
 // Stale original plans cannot be re-applied after the source tokens advanced.
 out=c.root/"stale-apply";const auto stale_p=rows(c.p),stale_w=rows(c.w);const auto refused_apply=c.call("apply",out,plan);refused(c,refused_apply,stale_p,stale_w,out);
 emit("maintenance-stale-original-apply-refused",{{"actualDurableFormat",format}});
}
void no_work(const Fixture& f,const Sources& archived,const fs::path& root){Case c(f,archived,root/"noop");const auto before=rows(c.p),wit=rows(c.w);const auto plan=c.root/"plan";const auto n=planned(c,plan);require(!n.work,"fresh typed source should already have archive");const auto out=c.root/"apply";auto r=c.call("apply",out,plan);require(r.code==0&&r.status.at("hasWork")==false&&rows(c.p)==before&&rows(c.w)==wit,"noWork archive initial source changed");unchanged_sources(archived);emit("maintenance-existing-archive-no-work-apply",{{"twoTokensUnchanged",true},{"ordinaryCompleteInitialIntentSupported",true}});}
void retention_fsync(const Fixture& f,const Sources& s,const fs::path& root){
#ifdef NANO_TEST_FSYNC_SHIM
 const auto shim=fs::canonical(NANO_TEST_FSYNC_SHIM).string();for(unsigned i=0;i<3;++i){Case c(f,s,root/("fsync-"+std::to_string(i)));const auto plan=c.root/"plan";planned(c,plan);const auto p=rows(c.p),w=rows(c.w);const auto out=c.root/"apply";const auto target=i==0?out/"primary-next.json":i==1?out/"witness-next.json":out;
  std::vector<std::string> env{"LD_PRELOAD="+shim,"NANO_TEST_RETENTION_FSYNC_TARGET="+target.string()};if(i==2)env.push_back("NANO_TEST_RETENTION_ARM_AFTER="+(out/"witness-next.json").string());const auto r=c.call("apply",out,plan,env);require(r.err.find("nano-test-retention-fsync-failed\n")!=std::string::npos,"genuine retention fsync fault not reached");refused(c,r,p,w,out);require(fs::exists(out/"primary-next.json"),"fault did not reach real primary retention file");if(i>0)require(fs::exists(out/"witness-next.json"),"fault did not reach real witness retention file");
 }
 emit("maintenance-retention-file-and-directory-fsync-refusals",{{"actualFsyncFaultCases",3},{"twoFilesAndPostWitnessDirectoryBoundaries",true},{"allBeforeProtocolPublication",true}});
#else
 emit("maintenance-retention-fsync-not-covered-on-this-platform",{{"actualFsyncFaultCases",0},{"qualificationOpen",true}});
#endif
}
}
int main(int argc,char** argv){try{if(argc==4&&std::string(argv[1])=="--hold-lock")return hold_lock(fs::path(argv[2]),std::stoi(argv[3]));require(argc==2&&std::string(argv[1])=="--self-test","usage --self-test");Temp temp;directory(temp.path/"inputs");const auto f=fixture(temp.path/"inputs");const auto s=sources(f,temp.path/"sources");existing_refusals(f,s,temp.path);locked_and_stopped(f,s,temp.path,fs::path(argv[0]));provenance(f,s,temp.path);retain_refusals(f,s,temp.path);retention_fsync(f,s,temp.path);apply_and_recover(f,s,temp.path,"hot");for(unsigned format=1;format<=2;++format){const auto name=format==1?"cold":"fragmented";const auto source=sources(f,temp.path/(std::string(name)+"-sources"),false,format);directory(temp.path/name);apply_and_recover(f,source,temp.path/name,name);}const auto archived=sources(f,temp.path/"archived-sources",true);no_work(f,archived,temp.path);checkpoint();emit("bootstrap-archive-maintenance-wrapper-self-test",{{"elapsedMs",elapsed()},{"maxRssKiB",rss()},{"actualCliUsed",true},{"alreadySignedPublicFixtureOnly",true},{"newSignaturesCreated",0},{"wholeNodeBackupQualified",false},{"interruptedWalRecoveryQualified",false},{"physicalPowerLossQualified",false},{"clockReadOrReset",false},{"dutiesResumed",false},{"networkUsed",false}
#ifdef NANO_TEST_FSYNC_SHIM
 ,{"retentionFsyncFaultCases",3}
#else
 ,{"retentionFsyncFaultCases",0}
#endif
 });return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
