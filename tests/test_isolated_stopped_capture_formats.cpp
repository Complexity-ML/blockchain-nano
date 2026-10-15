// Fresh temporary public fixtures only. Setup signatures are distinct from the
// stopped capture child, which has no signer, environment or write interface.
#include "support/isolated_capture_fixture.hpp"
#include "support/process.hpp"
#include "nano/checkpoint_blob_gc.hpp"
#include "nano/consensus_message_ssz.hpp"
#include <rocksdb/db.h>
#include <fcntl.h>
#include <sys/stat.h>
namespace {
using namespace nano;using namespace nano::test::isolated_capture;
uint64_t calls=0,accepted=0,refused=0,groups=0;
std::string sha(std::string_view b){return hex(sha256(bytes_of(std::string(b)))).substr(2);}
Json token(const ForkCheckpointToken& t){return {{"sequence",quantity(Int(std::to_string(t.sequence)))},{"digest",hex(t.digest)}};}
void directory(const fs::path& p){check(::mkdir(p.c_str(),0700)==0,"fresh private fixture directory");}
void save(const fs::path& p,std::string_view text){
 const int fd=::open(p.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);check(fd>=0,"new public fixture file");
 try{size_t at=0;while(at<text.size()){const auto n=::write(fd,text.data()+at,text.size()-at);check(n>0,"fixture write");at+=size_t(n);}check(::fsync(fd)==0,"fixture fsync");}catch(...){::close(fd);throw;}::close(fd);
 const int parent=::open(p.parent_path().c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);check(parent>=0,"fixture parent FD");const auto synced=::fsync(parent);::close(parent);check(synced==0,"fixture directory fsync");
}
void save_json(const fs::path& p,const Json& j){save(p,j.dump());}
Json material(const fs::path& p){
 struct stat s{};check(::lstat(p.c_str(),&s)==0&&!S_ISLNK(s.st_mode)&&(S_ISREG(s.st_mode)||S_ISDIR(s.st_mode)),"fixture material type");
#ifdef __APPLE__
 const auto mt=s.st_mtimespec,ct=s.st_ctimespec;
#else
 const auto mt=s.st_mtim,ct=s.st_ctim;
#endif
 Json z={{"dev",uint64_t(s.st_dev)},{"ino",uint64_t(s.st_ino)},{"mode",uint64_t(s.st_mode)},{"links",uint64_t(s.st_nlink)},{"uid",uint64_t(s.st_uid)},{"gid",uint64_t(s.st_gid)},{"bytes",uint64_t(s.st_size)},{"mtime",Json::array({mt.tv_sec,mt.tv_nsec})},{"ctime",Json::array({ct.tv_sec,ct.tv_nsec})}};
 if(S_ISREG(s.st_mode)){check(s.st_nlink==1,"fixture independent file");z["sha256"]=sha(read(p));}return z;
}
Json inventory(const fs::path& root){Json out=Json::object();out["."]=material(root);for(const auto& e:fs::recursive_directory_iterator(root))out[e.path().lexically_relative(root).generic_string()]=material(e.path());return out;}
template<class Database=rocksdb::DB>std::unique_ptr<Database> writable_fixture(const fs::path& path){
 rocksdb::Options options;options.create_if_missing=false;std::unique_ptr<Database> opened;
 if constexpr(requires {Database::Open(options,path.string(),&opened);}){check(Database::Open(options,path.string(),&opened).ok(),"synthetic invalid-blob setup open");}
 else{Database* raw=nullptr;const auto status=Database::Open(options,path.string(),&raw);opened.reset(raw);check(status.ok(),"synthetic invalid-blob setup open");}
 return opened;
}
std::pair<SignedConsensusBeaconHeader,SignedConsensusBeaconHeader> proof(const Fixture& f){
 const auto head=capella_fork_head(f.initial,f.rules,checkpoint);const auto& r=f.rules.transition;const auto ctx=capella_fork_block_context(head->state,head->header,{r.consensus.epoch,r.execution,r.maximum_state_bytes},r.consensus.state,checkpoint);
 ConsensusBeaconHeader a{999,0,ctx.block_root,{},{}},b=a;a.body_root.back()=1;b.body_root.back()=2;const auto epoch=a.slot/r.consensus.epoch.ffg.slots_per_epoch;const auto version=epoch<ctx.fork.epoch?ctx.fork.previous_version:ctx.fork.current_version;TestKey key(0);
 return {{a,key.sign(consensus_signing_root(consensus_beacon_header_root(a),consensus_domain({0,0,0,0},version,ctx.genesis_root)))},{b,key.sign(consensus_signing_root(consensus_beacon_header_root(b),consensus_domain({0,0,0,0},version,ctx.genesis_root)))}};
}
struct Sample {
 const Fixture& f;Temp temp;fs::path public_root,data,witness,manifest_path,config_path,stop_path;Json manifest,config,stop;
 ForkCheckpointRecord primary_record,witness_record;Json closure_description,closure_observation;
 Sample(const Fixture& value,const std::pair<SignedConsensusBeaconHeader,SignedConsensusBeaconHeader>& signed_proof,unsigned format):f(value){
  public_root=temp.path/"public";directory(public_root);const std::string campaign=sha("public-stopped-capture-format-"+std::to_string(format));
  const auto dbase=temp.path/"data",wbase=temp.path/"witness";directory(dbase);directory(wbase);directory(dbase/("isolated-capella-"+campaign));directory(wbase/("isolated-capella-"+campaign));data=dbase/("isolated-capella-"+campaign)/"mac-native";witness=wbase/("isolated-capella-"+campaign)/"mac-native";directory(data);directory(witness);
  auto owner=std::make_unique<Blockchain>(data/"chain",f.genesis);directory(witness/"chain-witness");auto independent=std::make_unique<ChainStore>(witness/"chain-witness");
  auto reactor=std::make_unique<CapellaDurableReactor>(*owner,f.rules,f.limits,f.initial,checkpoint,independent.get());check(reactor->proposer_slashing(signed_proof.first,signed_proof.second,checkpoint),"genuine pending proposer proof admitted");
  if(format==1)check(reactor->migrate_reference_checkpoint({2,256*1024},checkpoint),"genuine cold checkpoint migration");
  if(format==2)check(reactor->migrate_fragment_checkpoint(checkpoint),"genuine fragment checkpoint migration");
  primary_record=*owner->store().fork_checkpoint(f.genesis.chain_id,f.genesis.block.header.hash(),f.limits.envelope_bytes);witness_record=*independent->fork_checkpoint(f.genesis.chain_id,f.genesis.block.header.hash(),2048+8*f.limits.envelope_bytes);
  auto fetch=[&](const Bytes& id,size_t n){return owner->store().checkpoint_blob(f.genesis.chain_id,f.genesis.block.header.hash(),id,n);};const auto closure=authenticate_capella_checkpoint_blob_closure(primary_record.snapshot,primary_record.token.digest,f.rules,f.limits,fetch,{},checkpoint);closure_description=closure.description();closure_observation=closure.observation();
  check(closure_description.size()==3&&closure_description.contains("identity")&&closure_description.contains("executionHead")&&closure_description.contains("references"),"old three-field closure description unchanged");
  check(closure_observation.at("pending").at("proposerSlashings")==1&&closure_observation.at("retainedBlocks")==f.initial.blocks.size(),"actual v5 pending proof observed");auto altered=closure.observation();altered["headSlot"]="0xffff";check(closure.observation()==closure_observation,"observation is an owned copy");
  const auto context=closure.head_context();const auto genesis_root=hex(Bytes(context.genesis_root.begin(),context.genesis_root.end()));Json public_keys=Json::array(),keys=Json::array(),pins=Json::array();
  for(size_t i=0;i<65;++i)public_keys.push_back({{"index",i},{"publicKey",hex(TestKey(i).public_key)}});
  for(size_t i=0;i<16;++i){const auto jp=data/("journal-"+std::to_string(i)),wp=witness/("witness-"+std::to_string(i));directory(jp);directory(wp);ForkCheckpointToken retained;
   {ChainStore journal(jp),jw(wp);BeaconSigningJournal guard(journal,f.genesis.chain_id,f.genesis.block.header.hash(),context.genesis_root,TestKey(i).public_key,f.rules.transition.consensus.epoch.ffg.slots_per_epoch,{},&jw);retained=guard.commitment();}
   const auto index=quantity(Int(std::to_string(i)));keys.push_back({{"index",index},{"feeRecipient","0x000000000000000000000000000000000000002a"},{"keyEnv","NANO_TEST_NO_CAPTURE_SECRET_"+std::to_string(i)},{"journal",jp.string()},{"restart",token(retained)},{"recoveryWitness",wp.string()}});pins.push_back({{"index",index},{"restart",token(retained)}});
  }
  const auto& l=f.limits;Json limits={{"envelopeBytes",quantity(Int(std::to_string(l.envelope_bytes)))},{"blocks",quantity(Int(std::to_string(l.blocks)))},{"messages",quantity(Int(std::to_string(l.messages)))},{"equivocations",quantity(Int(std::to_string(l.equivocations)))},{"stateBytes",quantity(Int(std::to_string(l.state_bytes)))},{"totalStateBytes",quantity(Int(std::to_string(l.total_state_bytes)))},{"replaySlots",quantity(Int(std::to_string(l.replay_slots)))},{"syncTrials",quantity(Int(std::to_string(l.sync_trials)))}};
  config={{"capella",{{"profile",{{"file",(public_root/"profile.json").string()},{"sha256",hex(sha256(bytes_of(read(fs::path(NANO_TEST_SOURCE)/"tests/fixtures/capella-bootstrap-anchor-cb532/profile.json"))))}}},{"limits",limits},{"recoveryWitness",(witness/"chain-witness").string()},{"validator",{{"keys",keys}}}}}};
  stop={{"event","capella-runtime-stopped"},{"restart",token(primary_record.token)},{"clockHealthy",true},{"finalized",closure_observation.at("finalized")},{"justifiedEpoch",closure_observation.at("justified").at("epoch")},{"validator",{{"healthy",true},{"failure",nullptr},{"localKeys",16},{"awaitingRegistration",0}}},{"validatorKeys",pins}};
  // Test-generated independent stop authority is retained before the child.
  // This is not a claim that a production runtime emitted this fixture receipt.
  save_json(public_root/"genesis.json",f.genesis_document);check(Genesis::from_json(f.genesis_document).block.header.hash()==f.genesis.block.header.hash(),"serialized synthetic Genesis exact");save(public_root/"profile.json",read(fs::path(NANO_TEST_SOURCE)/"tests/fixtures/capella-bootstrap-anchor-cb532/profile.json"));save(public_root/"initial.json",capella_store_snapshot(f.initial,f.rules,f.limits,checkpoint));
  Json files=Json::object();for(const auto* name:{"genesis","profile","initial"}){const auto file=std::string(name)+".json",text=read(public_root/file);files[name]={{"path",file},{"sha256",sha(text)},{"bytes",text.size()}};}
  manifest={{"format","nano-isolated-capella-fixture-domain-v1"},{"campaignId",campaign},{"chainId",quantity(f.genesis.chain_id)},{"executionGenesisHash",hex(f.genesis.block.header.hash())},{"genesisValidatorsRoot",genesis_root},{"publicKeys",public_keys},{"files",files}};
  manifest_path=public_root/"manifest.json";config_path=public_root/"runtime.json";stop_path=public_root/"stop.json";save_json(manifest_path,manifest);save_json(config_path,config);save_json(stop_path,stop);
  reactor.reset();owner.reset();independent.reset();
 }
 Json observe(const fs::path& receipt,bool success){
  checkpoint();const auto before=Json::array({inventory(data),inventory(witness)});const auto signatures=test_signatures;::test::Process child({NANO_TEST_CAPTURE,"--capture",manifest_path.string(),config_path.string(),receipt.string(),data.string(),witness.string(),"mac-native"});++calls;const auto result=child.wait(25);const auto output=Json::parse(result.out);
  check(test_signatures==signatures&&before==Json::array({inventory(data),inventory(witness)}),"capture/refusal changed no source bytes or nine-field material");check(output.at("passed")==success&&(result.code==0)==success,"actual capture disposition");check(output.at("newSignaturesCreated")==0&&output.at("runtimeStarted")==false,"capture child never signs or runs duties");if(success)++accepted;else ++refused;return output;
 }
 fs::path pin_witnesses(const Json& first,const std::string& name="stop-pinned.json"){
  save(public_root/"retained-source-capture.json",first.dump()+"\n");auto pinned=stop;Json keys=Json::array();for(const auto& key:first.at("validatorKeys"))keys.push_back({{"index",key.at("index")},{"restart",key.at("witness")}});pinned["witnessCommitments"]={{"format","nano-isolated-stopped-external-witness-commitments-v1"},{"sourceCaptureSha256",sha(first.dump()+"\n")},{"core",first.at("recoveryWitnessCommitment")},{"validatorKeys",keys}};const auto path=public_root/name;save_json(path,pinned);return path;
 }
};
void emit(const char* scenario){++groups;std::cout<<Json{{"scenario",scenario},{"result","PASS"},{"actualCaptureCliInvocations",calls},{"captureSuccesses",accepted},{"captureRefusals",refused},{"testFixtureSignatures",test_signatures},{"captureSignatures",0},{"maxRssKiB",public_peak_rss_kib()},{"privateInputsRead",false},{"runtimeDutiesQualified",false},{"productionQualified",false}}.dump()<<std::endl;}
void formats(const Fixture& f,const std::pair<SignedConsensusBeaconHeader,SignedConsensusBeaconHeader>& signed_proof){
 for(unsigned format=0;format<3;++format){Sample sample(f,signed_proof,format);const auto first=sample.observe(sample.stop_path,true);check(first.at("observedWitnessPairsAuthenticated")==true&&first.at("externalWitnessTokensMatched")==false&&first.at("durablePendingMessageSemanticsVerified")==true&&first.at("canonicalHistoryReplayVerified")==false,"first capture scope exact");check(first.at("authenticatedCheckpointObservation")==sample.closure_observation,"captured sealed observation exact");const auto again=sample.observe(sample.pin_witnesses(first),true);check(again.at("externalWitnessTokensMatched")==true&&again.at("core")==first.at("core")&&again.at("validatorKeys")==first.at("validatorKeys"),"second capture exact externally retained P/W pins");emit(format==0?"stopped-capture-hot-v5":format==1?"stopped-capture-cold-v5":"stopped-capture-fragmented-v5");}
}
void bad_pins(const Fixture& f,const std::pair<SignedConsensusBeaconHeader,SignedConsensusBeaconHeader>& signed_proof){
 Sample sample(f,signed_proof,0);Json wrong=sample.stop;wrong["restart"]["digest"]="0x"+std::string(64,'0');auto path=sample.public_root/"wrong-primary.json";save_json(path,wrong);sample.observe(path,false);
 Json keys=Json::array();for(const auto& item:sample.stop.at("validatorKeys"))keys.push_back({{"index",item.at("index")},{"restart",item.at("restart")}});wrong=sample.stop;wrong["witnessCommitments"]={{"format","nano-isolated-stopped-external-witness-commitments-v1"},{"sourceCaptureSha256",sha("deliberately invalid public witness pin")},{"core",token(sample.primary_record.token)},{"validatorKeys",keys}};path=sample.public_root/"wrong-witness.json";save_json(path,wrong);sample.observe(path,false);emit("stopped-capture-external-pins-refused");
}
void bad_references(const Fixture& f,const std::pair<SignedConsensusBeaconHeader,SignedConsensusBeaconHeader>& signed_proof){
 Sample sample(f,signed_proof,1);const auto& refs=sample.closure_description.at("references");check(!refs.empty(),"cold fixture has a real required blob");const auto key="checkpoint-blob/v1/"+refs.at(0).at(0).get<std::string>();
 for(bool corrupt:{false,true}){auto db=writable_fixture(sample.data/"chain");rocksdb::WriteOptions write;write.sync=true;check((corrupt?db->Put(write,key,"not-the-committed-cold-record"):db->Delete(write,key)).ok(),"synthetic required blob changed before refusal inventory");db.reset();sample.observe(sample.stop_path,false);}
 emit("stopped-capture-missing-corrupt-cold-refs-refused");
}
}
int main(int argc,char** argv){try{check(argc==2&&std::string(argv[1])=="--self-test","usage --self-test");::umask(0077);const auto f=fixture();const auto signed_proof=proof(f);formats(f,signed_proof);bad_pins(f,signed_proof);bad_references(f,signed_proof);check(groups==5&&calls==10&&accepted==6&&refused==4&&test_signatures==4,"five groups/ten real captures/six accept/four refusals/four setup-only signatures");std::cout<<Json{{"scenario","stopped-capture-summary"},{"result","PASS"},{"semanticGroups",groups},{"actualCaptureCliInvocations",calls},{"captureSuccesses",accepted},{"captureRefusals",refused},{"testFixtureSignatures",test_signatures},{"captureSignatures",0},{"maxRssKiB",public_peak_rss_kib()},{"privateInputsRead",false},{"runtimeDutiesQualified",false},{"productionQualified",false}}.dump()<<std::endl;return 0;}catch(const std::exception& error){std::cout<<Json{{"diagnostic","stopped-capture-formats-refused"},{"message",error.what()},{"actualCaptureCliInvocations",calls},{"captureSignatures",0},{"testFixtureSignatures",test_signatures},{"productionQualified",false}}.dump()<<std::endl;return 1;}}
