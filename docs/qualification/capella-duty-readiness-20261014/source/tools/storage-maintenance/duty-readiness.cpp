// Offline maintenance candidate. ROOT supplies externally pinned configuration
// projections and independent stopped copies. No signing or recovery authority.
#include "nano/chain.hpp"
#include "nano/checkpoint_blob_gc.hpp"
#include "nano/consensus_beacon_journal_auth.hpp"
#include "nano/consensus_capella_reactor.hpp"
#include "nano/maintenance/duty_readiness.hpp"
#include "nano/storage_memory_policy.hpp"
#include "bootstrap-readonly-replica.hpp"
#include "rocksdb-owned-copy.hpp"
#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <functional>
#include <filesystem>
#include <fcntl.h>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace {
using namespace nano;
namespace fs=std::filesystem;
namespace rd=nano::maintenance::readonly_replica_detail;
using nano::maintenance::SealedReadonlyReplica;
volatile std::sig_atomic_t interrupted=0;
void interrupt(int){interrupted=1;}
using Check=std::function<void()>;
void checked(const Check& check){if(check)check();}
void require(bool ok){if(!ok)throw std::invalid_argument("Duty readiness refused");}
void io(bool ok){if(!ok)throw std::runtime_error("Duty readiness I/O refused");}
void shape(const Json& j,std::initializer_list<const char*> fields){
 require(j.is_object()&&j.size()==fields.size());for(auto key:fields)require(j.contains(key));
}
uint64_t natural(const Json& j){require(j.is_string());const auto& text=j.get_ref<const std::string&>();require(text.size()>=3&&text.size()<=18&&text.starts_with("0x"));const auto n=u64(quantity(j));require(quantity(Int(std::to_string(n)))==text);return n;}
uint64_t positive(const Json& j){const auto n=natural(j);require(n);return n;}
Bytes encoded(const Json& j,size_t size){require(j.is_string()&&size<=(SIZE_MAX-2)/2);const auto& text=j.get_ref<const std::string&>();require(text.size()==2+2*size&&text.starts_with("0x"));const auto b=unhex(text);require(b.size()==size&&hex(b)==text);return b;}
std::string digest(std::string_view b){rd::Hash h;h.add(b.data(),b.size());return h.finish();}
std::string sha_text(const Json& j){require(j.is_string());auto s=j.get<std::string>();require(s.size()==64&&s.find_first_not_of("0123456789abcdef")==std::string::npos);return s;}
Json parse(std::string_view text,bool canonical=true,const Check& check={}){
 checked(check);require(!text.empty());std::vector<std::set<std::string>> names;
 auto out=Json::parse(text.begin(),text.end(),[&](int depth,Json::parse_event_t event,Json& item){
  checked(check);require(depth>=0&&depth<=64);if(event==Json::parse_event_t::object_start)names.emplace_back();
  else if(event==Json::parse_event_t::object_end){require(!names.empty());names.pop_back();}
  else if(event==Json::parse_event_t::key){require(!names.empty());require(names.back().insert(item.get<std::string>()).second);}
  return true;
 });checked(check);require(!canonical||out.dump()==text);checked(check);return out;
}
fs::path path(const Json& j){require(j.is_string());const auto s=j.get<std::string>();fs::path p(s);require(!s.empty()&&s.size()<=4096&&s.find('\0')==std::string::npos&&p.is_absolute()&&p==p.lexically_normal()&&p!=p.root_path()&&fs::canonical(p)==p);return p;}
bool same_inode(const struct stat& a,const struct stat& b){return a.st_dev==b.st_dev&&a.st_ino==b.st_ino;}
bool physical_ancestor(const rd::Directory& parent,const rd::Directory& child){for(const auto& row:child.stamps)if(same_inode(parent.stamps.back(),row))return true;return false;}
void separate(const rd::Directory& a,const rd::Directory& b){a.recheck();b.recheck();require(!physical_ancestor(a,b)&&!physical_ancestor(b,a));}
bool same_context(const Phase0ForkSlashingContext& a,const Phase0ForkSlashingContext& b){return a.block_root==b.block_root&&a.genesis_root==b.genesis_root&&a.registry==b.registry&&a.fork.previous_version==b.fork.previous_version&&a.fork.current_version==b.fork.current_version&&a.fork.epoch==b.fork.epoch;}
ForkCheckpointToken token(const Json& j){shape(j,{"sequence","digest"});return {positive(j.at("sequence")),encoded(j.at("digest"),32)};}
Json token_json(const ForkCheckpointToken& t){return {{"sequence",quantity(Int(std::to_string(t.sequence)))},{"digest",hex(t.digest)}};}

// Own the pinned bytes and FD before any caller callback. Parent directories
// and leaf inode/material are rechecked; reading never creates or repairs a file.
struct Input {
 fs::path p;rd::Directory parent;rd::Fd fd;struct stat before{};std::string bytes;
 Input(fs::path at,std::string expected,size_t maximum,const std::function<void()>& check,bool private_file=false)
 :p(std::move(at)),parent(p.parent_path()),fd(::openat(parent.fd(),p.filename().c_str(),O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC)){
  io(::fstat(fd.n,&before)==0);require(S_ISREG(before.st_mode)&&before.st_uid==::geteuid()&&before.st_nlink==1&&(before.st_mode&0022)==0&&before.st_size>0&&uint64_t(before.st_size)<=maximum);
  require(!private_file||(before.st_mode&0777)==0600);checked(check);bytes.resize(size_t(before.st_size));checked(check);size_t at_byte=0;
  while(at_byte<bytes.size()){check();const auto n=::pread(fd.n,bytes.data()+at_byte,std::min<size_t>(65536,bytes.size()-at_byte),off_t(at_byte));if(n<0&&errno==EINTR)continue;io(n>0);at_byte+=size_t(n);}
  recheck(check);require(digest(bytes)==sha_text(Json(expected)));
 }
 void recheck(const std::function<void()>& check)const{
  parent.recheck();struct stat a{},b{};io(::fstat(fd.n,&a)==0&&::fstatat(parent.fd(),p.filename().c_str(),&b,AT_SYMLINK_NOFOLLOW)==0);require(rd::stable(before,a)&&rd::stable(before,b));
  rd::Hash hash;std::array<char,65536> buffer{};size_t count=0;
  while(count<bytes.size()){check();const auto n=::pread(fd.n,buffer.data(),std::min(buffer.size(),bytes.size()-count),off_t(count));if(n<0&&errno==EINTR)continue;io(n>0);require(std::equal(buffer.begin(),buffer.begin()+n,bytes.begin()+count));hash.add(buffer.data(),size_t(n));count+=size_t(n);}
  require(hash.finish()==digest(bytes));parent.recheck();io(::fstat(fd.n,&a)==0&&::fstatat(parent.fd(),p.filename().c_str(),&b,AT_SYMLINK_NOFOLLOW)==0);require(rd::stable(before,a)&&rd::stable(before,b));
 }
};
struct Artifact{fs::path p;std::string sha;size_t bytes;};
Artifact artifact(const Json& j,size_t maximum){shape(j,{"path","sha256","bytes"});require(j.at("bytes").is_number_unsigned());const auto n=j.at("bytes").get<uint64_t>();require(n&&n<=maximum&&n<=SIZE_MAX);return {path(j.at("path")),sha_text(j.at("sha256")),size_t(n)};}
struct Budget{uint64_t files,bytes,duration,rss;};
Budget budget(const Json& j){shape(j,{"inventoryFiles","inventoryBytes","durationMs","maximumRssKiB"});Budget b{positive(j.at("inventoryFiles")),positive(j.at("inventoryBytes")),positive(j.at("durationMs")),positive(j.at("maximumRssKiB"))};require(b.duration<=uint64_t(INT64_MAX)&&b.bytes<=SIZE_MAX&&b.rss<=SIZE_MAX/1024);return b;}
size_t value_limit(const Budget& b){const auto n=std::min<uint64_t>(b.bytes,b.rss*1024/4);require(n>=16384&&n<=SIZE_MAX);return size_t(n);}
uint64_t rss(){struct rusage r{};io(::getrusage(RUSAGE_SELF,&r)==0);require(r.ru_maxrss>=0);
#ifdef __APPLE__
 return uint64_t(r.ru_maxrss)/1024;
#else
 return uint64_t(r.ru_maxrss);
#endif
}
struct Selection{fs::path p;std::string inventory;ForkCheckpointToken current;std::optional<ForkCheckpointToken> floor;};
Selection selection(const Json& j,bool journal){
 if(journal)shape(j,{"path","inventorySha256","token","floor"});else shape(j,{"path","inventorySha256","token"});
 Selection s{path(j.at("path")),sha_text(j.at("inventorySha256")),token(j.at("token")),std::nullopt};
 if(journal&&!j.at("floor").is_null())s.floor=token(j.at("floor"));return s;
}
class Reader {
 Selection selected;const Budget b;Check check;SealedReadonlyReplica replica;std::unique_ptr<rocksdb::DB> db;
 std::optional<std::string> optional_get(const std::string& key,size_t maximum){
  check();require(bool(db)&&maximum);rocksdb::ReadOptions ro;ro.verify_checksums=true;ro.fill_cache=false;rocksdb::PinnableSlice value;
  const auto status=db->Get(ro,db->DefaultColumnFamily(),rocksdb::Slice(key),&value);
  if(status.IsNotFound()){check();return std::nullopt;}io(status.ok());require(value.size()<=maximum&&value.size()<=value_limit(b));
  check();std::string owned(value.data(),value.size());check();return owned;
 }
 std::string get(const std::string& key,size_t maximum){auto value=optional_get(key,maximum);require(value&&!value->empty());return std::move(*value);}
 // ChainStore's established canonical decoder copies values internally. Bound
 // every record on this immutable replica before that decoder is constructed;
 // this is an allocation fence, not an alternative chain authentication rule.
 void canonical_value_fence(){
  rocksdb::ReadOptions ro;ro.verify_checksums=true;ro.fill_cache=false;std::unique_ptr<rocksdb::Iterator> it(db->NewIterator(ro));require(bool(it));uint64_t count=0,total=0;
  for(it->SeekToFirst();it->Valid();it->Next()){
   check();io(it->status().ok());const auto key=it->key(),value=it->value();require(count<b.files&&key.size()<=4096&&value.size()<=value_limit(b));++count;
   require(key.size()<=b.bytes-total);total+=key.size();require(value.size()<=b.bytes-total);total+=value.size();
   const std::string_view name(key.data(),key.size());
   if(name=="meta/identity"||name=="fork-checkpoint/v1/token"||name=="execution-checkpoint/v1"||name=="execution-state-pruning/v1"||name.starts_with("block/")||name.starts_with("state/")||name.starts_with("certificate/"))
    (void)parse(std::string_view(value.data(),value.size()),false,check);
  }io(it->status().ok());check();
 }
public:
 Reader(Selection s,fs::path scratch,Budget bounds,Check callback,Json expected_identity):selected(std::move(s)),b(bounds),check(std::move(callback)),replica(SealedReadonlyReplica::capture(selected.p,scratch,b.files,b.bytes,check)){
  require(checkpoint_blob_gc_copy_inventory(selected.p,b.files,b.bytes,check)==selected.inventory);
  rocksdb::Options options;options.create_if_missing=false;options.paranoid_checks=true;apply_chain_store_memory_policy(options);
  check();db=nano::maintenance::open_read_only_copy(options,replica.read_only_path().string());check();require(parse(get("meta/identity",4096),true,check)==expected_identity);
 }
 ForkCheckpointRecord record(size_t maximum,bool journal){
  auto current=token(parse(get("fork-checkpoint/v1/token",256),true,check));auto snapshot=get("fork-checkpoint/v1/payload",maximum);require(current==selected.current&&hex(current.digest).substr(2)==digest(snapshot));
  if(journal)require(!optional_get("execution-checkpoint/v1",512));
  return {current,std::move(snapshot)};
 }
 std::optional<std::string> blob(const Bytes& id,size_t maximum){check();require(id.size()==32&&maximum);auto value=optional_get("checkpoint-blob/v1/"+hex(id),maximum);if(!value)return std::nullopt;require(!value->empty()&&value->size()==maximum&&digest(*value)==hex(id).substr(2));check();return value;}
 CapellaForkStore canonical_store(const ForkCheckpointRecord& current,const Json& head,const Genesis& g,const CapellaForkBlockRules& rules,const CapellaStoreSnapshotLimits& limits){
  const auto linked=parse(get("execution-checkpoint/v1",512),true,check);shape(linked,{"digest","tip"});require(linked.at("digest")==hex(current.token.digest)&&linked.at("tip")==head.at("hash"));
  canonical_value_fence();db.reset();check();replica.recheck();CapellaForkStore owned;
  {
   // Canonical read-only restoration, on the independent sealed replica only.
   // The RO store outlives the reactor. No owner, witness, recover or signer.
   ChainStore canonical(replica.read_only_path(),ChainStore::OpenMode::ReadOnly);check();
   canonical.require_existing_chain_binding(g.chain_id,g.block.header.hash(),"beacon",g.staking_contract);check();
   const auto genesis=canonical.block_by_number(0);check();require(genesis&&genesis->header.hash()==g.block.header.hash());
   const auto record=canonical.fork_checkpoint(g.chain_id,g.block.header.hash(),limits.envelope_bytes,selected.current);check();require(record&&record->token==current.token&&record->snapshot==current.snapshot);
   const auto latest=canonical.latest();check();require(bool(latest));
   require(quantity(latest->header.number)==head.at("number").get<std::string>()&&hex(latest->header.hash())==head.at("hash").get<std::string>()&&hex(latest->header.state_root)==head.at("stateRoot").get<std::string>());
   const auto state=canonical.state_at(latest->header.number);check();require(state&&hex(state->root())==head.at("stateRoot").get<std::string>());
   {
    CapellaDurableReactor restored(canonical,g.chain_id,g.block.header.hash(),rules,limits,selected.current,check);const auto published=restored.current();check();require(published.state&&published.token==selected.current);owned=*published.state;
    require(owned.blocks.size()<=limits.blocks);size_t total=0;
    for(auto& [_,entry]:owned.blocks){check();const auto held=entry.lease(check);require(held&&held->state.ssz.size()<=limits.state_bytes&&held->state.ssz.size()<=limits.total_state_bytes-total);total+=held->state.ssz.size();entry=CapellaForkRecord(CapellaForkBlock(*held));check();}
   }
  }replica.recheck();check();return owned;
 }
 void close(){db.reset();replica.recheck();require(checkpoint_blob_gc_copy_inventory(selected.p,b.files,b.bytes,check)==selected.inventory);replica.close();}
};
void boolean(const Json& value,bool wanted){require(value.is_boolean()&&value.get<bool>()==wanted);}
ConsensusRoot root_value(const Json& value){const auto bytes=encoded(value,32);ConsensusRoot out{};std::copy(bytes.begin(),bytes.end(),out.begin());return out;}
BeaconJournalReservations reservations(const Json& j){
 shape(j,{"proposal","attestation","retired"});require(j.at("retired").is_boolean());BeaconJournalReservations out;out.retired=j.at("retired").get<bool>();
 if(!j.at("proposal").is_null()){const auto& p=j.at("proposal");shape(p,{"slot","root"});out.proposal=BeaconJournalProposal{natural(p.at("slot")),std::nullopt};if(!p.at("root").is_null())out.proposal->root=root_value(p.at("root"));}
 if(!j.at("attestation").is_null()){const auto& a=j.at("attestation");shape(a,{"source","target","root"});const auto source=natural(a.at("source")),target=natural(a.at("target"));require(source<=target);out.attestation=BeaconJournalAttestation{source,target,std::nullopt};if(!a.at("root").is_null())out.attestation->root=root_value(a.at("root"));}
 return out;
}
nano::maintenance::DutyReadinessBounds preparation_bounds(const Json& j,const Budget& b){
 shape(j,{"maximumGapSlots","maximumChunkSlots","maximumChunks","maximumRetargets","maximumElapsedMs","maximumRssKiB","maximumObservationAgeMs","maximumRegistryEntries"});
 nano::maintenance::DutyReadinessBounds out{positive(j.at("maximumGapSlots")),positive(j.at("maximumChunkSlots")),positive(j.at("maximumChunks")),natural(j.at("maximumRetargets")),positive(j.at("maximumElapsedMs")),positive(j.at("maximumRssKiB")),positive(j.at("maximumObservationAgeMs")),positive(j.at("maximumRegistryEntries"))};require(out.maximum_elapsed_ms<=b.duration&&out.maximum_rss_kib<=b.rss);return out;
}
uint64_t utc_now(){const auto value=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();require(value>=0);return uint64_t(value);}
// The parent is held and physically fenced before source inspection. A report
// is first written to an owned private temporary file. No preexisting path is
// overwritten or deleted; a failed final fence removes only our held inode.
class Publication {
 const rd::Directory& parent;std::string leaf,temporary,expected_sha;std::optional<rd::Fd> fd;struct stat sealed{};bool linked=false,accepted=false;
 bool unlink_owned(const std::string& name){
  struct stat held{},entry{};io(::fstat(fd->n,&held)==0);
  if(::fstatat(parent.fd(),name.c_str(),&entry,AT_SYMLINK_NOFOLLOW)<0){if(errno==ENOENT)return false;io(false);}
  require(same_inode(held,entry)&&S_ISREG(entry.st_mode)&&entry.st_uid==::geteuid()&&(entry.st_mode&0777)==0600);io(::unlinkat(parent.fd(),name.c_str(),0)==0);return true;
 }
public:
 Publication(const rd::Directory& owned_parent,std::string name,const Check& check):parent(owned_parent),leaf(std::move(name)){
  require(!leaf.empty()&&leaf!="."&&leaf!=".."&&fs::path(leaf).filename()==leaf);parent.recheck();
  struct stat absent{};require(::fstatat(parent.fd(),leaf.c_str(),&absent,AT_SYMLINK_NOFOLLOW)<0&&errno==ENOENT);
  check();
 }
 ~Publication(){if(!accepted&&fd){try{if(linked)unlink_owned(leaf);unlink_owned(temporary);rd::sync(parent.fd());}catch(...){}}}
 void write(const Json& document,size_t maximum,const Check& check){
  check();const auto bytes=document.dump();check();require(!bytes.empty()&&bytes.size()<=maximum);expected_sha=digest(bytes);check();
  for(unsigned attempt=0;attempt<32;++attempt){check();temporary=".duty-readiness-"+std::to_string(::getpid())+"-"+std::to_string(attempt)+".tmp";require(temporary!=leaf);
   const auto raw=::openat(parent.fd(),temporary.c_str(),O_RDWR|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);
   if(raw>=0){fd.emplace(raw);break;}if(errno!=EEXIST)io(false);
  }require(bool(fd));struct stat created{};io(::fstat(fd->n,&created)==0);require(S_ISREG(created.st_mode)&&created.st_uid==::geteuid()&&created.st_nlink==1&&(created.st_mode&0777)==0600);parent.recheck();check();
  size_t count=0;
  while(count<bytes.size()){check();const auto n=::write(fd->n,bytes.data()+count,std::min<size_t>(65536,bytes.size()-count));if(n<0&&errno==EINTR)continue;io(n>0);count+=size_t(n);}
  check();rd::sync(fd->n);check();io(::fstat(fd->n,&sealed)==0);require(sealed.st_nlink==1&&sealed.st_size>=0&&uint64_t(sealed.st_size)==bytes.size());
  rd::Hash hash;std::array<char,65536> buffer{};count=0;
  while(count<bytes.size()){check();const auto n=::pread(fd->n,buffer.data(),std::min(buffer.size(),bytes.size()-count),off_t(count));if(n<0&&errno==EINTR)continue;io(n>0);require(std::equal(buffer.begin(),buffer.begin()+n,bytes.begin()+count));hash.add(buffer.data(),size_t(n));count+=size_t(n);}
  require(hash.finish()==digest(bytes));struct stat after{},entry{};io(::fstat(fd->n,&after)==0&&::fstatat(parent.fd(),temporary.c_str(),&entry,AT_SYMLINK_NOFOLLOW)==0);require(rd::stable(sealed,after)&&rd::stable(sealed,entry));parent.recheck();check();
 }
 void link(const Check& check){
  parent.recheck();check();io(::linkat(parent.fd(),temporary.c_str(),parent.fd(),leaf.c_str(),0)==0);linked=true;
  require(unlink_owned(temporary));check();rd::sync(parent.fd());check();parent.recheck();io(::fstat(fd->n,&sealed)==0);
  require(S_ISREG(sealed.st_mode)&&sealed.st_uid==::geteuid()&&sealed.st_nlink==1&&(sealed.st_mode&0777)==0600);fence(check);
 }
 void fence(const Check& check)const{
  parent.recheck();check();struct stat held{},entry{};io(::fstat(fd->n,&held)==0&&::fstatat(parent.fd(),leaf.c_str(),&entry,AT_SYMLINK_NOFOLLOW)==0);require(rd::stable(sealed,held)&&rd::stable(sealed,entry));
  rd::Hash hash;std::array<char,65536> buffer{};size_t count=0;const auto bytes=size_t(sealed.st_size);
  while(count<bytes){check();const auto n=::pread(fd->n,buffer.data(),std::min(buffer.size(),bytes-count),off_t(count));if(n<0&&errno==EINTR)continue;io(n>0);hash.add(buffer.data(),size_t(n));count+=size_t(n);}
  require(hash.finish()==expected_sha);parent.recheck();io(::fstat(fd->n,&held)==0&&::fstatat(parent.fd(),leaf.c_str(),&entry,AT_SYMLINK_NOFOLLOW)==0);require(rd::stable(sealed,held)&&rd::stable(sealed,entry));check();
 }
 void accept(){accepted=true;}
};
}
int main(int argc,char** argv){
 using namespace nano;using namespace nano::maintenance;const auto started=std::chrono::steady_clock::now();std::string stage="request",failure="validation_refused";uint64_t configured=0;bool completed=false,contexts=false;Json evaluation;
 try{
  require(std::signal(SIGTERM,interrupt)!=SIG_ERR&&std::signal(SIGINT,interrupt)!=SIG_ERR);
  const Check precheck=[&]{if(interrupted)throw DutyReadinessRefused("cancelled");if(std::chrono::steady_clock::now()-started>=std::chrono::seconds(30))throw DutyReadinessRefused("steady_deadline_exceeded");if(rss()>131072)throw DutyReadinessRefused("rss_limit_exceeded");};precheck();
  require(argc==6&&std::string(argv[1])=="--observe");const fs::path request_path(argv[2]),output(argv[4]),scratch(argv[5]);
  require(request_path.is_absolute()&&output.is_absolute()&&scratch.is_absolute()&&output==output.lexically_normal()&&output.string().size()<=4096&&output.string().find('\0')==std::string::npos&&!output.filename().empty());
  rd::Directory scratch_owner(scratch),output_parent(output.parent_path());separate(scratch_owner,output_parent);
  Input request(request_path,sha_text(Json(argv[3])),1024*1024,precheck,true);separate(scratch_owner,request.parent);separate(output_parent,request.parent);const auto j=parse(request.bytes,true,precheck);
  shape(j,{"format","genesis","profile","limits","core","configurationProjection","journalEvidence","budget","preparation","memoryPolicy"});require(j.at("format")=="nano-capella-duty-readiness-request-v1");const auto b=budget(j.at("budget"));require(b.rss<=131072&&b.duration<=180000);const auto record_bound=value_limit(b);const auto bounds=preparation_bounds(j.at("preparation"),b);
  const Check check=[&]{const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-started).count();if(interrupted)throw DutyReadinessRefused("cancelled");if(elapsed<0||uint64_t(elapsed)>=b.duration)throw DutyReadinessRefused("steady_deadline_exceeded");if(rss()>b.rss)throw DutyReadinessRefused("rss_limit_exceeded");};check();
  const auto scratch_empty=[&]{check();scratch_owner.recheck();const auto entries=rd::capture(scratch_owner.fd(),b.files,b.bytes,check);require(entries.size()==1&&entries.front().name.empty()&&S_ISDIR(entries.front().stamp.st_mode));scratch_owner.recheck();check();};scratch_empty();
  std::vector<std::unique_ptr<Input>> inputs;
  const auto load=[&](const Json& descriptor,size_t maximum,bool private_file)->Input&{const auto a=artifact(descriptor,std::min(maximum,record_bound));check();for(const auto& prior:inputs)require(prior->p!=a.p);auto owned=std::make_unique<Input>(a.p,a.sha,a.bytes,check,private_file);require(owned->bytes.size()==a.bytes);separate(scratch_owner,owned->parent);separate(output_parent,owned->parent);inputs.push_back(std::move(owned));return *inputs.back();};
  Input& gi=load(j.at("genesis"),16*1024*1024,false);Input& pi=load(j.at("profile"),1024*1024,false);Input& ci=load(j.at("configurationProjection"),1024*1024,true);Input& mi=load(j.at("memoryPolicy"),4096,true);
  const auto g=Genesis::from_json(parse(gi.bytes,false,check));check();const auto rules=load_capella_fork_profile(pi.bytes,unhex("0x"+sha_text(j.at("profile").at("sha256"))),pi.bytes.size());check();require(g.consensus=="beacon"&&g.evm_revision==EvmRevision::Shanghai&&rules.transition.chain_id==g.chain_id&&rules.transition.replay_policy==ReplayPolicy::RequireChainId);
  require(j.at("limits").is_array()&&j.at("limits").size()==8);for(const auto& value:j.at("limits")){check();positive(value);}const auto limits=checkpoint_blob_gc_snapshot_limits(j.at("limits"));
  require(limits.envelope_bytes<=record_bound&&limits.envelope_bytes<=(SIZE_MAX-2048)/8&&limits.state_bytes<=record_bound&&limits.total_state_bytes<=b.rss*1024/2&&limits.blocks<=b.files&&limits.messages<=b.files&&limits.equivocations<=b.files);
  require(bounds.maximum_registry_entries<=SIZE_MAX&&bounds.maximum_registry_entries<=rules.transition.consensus.state.validator_registry_limit&&bounds.maximum_registry_entries<=record_bound/(sizeof(ConsensusSlashingValidator)+4*sizeof(void*)));
  const auto mp=parse(mi.bytes,true,check);shape(mp,{"format","writeBufferBytes","maximumWriteBuffers","sharedMemtableBytes","sharedBlockCacheBytes"});require(mp.at("format")=="nano-chain-store-memory-policy-v1");const auto write=positive(mp.at("writeBufferBytes")),buffers=positive(mp.at("maximumWriteBuffers")),memtable=positive(mp.at("sharedMemtableBytes")),cache=natural(mp.at("sharedBlockCacheBytes"));require(write<=SIZE_MAX&&memtable<=SIZE_MAX&&cache<=SIZE_MAX&&buffers<=UINT32_MAX&&memtable<=b.rss*1024/2&&cache<=b.rss*1024/2-memtable&&write<=memtable&&buffers<=memtable/write);
  const ChainStoreMemoryPolicy policy{size_t(write),uint32_t(buffers),size_t(memtable),size_t(cache)};ScopedChainStoreMemoryPolicy memory(policy);check();
  const auto& core=j.at("core");shape(core,{"primary","witness","expected"});const auto cp=selection(core.at("primary"),false),cw=selection(core.at("witness"),false);const auto& expected=core.at("expected");shape(expected,{"headRoot","headStateRoot","storageAnchor","identity","executionHead"});root_value(expected.at("headRoot"));root_value(expected.at("headStateRoot"));root_value(expected.at("storageAnchor"));
  shape(expected.at("identity"),{"profile","networkAnchor","genesisValidatorsRoot","genesisTimeMs"});require(expected.at("identity").at("profile")==capella_fork_profile(rules));encoded(expected.at("identity").at("networkAnchor"),32);encoded(expected.at("identity").at("genesisValidatorsRoot"),32);natural(expected.at("identity").at("genesisTimeMs"));shape(expected.at("executionHead"),{"number","hash","stateRoot"});natural(expected.at("executionHead").at("number"));encoded(expected.at("executionHead").at("hash"),32);encoded(expected.at("executionHead").at("stateRoot"),32);
  stage="journal-evidence";const auto& evidence=j.at("journalEvidence");shape(evidence,{"request","report","binding"});Input& qi=load(evidence.at("request"),1024*1024,true);Input& ri=load(evidence.at("report"),1024*1024,true);Input& bi=load(evidence.at("binding"),16384,true);
  const auto q=parse(qi.bytes,true,check),report=parse(ri.bytes,true,check),binding=parse(bi.bytes,true,check),projection=parse(ci.bytes,true,check);shape(binding,{"format","request","report","originalBefore","hostOnly","fullResumeAuthorized"});require(binding.at("format")=="nano-root-configured-journal-complete-report-binding-v1"&&binding.at("request")==evidence.at("request")&&binding.at("report")==evidence.at("report"));boolean(binding.at("hostOnly"),true);boolean(binding.at("fullResumeAuthorized"),false);
  shape(q,{"format","genesis","profile","configurationProjection","limits","budget","maximumRegistryEntries","core","roles"});require(q.at("format")=="nano-configured-journal-readonly-observer-v1"&&q.at("configurationProjection")==j.at("configurationProjection"));shape(projection,{"format","originalConfigurationSha256","core","roles"});require(projection.at("format")=="nano-root-configured-journal-path-key-projection-v1"&&projection.at("core")==q.at("core")&&projection.at("roles")==q.at("roles"));sha_text(projection.at("originalConfigurationSha256"));
  for(const char* name:{"genesis","profile"}){const auto old=artifact(q.at(name),record_bound),now=artifact(j.at(name),record_bound);require(old.sha==now.sha&&old.bytes==now.bytes);if(old.p!=now.p)(void)load(q.at(name),record_bound,false);}
  Input& oi=load(binding.at("originalBefore"),16384,true);const auto original=parse(oi.bytes,true,check);shape(original,{"format","requestSha256","primary","witness","current"});require(original.at("format")=="nano-root-duty-journal-original-before-v1"&&original.at("requestSha256")==evidence.at("request").at("sha256"));
  shape(q.at("core"),{"primary","witness","expected"});const auto& old_expected=q.at("core").at("expected");shape(old_expected,{"headRoot","identity","executionHead"});require(old_expected.at("headRoot")==expected.at("headRoot")&&old_expected.at("identity")==expected.at("identity")&&old_expected.at("executionHead")==expected.at("executionHead"));
  const auto old_primary=selection(q.at("core").at("primary"),false),old_witness=selection(q.at("core").at("witness"),false);require(token(original.at("primary"))==old_primary.current&&token(original.at("witness"))==old_witness.current&&cp.current.sequence>=old_primary.current.sequence&&cw.current.sequence>=old_witness.current.sequence);const auto& before=original.at("current");shape(before,{"publishedClockMs","genesisTimeMs","genesisValidatorsRoot","networkAnchor","storageAnchor","headRoot","headStateRoot","executionBlockHash","justified","finalized"});natural(before.at("publishedClockMs"));
  require(before.at("genesisTimeMs")==expected.at("identity").at("genesisTimeMs")&&before.at("genesisValidatorsRoot")==expected.at("identity").at("genesisValidatorsRoot")&&before.at("networkAnchor")==expected.at("identity").at("networkAnchor")&&before.at("storageAnchor")==expected.at("storageAnchor")&&before.at("headRoot")==expected.at("headRoot")&&before.at("headStateRoot")==expected.at("headStateRoot")&&before.at("executionBlockHash")==expected.at("executionHead").at("hash"));for(const char* name:{"justified","finalized"}){shape(before.at(name),{"epoch","root"});natural(before.at(name).at("epoch"));encoded(before.at(name).at("root"),32);}
  shape(report,{"format","requestSha256","rows","twoIndependentColdReads","currentHeadAuthenticated","floorConstraintsObserved","historicalDigestAncestryVerified","deliveredSignatureHistoryVerified","executionAccountNoncesVerified","runtimeDutiesAuthorized","newSignaturesCreated","productionQualified"});require(report.at("format")=="nano-configured-journal-observation-private-v1"&&report.at("requestSha256")==evidence.at("request").at("sha256"));for(const char* flag:{"twoIndependentColdReads","currentHeadAuthenticated","floorConstraintsObserved"})boolean(report.at(flag),true);for(const char* flag:{"historicalDigestAncestryVerified","deliveredSignatureHistoryVerified","executionAccountNoncesVerified","runtimeDutiesAuthorized","productionQualified"})boolean(report.at(flag),false);require(report.at("newSignaturesCreated").is_number_unsigned()&&report.at("newSignaturesCreated").get<uint64_t>()==0);
  require(q.at("roles").is_array()&&!q.at("roles").empty()&&report.at("rows").is_array()&&!report.at("rows").empty()&&report.at("rows").size()<=bounds.maximum_registry_entries&&report.at("rows").size()<=b.files/2);std::vector<Selection> sources{cp,cw};std::vector<DutyReadinessKey> keys;std::set<std::string> roles,public_keys;size_t row_number=0;
  for(const auto& role:q.at("roles")){
   check();shape(role,{"role","keys"});require(role.at("role").is_string()&&!role.at("role").get_ref<const std::string&>().empty()&&role.at("role").get_ref<const std::string&>().size()<=1024&&roles.insert(role.at("role").get<std::string>()).second&&role.at("keys").is_array()&&!role.at("keys").empty());
   for(const auto& k:role.at("keys")){
    check();shape(k,{"publicKey","index","primary","witness"});require(row_number<report.at("rows").size());const auto& row=report.at("rows").at(row_number++);shape(row,{"role","publicKey","configuredIndex","registeredIndex","primary","witness","disposition","reservations","candidateReservations","primaryFloorSupplied","witnessFloorSupplied"});require(row.at("role")==role.at("role")&&row.at("publicKey")==k.at("publicKey")&&row.at("configuredIndex")==k.at("index"));const auto pub=encoded(k.at("publicKey"),48);require(public_keys.insert(hex(pub)).second);std::optional<uint64_t> index;if(!k.at("index").is_null())index=natural(k.at("index"));if(!row.at("registeredIndex").is_null())natural(row.at("registeredIndex"));
    const auto jp=selection(k.at("primary"),true),jw=selection(k.at("witness"),true);require(jp.floor&&jw.floor);require(token(row.at("primary"))==jp.current&&token(row.at("witness"))==jw.current);boolean(row.at("primaryFloorSupplied"),bool(jp.floor));boolean(row.at("witnessFloorSupplied"),bool(jw.floor));for(const auto* s:{&jp,&jw})if(s->floor)require(s->current.sequence>=s->floor->sequence&&(s->current.sequence!=s->floor->sequence||s->current.digest==s->floor->digest));
    require(row.at("disposition")=="completed"||row.at("disposition")=="pending");auto primary=reservations(row.at("reservations")),candidate=reservations(row.at("candidateReservations"));const bool pending=row.at("disposition")=="pending";if(!pending)require(row.at("reservations")==row.at("candidateReservations"));keys.push_back({pub,index,primary.retired||candidate.retired,pending,std::move(primary),std::move(candidate)});sources.push_back(jp);sources.push_back(jw);check();
   }
  }require(row_number==report.at("rows").size()&&!keys.empty());configured=keys.size();
  stage="source-inventories";std::vector<std::unique_ptr<rd::Directory>> owners;
  for(size_t a=0;a<sources.size();++a){check();owners.push_back(std::make_unique<rd::Directory>(sources[a].p));const auto& owner=*owners.back();separate(owner,scratch_owner);separate(owner,output_parent);require(!physical_ancestor(owner,request.parent));for(const auto& input:inputs){check();require(!physical_ancestor(owner,input->parent));}for(size_t z=0;z<a;++z)separate(owner,*owners[z]);}
  const auto source_fence=[&]{for(size_t n=0;n<sources.size();++n){check();owners[n]->recheck();require(checkpoint_blob_gc_copy_inventory(sources[n].p,b.files,b.bytes,check)==sources[n].inventory);owners[n]->recheck();}check();};source_fence();
  stage="current-primary";Json identity={{"chainId",quantity(g.chain_id)},{"genesisHash",hex(g.block.header.hash())},{"consensus","beacon"}};if(!g.staking_contract.empty())identity["stakingContract"]=address(g.staking_contract);Json witnessed_identity={{"chainId",quantity(g.chain_id)},{"genesisHash",hex(g.block.header.hash())},{"consensus","nano-capella-recovery-v1"}};
  std::optional<CapellaForkStore> store;
  {
   Reader primary(cp,scratch,b,check,identity),witness(cw,scratch,b,check,witnessed_identity);const auto p=primary.record(limits.envelope_bytes,false),w=witness.record(2048+8*limits.envelope_bytes,true);const auto fetch_primary=[&](const Bytes& id,size_t n){return primary.blob(id,n);};const auto pclosure=authenticate_capella_checkpoint_blob_closure(p.snapshot,p.token.digest,rules,limits,fetch_primary,{},check);const auto description=pclosure.description();check();const auto context=pclosure.head_context();check();require(context.registry.validators.size()<=bounds.maximum_registry_entries&&hex(Bytes(context.block_root.begin(),context.block_root.end()))==expected.at("headRoot").get<std::string>()&&description.at("identity")==expected.at("identity")&&description.at("executionHead")==expected.at("executionHead"));
   // The report's indices must also match this genuinely authenticated current
   // head registry; a projection or path suffix is not registry authority.
   for(size_t n=0;n<keys.size();++n){check();const auto& row=report.at("rows").at(n);std::optional<uint64_t> index;if(!row.at("registeredIndex").is_null())index=natural(row.at("registeredIndex"));const auto found=std::find_if(context.registry.validators.begin(),context.registry.validators.end(),[&](const auto& entry){check();return Bytes(entry.second.public_key.begin(),entry.second.public_key.end())==keys[n].public_key;});require((found==context.registry.validators.end()&&!index)||(found!=context.registry.validators.end()&&index&&found->first==*index));require(!keys[n].configured_index||(index&&*index==*keys[n].configured_index));}
   stage="current-witness";const auto intent=parse(w.snapshot,true,check);shape(intent,{"format","previous","next","snapshot","archive"});require(intent.at("format")=="nano-capella-recovery-v1"||intent.at("format")=="nano-capella-bootstrap-archive-recovery-v1");const auto next=token(intent.at("next"));require(next==p.token&&intent.at("snapshot").is_string()&&intent.at("snapshot").get_ref<const std::string&>()==p.snapshot);if(intent.at("previous").is_null())require(next.sequence==1);else{const auto previous=token(intent.at("previous"));require(previous.sequence<UINT64_MAX&&previous.sequence+1==next.sequence);}
   require(intent.at("archive").is_array()&&intent.at("archive").size()<=1);std::vector<CheckpointArchiveEntry> archive;
   for(const auto& row:intent.at("archive")){check();require(row.is_array()&&row.size()==3&&row.at(2).is_string());const auto& wire=row.at(2).get_ref<const std::string&>();require(limits.envelope_bytes<=(SIZE_MAX-2)/2&&wire.size()>2&&wire.size()<=2+2*limits.envelope_bytes&&wire.size()%2==0);auto bytes=unhex(wire);require(!bytes.empty()&&bytes.size()<=limits.envelope_bytes&&hex(bytes)==wire);archive.push_back({encoded(row.at(0),32),natural(row.at(1)),std::move(bytes)});}
   const auto fetch_witness=[&](const Bytes& id,size_t n){return witness.blob(id,n);};const auto wclosure=authenticate_capella_checkpoint_blob_closure(intent.at("snapshot").get_ref<const std::string&>(),next.digest,rules,limits,fetch_witness,archive,check);const auto wdescription=wclosure.description();check();require(wdescription.at("identity")==description.at("identity")&&wdescription.at("executionHead")==description.at("executionHead")&&same_context(wclosure.head_context(),context));
   stage="canonical-restoration";store.emplace(primary.canonical_store(p,description.at("executionHead"),g,rules,limits));const auto head=capella_fork_head(*store,rules,check);require(head->header.state_root==root_value(expected.at("headStateRoot"))&&store->timed.view.anchor==root_value(expected.at("storageAnchor"))&&head->header==store->blocks.at(context.block_root).header()&&consensus_beacon_header_root(head->header)==context.block_root&&store->timed.clock.time_ms>=natural(before.at("publishedClockMs")));primary.close();witness.close();scratch_empty();check();
  }
  stage="duty-preparation";const auto current_genesis_ms=store->timed.clock.genesis_time_ms;const auto observation_started=utc_now();require(observation_started>=current_genesis_ms&&observation_started>=store->timed.clock.time_ms);
  try{evaluation=evaluate_duty_readiness(std::move(*store),rules,limits,keys,bounds,utc_now,check,rss);}
  catch(const EmptyAltairSyncActiveSet&){evaluation={{"format","nano-capella-duty-readiness-evaluation-v1"},{"result","UNAVAILABLE_READONLY_ONLY"},{"reason","empty_sync_active_set"},{"asOfUtcMs",observation_started},{"slot",(observation_started-current_genesis_ms)/rules.slot_duration_ms},{"probeCompleted",true},{"configuredKeys",configured},{"headProjectionReady",false},{"targetEpochReady",false},{"blockStateReady",false},{"syncCommitteeReady",false},{"completeDutyContexts",false},{"normalNonassignmentPossible",true},{"keys",Json::array()},{"resumeAuthorized",false},{"headProjectionPersisted",false},{"unsignedBodyConstructionQualified",false},{"newSignatures",0},{"runtimeStarted",false},{"productionQualified",false}};check();}
  completed=true;contexts=evaluation.at("completeDutyContexts").get<bool>();uint64_t attestations=0,proposals=0,sync=0,nonassigned=0;for(const auto& key:evaluation.at("keys")){check();attestations+=key.at("attestationEligible").get<bool>();proposals+=key.at("proposalPotentiallyEligible").get<bool>();sync+=key.at("syncPotentiallyEligible").get<bool>();nonassigned+=key.at("classification")=="nonassigned";}
  const auto observation_fence=[&]{check();const auto fresh=utc_now(),asof=evaluation.at("asOfUtcMs").get<uint64_t>();if(fresh<asof||fresh-asof>bounds.maximum_observation_age_ms||fresh<current_genesis_ms||(fresh-current_genesis_ms)/rules.slot_duration_ms!=evaluation.at("slot").get<uint64_t>())throw DutyReadinessRefused("observation_expired");};
  const auto final_fence=[&]{observation_fence();output_parent.recheck();request.recheck(check);for(const auto& input:inputs){check();input->recheck(check);}source_fence();scratch_empty();observation_fence();};stage="closed-input-readback";final_fence();
  const Json document={{"format","nano-capella-duty-readiness-private-v1"},{"requestSha256",argv[3]},{"currentPrimary",token_json(cp.current)},{"currentWitness",token_json(cw.current)},{"expected",expected},{"evaluation",evaluation},{"primaryAndWitnessAuthenticated",true},{"canonicalExecutionCouplingVerified",true},{"journalReportReused",true},{"journalReportSha256",evidence.at("report").at("sha256")},{"journalLeafInventoriesUnchanged",true},{"journalSemanticReportExternallySupplied",true},{"newJournalDatabaseReads",0},{"historicalDigestAncestryVerified",false},{"deliveredSignatureHistoryVerified",false},{"executionAccountNoncesVerified",false},{"runtimeDutiesAuthorized",false},{"newSignaturesCreated",0},{"productionQualified",false}};
  stage="publication";Publication publication(output_parent,output.filename().string(),check);publication.write(document,record_bound,check);final_fence();publication.link(check);final_fence();publication.fence(check);
  const auto typed=Json{{"format","nano-capella-duty-readiness-typed-v1"},{"status","READONLY_DUTY_PROBE_COMPLETED"},{"probeCompleted",true},{"completeDutyContexts",contexts},{"configuredKeys",configured},{"keysEvaluated",evaluation.at("keys").size()},{"normalNonassignedKeys",nonassigned},{"eligibleAttestations",attestations},{"potentialProposalAssignments",proposals},{"potentialSyncAssignments",sync},{"maximumRssKiB",rss()},{"newSignaturesCreated",0},{"keysLoaded",false},{"runtimeStarted",false},{"runtimeDutiesAuthorized",false},{"resumeAuthorized",false},{"productionQualified",false}}.dump();check();publication.fence(check);observation_fence();publication.accept();std::cout<<typed<<std::endl;return 0;
 }catch(const DutyReadinessRefused& error){const std::string reason=error.what();const std::set<std::string> fixed={"missing_observation_callbacks","invalid_preparation_bounds","chunk_exceeds_actual_profile","epoch_profile_mismatch","selection_cardinality","cold_record_requires_owned_materialization","owned_state_byte_limit","steady_deadline_exceeded","rss_limit_exceeded","cancelled","total_slot_work_exceeded","total_chunk_work_exceeded","configured_key_invalid_or_duplicate","utc_before_current_or_genesis","invalid_current_slot","tick_target_overflow","tick_target_missing","tick_target_future","ephemeral_tick_changed_head","invalid_target_ancestry","utc_regressed","utc_before_genesis","invalid_observation_slot","observation_expired","retarget_overflow","retarget_refused","projected_registry_cardinality","sync_offset_overflow","sync_committee_bounds","duplicate_registry_key","configured_index_mismatch"};failure=fixed.contains(reason)?reason:"evaluation_refused";}catch(...){failure="validation_or_io_refused";}
 std::cout<<Json{{"format","nano-capella-duty-readiness-typed-v1"},{"status","REFUSED_NO_AUTOMATIC_RETRY"},{"refusedStage",stage},{"reason",failure},{"probeCompleted",false},{"preparationCompletedBeforeRefusal",completed},{"configuredKeys",configured},{"maximumRssKiB",rss()},{"newSignaturesCreated",0},{"keysLoaded",false},{"runtimeStarted",false},{"runtimeDutiesAuthorized",false},{"resumeAuthorized",false},{"productionQualified",false}}.dump()<<std::endl;return 1;
}
