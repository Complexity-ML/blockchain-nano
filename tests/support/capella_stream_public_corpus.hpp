#pragma once
#include "capella_stream_signed_fixture.hpp"
#include <iomanip>
#include <sstream>
#include <utility>

namespace nano::test::canonical_stream_fixture {
enum class PublicCorpusMode { Complete, Stalled };
struct PublicCorpusPin { fs::path manifest; std::string sha256; size_t bytes; };
struct LoadedFixture {
 Fixture f;
 std::map<ConsensusRoot,CheckpointArchiveEntry> archives;
 std::optional<CheckpointArchiveEntry> wrong_branch;
 std::map<ConsensusRoot,std::pair<Block,Bytes>> execution;
 ConsensusBeaconHeader head;
 ConsensusCheckpoint finalized;
 size_t peak_generator_records=0,peak_generator_state_bytes=0;
 uint64_t generator_prunes=0,archive_bytes=0;
 Json producer_statistics;
};
inline constexpr size_t public_corpus_manifest_maximum=1024*1024;
inline constexpr size_t public_corpus_body_maximum=32*1024*1024;
inline constexpr uint64_t public_corpus_archive_maximum=16*1024*1024;
inline constexpr uint64_t public_corpus_complete_blocks=1026;
inline constexpr uint64_t public_corpus_stalled_blocks=255;
inline constexpr const char* public_corpus_manifest_format="nano-test-capella-stream-corpus-manifest-v1";
inline constexpr const char* public_corpus_body_format="nano-test-capella-stream-corpus-v1";

namespace public_corpus_detail {
inline const char* mode_name(PublicCorpusMode mode){check(mode==PublicCorpusMode::Complete||mode==PublicCorpusMode::Stalled,"public corpus exact mode enum");return mode==PublicCorpusMode::Complete?"complete":"stalled";}
inline uint64_t mode_count(PublicCorpusMode mode){(void)mode_name(mode);return mode==PublicCorpusMode::Complete?public_corpus_complete_blocks:public_corpus_stalled_blocks;}
inline std::string digest(const Bytes& bytes){return hex(sha256(bytes)).substr(2);}
inline Bytes bytes_of_json(const Json& value){return bytes_of(value.dump());}
inline void fields(const Json& value,std::initializer_list<const char*> expected){
 check(value.is_object()&&value.size()==expected.size(),"public corpus exact object fields");
 for(const auto* key:expected)check(value.contains(key),"public corpus required field");
}
inline uint64_t unsigned_number(const Json& value){check(value.is_number_unsigned(),"public corpus unsigned integer");return value.get<uint64_t>();}
inline bool sha_text(const std::string& value){return value.size()==64&&std::all_of(value.begin(),value.end(),[](char c){return(c>='0'&&c<='9')||(c>='a'&&c<='f');});}
inline ConsensusRoot root(const Json& value){
 check(value.is_string(),"public corpus root string");const auto text=value.get<std::string>();
 check(text.size()==66&&text.starts_with("0x"),"public corpus root width");const auto bytes=unhex(text);check(bytes.size()==32&&hex(bytes)==text,"public corpus canonical root");return test_root(bytes);
}
inline std::string root_text(const ConsensusRoot& value){return hex(Bytes(value.begin(),value.end()));}
inline Json header(const ConsensusBeaconHeader& value){return Json::array({value.slot,value.proposer_index,root_text(value.parent_root),root_text(value.state_root),root_text(value.body_root)});}
inline ConsensusBeaconHeader header(const Json& value){
 check(value.is_array()&&value.size()==5,"public corpus header shape");
 return {unsigned_number(value[0]),unsigned_number(value[1]),root(value[2]),root(value[3]),root(value[4])};
}
inline Json checkpoint_json(const ConsensusCheckpoint& value){return Json::array({value.epoch,root_text(value.root)});}
inline ConsensusCheckpoint checkpoint_value(const Json& value){check(value.is_array()&&value.size()==2,"public corpus checkpoint shape");return {unsigned_number(value[0]),root(value[1])};}
inline bool same_material(const struct stat& a,const struct stat& b){
#ifdef __APPLE__
 const auto am=a.st_mtimespec,bm=b.st_mtimespec,ac=a.st_ctimespec,bc=b.st_ctimespec;
#else
 const auto am=a.st_mtim,bm=b.st_mtim,ac=a.st_ctim,bc=b.st_ctim;
#endif
 return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_mode==b.st_mode&&a.st_nlink==b.st_nlink&&a.st_uid==b.st_uid&&a.st_gid==b.st_gid&&a.st_size==b.st_size&&am.tv_sec==bm.tv_sec&&am.tv_nsec==bm.tv_nsec&&ac.tv_sec==bc.tv_sec&&ac.tv_nsec==bc.tv_nsec;
}
struct File {
 int value=-1;
 explicit File(int descriptor):value(descriptor){}
 File(const File&)=delete;
 ~File(){if(value>=0)::close(value);}
 void close(){const auto descriptor=std::exchange(value,-1);check(descriptor>=0&&::close(descriptor)==0,"public corpus descriptor close");}
};
inline Bytes read_pinned(const fs::path& path,const std::string& expected_sha,size_t expected_bytes,size_t maximum){
 checkpoint();check(sha_text(expected_sha)&&expected_bytes&&expected_bytes<=maximum,"public corpus external SHA/byte bound");
 check(path.is_absolute()&&path.lexically_normal()==path&&fs::canonical(path)==path,"public corpus canonical file");
 File file(::open(path.c_str(),O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC));check(file.value>=0,"public corpus input open");
 struct stat before{},after{},linked{};check(::fstat(file.value,&before)==0&&S_ISREG(before.st_mode)&&before.st_nlink==1&&before.st_size>=0&&uint64_t(before.st_size)==expected_bytes,"public corpus size before allocation");
 Bytes out;out.reserve(expected_bytes);std::array<uint8_t,65536> buffer{};
 for(;;){checkpoint();const auto n=::read(file.value,buffer.data(),buffer.size());if(n<0&&errno==EINTR)continue;check(n>=0,"public corpus input read");if(!n)break;check(size_t(n)<=expected_bytes-out.size(),"public corpus input grew");out.insert(out.end(),buffer.begin(),buffer.begin()+n);}
 check(out.size()==expected_bytes&&digest(out)==expected_sha,"public corpus exact input SHA/bytes");
 check(::fstat(file.value,&after)==0&&::lstat(path.c_str(),&linked)==0&&same_material(before,after)&&same_material(after,linked),"public corpus FD/path material changed");file.close();checkpoint();return out;
}
inline Json parse_canonical(const Bytes& bytes){auto out=Json::parse(bytes.begin(),bytes.end());check(out.dump()==std::string(bytes.begin(),bytes.end()),"public corpus canonical JSON");return out;}
inline Json fixed_inputs(){return Json::array({
 Json{{"file","tests/fixtures/capella-bootstrap-anchor-cb532/manifest.json"},{"sha256","ec150c28c8c187e6728cea7ef717acc6383e0b75242302b0a481fc2a9c425fc6"},{"bytes",uint64_t(23260)}},
 Json{{"file","tests/fixtures/capella-bootstrap-anchor-cb532/profile.json"},{"sha256","e3440eba0a871a8eedf84a6e2cdccee53cc4389e8ce6b979166621c8e893890d"},{"bytes",uint64_t(1331)}},
 Json{{"file","tests/fixtures/capella-bootstrap-anchor-cb532/genesis-state.ssz"},{"sha256","611643b5ebf9e294bd65a4517e2d69609cdf92043d020b761696675a7dcb1879"},{"bytes",uint64_t(19749)}},
 Json{{"file","tests/fixtures/capella-bootstrap-anchor-cb532/genesis.json"},{"sha256","3373c76de09192ad440625613db97c17b50fbfeddb5adb3190c8f0eb83a22005"},{"bytes",uint64_t(15996)}},
 Json{{"file","tests/fixtures/consensus-capella-slots-official/slots_1/pre.ssz"},{"sha256","f5a464175565a8ed6a74790a60b84477c16a5b1bf71ece55736f192142cbca3d"},{"bytes",uint64_t(19717)}}});}
inline void verify_fixed_inputs(){for(const auto& pin:fixed_inputs())read_pinned(fs::path(NANO_TEST_SOURCE)/pin.at("file").get<std::string>(),pin.at("sha256").get<std::string>(),size_t(unsigned_number(pin.at("bytes"))),32*1024*1024);}
inline Json binding(const Fixture& value){
 const auto anchor=value.initial.blocks.at(value.initial.timed.view.anchor).lease(checkpoint);
 const auto snapshot=capella_store_snapshot(value.initial,value.rules,value.limits,checkpoint);
 return {{"chainId",quantity(value.rules.transition.chain_id)},{"genesisBlockHash",hex(value.genesis.block.header.hash())},{"genesisStateRoot",hex(value.genesis.state.root())},
  {"anchorRoot",root_text(value.initial.timed.view.anchor)},{"anchorHeader",header(anchor->header)},{"anchorStateSha256",digest(anchor->state.ssz)},{"anchorStateBytes",uint64_t(anchor->state.ssz.size())},
  {"anchorWireSha256",digest(value.wire)},{"anchorWireBytes",uint64_t(value.wire.size())},{"initialSnapshotSha256",digest(bytes_of(snapshot))},{"initialSnapshotBytes",uint64_t(snapshot.size())},
  {"slotsPerEpoch",value.rules.transition.consensus.epoch.ffg.slots_per_epoch},{"slotDurationMs",value.rules.slot_duration_ms},{"nativeDepositVotes",value.rules.native_deposit_votes},
  {"snapshotLimits",Json::array({uint64_t(value.limits.envelope_bytes),uint64_t(value.limits.blocks),uint64_t(value.limits.messages),uint64_t(value.limits.equivocations),uint64_t(value.limits.state_bytes),uint64_t(value.limits.total_state_bytes),value.limits.replay_slots,value.limits.sync_trials})}};
}
inline Json archive(const CheckpointArchiveEntry& value){return {{"root",hex(value.root)},{"slot",value.slot},{"ssz",hex(value.payload)}};}
inline CheckpointArchiveEntry archive(const Json& value,uint64_t& total){
 fields(value,{"root","slot","ssz"});const auto id=root(value.at("root"));const auto slot=unsigned_number(value.at("slot"));check(value.at("ssz").is_string(),"public corpus signed bytes");const auto& text=value.at("ssz").get_ref<const std::string&>();
 check(text.starts_with("0x")&&text.size()%2==0&&text.size()>=2+404*2,"public corpus signed envelope width");const auto size=(text.size()-2)/2;check(total<=public_corpus_archive_maximum&&size<=public_corpus_archive_maximum-total,"public corpus cumulative signed byte bound");
 auto wire=unhex(text);check(hex(wire)==text&&test_little(wire,0,4)==100&&test_little(wire,180,4)==84&&test_little(wire,100)==slot,"public corpus canonical signed layout");total+=wire.size();return {Bytes(id.begin(),id.end()),slot,std::move(wire)};
}
inline void only_two_files(const fs::path& root){
 size_t count=0;for(const auto& item:fs::directory_iterator(root)){checkpoint();const auto name=item.path().filename().string();const auto status=item.symlink_status();check(fs::is_regular_file(status)&&(name=="manifest.json"||name=="corpus.json"),"public corpus exact ordinary-file namespace");++count;}check(count==2,"public corpus two-file namespace");
}
inline void write_new(const fs::path& root,int directory,const char* name,const Bytes& bytes){
 check(bytes.size()<=public_corpus_body_maximum,"public corpus output size");
 File file(::openat(directory,name,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600));check(file.value>=0,"public corpus exclusive output");
 size_t at=0;while(at<bytes.size()){checkpoint();const auto n=::write(file.value,bytes.data()+at,bytes.size()-at);if(n<0&&errno==EINTR)continue;check(n>0,"public corpus output write");at+=size_t(n);}check(::fsync(file.value)==0,"public corpus output fsync");
 struct stat held{},linked{};check(::fstat(file.value,&held)==0&&::lstat((root/name).c_str(),&linked)==0&&same_material(held,linked)&&S_ISREG(held.st_mode)&&held.st_nlink==1&&held.st_uid==geteuid()&&(held.st_mode&07777)==0600&&uint64_t(held.st_size)==bytes.size(),"public corpus output identity");file.close();
 (void)read_pinned(root/name,digest(bytes),bytes.size(),public_corpus_body_maximum);
}
inline struct stat create_directory(const fs::path& root){
 check(root.is_absolute()&&root.lexically_normal()==root&&!root.filename().empty()&&fs::canonical(root.parent_path())==root.parent_path(),"public corpus explicit canonical output parent");
 check(::mkdir(root.c_str(),0700)==0,"public corpus NEW dedicated output directory");
 struct stat value{};check(::lstat(root.c_str(),&value)==0&&S_ISDIR(value.st_mode)&&value.st_uid==geteuid()&&(value.st_mode&07777)==0700&&fs::canonical(root)==root,"public corpus owned output directory");return value;
}
}

inline LoadedFixture from_generated(LongFixture&& value){
 LoadedFixture out{std::move(value.f),std::move(value.archives),std::move(value.wrong_branch),std::move(value.execution),value.head,value.finalized,value.peak_generator_records,value.peak_generator_state_bytes,value.generator_prunes,value.archive_bytes,Json::object()};return out;
}

inline Json export_public_corpus(PublicCorpusMode mode,const fs::path& root){
 namespace d=public_corpus_detail;
 checkpoint();const auto created=d::create_directory(root);d::File directory(::open(root.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));struct stat opened{};
 check(directory.value>=0&&::fstat(directory.value,&opened)==0&&d::same_material(created,opened),"public corpus created directory FD identity");
 d::verify_fixed_inputs();const auto signatures_before=test_signatures;
 LongFixture generated(d::mode_count(mode),mode==PublicCorpusMode::Complete);auto value=from_generated(std::move(generated));
 Json records=Json::array(),execution=Json::array();std::vector<const CheckpointArchiveEntry*> ordered;
 for(const auto& [_,entry]:value.archives)ordered.push_back(&entry);std::sort(ordered.begin(),ordered.end(),[](auto a,auto b){return a->slot<b->slot;});
 for(const auto* entry:ordered){checkpoint();records.push_back(d::archive(*entry));}
 for(const auto& [id,expected]:value.execution){checkpoint();execution.push_back(Json{{"root",d::root_text(id)},{"block",expected.first.to_json(true)},{"stateRoot",hex(expected.second)}});}
 const Json statistics{{"scope","producer-generation-only"},{"fixtureBlsSignatures",test_signatures-signatures_before},{"peakGeneratorRecords",uint64_t(value.peak_generator_records)},{"peakGeneratorStateSszBytes",uint64_t(value.peak_generator_state_bytes)},{"generatorPrunes",value.generator_prunes},{"archiveBytes",value.archive_bytes}};
 Json body{{"format",public_corpus_body_format},{"mode",d::mode_name(mode)},{"signedDescendants",d::mode_count(mode)},{"sourceInputs",d::fixed_inputs()},{"binding",d::binding(value.f)},
  {"head",d::header(value.head)},{"finalized",d::checkpoint_json(value.finalized)},{"archives",std::move(records)},{"execution",std::move(execution)},
  {"sibling",value.wrong_branch?d::archive(*value.wrong_branch):Json(nullptr)},{"producerStatistics",statistics}};
 const auto body_bytes=d::bytes_of_json(body);check(body_bytes.size()<=public_corpus_body_maximum,"public corpus encoded body budget");
 const Json manifest{{"format",public_corpus_manifest_format},{"mode",d::mode_name(mode)},{"body",{{"file","corpus.json"},{"sha256",d::digest(body_bytes)},{"bytes",uint64_t(body_bytes.size())}}}};
 const auto manifest_bytes=d::bytes_of_json(manifest);check(manifest_bytes.size()<=public_corpus_manifest_maximum,"public corpus manifest budget");
 d::write_new(root,directory.value,"corpus.json",body_bytes);d::write_new(root,directory.value,"manifest.json",manifest_bytes);d::only_two_files(root);
 struct stat held_directory{},named_directory{};check(::fstat(directory.value,&held_directory)==0&&::lstat(root.c_str(),&named_directory)==0&&d::same_material(held_directory,named_directory)&&held_directory.st_dev==created.st_dev&&held_directory.st_ino==created.st_ino&&held_directory.st_mode==created.st_mode&&held_directory.st_uid==created.st_uid&&held_directory.st_gid==created.st_gid,"public corpus output directory retained identity");
 check(::fsync(directory.value)==0,"public corpus directory fsync");directory.close();checkpoint();
 return {{"mode",d::mode_name(mode)},{"manifestSha256",d::digest(manifest_bytes)},{"manifestBytes",uint64_t(manifest_bytes.size())},{"bodySha256",d::digest(body_bytes)},{"bodyBytes",uint64_t(body_bytes.size())},{"producerStatistics",statistics}};
}

inline LoadedFixture load_public_corpus(PublicCorpusPin pin,PublicCorpusMode mode){
 namespace d=public_corpus_detail;
 checkpoint();check(pin.manifest.filename()=="manifest.json","public corpus fixed manifest name");
 const auto manifest_bytes=d::read_pinned(pin.manifest,pin.sha256,pin.bytes,public_corpus_manifest_maximum);const auto manifest=d::parse_canonical(manifest_bytes);
 d::fields(manifest,{"format","mode","body"});check(manifest.at("format")==public_corpus_manifest_format&&manifest.at("mode")==d::mode_name(mode),"public corpus explicit manifest mode/format");
 const auto& body_pin=manifest.at("body");d::fields(body_pin,{"file","sha256","bytes"});check(body_pin.at("file")=="corpus.json"&&body_pin.at("sha256").is_string(),"public corpus fixed body reference");
 const auto body_size=d::unsigned_number(body_pin.at("bytes"));check(body_size<=public_corpus_body_maximum,"public corpus body byte bound");
 const auto body_bytes=d::read_pinned(pin.manifest.parent_path()/"corpus.json",body_pin.at("sha256").get<std::string>(),size_t(body_size),public_corpus_body_maximum);const auto body=d::parse_canonical(body_bytes);
 d::fields(body,{"format","mode","signedDescendants","sourceInputs","binding","head","finalized","archives","execution","sibling","producerStatistics"});
 check(body.at("format")==public_corpus_body_format&&body.at("mode")==d::mode_name(mode)&&d::unsigned_number(body.at("signedDescendants"))==d::mode_count(mode)&&body.at("sourceInputs")==d::fixed_inputs(),"public corpus closed body identity/count");
 const auto& declared_binding=body.at("binding");d::fields(declared_binding,{"chainId","genesisBlockHash","genesisStateRoot","anchorRoot","anchorHeader","anchorStateSha256","anchorStateBytes","anchorWireSha256","anchorWireBytes","initialSnapshotSha256","initialSnapshotBytes","slotsPerEpoch","slotDurationMs","nativeDepositVotes","snapshotLimits"});
 const auto declared_anchor=d::root(declared_binding.at("anchorRoot"));const auto declared_anchor_header=d::header(declared_binding.at("anchorHeader"));check(consensus_beacon_header_root(declared_anchor_header)==declared_anchor,"public corpus declared anchor header identity");
 const auto count=d::mode_count(mode);check(body.at("archives").is_array()&&body.at("archives").size()==count&&body.at("execution").is_array()&&body.at("execution").size()==count+1,"public corpus complete archive/oracle counts");
 std::map<ConsensusRoot,CheckpointArchiveEntry> archives;std::map<ConsensusRoot,std::pair<Block,Bytes>> execution;uint64_t raw_bytes=0,previous_slot=0;ConsensusRoot first_root{};
 for(const auto& row:body.at("archives")){checkpoint();auto entry=d::archive(row,raw_bytes);const auto id=test_root(entry.root);check(entry.slot>previous_slot&&archives.emplace(id,std::move(entry)).second,"public corpus distinct ordered signed records");if(previous_slot==0)first_root=id;previous_slot=archives.at(id).slot;}
 for(const auto& row:body.at("execution")){
  checkpoint();d::fields(row,{"root","block","stateRoot"});const auto id=d::root(row.at("root"));const auto state=d::root(row.at("stateRoot"));auto block=Block::from_json(row.at("block"));
  check(block.to_json(true)==row.at("block")&&block.header.state_root==Bytes(state.begin(),state.end())&&execution.emplace(id,std::make_pair(std::move(block),Bytes(state.begin(),state.end()))).second,"public corpus canonical independent execution oracle");
 }
 const auto head=d::header(body.at("head"));const auto finalized=d::checkpoint_value(body.at("finalized"));const auto head_id=consensus_beacon_header_root(head);
 check(archives.contains(head_id)&&archives.at(head_id).slot==head.slot&&head.slot==previous_slot,"public corpus head is the complete signed tip");
 check(execution.contains(declared_anchor)&&execution.size()==archives.size()+1&&(finalized.root==declared_anchor||archives.contains(finalized.root)),"public corpus complete anchor/finality oracle coverage");for(const auto& [id,_]:archives)check(execution.contains(id),"public corpus every archive oracle present before bootstrap");
 std::optional<CheckpointArchiveEntry> sibling;
 if(mode==PublicCorpusMode::Complete){check(!body.at("sibling").is_null(),"public corpus genuine sibling required");uint64_t sibling_bytes=raw_bytes;sibling=d::archive(body.at("sibling"),sibling_bytes);check(sibling->slot==archives.at(first_root).slot&&sibling->root!=archives.at(first_root).root&&test_root(sibling->root)!=declared_anchor,"public corpus distinct same-slot sibling");}
 else check(body.at("sibling").is_null(),"public corpus stalled mode has no sibling");
 const auto& statistics=body.at("producerStatistics");d::fields(statistics,{"scope","fixtureBlsSignatures","peakGeneratorRecords","peakGeneratorStateSszBytes","generatorPrunes","archiveBytes"});
 check(statistics.at("scope")=="producer-generation-only"&&d::unsigned_number(statistics.at("archiveBytes"))==raw_bytes&&d::unsigned_number(statistics.at("fixtureBlsSignatures"))<=20000,"public corpus static producer provenance");
 const auto records=d::unsigned_number(statistics.at("peakGeneratorRecords")),state_bytes=d::unsigned_number(statistics.at("peakGeneratorStateSszBytes")),prunes=d::unsigned_number(statistics.at("generatorPrunes"));
 check(records<=256,"public corpus original generator record ceiling");d::only_two_files(pin.manifest.parent_path());d::verify_fixed_inputs();
 // Only now may a fully pinned/closed corpus invoke the two-signature fixed
 // bootstrap. Pin/mode/count/missing-file failures above create no signatures.
 Fixture base=fixture();check(d::binding(base)==body.at("binding"),"public corpus recomputed fixed anchor/profile/genesis identity");
 const auto anchor=base.initial.timed.view.anchor;check(execution.contains(anchor)&&execution.size()==archives.size()+1,"public corpus anchor execution oracle");
 const auto initial=base.initial.blocks.at(anchor).lease(checkpoint);check(execution.at(anchor).first.to_json(true)==initial->execution_block.to_json(true)&&execution.at(anchor).second==initial->execution_state.root(),"public corpus exact authenticated anchor oracle");
 for(const auto& [id,_]:archives)check(execution.contains(id),"public corpus every signed block has a producer oracle");
 check(state_bytes<=base.limits.total_state_bytes&&finalized.epoch<=head.slot/base.rules.transition.consensus.epoch.ffg.slots_per_epoch,"public corpus provenance/finality bounds");
 if(mode==PublicCorpusMode::Complete){
  check(prunes>0&&finalized.epoch>0,"public corpus complete mode genuine finality provenance");
  const auto sibling_id=test_root(sibling->root);check(!base.initial.blocks.contains(sibling_id),"public corpus sibling cannot reuse an existing anchor record");
  check(test_root(sibling->payload,116)==anchor,"public corpus sibling parent bound to fixed anchor");
  const auto at=base.initial.timed.clock.genesis_time_ms+sibling->slot*base.rules.slot_duration_ms;
  auto store=prepare_capella_resumed_store_tick(base.initial,at,base.rules,base.limits.replay_slots,base.limits.sync_trials,checkpoint);
  const auto admitted=prepare_capella_resumed_fork_block(store,sibling->payload,base.rules,base.limits.replay_slots,base.limits.sync_trials,checkpoint);
  check(admitted&&admitted->blocks.size()==store.blocks.size()+1&&admitted->blocks.contains(sibling_id),"public corpus sibling must itself pass genuine Core admission");
  const auto& sibling_header=admitted->blocks.at(sibling_id).header();check(sibling_header.slot==sibling->slot&&sibling_header.parent_root==anchor,"public corpus newly admitted sibling identity differs");
 }else check(prunes==0&&finalized==base.initial.timed.view.finalized,"public corpus stalled finality cannot be fabricated");
 checkpoint();return {std::move(base),std::move(archives),std::move(sibling),std::move(execution),head,finalized,size_t(records),size_t(state_bytes),prunes,raw_bytes,statistics};
}
}
