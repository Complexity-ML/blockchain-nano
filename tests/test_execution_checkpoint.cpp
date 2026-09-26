#include "support/process.hpp"
#include "nano/storage.hpp"
#include <atomic>
#include <barrier>
#ifdef NANO_SYNC_FAULT
extern "C" void nano_test_arm_sync_fault(const char*);
extern "C" unsigned nano_test_sync_failures();
#endif
using namespace test;
namespace {
constexpr size_t budget=8*1024*1024;
struct Fixture {
 Bytes key=bytes(Int(777),32);std::string recipient="0x0000000000000000000000000000000000001234";
 Block genesis;State state;ExecutionCheckpointBlock a,b,c;
 Fixture(){Transaction tx;tx.to=recipient;tx.gas=21000;tx.gas_price=2000000000;const auto sender=SignedTransaction::sign(tx,key).sender();state.get(sender).balance=Int("1000000000000000000");genesis.header.difficulty=0;genesis.header.state_root=state.root();
  auto make=[&](const Block& parent,const State& before,unsigned value,unsigned nonce){ExecutionCheckpointBlock out;out.state=before;auto& h=out.block.header;h.number=parent.header.number+1;h.parent_hash=parent.header.hash();h.difficulty=0;h.timestamp=parent.header.timestamp+12;h.base_fee=next_base_fee(parent.header);auto transfer=tx;transfer.value=value;transfer.nonce=nonce;out.block.transactions.push_back(SignedTransaction::sign(transfer,key));out.receipts=ExecutionEngine(1337,{},EvmRevision::Paris,ReplayPolicy::RequireChainId).execute_block(out.state,out.block,false);return out;};
  a=make(genesis,state,3,0);b=make(genesis,state,7,0);c=make(a.block,a.state,5,1);
 }
 void init(ChainStore& disk){disk.bind_chain(1337,genesis.header.hash(),"bellatrix-storage-test");disk.commit(genesis,state,{});}
 std::string snapshot(const ExecutionCheckpointBlock& x)const{return Json{{"head",hex(x.block.header.hash())},{"state",hex(x.state.root())},{"padding",std::string(1024*1024,'x')}}.dump();}
 CheckpointArchiveEntry archived(const ExecutionCheckpointBlock& x)const{const auto text=x.block.to_json(true).dump();return {x.block.header.hash(),u64(x.block.header.number),Bytes(text.begin(),text.end())};}
 ForkCheckpointToken save(ChainStore& disk,const std::optional<ForkCheckpointToken>& token,const Bytes& tip,const std::vector<ExecutionCheckpointBlock>& suffix){std::vector<CheckpointArchiveEntry> records;for(const auto& x:suffix)records.push_back(archived(x));return disk.commit_execution_checkpoint(1337,genesis.header.hash(),token,tip,0,genesis.header.hash(),suffix,snapshot(suffix.back()),budget,8,records);}
 ForkCheckpointRecord verify(ChainStore& disk){auto record=disk.fork_checkpoint(1337,genesis.header.hash(),budget);check(bool(record),"checkpoint missing");const auto data=Json::parse(record->snapshot);const auto head=disk.latest();check(head&&hex(head->header.hash())==data.at("head").get<std::string>()&&hex(disk.state_at(head->header.number)->root())==data.at("state").get<std::string>(),"mixed execution/checkpoint recovered");const bool branch_b=head->header.hash()==b.block.header.hash();check(head->header.hash()==a.block.header.hash()||branch_b,"unexpected recovered branch");const auto& yes=branch_b?b:a;const auto& no=branch_b?a:b;const auto stored=disk.checkpoint_archive(head->header.hash(),budget);check(stored&&stored->payload==archived(yes).payload&&stored->slot==u64(head->header.number),"archive/checkpoint mixed after recovery");check(disk.receipt(yes.block.transactions[0].hash())->block_hash==head->header.hash()&&!disk.receipt(no.block.transactions[0].hash())&&!disk.transaction(no.block.transactions[0].hash())&&!disk.block_by_hash(no.block.header.hash()),"mixed canonical indexes");check(disk.count()==2&&!disk.block_by_number(2)&&!disk.receipt(c.block.transactions[0].hash()),"old suffix survived reorg");return *record;}
};
}
int main(int argc,char** argv){try{
 Fixture f;
 if(argc==3){ChainStore disk(argv[2]);
#ifdef NANO_SYNC_FAULT
  if(std::string(argv[1])=="--fail"){
   f.init(disk);const auto token=f.save(disk,{},f.genesis.header.hash(),{f.a});fs::path wal;for(const auto& e:fs::directory_iterator(argv[2]))if(e.path().extension()==".log"&&(wal.empty()||e.path().filename()>wal.filename()))wal=e.path();check(!wal.empty(),"WAL missing");const auto before=fs::file_size(wal);nano_test_arm_sync_fault(wal.c_str());bool failed=false;try{f.save(disk,token,f.a.block.header.hash(),{f.b});}catch(const std::runtime_error&){failed=true;}check(failed&&nano_test_sync_failures(),"sync fault not observed");unsigned blocked=0;try{disk.latest();}catch(const std::runtime_error&){++blocked;}try{disk.fork_checkpoint(1337,f.genesis.header.hash(),budget);}catch(const std::runtime_error&){++blocked;}try{f.save(disk,token,f.a.block.header.hash(),{f.b});}catch(const std::runtime_error&){++blocked;}check(blocked==3,"uncertain disk remained usable");std::cout<<Json{{"wal",wal.filename().string()},{"before",before},{"after",fs::file_size(wal)}}.dump()<<std::endl;_exit(0);
  }
#endif
  auto record=f.verify(disk);std::cout<<"ready"<<std::endl;
  for(unsigned i=0;i<128;++i){const auto tip=disk.latest()->header.hash();const auto& next=tip==f.a.block.header.hash()?f.b:f.a;record.token=f.save(disk,record.token,tip,{next});std::cout<<record.token.sequence<<std::endl;}_exit(0);
 }
#ifdef NANO_SYNC_FAULT
 for(bool lose:{false,true}){Temp tmp;auto path=tmp/"db";const auto result=success(run({argv[0],"--fail",path.string()}));auto wal=path/result.at("wal").get<std::string>();check(fs::file_size(wal)==result.at("after").get<uint64_t>()&&result.at("after")>result.at("before"),"WAL suffix missing");if(lose){int fd=open(wal.c_str(),O_WRONLY);check(fd>=0,"open owned WAL");bool ok=ftruncate(fd,result.at("before").get<uint64_t>())==0&&fsync(fd)==0;close(fd);check(ok,"truncate owned WAL");}ChainStore disk(path);auto record=f.verify(disk);check(record.token.sequence==(lose?1:2)&&disk.latest()->header.hash()==(lose?f.a:f.b).block.header.hash(),"wrong recovery disposition");check(bool(disk.checkpoint_archive(f.b.block.header.hash(),budget))==!lose,"archive detached from retained/lost WAL suffix");f.save(disk,record.token,disk.latest()->header.hash(),{f.a});f.verify(disk);}
 std::cout<<Json{{"scenario","atomic-execution-checkpoint-sync"},{"retainedAndLostSuffix",true},{"physicalPowerLoss",false}}.dump()<<'\n';
#else
 Temp tmp;const auto path=tmp/"db";ForkCheckpointToken token{};unsigned rejects=0;
 {ChainStore disk(path);f.init(disk);token=f.save(disk,{},f.genesis.header.hash(),{f.a,f.c});check(disk.count()==3&&disk.latest()->header.number==2,"initial suffix missing");token=f.save(disk,token,f.c.block.header.hash(),{f.b});f.verify(disk);
  token=disk.commit_execution_checkpoint(1337,f.genesis.header.hash(),token,f.b.block.header.hash(),1,f.b.block.header.hash(),{},f.snapshot(f.b),budget,0);f.verify(disk);
  check(disk.checkpoint_archive(f.a.block.header.hash(),budget)->payload==f.archived(f.a).payload&&disk.checkpoint_archive(f.c.block.header.hash(),budget)->payload==f.archived(f.c).payload,"reorg erased historical records");
  auto reject=[&](auto action){bool bad=false;try{action();}catch(const std::exception&){bad=true;}check(bad,"unsafe execution checkpoint accepted");f.verify(disk);++rejects;};
  auto archive_save=[&](const std::vector<CheckpointArchiveEntry>& records,size_t cap=8){return disk.commit_execution_checkpoint(1337,f.genesis.header.hash(),token,f.b.block.header.hash(),1,f.b.block.header.hash(),{},f.snapshot(f.b),budget,cap,records);};
  auto conflict=f.archived(f.a);conflict.payload[0]^=1;
  reject([&]{archive_save({conflict});});
  auto fresh=f.archived(f.a);fresh.root=Bytes(32,0xab);auto duplicate=fresh;duplicate.slot++;
  reject([&]{archive_save({fresh,duplicate});});
  check(!disk.checkpoint_archive(fresh.root,budget),"rejected archive leaked writes");
  reject([&]{archive_save({fresh},0);});
  auto oversized=fresh;oversized.payload.resize(budget,1);reject([&]{archive_save({oversized});});
  reject([&]{disk.checkpoint_archive(f.a.block.header.hash(),8);});
  token=archive_save({f.archived(f.a),f.archived(f.a)});
  reject([&]{f.save(disk,{},f.b.block.header.hash(),{f.a});});reject([&]{f.save(disk,token,f.a.block.header.hash(),{f.a});});
  auto bad=f.a;bad.receipts[0].gas_used+=1;reject([&]{f.save(disk,token,f.b.block.header.hash(),{bad});});bad=f.a;bad.state.get(f.recipient).balance+=1;reject([&]{f.save(disk,token,f.b.block.header.hash(),{bad});});
  reject([&]{disk.rewind(0);});reject([&]{disk.commit(f.c.block,f.c.state,f.c.receipts);});reject([&]{disk.commit_fork_checkpoint(1337,f.genesis.header.hash(),token,"detached",budget);});
  reject([&]{disk.commit_execution_checkpoint(1337,f.genesis.header.hash(),token,f.b.block.header.hash(),0,f.genesis.header.hash(),{f.a},"tiny",32,8);});
 }
 {ChainStore disk(path);std::barrier start(3);std::atomic<unsigned> winners=0,stale=0;std::exception_ptr failure;std::mutex errors;
  auto writer=[&](const ExecutionCheckpointBlock& next){start.arrive_and_wait();try{f.save(disk,token,f.b.block.header.hash(),{next});++winners;}catch(const std::invalid_argument&){++stale;}catch(...){std::lock_guard lock(errors);failure=std::current_exception();}};
  std::thread a(writer,std::cref(f.a)),b(writer,std::cref(f.b));start.arrive_and_wait();a.join();b.join();if(failure)std::rethrow_exception(failure);check(winners==1&&stale==1,"concurrent checkpoint CAS failed");token=f.verify(disk).token;
 }
 for(unsigned i=0;i<4;++i){Process writer({argv[0],"--writer",path.string()});writer.line([](const auto& s){return s=="ready";});if(i%2)writer.line([](const auto& s){return !s.empty()&&s[0]>='0'&&s[0]<='9';});writer.signal(SIGKILL);auto stopped=writer.wait(10);check(stopped.code==-SIGKILL,"writer not killed");uint64_t acknowledged=token.sequence;std::istringstream output(stopped.out);std::string line;while(std::getline(output,line))if(!line.empty()&&line[0]>='0'&&line[0]<='9')acknowledged=std::max(acknowledged,uint64_t(std::stoull(line)));ChainStore disk(path);const auto record=f.verify(disk);check(record.token.sequence>=acknowledged,"acknowledged commit lost");token=record.token;}
 std::cout<<Json{{"scenario","atomic-execution-checkpoint"},{"rejections",rejects},{"sigkillReopens",4},{"shorterReorg",true},{"concurrentCAS",true},{"unchangedHeadCheckpoint",true},{"bellatrixReactor",false},{"archiveAtomicRecovery",true},{"archiveSurvivesReorg",true}}.dump()<<'\n';
#endif
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
