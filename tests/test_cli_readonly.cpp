#include "nano/chain.hpp"
#include "../tools/storage-maintenance/bootstrap-readonly-replica.hpp"
#include <rocksdb/db.h>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace nano;
namespace fs=std::filesystem;
namespace rd=nano::maintenance::readonly_replica_detail;
namespace {
const auto began=std::chrono::steady_clock::now();
size_t checks=0,refusals=0,groups=0,write_refusals=0,readonly_opens=0;
void require(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
uint64_t rss(){rusage r{};require(::getrusage(RUSAGE_SELF,&r)==0,"resource observation");
#ifdef __APPLE__
 return uint64_t(r.ru_maxrss)/1024;
#else
 return uint64_t(r.ru_maxrss);
#endif
}
void checkpoint(){require(std::chrono::steady_clock::now()-began<std::chrono::seconds(120)&&rss()<=131072,"readonly test resource budget");}
struct Temp{
 fs::path path;
 Temp(){auto pattern=(fs::temp_directory_path()/"nano-cli-readonly-XXXXXX").string();require(::mkdtemp(pattern.data())!=nullptr,"owned test root");path=fs::canonical(pattern);}
 ~Temp(){std::error_code ignored;fs::remove_all(path,ignored);}
};
void directory(const fs::path& p){require(::mkdir(p.c_str(),0700)==0,"new owned database directory");}
struct Fence{
 rd::Directory directory;std::vector<rd::Entry> entries;
 explicit Fence(const fs::path& p):directory(p),entries(rd::capture(directory.fd(),4096,32*1024*1024,checkpoint)){}
 void check()const{directory.recheck();require(rd::same_entries(entries,rd::capture(directory.fd(),4096,32*1024*1024,checkpoint)),"source full9 and SHA unchanged");directory.recheck();}
};
template<class F>void unchanged(Fence& fence,F action){
 fence.check();std::exception_ptr first;
 try{action();}catch(...){first=std::current_exception();}
 try{fence.check();}catch(...){if(first){std::cerr<<"source fence also failed after the initial test error\n";std::rethrow_exception(first);}throw;}
 if(first)std::rethrow_exception(first);
}
template<class E,class F>void refuses(F action,const std::string& message,bool prefix=false){
 bool rejected=false;
 try{action();}catch(const E& e){const std::string actual=e.what();require(prefix?actual.starts_with(message):actual==message,"expected refusal class/message");rejected=true;}
 require(rejected,"expected readonly refusal");++refusals;
}
template<class Database=rocksdb::DB>std::unique_ptr<Database> open_owned(const fs::path& p){
 rocksdb::Options options;options.create_if_missing=false;std::unique_ptr<Database> db;
 if constexpr(requires{Database::Open(options,p.string(),&db);})require(Database::Open(options,p.string(),&db).ok(),"owned adverse database open");
 else{Database* raw=nullptr;const auto status=Database::Open(options,p.string(),&raw);db.reset(raw);require(status.ok(),"owned adverse database open");}
 return db;
}
void erase_owned(const fs::path& p,const char* key){auto db=open_owned(p);rocksdb::WriteOptions options;options.sync=true;require(db->Delete(options,key).ok(),"owned missing-metadata fixture");}
struct Scratch{
 fs::path path;std::optional<std::string> previous;bool closed=false;
 explicit Scratch(const fs::path& p):path(p){directory(path);if(const char* value=::getenv("TMPDIR"))previous=value;require(::setenv("TMPDIR",path.c_str(),1)==0,"private scratch environment");require(fs::canonical(fs::temp_directory_path())==path,"audit scratch parent is separate");}
 void close(){const int status=previous ? ::setenv("TMPDIR",previous->c_str(),1) : ::unsetenv("TMPDIR");require(status==0,"restore scratch environment");closed=true;require(fs::is_empty(path),"audit removed its separate scratch database");}
 ~Scratch(){if(!closed){if(previous)(void)::setenv("TMPDIR",previous->c_str(),1);else(void)::unsetenv("TMPDIR");}}
};
Json view(const Blockchain& chain){return {{"tip",chain.tip().to_json(true)},{"state",chain.state().to_json()},{"count",chain.store().count()},{"difficulty",quantity(chain.store().total_difficulty())},{"pending",chain.pending_count()},{"cursor",chain.event_cursor()}};}
void legacy(const fs::path& root,const Genesis& genesis){
 const auto db=root/"legacy";directory(db);
 {Blockchain writer(db,genesis);require(writer.tip().header.number==0,"genuine initial Genesis");}
 {
  Fence source(db);Scratch scratch(root/"audit-scratch-genesis");
  unchanged(source,[&]{Blockchain reader(db,genesis,ChainStore::OpenMode::ReadOnly);++readonly_opens;require(reader.tip().to_json(true)==genesis.block.to_json(true)&&reader.state().to_json()==genesis.state.to_json(),"RO Genesis state/tip");const auto audit=reader.audit();require(audit.at("verified")==true&&audit.at("blocks")==1&&audit.at("transactions")=="0x0"&&audit.at("blockHash")==hex(genesis.block.header.hash())&&audit.at("stateRoot")==hex(genesis.state.root()),"RO Genesis replay oracle");});
  scratch.close();source.check();++groups;
 }
 Block first;State expected;Int difficulty;
 // The first fence has ended. This explicit existing-owner RW operation creates
 // the real height-one block; no admitted source is refreshed around it.
 {Blockchain writer(db,genesis,ChainStore::OpenMode::ReadWriteExisting);first=writer.mine("0x00000000000000000000000000000000000000aa",24,std::chrono::seconds(2));expected=writer.state();difficulty=writer.store().total_difficulty();require(first.header.number==1&&first.header.parent_hash==genesis.block.header.hash()&&ProofOfWork::verify(first.header),"genuine height-one PoW block");}
 Fence source(db);
 unchanged(source,[&]{Blockchain reader(db,genesis,ChainStore::OpenMode::ReadOnly);++readonly_opens;require(reader.tip().to_json(true)==first.to_json(true)&&reader.state().to_json()==expected.to_json()&&reader.store().total_difficulty()==difficulty,"RO persisted height-one state/tip");const auto before=view(reader);refuses<std::runtime_error>([&]{(void)reader.restore(0,genesis.block.header.hash());},"RocksDB:",true);++write_refusals;require(view(reader)==before,"failed restore preserves in-memory state/tip/pool/events and metadata");});
 unchanged(source,[&]{ChainStore reader(db,ChainStore::OpenMode::ReadOnly);++readonly_opens;const auto before=reader.latest();require(before&&before->header.number==1,"rewind is a real height-one mutation");refuses<std::runtime_error>([&]{reader.rewind(0);},"RocksDB:",true);++write_refusals;const auto after=reader.latest();const auto state=reader.state_at(1);require(after&&state&&after->to_json(true)==before->to_json(true)&&state->to_json()==expected.to_json()&&reader.count()==2&&reader.total_difficulty()==difficulty,"failed store rewind preserves canonical state and metadata");});
 {
  Scratch scratch(root/"audit-scratch-height-one");
  unchanged(source,[&]{Blockchain reader(db,genesis,ChainStore::OpenMode::ReadOnly);++readonly_opens;const auto before=view(reader),audit=reader.audit();require(audit.at("verified")==true&&audit.at("blocks")==2&&audit.at("transactions")=="0x0"&&audit.at("blockHash")==hex(first.header.hash())&&audit.at("stateRoot")==hex(expected.root())&&view(reader)==before,"RO height-one full replay oracle");});
  scratch.close();source.check();
 }
 ++groups;
 auto wrong=genesis;wrong.chain_id+=1;
 unchanged(source,[&]{refuses<std::invalid_argument>([&]{Blockchain reader(db,wrong,ChainStore::OpenMode::ReadOnly);},"database belongs to another chain");});++groups;
}
void absent_and_invalid(const fs::path& root,const Genesis& genesis){
 const auto parent=root/"absent",db=parent/"db";require(!fs::exists(parent),"absent-parent precondition");
 refuses<std::runtime_error>([&]{Blockchain reader(db,genesis,ChainStore::OpenMode::ReadOnly);},"RocksDB:",true);require(!fs::exists(parent),"RO missing open creates no parent");
 refuses<std::runtime_error>([&]{Blockchain reader(db,genesis,ChainStore::OpenMode::ReadWriteExisting);},"RocksDB:",true);require(!fs::exists(parent),"Existing missing open creates no parent");
 refuses<std::invalid_argument>([&]{Blockchain reader(db,genesis,static_cast<ChainStore::OpenMode>(999));},"Blockchain owner requires ReadWrite, ReadWriteExisting or ReadOnly");require(!fs::exists(parent),"invalid enum refused before database creation");++groups;
}
void missing_metadata(const fs::path& root,const Genesis& genesis){
 for(const char* key:{"meta/identity","meta/tip"}){
  const auto db=root/(std::string(key)=="meta/tip"?"missing-tip":"missing-identity");directory(db);{Blockchain writer(db,genesis);}erase_owned(db,key);
  Fence source(db);
  unchanged(source,[&]{if(std::string(key)=="meta/tip")refuses<std::runtime_error>([&]{Blockchain reader(db,genesis,ChainStore::OpenMode::ReadOnly);},"read-only Blockchain database has no canonical tip");else refuses<std::invalid_argument>([&]{Blockchain reader(db,genesis,ChainStore::OpenMode::ReadOnly);},"existing database has no chain identity");});
 }
 ++groups;
}
void beacon(const fs::path& root){
 const auto genesis=Genesis::from_json({{"config",{{"chainId",1337},{"consensus","beacon"},{"evmFork","Shanghai"}}},{"alloc",Json::object()}});const auto db=root/"beacon-genesis";directory(db);{Blockchain writer(db,genesis);}
 Fence source(db);
 unchanged(source,[&]{Blockchain reader(db,genesis,ChainStore::OpenMode::ReadOnly);++readonly_opens;const auto persisted=reader.store().latest();require(persisted&&persisted->to_json(true)==genesis.block.to_json(true),"RO Beacon Genesis storage is readable without readiness");refuses<std::runtime_error>([&]{(void)reader.tip();},"Beacon execution unavailable until checkpoint verification");refuses<std::runtime_error>([&]{(void)reader.state();},"Beacon execution unavailable until checkpoint verification");refuses<std::invalid_argument>([&]{(void)reader.audit();},"Beacon audit requires consensus archive replay, not legacy block replay");});++groups;
}
} // namespace
int main(int argc,char** argv){try{
 require(argc==2&&std::string(argv[1])=="--self-test","usage: nano-cli-readonly-tests --self-test");Temp root;
 const auto genesis=Genesis::from_json({{"config",{{"chainId",1337},{"consensus","pow"},{"evmFork","Shanghai"}}},{"difficulty","0x1"},{"alloc",Json::object()}});
 legacy(root.path,genesis);absent_and_invalid(root.path,genesis);missing_metadata(root.path,genesis);beacon(root.path);checkpoint();
 require(groups==6&&refusals==11&&write_refusals==2&&readonly_opens==5,"closed readonly scenario counts");
 std::cout<<Json{{"scenario","cli-readonly-core"},{"result","PASS"},{"groups",groups},{"expectedRefusals",refusals},{"actualWriteRefusals",write_refusals},{"positiveReadonlyOpens",readonly_opens},{"genuinePowBlocksCreated",1},{"sourceFull9ShaUnchanged",true},{"inMemoryStatePreservedAfterWriteRefusal",true},{"auditScratchSeparateAndRemoved",true},{"beaconReadinessBypassed",false},{"realCliInvocations",0},{"newSignaturesCreated",0},{"runtimeDutiesAuthorized",false},{"productionQualified",false},{"elapsedMs",std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-began).count()},{"peakRssKiB",rss()}}.dump()<<'\n';return 0;
 }catch(const std::exception& e){std::cerr<<"cli-readonly test failed after "<<checks<<" checks: "<<e.what()<<'\n';return 1;}}
