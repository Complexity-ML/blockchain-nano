// Standalone O1 test driver; real Core audit, database and fixed public fixture.
// No network actor, validator journal, external key, seed or campaign input.
#include "nano/consensus_capella_audit.hpp"
#include "nano/consensus_capella_canonical_stream.hpp"
#include "support/capella_stream_signed_fixture.hpp"
#include <rocksdb/db.h>
#include <openssl/evp.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <cerrno>
#include <cstring>
#include <tuple>
#include <utility>

namespace {
using namespace nano;
namespace fs=std::filesystem;
namespace f=nano::test::canonical_stream_fixture;
constexpr size_t signed_blocks=160,fixture_live_blocks=64,explicit_live_blocks=48;
uint64_t process_high_water_bytes(){rusage usage{};f::check(::getrusage(RUSAGE_SELF,&usage)==0&&usage.ru_maxrss>=0,"whole driver RSS observation");
#ifdef __APPLE__
 return uint64_t(usage.ru_maxrss);
#else
 f::check(uint64_t(usage.ru_maxrss)<=UINT64_MAX/1024,"Linux RSS byte conversion");return uint64_t(usage.ru_maxrss)*1024;
#endif
}
struct PhysicalRow {
 bool directory=false;std::array<std::string,9> material;Bytes digest;
 bool operator==(const PhysicalRow&)const=default;
};
using Inventory=std::map<std::string,PhysicalRow>;
uint64_t opened_fence_files=0,closed_fence_files=0;
std::array<std::string,9> material(const struct stat& s){
#ifdef __APPLE__
 const auto mt=s.st_mtimespec,ct=s.st_ctimespec;
#else
 const auto mt=s.st_mtim,ct=s.st_ctim;
#endif
 return {std::to_string(s.st_dev),std::to_string(s.st_ino),std::to_string(s.st_mode),std::to_string(s.st_nlink),std::to_string(s.st_uid),std::to_string(s.st_gid),std::to_string(s.st_size),
  std::to_string(mt.tv_sec)+":"+std::to_string(mt.tv_nsec),std::to_string(ct.tv_sec)+":"+std::to_string(ct.tv_nsec)};
}
struct File {
 int value=-1;
 explicit File(const fs::path& p):value(::open(p.c_str(),O_RDONLY|O_NOFOLLOW|O_NONBLOCK)){f::check(value>=0,"physical fence open");++opened_fence_files;}
 ~File(){if(value>=0&&::close(value)==0)++closed_fence_files;}
 void close(){const int fd=std::exchange(value,-1);f::check(fd>=0&&::close(fd)==0,"physical fence close");++closed_fence_files;}
};
PhysicalRow physical(const fs::path& path){
 f::checkpoint();struct stat before{},after{},linked{};f::check(::lstat(path.c_str(),&before)==0,"physical lstat");
 f::check(!S_ISLNK(before.st_mode)&&(S_ISDIR(before.st_mode)||S_ISREG(before.st_mode)),"physical regular namespace");
 if(S_ISDIR(before.st_mode))return {true,material(before),{}};
 f::check(before.st_nlink==1&&before.st_size>=0&&uint64_t(before.st_size)<=256*1024*1024,"physical file bound/links");
 File file(path);f::check(::fstat(file.value,&after)==0&&material(after)==material(before),"physical opened identity");
 std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)> hash(EVP_MD_CTX_new(),EVP_MD_CTX_free);f::check(bool(hash)&&EVP_DigestInit_ex(hash.get(),EVP_sha256(),nullptr)==1,"physical SHA init");
 std::array<uint8_t,65536> chunk{};uint64_t total=0;for(;;){f::checkpoint();const auto n=::read(file.value,chunk.data(),chunk.size());if(n<0&&errno==EINTR)continue;f::check(n>=0,"physical SHA read");if(!n)break;total+=uint64_t(n);f::check(total<=uint64_t(before.st_size)&&EVP_DigestUpdate(hash.get(),chunk.data(),size_t(n))==1,"physical SHA bytes");}
 Bytes digest(32);unsigned size=0;f::check(EVP_DigestFinal_ex(hash.get(),digest.data(),&size)==1&&size==32,"physical SHA final");
 f::check(total==uint64_t(before.st_size)&&::fstat(file.value,&after)==0&&::lstat(path.c_str(),&linked)==0&&material(before)==material(after)&&material(after)==material(linked),"physical full-nine read fence");file.close();return {false,material(after),std::move(digest)};
}
Inventory inventory(const fs::path& root){
 Inventory out;out.emplace("",physical(root));f::check(out.begin()->second.directory,"physical database directory");
 for(const auto& e:fs::recursive_directory_iterator(root)){f::checkpoint();f::check(out.size()<10000,"physical namespace count bound");out.emplace(e.path().lexically_relative(root).generic_string(),physical(e.path()));}
 return out;
}
std::string q(uint64_t value){return quantity(Int(std::to_string(value)));}
void write(const fs::path& path,const std::string& bytes){std::ofstream out(path,std::ios::binary);f::check(bool(out),"owned public request write");out.write(bytes.data(),std::streamsize(bytes.size()));out.close();f::check(bool(out),"owned public request close");}
struct Tmpdir {
 std::optional<std::string> previous;
 explicit Tmpdir(const fs::path& path){if(const char* value=::getenv("TMPDIR"))previous=value;f::check(::setenv("TMPDIR",path.c_str(),1)==0,"owned temporary parent selection");}
 ~Tmpdir(){if(previous)::setenv("TMPDIR",previous->c_str(),1);else ::unsetenv("TMPDIR");}
};
template<class Database=rocksdb::DB>std::unique_ptr<Database> mutation_database(const fs::path& path){
 rocksdb::Options options;options.create_if_missing=false;std::unique_ptr<Database> db;
 if constexpr(requires{Database::Open(options,path.string(),&db);})f::check(Database::Open(options,path.string(),&db).ok(),"existing mutation copy open");
 else{Database* raw=nullptr;const auto result=Database::Open(options,path.string(),&raw);db.reset(raw);f::check(result.ok(),"existing mutation copy open");}return db;
}
void corrupt_old_signature(const fs::path& path,const CheckpointArchiveEntry& original){
 auto db=mutation_database(path);const std::string key="checkpoint-archive/v1/"+hex(original.root);std::string raw;
 f::check(db->Get(rocksdb::ReadOptions(),key,&raw).ok(),"old signature archive exists on independent copy");auto value=Json::parse(raw);auto bytes=unhex(value.at("payload").get<std::string>());
 f::check(bytes==original.payload&&bytes.size()>100,"mutation uses exact old signed prefix");bytes[4]^=1;value["payload"]=hex(bytes);value["digest"]=hex(sha256(bytes));rocksdb::WriteOptions options;options.sync=true;
 f::check(db->Put(options,key,value.dump()).ok(),"write only copied historical BLS signature with matching outer digest");
}
struct Boundary {
 ForkCheckpointToken token;Bytes execution_hash,execution_state;ConsensusRoot consensus_head,consensus_state,warm_anchor;
 ConsensusCheckpoint finalized;uint64_t warm_anchor_slot=0;
};
Boundary persist(const fs::path& path,const f::LongFixture& source){
 const auto& c=source.f;ChainStore disk(path);disk.bind_chain(c.genesis.chain_id,c.genesis.block.header.hash(),"beacon");disk.commit(c.genesis.block,c.genesis.state,{});
 {const auto anchor=c.initial.blocks.at(c.initial.timed.view.anchor).lease(f::checkpoint);disk.commit(anchor->execution_block,anchor->execution_state,anchor->receipts);}
 CapellaDurableReactor reactor(disk,c.genesis.chain_id,c.genesis.block.header.hash(),c.rules,c.limits,c.initial,f::checkpoint);reactor.set_automatic_pruning(true);reactor.set_historical_state_pruning(true);
 std::vector<const CheckpointArchiveEntry*> ordered;for(const auto& [_,entry]:source.archives)ordered.push_back(&entry);std::sort(ordered.begin(),ordered.end(),[](auto a,auto b){return a->slot<b->slot;});
 f::check(ordered.size()==signed_blocks,"fixed complete signed history");for(const auto* entry:ordered){f::checkpoint();reactor.tick(c.initial.timed.clock.genesis_time_ms+entry->slot*c.rules.slot_duration_ms,f::checkpoint);f::check(reactor.block(entry->payload,f::checkpoint),"actual durable canonical signed admission");}
 const auto saved=reactor.current();const auto head=capella_fork_head(*saved.state,c.rules,f::checkpoint);f::check(head->header==source.head&&saved.state->timed.view.finalized==source.finalized,"durable fixture matches independently generated head/finality");
 const auto anchor=saved.state->blocks.at(saved.state->timed.view.anchor).header();f::check(anchor.slot>16&&disk.state_pruning().has_value(),"real durable pruning moved warm anchor beyond old external checkpoint");
 return {saved.token,head->execution_block.header.hash(),head->execution_state.root(),consensus_beacon_header_root(head->header),head->header.state_root,saved.state->timed.view.anchor,saved.state->timed.view.finalized,anchor.slot};
}
Json request(const f::LongFixture& fixture,const fs::path& anchor,const Boundary& stopped,const ConsensusCheckpoint& required){
 const auto& c=fixture.f;const auto& l=c.limits;const auto profile=fs::path(NANO_TEST_SOURCE)/"tests/fixtures/capella-bootstrap-anchor-cb532/profile.json";
 const auto profile_bytes=f::bytes_of(f::read(profile)),anchor_bytes=f::bytes_of(f::read(anchor));return {
  {"profile",{{"file",profile.string()},{"sha256",hex(sha256(profile_bytes))}}},{"anchor",{{"file",anchor.string()},{"sha256",hex(sha256(anchor_bytes))}}},
  {"limits",{{"envelopeBytes",q(l.envelope_bytes)},{"blocks",q(l.blocks)},{"messages",q(l.messages)},{"equivocations",q(l.equivocations)},{"stateBytes",q(l.state_bytes)},{"totalStateBytes",q(l.total_state_bytes)},{"replaySlots",q(l.replay_slots)},{"syncTrials",q(l.sync_trials)}}},
  {"expectedCheckpoint",{{"sequence",q(stopped.token.sequence)},{"digest",hex(stopped.token.digest)}}},{"maximumBlocks",q(signed_blocks)},{"maximumArchiveBytes",q(fixture.archive_bytes)},
  {"requiredFinalized",{{"epoch",q(required.epoch)},{"root",hex(Bytes(required.root.begin(),required.root.end()))}}}};
}
void same_boundary(const fs::path& db,const f::LongFixture& fixture,const Boundary& expected){
 const auto& c=fixture.f;ChainStore disk(db,ChainStore::OpenMode::ReadOnly);const auto record=disk.fork_checkpoint(c.genesis.chain_id,c.genesis.block.header.hash(),c.limits.envelope_bytes,expected.token);const auto tip=disk.latest();
 f::check(record&&record->token==expected.token&&tip&&tip->header.hash()==expected.execution_hash,"actual RO checkpoint/tip preserved");const auto state=disk.state_at(tip->header.number);f::check(state&&state->root()==expected.execution_state&&tip->header.state_root==expected.execution_state,"actual RO execution roots preserved");
 CapellaDurableReactor restored(disk,c.genesis.chain_id,c.genesis.block.header.hash(),c.rules,c.limits,expected.token,f::checkpoint);const auto saved=restored.current();const auto head=capella_fork_head(*saved.state,c.rules,f::checkpoint);
 f::check(saved.token==expected.token&&consensus_beacon_header_root(head->header)==expected.consensus_head&&head->header.state_root==expected.consensus_state&&saved.state->timed.view.finalized==expected.finalized&&saved.state->timed.view.anchor==expected.warm_anchor,"actual RO consensus roots/finality/token preserved");
}
void verified(const Json& result,const f::LongFixture& source,const Boundary& stopped,const ConsensusCheckpoint& old,size_t live){
 f::check(result.at("canonicalHistoryVerified")==true&&result.at("externalFinalizedAncestorVerified")==true&&result.at("historicalForkChoiceVerified")==false,"real frontend canonical/prefix scope");
 f::check(result.at("head")==hex(Bytes(stopped.consensus_head.begin(),stopped.consensus_head.end()))&&result.at("checkpointSequence")==stopped.token.sequence&&result.at("checkpointDigest")==hex(stopped.token.digest),"audited exact stopped head/token");
 f::check(result.at("blocksIncludingTrustedAnchor")==signed_blocks+1&&result.at("transactions")==0&&result.at("archiveBytes")==source.archive_bytes,"complete canonical prefix counters, including anchor");
 f::check(result.at("requiredFinalized").at("epoch")==q(old.epoch)&&result.at("requiredFinalized").at("root")==hex(Bytes(old.root.begin(),old.root.end()))&&result.at("finalized").at("epoch")==q(stopped.finalized.epoch)&&result.at("finalized").at("root")==hex(Bytes(stopped.finalized.root.begin(),stopped.finalized.root.end())),"independent old required checkpoint and real finality retained");
 const auto& w=result.at("replayWindow");f::check(w.at("maximumLiveBlocks")==live&&w.at("maximumLiveStateBytes")==source.f.limits.total_state_bytes&&w.at("verifiedSignedBlocks")==signed_blocks&&w.at("indexBytes")==signed_blocks*112&&w.at("prunes").get<size_t>()>0,"real bounded stream required, no unbounded wrapper fallback");
 f::check(w.at("privateIndexFile").is_string()&&w.at("privateIndexAutomaticDeletion")==false&&w.at("privateIndexOwnershipForDeletionVerified")==false&&w.at("processRssQualified")==false&&w.at("auxiliaryDepositMemoryBounded")==false,"retained private locator cannot become deletion, RSS or auxiliary-memory authority");
 f::check(w.at("peakLiveBlocks").get<size_t>()<=live&&w.at("peakLiveStateSszPlusNextReserveBytes").get<size_t>()<=source.f.limits.total_state_bytes,"real live window/reservation high-water within declared bounds");
 const auto warm_slot=w.at("finalReplayAnchorSlot").get<uint64_t>();const auto warm_root=unhex(w.at("finalReplayAnchorRoot").get<std::string>());const auto known=std::find_if(source.archives.begin(),source.archives.end(),[&](const auto& row){return row.second.slot==warm_slot;});
 f::check(warm_slot>16&&warm_root.size()==32&&known!=source.archives.end()&&warm_root==known->second.root,"old external checkpoint lies strictly outside actual final canonical warm window");
 f::check(result.at("prunedExecutionStatesReplayed").get<size_t>()>0&&result.at("allHistoricalStatesMaterialized")==false,"actual pruned execution states checked by full signed replay");
}
Json public_result(Json result){result.at("replayWindow").erase("privateIndexFile");result["replayWindow"]["privateIndexLocatorRedacted"]=true;return result;}
} // namespace

int main(){try{
 f::Temp temporary;const auto root=temporary.path,scratch=root/"audit-scratch",db=root/"canonical";fs::create_directory(scratch);fs::permissions(scratch,fs::perms::owner_all,fs::perm_options::replace);
 f::test_stage="fixed-public-unit-fixture";f::LongFixture source(signed_blocks);source.f.limits.blocks=fixture_live_blocks;f::check(source.archives.size()>source.f.limits.blocks&&source.finalized.epoch>2,"history exceeds declared legacy live window with real FFG advances");const auto signatures=f::test_signatures;
 const auto stopped=persist(db,source);const auto anchor=scratch/"trusted-anchor.json";write(anchor,capella_store_snapshot(source.f.initial,source.f.rules,source.f.limits,f::checkpoint));
 const auto old_entry=std::find_if(source.archives.begin(),source.archives.end(),[](const auto& row){return row.second.slot==16;});f::check(old_entry!=source.archives.end(),"known old epoch boundary exists");const ConsensusCheckpoint required{2,old_entry->first};
 auto input=request(source,anchor,stopped,required);const auto before=inventory(db);Json groups=Json::array();
 const auto rss_before_default=process_high_water_bytes();f::test_stage="legacy-frontend-bounded";const auto default_result=audit_capella_database(source.f.genesis,db,input);const auto rss_after_default=process_high_water_bytes();verified(default_result,source,stopped,required,source.f.limits.blocks);same_boundary(db,source,stopped);f::check(inventory(db)==before,"default RO frontend changed database namespace/full9/SHA");groups.push_back({{"case","legacy-default-wrapper"},{"audit",public_result(default_result)}});
 auto explicit_request=input;const auto index=scratch/"explicit.index";explicit_request["replayWindow"]={{"maximumLiveBlocks",q(explicit_live_blocks)},{"maximumLiveStateBytes",q(source.f.limits.total_state_bytes)},{"newPrivateIndex",index.string()}};
 const auto rss_before_explicit=process_high_water_bytes();f::test_stage="explicit-frontend-bounded";const auto explicit_result=audit_capella_database(source.f.genesis,db,explicit_request);const auto rss_after_explicit=process_high_water_bytes();verified(explicit_result,source,stopped,required,explicit_live_blocks);same_boundary(db,source,stopped);f::check(inventory(db)==before,"explicit RO frontend changed database namespace/full9/SHA");groups.push_back({{"case","explicit-48-live"},{"audit",public_result(explicit_result)}});
 const auto corrupt=root/"old-signature-copy";fs::copy(db,corrupt,fs::copy_options::recursive);const auto first=std::find_if(source.archives.begin(),source.archives.end(),[](const auto& row){return row.second.slot==2;});f::check(first!=source.archives.end()&&first->second.slot<explicit_result.at("replayWindow").at("finalReplayAnchorSlot").get<uint64_t>(),"old corrupted signature is outside final live window");corrupt_old_signature(corrupt,first->second);
 const auto corrupt_before=inventory(corrupt);std::set<std::pair<std::string,std::string>> source_inodes;for(const auto& [_,row]:before)if(!row.directory)source_inodes.emplace(row.material[0],row.material[1]);
 for(const auto& [_,row]:corrupt_before)if(!row.directory)f::check(!source_inodes.contains({row.material[0],row.material[1]}),"mutation copy has independent file inodes even if its intentional writable open rotated WAL/log names");
 {ChainStore copy(corrupt,ChainStore::OpenMode::ReadOnly);const auto entry=copy.checkpoint_archive(first->second.root,source.f.limits.envelope_bytes);f::check(entry&&entry->payload!=first->second.payload&&entry->root==first->second.root,"modified outer SHA remains coherent before BLS-prefix audit");}
 auto bad=explicit_request;bad["replayWindow"]["newPrivateIndex"]=(scratch/"old-corrupt.index").string();bool refused=false;f::test_stage="old-signature-prefix-refusal";try{(void)audit_capella_database(source.f.genesis,corrupt,bad);}catch(const std::runtime_error& e){refused=std::string(e.what()).find("stream canonical signed transition rejected")!=std::string::npos;}f::check(refused,"old signature outside warm window must fail actual full signed prefix");f::check(inventory(corrupt)==corrupt_before&&inventory(db)==before,"corrupt prefix audit changed either DB");groups.push_back({{"case","old-signature-outside-window"},{"refused",true}});
 auto small=explicit_request;small["replayWindow"]["maximumLiveBlocks"]=q(2);small["replayWindow"]["newPrivateIndex"]=(scratch/"insufficient.index").string();refused=false;f::test_stage="insufficient-window-refusal";try{(void)audit_capella_database(source.f.genesis,db,small);}catch(const std::runtime_error& e){refused=std::string(e.what()).find("stream finalized window unavailable before next state")!=std::string::npos;}f::check(refused&&inventory(db)==before,"valid but insufficient live window must refuse without changing DB");groups.push_back({{"case","insufficient-two-live"},{"refused",true}});
 auto cancelled=explicit_request;const auto cancelled_index=scratch/"cancelled.index";cancelled["replayWindow"]["newPrivateIndex"]=cancelled_index.string();bool hit=false;uint64_t checks=0;f::test_stage="canonical-index-cancellation";
 try{(void)audit_capella_database(source.f.genesis,db,cancelled,[&]{f::checkpoint();++checks;if(fs::exists(cancelled_index)&&fs::file_size(cancelled_index)>=8*112)throw ExecutionCancelled("bounded-audit-owned-index-cancel");});}catch(const ExecutionCancelled& e){hit=std::string(e.what())=="bounded-audit-owned-index-cancel";}
 f::check(hit&&checks>0&&inventory(db)==before,"real callback cancellation propagates after index progress with RO DB unchanged");same_boundary(db,source,stopped);groups.push_back({{"case","cancellation-after-index-progress"},{"refused",true},{"checkpoints",checks}});
 auto inside=explicit_request;const auto forbidden=db/"audit-index-must-not-exist";inside["replayWindow"]["newPrivateIndex"]=forbidden.string();refused=false;f::test_stage="index-namespace-guards";
 try{(void)audit_capella_database(source.f.genesis,db,inside);}catch(const std::invalid_argument& e){refused=std::string(e.what()).find("private index must be outside database")!=std::string::npos;}
 f::check(refused&&!fs::exists(forbidden)&&inventory(db)==before,"an explicit index in the source DB must be refused before creation");
 const auto owned_parent=scratch/"legacy-index-guard";fs::create_directory(owned_parent);fs::permissions(owned_parent,fs::perms::owner_all,fs::perm_options::replace);fs::path injected;std::optional<PhysicalRow> injected_before;
 {Tmpdir select(owned_parent);f::check(fs::canonical(fs::temp_directory_path())==owned_parent,"actual default scratch must use the owned test parent");bool exclusive=false;
  try{(void)audit_capella_database(source.f.genesis,db,input,[&]{f::checkpoint();if(!injected.empty())return;for(const auto& e:fs::directory_iterator(owned_parent)){
   f::check(e.is_directory()&&e.path().filename().string().starts_with("nano-capella-canonical-audit-"),"only the newly created default audit directory is injectable");const auto target=e.path()/"canonical-index";const std::string known="unit-test-owned-preexisting-index\n";
   const int fd=::open(target.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);f::check(fd>=0,"inject distinct index before the Core O_EXCL create");const auto n=::write(fd,known.data(),known.size());const auto closed=::close(fd);f::check(n==std::ptrdiff_t(known.size())&&closed==0,"known injected index closed");injected=target;injected_before=physical(target);break;
  }});}catch(const std::runtime_error& e){exclusive=std::string(e.what()).find("stream exclusive index create")!=std::string::npos;}
  f::check(exclusive&&!injected.empty()&&injected_before&&physical(injected)==*injected_before,"preexisting foreign index survives the actual default-wrapper O_EXCL refusal");
 }
 f::check(inventory(db)==before,"index namespace guards did not mutate source DB");groups.push_back({{"case","index-namespace-and-preexisting-ownership"},{"insideDatabaseRefusedBeforeCreation",true},{"preexistingIndexSurvived",true}});
 f::check(groups.size()==6&&f::test_signatures==signatures&&opened_fence_files==closed_fence_files,"six cases, no audit signatures, all fence file descriptors closed");
 std::cout<<Json{{"scenario","actual-core-bounded-canonical-audit"},{"passed",true},{"groups",groups},{"signedHistoryBlocks",signed_blocks},{"fixtureBlsSignatures",signatures},{"auditBlsSignaturesCreated",f::test_signatures-signatures},{"physicalFenceFilesOpened",opened_fence_files},{"physicalFenceFilesClosed",closed_fence_files},{"databaseFullNineAndShaUnchanged",true},{"processHighWater",{{"unit","bytes"},{"scope","whole driver including public fixture generation, database materialization and audits"},{"beforeDefaultAudit",rss_before_default},{"afterDefaultAudit",rss_after_default},{"beforeExplicitAudit",rss_before_explicit},{"afterExplicitAudit",rss_after_explicit},{"final",process_high_water_bytes()},{"isolatedAuditRssBoundQualified",false}}},{"stateReservationIsProcessRss",false},{"networkOrCampaignStarted",false},{"productionQualified",false}}.dump()<<'\n';return 0;
 }catch(const std::exception& e){std::cerr<<Json{{"scenario","actual-core-bounded-canonical-audit"},{"passed",false},{"stage",f::test_stage},{"error",e.what()},{"networkOrCampaignStarted",false},{"productionQualified",false}}.dump()<<'\n';return 1;}}
