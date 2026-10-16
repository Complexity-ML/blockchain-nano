#include "support/process.hpp"
#include "nano/storage_memory_policy.hpp"
#include <rocksdb/db.h>
#include <rocksdb/table.h>
#include <rocksdb/write_batch.h>
#include <array>
#include <chrono>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace test;
namespace {
const auto began=std::chrono::steady_clock::now();
uint64_t elapsed(){return uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-began).count());}
uint64_t rss(){rusage s{};check(::getrusage(RUSAGE_SELF,&s)==0,"RSS observation");
#ifdef __APPLE__
 return uint64_t(s.ru_maxrss)/1024;
#else
 return uint64_t(s.ru_maxrss);
#endif
}
void bounded(){check(elapsed()<120000&&rss()<=131072,"test resource budget");}
void private_dir(const fs::path& p){check(fs::create_directory(p),"new private directory");check(::chmod(p.c_str(),0700)==0,"private directory mode");}
void file(const fs::path& p,std::string_view b){int fd=::open(p.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);check(fd>=0,"new owned file");size_t pos=0;while(pos<b.size()){auto n=::write(fd,b.data()+pos,b.size()-pos);if(n<0&&errno==EINTR)continue;if(n<=0){::close(fd);throw std::runtime_error("fixture write");}pos+=size_t(n);}check(::fsync(fd)==0,"fixture fsync");check(::close(fd)==0,"fixture close");}
std::string digest(std::string_view s){return hex(sha256(Bytes(s.begin(),s.end()))).substr(2);}
std::array<int64_t,9> material(const struct stat& s){
#ifdef __APPLE__
 const auto mt=int64_t(s.st_mtimespec.tv_sec)*1000000000+s.st_mtimespec.tv_nsec;
 const auto ct=int64_t(s.st_ctimespec.tv_sec)*1000000000+s.st_ctimespec.tv_nsec;
#else
 const auto mt=int64_t(s.st_mtim.tv_sec)*1000000000+s.st_mtim.tv_nsec;
 const auto ct=int64_t(s.st_ctim.tv_sec)*1000000000+s.st_ctim.tv_nsec;
#endif
 return {int64_t(s.st_dev),int64_t(s.st_ino),int64_t(s.st_mode),int64_t(s.st_uid),int64_t(s.st_gid),int64_t(s.st_nlink),int64_t(s.st_size),mt,ct};
}
using Inventory=std::map<std::string,Json>;
// Refusal oracle uses lstat for every type, never follows links or opens FIFOs.
// Atime is excluded; all other nine fields and all regular bytes are retained.
Inventory inventory(const fs::path& root){Inventory out;uint64_t bytes=0;auto one=[&](const fs::path& p){bounded();check(out.size()<4096,"inventory file bound");struct stat a{},b{},f{};check(::lstat(p.c_str(),&a)==0,"inventory lstat");Json row{{"material",material(a)}};if(S_ISREG(a.st_mode)){check(a.st_size>=0&&uint64_t(a.st_size)<=64*1024*1024-bytes,"inventory byte bound");bytes+=uint64_t(a.st_size);int fd=::open(p.c_str(),O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);check(fd>=0,"inventory regular FD");check(::fstat(fd,&f)==0&&material(f)==material(a),"inventory FD/path binding");std::string raw;raw.reserve(size_t(a.st_size));std::array<char,8192> block{};for(;;){auto n=::read(fd,block.data(),block.size());if(n<0&&errno==EINTR)continue;check(n>=0,"inventory read");if(!n)break;raw.append(block.data(),size_t(n));check(raw.size()<=uint64_t(a.st_size),"inventory file grew");}check(::fstat(fd,&f)==0&&material(f)==material(a),"inventory FD stability");check(::close(fd)==0,"inventory close");check(raw.size()==uint64_t(a.st_size),"inventory length");row["sha256"]=digest(raw);}else if(S_ISLNK(a.st_mode)){row["target"]=fs::read_symlink(p).string();}check(::lstat(p.c_str(),&b)==0&&material(a)==material(b),"inventory path stability");check(out.emplace(p.lexically_relative(root).string(),std::move(row)).second,"inventory unique path");};one(root);for(const auto& x:fs::recursive_directory_iterator(root))one(x.path());return out;}
rocksdb::Options options(){rocksdb::Options o;o.create_if_missing=false;o.paranoid_checks=true;o.allow_fallocate=false;rocksdb::BlockBasedTableOptions table;table.format_version=6;o.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table));return o;}
template<class Database=rocksdb::DB>
std::unique_ptr<Database> open_db(const fs::path& p,bool create,bool ro=false){
 auto o=options();o.create_if_missing=create;std::unique_ptr<Database> result;
 const auto good=[](const rocksdb::Status& status){check(status.ok(),"synthetic RocksDB open: "+status.ToString());};
 if(ro){if constexpr(requires { Database::OpenForReadOnly(o,p.string(),&result); })good(Database::OpenForReadOnly(o,p.string(),&result));else{Database* raw=nullptr;auto status=Database::OpenForReadOnly(o,p.string(),&raw);result.reset(raw);good(status);}}
 else{if constexpr(requires { Database::Open(o,p.string(),&result); })good(Database::Open(o,p.string(),&result));else{Database* raw=nullptr;auto status=Database::Open(o,p.string(),&raw);result.reset(raw);good(status);}}
 return result;
}
using Rows=std::map<std::string,std::string>;
Rows logical(const fs::path& p){auto db=open_db(p,false,true);rocksdb::ReadOptions o;o.verify_checksums=true;o.fill_cache=false;std::unique_ptr<rocksdb::Iterator> i(db->NewIterator(o));Rows rows;size_t bytes=0;for(i->SeekToFirst();i->Valid();i->Next()){bounded();bytes+=i->key().size()+i->value().size();check(rows.size()<256&&bytes<=1024*1024,"synthetic logical bound");rows.emplace(i->key().ToString(),i->value().ToString());}check(i->status().ok(),"synthetic logical scan");return rows;}
struct Fixture{
 fs::path root,source,copy,request;bool migration;
 Fixture(const fs::path& parent,const std::string& name,bool migrate):root(parent/name),migration(migrate){private_dir(root);private_dir(root/"original");private_dir(root/"candidate");source=root/"original/chain";copy=root/"candidate/chain";private_dir(source);const std::string payload="public synthetic opaque checkpoint payload";const std::string sum="0x"+digest(payload);Json token{{"sequence","0x1"},{"digest",sum}};{auto db=open_db(source,true);rocksdb::WriteBatch batch;batch.Put("meta/schema","nano/1");batch.Put("fork-checkpoint/v1/token",token.dump());batch.Put("fork-checkpoint/v1/payload",payload);batch.Put("execution-checkpoint/v1",Json{{"digest",sum}}.dump());batch.Put("execution-state-pruning/v1",Json{{"before","0x2"},{"anchorNumber","0x2"},{"anchorHash","0x"+std::string(64,'1')}}.dump());for(unsigned i=0;i<32;++i)batch.Put("public/"+std::to_string(i),std::string(1024,char('a'+i%26)));rocksdb::WriteOptions w;w.sync=true;check(db->Write(w,&batch).ok(),"synthetic fixture batch");rocksdb::FlushOptions f;f.wait=true;check(db->Flush(f).ok(),"synthetic fixture SST");}check(::chmod(source.c_str(),0700)==0,"synthetic source root");fs::copy(source,copy,fs::copy_options::recursive);check(::chmod(copy.c_str(),0700)==0,"synthetic copy root");const auto marker=root/"candidate"/(migration?".nano-owned-schema-migration-copy":".nano-owned-compaction-copy");file(marker,migration?"owned-offline-schema-migration-v1\n":"owned-offline-qualification-copy-v1\n");request=root/"request.json";file(request,Json{{"expectedCheckpoint",token},{"limits",{{"stateBytes","0x100000"}}}}.dump());check(logical(source)==logical(copy),"fresh copy logical equality");
 }
 fs::path marker()const{return root/"candidate"/(migration?".nano-owned-schema-migration-copy":".nano-owned-compaction-copy");}
 std::vector<std::string> args(const std::string& executable={})const{const auto binary=executable.empty()?std::string(migration?NANO_TEST_MIGRATE:NANO_TEST_COMPACT):executable;std::vector<std::string> a{binary,"--offline-owned-copy",copy.string()};if(migration){a.push_back("--audit-request");a.push_back(request.string());}return a;}
};
uint64_t calls=0;
Result invoke(const Fixture& f,const std::string& binary={}){bounded();Process child(f.args(binary),{{"NANO_TEST_MIGRATION_STOP",std::nullopt}});++calls;auto r=child.wait(10);bounded();return r;}
void emit(const char* name,Json fields){bounded();fields["scenario"]=name;fields["result"]="PASS";fields["newSignaturesCreated"]=0;fields["privateInputsRead"]=false;fields["productionQualified"]=false;fields["elapsedMs"]=elapsed();fields["maxRssKiB"]=rss();std::cout<<fields.dump()<<std::endl;}
void independent(const fs::path& p){uint64_t start=calls;for(bool migration:{false,true}){Fixture f(p,migration?"independent-migration":"independent-compaction",migration);private_dir(f.copy/"ordinary-subdirectory");file(f.copy/"ordinary-subdirectory/public-leaf","ordinary independent bytes");check(::chmod(f.copy.c_str(),0755)==0,"nonprivate copy mode remains supported");const auto source=inventory(f.root/"original");const auto expected=logical(f.copy);const auto r=invoke(f);check(r.code==0,"valid independent CLI copy refused: "+r.err);const auto report=Json::parse(r.out);auto after=logical(f.copy);if(migration){check(report.at("onlySchemaChanged")==true,"migration did not attest exact non-schema equality");auto changed=expected;changed["meta/schema"]="nano/2-state-pruned";check(after==changed,"migration changed anything except schema");const auto again=invoke(f);check(again.code==0,"migration idempotent replay refused");const auto j=Json::parse(again.out);check(j.at("alreadyMigrated")==true&&j.at("writes")==0,"migration replay wrote");check(logical(f.copy)==changed,"migration replay changed logical rows");}else check(report.at("logicalContentUnchanged")==true&&after==expected,"compaction altered logical rows");check(inventory(f.root/"original")==source,"successful independent CLI modified source");}emit("owned-copy-independent-cli-success",{{"actualCliInvocations",calls-start},{"compactionLogicalRowsExact",true},{"migrationOnlySchemaChanged",true},{"migrationReplayWritesZero",true},{"originalBytesAndMaterialUnchanged",true},{"descendantDirectoriesAccepted",true},{"root0755Accepted",true}});}
// Each case contains a complete, otherwise accepted synthetic database. An
// irrelevant extra leaf therefore cannot hide a mutable RocksDB open.
void refused(const fs::path& p,unsigned mode,const char* name,const std::function<void(Fixture&)>& alter){uint64_t start=calls;for(bool migration:{false,true}){Fixture f(p,std::string(name)+(migration?"-migration":"-compaction"),migration);alter(f);const auto before=inventory(f.root);auto r=invoke(f);check(r.code==1,"unsafe physical copy was not refused");check(!r.err.empty(),"physical refusal diagnostic missing");check(inventory(f.root)==before,"refusal opened/mutated source, copy, marker or external alias");}emit(name,{{"caseId",mode},{"actualCliInvocations",calls-start},{"bothCliRefused",true},{"allBytesAndNineMaterialFieldsUnchanged",true},{"atimeCompared",false}});}
void marker_aliases(const fs::path& p){const auto start=calls;for(bool migration:{false,true})for(unsigned variant=0;variant<2;++variant){Fixture f(p,std::string("marker-alias-")+(migration?"migration-":"compact-")+std::to_string(variant),migration);if(variant==0)fs::create_hard_link(f.marker(),f.root/"external-marker");else{const auto bytes=read(f.marker());fs::remove(f.marker());file(f.root/"external-marker",bytes);fs::create_symlink(f.root/"external-marker",f.marker());}const auto before=inventory(f.root);const auto r=invoke(f);check(r.code==1&&!r.err.empty(),"marker alias was not refused");check(inventory(f.root)==before,"marker alias refusal changed source/copy/external marker");}emit("owned-copy-marker-alias-refusal",{{"caseId",8},{"actualCliInvocations",calls-start},{"bothCliRefused",true},{"allBytesAndNineMaterialFieldsUnchanged",true},{"atimeCompared",false},{"markerHardlinkAndSymlinkRefused",true}});}

void baseline(const fs::path& p,const std::string& compact,const std::string& migrate){for(bool migration:{false,true}){Fixture f(p,migration?"baseline-migrate":"baseline-compact",migration);file(f.root/"original/public-alias", "public synthetic alias bytes");fs::create_hard_link(f.root/"original/public-alias",f.copy/"public-alias");const auto original=inventory(f.root/"original");const auto r=invoke(f,migration?migrate:compact);check(r.code==0,"old CLI did not accept aliased synthetic copy: "+r.err);check(inventory(f.root/"original")==original,"baseline altered external alias unexpectedly");}emit("owned-copy-old-cli-alias-acceptance",{{"actualCliInvocations",calls},{"oldAcceptedPhysicallyAliasedCopy",true},{"originalBytesAndMaterialUnchanged",true},{"baselineConsumerObjectMayDiffer",true},{"correctedCodeQualified",false}});}

// Dedicated state-retention/reclaim scenario. Old physical-admission cases are not run.
uint64_t reclaim_fixture_signatures=0;
using ReclaimRows=std::map<std::string,Json>;
Json reclaim_row(std::string_view value){return {{"bytes",value.size()},{"sha256",digest(value)}};}
ReclaimRows reclaim_rows(const fs::path& p){
 auto db=open_db(p,false,true);rocksdb::ReadOptions o;o.verify_checksums=true;o.fill_cache=false;
 std::unique_ptr<rocksdb::Iterator> it(db->NewIterator(o));ReclaimRows rows;uint64_t bytes=0;
 for(it->SeekToFirst();it->Valid();it->Next()){
  bounded();const auto n=uint64_t(it->key().size())+uint64_t(it->value().size());
  check(rows.size()<4096&&n<=32*1024*1024-bytes,"reclaim logical scan budget");bytes+=n;
  check(rows.emplace(it->key().ToString(),reclaim_row(std::string_view(it->value().data(),it->value().size()))).second,"reclaim unique logical key");
 }check(it->status().ok(),"reclaim readonly logical scan");return rows;
}
std::map<std::string,std::string> reclaim_file_bytes(const Inventory& rows){std::map<std::string,std::string> out;for(const auto& [path,row]:rows)if(row.contains("sha256"))out.emplace(path,row.at("sha256").get<std::string>());return out;}
Json reclaim_compact(const fs::path& path){
 bounded();Process child({NANO_TEST_COMPACT,"--offline-owned-copy",path.string()});++calls;
 const auto result=child.wait(20);bounded();check(result.code==0&&result.err.empty(),"real compaction CLI must exit zero without stderr");
 const auto report=Json::parse(result.out);
 check(report.at("ownedCopyOnly")==true&&report.at("logicalContentUnchanged")==true&&report.at("logicalBefore")==report.at("logicalAfter"),"each CLI must preserve its complete logical commitment");
 for(const auto* phase:{"beforeFiles","afterFiles"})for(const auto* key:{"sstBytes","walBytes","apparentBytes","allocatedBytes"})
  check(report.at(phase).at(key).is_number_unsigned()&&report.at(phase).at(key).get<uint64_t>()<=96*1024*1024,"finite physical measurement budget");
 return report;
}
void reclaim_emit(const char* scenario,Json fields){bounded();fields["format"]="nano-pruning-sst-reclaim-scenario-v1";fields["scenario"]=scenario;fields["passed"]=true;fields["fixtureKeyPublic"]=true;fields["newOperationalSignaturesCreated"]=0;fields["runtimeDutiesResumed"]=false;fields["productionQualified"]=false;std::cout<<fields.dump()<<std::endl;}
void pruning_sst_reclaim(const fs::path& root){
 const ChainStoreMemoryPolicy policy{4*1024*1024,2,8*1024*1024,4*1024*1024};ScopedChainStoreMemoryPolicy memory(policy);
 constexpr unsigned blocks=64,slots=1024,per_batch=4;
 constexpr size_t checkpoint_budget=4*1024*1024;
 constexpr uint64_t minimum_sst=1024*1024,minimum_reclaim=64*1024;
 private_dir(root);private_dir(root/"baseline");private_dir(root/"control");private_dir(root/"pruned");
 const auto source=root/"baseline/chain",control=root/"control/chain",pruned=root/"pruned/chain";private_dir(source);
 file(root/"baseline/.nano-owned-compaction-copy","owned-offline-qualification-copy-v1\n");
 const Bytes public_key_bytes=bytes(Int(777),32);const auto sender=public_address(public_key(public_key_bytes));
 const std::string contract="0x00000000000000000000000000000000000000c1";
 Bytes code;const auto push2=[&](unsigned n){check(n<=65535,"fixture immediate bound");code.push_back(0x61);code.push_back(uint8_t(n>>8));code.push_back(uint8_t(n));};
 // Each fixed slot takes its new value from calldata; no raw state/DB writes.
 for(unsigned slot=0;slot<slots;++slot){push2(slot*32);code.push_back(0x35);push2(slot);code.push_back(0x55);}code.push_back(0x00);
 check(code.size()==8*slots+1&&code.size()<24576,"finite deployed fixture bytecode");
 auto genesis=Genesis::from_json({{"config",{{"chainId",1337},{"evmFork","Shanghai"}}},{"alloc",{{sender,{{"balance","0x100000000000000000000000"}}},{contract,{{"code",hex(code)}}}}}});
 std::vector<Bytes> block_hashes{genesis.block.header.hash()},state_roots{genesis.state.root()},tx_hashes;
 ForkCheckpointToken initial_token;Bytes tip;std::string checkpoint;
 {
  Blockchain chain(source,genesis);
  for(unsigned height=1;height<=blocks;++height){
   bounded();Transaction transaction;transaction.chain_id=1337;transaction.to=contract;transaction.nonce=Int(height-1);transaction.gas=29000000;transaction.gas_price=2000000000;
   for(unsigned slot=0;slot<slots;++slot){auto input=bytes(Int(height),8);const auto id=bytes(Int(slot),8);input.insert(input.end(),id.begin(),id.end());auto value=sha256(input);value[0]|=1;transaction.data.insert(transaction.data.end(),value.begin(),value.end());}
   const auto signed_transaction=SignedTransaction::sign(transaction,public_key_bytes);++reclaim_fixture_signatures;check(chain.submit(signed_transaction),"real fixture SSTORE transaction admission");
   const auto block=chain.mine(sender,Int(height*12));const auto receipt=chain.store().receipt(signed_transaction.hash());
   check(receipt&&receipt->success&&receipt->block_hash==block.header.hash()&&receipt->block_number==Int(height),"every fixture SSTORE transaction must execute successfully");
   const auto state=chain.state();check(state.get(contract).storage.size()==slots,"genuine SSTORE state must contain the exact live slot count");for(unsigned slot=0;slot<slots;++slot)check(state.get_storage(contract,Int(slot))==integer(Bytes(transaction.data.begin()+slot*32,transaction.data.begin()+(slot+1)*32)),"every genuine committed SSTORE value must match calldata");
   block_hashes.push_back(block.header.hash());state_roots.push_back(state.root());tx_hashes.push_back(signed_transaction.hash());
  }
  tip=block_hashes.back();checkpoint=Json{{"format","synthetic-sst-retention-checkpoint-v1"},{"head",hex(tip)},{"stateRoot",hex(state_roots.back())}}.dump();
  auto& disk=const_cast<ChainStore&>(chain.store());initial_token=disk.commit_execution_checkpoint(1337,genesis.block.header.hash(),{},tip,blocks,tip,{},checkpoint,checkpoint_budget,0);
 }
 const auto baseline_report=reclaim_compact(source);check(baseline_report.at("afterFiles").at("sstBytes").get<uint64_t>()>=minimum_sst,"predeclared baseline SST must reach one MiB");
 const auto original_rows=reclaim_rows(source);const auto original=inventory(root/"baseline");
 fs::copy(source,control,fs::copy_options::recursive);fs::copy(source,pruned,fs::copy_options::recursive);
 for(const auto& path:{control,pruned}){check(::chmod(path.c_str(),0700)==0,"new reclaim copy mode");file(path.parent_path()/".nano-owned-compaction-copy","owned-offline-qualification-copy-v1\n");check(reclaim_file_bytes(inventory(path))==reclaim_file_bytes(inventory(source)),"new copies must contain the complete original file bytes");check(reclaim_rows(path)==original_rows,"new copies must have the same complete logical rows");}
 reclaim_emit("real-sstore-and-compacted-baseline",{{"blocks",blocks},{"fixtureTransactions",blocks},{"fixtureEcdsaSignatures",blocks},{"sstoreWrites",blocks*slots},{"liveStorageSlots",slots},{"baselineCli",baseline_report}});
 const auto batches=(blocks-1+per_batch-1)/per_batch;ForkCheckpointToken control_token=initial_token,pruned_token=initial_token;unsigned deleted=0;
 for(bool apply:{false,true}){
  const auto path=apply?pruned:control;auto& token=apply?pruned_token:control_token;
  ChainStore disk(path,ChainStore::OpenMode::ReadWriteExisting);const auto initial=disk.fork_checkpoint(1337,genesis.block.header.hash(),checkpoint_budget,initial_token);check(initial&&initial->snapshot==checkpoint,"fresh copy must match the independently retained checkpoint");
  for(unsigned batch=0;batch<batches;++batch){bounded();const auto previous=token;
   token=disk.commit_execution_checkpoint(1337,genesis.block.header.hash(),token,tip,blocks,tip,{},checkpoint,checkpoint_budget,per_batch,{},apply?std::optional(ExecutionStateRetentionAnchor{blocks,tip}):std::nullopt);
   check(token.sequence==previous.sequence+1&&token.digest==initial_token.digest,"each control/retention publication must advance the same exact token sequence");
   const auto marker=disk.state_pruning();const auto before=std::min(blocks,1+(batch+1)*per_batch);
   check(apply?(marker&&marker->before==Int(before)&&marker->anchor.number==Int(blocks)&&marker->anchor.hash==tip):!marker,"retention must progress by the exact bounded frontier only");
   if(apply){const auto start=1+batch*per_batch;for(unsigned height=start;height<before;++height)check(!disk.state_at(Int(height)),"each bounded batch must remove exactly its newly obsolete prefix");check(disk.state_at(0)&&disk.state_at(Int(before)),"genesis and the retained frontier must survive every batch");}else check(disk.state_at(0)&&disk.state_at(1),"matched control must retain genesis and its oldest historical state");
   const auto head=disk.latest();const auto live=disk.state_at(blocks);check(head&&live&&head->header.hash()==tip&&live->root()==state_roots.back(),"retention must not change the canonical head or live state");
   if(apply)deleted=before-1;
  }
 }
 check(deleted==blocks-1&&control_token==pruned_token,"paired copies must differ by retention, never token publication count");
 auto expected_control=original_rows;expected_control["fork-checkpoint/v1/token"]=reclaim_row(Json{{"sequence",quantity(Int(std::to_string(control_token.sequence)))},{"digest",hex(control_token.digest)}}.dump());
 check(reclaim_rows(control)==expected_control,"control publications must change only their legitimate token bytes");
 auto expected_pruned=expected_control;for(unsigned height=1;height<blocks;++height)check(expected_pruned.erase("state/"+hex(bytes(Int(height),8)))==1,"every planned historical state must exist exactly once");
 expected_pruned["meta/schema"]=reclaim_row("nano/2-state-pruned");expected_pruned["execution-state-pruning/v1"]=reclaim_row(Json{{"before",quantity(Int(blocks))},{"anchorNumber",quantity(Int(blocks))},{"anchorHash",hex(tip)}}.dump());
 check(reclaim_rows(pruned)==expected_pruned,"all non-state rows and exact checkpoint bindings must be preserved except the schema/retention marker");
 reclaim_emit("bounded-retention-and-matched-control",{{"retentionBatches",batches},{"controlCheckpointBatches",batches},{"maximumDeletedStatesPerBatch",per_batch},{"deletedStates",deleted},{"retentionBefore",blocks},{"retentionAnchor",blocks},{"allOtherLogicalRowsCommitted",true}});
 const auto control_report=reclaim_compact(control),pruned_report=reclaim_compact(pruned);
 check(reclaim_rows(control)==expected_control&&reclaim_rows(pruned)==expected_pruned,"physical compaction must preserve each exact expected logical table");
 for(bool retained:{false,true}){
  ChainStore disk(retained?pruned:control,ChainStore::OpenMode::ReadOnly);const auto token=retained?pruned_token:control_token;
  const auto reopened=disk.fork_checkpoint(1337,genesis.block.header.hash(),checkpoint_budget,token);check(reopened&&reopened->snapshot==checkpoint,"cold reopen must verify the original independent post-retention token");
  for(unsigned height=0;height<=blocks;++height){const auto state=disk.state_at(Int(height));check(bool(state)==(!retained||height==0||height==blocks),"cold state frontier after compaction");if(state)check(state->root()==state_roots[height],"each surviving state root must match its actual executed history");const auto block=disk.block_by_number(Int(height));check(block&&block->header.hash()==block_hashes[height],"compaction must preserve every canonical block");if(height){const auto transaction=disk.transaction(tx_hashes[height-1]);const auto receipt=disk.receipt(tx_hashes[height-1]);check(transaction&&receipt&&receipt->success&&receipt->block_hash==block_hashes[height],"compaction must preserve every real transaction and receipt");}}
 }
 check(inventory(root/"baseline")==original,"original normalized source files and material must remain unchanged");
 const auto cs=control_report.at("afterFiles").at("sstBytes").get<uint64_t>(),ps=pruned_report.at("afterFiles").at("sstBytes").get<uint64_t>();
 const auto ca=control_report.at("afterFiles").at("allocatedBytes").get<uint64_t>(),pa=pruned_report.at("afterFiles").at("allocatedBytes").get<uint64_t>();
 check(cs>=ps&&cs-ps>=minimum_reclaim,"paired SST reclaim must meet the predeclared 64 KiB threshold");
 check(ca>=pa&&ca-pa>=minimum_reclaim,"paired allocated-file reclaim must meet the predeclared 64 KiB threshold");
 reclaim_emit("sst-reclaim-and-cold-logical-invariants",{{"controlCli",control_report},{"prunedCli",pruned_report},{"sstReclaimedBytes",cs-ps},{"allocatedFileBytesReclaimed",ca-pa},{"originalSourceUnchanged",true},{"coldStateRootsAndReceiptsVerified",true},{"filesystemFileAllocationOnly",true},{"hostOrDockerBackingSpaceGuaranteed",false}});
 bounded();check(reclaim_fixture_signatures==blocks,"exact executed fixture signature count");std::cout<<Json{{"format","nano-pruning-sst-reclaim-v1"},{"passed",true},{"scenarioRows",3},{"actualCliInvocations",calls},{"fixtureBlocks",blocks},{"fixtureTransactions",blocks},{"fixtureEcdsaSignatures",blocks},{"fixtureKeyPublic",true},{"sstoreWrites",blocks*slots},{"liveStorageSlots",slots},{"retentionBatches",batches},{"controlCheckpointBatches",batches},{"deletedStates",deleted},{"logicalMaximumBytes",32*1024*1024},{"logicalMaximumKeys",4096},{"fixtureWriteBufferBytes",4*1024*1024},{"fixtureSharedMemtableBytes",8*1024*1024},{"fixtureBlockCacheBytes",4*1024*1024},{"minimumBaselineSstBytes",minimum_sst},{"minimumReclaimBytes",minimum_reclaim},{"sstReclaimedBytes",cs-ps},{"allocatedFileBytesReclaimed",ca-pa},{"elapsedMs",elapsed()},{"maxRssKiB",rss()},{"rawStateDbWritesOrDeletes",false},{"oldPhysicalAdmissionCasesReplayed",false},{"storageSourceModified",false},{"runtimeStarted",false},{"networkUsed",false},{"physicalPowerLossQualified",false},{"fullEvmPruningQualified",false},{"productionQualified",false}}.dump()<<std::endl;
}
}
int main(int argc,char** argv){try{Temp temp;check(::chmod(temp.path.c_str(),0700)==0,"private test root");if(argc==2&&std::string(argv[1])=="--pruning-sst-reclaim"){pruning_sst_reclaim(temp.path/"sst-reclaim");return 0;}if(argc==4&&std::string(argv[1])=="--baseline-aliased-cli"){baseline(temp.path,argv[2],argv[3]);return 0;}check(argc==2&&std::string(argv[1])=="--self-test","usage: --self-test or --baseline-aliased-cli OLD-COMPACT OLD-MIGRATE");independent(temp.path);
 refused(temp.path,1,"owned-copy-external-hardlink-refusal",[](Fixture& f){fs::remove(f.copy/"CURRENT");fs::create_hard_link(f.source/"CURRENT",f.copy/"CURRENT");});
 refused(temp.path,2,"owned-copy-internal-hardlink-refusal",[](Fixture& f){fs::create_hard_link(f.copy/"CURRENT",f.copy/"second-current");});
 refused(temp.path,3,"owned-copy-symlink-refusal",[](Fixture& f){fs::create_symlink(f.source/"CURRENT",f.copy/"unexpected-link");});
 refused(temp.path,4,"owned-copy-special-file-refusal",[](Fixture& f){check(::mkfifo((f.copy/"unexpected-fifo").c_str(),0600)==0,"owned FIFO fixture");});
 refused(temp.path,5,"owned-copy-directory-current-refusal",[](Fixture& f){fs::remove(f.copy/"CURRENT");private_dir(f.copy/"CURRENT");});
 refused(temp.path,6,"owned-copy-descendant-hardlink-refusal",[](Fixture& f){private_dir(f.copy/"nested");file(f.root/"original/public-leaf","public synthetic nested alias");fs::create_hard_link(f.root/"original/public-leaf",f.copy/"nested/public-leaf");});
 refused(temp.path,7,"owned-copy-lock-hardlink-refusal",[](Fixture& f){check(fs::is_regular_file(f.copy/"LOCK"),"real RocksDB LOCK present");fs::create_hard_link(f.copy/"LOCK",f.root/"external-lock");});
 marker_aliases(temp.path);
 emit("owned-copy-physical-admission-summary",{{"scenarioRows",9},{"actualCliInvocations",calls},{"unsafeCliRefusals",18},{"successfulCliInvocations",3},{"oldTestsReplayed",false},{"liveWriterOrSameUidAdversaryQualified",false},{"sourceCopyIndependenceQualified",true},{"runtimeStarted",false},{"networkUsed",false}});return 0;
}catch(const std::exception& e){if(argc==2&&std::string(argv[1])=="--pruning-sst-reclaim"){std::cout<<Json{{"format","nano-pruning-sst-reclaim-v1"},{"passed",false},{"reason",e.what()},{"actualCliInvocations",calls},{"fixtureEcdsaSignatures",reclaim_fixture_signatures},{"newOperationalSignaturesCreated",0},{"productionQualified",false}}.dump()<<std::endl;return 1;}std::cout<<Json{{"diagnostic","owned-copy-physical-admission-refused"},{"reason",e.what()},{"actualCliInvocations",calls},{"newSignaturesCreated",0},{"privateInputsRead",false},{"productionQualified",false}}.dump()<<std::endl;return 1;}}
