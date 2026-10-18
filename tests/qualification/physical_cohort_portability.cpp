// Public, synthetic stopped-cohort fixture only. No scheduler, service or P2P.
#include "../support/durable_owned_fixture.hpp"
#include "nano/consensus_beacon_journal_auth.hpp"
#include "nano/consensus_capella_audit.hpp"
#include "../../tools/storage-maintenance/bootstrap-readonly-replica.hpp"
#include <csignal>
#include <optional>
#include <map>

namespace {
using namespace nano;
using namespace nano::test::durable_owned;
namespace rd=nano::maintenance::readonly_replica_detail;
constexpr uint64_t maximum_files=4096,maximum_bytes=128*1024*1024;
constexpr size_t maximum_manifest=1024*1024;
volatile std::sig_atomic_t interrupted=0;
void interrupt(int){interrupted=1;}
void guard(){checkpoint();check(!interrupted,"public cohort interrupted");check(test_signatures<=32,"public cohort signature cap");}
std::string digest(std::string_view s){return hex(sha256(Bytes(s.begin(),s.end()))).substr(2);}
std::string root_text(const ConsensusRoot& r){return hex(Bytes(r.begin(),r.end()));}
std::string number(uint64_t n){return quantity(Int(std::to_string(n)));}
Json pin(std::string_view s){return {{"bytes",s.size()},{"sha256",digest(s)}};}
Json token_json(const ForkCheckpointToken& t){return {{"sequence",number(t.sequence)},{"digest",hex(t.digest)}};}
void shape(const Json& j,std::initializer_list<const char*> keys){check(j.is_object()&&j.size()==keys.size(),"exact public manifest shape");for(auto k:keys)check(j.contains(k),"required public manifest field");}
std::string sha_text(const Json& j){check(j.is_string(),"SHA string");const auto s=j.get<std::string>();check(s.size()==64&&s.find_first_not_of("0123456789abcdef")==std::string::npos,"canonical SHA256");return s;}
uint64_t natural(const Json& j){check(j.is_string(),"canonical quantity string");auto n=u64(quantity(j));check(number(n)==j.get<std::string>(),"canonical quantity");return n;}
Bytes fixed(const Json& j,size_t n){check(j.is_string(),"hex string");const auto s=j.get<std::string>();check(s.size()==2+2*n&&s.starts_with("0x"),"hex width");auto b=unhex(s);check(b.size()==n&&hex(b)==s,"canonical hex bytes");return b;}
ForkCheckpointToken token_of(const Json& j){shape(j,{"sequence","digest"});auto n=natural(j.at("sequence"));check(n>0,"positive checkpoint sequence");return {n,fixed(j.at("digest"),32)};}
Json parse(std::string_view s){
 std::vector<std::set<std::string>> names;
 auto j=Json::parse(s.begin(),s.end(),[&](int depth,Json::parse_event_t event,Json& value){
  guard();check(depth>=0&&depth<=64,"public document depth");
  if(event==Json::parse_event_t::object_start)names.emplace_back();
  else if(event==Json::parse_event_t::object_end){check(!names.empty(),"JSON object stack");names.pop_back();}
  else if(event==Json::parse_event_t::key)check(!names.empty()&&names.back().insert(value.get<std::string>()).second,"duplicate JSON key");
  return true;
 });guard();return j;
}
struct Input {
 const rd::Directory& parent;std::string name;rd::Fd fd;struct stat original{};std::string text;
 Input(const rd::Directory& p,std::string leaf,size_t maximum):parent(p),name(std::move(leaf)),fd(rd::relative_fd(parent.fd(),name,O_RDONLY|O_NONBLOCK)){
  parent.recheck();rd::io(::fstat(fd.n,&original)==0);
  check(S_ISREG(original.st_mode)&&original.st_uid==::geteuid()&&original.st_nlink==1&&(original.st_mode&0022)==0&&original.st_size>0&&uint64_t(original.st_size)<=maximum,"bounded independent public input");
  text.resize(size_t(original.st_size));size_t at=0;
  while(at<text.size()){guard();auto n=::pread(fd.n,text.data()+at,std::min<size_t>(65536,text.size()-at),off_t(at));if(n<0&&errno==EINTR)continue;rd::io(n>0);at+=size_t(n);}fence();
 }
 void fence()const{
  guard();parent.recheck();struct stat held{},linked{};rd::io(::fstat(fd.n,&held)==0&&::fstatat(parent.fd(),name.c_str(),&linked,AT_SYMLINK_NOFOLLOW)==0);
  check(rd::stable(original,held)&&rd::stable(original,linked),"public input full identity unchanged");
  check(rd::file_hash(fd.n,original,guard)==digest(text),"public input bytes unchanged");parent.recheck();
 }
};
void write_new(const rd::Directory& parent,const std::string& leaf,std::string_view bytes){
 guard();check(!bytes.empty()&&bytes.size()<=maximum_manifest,"bounded public output");parent.recheck();
 rd::Fd fd(::openat(parent.fd(),leaf.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600));size_t at=0;
 while(at<bytes.size()){guard();auto n=::write(fd.n,bytes.data()+at,std::min<size_t>(65536,bytes.size()-at));if(n<0&&errno==EINTR)continue;rd::io(n>0);at+=size_t(n);}rd::sync(fd.n);rd::sync(parent.fd());parent.recheck();
 Input readback(parent,leaf,maximum_manifest);check(readback.text==bytes,"new public output exact durable readback");
}
void make_directory(const fs::path& p){check(::mkdir(p.c_str(),0700)==0,"new namespace must not exist");check(::chmod(p.c_str(),0700)==0,"new namespace private mode");}
CapellaStoreSnapshotLimits limits_for(const CapellaForkBlockRules& r){const auto max=r.transition.maximum_state_bytes;check(max>0&&max<=SIZE_MAX/128,"profile state maximum");return {16*1024*1024,128,256,4,max,128*max,64,100000};}
Json limits_json(const CapellaStoreSnapshotLimits& l){return Json::array({number(l.envelope_bytes),number(l.blocks),number(l.messages),number(l.equivocations),number(l.state_bytes),number(l.total_state_bytes),number(l.replay_slots),number(l.sync_trials)});}
// Portable names/type/bytes/digest deliberately exclude host inode/time/UID.
// Every individual scan nevertheless uses the established strict FD9 reader.
Json portable(const std::vector<rd::Entry>& entries){
 Json rows=Json::array();uint64_t total=0;
 for(const auto& e:entries){guard();if(S_ISDIR(e.stamp.st_mode))rows.push_back(Json::array({e.name,"directory"}));else{check(e.stamp.st_size>=0,"inventory size");total+=uint64_t(e.stamp.st_size);check(total<=maximum_bytes,"aggregate inventory bytes");rows.push_back(Json::array({e.name,"file",e.stamp.st_size,e.sha}));}}
 return {{"sha256",digest(rows.dump())},{"entries",entries.size()},{"bytes",total}};
}
// Diagnostic only: never recapture, mutate, reseal or replace the refusal.
Json physical_fd9(const struct stat& s){
#ifdef __APPLE__
 const auto mt=s.st_mtimespec,ct=s.st_ctimespec;
#else
 const auto mt=s.st_mtim,ct=s.st_ctim;
#endif
 auto time=[](const auto& t){return Json{{"seconds",std::to_string(std::intmax_t(t.tv_sec))},{"nanoseconds",std::to_string(std::intmax_t(t.tv_nsec))}};};
 return {{"st_dev",std::to_string(std::uintmax_t(s.st_dev))},{"st_ino",std::to_string(std::uintmax_t(s.st_ino))},{"st_mode",std::to_string(std::uintmax_t(s.st_mode))},{"st_nlink",std::to_string(std::uintmax_t(s.st_nlink))},{"st_uid",std::to_string(std::uintmax_t(s.st_uid))},{"st_gid",std::to_string(std::uintmax_t(s.st_gid))},{"st_size",std::to_string(std::intmax_t(s.st_size))},{"st_mtime",time(mt)},{"st_ctime",time(ct)}};
}
Json physical_entry(const rd::Entry* e){
 if(!e)return nullptr;
 return {{"type",S_ISDIR(e->stamp.st_mode)?"directory":S_ISREG(e->stamp.st_mode)?"file":"other"},{"sha256",e->sha},{"fd9",physical_fd9(e->stamp)}};
}
void physical_difference(const std::string& relative,const std::vector<rd::Entry>& before,const std::vector<rd::Entry>& after)noexcept{
 try{
  // Views refer only to the two retained immutable captures; no file is read.
  std::map<std::string_view,std::pair<const rd::Entry*,const rd::Entry*>> names;
  for(const auto& e:before)names[e.name].first=&e;for(const auto& e:after)names[e.name].second=&e;
  Json differences=Json::array();size_t changed=0;
  for(const auto& [name,pair]:names){const auto* a=pair.first;const auto* b=pair.second;
   if(a&&b&&a->sha==b->sha&&rd::stable(a->stamp,b->stamp))continue;
   ++changed;if(differences.size()==32)continue;
   bool printable=true;for(unsigned char c:name)if(c<32||c>126)printable=false;
   differences.push_back(Json{{"name",printable?Json(std::string(name)):Json(nullptr)},{"nameHex",hex(Bytes(name.begin(),name.end()))},{"before",physical_entry(a)},{"after",physical_entry(b)}});
  }
  Json result={{"diagnostic","physical-closed-tree-difference-v1"},{"stage",test_stage},{"rootRelative",relative},{"role",relative=="."?"root":fs::path(relative).filename().string()},{"beforeEntries",before.size()},{"afterEntries",after.size()},{"namedDifferences",changed},{"maximumReportedDifferences",32},{"differences",differences},{"truncated",changed>differences.size()},{"captureRepeated",false},{"oracleRelaxed",false}};
  auto text=result.dump();if(text.size()>1024*1024){result["differences"]=Json::array();result["truncated"]=true;result["byteCapExceeded"]=true;text=result.dump();}
  std::cerr<<text<<std::endl;
 }catch(...){try{std::cerr<<"{\"diagnostic\":\"physical-closed-tree-difference-unavailable-v1\",\"originalRefusalPreserved\":true}"<<std::endl;}catch(...){}}
}
// Context only: preserve the original capture exception and its single call.
std::vector<rd::Entry> capture_labeled(int fd,const std::string& relative,const char* phase){
 auto diagnose=[&](const char* message)noexcept{try{
  std::string what=message?message:"exception without message";if(what.size()>512)what.resize(512);
  std::cerr<<Json{{"diagnostic","physical-capture-context-v1"},{"stage",test_stage},{"rootRelative",relative},{"role",relative=="."?"root":fs::path(relative).filename().string()},{"phase",phase},{"what",what},{"maximumWhatBytes",512},{"captureRepeated",false},{"originalExceptionRethrown",true}}.dump()<<std::endl;
 }catch(...){}};
 try{return rd::capture(fd,maximum_files,maximum_bytes,guard);}
 catch(const std::exception& e){diagnose(e.what());throw;}
 catch(...){diagnose("non-standard exception");throw;}
}
struct PhysicalFence {
 rd::Directory directory;std::vector<rd::Entry> entries;std::string relative;
 explicit PhysicalFence(const fs::path& p,std::string label="."):directory(p),entries(capture_labeled(directory.fd(),label,"initial")),relative(std::move(label)){}
 Json description()const{return portable(entries);}
 void fence()const{directory.recheck();const auto after=capture_labeled(directory.fd(),relative,"recapture");if(!rd::same_entries(entries,after))physical_difference(relative,entries,after);check(rd::same_entries(entries,after),"closed physical tree FD9 and bytes unchanged");directory.recheck();}
};
ForkCheckpointRecord record(ChainStore& d,const Int& chain,const Bytes& genesis,size_t max,const ForkCheckpointToken& expected){auto r=d.fork_checkpoint(chain,genesis,max,expected);check(bool(r),"required original checkpoint present");return *r;}
Json record_pin(const ForkCheckpointRecord& r){return {{"token",token_json(r.token)},{"payload",pin(r.snapshot)}};}
void record_matches(const ForkCheckpointRecord& r,const Json& expected){shape(expected,{"token","payload"});check(record_pin(r)==expected,"exact checkpoint token/payload from external manifest");}
std::vector<CheckpointArchiveEntry> intent_archive(const Json& j,size_t maximum){
 check(j.is_array()&&j.size()<=1,"bounded witness archive");std::vector<CheckpointArchiveEntry> out;
 for(const auto& x:j){guard();check(x.is_array()&&x.size()==3&&x[2].is_string(),"witness archive shape");const auto& s=x[2].get_ref<const std::string&>();check(s.size()>2&&s.size()<=2+2*maximum&&s.size()%2==0,"witness wire bound");auto b=unhex(s);check(hex(b)==s,"witness wire canonical");out.push_back({fixed(x[0],32),natural(x[1]),std::move(b)});}return out;
}
Json reservation(const BeaconJournalReservations& r){
 auto root=[](const std::optional<ConsensusRoot>& x)->Json{return x?Json(root_text(*x)):Json(nullptr);};Json proposal=nullptr,attestation=nullptr;
 if(r.proposal)proposal={{"slot",number(r.proposal->slot)},{"root",root(r.proposal->root)}};
 if(r.attestation)attestation={{"source",number(r.attestation->source)},{"target",number(r.attestation->target)},{"root",root(r.attestation->root)}};
 return {{"proposal",proposal},{"attestation",attestation},{"retired",r.retired}};
}
ConsensusBeaconHeader header_of(const Bytes& wire,const CapellaForkBlockRules& rules){
 const auto& t=rules.transition;check(wire.size()>=404&&wire.size()<=t.consensus.maximum_block_bytes&&test_little(wire,0,4)==100&&test_little(wire,180,4)==84,"full signed child wire");
 return {test_little(wire,100),test_little(wire,108),test_root(wire,116),test_root(wire,148),capella_beacon_body_root(std::span<const uint8_t>(wire).subspan(184),t.consensus.body,t.consensus.state.sync_committee_size,t.execution,t.withdrawals.maximum_per_payload,t.maximum_bls_changes,t.consensus.maximum_block_bytes,guard)};
}
struct Pair {
 const Fixture& f;fs::path path;std::unique_ptr<Blockchain> owner;std::unique_ptr<ChainStore> witness;std::unique_ptr<CapellaDurableReactor> node;
 Pair(const Fixture& x,fs::path p):f(x),path(std::move(p)){
  make_directory(path);for(auto name:{"primary","witness","signer","signer-witness"})make_directory(path/name);
  owner=std::make_unique<Blockchain>(path/"primary",f.genesis);witness=std::make_unique<ChainStore>(path/"witness");node=std::make_unique<CapellaDurableReactor>(*owner,f.rules,f.limits,f.initial,guard,witness.get());
 }
 void close(){node.reset();witness.reset();owner.reset();}
 ~Pair(){close();}
};
struct LastJournal {Json primary,witness;uint64_t index=0;std::string public_key,signing_root;uint64_t slot=0;};
// Same complete block preparation as the public owned fixture; the final child
// uses the real journal before the public provider supplies its BLS signature.
Bytes child(Pair& p,uint64_t slot,LastJournal* journal){
 const auto& f=p.f;const auto& t=f.rules.transition;p.node->tick(f.initial.timed.clock.genesis_time_ms+slot*f.rules.slot_duration_ms,guard);
 for(size_t tries=0;!p.node->prepare_current_duties(slot,guard);++tries)check(tries<8,"finite public child preparation");
 const auto index=p.node->current_proposer(slot,guard);check(bool(index),"actual public child proposer");const auto c=p.node->current();const auto parent=capella_fork_head(*c.state,f.rules,guard);
 const auto ctx=capella_fork_block_context(parent->state,parent->header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state,guard);
 const auto epoch=slot/t.consensus.epoch.ffg.slots_per_epoch;const auto version=epoch<ctx.fork.epoch?ctx.fork.previous_version:ctx.fork.current_version;
 TestKey key(*index);check(key.public_key==Bytes(ctx.registry.validators.at(*index).public_key.begin(),ctx.registry.validators.at(*index).public_key.end()),"fixture key actually selected");
 ConsensusRoot epoch_root{};for(size_t i=0;i<8;++i)epoch_root[i]=uint8_t(epoch>>(8*i));
 const auto reveal=key.sign(consensus_signing_root(epoch_root,consensus_domain({2,0,0,0},version,ctx.genesis_root)));
 const auto body=p.node->prepare_local_body(*index,slot,reveal,parent->execution_block.header.coinbase,c.token,{},1,64,guard);check(bool(body),"complete normal child body");
 const auto parent_root=consensus_beacon_header_root(parent->header);auto history=[&](const Int& n){return capella_execution_history(*c.state,parent_root,n,guard);};
 auto candidate=prepare_capella_unsigned_block(parent->state,slot,*index,parent_root,*body,t,parent->execution_block.header,parent->execution_state,history,f.limits.replay_slots,f.limits.sync_trials,guard);check(bool(candidate),"independent complete transition");
 const auto& m=candidate->message_ssz;const auto body_root=capella_beacon_body_root(std::span<const uint8_t>(m).subspan(84),t.consensus.body,t.consensus.state.sync_committee_size,t.execution,t.withdrawals.maximum_per_payload,t.maximum_bls_changes,t.consensus.maximum_block_bytes,guard);
 const ConsensusBeaconHeader header{test_little(m,0),test_little(m,8),test_root(m,16),test_root(m,48),body_root};const auto signing=consensus_signing_root(consensus_beacon_header_root(header),consensus_domain({0,0,0,0},version,ctx.genesis_root));Bytes signature;
 if(journal){
  ChainStore disk(p.path/"signer"),witness(p.path/"signer-witness");BeaconSigningJournal signer(disk,f.genesis.chain_id,f.genesis.block.header.hash(),ctx.genesis_root,key.public_key,t.consensus.epoch.ffg.slots_per_epoch,std::nullopt,&witness);
  size_t calls=0;signature=signer.sign_proposal(header,version,[&](const ConsensusRoot& root){guard();check(++calls==1&&root==signing,"actual journal signing root");return key.sign(root);});check(calls==1,"exact single public provider call");
  const auto pr=disk.fork_checkpoint(f.genesis.chain_id,f.genesis.block.header.hash(),4096,signer.commitment()),wr=witness.fork_checkpoint(f.genesis.chain_id,f.genesis.block.header.hash(),16384);check(pr&&wr,"both real signer records");
  journal->primary=record_pin(*pr);journal->witness=record_pin(*wr);journal->index=*index;journal->public_key=hex(key.public_key);journal->signing_root=root_text(signing);journal->slot=slot;
 }else signature=key.sign(signing);
 Bytes wire(100);test_put(wire,0,100,4);std::copy(signature.begin(),signature.end(),wire.begin()+4);wire.insert(wire.end(),m.begin(),m.end());
 bool accepted=false;for(size_t i=0;i<8&&!accepted;++i){auto d=p.node->block_admission(wire,guard);check(d!=CapellaBlockAdmission::rejected,"real normal signed child refused");accepted=d==CapellaBlockAdmission::accepted;}check(accepted,"bounded normal child admission");return wire;
}
Json produce_cohort(const Fixture& f,const fs::path& root,const std::string& name,uint64_t children){
 test_stage=name=="alpha"?"produce-alpha":"produce-beta";Json row;{
  Pair p(f,root/name);LastJournal journal;std::vector<Bytes> wires;
  for(uint64_t i=0;i<children;++i)wires.push_back(child(p,i+2,i+1==children?&journal:nullptr));
  // Keep one independently signed, validated operation pending in the cold DB.
  const uint64_t change_index=name=="alpha"?10:11;SignedCapellaBlsChange change{};change.validator_index=change_index;TestKey withdrawal(10000+change_index);test_copy(change.from_bls_pubkey,withdrawal.public_key);change.to_execution_address.back()=uint8_t(change_index);
  const auto& t=f.rules.transition;const auto genesis_root=[&]{const auto h=capella_fork_head(*p.node->current().state,f.rules,guard);return test_root(h->state.ssz,8);}();
  test_copy(change.signature,withdrawal.sign(consensus_signing_root(capella_bls_change_root(change),consensus_domain({10,0,0,0},t.genesis_version,genesis_root))));
  const auto change_wire=encode_capella_bls_change(change);check(p.node->bls_change_ssz(change_wire,guard),"genuine pending BLS change");
  check(p.node->migrate_fragment_checkpoint(guard),"actual fragment migration");check(p.node->migrate_reference_checkpoint({2,256*1024},guard),"actual cold reference migration");
  check(p.node->pending_bls_changes().size()==1&&encode_capella_bls_change(p.node->pending_bls_changes().front())==change_wire,"pending operation retained after cold migration");
  const auto primary=p.owner->store().fork_checkpoint(f.genesis.chain_id,f.genesis.block.header.hash(),f.limits.envelope_bytes);const auto witness=p.witness->fork_checkpoint(f.genesis.chain_id,f.genesis.block.header.hash(),2048+8*f.limits.envelope_bytes);check(primary&&witness,"real core P/W checkpoints");
  auto fetch=[&](const Bytes& id,size_t n){return p.owner->store().checkpoint_blob(f.genesis.chain_id,f.genesis.block.header.hash(),id,n);};const auto closure=authenticate_capella_checkpoint_blob_closure(primary->snapshot,primary->token.digest,f.rules,f.limits,fetch,{},guard);
  check(!closure.description().at("references").empty()&&closure.observation().at("pending").at("blsChanges")==1,"nonempty authenticated cold closure and queue");
  Json archives=Json::array();
  for(const auto& wire:wires){const auto header=header_of(wire,f.rules);const auto id=consensus_beacon_header_root(header);const auto stored=p.owner->store().checkpoint_archive(Bytes(id.begin(),id.end()),f.limits.envelope_bytes);check(stored&&stored->slot==header.slot&&stored->payload==wire,"actual archived signed child");archives.push_back(Json{{"root",root_text(id)},{"slot",number(header.slot)},{"wire",pin(std::string_view(reinterpret_cast<const char*>(wire.data()),wire.size()))}});}
  row={{"name",name},{"children",children},{"primary",record_pin(*primary)},{"witness",record_pin(*witness)},
   {"signer",journal.primary},{"signerWitness",journal.witness},{"journal",{{"index",journal.index},{"publicKey",journal.public_key},{"proposalSlot",number(journal.slot)},{"proposalSigningRoot",journal.signing_root}}},
   {"closure",closure.description()},{"observation",closure.observation()},{"headRoot",root_text(closure.head_context().block_root)},{"archives",archives}};
  check(p.node->current().token==primary->token,"current publication token");p.close();
 }
 // All mutable DBs/signing handles have been destroyed before physical seals.
 Json physical=Json::object();for(auto role:{"primary","witness","signer","signer-witness"}){PhysicalFence fencer(root/name/role,name+"/"+role);physical[role]=fencer.description();fencer.fence();}row["physical"]=std::move(physical);return row;
}
Json verify_cohort(const fs::path& root,const Json& row,const Json& chain,const CapellaForkBlockRules& rules,const CapellaStoreSnapshotLimits& limits,const CheckpointArchiveEntry& anchor,const std::string& name,uint64_t children){
 test_stage=name=="alpha"?"check-alpha":"check-beta";shape(row,{"name","children","primary","witness","signer","signerWitness","journal","closure","observation","headRoot","archives","physical"});
 check(row.at("name")==name&&row.at("children")==children,"exact two independent cohort namespaces");
 PhysicalFence cohort(root/name,name);std::set<std::string> roles;for(const auto& e:cohort.entries)if(!e.name.empty()&&fs::path(e.name).parent_path().empty())roles.insert(e.name);
 check(roles==std::set<std::string>{"primary","witness","signer","signer-witness"},"exact role directories, no uncommitted sibling inputs");
 std::vector<std::unique_ptr<PhysicalFence>> fences;
 for(auto role:{"primary","witness","signer","signer-witness"}){auto f=std::make_unique<PhysicalFence>(root/name/role,name+"/"+role);check(f->description()==row.at("physical").at(role),"portable closed physical inventory matches external commitment");fences.push_back(std::move(f));}
 shape(row.at("physical"),{"primary","witness","signer","signer-witness"});
 auto close_fence=[&]{for(const auto& f:fences)f->fence();cohort.fence();};
 const auto chain_id=quantity(chain.at("chainId"));const auto genesis=fixed(chain.at("genesisHash"),32);const auto staking=chain.at("stakingContract").get<std::string>();
 std::optional<AuthenticatedCheckpointBlobClosure> authenticated;std::optional<ConsensusBeaconHeader> last_header;
 Json canonical_history_audit;bool external_finalized_before_anchor_refused=false;
 {
  ChainStore primary(root/name/"primary",ChainStore::OpenMode::ReadOnly),witness(root/name/"witness",ChainStore::OpenMode::ReadOnly);
  primary.require_existing_chain_binding(chain_id,genesis,"beacon",staking);witness.require_existing_chain_binding(chain_id,genesis,"nano-capella-recovery-v1");
  const auto p=record(primary,chain_id,genesis,limits.envelope_bytes,token_of(row.at("primary").at("token")));
  const auto w=record(witness,chain_id,genesis,2048+8*limits.envelope_bytes,token_of(row.at("witness").at("token")));record_matches(p,row.at("primary"));record_matches(w,row.at("witness"));
  auto fetch=[&](const Bytes& id,size_t n){return primary.checkpoint_blob(chain_id,genesis,id,n);};authenticated.emplace(authenticate_capella_checkpoint_blob_closure(p.snapshot,p.token.digest,rules,limits,fetch,{anchor},guard));
  check(authenticated->description().at("identity").at("networkAnchor")==hex(anchor.root),"actual signed anchor binds the complete checkpoint network identity");
  const auto context=authenticated->head_context();check(context.registry.validators.size()==64&&root_text(context.block_root)==row.at("headRoot").get<std::string>(),"authenticated exact head and complete registry");
  check(authenticated->description()==row.at("closure")&&authenticated->observation()==row.at("observation")&&!authenticated->description().at("references").empty(),"complete cold closure/operations match external manifest");
  check(authenticated->observation().at("pending").at("blsChanges")==1,"real pending operation retained");
  const auto g=primary.block_by_number(0);const auto gs=primary.state_at(0);check(g&&gs&&g->header.hash()==genesis&&gs->root()==g->header.state_root,"existing execution genesis block and state audited");
  const auto tip=primary.latest();check(bool(tip),"existing execution head");const auto state=primary.state_at(tip->header.number);check(state&&state->root()==tip->header.state_root,"existing canonical execution state root");
  const auto expected=Json{{"number",quantity(tip->header.number)},{"hash",hex(tip->header.hash())},{"stateRoot",hex(state->root())}};check(expected==authenticated->description().at("executionHead"),"execution/checkpoint coupling");
  const auto intent=parse(w.snapshot);shape(intent,{"format","previous","next","snapshot","archive"});check(intent.at("format")=="nano-capella-recovery-v1"&&intent.dump()==w.snapshot&&intent.at("snapshot").is_string(),"canonical recovery witness namespace");
  check(token_of(intent.at("next"))==p.token&&intent.at("snapshot").get_ref<const std::string&>()==p.snapshot,"completed witness exactly matches primary, no recovery");
  if(intent.at("previous").is_null())check(p.token.sequence==1,"initial witness sequence");else{const auto previous=token_of(intent.at("previous"));check(previous.sequence<UINT64_MAX&&previous.sequence+1==p.token.sequence,"witness CAS predecessor sequence");}
  auto witness_fetch=[&](const Bytes& id,size_t n){return witness.checkpoint_blob(chain_id,genesis,id,n);};const auto witness_closure=authenticate_capella_checkpoint_blob_closure(p.snapshot,p.token.digest,rules,limits,witness_fetch,{anchor},guard);
  for(const auto& entry:intent_archive(intent.at("archive"),limits.envelope_bytes)){const auto verified=authenticate_capella_checkpoint_blob_closure(p.snapshot,p.token.digest,rules,limits,witness_fetch,{entry},guard);check(verified.description()==witness_closure.description(),"Core authenticates any completed witness archive");}
  check(witness_closure.description()==authenticated->description()&&witness_closure.observation()==authenticated->observation(),"independent witness complete closure exact");
  check(row.at("archives").is_array()&&row.at("archives").size()==children,"exact child archive count");uint64_t at=0;
  for(const auto& a:row.at("archives")){
   guard();shape(a,{"root","slot","wire"});const auto id=fixed(a.at("root"),32);const auto entry=primary.checkpoint_archive(id,limits.envelope_bytes);check(entry&&entry->root==id&&entry->slot==natural(a.at("slot"))&&entry->slot==2+at,"original signed archive and slot sequence");
   const auto bytes=std::string_view(reinterpret_cast<const char*>(entry->payload.data()),entry->payload.size());check(pin(bytes)==a.at("wire"),"external archive payload digest");
   const auto verified=authenticate_capella_checkpoint_blob_closure(p.snapshot,p.token.digest,rules,limits,fetch,{*entry},guard);check(verified.description()==authenticated->description(),"Core authenticates actual archived BLS signature and retained state");
   last_header=header_of(entry->payload,rules);check(root_text(consensus_beacon_header_root(*last_header))==a.at("root").get<std::string>(),"archived beacon header root");++at;
  }
  // Export no authority and construct no signing provider: recover only the
  // externally committed, already authenticated anchor through the RO reader.
  CapellaForkStore trusted_anchor;
  {
   CapellaDurableReactor restored(primary,chain_id,genesis,rules,limits,p.token,guard);
   const auto publication=restored.current();check(publication.token==p.token,"audit anchor uses exact external checkpoint");
   const auto anchor_root=test_root(anchor.root);const auto retained=publication.state->blocks.at(anchor_root).lease(guard);
   check(consensus_beacon_header_root(retained->header)==anchor_root&&retained->header.slot==anchor.slot,"audit anchor matches external signed archive");
   trusted_anchor=initialize_capella_fork_store(retained->state,std::span<const uint8_t>(anchor.payload).subspan(100),retained->execution_block,retained->execution_state,retained->receipts,{g->header},rules,guard,retained->deposit_history);
   check(trusted_anchor.blocks.size()==1&&trusted_anchor.timed.view.anchor==anchor_root,"audit uses reconstructed singleton anchor");
  }
  const auto& finalized=row.at("observation").at("finalized");shape(finalized,{"epoch","root"});
  ConsensusCheckpoint required_finalized{natural(finalized.at("epoch")),test_root(fixed(finalized.at("root"),32))};
  canonical_history_audit=audit_capella_canonical_history(primary,chain_id,genesis,rules,limits,trusted_anchor,p.token,1+children,children*limits.envelope_bytes,guard,std::nullopt);
  check(canonical_history_audit.at("canonicalHistoryVerified")==true&&canonical_history_audit.at("externalFinalizedAncestorVerified")==false&&canonical_history_audit.at("requiredFinalized").is_null(),"real Core canonical audit without historical finalized ancestry claim");
  check(canonical_history_audit.at("checkpointSequence")==p.token.sequence&&canonical_history_audit.at("checkpointDigest")==hex(p.token.digest)&&canonical_history_audit.at("head")==row.at("headRoot")&&canonical_history_audit.at("trustedAnchor")==hex(anchor.root),"canonical audit matches exact external checkpoint/head/anchor");
  check(canonical_history_audit.at("finalized")==finalized,"canonical audit matches externally committed finalized checkpoint");
  // Epoch zero precedes this fixture's retained slot-one anchor. Preserve the
  // ancestry rule: that external checkpoint must be refused, never inferred.
  try{(void)audit_capella_canonical_history(primary,chain_id,genesis,rules,limits,trusted_anchor,p.token,1+children,children*limits.envelope_bytes,guard,required_finalized);}
  catch(const std::invalid_argument& error){if(std::string(error.what())!="observed finalized checkpoint outside replayed ancestry")throw;external_finalized_before_anchor_refused=true;}
  check(external_finalized_before_anchor_refused,"exact external finalized-before-anchor refusal required");
  // This historical synthetic fixture has empty execution blocks: no transaction
  // replay coverage is inferred from the actual canonical-history audit.
  check(canonical_history_audit.at("blocksIncludingTrustedAnchor")==1+children&&canonical_history_audit.at("transactions")==0,"fixture canonical audit block and empty-transaction counts");
 }
 // Every DB handle above is destroyed before immutable-file fences/reopen.
 close_fence();check(authenticated&&last_header,"complete independent readonly capture");
 const auto context=authenticated->head_context();const auto& j=row.at("journal");shape(j,{"index","publicKey","proposalSlot","proposalSigningRoot"});
 check(j.at("index").is_number_unsigned(),"journal registry index type");const auto index=j.at("index").get<uint64_t>();check(index<64&&index==last_header->proposer_index&&natural(j.at("proposalSlot"))==last_header->slot,"journal belongs to actual last admitted proposer");
 const auto public_key=fixed(j.at("publicKey"),48);const auto epoch=last_header->slot/rules.transition.consensus.epoch.ffg.slots_per_epoch;const auto version=epoch<context.fork.epoch?context.fork.previous_version:context.fork.current_version;
 const auto signing_root=consensus_signing_root(consensus_beacon_header_root(*last_header),consensus_domain({0,0,0,0},version,context.genesis_root));check(root_text(signing_root)==j.at("proposalSigningRoot").get<std::string>(),"actual archived header signing root");
 {
  ChainStore primary(root/name/"signer",ChainStore::OpenMode::ReadOnly),witness(root/name/"signer-witness",ChainStore::OpenMode::ReadOnly);
  primary.require_existing_chain_binding(chain_id,genesis,"nano-beacon-signing-v1");witness.require_existing_chain_binding(chain_id,genesis,"nano-beacon-signing-witness-v1");
  const auto p=record(primary,chain_id,genesis,4096,token_of(row.at("signer").at("token"))),w=record(witness,chain_id,genesis,16384,token_of(row.at("signerWitness").at("token")));record_matches(p,row.at("signer"));record_matches(w,row.at("signerWitness"));
  const auto pair=authenticate_beacon_signing_journal_pair({p,w,p.token,w.token,p.token,w.token},{public_key,context.genesis_root,rules.transition.consensus.epoch.ffg.slots_per_epoch},guard);
  check(pair.disposition()==BeaconJournalPairDisposition::Completed&&pair.primary_floor().current_equals_floor&&pair.witness_floor().current_equals_floor,"exact external journal tokens, completed pair only");
  const auto registered=observe_beacon_journal_registration(pair,context.registry,index,64,guard);check(registered.index==std::optional<uint64_t>(index),"actual journal key registered on authenticated head");
  const Json expected={{"proposal",{{"slot",number(last_header->slot)},{"root",root_text(signing_root)}}},{"attestation",nullptr},{"retired",false}};
  check(reservation(pair.primary())==expected&&reservation(pair.candidate())==expected,"exact durable journal reservation for delivered fixture block");
 }
 close_fence();return {{"name",name},{"primaryToken",row.at("primary").at("token")},{"witnessToken",row.at("witness").at("token")},{"headRoot",row.at("headRoot")},{"archivesAuthenticated",children},{"journalProposalBoundToArchivedBlock",true},{"pendingBlsChanges",1},{"coldClosureAuthenticated",true},{"canonicalHistoryAudit",canonical_history_audit},{"externalFinalizedBeforeAnchorRefused",external_finalized_before_anchor_refused},{"signedAnchorAuthenticatedOnBothStores",true},{"executionCoupled",true},{"allFourStoresReadOnly",true},{"closedPhysicalInventoryUnchanged",true}};
}
Json verify_manifest(const fs::path& root,const Json& manifest){
 const auto signatures_before=test_signatures;
 shape(manifest,{"format","syntheticPublicFixture","fixtureSeedBase","chain","profile","limits","anchor","cohorts","newTestOnlySignaturesCreated","budgets","scope"});
 check(manifest.at("format")=="nano-physical-authenticated-public-cohort-v1"&&manifest.at("syntheticPublicFixture")==true&&manifest.at("fixtureSeedBase")==1800001,"exact public fixture format");
 check(manifest.at("newTestOnlySignaturesCreated")==14,"exact fixture signature count, no operational history claim");
 check(manifest.at("budgets")==Json{{"maximumFiles",maximum_files},{"maximumBytes",maximum_bytes},{"maximumRssKiB",131072},{"maximumWallSeconds",180},{"maximumProduceSignatures",32},{"maximumCheckSignatures",0}},"fixed administrative fixture budgets");
 check(manifest.at("scope")==Json{{"runtimeDutiesAuthorized",false},{"networkTransport",false},{"productionQualified",false},{"finalityAdvanceQualified",false},{"historicalSignatureDeliveryVerified",false}},"no operational authorization");
 const auto& chain=manifest.at("chain");shape(chain,{"chainId","genesisHash","stakingContract"});check(quantity(chain.at("chainId"))>0&&quantity(quantity(chain.at("chainId")))==chain.at("chainId").get<std::string>(),"chain ID canonical");fixed(chain.at("genesisHash"),32);check(chain.at("stakingContract").is_string(),"staking identity string");
 rd::Directory directory(root);Input profile(directory,"profile.json",maximum_manifest);check(pin(profile.text)==manifest.at("profile")&&digest(profile.text)=="e3440eba0a871a8eedf84a6e2cdccee53cc4389e8ce6b979166621c8e893890d","original pinned public profile");
 const auto rules=load_capella_fork_profile(profile.text,sha256(bytes_of(profile.text)),maximum_manifest);check(rules.transition.chain_id==quantity(chain.at("chainId")),"chain/profile agreement");const auto limits=limits_for(rules);check(limits_json(limits)==manifest.at("limits"),"exact fixed profile-derived limits");
 const auto& a=manifest.at("anchor");shape(a,{"root","slot","ssz"});check(a.at("ssz").is_string(),"public anchor signed wire");const auto& encoded=a.at("ssz").get_ref<const std::string&>();check(encoded.size()>2&&encoded.size()<=2+2*rules.transition.consensus.maximum_block_bytes&&encoded.size()%2==0,"public anchor wire bound");
 const auto wire=unhex(encoded);check(hex(wire)==encoded&&natural(a.at("slot"))==1,"canonical public slot1 anchor");const auto header=header_of(wire,rules);check(header.slot==1&&root_text(consensus_beacon_header_root(header))==a.at("root").get<std::string>(),"actual anchor signed header root");const CheckpointArchiveEntry anchor{fixed(a.at("root"),32),1,wire};
 const auto& cohorts=manifest.at("cohorts");check(cohorts.is_array()&&cohorts.size()==2,"two fixture cohorts only");
 check(cohorts[0].at("headRoot")!=cohorts[1].at("headRoot")&&cohorts[0].at("primary").at("token")!=cohorts[1].at("primary").at("token")&&cohorts[0].at("signer").at("token")!=cohorts[1].at("signer").at("token"),"cohort mix carries distinct independent commitments");
 Json observations=Json::array();observations.push_back(verify_cohort(root,cohorts[0],chain,rules,limits,anchor,"alpha",2));observations.push_back(verify_cohort(root,cohorts[1],chain,rules,limits,anchor,"beta",3));profile.fence();check(test_signatures==signatures_before,"readonly authentication never calls a signing provider");return observations;
}
void report(const char* mode,const Json& rows,const std::string& manifest_sha,size_t passes){
 for(const auto& row:rows){Json result=row;result["scenario"]="physical-authenticated-cohort";result["mode"]=mode;result["result"]="PASS";result["independentReadonlyPasses"]=passes;std::cout<<result.dump()<<std::endl;}
 guard();std::cout<<Json{{"scenario","physical-authenticated-cohort-summary"},{"mode",mode},{"result","PASS"},{"manifestSha256",manifest_sha},{"cohorts",2},{"databases",8},{"archivesAuthenticated",5},{"independentReadonlyPasses",passes},{"newTestOnlySignaturesCreated",test_signatures},{"newSignaturesInCheck",0},{"actualPeakRssKiB",public_peak_rss_kib()},{"maximumRssKiB",131072},{"maximumWallSeconds",180},{"noMissingStoreCreation",true},{"automaticRecoveryUsed",false},{"runtimeDutiesAuthorized",false},{"networkTransport",false},{"productionQualified",false},{"finalityAdvanceQualified",false}}.dump()<<std::endl;
}
void produce(const fs::path& root){
 test_stage="create-public-fixture";check(fs::canonical(root.parent_path())==root.parent_path(),"physical canonical existing parent");make_directory(root);rd::Directory directory(root);
 const auto f=fixture();check(test_signatures==2,"two actual signed public anchor operations");
 const auto profile=read(fs::path(NANO_TEST_SOURCE)/"tests/fixtures/capella-bootstrap-anchor-cb532/profile.json");check(digest(profile)=="e3440eba0a871a8eedf84a6e2cdccee53cc4389e8ce6b979166621c8e893890d","public profile literal pin");write_new(directory,"profile.json",profile);
 auto cohorts=Json::array();cohorts.push_back(produce_cohort(f,root,"alpha",2));cohorts.push_back(produce_cohort(f,root,"beta",3));check(test_signatures==14,"exact two-cohort public signatures");
 Json manifest={{"format","nano-physical-authenticated-public-cohort-v1"},{"syntheticPublicFixture",true},{"fixtureSeedBase",1800001},
  {"chain",{{"chainId",quantity(f.genesis.chain_id)},{"genesisHash",hex(f.genesis.block.header.hash())},{"stakingContract",f.genesis.staking_contract}}},{"profile",pin(profile)},{"limits",limits_json(f.limits)},{"anchor",{{"root",root_text(f.initial.timed.view.anchor)},{"slot",number(1)},{"ssz",hex(f.wire)}}},{"cohorts",cohorts},{"newTestOnlySignaturesCreated",test_signatures},
  {"budgets",{{"maximumFiles",maximum_files},{"maximumBytes",maximum_bytes},{"maximumRssKiB",131072},{"maximumWallSeconds",180},{"maximumProduceSignatures",32},{"maximumCheckSignatures",0}}},
  {"scope",{{"runtimeDutiesAuthorized",false},{"networkTransport",false},{"productionQualified",false},{"finalityAdvanceQualified",false},{"historicalSignatureDeliveryVerified",false}}}};
 test_stage="produce-readonly-reopen-first";const auto first=verify_manifest(root,manifest);test_stage="produce-readonly-reopen-second";check(verify_manifest(root,manifest)==first,"two fresh RO passes preserve exact authenticated observations");
 write_new(directory,"manifest.json",manifest.dump());Input final_manifest(directory,"manifest.json",maximum_manifest);check(parse(final_manifest.text)==manifest,"durable manifest exact");PhysicalFence closed(root);check(closed.description().at("bytes").get<uint64_t>()<=maximum_bytes,"whole cohort byte budget");closed.fence();final_manifest.fence();report("produce",first,digest(final_manifest.text),2);
}
void inspect(const fs::path& root,const std::string& expected){
 test_stage="check-external-manifest";sha_text(Json(expected));PhysicalFence closed(root);Input manifest(closed.directory,"manifest.json",maximum_manifest);check(digest(manifest.text)==expected,"external independently retained manifest SHA256 required");
 const auto document=parse(manifest.text);check(document.dump()==manifest.text,"canonical public manifest bytes");
 // Only these top-level names are admitted. No broad discovery/open/recovery.
 std::set<std::string> names;for(const auto& e:closed.entries)if(!e.name.empty()&&fs::path(e.name).parent_path().empty())names.insert(e.name);
 check(names==std::set<std::string>{"alpha","beta","profile.json","manifest.json"},"exact whole-cohort top-level inventory");
 const auto first=verify_manifest(root,document);check(verify_manifest(root,document)==first,"second fresh RO pass exact");check(test_signatures==0,"check never instantiates fixture keys or signatures");manifest.fence();closed.fence();report("check",first,expected,2);
}
}
int main(int argc,char** argv){::umask(0077);try{
 check(std::signal(SIGTERM,interrupt)!=SIG_ERR&&std::signal(SIGINT,interrupt)!=SIG_ERR,"fixture signal handlers");
 check((argc==3&&std::string(argv[1])=="produce")||(argc==4&&std::string(argv[1])=="check"),"usage: physical-cohort-portability produce ABSENT_ROOT | check EXISTING_ROOT MANIFEST_SHA256");
 const fs::path root(argv[2]);check(root.is_absolute()&&root==root.lexically_normal()&&root!=root.root_path()&&root.string().size()<=4096&&root.string().find('\0')==std::string::npos,"absolute normalized dedicated root");
 ScopedChainStoreMemoryPolicy memory({4*1024*1024,2,16*1024*1024,0});
 if(std::string(argv[1])=="produce")produce(root);else inspect(root,argv[3]);return 0;
 }catch(const std::exception& e){std::string why=e.what();if(why.size()>512)why.resize(512);std::cerr<<Json{{"diagnostic","physical-public-cohort-refused-v1"},{"stage",test_stage},{"reason",why},{"newTestOnlySignaturesCreated",test_signatures},{"actualPeakRssKiB",public_peak_rss_kib()},{"automaticRecoveryUsed",false},{"runtimeDutiesAuthorized",false},{"networkTransport",false},{"productionQualified",false}}.dump()<<std::endl;return 1;}}
