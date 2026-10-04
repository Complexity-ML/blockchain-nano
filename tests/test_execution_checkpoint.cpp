#include "support/process.hpp"
#include "nano/storage.hpp"
#include <atomic>
#include <barrier>
#include <rocksdb/db.h>
#ifdef NANO_SYNC_FAULT
extern "C" void nano_test_arm_sync_fault(const char*);
extern "C" unsigned nano_test_sync_failures();
#endif
using namespace test;
namespace {
constexpr size_t budget=8*1024*1024;
#ifdef NANO_SHANGHAI_CHECKPOINT
constexpr bool shanghai=true;
#else
constexpr bool shanghai=false;
#endif
struct Fixture {
 Bytes key=bytes(Int(777),32);std::string recipient="0x0000000000000000000000000000000000001234";
 Block genesis;State state;ExecutionCheckpointBlock a,b,c;
 Fixture(){Transaction tx;tx.to=recipient;tx.gas=21000;tx.gas_price=2000000000;const auto sender=SignedTransaction::sign(tx,key).sender();state.get(sender).balance=Int("1000000000000000000");genesis.header.difficulty=0;genesis.header.state_root=state.root();
  auto make=[&](const Block& parent,const State& before,unsigned value,unsigned nonce){ExecutionCheckpointBlock out;out.state=before;auto& h=out.block.header;h.number=parent.header.number+1;h.parent_hash=parent.header.hash();h.difficulty=0;h.timestamp=parent.header.timestamp+12;h.base_fee=next_base_fee(parent.header);auto transfer=tx;transfer.value=value;transfer.nonce=nonce;out.block.transactions.push_back(SignedTransaction::sign(transfer,key));if(shanghai)out.block.withdrawals.push_back(Json{{"index",quantity(Int(nonce))},{"validatorIndex","0x0"},{"address",recipient},{"amount",quantity(Int(value))}});out.receipts=ExecutionEngine(1337,{},shanghai?EvmRevision::Shanghai:EvmRevision::Paris,ReplayPolicy::RequireChainId,shanghai?ConsensusInputPolicy::EthereumValidated:ConsensusInputPolicy::RejectExternalInputs).execute_block(out.state,out.block,false);return out;};
  a=make(genesis,state,3,0);b=make(genesis,state,7,0);c=make(a.block,a.state,5,1);
 }
 void init(ChainStore& disk){disk.bind_chain(1337,genesis.header.hash(),"bellatrix-storage-test");disk.commit(genesis,state,{});}
 std::string snapshot(const ExecutionCheckpointBlock& x)const{return Json{{"head",hex(x.block.header.hash())},{"state",hex(x.state.root())},{"padding",std::string(1024*1024,'x')}}.dump();}
 CheckpointArchiveEntry archived(const ExecutionCheckpointBlock& x)const{const auto text=x.block.to_json(true).dump();return {x.block.header.hash(),u64(x.block.header.number),Bytes(text.begin(),text.end())};}
 ForkCheckpointToken save(ChainStore& disk,const std::optional<ForkCheckpointToken>& token,const Bytes& tip,const std::vector<ExecutionCheckpointBlock>& suffix){std::vector<CheckpointArchiveEntry> records;for(const auto& x:suffix)records.push_back(archived(x));return disk.commit_execution_checkpoint(1337,genesis.header.hash(),token,tip,0,genesis.header.hash(),suffix,snapshot(suffix.back()),budget,8,records);}
 ForkCheckpointRecord verify(ChainStore& disk){auto record=disk.fork_checkpoint(1337,genesis.header.hash(),budget);check(bool(record),"checkpoint missing");const auto data=Json::parse(record->snapshot);const auto head=disk.latest();check(head&&hex(head->header.hash())==data.at("head").get<std::string>()&&hex(disk.state_at(head->header.number)->root())==data.at("state").get<std::string>(),"mixed execution/checkpoint recovered");const bool branch_b=head->header.hash()==b.block.header.hash();check(head->header.hash()==a.block.header.hash()||branch_b,"unexpected recovered branch");const auto& yes=branch_b?b:a;const auto& no=branch_b?a:b;check(head->withdrawals==yes.block.withdrawals&&head->header.withdrawals_root==yes.block.header.withdrawals_root,"withdrawals not restored");if(shanghai){check(head->header.withdrawals_root==withdrawals_root(head->withdrawals),"withdrawal commitment mismatch");const auto amount=branch_b?7:3;check(disk.state_at(head->header.number)->get(recipient).balance==Int(amount)*Int("1000000001"),"withdrawal balance lost or doubled");}const auto stored=disk.checkpoint_archive(head->header.hash(),budget);check(stored&&stored->payload==archived(yes).payload&&stored->slot==u64(head->header.number),"archive/checkpoint mixed after recovery");check(disk.receipt(yes.block.transactions[0].hash())->block_hash==head->header.hash()&&!disk.receipt(no.block.transactions[0].hash())&&!disk.transaction(no.block.transactions[0].hash())&&!disk.block_by_hash(no.block.header.hash()),"mixed canonical indexes");check(disk.count()==2&&!disk.block_by_number(2)&&!disk.receipt(c.block.transactions[0].hash()),"old suffix survived reorg");return *record;}
};
}
int main(int argc,char** argv){try{
 Fixture f;
 if(argc==3&&std::string(argv[1])=="--pruning-schema"){
  Temp tmp;const auto path=tmp/"db";ForkCheckpointToken token;
  {ChainStore disk(path);f.init(disk);token=f.save(disk,{},f.genesis.header.hash(),{f.a,f.c});}
  check(run({argv[2],path.string()}).code==0,"legacy reader cannot open unpruned schema");
  {ChainStore disk(path);token=disk.commit_execution_checkpoint(1337,f.genesis.header.hash(),token,f.c.block.header.hash(),2,f.c.block.header.hash(),{},f.snapshot(f.c),budget,8,{},ExecutionStateRetentionAnchor{2,f.c.block.header.hash()});}
  auto file_hashes=[](const fs::path& root){std::map<std::string,std::string> out;for(const auto& e:fs::recursive_directory_iterator(root))if(e.is_regular_file()){const auto d=read(e.path());out.emplace(fs::relative(e.path(),root).string(),hex(sha256(Bytes(d.begin(),d.end()))));}return out;};
  const auto before=file_hashes(path);const auto legacy=run({argv[2],path.string()});check(legacy.code!=0&&legacy.err.find("unsupported Nano database schema")!=std::string::npos,"legacy reader accepted pruned schema or failed for unrelated reason");check(file_hashes(path)==before,"legacy read-only refusal mutated pruned database");
  {ChainStore disk(path,ChainStore::OpenMode::ReadOnly);check(disk.state_pruning()->before==2&&!disk.state_at(1)&&disk.state_at(2)->root()==f.c.state.root()&&disk.fork_checkpoint(1337,f.genesis.header.hash(),budget,token).has_value(),"new schema reader lost checkpoint/retention");}
  check(file_hashes(path)==before,"new schema read-only open mutated files");
  for(unsigned attack=0;attack<2;++attack){const auto copy=tmp/("bad-"+std::to_string(attack));fs::copy(path,copy,fs::copy_options::recursive);rocksdb::Options options;options.create_if_missing=false;rocksdb::DB* raw=nullptr;check(rocksdb::DB::Open(options,copy.string(),&raw).ok(),"open owned schema corruption fixture");std::unique_ptr<rocksdb::DB> db(raw);rocksdb::WriteOptions write;write.sync=true;
   if(attack==0)check(db->Put(write,"meta/schema","nano/1").ok(),"write owned legacy pruning schema");else check(db->Delete(write,"execution-state-pruning/v1").ok(),"remove owned pruning marker");db.reset();bool refused=false;try{ChainStore disk(copy,ChainStore::OpenMode::ReadOnly);}catch(const std::runtime_error&){refused=true;}check(refused,"schema/marker disagreement accepted");}
  std::cout<<Json{{"scenario","pruned-schema-legacy-refusal"},{"legacyUnprunedReadable",true},{"legacyPrunedRefused",true},{"newPrunedReadable",true},{"readOnlyFilesUnchanged",true},{"schemaMarkerCorruptionsRejected",2},{"schemaUpgradeSharesCheckpointBatch",true},{"automaticDowngrade",false}}.dump()<<std::endl;return 0;
 }
 if(argc==2&&std::string(argv[1])=="--pruning-restore"){
  Temp tmp;const auto source=tmp/"source",backup=tmp/"backup",restored=tmp/"restored";
  std::vector<ExecutionCheckpointBlock> history{f.a,f.c};
  for(unsigned number=3;number<=6;++number){
   const auto& parent=history.back();ExecutionCheckpointBlock next;next.state=parent.state;
   auto& h=next.block.header;h.number=number;h.parent_hash=parent.block.header.hash();h.difficulty=0;h.timestamp=parent.block.header.timestamp+12;h.base_fee=next_base_fee(parent.block.header);
   next.receipts=ExecutionEngine(1337,{},EvmRevision::Paris,ReplayPolicy::RequireChainId,ConsensusInputPolicy::RejectExternalInputs).execute_block(next.state,next.block,false);history.push_back(std::move(next));
  }
  const auto& head=history.back();const auto head_hash=head.block.header.hash();const auto snapshot=f.snapshot(head);ForkCheckpointToken token;
  auto verify=[&](ChainStore& disk,unsigned boundary){
   const auto marker=disk.state_pruning();check(marker&&marker->before==boundary&&marker->anchor.number==6&&marker->anchor.hash==head_hash,"restored pruning boundary differs");
   check(disk.latest()->header.hash()==head_hash&&disk.state_at(0)->root()==f.state.root()&&disk.state_at(6)->root()==head.state.root(),"restored genesis/head differs");
   for(unsigned n=1;n<=6;++n){check(bool(disk.state_at(n))==(n>=boundary),"pruning crossed bounded frontier");const auto& original=history.at(n-1);check(disk.block_by_number(n)->header.hash()==original.block.header.hash()&&disk.checkpoint_archive(original.block.header.hash(),budget)->payload==f.archived(original).payload,"pruning lost canonical block/archive");}
   check(disk.receipt(f.a.block.transactions[0].hash()).has_value()&&disk.receipt(f.c.block.transactions[0].hash()).has_value(),"pruning lost receipt");
   check(disk.fork_checkpoint(1337,f.genesis.header.hash(),budget,token).has_value(),"restored checkpoint commitment differs");
  };
  {
   ChainStore disk(source);f.init(disk);token=f.save(disk,{},f.genesis.header.hash(),history);
   const auto before=token;bool rejected=false;
   try{disk.commit_execution_checkpoint(1337,f.genesis.header.hash(),token,head_hash,6,head_hash,{},snapshot,snapshot.size(),2,{},ExecutionStateRetentionAnchor{6,head_hash});}catch(const std::length_error&){rejected=true;}
   check(rejected&&!disk.state_pruning()&&disk.state_at(1).has_value()&&disk.fork_checkpoint(1337,f.genesis.header.hash(),budget,before).has_value(),"byte budget failure published pruning changes");
   token=disk.commit_execution_checkpoint(1337,f.genesis.header.hash(),token,head_hash,6,head_hash,{},snapshot,budget,0,{},ExecutionStateRetentionAnchor{6,head_hash});
   check(!disk.state_pruning()&&disk.state_at(1).has_value(),"zero block budget deleted state");
   token=disk.commit_execution_checkpoint(1337,f.genesis.header.hash(),token,head_hash,6,head_hash,{},snapshot,budget,2,{},ExecutionStateRetentionAnchor{6,head_hash});verify(disk,3);
  }
  auto file_hashes=[](const fs::path& directory){std::map<std::string,std::string> result;for(const auto& entry:fs::recursive_directory_iterator(directory))if(entry.is_regular_file()){const auto data=read(entry.path());result.emplace(fs::relative(entry.path(),directory).string(),hex(sha256(Bytes(data.begin(),data.end()))));}return result;};
  const auto source_hashes=file_hashes(source);fs::copy(source,backup,fs::copy_options::recursive);check(file_hashes(backup)==source_hashes,"offline backup differs");fs::copy(backup,restored,fs::copy_options::recursive);
  const auto readonly_before=file_hashes(restored);{ChainStore disk(restored,ChainStore::OpenMode::ReadOnly);verify(disk,3);}check(file_hashes(restored)==readonly_before,"restored read-only verification wrote files");
  for(unsigned boundary:{5u,6u}){
   ChainStore disk(restored);token=disk.commit_execution_checkpoint(1337,f.genesis.header.hash(),token,head_hash,6,head_hash,{},snapshot,budget,2,{},ExecutionStateRetentionAnchor{6,head_hash});verify(disk,boundary);
  }
  {ChainStore disk(restored);const auto before=token;token=disk.commit_execution_checkpoint(1337,f.genesis.header.hash(),token,head_hash,6,head_hash,{},snapshot,budget,2,{},ExecutionStateRetentionAnchor{6,head_hash});check(token.sequence==before.sequence+1,"completed pruning cannot resume checkpoint");verify(disk,6);
   bool refused=false;try{disk.commit_execution_checkpoint(1337,f.genesis.header.hash(),token,head_hash,5,history.at(4).block.header.hash(),{},snapshot,budget,2);}catch(const std::invalid_argument&){refused=true;}check(refused&&disk.fork_checkpoint(1337,f.genesis.header.hash(),budget,token).has_value(),"replacement below retained anchor was published");}
  check(file_hashes(source)==source_hashes&&file_hashes(backup)==source_hashes,"restore/resume changed backup source");
  std::cout<<Json{{"scenario","historical-state-pruning-offline-restore"},{"frontiers",Json::array({3,5,6})},{"zeroBlockBudgetRetainedAllStates",true},{"byteBudgetRejectedAtomically",true},{"physicalCopyVerified",true},{"readOnlyCopyUnchanged",true},{"sourceBackupUnchanged",true},{"resumedCheckpoint",true},{"physicalPowerLoss",false},{"consensusWitnessesQualified",false}}.dump()<<std::endl;return 0;
 }
 if(argc==2&&std::string(argv[1])=="--pruning-corruption"){
  for(unsigned attack=0;attack<11;++attack){Temp tmp;const auto path=tmp/"db";
   {ChainStore disk(path);f.init(disk);const auto token=f.save(disk,{},f.genesis.header.hash(),{f.a,f.c});disk.commit_execution_checkpoint(1337,f.genesis.header.hash(),token,f.c.block.header.hash(),2,f.c.block.header.hash(),{},f.snapshot(f.c),budget,8,{},ExecutionStateRetentionAnchor{2,f.c.block.header.hash()});check(disk.state_pruning()->before==2&&!disk.state_at(1),"corruption fixture was not pruned");}
   rocksdb::Options options;options.create_if_missing=false;rocksdb::DB* raw=nullptr;check(rocksdb::DB::Open(options,path.string(),&raw).ok(),"open owned corruption fixture");std::unique_ptr<rocksdb::DB> db(raw);
   Json marker{{"before","0x2"},{"anchorNumber","0x2"},{"anchorHash",hex(f.c.block.header.hash())}};const auto state_key="state/"+hex(bytes(Int(2),8));rocksdb::WriteOptions write;write.sync=true;
   if(attack==0)marker["before"]="0x0";
   if(attack==1)marker["before"]="0x3";
   if(attack==2)marker["anchorNumber"]="0x10000000000000000";
   if(attack==3)marker["before"]="0x02";
   if(attack==4)marker["anchorHash"]=hex(Bytes(32,0xab));
   if(attack==5)marker["extra"]=true;
   if(attack==6)marker.erase("anchorNumber");
   if(attack==7)marker["anchorHash"]="0xab";
   if(attack<8)check(db->Put(write,"execution-state-pruning/v1",marker.dump()).ok(),"write owned bad marker");
   if(attack==8)check(db->Delete(write,state_key).ok(),"delete owned anchor state");
   if(attack==9)check(db->Put(write,state_key,f.state.to_json().dump()).ok(),"write owned wrong state root");
   if(attack==10)check(db->Delete(write,"execution-checkpoint/v1").ok(),"delete owned checkpoint binding");
   db.reset();bool refused=false;try{ChainStore disk(path,ChainStore::OpenMode::ReadOnly);(void)disk.state_pruning();}catch(const std::exception&){refused=true;}check(refused,"corrupt pruning marker/anchor/checkpoint accepted");
  }
  std::cout<<Json{{"scenario","historical-state-pruning-corruption"},{"corruptionsRejected",11},{"ownedFixturesOnly",true}}.dump()<<std::endl;return 0;
 }
 if(argc==3){ChainStore disk(argv[2]);
#ifdef NANO_SYNC_FAULT
  if(std::string(argv[1])=="--fail-pruning"){
   f.init(disk);const auto token=f.save(disk,{},f.genesis.header.hash(),{f.a,f.c});
   fs::path wal;for(const auto& e:fs::directory_iterator(argv[2]))if(e.path().extension()==".log"&&(wal.empty()||e.path().filename()>wal.filename()))wal=e.path();check(!wal.empty(),"pruning WAL missing");const auto before=fs::file_size(wal);
   nano_test_arm_sync_fault(wal.c_str());bool failed=false;
   try{disk.commit_execution_checkpoint(1337,f.genesis.header.hash(),token,f.c.block.header.hash(),2,f.c.block.header.hash(),{},f.snapshot(f.c),budget,8,{},ExecutionStateRetentionAnchor{2,f.c.block.header.hash()});}catch(const std::runtime_error&){failed=true;}
   check(failed&&nano_test_sync_failures(),"pruning sync fault not observed");bool blocked=false;try{disk.state_pruning();}catch(const std::runtime_error&){blocked=true;}check(blocked,"uncertain pruning database remained readable");
   std::cout<<Json{{"wal",wal.filename().string()},{"before",before},{"after",fs::file_size(wal)}}.dump()<<std::endl;_exit(0);
  }
  if(std::string(argv[1])=="--fail"){
   f.init(disk);const auto token=f.save(disk,{},f.genesis.header.hash(),{f.a});fs::path wal;for(const auto& e:fs::directory_iterator(argv[2]))if(e.path().extension()==".log"&&(wal.empty()||e.path().filename()>wal.filename()))wal=e.path();check(!wal.empty(),"WAL missing");const auto before=fs::file_size(wal);nano_test_arm_sync_fault(wal.c_str());bool failed=false;try{f.save(disk,token,f.a.block.header.hash(),{f.b});}catch(const std::runtime_error&){failed=true;}check(failed&&nano_test_sync_failures(),"sync fault not observed");unsigned blocked=0;try{disk.latest();}catch(const std::runtime_error&){++blocked;}try{disk.fork_checkpoint(1337,f.genesis.header.hash(),budget);}catch(const std::runtime_error&){++blocked;}try{f.save(disk,token,f.a.block.header.hash(),{f.b});}catch(const std::runtime_error&){++blocked;}check(blocked==3,"uncertain disk remained usable");std::cout<<Json{{"wal",wal.filename().string()},{"before",before},{"after",fs::file_size(wal)}}.dump()<<std::endl;_exit(0);
  }
#endif
  auto record=f.verify(disk);std::cout<<"ready"<<std::endl;
  for(unsigned i=0;i<128;++i){const auto tip=disk.latest()->header.hash();const auto& next=tip==f.a.block.header.hash()?f.b:f.a;record.token=f.save(disk,record.token,tip,{next});std::cout<<record.token.sequence<<std::endl;}_exit(0);
 }
#ifdef NANO_SYNC_FAULT
 if(argc==2&&std::string(argv[1])=="--pruning-sync"){
  for(bool lose:{false,true}){Temp tmp;const auto path=tmp/"db";const auto result=success(run({argv[0],"--fail-pruning",path.string()}));const auto wal=path/result.at("wal").get<std::string>();check(fs::file_size(wal)==result.at("after").get<uint64_t>()&&result.at("after")>result.at("before"),"pruning WAL suffix missing");
   if(lose){int fd=open(wal.c_str(),O_WRONLY);check(fd>=0,"open owned pruning WAL");const bool ok=ftruncate(fd,result.at("before").get<uint64_t>())==0&&fsync(fd)==0;close(fd);check(ok,"truncate owned pruning WAL");}
   ChainStore disk(path);const auto cp=disk.fork_checkpoint(1337,f.genesis.header.hash(),budget);const auto marker=disk.state_pruning();
   check(cp&&cp->token.sequence==(lose?1:2)&&bool(marker)==!lose,"pruning checkpoint/marker split across WAL recovery");
   check(disk.state_at(0).has_value()&&disk.state_at(2)->root()==f.c.state.root()&&bool(disk.state_at(1))==lose,"pruning state deletion split across WAL recovery");
   check(disk.latest()->header.hash()==f.c.block.header.hash()&&disk.block_by_number(1)->header.hash()==f.a.block.header.hash()&&disk.checkpoint_archive(f.a.block.header.hash(),budget)->payload==f.archived(f.a).payload&&disk.receipt(f.a.block.transactions[0].hash()).has_value(),"pruning removed canonical archives/indexes");
   if(marker)check(marker->before==2&&marker->anchor.number==2&&marker->anchor.hash==f.c.block.header.hash(),"pruning recovered wrong boundary");
   const auto next=disk.commit_execution_checkpoint(1337,f.genesis.header.hash(),cp->token,f.c.block.header.hash(),2,f.c.block.header.hash(),{},f.snapshot(f.c),budget,8,{},ExecutionStateRetentionAnchor{2,f.c.block.header.hash()});
   check(next.sequence==cp->token.sequence+1&&disk.state_pruning()->before==2&&!disk.state_at(1),"pruning recovery could not continue/idempotent boundary failed");
  }
  std::cout<<Json{{"scenario","atomic-historical-state-pruning-sync"},{"retainedAndLostSuffix",true},{"markerStateCheckpointAtomic",true},{"continuedAfterRecovery",true},{"physicalPowerLoss",false}}.dump()<<std::endl;return 0;
 }
 for(bool lose:{false,true}){Temp tmp;auto path=tmp/"db";const auto result=success(run({argv[0],"--fail",path.string()}));auto wal=path/result.at("wal").get<std::string>();check(fs::file_size(wal)==result.at("after").get<uint64_t>()&&result.at("after")>result.at("before"),"WAL suffix missing");if(lose){int fd=open(wal.c_str(),O_WRONLY);check(fd>=0,"open owned WAL");bool ok=ftruncate(fd,result.at("before").get<uint64_t>())==0&&fsync(fd)==0;close(fd);check(ok,"truncate owned WAL");}ChainStore disk(path);auto record=f.verify(disk);check(record.token.sequence==(lose?1:2)&&disk.latest()->header.hash()==(lose?f.a:f.b).block.header.hash(),"wrong recovery disposition");check(bool(disk.checkpoint_archive(f.b.block.header.hash(),budget))==!lose,"archive detached from retained/lost WAL suffix");f.save(disk,record.token,disk.latest()->header.hash(),{f.a});f.verify(disk);}
 std::cout<<Json{{"scenario","atomic-execution-checkpoint-sync"},{"shanghai",shanghai},{"retainedAndLostSuffix",true},{"physicalPowerLoss",false}}.dump()<<'\n';
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
  bad=f.a;if(shanghai)bad.block.withdrawals[0]["amount"]="0xff";else bad.block.withdrawals.push_back(Json{{"index","0x0"},{"validatorIndex","0x0"},{"address",f.recipient},{"amount","0x1"}});
  {bool refused=false;try{disk.commit_execution_checkpoint(1337,f.genesis.header.hash(),token,f.b.block.header.hash(),0,f.genesis.header.hash(),{bad},f.snapshot(bad),budget,8);}
   catch(const std::invalid_argument& e){refused=std::string(e.what())=="execution checkpoint withdrawal binding";}
   check(refused,"uncommitted withdrawal body accepted or wrong rejection");f.verify(disk);++rejects;}
  reject([&]{disk.rewind(0);});reject([&]{disk.commit(f.c.block,f.c.state,f.c.receipts);});reject([&]{disk.commit_fork_checkpoint(1337,f.genesis.header.hash(),token,"detached",budget);});
  reject([&]{disk.commit_execution_checkpoint(1337,f.genesis.header.hash(),token,f.b.block.header.hash(),0,f.genesis.header.hash(),{f.a},"tiny",32,8);});
 }
 {ChainStore disk(path);std::barrier start(3);std::atomic<unsigned> winners=0,stale=0;std::exception_ptr failure;std::mutex errors;
  auto writer=[&](const ExecutionCheckpointBlock& next){start.arrive_and_wait();try{f.save(disk,token,f.b.block.header.hash(),{next});++winners;}catch(const std::invalid_argument&){++stale;}catch(...){std::lock_guard lock(errors);failure=std::current_exception();}};
  std::thread a(writer,std::cref(f.a)),b(writer,std::cref(f.b));start.arrive_and_wait();a.join();b.join();if(failure)std::rethrow_exception(failure);check(winners==1&&stale==1,"concurrent checkpoint CAS failed");token=f.verify(disk).token;
 }
 for(unsigned i=0;i<4;++i){Process writer({argv[0],"--writer",path.string()});writer.line([](const auto& s){return s=="ready";});if(i%2)writer.line([](const auto& s){return !s.empty()&&s[0]>='0'&&s[0]<='9';});writer.signal(SIGKILL);auto stopped=writer.wait(10);check(stopped.code==-SIGKILL,"writer not killed");uint64_t acknowledged=token.sequence;std::istringstream output(stopped.out);std::string line;while(std::getline(output,line))if(!line.empty()&&line[0]>='0'&&line[0]<='9')acknowledged=std::max(acknowledged,uint64_t(std::stoull(line)));ChainStore disk(path);const auto record=f.verify(disk);check(record.token.sequence>=acknowledged,"acknowledged commit lost");token=record.token;}
 std::cout<<Json{{"scenario","atomic-execution-checkpoint"},{"shanghai",shanghai},{"rejections",rejects},{"sigkillReopens",4},{"shorterReorg",true},{"concurrentCAS",true},{"unchangedHeadCheckpoint",true},{"bellatrixReactor",false},{"archiveAtomicRecovery",true},{"archiveSurvivesReorg",true}}.dump()<<'\n';
#endif
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
