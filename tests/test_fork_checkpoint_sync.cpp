#include "support/process.hpp"
#include "nano/storage.hpp"
extern "C" void nano_test_arm_sync_fault(const char*);
extern "C" unsigned nano_test_sync_failures();
using namespace test;
int main(int argc,char** argv){try{
 const Bytes genesis(32,7);constexpr size_t limit=1024*1024;
 if(argc==3&&std::string(argv[1])=="--fail"){
  const fs::path path=argv[2];ChainStore store(path);store.bind_chain(1337,genesis,"checkpoint-test");const auto first=store.commit_fork_checkpoint(1337,genesis,std::nullopt,"before",limit);
  fs::path wal;for(const auto& e:fs::directory_iterator(path))if(e.path().extension()==".log"&&(wal.empty()||e.path().filename()>wal.filename()))wal=e.path();check(!wal.empty(),"WAL missing");const auto before=fs::file_size(wal);
  nano_test_arm_sync_fault(wal.c_str());bool rejected=false;try{store.commit_fork_checkpoint(1337,genesis,first,"after",limit);}catch(const std::runtime_error&){rejected=true;}
  check(rejected&&nano_test_sync_failures()>0,"commit did not report injected sync error");
  bool read_blocked=false,retry_blocked=false;
  try{store.fork_checkpoint(1337,genesis,limit);}catch(const std::runtime_error&){read_blocked=true;}
  try{store.commit_fork_checkpoint(1337,genesis,first,"retry",limit);}catch(const std::runtime_error&){retry_blocked=true;}
  check(read_blocked&&retry_blocked,"uncertain checkpoint remained usable before reopen");
  std::cout<<Json{{"wal",wal.filename().string()},{"before",before},{"after",fs::file_size(wal)},{"failures",nano_test_sync_failures()}}.dump()<<std::endl;_exit(0);
 }
 unsigned recovered=0;
 for(const bool lose_suffix:{false,true}){
  Temp directory;const auto path=directory/"db";const auto failure=success(run({argv[0],"--fail",path.string()}));const auto name=failure.at("wal").get<std::string>();check(fs::path(name).filename()==name,"invalid WAL basename");const auto wal=path/name;
  const auto before=failure.at("before").get<uint64_t>(),after=failure.at("after").get<uint64_t>();check(after>before&&fs::file_size(wal)==after,"unobserved WAL append");
  if(lose_suffix){const int fd=open(wal.c_str(),O_WRONLY);check(fd>=0,"owned WAL open");const bool ok=ftruncate(fd,before)==0&&fsync(fd)==0;close(fd);check(ok,"unsynced suffix loss simulation failed");}
  ChainStore store(path);store.bind_chain(1337,genesis,"checkpoint-test");const auto record=store.fork_checkpoint(1337,genesis,limit);check(bool(record),"previous durable checkpoint lost");
  check(record->token.sequence==(lose_suffix?1:2)&&record->snapshot==(lose_suffix?"before":"after"),"mixed payload/token after sync failure");
  const auto next=store.commit_fork_checkpoint(1337,genesis,record->token,"resumed",limit);check(next.sequence==record->token.sequence+1&&store.fork_checkpoint(1337,genesis,limit,next)->snapshot=="resumed","cannot continue after uncertainty resolved");++recovered;
 }
 std::cout<<Json{{"scenario","fork-checkpoint-fsync-error"},{"recoveredDispositions",recovered},{"retainedAndLostSuffix",true},{"uncertainHandleBlocked",true},{"platform","macOS"},{"physicalPowerLossQualified",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
