#include "nano/chain.hpp"
#include "tools/storage-maintenance/bootstrap-readonly-replica.hpp"
#include <rocksdb/db.h>
#include <array>
#include <chrono>
#include <cerrno>
#include <csignal>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <set>
#include <spawn.h>
#include <string>
#include <thread>
#include <vector>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace nano;
namespace fs=std::filesystem;
namespace rd=nano::maintenance::readonly_replica_detail;
namespace {
const auto began=std::chrono::steady_clock::now();
volatile std::sig_atomic_t interrupted=0;
size_t commands=0,successes=0,refusals=0,readonly_checks=0;
void need(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void on_signal(int number){interrupted=number;}
uint64_t rss(){rusage r{};need(::getrusage(RUSAGE_SELF,&r)==0,"resource observation");
#ifdef __APPLE__
 return uint64_t(r.ru_maxrss)/1024;
#else
 return uint64_t(r.ru_maxrss);
#endif
}
void checkpoint(){need(!interrupted&&std::chrono::steady_clock::now()-began<std::chrono::seconds(180)&&rss()<=131072,"CLI probe interrupted or resource limit");}
struct Signals{
 using Handler=void(*)(int);std::array<Handler,3> previous{};size_t installed=0;bool closed=false;
 Signals(){try{for(int n:{SIGINT,SIGTERM,SIGHUP}){auto p=::signal(n,on_signal);need(p!=SIG_ERR,"install probe signal handler");previous[installed++]=p;}}catch(...){const auto first=std::current_exception();try{close();}catch(...){std::cerr<<"secondary signal restoration failure\n";}std::rethrow_exception(first);}}
 void close(){bool ok=true;const std::array<int,3> numbers{SIGINT,SIGTERM,SIGHUP};for(size_t i=0;i<installed;++i)if(::signal(numbers[i],previous[i])==SIG_ERR)ok=false;closed=true;need(ok,"restore probe signal handlers");}
 ~Signals(){if(!closed)try{close();}catch(...){}}
};
std::string digest(const std::string& body){return hex(sha256(Bytes(body.begin(),body.end()))).substr(2);}
void create_file(const fs::path& path,const std::string& body){
 rd::Fd fd(::open(path.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600));size_t at=0;
 while(at<body.size()){const auto n=::write(fd.n,body.data()+at,body.size()-at);if(n<0&&errno==EINTR)continue;need(n>0,"exclusive evidence write");at+=size_t(n);}rd::sync(fd.n);rd::Fd parent(::open(path.parent_path().c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));rd::sync(parent.n);
}
struct FilePin{
 fs::path path;rd::Fd fd;struct stat stamp{};std::string sha;
 FilePin(const fs::path& p,const std::string& expected,uint64_t bytes):path(p),fd(::open(p.c_str(),O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC)),sha(expected){need(p.is_absolute()&&fs::canonical(p)==p&&::fstat(fd.n,&stamp)==0&&S_ISREG(stamp.st_mode)&&stamp.st_nlink==1&&stamp.st_size>=0&&uint64_t(stamp.st_size)==bytes,"pinned physical regular input");check();}
 void check()const{struct stat linked{},held{};need(::lstat(path.c_str(),&linked)==0&&::fstat(fd.n,&held)==0&&rd::stable(stamp,linked)&&rd::stable(stamp,held)&&rd::file_hash(fd.n,stamp,checkpoint)==sha,"strict input full9/SHA unchanged");}
};
struct Fence{
 rd::Directory directory;std::vector<rd::Entry> rows;
 explicit Fence(const fs::path& p):directory(p),rows(rd::capture(directory.fd(),4096,32*1024*1024,checkpoint)){}
 void check()const{directory.recheck();need(rd::same_entries(rows,rd::capture(directory.fd(),4096,32*1024*1024,checkpoint)),"CLI source full9/SHA unchanged");directory.recheck();}
};
struct Exit {int code;std::string out,err;};
class Cli{
 const FilePin& binary;rd::Directory& root;fs::path logs,scratch;
 static bool absent(pid_t pid){if(::kill(-pid,0)<0){need(errno==ESRCH,"owned group absence query");return true;}return false;}
 static void join_group(pid_t pid,bool& reaped,int& status){
  need(pid>1&&pid!=28722,"reserved PID excluded");
  if(!absent(pid))need(::kill(-pid,SIGTERM)==0||errno==ESRCH,"owned group TERM");
  const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(3);
  while(!reaped&&std::chrono::steady_clock::now()<until){const auto n=::waitpid(pid,&status,WNOHANG);if(n==pid)reaped=true;else need(n==0||(n<0&&errno==EINTR),"owned child wait");if(!reaped)std::this_thread::sleep_for(std::chrono::milliseconds(10));}
  if(!absent(pid))need(::kill(-pid,SIGKILL)==0||errno==ESRCH,"owned group KILL");
  const auto kill_end=std::chrono::steady_clock::now()+std::chrono::seconds(3);while(!reaped&&std::chrono::steady_clock::now()<kill_end){const auto n=::waitpid(pid,&status,WNOHANG);if(n==pid)reaped=true;else need(n==0||(n<0&&errno==EINTR),"owned child final wait");if(!reaped)std::this_thread::sleep_for(std::chrono::milliseconds(10));}need(reaped,"owned child final join");
  const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(2);while(!absent(pid)&&std::chrono::steady_clock::now()<end)std::this_thread::sleep_for(std::chrono::milliseconds(10));need(absent(pid),"owned group remains");
 }
 static std::string read_log(int fd){struct stat s{};need(::fstat(fd,&s)==0&&s.st_size>=0&&s.st_size<=65536,"CLI stream bound");std::string out(size_t(s.st_size),'\0');size_t at=0;while(at<out.size()){auto n=::pread(fd,out.data()+at,out.size()-at,off_t(at));if(n<0&&errno==EINTR)continue;need(n>0,"complete CLI stream");at+=size_t(n);}return out;}
public:
 Cli(const FilePin& b,rd::Directory& r,const fs::path& l,const fs::path& s):binary(b),root(r),logs(l),scratch(s){}
 Exit run(const std::string& phase,const std::vector<std::string>& tail){
  checkpoint();binary.check();root.recheck();need(phase.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-")==std::string::npos,"closed phase name");
  rd::Fd out(::open((logs/(phase+".stdout.log")).c_str(),O_RDWR|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600)),err(::open((logs/(phase+".stderr.log")).c_str(),O_RDWR|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600));
  std::vector<std::string> args{binary.path.string()};args.insert(args.end(),tail.begin(),tail.end());std::vector<char*> argv;for(auto& a:args)argv.push_back(a.data());argv.push_back(nullptr);
  std::vector<std::string> environment{"PATH=/usr/bin:/bin","LANG=C","LC_ALL=C","TMPDIR="+scratch.string()};std::vector<char*> env;for(auto& v:environment)env.push_back(v.data());env.push_back(nullptr);
  posix_spawn_file_actions_t actions;posix_spawnattr_t attributes;need(::posix_spawn_file_actions_init(&actions)==0,"spawn actions");if(::posix_spawnattr_init(&attributes)!=0){::posix_spawn_file_actions_destroy(&actions);throw std::runtime_error("spawn attributes");}
  bool initialized=true;pid_t child=-1;int status=0;bool reaped=false,group_closed=false;std::exception_ptr first;Exit result{};
  try{
   need(::posix_spawn_file_actions_adddup2(&actions,out.n,STDOUT_FILENO)==0&&::posix_spawn_file_actions_adddup2(&actions,err.n,STDERR_FILENO)==0&&::posix_spawn_file_actions_addopen(&actions,STDIN_FILENO,"/dev/null",O_RDONLY,0)==0&&::posix_spawnattr_setpgroup(&attributes,0)==0&&::posix_spawnattr_setflags(&attributes,POSIX_SPAWN_SETPGROUP)==0,"closed child streams/group");
   ++commands;pid_t candidate=-1;const int spawned=::posix_spawn(&candidate,binary.path.c_str(),&actions,&attributes,argv.data(),env.data());need(spawned==0&&candidate>1&&candidate!=28722,"explicit CLI spawn");child=candidate;::posix_spawn_file_actions_destroy(&actions);::posix_spawnattr_destroy(&attributes);initialized=false;
   const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(30);
   while(!reaped){checkpoint();need(std::chrono::steady_clock::now()<end,"CLI deadline");struct stat a{},b{};need(::fstat(out.n,&a)==0&&::fstat(err.n,&b)==0&&a.st_size<=65536&&b.st_size<=65536,"CLI output cap");const auto n=::waitpid(child,&status,WNOHANG);if(n==child)reaped=true;else need(n==0||(n<0&&errno==EINTR),"CLI wait");if(!reaped)std::this_thread::sleep_for(std::chrono::milliseconds(10));}
   need(WIFEXITED(status),"CLI must exit normally");result.code=WEXITSTATUS(status);
  }catch(...){first=std::current_exception();}
  if(initialized){::posix_spawn_file_actions_destroy(&actions);::posix_spawnattr_destroy(&attributes);}
  if(child>1&&child!=28722)try{join_group(child,reaped,status);group_closed=true;}catch(...){if(!first)first=std::current_exception();else std::cerr<<"secondary owned-group cleanup failure\n";}
  const auto attempt=[&](auto action){try{action();}catch(...){if(!first)first=std::current_exception();else std::cerr<<"secondary CLI closure failure\n";}};
  attempt([&]{rd::sync(out.n);});attempt([&]{rd::sync(err.n);});attempt([&]{result.out=read_log(out.n);});attempt([&]{result.err=read_log(err.n);});
  attempt([&]{create_file(logs/(phase+".exit.json"),Json{{"pid",child},{"normalExit",reaped&&WIFEXITED(status)},{"exitCode",reaped&&WIFEXITED(status)?Json(WEXITSTATUS(status)):Json(nullptr)},{"ownedGroupJoined",group_closed},{"controllerError",bool(first)}}.dump()+"\n");});attempt([&]{binary.check();});attempt([&]{root.recheck();});
  if(first)std::rethrow_exception(first);return result;
 }
};
Json success(const Exit& e){need(e.code==0&&e.err.empty(),"actual CLI success/empty stderr");++successes;std::vector<std::set<std::string>> keys;const auto callback=[&](int,Json::parse_event_t event,Json& value){if(event==Json::parse_event_t::object_start)keys.emplace_back();else if(event==Json::parse_event_t::key)need(!keys.empty()&&keys.back().insert(value.get<std::string>()).second,"duplicate CLI JSON key");else if(event==Json::parse_event_t::object_end){need(!keys.empty(),"balanced CLI JSON object");keys.pop_back();}return true;};auto result=Json::parse(e.out,callback);need(keys.empty(),"complete CLI JSON");return result;}
void rejection(const Exit& e,const std::string& message,bool prefix=false){need(e.code==1&&e.out.empty()&&(prefix?e.err.starts_with(message):e.err==message),"actual CLI refusal status/stdout/reason");++refusals;}
Json status(const Genesis& g,const Block& b,const State& s,const Int& difficulty){return {{"chainId",quantity(g.chain_id)},{"consensus",g.consensus},{"blockNumber",quantity(b.header.number)},{"blockHash",hex(b.header.hash())},{"stateRoot",hex(s.root())},{"totalDifficulty",quantity(difficulty)}};}
void status_matches(const Json& actual,const Json& expected){need(actual.is_object()&&actual.size()==6,"closed status object");for(const char* k:{"chainId","consensus","blockNumber","blockHash","stateRoot","totalDifficulty"})need(actual.at(k).is_string(),"typed status fields");need(actual==expected,"exact CLI status oracle");}
void audit_matches(const Json& a,const Block& block,const State& state,size_t blocks){need(a.is_object()&&a.size()==7&&a.at("verified").is_boolean()&&a.at("blocks").is_number_unsigned()&&a.at("posSignatureVersion").is_number_unsigned(),"closed typed audit object");for(const char* k:{"certifiedBlocks","transactions","blockHash","stateRoot"})need(a.at(k).is_string(),"typed audit quantities/hashes");need(a==Json{{"verified",true},{"certifiedBlocks","0x0"},{"blocks",blocks},{"transactions","0x0"},{"blockHash",hex(block.header.hash())},{"stateRoot",hex(state.root())},{"posSignatureVersion",0}},"exact CLI audit oracle");}
template<class F>void readonly_case(Fence& fence,F action){fence.check();std::exception_ptr error;try{action();}catch(...){error=std::current_exception();}try{fence.check();}catch(...){if(!error)error=std::current_exception();else std::cerr<<"secondary readonly-source fence failure\n";}if(error)std::rethrow_exception(error);++readonly_checks;}
template<class Database=rocksdb::DB>std::unique_ptr<Database> adverse_open(const fs::path& p){rocksdb::Options o;o.create_if_missing=false;std::unique_ptr<Database> db;if constexpr(requires{Database::Open(o,p.string(),&db);})need(Database::Open(o,p.string(),&db).ok(),"new adverse fixture open");else{Database* raw=nullptr;const auto s=Database::Open(o,p.string(),&raw);db.reset(raw);need(s.ok(),"new adverse fixture open");}return db;}
void alter_new(const fs::path& p,bool schema){auto db=adverse_open(p);rocksdb::WriteOptions w;w.sync=true;need((schema?db->Put(w,"meta/schema","synthetic-cli-readonly/unsupported"):db->Delete(w,"meta/tip")).ok(),"closed adverse fixture mutation");}
} // namespace

int main(int argc,char** argv){try{
 need(argc==5,"usage: probe CANDIDATE_ABSOLUTE SHA256 BYTES ABSENT_SYNTHETIC_ROOT");const std::string sha=argv[2],size=argv[3];need(sha.size()==64&&sha.find_first_not_of("0123456789abcdef")==std::string::npos&&!size.empty()&&size.find_first_not_of("0123456789")==std::string::npos,"explicit candidate pin");const auto bytes=std::stoull(size);need(bytes>0&&bytes<=128*1024*1024,"candidate size bound");FilePin binary(fs::path(argv[1]),sha,bytes);
 const fs::path run=argv[4];struct stat absent{};need(run.is_absolute()&&run==run.lexically_normal()&&run.filename().string().starts_with("synthetic-cli-readonly-")&&fs::canonical(run.parent_path())==run.parent_path()&&::lstat(run.c_str(),&absent)<0&&errno==ENOENT,"absent synthetic root only");need(::mkdir(run.c_str(),0700)==0,"exclusive test root");rd::Directory root(run);Signals signals;
 const auto logs=run/"cli-evidence",scratch=run/"audit-scratch";need(::mkdir(logs.c_str(),0700)==0&&::mkdir(scratch.c_str(),0700)==0,"new logs and separate scratch");Cli cli(binary,root,logs,scratch);
 const Json document{{"config",{{"chainId",1337},{"consensus","pow"},{"evmFork","Shanghai"}}},{"difficulty","0x1"},{"alloc",Json::object()}};const auto genesis=Genesis::from_json(document);const auto gfile=run/"synthetic-genesis.json";const auto gbody=document.dump()+"\n";create_file(gfile,gbody);FilePin genesis_pin(gfile,digest(gbody),gbody.size());
 const auto db=run/"pow";need(::mkdir(db.c_str(),0700)==0,"new empty PoW directory");status_matches(success(cli.run("init-genesis",{"init",gfile.string(),db.string()})),status(genesis,genesis.block,genesis.state,1));
 {
  Fence source(db);readonly_case(source,[&]{status_matches(success(cli.run("status-genesis",{"status",gfile.string(),db.string()})),status(genesis,genesis.block,genesis.state,1));});readonly_case(source,[&]{audit_matches(success(cli.run("audit-genesis",{"audit",gfile.string(),db.string()})),genesis.block,genesis.state,1);need(fs::is_empty(scratch),"Genesis audit scratch removed");});
 }
 const std::string miner="0x00000000000000000000000000000000000000aa";auto block=Block::from_json(success(cli.run("mine-height-one",{"mine",gfile.string(),db.string(),miner,"24"})));State state=genesis.state;state.get(miner).balance+=Int("2000000000000000000");need(block.header.number==1&&block.header.timestamp==24&&block.header.parent_hash==genesis.block.header.hash()&&block.header.difficulty==1&&address(block.header.coinbase)==miner&&block.transactions.empty()&&block.uncles.empty()&&block.header.state_root==state.root()&&ProofOfWork::verify(block.header),"real empty height-one PoW/reward oracle");
 Fence source(db);readonly_case(source,[&]{ChainStore disk(db,ChainStore::OpenMode::ReadOnly);const auto persisted=disk.latest();const auto saved=disk.state_at(1);need(persisted&&saved&&persisted->to_json(true)==block.to_json(true)&&saved->to_json()==state.to_json()&&disk.count()==2&&disk.total_difficulty()==2,"independent persisted CLI output oracle");});
 readonly_case(source,[&]{status_matches(success(cli.run("status-height-one",{"status",gfile.string(),db.string()})),status(genesis,block,state,2));});readonly_case(source,[&]{audit_matches(success(cli.run("audit-height-one",{"audit",gfile.string(),db.string()})),block,state,2);need(fs::is_empty(scratch),"height-one audit scratch removed");});
 const auto missing=run/"absent-parent/db";rejection(cli.run("status-absent",{"status",gfile.string(),missing.string()}),"Nano: RocksDB: ",true);need(!fs::exists(missing.parent_path()),"status missing created no parent");rejection(cli.run("audit-absent",{"audit",gfile.string(),missing.string()}),"Nano: audit requires an existing RocksDB database\n");need(!fs::exists(missing.parent_path()),"audit missing created no parent");readonly_checks+=2;
 auto wrong=document;wrong["config"]["chainId"]=1338;const auto wrongfile=run/"synthetic-wrong-chain.json";const auto wrongbody=wrong.dump()+"\n";create_file(wrongfile,wrongbody);FilePin wrong_pin(wrongfile,digest(wrongbody),wrongbody.size());
 for(const char* verb:{"status","audit"})readonly_case(source,[&]{rejection(cli.run(std::string(verb)+"-wrong-chain",{verb,wrongfile.string(),db.string()}),"Nano: database belongs to another chain\n");});
 std::vector<std::unique_ptr<Fence>> adverse_sources;
 for(const bool schema:{false,true}){
  const std::string name=schema?"schema":"missing-tip";const auto bad=run/("synthetic-"+name);need(::mkdir(bad.c_str(),0700)==0,"new empty adverse directory");status_matches(success(cli.run("init-"+name,{"init",gfile.string(),bad.string()})),status(genesis,genesis.block,genesis.state,1));alter_new(bad,schema);adverse_sources.push_back(std::make_unique<Fence>(bad));
  for(const char* verb:{"status","audit"})readonly_case(*adverse_sources.back(),[&]{rejection(cli.run(std::string(verb)+"-"+name,{verb,gfile.string(),bad.string()}),schema?"Nano: unsupported Nano database schema\n":"Nano: read-only Blockchain database has no canonical tip\n");});
 }
 auto beacon_doc=document;beacon_doc["config"]["consensus"]="beacon";beacon_doc["difficulty"]="0x0";const auto beacon_file=run/"synthetic-beacon-genesis.json";const auto beacon_body=beacon_doc.dump()+"\n";create_file(beacon_file,beacon_body);FilePin beacon_pin(beacon_file,digest(beacon_body),beacon_body.size());const auto beacon_db=run/"synthetic-beacon";need(::mkdir(beacon_db.c_str(),0700)==0,"new empty Beacon fixture directory");{Blockchain writer(beacon_db,Genesis::from_json(beacon_doc));}Fence beacon_source(beacon_db);
 readonly_case(beacon_source,[&]{rejection(cli.run("status-beacon",{"status",beacon_file.string(),beacon_db.string()}),"Nano: Beacon execution unavailable until checkpoint verification\n");});readonly_case(beacon_source,[&]{rejection(cli.run("audit-beacon",{"audit",beacon_file.string(),beacon_db.string()}),"Nano: Beacon audit requires consensus archive replay, not legacy block replay\n");});
 source.check();for(const auto& fence:adverse_sources)fence->check();beacon_source.check();binary.check();genesis_pin.check();wrong_pin.check();beacon_pin.check();root.recheck();need(fs::is_empty(scratch)&&commands==18&&successes==8&&refusals==10&&readonly_checks==15,"closed actual CLI counts/scratch");checkpoint();signals.close();
 std::cout<<Json{{"scenario","real-cli-readonly"},{"result","PASS"},{"actualCliCommands",commands},{"successfulCliExits",successes},{"expectedCliRefusals",refusals},{"readonlySourceChecks",readonly_checks},{"sourceFull9ShaUnchanged",true},{"candidatePinHeldInRun",true},{"candidateHistoricalFd9Qualified",false},{"auditScratchSeparateAndRemoved",true},{"genuinePowBlocksCreated",1},{"metadataFixtureMutations",2},{"beaconFixtureWriterOpens",1},{"newSignaturesCreated",0},{"oldDataOpened",false},{"runtimeDutiesAuthorized",false},{"productionQualified",false},{"elapsedMs",std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-began).count()},{"peakRssKiB",rss()}}.dump()<<'\n';return 0;
 }catch(const std::exception& e){std::cerr<<"real CLI readonly probe refused: "<<e.what()<<'\n';return 1;}}
