#include "nano/chain.hpp"
#include "nano/checkpoint_blob_gc.hpp"
#include "nano/consensus_capella_bootstrap_archive.hpp"
#include "nano/consensus_capella_reactor.hpp"
#include "nano/storage_memory_policy.hpp"
#include <rocksdb/db.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cerrno>
#include <cstdlib>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <functional>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdint>
#include <utility>
#include <iterator>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
using namespace nano;namespace fs=std::filesystem;
#ifndef NANO_TEST_SOURCE
#error "NANO_TEST_SOURCE must bind the actual public source view"
#endif
#ifndef NANO_TEST_TOOL
#error "NANO_TEST_TOOL must bind the actual qualified wrapper ELF"
#endif
#ifndef NANO_TEST_IO_SHIM
#error "NANO_TEST_IO_SHIM must bind the test-only I/O shim"
#endif
namespace {
using Clock=std::chrono::steady_clock;
const auto began=Clock::now();size_t calls=0,refusals=0,positives=0,events=0;std::string current_case;
constexpr std::array<std::string_view,17> request_steps{"arguments","directories","request-read","parse","budget","scratch","artifactdescriptors","artifactreads","genesis-profile","limits","chain-profile","projection","core-selections","roles","source-owners","source-inventories","request-complete"};Json last_tool=Json::object();std::optional<int> last_tool_exit;
void need(bool ok){if(!ok)throw std::runtime_error("Public journal I/O check refused");}
uint64_t peak(){rusage r{};need(::getrusage(RUSAGE_SELF,&r)==0);
#ifdef __APPLE__
 return uint64_t(r.ru_maxrss)/1024;
#else
 return uint64_t(r.ru_maxrss);
#endif
}
void check(){need(Clock::now()-began<std::chrono::seconds(140)&&peak()<=131072);}
Bytes bytes(std::string_view s){return {s.begin(),s.end()};}
std::string digest(std::string_view s){return hex(sha256(bytes(s))).substr(2);}
std::string read(const fs::path& p,size_t maximum=16*1024*1024){need(fs::is_regular_file(p)&&fs::file_size(p)<=maximum);std::ifstream f(p,std::ios::binary);need(bool(f));std::string b((std::istreambuf_iterator<char>(f)),{});need(!f.bad()&&b.size()<=maximum);return b;}
void write(const fs::path& p,std::string_view b){int fd=::open(p.c_str(),O_WRONLY|O_CREAT|O_TRUNC|O_NOFOLLOW|O_CLOEXEC,0600);need(fd>=0);size_t at=0;while(at<b.size()){auto n=::write(fd,b.data()+at,b.size()-at);if(n<0&&errno==EINTR)continue;need(n>0);at+=size_t(n);}need(::fsync(fd)==0&&::close(fd)==0);}
std::string q(uint64_t n){return quantity(Int(std::to_string(n)));}
Json token(const ForkCheckpointToken& t){return {{"sequence",q(t.sequence)},{"digest",hex(t.digest)}};}
ForkCheckpointToken pin(const Json& j){return {u64(quantity(j.at("sequence"))),unhex(j.at("digest").get<std::string>())};}
struct Temp{fs::path p;Temp(){char s[]="/tmp/nano-journal-io-XXXXXX";auto n=::mkdtemp(s);need(n);p=n;need(::chmod(p.c_str(),0700)==0);}~Temp(){std::error_code e;fs::remove_all(p,e);}};
using Files=std::map<std::string,std::string>;
Files hashes(const fs::path& p){Files r;auto record=[&](const fs::path& path,const std::string& rel){check();struct stat st{};need(::lstat(path.c_str(),&st)==0);Json z{{"device",uint64_t(st.st_dev)},{"inode",uint64_t(st.st_ino)},{"uid",uint64_t(st.st_uid)},{"gid",uint64_t(st.st_gid)},{"mode",uint64_t(st.st_mode)},{"links",uint64_t(st.st_nlink)},{"bytes",uint64_t(st.st_size)}};
#ifdef __APPLE__
 z["mtime"]=Json::array({st.st_mtimespec.tv_sec,st.st_mtimespec.tv_nsec});z["ctime"]=Json::array({st.st_ctimespec.tv_sec,st.st_ctimespec.tv_nsec});
#else
 z["mtime"]=Json::array({st.st_mtim.tv_sec,st.st_mtim.tv_nsec});z["ctime"]=Json::array({st.st_ctim.tv_sec,st.st_ctim.tv_nsec});
#endif
 if(S_ISREG(st.st_mode))z["sha256"]=digest(read(path,64*1024*1024));if(S_ISLNK(st.st_mode))z["linkTarget"]=fs::read_symlink(path).string();need(r.emplace(rel,z.dump()).second);};record(p,".");for(const auto& n:fs::recursive_directory_iterator(p))record(n.path(),n.path().lexically_relative(p).generic_string());return r;}
struct Fixture{
 Genesis genesis;CapellaForkBlockRules rules;CapellaStoreSnapshotLimits limits;CapellaSlotState pre;CapellaForkStore initial;CapellaSignedBootstrapArchive authority;Json manifest;std::string genesis_text,profile_text;Phase0ForkSlashingContext context;Json description;
 Fixture(Genesis g,CapellaForkBlockRules r,CapellaStoreSnapshotLimits l,CapellaSlotState s,CapellaForkStore v,CapellaSignedBootstrapArchive a,Json m,std::string gt,std::string pt,Phase0ForkSlashingContext c,Json d):genesis(std::move(g)),rules(std::move(r)),limits(l),pre(std::move(s)),initial(std::move(v)),authority(std::move(a)),manifest(std::move(m)),genesis_text(std::move(gt)),profile_text(std::move(pt)),context(std::move(c)),description(std::move(d)){}
};
Fixture fixture(){
 auto dir=fs::path(NANO_TEST_SOURCE)/"tests/fixtures/capella-bootstrap-anchor-cb532";auto mt=read(dir/"manifest.json");need(digest(mt)=="ec150c28c8c187e6728cea7ef717acc6383e0b75242302b0a481fc2a9c425fc6");auto m=Json::parse(mt);
 auto file=[&](const char* field){const auto& x=m.at("files").at(field);auto b=read(dir/x.at("path").get<std::string>());need(digest(b)==x.at("sha256").get<std::string>()&&b.size()==x.at("bytes").get<size_t>());return b;};
 auto gt=file("genesis"),pt=file("profile"),it=file("initial");auto g=Genesis::from_json(Json::parse(gt));auto r=load_capella_fork_profile(pt,sha256(bytes(pt)),1024*1024);CapellaStoreSnapshotLimits l{1024*1024,4,64,64,1024*1024,4*1024*1024,64,100000};CapellaSlotState pre{bytes(file("genesisState")),std::nullopt};auto store=capella_store_from_snapshot(it,sha256(bytes(it)),r,l,check);auto wire=unhex(m.at("bootstrapSigningHistory").at("block").at("ssz").get<std::string>());auto a=prepare_capella_signed_bootstrap_archive(g,pre,wire,r,l.replay_slots,l.sync_trials,check);need(a.has_value());auto closed=authenticate_capella_checkpoint_blob_closure(it,sha256(bytes(it)),r,l,{}, {},check);return {std::move(g),std::move(r),l,std::move(pre),std::move(store),std::move(*a),std::move(m),std::move(gt),std::move(pt),closed.head_context(),closed.description()};
}
void mutate(const fs::path& p,const std::function<void(rocksdb::DB&)>& action){rocksdb::Options o;o.create_if_missing=false;apply_chain_store_memory_policy(o);rocksdb::DB* raw=nullptr;need(rocksdb::DB::Open(o,p.string(),&raw).ok());std::unique_ptr<rocksdb::DB> db(raw);action(*db);}
std::string get(rocksdb::DB& db,const std::string& k){std::string v;need(db.Get(rocksdb::ReadOptions{},k,&v).ok());return v;}
void put(rocksdb::DB& db,const std::string& k,const std::string& v){rocksdb::WriteOptions o;o.sync=true;need(db.Put(o,k,v).ok());}
Json descriptor(const fs::path& p){auto b=read(p);return {{"path",p.string()},{"sha256",digest(b)},{"bytes",uint64_t(b.size())}};}
Json reservation(const Fixture& f,const Bytes& key,uint64_t slot,bool unknown=false,bool retired=false){Json root=unknown?Json(nullptr):Json("0x"+std::string(64,'1'));Json j{{"format",unknown?"nano-beacon-signing-v3":retired?"nano-beacon-signing-v2":"nano-beacon-signing-v1"},{"publicKey",hex(key)},{"genesisValidatorsRoot",hex(Bytes(f.context.genesis_root.begin(),f.context.genesis_root.end()))},{"slotsPerEpoch",q(f.rules.transition.consensus.epoch.ffg.slots_per_epoch)},{"proposal",Json{{"slot",q(slot)},{"root",root}}},{"attestation",nullptr}};if(unknown||retired)j["retired"]=retired;return j;}
struct Arrangement{
 const Fixture& f;Temp owned;fs::path inputs,databases,result_parent,scratch;Json request;std::vector<fs::path> journals,witnesses;std::vector<Bytes> keys;std::vector<std::optional<uint64_t>> indices;
 Arrangement(const Fixture& fixture,bool cold=false,bool more=false):f(fixture){
  inputs=owned.p/"inputs";databases=owned.p/"databases";result_parent=owned.p/"reports";scratch=owned.p/"scratch";for(auto p:{inputs,databases,result_parent,scratch})need(fs::create_directory(p)&&::chmod(p.c_str(),0700)==0);
  write(inputs/"genesis.json",f.genesis_text);write(inputs/"profile.json",f.profile_text);ForkCheckpointToken cp,cw;
  {Blockchain owner(databases/"core-primary",f.genesis);ChainStore w(databases/"core-witness");CapellaDurableReactor reactor(owner,f.rules,f.limits,f.initial,f.authority,w,check);if(cold)need(reactor.migrate_reference_checkpoint({2,256*1024},check));cp=reactor.current().token;auto rec=w.fork_checkpoint(f.genesis.chain_id,f.genesis.block.header.hash(),2048+8*f.limits.envelope_bytes);need(rec.has_value());cw=rec->token;}
  auto member=f.context.registry.validators.begin();for(size_t i=0;i<(more?3u:2u);++i){need(member!=f.context.registry.validators.end());keys.emplace_back(member->second.public_key.begin(),member->second.public_key.end());indices.emplace_back(member->first);++member;}
  Bytes absent;for(const auto& row:f.manifest.at("publicKeys")){auto k=unhex(row.at("publicKey").get<std::string>());bool found=false;for(const auto& [id,v]:f.context.registry.validators){(void)id;if(Bytes(v.public_key.begin(),v.public_key.end())==k)found=true;}if(!found){absent=std::move(k);break;}}need(absent.size()==48);keys.push_back(absent);indices.push_back(std::nullopt);
  Json roles=Json::array({Json{{"role","synthetic-role-a"},{"keys",Json::array()}},Json{{"role","synthetic-role-b"},{"keys",Json::array()}}});
  for(size_t i=0;i<keys.size();++i){auto p=databases/("reservation-"+std::to_string(i)),w=databases/("intent-"+std::to_string(i));journals.push_back(p);witnesses.push_back(w);ForkCheckpointToken pt,wt;auto text=reservation(f,keys[i],1).dump();{ChainStore a(p),b(w);a.bind_chain(f.genesis.chain_id,f.genesis.block.header.hash(),"nano-beacon-signing-v1");b.bind_chain(f.genesis.chain_id,f.genesis.block.header.hash(),"nano-beacon-signing-witness-v1");pt=a.commit_fork_checkpoint(f.genesis.chain_id,f.genesis.block.header.hash(),std::nullopt,text,4096);auto intent=Json{{"format","nano-beacon-signing-witness-v1"},{"previous",nullptr},{"next",token(pt)},{"snapshot",text}}.dump();wt=b.commit_fork_checkpoint(f.genesis.chain_id,f.genesis.block.header.hash(),std::nullopt,intent,16384);}
   auto sel=[&](const fs::path& x,const ForkCheckpointToken& t){return Json{{"path",x.string()},{"inventorySha256",checkpoint_blob_gc_copy_inventory(x,4096,128*1024*1024,check)},{"token",token(t)},{"floor",nullptr}};};roles.at(i%2).at("keys").push_back(Json{{"publicKey",hex(keys[i])},{"index",indices[i]?Json(q(*indices[i])):Json(nullptr)},{"primary",sel(p,pt)},{"witness",sel(w,wt)}});
  }
  auto core_sel=[&](const fs::path& x,const ForkCheckpointToken& t){return Json{{"path",x.string()},{"inventorySha256",checkpoint_blob_gc_copy_inventory(x,4096,128*1024*1024,check)},{"token",token(t)}};};Json core{{"primary",core_sel(databases/"core-primary",cp)},{"witness",core_sel(databases/"core-witness",cw)},{"expected",Json{{"headRoot",hex(Bytes(f.context.block_root.begin(),f.context.block_root.end()))},{"identity",f.description.at("identity")},{"executionHead",f.description.at("executionHead")}}}};
  Json limits=Json::array({q(f.limits.envelope_bytes),q(f.limits.blocks),q(f.limits.messages),q(f.limits.equivocations),q(f.limits.state_bytes),q(f.limits.total_state_bytes),q(f.limits.replay_slots),q(f.limits.sync_trials)});
  request={{"format","nano-configured-journal-readonly-observer-v1"},{"genesis",descriptor(inputs/"genesis.json")},{"profile",descriptor(inputs/"profile.json")},{"configurationProjection",nullptr},{"limits",limits},{"budget",Json{{"inventoryFiles",q(4096)},{"inventoryBytes",q(128*1024*1024)},{"durationMs",q(5000)},{"maximumRssKiB",q(131072)}}},{"maximumRegistryEntries",q(f.context.registry.validators.size()+1)},{"core",core},{"roles",roles}};if(more){std::reverse(request.at("roles").begin(),request.at("roles").end());for(auto& r:request.at("roles"))std::reverse(r.at("keys").begin(),r.at("keys").end());}seal_projection();
 }
 void seal_projection(){Json projection{{"format","nano-root-configured-journal-path-key-projection-v1"},{"originalConfigurationSha256",digest("public synthetic configuration attestation only")},{"core",request.at("core")},{"roles",request.at("roles")}};write(inputs/"projection.json",projection.dump());request["configurationProjection"]=descriptor(inputs/"projection.json");}
 void refresh(const fs::path& p){for(auto* sel:{&request.at("core").at("primary"),&request.at("core").at("witness")})if(sel->at("path")==p.string())sel->at("inventorySha256")=checkpoint_blob_gc_copy_inventory(p,4096,128*1024*1024,check);for(auto& role:request.at("roles"))for(auto& key:role.at("keys"))for(auto field:{"primary","witness"})if(key.at(field).at("path")==p.string())key.at(field).at("inventorySha256")=checkpoint_blob_gc_copy_inventory(p,4096,128*1024*1024,check);seal_projection();}
 Json& key(size_t index){for(auto& role:request.at("roles"))for(auto& k:role.at("keys"))if(k.at("publicKey")==hex(keys.at(index)))return k;throw std::runtime_error("synthetic key missing");}
 void journal(size_t i,const Json& primary,const Json& candidate,bool pending){ForkCheckpointToken pt,wt;{ChainStore a(journals.at(i)),b(witnesses.at(i));auto oldp=a.fork_checkpoint(f.genesis.chain_id,f.genesis.block.header.hash(),4096);auto oldw=b.fork_checkpoint(f.genesis.chain_id,f.genesis.block.header.hash(),16384);need(oldp&&oldw);pt=a.commit_fork_checkpoint(f.genesis.chain_id,f.genesis.block.header.hash(),oldp->token,primary.dump(),4096);auto next=pt;if(pending){++next.sequence;next.digest=sha256(bytes(candidate.dump()));}auto intent=Json{{"format","nano-beacon-signing-witness-v1"},{"previous",pending?token(pt):token(oldp->token)},{"next",token(next)},{"snapshot",candidate.dump()}}.dump();wt=b.commit_fork_checkpoint(f.genesis.chain_id,f.genesis.block.header.hash(),oldw->token,intent,16384);}key(i).at("primary").at("token")=token(pt);key(i).at("witness").at("token")=token(wt);refresh(journals.at(i));refresh(witnesses.at(i));}
};
struct ChildGuard{pid_t pid;bool live=true;~ChildGuard(){if(live){(void)::kill(pid,SIGKILL);int status;while(::waitpid(pid,&status,0)<0&&errno==EINTR){}}}};
struct PipeReadGuard{int fd;~PipeReadGuard(){if(fd>=0)(void)::close(fd);}};
struct Outcome{int status=0;bool timed=false;std::string log,event;Json typed;fs::path report;};
Outcome child(Arrangement& a,const std::string& name,std::optional<fs::path> output={},std::string fault={},fs::path fault_target={},bool terminate=false){
 check();current_case=name;last_tool=Json::object();last_tool_exit=std::nullopt;++calls;auto req=a.inputs/("request-"+std::to_string(calls)+".json");write(req,a.request.dump());auto out=output.value_or(a.result_parent/("result-"+std::to_string(calls)+".json"));auto log=a.owned.p/("child-"+std::to_string(calls)+".log");auto before=hashes(a.databases);auto input_before=hashes(a.inputs);need(fs::is_empty(a.scratch));int pipefd[2];need(::pipe(pipefd)==0);auto pid=::fork();need(pid>=0);
 if(pid==0){::close(pipefd[0]);int fd=::open(log.c_str(),O_WRONLY|O_CREAT|O_EXCL,0600);if(fd<0||::dup2(fd,STDOUT_FILENO)<0||::dup2(fd,STDERR_FILENO)<0)_exit(120);::close(fd);::clearenv();::setenv("PATH","/usr/bin:/bin",1);::setenv("LC_ALL","C",1);
  if(!fault.empty()){::setenv("LD_PRELOAD",NANO_TEST_IO_SHIM,1);::setenv("NANO_TEST_JOURNAL_IO_MODE",fault.c_str(),1);auto target=fault_target.empty()?req:fault_target;::setenv("NANO_TEST_JOURNAL_IO_TARGET",target.c_str(),1);::setenv("NANO_TEST_JOURNAL_IO_EVENT",std::to_string(pipefd[1]).c_str(),1);::setenv("NANO_TEST_JOURNAL_IO_TRIGGER",req.c_str(),1);}else ::close(pipefd[1]);auto sh=digest(a.request.dump());::execl(NANO_TEST_TOOL,NANO_TEST_TOOL,"--observe",req.c_str(),sh.c_str(),out.c_str(),a.scratch.c_str(),nullptr);_exit(121);
 }
 ChildGuard child_guard{pid};PipeReadGuard read_guard{pipefd[0]};::close(pipefd[1]);need(::fcntl(pipefd[0],F_SETFL,O_NONBLOCK)==0);Outcome result;result.report=out;const auto at=Clock::now();bool stopped=false;for(;;){char b[16];auto n=::read(pipefd[0],b,sizeof(b));if(n>0)result.event.append(b,size_t(n));auto state=::waitpid(pid,&result.status,WNOHANG|WUNTRACED);if(state==pid){if(WIFSTOPPED(result.status)){need(terminate&&!stopped&&WSTOPSIG(result.status)==SIGSTOP);stopped=true;need(::kill(pid,SIGTERM)==0&&::kill(pid,SIGCONT)==0);continue;}child_guard.live=false;break;}need(state==0||errno==EINTR);if(Clock::now()-at>std::chrono::seconds(8)){result.timed=true;::kill(pid,SIGKILL);while(::waitpid(pid,&result.status,0)<0){need(errno==EINTR);}child_guard.live=false;break;}::usleep(1000);}
 char rest[32];for(;;){auto n=::read(pipefd[0],rest,sizeof(rest));if(n<=0)break;result.event.append(rest,size_t(n));}::close(pipefd[0]);read_guard.fd=-1;if(!result.event.empty())++events;result.log=read(log,65536);for(auto at=size_t(0);at<result.log.size();){auto end=result.log.find('\n',at);if(end==std::string::npos)end=result.log.size();try{auto j=Json::parse(result.log.substr(at,end-at));if(j.is_object()&&j.value("format",std::string{})=="nano-configured-journal-observation-typed-v1")result.typed=std::move(j);}catch(...){}at=end+1;}
 last_tool=result.typed;last_tool_exit=WIFEXITED(result.status)?std::optional<int>(WEXITSTATUS(result.status)):std::nullopt;need(!result.timed&&hashes(a.databases)==before&&hashes(a.inputs)==input_before);if(terminate)need(stopped&&!fs::exists(out)&&((WIFSIGNALED(result.status)&&WTERMSIG(result.status)==SIGTERM)||(WIFEXITED(result.status)&&WEXITSTATUS(result.status)==1&&result.typed.at("status")=="REFUSED_NO_AUTOMATIC_RETRY")));check();return result;
}
void safety(const Json& t){need(t.at("newSignaturesCreated")==0&&t.at("keysLoaded")==false&&t.at("runtimeStarted")==false&&t.at("runtimeDutiesAuthorized")==false&&t.at("productionQualified")==false);}
Json accepted(Arrangement& a,const std::string& label){auto o=child(a,label);need(WIFEXITED(o.status)&&WEXITSTATUS(o.status)==0&&o.typed.at("status")=="READONLY_CONFIGURED_JOURNAL_RESERVATIONS_OBSERVED");safety(o.typed);need(o.typed.at("pairsObserved")==a.keys.size()&&o.typed.at("independentColdPasses")==2&&fs::is_empty(a.scratch));struct stat s{};need(::lstat(o.report.c_str(),&s)==0&&(s.st_mode&0777)==0600);auto r=Json::parse(read(o.report));need(r.at("rows").size()==a.keys.size()&&r.at("twoIndependentColdReads")==true&&r.at("currentHeadAuthenticated")==true&&r.at("floorConstraintsObserved")==true);for(auto k:{"historicalDigestAncestryVerified","deliveredSignatureHistoryVerified","executionAccountNoncesVerified","runtimeDutiesAuthorized","productionQualified"})need(r.at(k)==false);++positives;return r;}
void denied(Arrangement& a,const std::string& label,std::optional<fs::path> out={},std::string fault={},fs::path target={},std::string expected_stage="request"){auto reports_before=hashes(a.result_parent);reports_before.erase(".");auto o=child(a,label,out,fault,target);auto reports_after=hashes(a.result_parent);reports_after.erase(".");need(reports_before==reports_after);need(WIFEXITED(o.status)&&WEXITSTATUS(o.status)==1&&o.typed.at("status")=="REFUSED_NO_AUTOMATIC_RETRY");safety(o.typed);need(o.typed.at("refusedStage")==expected_stage);need(!fs::exists(o.report));need(fs::is_empty(a.scratch));if(!fault.empty())need(!o.event.empty());++refusals;}
void emit(const char* scenario,const Json& extra=Json::object()){auto row=extra;row["scenario"]=scenario;row["result"]="PASS";row["syntheticOnly"]=true;row["newSignaturesCreated"]=0;row["signerConstructorsUsed"]=0;row["privateInputsRead"]=false;row["runtimeDutiesAuthorized"]=false;row["productionQualified"]=false;std::cout<<row.dump()<<'\n';}
void diagnose_first_positive(const Fixture& f){
 {Arrangement a(f);auto report=accepted(a,"dynamic-hot-three-keys-two-roles");std::vector<std::string> expected;for(const auto& r:a.request.at("roles"))for(const auto& k:r.at("keys"))expected.push_back(k.at("publicKey").get<std::string>());for(size_t i=0;i<expected.size();++i)need(report.at("rows").at(i).at("publicKey")==expected[i]);bool absent_seen=false;for(const auto& row:report.at("rows"))if(row.at("publicKey")==hex(a.keys.back())){need(row.at("registeredIndex").is_null());absent_seen=true;}need(absent_seen);}
 emit("configured-journal-first-positive-diagnostic",{{"configuredKeys",3},{"roleCount",2},{"unchangedFirstPositiveCase",true}});
 emit("configured-journal-first-positive-diagnostic-summary",{{"actualWrapperCalls",calls},{"acceptedCalls",positives},{"refusedCalls",refusals},{"actualTestIoEventCalls",events},{"maxRssKiB",peak()},{"elapsedMs",std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-began).count()},{"fullSuiteRun",false}});
}
void groups(const Fixture& f){
 {Arrangement a(f);auto report=accepted(a,"dynamic-hot-three-keys-two-roles");std::vector<std::string> expected;for(const auto& r:a.request.at("roles"))for(const auto& k:r.at("keys"))expected.push_back(k.at("publicKey").get<std::string>());for(size_t i=0;i<expected.size();++i)need(report.at("rows").at(i).at("publicKey")==expected[i]);bool absent_seen=false;for(const auto& row:report.at("rows"))if(row.at("publicKey")==hex(a.keys.back())){need(row.at("registeredIndex").is_null());absent_seen=true;}need(absent_seen);}
 {Arrangement a(f,true,true);auto report=accepted(a,"dynamic-cold-four-keys-reordered-roles");size_t index=0;for(const auto& r:a.request.at("roles"))for(const auto& k:r.at("keys")){need(report.at("rows").at(index).at("role")==r.at("role")&&report.at("rows").at(index).at("publicKey")==k.at("publicKey"));++index;}}
 emit("dynamic-configured-mapping-positive",{{"arrangements",2},{"roleCount",2},{"configuredCounts",Json::array({3,4})},{"allSourceHandlesClosedBeforeChildren",true},{"actualOriginalConfigurationQualified",false}});
 {Arrangement a(f);auto valid=a.request;
  a.request.at("roles").at(0).at("keys").erase(a.request.at("roles").at(0).at("keys").begin());denied(a,"projection-omitted-key");a.request=valid;
  a.request.at("roles").at(0).at("keys").push_back(a.request.at("roles").at(1).at("keys").at(0));denied(a,"projection-extra-key");a.request=valid;
  std::reverse(a.request.at("roles").begin(),a.request.at("roles").end());denied(a,"projection-reordered-request-only");a.request=valid;
  a.request.at("configurationProjection").at("sha256")=std::string(64,'0');denied(a,"projection-descriptor-digest-refreshed-request");
 }
 emit("configuration-projection-equality-refusals",{{"requestShaRefreshedEachCase",true},{"projectionEqualityReached",true}});
 {Arrangement a(f);auto valid=a.request;
  a.key(0).at("primary").at("token").at("digest")="0x"+std::string(64,'0');a.seal_projection();denied(a,"wrong-current-primary",{},{},{},"configured-journals");a.request=valid;a.seal_projection();
  a.key(0).at("witness").at("token").at("sequence")=q(pin(a.key(0).at("witness").at("token")).sequence+1);a.seal_projection();denied(a,"wrong-current-witness",{},{},{},"configured-journals");a.request=valid;a.seal_projection();
  auto current=pin(a.key(0).at("primary").at("token"));auto floor=current;floor.digest[0]^=1;a.key(0).at("primary").at("floor")=token(floor);a.seal_projection();denied(a,"equal-sequence-different-floor-digest",{},{},{},"configured-journals");a.request=valid;a.seal_projection();
  floor=current;++floor.sequence;a.key(0).at("primary").at("floor")=token(floor);a.seal_projection();denied(a,"higher-floor",{},{},{},"configured-journals");a.request=valid;a.seal_projection();
  Json valid_identity;mutate(a.journals[0],[&](rocksdb::DB& db){valid_identity=Json::parse(get(db,"meta/identity"));});for(const auto field:{"consensus","chainId","genesisHash"}){auto wrong=valid_identity;wrong[field]=std::string(field)=="consensus"?Json("nano-beacon-signing-witness-v1"):std::string(field)=="chainId"?Json(q(u64(f.genesis.chain_id)+1)):Json("0x"+std::string(64,'0'));mutate(a.journals[0],[&](rocksdb::DB& db){put(db,"meta/identity",wrong.dump());});a.refresh(a.journals[0]);denied(a,std::string("wrong-journal-namespace-")+field,{},{},{},"configured-journals");}
 }
 {Arrangement a(f);auto p=reservation(f,a.keys[0],2);a.journal(0,p,p,false);a.key(0).at("primary").at("floor")=Json{{"sequence",q(1)},{"digest","0x"+std::string(64,'7')}};a.seal_projection();auto r=accepted(a,"lower-numeric-floor-not-ancestry");need(r.at("historicalDigestAncestryVerified")==false);}
 emit("namespaces-and-current-pins-refusals",{{"distinctCurrentAndFloors",true},{"lowerNumericFloorDoesNotProveAncestry",true}});
 {Arrangement a(f);auto p=reservation(f,a.keys[0],2),next=reservation(f,a.keys[0],3);a.journal(0,p,next,true);auto retired=reservation(f,a.keys[1],1,false,true);a.journal(1,retired,retired,false);auto conservative=reservation(f,a.keys[2],1,true);a.journal(2,conservative,conservative,false);auto r=accepted(a,"pending-retired-conservative-observation");bool pending=false,retired_seen=false,unknown=false;for(const auto& row:r.at("rows")){pending=pending||row.at("disposition")=="pending";retired_seen=retired_seen||row.at("reservations").at("retired")==true;unknown=unknown||row.at("reservations").at("proposal").at("root").is_null();}need(pending&&retired_seen&&unknown);
  auto weaker=reservation(f,a.keys[0],1);a.journal(0,p,weaker,true);denied(a,"pending-dropped-watermark",{},{},{},"configured-journals");
 }
 {Arrangement a(f);a.key(0).at("index")=q(*a.indices[1]);a.seal_projection();denied(a,"fixed-index-other-real-key",{},{},{},"configured-journals");}
 emit("journal-completed-pending-retired-and-index",{{"pendingObservedWithoutRepair",true},{"conservativeUnknownRootAndRetiredObserved",true},{"absentAutoRegistrationIsNotActivation",true}});
 for(unsigned variant=0;variant<4;++variant){Arrangement a(f);const auto primary=a.databases/"core-primary";mutate(primary,[&](rocksdb::DB& db){auto tip=get(db,"meta/tip");if(variant==0)need(db.Delete(rocksdb::WriteOptions{},"meta/tip").ok());else if(variant==1)need(db.Delete(rocksdb::WriteOptions{},"block/"+tip).ok());else if(variant==2){need(!get(db,"state/"+tip).empty());need(db.Delete(rocksdb::WriteOptions{},"state/"+tip).ok());}else {auto marker=Json::parse(get(db,"execution-checkpoint/v1"));marker["tip"]="0x"+std::string(64,'0');put(db,"execution-checkpoint/v1",marker.dump());}});a.refresh(primary);denied(a,"canonical-core-binding-"+std::to_string(variant),{},{},{},"current-head");}
 {Arrangement a(f);auto path=a.databases/"core-witness";ForkCheckpointToken wt;{ChainStore w(path);auto old=w.fork_checkpoint(f.genesis.chain_id,f.genesis.block.header.hash(),2048+8*f.limits.envelope_bytes);need(old.has_value());auto intent=Json::parse(old->snapshot);intent["previous"]=a.request.at("core").at("primary").at("token");auto n=pin(intent.at("next"));++n.sequence;intent["next"]=token(n);wt=w.commit_fork_checkpoint(f.genesis.chain_id,f.genesis.block.header.hash(),old->token,intent.dump(),2048+8*f.limits.envelope_bytes);}a.request.at("core").at("witness").at("token")=token(wt);a.refresh(path);denied(a,"genuine-full-next-core-pending-never-repaired",{},{},{},"current-head");}
 {Arrangement a(f,true);auto primary=a.databases/"core-primary";mutate(primary,[&](rocksdb::DB& db){auto iter=std::unique_ptr<rocksdb::Iterator>(db.NewIterator(rocksdb::ReadOptions{}));iter->Seek("checkpoint-blob/v1/");need(iter->Valid()&&iter->key().starts_with(rocksdb::Slice("checkpoint-blob/v1/")));auto key=iter->key().ToString();auto value=iter->value().ToString();need(!value.empty());value[0]^=1;iter.reset();put(db,key,value);});a.refresh(primary);denied(a,"corrupt-real-cold-reference",{},{},{},"current-head");}
 emit("canonical-core-and-full-closure-refusals",{{"actualCanonicalTipBlockStateAndMarkerChecked",true},{"settledCoreRequired",true},{"actualColdBlobCorruptionTested",true}});
 {Arrangement a(f);auto valid=a.request;auto alias=a.inputs/"profile-alias.json";fs::create_symlink(a.inputs/"profile.json",alias);a.request.at("profile").at("path")=alias.string();denied(a,"symlink-artifact");a.request=valid;
  auto fifo=a.inputs/"no-writer.fifo";need(::mkfifo(fifo.c_str(),0600)==0);a.request.at("profile").at("path")=fifo.string();denied(a,"fifo-without-writer-bounded");a.request=valid;
  auto hard=a.inputs/"genesis-hard.json";need(::link((a.inputs/"genesis.json").c_str(),hard.c_str())==0);a.request.at("genesis").at("path")=hard.string();denied(a,"hardlinked-artifact");fs::remove(hard);a.request=valid;
  a.request.at("profile").at("bytes")=uint64_t(1);denied(a,"artifact-size-mismatch");a.request=valid;
  a.request.at("limits").at(0)=q(UINT64_MAX);denied(a,"witness-bound-overflow");
 }
 {Arrangement a(f);auto p=a.journals[0];auto value=std::string(65536,'x');auto cur=pin(a.key(0).at("primary").at("token"));cur.digest=sha256(bytes(value));mutate(p,[&](rocksdb::DB& db){put(db,"fork-checkpoint/v1/payload",value);put(db,"fork-checkpoint/v1/token",token(cur).dump());});a.key(0).at("primary").at("token")=token(cur);a.refresh(p);denied(a,"semantic-oversize-real-sdk-value",{},{},{},"configured-journals");}
 emit("bounded-artifacts-and-db-records",{{"fifoHasNoWriter",true},{"oversizedSdkValueBytes",65536},{"semanticSliceGuardObservedAsRefusal",true},{"interiorAllocationCountMeasured",false}});
 {Arrangement a(f);denied(a,"output-inside-source",a.journals[0]/"report.json");denied(a,"output-inside-scratch",a.scratch/"report.json");denied(a,"output-inside-inputs",a.inputs/"report.json");auto existing=a.result_parent/"sentinel.json";write(existing,"never delete or overwrite");auto before=read(existing);auto o=child(a,"preexisting-output-sentinel",existing);need(WIFEXITED(o.status)&&WEXITSTATUS(o.status)==1&&o.typed.at("status")=="REFUSED_NO_AUTOMATIC_RETRY"&&read(existing)==before);++refusals;}
 {Arrangement a(f);auto target=a.result_parent;auto o=child(a,"output-parent-rebind-during-request-read",{},"rename-parent",target);need(o.event.find('R')!=std::string::npos&&WIFEXITED(o.status)&&WEXITSTATUS(o.status)==1&&o.typed.at("status")=="REFUSED_NO_AUTOMATIC_RETRY"&&read(target/"substituted-sentinel")=="test sentinel\n"&&!fs::exists(o.report));need(fs::is_empty(a.scratch));++refusals;}
 emit("physical-alias-output-and-readback",{{"exactPathsAndHeldParentRebindTested",true},{"substitutedSentinelPreserved",true},{"privilegedBindMountOrCaseAliasTested",false}});
 {Arrangement a(f);a.request.at("budget").at("durationMs")=q(5);denied(a,"finite-deadline-after-observed-read-delay",{},"delay");}
 for(auto mode:{std::string("file-fsync"),std::string("parent-fsync")}){Arrangement a(f);auto output=a.result_parent/"fault-report.json";denied(a,"observed-"+mode,output,mode,output,"publication");}
 {Arrangement a(f);auto o=child(a,"observed-sigstop-sigterm-before-replica",{},"stop",{},true);need(o.event.find('S')!=std::string::npos&&fs::is_empty(a.scratch));if(WIFEXITED(o.status)){safety(o.typed);need(o.typed.at("refusedStage")=="request");}++refusals;}
 emit("cancellation-partial-and-publish-failure",{{"actualIoEventsRequired",true},{"realStoppedChildTerminatedAndJoined",true},{"fileAndParentFsyncFailuresObserved",true},{"internalCallbackCancellationAfterFirstPairOrFinalCheckTested",false},{"physicalPowerLossTested",false},{"acceptedCompleteReportAfterFailure",false}});
}
}
int main(int argc,char** argv){::umask(0077);try{need(argc==2&&(std::string(argv[1])=="--self-test"||std::string(argv[1])=="--diagnose-first-positive"));const ChainStoreMemoryPolicy policy{4*1024*1024,2,16*1024*1024,4*1024*1024};ScopedChainStoreMemoryPolicy scope(policy);auto f=fixture();if(std::string(argv[1])=="--diagnose-first-positive"){diagnose_first_positive(f);return 0;}groups(f);emit("configured-journal-observer-public-io-summary",{{"actualWrapperCalls",calls},{"acceptedCalls",positives},{"refusedCalls",refusals},{"actualTestIoEventCalls",events},{"maxRssKiB",peak()},{"elapsedMs",std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-began).count()},{"syntheticRwSetupDatabasesUsed",true},{"allRwClosedBeforeToolRo",true},{"actualOperatorConfigurationQualified",false}});return 0;}catch(...){Json d{{"diagnostic","configured-journal-observer-public-io-failure-v1"},{"case",current_case},{"toolExit",last_tool_exit?Json(*last_tool_exit):Json(nullptr)},{"syntheticOnly",true},{"privateInputsRead",false},{"productionQualified",false}};if(last_tool.is_object()){for(auto key:{"status","refusedStage","pairsObserved","maximumRssKiB"})if(last_tool.contains(key))d[key]=last_tool.at(key);if(last_tool.contains("requestStep")&&last_tool.at("requestStep").is_string()){auto step=last_tool.at("requestStep").get<std::string>();if(std::find(request_steps.begin(),request_steps.end(),std::string_view(step))!=request_steps.end())d["requestStep"]=step;}}std::cerr<<d.dump()<<"\n";return 1;}}
