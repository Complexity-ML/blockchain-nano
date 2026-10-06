#include "support/process.hpp"
#include "nano/storage.hpp"
#include <rocksdb/db.h>
#include <barrier>
#include <atomic>
#ifdef NANO_SYNC_FAULT
extern "C" void nano_test_arm_sync_fault(const char*);
extern "C" unsigned nano_test_sync_failures();
#endif
using namespace test;
namespace {
constexpr size_t budget=1024*1024;
const Bytes genesis_id(32,7);
CheckpointBlob fragment(const std::string& payload){return {sha256(Bytes(payload.begin(),payload.end())),payload};}
std::string manifest(const CheckpointBlob& blob){return Json{{"blob",hex(blob.digest)}}.dump();}
template<class Action>void refused(Action&& action,const std::string& reason){bool rejected=false;try{action();}catch(const std::exception&){rejected=true;}check(rejected,reason);}
std::map<std::string,std::string> file_hashes(const fs::path& path){std::map<std::string,std::string> result;for(const auto& file:fs::recursive_directory_iterator(path))if(file.is_regular_file()){const auto data=read(file.path());result.emplace(fs::relative(file.path(),path).string(),hex(sha256(Bytes(data.begin(),data.end()))));}return result;}
}
int main(int argc,char** argv){try{
 const auto a=fragment(std::string("immutable\0first",15)),b=fragment("immutable-second"),c=fragment("uncommitted-fragment");
#ifdef NANO_SYNC_FAULT
 if(argc==3&&std::string(argv[1])=="--sync-writer"){
  const fs::path path=argv[2];ChainStore disk(path);disk.bind_chain(1337,genesis_id,"blob-test");
  const auto token=disk.commit_fork_checkpoint_with_blobs(1337,genesis_id,{},manifest(a),budget,{a},1);
  fs::path wal;for(const auto& entry:fs::directory_iterator(path))if(entry.path().extension()==".log"&&(wal.empty()||entry.path().filename()>wal.filename()))wal=entry.path();check(!wal.empty(),"missing WAL");const auto before=fs::file_size(wal);
  nano_test_arm_sync_fault(wal.c_str());refused([&]{disk.commit_fork_checkpoint_with_blobs(1337,genesis_id,token,manifest(b),budget,{b},1);},"fsync failure acknowledged");check(nano_test_sync_failures()>0,"fault did not hit WAL sync");
  refused([&]{disk.checkpoint_blob(1337,genesis_id,a.digest,budget);},"uncertain fragment read allowed");refused([&]{disk.fork_checkpoint(1337,genesis_id,budget);},"uncertain checkpoint read allowed");refused([&]{disk.commit_fork_checkpoint_with_blobs(1337,genesis_id,token,"retry",budget,{c},1);},"uncertain retry allowed");
  std::cout<<Json{{"wal",wal.filename().string()},{"before",before},{"after",fs::file_size(wal)}}.dump()<<std::endl;_exit(0);
 }
 if(argc==2&&std::string(argv[1])=="--sync"){
  for(const bool lose:{false,true}){Temp tmp;const auto path=tmp/"db";const auto observation=success(run({argv[0],"--sync-writer",path.string()}));const auto name=observation.at("wal").get<std::string>();check(fs::path(name).filename()==name,"WAL basename");const auto before=observation.at("before").get<uint64_t>(),after=observation.at("after").get<uint64_t>();check(after>before,"fault did not append");
   if(lose){const int fd=open((path/name).c_str(),O_WRONLY);check(fd>=0,"owned WAL open");const bool ok=ftruncate(fd,before)==0&&fsync(fd)==0;close(fd);check(ok,"owned WAL suffix loss");}
   ChainStore disk(path);const auto stored=disk.fork_checkpoint(1337,genesis_id,budget);check(stored&&stored->snapshot==manifest(lose?a:b)&&stored->token.sequence==(lose?1:2),"mixed checkpoint disposition");check(disk.checkpoint_blob(1337,genesis_id,a.digest,budget)==a.payload,"previous immutable fragment lost");check(disk.checkpoint_blob(1337,genesis_id,b.digest,budget)==(lose?std::optional<std::string>{}:std::optional<std::string>{b.payload}),"fragment/checkpoint torn batch");
   const auto resumed=disk.commit_fork_checkpoint_with_blobs(1337,genesis_id,stored->token,manifest(c),budget,{c},1);check(resumed.sequence==stored->token.sequence+1&&disk.checkpoint_blob(1337,genesis_id,c.digest,budget)==c.payload,"resume failed");
  }
  std::cout<<Json{{"scenario","checkpoint-blob-sync-failure"},{"reopenedDispositions",2},{"fragmentAndTokenAtomic",true},{"uncertainReadsAndRetriesBlocked",true},{"physicalPowerLossQualified",false},{"productionQualified",false}}.dump()<<'\n';return 0;
 }
#endif
 Temp tmp;const auto path=tmp/"db";ForkCheckpointToken token;
 {
  ChainStore disk(path);refused([&]{disk.commit_fork_checkpoint_with_blobs(1337,genesis_id,{},manifest(a),budget,{a},1);},"unbound publication allowed");disk.bind_chain(1337,genesis_id,"blob-test");
  token=disk.commit_fork_checkpoint_with_blobs(1337,genesis_id,{},manifest(a),budget,{a,a},2);check(disk.checkpoint_blob(1337,genesis_id,a.digest,budget)==a.payload,"binary fragment changed");
  auto invalid=c;invalid.digest[0]^=1;
  refused([&]{disk.commit_fork_checkpoint_with_blobs(1337,genesis_id,token,manifest(c),budget,{c,invalid},2);},"wrong digest accepted");check(!disk.checkpoint_blob(1337,genesis_id,c.digest,budget),"partial fragment published after rejection");
  refused([&]{disk.commit_fork_checkpoint_with_blobs(1337,genesis_id,token,manifest(c),budget,{c},0);},"record bound ignored");
  refused([&]{disk.commit_fork_checkpoint_with_blobs(1337,genesis_id,token,manifest(c),c.payload.size()-1,{c},1);},"byte bound ignored");
  const auto large=fragment(std::string(2048,'b'));refused([&]{disk.commit_fork_checkpoint_with_blobs(1337,genesis_id,token,"budget",large.payload.size()+1,{large},1);},"aggregate batch bound ignored");check(!disk.checkpoint_blob(1337,genesis_id,large.digest,budget),"oversized batch persisted fragment");
  refused([&]{disk.checkpoint_blob(1338,genesis_id,a.digest,budget);},"wrong chain read allowed");refused([&]{disk.checkpoint_blob(1337,Bytes(32,8),a.digest,budget);},"wrong genesis read allowed");refused([&]{disk.checkpoint_blob(1337,genesis_id,a.digest,a.payload.size()-1);},"oversized fragment read allowed");
  std::barrier start(3);std::atomic<unsigned> wins=0,stale=0;std::exception_ptr failure;std::mutex gate;
  auto writer=[&](const CheckpointBlob& blob){start.arrive_and_wait();try{disk.commit_fork_checkpoint_with_blobs(1337,genesis_id,token,manifest(blob),budget,{blob},1);++wins;}catch(const std::invalid_argument&){++stale;}catch(...){std::lock_guard lock(gate);failure=std::current_exception();}};
  std::thread first(writer,std::cref(b)),second(writer,std::cref(c));start.arrive_and_wait();first.join();second.join();if(failure)std::rethrow_exception(failure);check(wins==1&&stale==1,"fragment CAS race not serialized");
  const auto stored=disk.fork_checkpoint(1337,genesis_id,budget);token=stored->token;const bool b_won=stored->snapshot==manifest(b);check(disk.checkpoint_blob(1337,genesis_id,b.digest,budget).has_value()==b_won&&disk.checkpoint_blob(1337,genesis_id,c.digest,budget).has_value()!=b_won,"loser fragment published");
  token=disk.commit_fork_checkpoint_with_blobs(1337,genesis_id,token,manifest(a),budget,{a},1);check(disk.checkpoint_blob(1337,genesis_id,a.digest,budget)==a.payload,"duplicate immutable content changed");
  token=disk.commit_fork_checkpoint(1337,genesis_id,token,"legacy",budget);check(disk.fork_checkpoint(1337,genesis_id,budget,token)->snapshot=="legacy","legacy publication regressed");
 }
 const auto before=file_hashes(path);const auto copy=tmp/"copy";fs::copy(path,copy,fs::copy_options::recursive);
 {ChainStore disk(copy,ChainStore::OpenMode::ReadOnly);check(disk.fork_checkpoint(1337,genesis_id,budget,token).has_value()&&disk.checkpoint_blob(1337,genesis_id,a.digest,budget)==a.payload,"readonly restore failed");}
 check(file_hashes(copy)==before&&file_hashes(path)==before,"readonly restore changed data");
 {
  rocksdb::Options options;options.create_if_missing=false;rocksdb::DB* raw=nullptr;check(rocksdb::DB::Open(options,copy.string(),&raw).ok(),"owned corrupt copy open");std::unique_ptr<rocksdb::DB> db(raw);rocksdb::WriteOptions write;write.sync=true;check(db->Put(write,"checkpoint-blob/v1/"+hex(a.digest),"tampered").ok(),"owned tampering failed");db.reset();
  ChainStore disk(copy);refused([&]{disk.checkpoint_blob(1337,genesis_id,a.digest,budget);},"corrupt digest read accepted");refused([&]{disk.commit_fork_checkpoint_with_blobs(1337,genesis_id,token,manifest(a),budget,{a},1);},"corrupt immutable fragment replaced");check(disk.fork_checkpoint(1337,genesis_id,budget,token)->snapshot=="legacy","corruption rejection changed token");
 }
 {
  const auto execution_path=tmp/"execution";State state;Block genesis;genesis.header.state_root=state.root();const auto hash=genesis.header.hash();ChainStore disk(execution_path);disk.bind_chain(1337,hash,"blob-execution-test");disk.commit(genesis,state,{});
  refused([&]{disk.commit_execution_checkpoint_with_blobs(1337,hash,{},Bytes(32,1),0,hash,{},manifest(a),budget,0,{a},1);},"wrong execution CAS accepted");check(!disk.checkpoint_blob(1337,hash,a.digest,budget),"failed execution CAS leaked fragment");
  const auto first=disk.commit_execution_checkpoint_with_blobs(1337,hash,{},hash,0,hash,{},manifest(a),budget,0,{a},1);check(disk.fork_checkpoint(1337,hash,budget,first)->snapshot==manifest(a)&&disk.checkpoint_blob(1337,hash,a.digest,budget)==a.payload&&disk.latest()->header.hash()==hash,"execution fragment atomic binding failed");
  const auto legacy=disk.commit_execution_checkpoint(1337,hash,first,hash,0,hash,{},"legacy-execution",budget,0);check(disk.fork_checkpoint(1337,hash,budget,legacy)->snapshot=="legacy-execution"&&disk.checkpoint_blob(1337,hash,a.digest,budget)==a.payload,"legacy execution API regressed");
 }
 std::cout<<Json{{"scenario","checkpoint-immutable-blobs"},{"binaryFragments",true},{"duplicateIdempotence",true},{"digestAndAggregateBounds",true},{"concurrentCasLoserAbsent",true},{"readonlyCopyUnchanged",true},{"tamperingRejectedWithoutOverwrite",true},{"executionCheckpointCoupling",true},{"legacyApisPreserved",true},{"runtimeGraphPagingImplemented",false},{"productionQualified",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
