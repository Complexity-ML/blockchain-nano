#include "support/process.hpp"
#include "nano/storage.hpp"
#include <atomic>
#include <barrier>
using namespace test;
int main(int argc,char** argv){try{
 const Int chain_id=1337;const Bytes genesis(32,7);constexpr size_t limit=8*1024*1024;
 if(argc==3&&std::string(argv[1])=="--writer"){
  ChainStore store(argv[2]);store.bind_chain(chain_id,genesis,"checkpoint-test");auto record=store.fork_checkpoint(chain_id,genesis,limit);check(bool(record),"missing initial checkpoint");
  auto token=record->token;std::string payload(4*1024*1024,'x');
  std::cout<<"ready"<<std::endl;
  for(size_t i=0;i<128;++i){payload[0]=char('a'+i%26);token=store.commit_fork_checkpoint(chain_id,genesis,token,payload,limit);std::cout<<token.sequence<<std::endl;}
  _exit(0);
 }
 Temp location;const auto db=location/"live",backup=location/"backup";ForkCheckpointToken first{},latest{};unsigned rejections=0;
 auto refuse=[&](auto action){bool bad=false;try{action();}catch(const std::exception&){bad=true;}check(bad,"unsafe checkpoint operation accepted");++rejections;};
 {
  ChainStore store(db);refuse([&]{store.commit_fork_checkpoint(chain_id,genesis,std::nullopt,"before-binding",limit);});
  store.bind_chain(chain_id,genesis,"checkpoint-test");check(!store.fork_checkpoint(chain_id,genesis,limit),"new checkpoint exists");
  first=store.commit_fork_checkpoint(chain_id,genesis,std::nullopt,"first",limit);check(first.sequence==1,"initial sequence");
  refuse([&]{store.commit_fork_checkpoint(chain_id,genesis,std::nullopt,"stale",limit);});
  refuse([&]{store.commit_fork_checkpoint(chain_id,genesis,first,"",limit);});
  refuse([&]{store.commit_fork_checkpoint(chain_id,genesis,first,"too-big",2);});
  refuse([&]{store.fork_checkpoint(99,genesis,limit);});
  auto wrong=first;wrong.digest[0]^=1;refuse([&]{store.commit_fork_checkpoint(chain_id,genesis,wrong,"forged",limit);});
  check(store.fork_checkpoint(chain_id,genesis,limit,first)->snapshot=="first","rejection changed committed payload");
 }
 fs::copy(db,backup,fs::copy_options::recursive);
 {
  ChainStore store(db);std::barrier start(3);std::atomic<unsigned> wins=0,stale=0;std::exception_ptr unexpected;std::mutex errors;
  auto writer=[&](const std::string& payload){start.arrive_and_wait();try{store.commit_fork_checkpoint(chain_id,genesis,first,payload,limit);++wins;}catch(const std::invalid_argument&){++stale;}catch(...){std::lock_guard lock(errors);unexpected=std::current_exception();}};
  std::thread a(writer,"writer-a"),b(writer,"writer-b");start.arrive_and_wait();a.join();b.join();if(unexpected)std::rethrow_exception(unexpected);
  check(wins==1&&stale==1,"CAS did not serialize concurrent writers");auto record=store.fork_checkpoint(chain_id,genesis,limit);latest=record->token;
  check(latest.sequence==2&&(record->snapshot=="writer-a"||record->snapshot=="writer-b"),"torn concurrent checkpoint");
  refuse([&]{store.fork_checkpoint(chain_id,genesis,limit,first);});
 }
 {
  ChainStore rolled_back(backup);refuse([&]{rolled_back.fork_checkpoint(chain_id,genesis,limit,latest);});
  check(rolled_back.fork_checkpoint(chain_id,genesis,limit)->token==first,"backup fixture wrong");
 }
 unsigned killed=0;
 for(unsigned attempt=0;attempt<6;++attempt){
  Process writer({argv[0],"--writer",db.string()});writer.line([](const auto& s){return s=="ready";});
  if(attempt%2)writer.line([](const auto& s){return !s.empty()&&s[0]>='0'&&s[0]<='9';});
  writer.signal(SIGKILL);const auto stopped=writer.wait(10);check(stopped.code==-SIGKILL,"writer was not killed");++killed;
  uint64_t acknowledged=latest.sequence;std::istringstream output(stopped.out);std::string line;while(std::getline(output,line))if(!line.empty()&&line[0]>='0'&&line[0]<='9')acknowledged=std::max(acknowledged,uint64_t(std::stoull(line)));
  ChainStore store(db);const auto record=store.fork_checkpoint(chain_id,genesis,limit);check(record&&record->token.sequence>=acknowledged,"lost acknowledged checkpoint");
  check(record->token.digest==sha256(Bytes(record->snapshot.begin(),record->snapshot.end())),"torn checkpoint after kill");
  if(record->token.sequence==latest.sequence)check(record->token==latest,"same sequence different digest");
  else check(record->snapshot.size()==4*1024*1024&&record->snapshot.substr(1)==std::string(4*1024*1024-1,'x'),"partial replacement payload");
  latest=record->token;
 }
 std::cout<<Json{{"scenario","sequenced-fork-checkpoint"},{"rejections",rejections},{"concurrentWinners",1},{"staleWriters",1},{"sigkillReopens",killed},{"powerLossQualified",false},{"externalCommitmentRequiredForDiskRollback",true},{"executionAtomicBinding",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
