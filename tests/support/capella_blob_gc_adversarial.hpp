#pragma once
#include "nano/checkpoint_blob_gc.hpp"
#include <poll.h>
#include <sys/wait.h>
#include <csignal>
#include <thread>
// Adversarial tests use real, short, closed Capella checkpoint pairs. Every
// mutation below targets a NEW synthetic source clone, never the shared fixture.
static std::string gc_case_logical(const fs::path& path){
 rocksdb::Options options;options.create_if_missing=false;std::unique_ptr<rocksdb::DB> db;
 // API detection, as in the existing test helpers; no schema interpretation.
 const auto open_ro=[]<class DB=rocksdb::DB>(const rocksdb::Options& o,const std::string& p){std::unique_ptr<DB> out;if constexpr(requires{DB::OpenForReadOnly(o,p,&out);})check(DB::OpenForReadOnly(o,p,&out).ok(),"GC test RO open");else{DB* raw=nullptr;auto result=DB::OpenForReadOnly(o,p,&raw);out.reset(raw);check(result.ok(),"GC test RO open");}return out;};
 db=open_ro(options,path.string());rocksdb::ReadOptions read;read.verify_checksums=true;std::unique_ptr<rocksdb::Iterator> it(db->NewIterator(read));Json rows=Json::array();
 for(it->SeekToFirst();it->Valid();it->Next())rows.push_back(Json::array({it->key().ToString(),it->value().ToString()}));check(it->status().ok(),"GC test iterator status");const auto encoded=rows.dump();return hex(sha256(Bytes(encoded.begin(),encoded.end())));
}
static bool gc_case_has(const fs::path& path,const std::string& key){
 rocksdb::Options options;options.create_if_missing=false;
 const auto ro=[]<class DB=rocksdb::DB>(const rocksdb::Options& o,const std::string& p){std::unique_ptr<DB> out;if constexpr(requires{DB::OpenForReadOnly(o,p,&out);})check(DB::OpenForReadOnly(o,p,&out).ok(),"GC boundary RO open");else{DB* raw=nullptr;auto result=DB::OpenForReadOnly(o,p,&raw);out.reset(raw);check(result.ok(),"GC boundary RO open");}return out;};
 auto db=ro(options,path.string());std::string value;const auto result=db->Get(rocksdb::ReadOptions(),key,&value);if(result.IsNotFound())return false;check(result.ok(),"GC boundary read");return true;
}
static void gc_case_mutate(const fs::path& path,const std::function<void(rocksdb::DB&)>& change){rocksdb::Options options;options.create_if_missing=false;auto db=open_corruption_database(options,path.string());change(*db);}
static std::string gc_live_key(const fs::path& path,const Genesis& g,const CapellaStoreSnapshotLimits& limits){
 ChainStore disk(path,ChainStore::OpenMode::ReadOnly);const auto checkpoint=disk.fork_checkpoint(g.chain_id,g.block.header.hash(),limits.envelope_bytes);check(bool(checkpoint),"GC short fixture checkpoint missing");const auto outer=Json::parse(checkpoint->snapshot);const auto& durable=outer.at("checkpoint");const auto& store=durable.contains("store")?durable.at("store"):durable;const auto& rows=store.at("snapshot").at("blocks");check(!rows.empty(),"GC short fixture has no live refs");return "checkpoint-blob/v1/"+rows[0][1].get<std::string>();
}
struct GcAdverseCase {
 Temp temporary;fs::path source,witness_source,primary,witness;Genesis genesis;CapellaForkBlockRules rules;CapellaStoreSnapshotLimits limits;CheckpointBlobGcBudget budget;
 std::string stopped,request;Json request_document;Bytes stopped_sha,request_sha;
 GcAdverseCase(const fs::path& a,const fs::path& b,const Genesis& g,const CapellaForkBlockRules& r,const CapellaStoreSnapshotLimits& l):source(fs::canonical(temporary.path)/"source"),witness_source(fs::canonical(temporary.path)/"source-witness"),primary(fs::canonical(temporary.path)/"copy"),witness(fs::canonical(temporary.path)/"copy-witness"),genesis(g),rules(r),limits(l),budget{1024,128*1024*1024,16384,128*1024*1024,1024,64*1024*1024,1024*1024,60000}{
  temporary.path=fs::canonical(temporary.path);
  fs::copy(a,source,fs::copy_options::recursive);fs::copy(b,witness_source,fs::copy_options::recursive);check(::chmod(source.c_str(),0700)==0&&::chmod(witness_source.c_str(),0700)==0,"GC adverse source permissions");
 }
 Json encoded_budget()const{return Json::array({quantity(Int(std::to_string(budget.inventory_files))),quantity(Int(std::to_string(budget.inventory_bytes))),quantity(Int(std::to_string(budget.scan_records))),quantity(Int(std::to_string(budget.scan_bytes))),quantity(Int(std::to_string(budget.delete_records))),quantity(Int(std::to_string(budget.delete_bytes))),quantity(Int(std::to_string(budget.batch_bytes))),quantity(Int(std::to_string(budget.duration_ms)))});}
 void repin(){request=request_document.dump();request_sha=sha256(Bytes(request.begin(),request.end()));}
 void seal(){
  ForkCheckpointToken pp,wp;Json next;
  const auto closed=[&](const fs::path& path,bool recovery){ForkCheckpointToken result;gc_case_mutate(path,[&](rocksdb::DB& db){std::string t,payload;check(db.Get(rocksdb::ReadOptions(),"fork-checkpoint/v1/token",&t).ok()&&db.Get(rocksdb::ReadOptions(),"fork-checkpoint/v1/payload",&payload).ok(),"GC adverse source pins missing");const auto row=Json::parse(t);result={u64(quantity(row.at("sequence"))),unhex(row.at("digest").get<std::string>())};if(recovery)next=Json::parse(payload).at("next");});return result;};pp=closed(source,false);wp=closed(witness_source,true);
  fs::copy(source,primary,fs::copy_options::recursive);fs::copy(witness_source,witness,fs::copy_options::recursive);check(::chmod(primary.c_str(),0700)==0&&::chmod(witness.c_str(),0700)==0,"GC adverse copy permissions");{std::ofstream file(temporary/".nano-owned-checkpoint-gc-copy");file<<"owned-offline-checkpoint-gc-v1\n";check(bool(file),"GC adverse marker");}

  const auto pi=checkpoint_blob_gc_copy_inventory(source,budget.inventory_files,budget.inventory_bytes),wi=checkpoint_blob_gc_copy_inventory(witness_source,budget.inventory_files,budget.inventory_bytes);
  check(checkpoint_blob_gc_copy_inventory(primary,budget.inventory_files,budget.inventory_bytes)==pi&&checkpoint_blob_gc_copy_inventory(witness,budget.inventory_files,budget.inventory_bytes)==wi,"GC adverse copy exactness");
  const auto time=quantity(Int(std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())));
  stopped=Json{{"format","nano-stopped-checkpoint-copy-receipt-v1"},{"scope","isolated-owned-copies"},{"allWritersStopped",true},{"allReadersStopped",true},{"stoppedAtMs",time},{"copies",Json::array({Json{{"role","primary"},{"path",primary.string()},{"inventorySha256",pi},{"sourcePath",source.string()},{"sourceInventorySha256",pi}},Json{{"role","witness"},{"path",witness.string()},{"inventorySha256",wi},{"sourcePath",witness_source.string()},{"sourceInventorySha256",wi}}})}}.dump();stopped_sha=sha256(Bytes(stopped.begin(),stopped.end()));const auto profile=capella_fork_profile(rules).dump();
  request_document={{"format","nano-owned-stopped-checkpoint-gc-v1"},{"chainId",quantity(genesis.chain_id)},{"genesisHash",hex(genesis.block.header.hash())},{"profileSha256",hex(sha256(Bytes(profile.begin(),profile.end()))).substr(2)},
   {"primary",{{"path",primary.string()},{"inventorySha256",pi},{"token",gc_token(pp)},{"sourcePath",source.string()},{"sourceInventorySha256",pi}}},{"witness",{{"path",witness.string()},{"inventorySha256",wi},{"token",gc_token(wp)},{"sourcePath",witness_source.string()},{"sourceInventorySha256",wi}}},{"next",next},{"stopReceiptSha256",hex(stopped_sha).substr(2)},{"limits",gc_limits(limits)},{"budget",encoded_budget()}};repin();
 }
 std::unique_ptr<CheckpointBlobGcCopies> open(){return CheckpointBlobGcCopies::open(request,request_sha,stopped,stopped_sha,budget);}
};
static void gc_adverse_intent(const fs::path& path,const std::function<void(Json&)>& change){
 gc_case_mutate(path,[&](rocksdb::DB& db){std::string bytes,encoded;check(db.Get(rocksdb::ReadOptions(),"fork-checkpoint/v1/payload",&bytes).ok()&&db.Get(rocksdb::ReadOptions(),"fork-checkpoint/v1/token",&encoded).ok(),"GC adverse intent read");auto intent=Json::parse(bytes);change(intent);bytes=intent.dump();auto t=Json::parse(encoded);t["digest"]=hex(sha256(Bytes(bytes.begin(),bytes.end())));rocksdb::WriteOptions write;write.sync=true;check(db.Put(write,"fork-checkpoint/v1/payload",bytes).ok()&&db.Put(write,"fork-checkpoint/v1/token",t.dump()).ok(),"GC adverse intent staging");});
}
static int gc_crash_child(int argc,char** argv){
 check(argc==9,"GC crash-child exact arguments");const auto load=[](const fs::path& path){check(fs::is_regular_file(path)&&!fs::is_symlink(path)&&fs::file_size(path)<=1024*1024,"GC child bounded public input");std::ifstream stream(path,std::ios::binary);std::string bytes;char block[4096];while(stream){stream.read(block,sizeof(block));bytes.append(block,size_t(stream.gcount()));check(bytes.size()<=1024*1024,"GC child input grew");}check(!stream.bad(),"GC child input read");return bytes;};
 const auto digest=[](const std::string& text){check(text.size()==64&&text.find_first_not_of("0123456789abcdef")==std::string::npos,"GC child canonical pin");return unhex("0x"+text);};
 const auto request=load(argv[2]),stopped=load(argv[4]),profile=load(argv[6]);const auto pin=digest(argv[3]),stop_pin=digest(argv[5]),profile_pin=digest(argv[7]);const auto rules=load_capella_fork_profile(profile,profile_pin,1024*1024);const auto parsed=Json::parse(request);const auto budget=checkpoint_blob_gc_budget(parsed.at("budget"));const auto limits=checkpoint_blob_gc_snapshot_limits(parsed.at("limits"));
 const auto descriptor=std::stoul(argv[8]);check(descriptor<=uint64_t(INT_MAX)&&std::to_string(descriptor)==argv[8],"GC child descriptor encoding");const int output=int(descriptor);
 auto owned=CheckpointBlobGcCopies::open(request,pin,stopped,stop_pin,budget);const auto plan=owned->plan(rules,limits,budget);const auto described=plan.description();check(!described.at("deletePrimary").empty()&&!described.at("deleteWitness").empty(),"GC kill fixture has no candidates");const auto pk=described.at("deletePrimary")[0][0].get<std::string>(),wk=described.at("deleteWitness")[0][0].get<std::string>();const fs::path primary=parsed.at("primary").at("path").get<std::string>(),witness=parsed.at("witness").at("path").get<std::string>();
 // The callback also runs for every preflight inventory chunk. Opening new
 // RocksDB readers at each such callback can exhaust the parent's 30s bound.
 // Only WAL growth/new files trigger a fresh logical check; no reader is cached.
 const auto primary_wals=[&]{std::map<std::string,uint64_t> sizes;
  for(const auto& entry:fs::directory_iterator(primary)){const auto name=entry.path().filename().string();if(name.size()<=4||name.substr(name.size()-4)!=".log"||name.substr(0,name.size()-4).find_first_not_of("0123456789")!=std::string::npos)continue;
   struct stat metadata{};check(::lstat(entry.path().c_str(),&metadata)==0&&S_ISREG(metadata.st_mode)&&metadata.st_size>=0,"GC child WAL metadata");check(sizes.emplace(name,uint64_t(metadata.st_size)).second&&sizes.size()<=budget.inventory_files,"GC child WAL inventory bound");
  }check(!sizes.empty(),"GC child active primary WAL required");return sizes;
 };
 auto observed_wals=primary_wals();uint64_t callbacks=0,logical_checks=0;
 std::cerr<<Json{{"scenario","gc-crash-child-phase"},{"phase","planned-before-delete"},{"primaryWalFiles",observed_wals.size()},{"logicalChecks",logical_checks},{"productionQualified",false}}.dump()<<std::endl;
 (void)owned->collect(plan,[&]{++callbacks;const auto current=primary_wals();bool grew=false;for(const auto& [name,size]:current){const auto old=observed_wals.find(name);if(old==observed_wals.end()||size>old->second){grew=true;break;}}observed_wals=current;if(!grew)return;
  ++logical_checks;const bool primary_removed=!gc_case_has(primary,pk);const bool witness_retained=gc_case_has(witness,wk);
  std::cerr<<Json{{"scenario","gc-crash-child-phase"},{"phase","wal-change-logical-check"},{"callbacks",callbacks},{"logicalChecks",logical_checks},{"primaryOrphanAbsent",primary_removed},{"witnessOrphanPresent",witness_retained},{"productionQualified",false}}.dump()<<std::endl;
  if(primary_removed&&witness_retained){const char ready='P';check(::write(output,&ready,1)==1,"GC child phase publication");::raise(SIGSTOP);throw std::runtime_error("GC child unexpectedly resumed");}
 });throw std::runtime_error("GC child missed between-store crash boundary");
}
static void qualify_capella_gc_adversarial(const fs::path& baseline_primary,const fs::path& baseline_witness,
 const Genesis& g,const CapellaForkBlockRules& rules,const CapellaStoreSnapshotLimits& limits,const Bytes& child){
 check(!test::isolated_capella::active,"GC adverse fixture cannot use campaign data");Json passed=Json::array();
 const auto unchanged_refusal=[&](GcAdverseCase& c,const char* id,const char* expected,const std::function<void()>& action){
  const auto p=gc_case_logical(c.primary),w=gc_case_logical(c.witness);bool refused=false;
  try{action();}catch(const std::exception& error){check(std::string(error.what()).find(expected)!=std::string::npos,std::string(id)+" failed at unexpected boundary: "+error.what());refused=true;}
  check(refused&&gc_case_logical(c.primary)==p&&gc_case_logical(c.witness)==w,std::string(id)+" admitted or changed logical contents");passed.push_back(id);
 };
 const auto ordinary=[&](GcAdverseCase& c){auto owned=c.open();const auto plan=owned->plan(rules,limits,c.budget);(void)owned->collect(plan);};
 // Authenticity boundary: complete root decoders, not a free list of live hashes.
 for(unsigned fault=0;fault<4;++fault){GcAdverseCase c(baseline_primary,baseline_witness,g,rules,limits);const auto path=fault<2?c.source:c.witness_source;const auto key=gc_live_key(c.source,g,limits);
  gc_case_mutate(path,[&](rocksdb::DB& db){rocksdb::WriteOptions write;write.sync=true;check((fault%2?db.Put(write,key,"corrupt-live-row"):db.Delete(write,key)).ok(),"GC adverse live ref mutation");});c.seal();
  unchanged_refusal(c,fault<2?(fault%2?"primary-corrupt-live-ref":"primary-missing-live-ref"):(fault%2?"witness-corrupt-live-ref":"witness-missing-live-ref"),fault%2?"checkpoint GC corrupt referenced blob":"Invalid Capella durable archive",[&]{ordinary(c);});
 }
 {GcAdverseCase c(baseline_primary,baseline_witness,g,rules,limits);gc_adverse_intent(c.witness_source,[](Json& j){const auto n=u64(quantity(j.at("next").at("sequence")));j["next"]["sequence"]=quantity(Int(std::to_string(n+2)));});c.seal();unchanged_refusal(c,"pending-sequence-divergence","checkpoint GC pending-next external binding mismatch",[&]{ordinary(c);});}
 {GcAdverseCase c(baseline_primary,baseline_witness,g,rules,limits);gc_case_mutate(c.source,[&](rocksdb::DB& db){std::string bytes;check(db.Get(rocksdb::ReadOptions(),"fork-checkpoint/v1/token",&bytes).ok(),"GC primary token read");auto j=Json::parse(bytes);j["sequence"]=quantity(Int(std::to_string(u64(quantity(j.at("sequence")))+10)));rocksdb::WriteOptions write;write.sync=true;check(db.Put(write,"fork-checkpoint/v1/token",j.dump()).ok(),"GC divergence staging");});c.seal();unchanged_refusal(c,"primary-neither-previous-nor-next","checkpoint GC primary is neither completed nor pending previous",[&]{ordinary(c);});}
 {GcAdverseCase c(baseline_primary,baseline_witness,g,rules,limits);c.seal();c.request_document["witness"]["token"]["digest"]="0x"+std::string(64,'a');c.repin();unchanged_refusal(c,"wrong-independent-witness-pin","checkpoint GC independently pinned primary/witness mismatch",[&]{ordinary(c);});}
 {GcAdverseCase c(baseline_primary,baseline_witness,g,rules,limits);c.seal();c.request_document["next"]["digest"]="0x"+std::string(64,'b');c.repin();unchanged_refusal(c,"wrong-independent-pending-next-pin","checkpoint GC pending-next external binding mismatch",[&]{ordinary(c);});}
 {GcAdverseCase c(baseline_primary,baseline_witness,g,rules,limits);c.seal();c.request_sha[0]^=1;unchanged_refusal(c,"request-hash-before-parse","checkpoint GC request pin mismatch",[&]{ordinary(c);});}
 {GcAdverseCase c(baseline_primary,baseline_witness,g,rules,limits);gc_case_mutate(c.source,[&](rocksdb::DB& db){std::string bytes;check(db.Get(rocksdb::ReadOptions(),"meta/identity",&bytes).ok(),"GC role identity missing");auto identity=Json::parse(bytes);identity["consensus"]="nano-beacon-signing-v1";rocksdb::WriteOptions write;write.sync=true;check(db.Put(write,"meta/identity",identity.dump()).ok(),"GC role staging");});c.seal();unchanged_refusal(c,"signing-store-role-forbidden","checkpoint GC store role refused",[&]{ordinary(c);});}
 {GcAdverseCase c(baseline_primary,baseline_witness,g,rules,limits);gc_case_mutate(c.source,[&](rocksdb::DB& db){rocksdb::WriteOptions write;write.sync=true;check(db.Put(write,"meta/schema","nano/future-unknown").ok(),"GC unknown schema staging");});c.seal();unchanged_refusal(c,"future-schema-refused","unsupported Nano database schema",[&]{ordinary(c);});}
 {GcAdverseCase c(baseline_primary,baseline_witness,g,rules,limits);c.seal();auto stopped=Json::parse(c.stopped);stopped["allReadersStopped"]=false;c.stopped=stopped.dump();c.stopped_sha=sha256(Bytes(c.stopped.begin(),c.stopped.end()));c.request_document["stopReceiptSha256"]=hex(c.stopped_sha).substr(2);c.repin();unchanged_refusal(c,"readers-not-stopped","checkpoint GC externally pinned stopped-copy receipt required",[&]{ordinary(c);});}
 // Resource/preflight boundary: complete scan and deletion budgets, no Delete.
 for(unsigned fault=0;fault<3;++fault){GcAdverseCase c(baseline_primary,baseline_witness,g,rules,limits);if(fault==0)c.budget.scan_records=1;else if(fault==1)c.budget.delete_records=1;else c.budget.batch_bytes=8;c.seal();unchanged_refusal(c,fault==0?"complete-scan-budget":fault==1?"total-delete-budget":"single-batch-budget",fault==0?"checkpoint GC scan budget exhausted":fault==1?"checkpoint GC delete budget exhausted":"checkpoint GC single Delete exceeds batch budget",[&]{ordinary(c);});}
 {GcAdverseCase c(baseline_primary,baseline_witness,g,rules,limits);c.budget.duration_ms=uint64_t(INT64_MAX);c.seal();auto owned=c.open();const auto plan=owned->plan(rules,limits,c.budget);check(!plan.description().at("deletePrimary").empty(),"large finite deadline overflowed or skipped a complete plan");passed.push_back("large-finite-duration-no-timepoint-overflow");}
 {GcAdverseCase c(baseline_primary,baseline_witness,g,rules,limits);c.seal();auto owned=c.open();unsigned calls=0;unchanged_refusal(c,"inventory-cancellation","inventory-cancel-test",[&]{(void)checkpoint_blob_gc_copy_inventory(c.source,c.budget.inventory_files,c.budget.inventory_bytes,[&]{++calls;throw ExecutionCancelled("inventory-cancel-test");});});check(calls==1,"inventory callback not reached at first scanned entry");}
 // Existing-only acquisition never creates a missing DB or initializes an
 // empty existing one; the legacy default remains outside this maintenance API.
 {Temp t;const auto absent=t/"absent-existing-db";bool refused=false;try{ChainStore missing(absent,ChainStore::OpenMode::ReadWriteExisting);}catch(const std::runtime_error&){refused=true;}check(refused&&!fs::exists(absent),"existing maintenance mode created absent DB");
  // A successful closed read-only probe must not authorize recreation if its
  // private synthetic database disappears before maintenance acquisition.
  {const auto probed=t/"probed-existing-db",moved=t/"moved-probed-db";fs::copy(baseline_primary,probed,fs::copy_options::recursive);check(::chmod(probed.c_str(),0700)==0,"existing probe permissions");
   {ChainStore probe(probed,ChainStore::OpenMode::ReadOnly);}const auto before=gc_case_logical(probed);fs::rename(probed,moved);bool vanished_refused=false;
   try{ChainStore vanished(probed,ChainStore::OpenMode::ReadWriteExisting);}catch(const std::runtime_error&){vanished_refused=true;}
   check(vanished_refused&&!fs::exists(probed)&&gc_case_logical(moved)==before,"existing mode recreated database after read-only probe disappearance");
  }passed.push_back("existing-mode-never-creates-missing-db");}
 {Temp t;const auto empty=t/"existing-empty-db";{rocksdb::Options options;options.create_if_missing=true;auto db=open_corruption_database(options,empty.string());}bool refused=false;try{ChainStore missing_schema(empty,ChainStore::OpenMode::ReadWriteExisting);}catch(const std::runtime_error& error){refused=std::string(error.what()).find("existing database has no Nano schema")!=std::string::npos;}check(refused,"existing mode initialized schema or failed at wrong boundary");gc_case_mutate(empty,[&](rocksdb::DB& db){std::string value;check(db.Get(rocksdb::ReadOptions(),"meta/schema",&value).IsNotFound(),"existing mode wrote a missing schema");});passed.push_back("existing-mode-never-initializes-missing-schema");}
 // Physical authority boundary; exclusive RW session does not accept replacement.
 {GcAdverseCase c(baseline_primary,baseline_witness,g,rules,limits);c.seal();fs::remove(c.primary/"CURRENT");fs::create_hard_link(c.source/"CURRENT",c.primary/"CURRENT");unchanged_refusal(c,"hardlinked-original-copy","checkpoint GC shared hardlink refused",[&]{ordinary(c);});}
 {GcAdverseCase c(baseline_primary,baseline_witness,g,rules,limits);c.seal();fs::remove(c.primary/"CURRENT");fs::create_symlink(c.source/"CURRENT",c.primary/"CURRENT");unchanged_refusal(c,"symlinked-copy-file","checkpoint GC source links/special files/owner refused",[&]{ordinary(c);});}
 {GcAdverseCase c(baseline_primary,baseline_witness,g,rules,limits);c.seal();auto owned=c.open();const auto plan=owned->plan(rules,limits,c.budget);const auto old=c.temporary/"renamed-held-copy";fs::rename(c.primary,old);fs::copy(c.source,c.primary,fs::copy_options::recursive);check(::chmod(c.primary.c_str(),0700)==0,"GC stale path permissions");const auto held_before=gc_case_logical(old);unchanged_refusal(c,"stale-plan-directory-replaced","checkpoint GC copy directory changed",[&]{(void)owned->collect(plan);});owned.reset();check(gc_case_logical(old)==held_before,"stale plan deleted from held replaced directory");}
 {GcAdverseCase c(baseline_primary,baseline_witness,g,rules,limits);gc_case_mutate(c.source,[&](rocksdb::DB& db){rocksdb::WriteOptions write;write.sync=true;check(db.Put(write,"checkpoint-blob/v2/unknown-consumer","future-opaque").ok(),"GC future namespace staging");});c.seal();unchanged_refusal(c,"unknown-future-blob-namespace","checkpoint GC unknown blob namespace",[&]{ordinary(c);});}
 {GcAdverseCase c(baseline_primary,baseline_witness,g,rules,limits);gc_case_mutate(c.source,[&](rocksdb::DB& db){rocksdb::WriteOptions write;write.sync=true;check(db.Put(write,"application/outside-gc-prefix","keep-this-application-value").ok(),"GC nonprefix key staging");});c.seal();ordinary(c);gc_case_mutate(c.primary,[&](rocksdb::DB& db){std::string v;check(db.Get(rocksdb::ReadOptions(),"application/outside-gc-prefix",&v).ok()&&v=="keep-this-application-value","GC deleted nonprefix key");});passed.push_back("successful-gc-preserves-nonprefix-values");}
 // Genuine pending publication: produce the new child normally, retain its
 // witness intent, and restore only the closed previous PRIMARY physical copy.
 {GcAdverseCase c(baseline_primary,baseline_witness,g,rules,limits);const auto previous=c.temporary/"previous-primary";fs::copy(c.source,previous,fs::copy_options::recursive);ForkCheckpointToken old_pin,next_pin;
  {Blockchain owner(c.source,g);ChainStore witness(c.witness_source);const auto before=owner.store().fork_checkpoint(g.chain_id,g.block.header.hash(),limits.envelope_bytes);check(bool(before),"pending fixture before missing");old_pin=before->token;CapellaDurableReactor reactor(owner,rules,limits,witness,{},old_pin);bool admitted=false;for(unsigned n=0;n<64&&!admitted;++n)admitted=reactor.block(child);check(admitted,"pending fixture next child failed normal admission");next_pin=reactor.current().token;check(next_pin.sequence>old_pin.sequence,"pending fixture did not advance");}
  const auto committed=c.temporary/"committed-primary";fs::rename(c.source,committed);fs::copy(previous,c.source,fs::copy_options::recursive);check(::chmod(c.source.c_str(),0700)==0,"pending source permission");c.seal();
  {auto owned=c.open();const auto plan=owned->plan(rules,limits,c.budget);const auto result=owned->collect(plan);check(result.at("primary")==gc_token(old_pin)&&result.at("pendingNext")==gc_token(next_pin)&&result.at("noWork")==false,"pending union GC repaired publication or removed no orphan");}
  {ChainStore primary(c.primary,ChainStore::OpenMode::ReadOnly);check(primary.fork_checkpoint(g.chain_id,g.block.header.hash(),limits.envelope_bytes,old_pin).has_value(),"GC completed pending publication without authorization");}
  {Blockchain owner(c.primary,g);ChainStore witness(c.witness);CapellaDurableReactor reactor(owner,rules,limits,witness,{},next_pin);check(reactor.current().token==next_pin,"pending next refs lost after GC; normal recovery failed");}
  // The next-only child ref does not yet exist in previous PRIMARY. Missing it
  // from WITNESS is fatal; an existing corrupt PRIMARY copy cannot be ignored.
  std::string next_only_key;std::string next_payload;
  {ChainStore old(c.source,ChainStore::OpenMode::ReadOnly),committed_store(committed,ChainStore::OpenMode::ReadOnly);const auto cp=committed_store.fork_checkpoint(g.chain_id,g.block.header.hash(),limits.envelope_bytes,next_pin);check(bool(cp),"pending committed manifest missing");const auto outer=Json::parse(cp->snapshot);const auto& durable=outer.at("checkpoint");const auto& store=durable.contains("store")?durable.at("store"):durable;
   for(const auto& row:store.at("snapshot").at("blocks")){const auto id=unhex(row[1].get<std::string>());const auto n=size_t(u64(quantity(row[2])));if(!old.checkpoint_blob(g.chain_id,g.block.header.hash(),id,n)){const auto bytes=committed_store.checkpoint_blob(g.chain_id,g.block.header.hash(),id,n);check(bool(bytes),"pending next ref missing from normal publication");next_only_key="checkpoint-blob/v1/"+hex(id);next_payload=*bytes;break;}}
  }check(!next_only_key.empty()&&!next_payload.empty(),"pending normal child did not produce a new live reference");
  {GcAdverseCase d(c.source,c.witness_source,g,rules,limits);gc_case_mutate(d.witness_source,[&](rocksdb::DB& db){rocksdb::WriteOptions write;write.sync=true;check(db.Delete(write,next_only_key).ok(),"pending missing-next staging");});d.seal();unchanged_refusal(d,"pending-missing-next-only-witness-ref","Invalid Capella durable archive",[&]{ordinary(d);});}
  {GcAdverseCase d(c.source,c.witness_source,g,rules,limits);gc_case_mutate(d.source,[&](rocksdb::DB& db){rocksdb::WriteOptions write;write.sync=true;check(db.Put(write,next_only_key,"corrupt-pending-row").ok(),"pending corrupt primary staging");});d.seal();unchanged_refusal(d,"pending-corrupt-existing-next-primary-ref","checkpoint GC corrupt referenced blob",[&]{ordinary(d);});}
  passed.push_back("pending-union-preserved-no-gc-repair-then-normal-recovery");
 }
 // A real process death after PRIMARY's Delete is durable and before WITNESS
 // cleanup. The fork child immediately execs a FRESH process before opening
 // RocksDB; no inherited Env threads or mutexes are used. This is process-crash
 // recovery on a copy, not electrical power loss.
 {GcAdverseCase c(baseline_primary,baseline_witness,g,rules,limits);c.seal();const auto request_path=(c.temporary/"crash-request.json").string(),stop_path=(c.temporary/"crash-stop.json").string(),profile_path=(c.temporary/"crash-profile.json").string();const auto profile=capella_fork_profile(rules).dump();
  for(const auto& row:{std::pair{request_path,c.request},std::pair{stop_path,c.stopped},std::pair{profile_path,profile}}){std::ofstream file(row.first);file<<row.second;check(bool(file),"GC child public input write");}
  const auto binary_path=fs::canonical(executable).string(),request_pin=hex(c.request_sha).substr(2),stop_pin=hex(c.stopped_sha).substr(2),profile_pin=hex(sha256(Bytes(profile.begin(),profile.end()))).substr(2);int channel[2];check(::pipe(channel)==0,"GC crash pipe");const auto descriptor=std::to_string(channel[1]);const auto child_process=::fork();check(child_process>=0,"GC crash fork");
  if(child_process==0){::close(channel[0]);::execl(binary_path.c_str(),binary_path.c_str(),"--gc-crash-child",request_path.c_str(),request_pin.c_str(),stop_path.c_str(),stop_pin.c_str(),profile_path.c_str(),profile_pin.c_str(),descriptor.c_str(),static_cast<char*>(nullptr));::_exit(127);}
  ::close(channel[1]);struct Child {pid_t pid;~Child(){if(pid>0){::kill(pid,SIGKILL);while(::waitpid(pid,nullptr,0)<0&&errno==EINTR){}}}} cleanup{child_process};struct Channel {int fd;~Channel(){::close(fd);}} read_end{channel[0]};
  pollfd event{channel[0],POLLIN,0};int polled=0;const auto ready_deadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);
  for(;;){const auto now=std::chrono::steady_clock::now();if(now>=ready_deadline)break;const auto remaining=std::chrono::duration_cast<std::chrono::milliseconds>(ready_deadline-now).count();polled=::poll(&event,1,int(std::max<int64_t>(1,remaining)));if(polled>=0||errno!=EINTR)break;}
  if(polled==1&&!(event.revents&POLLIN)&&(event.revents&(POLLHUP|POLLERR|POLLNVAL))){int state=0;const auto ended=::waitpid(child_process,&state,WNOHANG);if(ended==child_process)cleanup.pid=-1;throw std::runtime_error("GC child exited or closed phase pipe before durable boundary; wait status="+std::to_string(ended==child_process?state:-1));}
  check(polled==1&&(event.revents&POLLIN),"GC child did not reach durable between-stores boundary within 30s");char ready=0;check(::read(channel[0],&ready,1)==1&&ready=='P',"GC child failed before natural durable boundary");
  int state=0;bool stopped=false;const auto stop_deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);while(std::chrono::steady_clock::now()<stop_deadline){const auto waited=::waitpid(child_process,&state,WUNTRACED|WNOHANG);if(waited==child_process){stopped=WIFSTOPPED(state);if(!stopped)cleanup.pid=-1;break;}check(waited==0||errno==EINTR,"GC stop wait failed");std::this_thread::sleep_for(std::chrono::milliseconds(5));}
  check(stopped,"GC child did not confirm SIGSTOP boundary");check(::kill(child_process,SIGKILL)==0,"GC child SIGKILL failed");while(::waitpid(child_process,&state,0)<0){check(errno==EINTR,"GC kill wait failed");}cleanup.pid=-1;check(WIFSIGNALED(state)&&WTERMSIG(state)==SIGKILL,"GC child did not terminate by SIGKILL");
  GcAdverseCase recovered(c.primary,c.witness,g,rules,limits);recovered.seal();auto owned=recovered.open();const auto plan=owned->plan(rules,limits,recovered.budget);check(plan.description().at("deletePrimary").empty()&&!plan.description().at("deleteWitness").empty(),"between-stores kill did not leave expected partial cleanup");const auto result=owned->collect(plan);check(result.at("allProtectedReferencesReopened")==true,"SIGKILL reopen lost live closure");passed.push_back("sigkill-after-primary-before-witness-new-plan-reopen");
 }
#ifdef NANO_SYNC_FAULT
 // Reuse the existing real WAL fsync shim. Both stores retain the exact roots;
 // never count an ambiguous Delete as complete or retry on the same handle.
 for(const bool witness_fault:{false,true}){
  GcAdverseCase c(baseline_primary,baseline_witness,g,rules,limits);c.seal();auto owned=c.open();const auto plan=owned->plan(rules,limits,c.budget);
  const auto target=witness_fault?c.witness:c.primary;fs::path wal;for(const auto& entry:fs::directory_iterator(target))if(entry.path().extension()==".log"&&(wal.empty()||entry.path().filename()>wal.filename()))wal=entry.path();check(!wal.empty(),"GC fault WAL missing");
  nano_test_arm_sync_fault(wal.c_str());bool refused=false;try{(void)owned->collect(plan);}catch(const std::runtime_error&){refused=true;}check(refused&&nano_test_sync_failures()>0,"GC fsync error was not rejected");
  bool poisoned=false;try{(void)owned->plan(rules,limits,c.budget);}catch(const std::invalid_argument& error){poisoned=std::string(error.what()).find("incomplete")!=std::string::npos;}check(poisoned,"GC incomplete handle remained usable");owned.reset();
  GcAdverseCase reopened(c.primary,c.witness,g,rules,limits);reopened.seal();auto fresh=reopened.open();const auto retry=fresh->plan(rules,limits,reopened.budget);(void)fresh->collect(retry);passed.push_back(witness_fault?"witness-sync-failure-close-new-plan":"primary-sync-failure-close-new-plan");
 }
#endif
 std::cout<<Json{{"scenario","checkpoint-gc-adversarial-boundaries"},{"passedBoundaries",passed},{"allPreDeletionRefusalsLogicalContentsUnchanged",true},{"syncFailureMayLeaveDurablePartialDeletion",true},{"sigkillPartialProgressExplicit",true},{"physicalPowerLossQualified",false},{"fixtureOnly",true},{"localSigningProviderCalls",0},{"networkQualified",false},{"productionQualified",false}}.dump()<<std::endl;
}
