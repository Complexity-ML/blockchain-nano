// Offline maintenance candidate. ROOT supplies externally pinned configuration
// projections and independent stopped copies. No signing or recovery authority.
#include "nano/chain.hpp"
#include "nano/checkpoint_blob_gc.hpp"
#include "nano/consensus_beacon_journal_auth.hpp"
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
void require(bool ok){if(!ok)throw std::invalid_argument("Journal observation refused");}
void io(bool ok){if(!ok)throw std::runtime_error("Journal observation I/O refused");}
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
 void coupled(const ForkCheckpointRecord& current,const Json& head,const Genesis& g,size_t maximum){
  const auto linked=parse(get("execution-checkpoint/v1",512),true,check);shape(linked,{"digest","tip"});require(linked.at("digest")==hex(current.token.digest)&&linked.at("tip")==head.at("hash"));
  canonical_value_fence();db.reset();check();replica.recheck();
  // The only ChainStore constructed by this tool is ReadOnly on the sealed
  // independent replica. Never bind, initialize, recover, commit or sign.
  {
   ChainStore canonical(replica.read_only_path(),ChainStore::OpenMode::ReadOnly);check();
   canonical.require_existing_chain_binding(g.chain_id,g.block.header.hash(),"beacon",g.staking_contract);check();
   const auto genesis=canonical.block_by_number(0);check();require(genesis&&genesis->header.hash()==g.block.header.hash());
   const auto record=canonical.fork_checkpoint(g.chain_id,g.block.header.hash(),maximum,selected.current);check();
   require(record&&record->token==current.token&&record->snapshot==current.snapshot);
   const auto latest=canonical.latest();check();require(bool(latest));
   require(quantity(latest->header.number)==head.at("number").get<std::string>()&&hex(latest->header.hash())==head.at("hash").get<std::string>()&&hex(latest->header.state_root)==head.at("stateRoot").get<std::string>());
   const auto state=canonical.state_at(latest->header.number);check();require(state&&hex(state->root())==head.at("stateRoot").get<std::string>());check();
  }replica.recheck();check();
 }
 void close(){db.reset();replica.recheck();require(checkpoint_blob_gc_copy_inventory(selected.p,b.files,b.bytes,check)==selected.inventory);replica.close();}
};
Json reservation(const BeaconJournalReservations& r){
 const auto root=[](const std::optional<ConsensusRoot>& x)->Json{return x?Json(hex(Bytes(x->begin(),x->end()))):Json(nullptr);};
 Json proposal=nullptr,attestation=nullptr;
 if(r.proposal)proposal={{"slot",quantity(Int(std::to_string(r.proposal->slot)))},{"root",root(r.proposal->root)}};
 if(r.attestation)attestation={{"source",quantity(Int(std::to_string(r.attestation->source)))},{"target",quantity(Int(std::to_string(r.attestation->target)))},{"root",root(r.attestation->root)}};
 return {{"proposal",proposal},{"attestation",attestation},{"retired",r.retired}};
}
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
  for(unsigned attempt=0;attempt<32;++attempt){check();temporary=".journal-observation-"+std::to_string(::getpid())+"-"+std::to_string(attempt)+".tmp";require(temporary!=leaf);
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
 using namespace nano;const auto started=std::chrono::steady_clock::now();uint64_t pairs=0;std::string stage="request";const char* request_step="arguments";
 try{
  require(std::signal(SIGTERM,interrupt)!=SIG_ERR&&std::signal(SIGINT,interrupt)!=SIG_ERR);
  const Check precheck=[&]{require(!interrupted&&std::chrono::steady_clock::now()-started<std::chrono::seconds(30)&&rss()<=131072);};precheck();
  require(argc==6&&std::string(argv[1])=="--observe");const fs::path request_path(argv[2]),output(argv[4]),scratch(argv[5]);
  require(request_path.is_absolute()&&output.is_absolute()&&scratch.is_absolute()&&output==output.lexically_normal()&&output.string().size()<=4096&&output.string().find('\0')==std::string::npos&&!output.filename().empty());
  request_step="directories";rd::Directory scratch_owner(scratch),output_parent(output.parent_path());separate(scratch_owner,output_parent);
  request_step="request-read";Input request(request_path,sha_text(Json(argv[3])),1024*1024,precheck,true);separate(scratch_owner,request.parent);separate(output_parent,request.parent);request_step="parse";const auto j=parse(request.bytes,true,precheck);
  shape(j,{"format","genesis","profile","configurationProjection","limits","budget","maximumRegistryEntries","core","roles"});require(j.at("format")=="nano-configured-journal-readonly-observer-v1");request_step="budget";const auto b=budget(j.at("budget"));
  require(b.rss<=131072);const auto record_bound=value_limit(b);
  const Check check=[&]{const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-started).count();require(!interrupted&&elapsed>=0&&uint64_t(elapsed)<b.duration&&rss()<=b.rss);};check();
  request_step="scratch";const auto scratch_empty=[&]{check();scratch_owner.recheck();const auto entries=rd::capture(scratch_owner.fd(),b.files,b.bytes,check);require(entries.size()==1&&entries.front().name.empty()&&S_ISDIR(entries.front().stamp.st_mode));scratch_owner.recheck();check();};scratch_empty();
  request_step="artifactdescriptors";const auto ga=artifact(j.at("genesis"),16*1024*1024),pa=artifact(j.at("profile"),1024*1024),ca=artifact(j.at("configurationProjection"),1024*1024);
  require(ga.bytes<=record_bound&&pa.bytes<=record_bound&&ca.bytes<=record_bound);check();
  request_step="artifactreads";Input gi(ga.p,ga.sha,ga.bytes,check),pi(pa.p,pa.sha,pa.bytes,check),ci(ca.p,ca.sha,ca.bytes,check,true);require(gi.bytes.size()==ga.bytes&&pi.bytes.size()==pa.bytes&&ci.bytes.size()==ca.bytes);
  for(const auto* input:{&gi,&pi,&ci}){check();separate(scratch_owner,input->parent);separate(output_parent,input->parent);}
  request_step="genesis-profile";const auto g=Genesis::from_json(parse(gi.bytes,false,check));check();const auto rules=load_capella_fork_profile(pi.bytes,unhex("0x"+pa.sha),1024*1024);check();request_step="limits";require(j.at("limits").is_array()&&j.at("limits").size()==8);for(const auto& value:j.at("limits")){check();positive(value);}const auto limits=checkpoint_blob_gc_snapshot_limits(j.at("limits"));
  require(limits.envelope_bytes&&limits.envelope_bytes<=record_bound&&limits.envelope_bytes<=(SIZE_MAX-2048)/8&&limits.state_bytes&&limits.state_bytes<=record_bound&&limits.total_state_bytes<=b.rss*1024/2);
  require(limits.blocks<=b.files&&limits.messages<=b.files&&limits.equivocations<=b.files);const auto witness_bound=2048+8*limits.envelope_bytes;check();
  request_step="chain-profile";require(g.consensus=="beacon"&&rules.transition.chain_id==g.chain_id&&rules.transition.replay_policy==ReplayPolicy::RequireChainId);const auto maximum=positive(j.at("maximumRegistryEntries"));require(maximum<=SIZE_MAX&&maximum<=rules.transition.consensus.state.validator_registry_limit&&maximum<=record_bound/(sizeof(ConsensusSlashingValidator)+4*sizeof(void*)));
  request_step="projection";const auto configuration=parse(ci.bytes,true,check);shape(configuration,{"format","originalConfigurationSha256","core","roles"});require(configuration.at("format")=="nano-root-configured-journal-path-key-projection-v1");sha_text(configuration.at("originalConfigurationSha256"));
  require(configuration.at("core")==j.at("core")&&configuration.at("roles")==j.at("roles"));request_step="core-selections";const auto& core=j.at("core");shape(core,{"primary","witness","expected"});const auto cp=selection(core.at("primary"),false),cw=selection(core.at("witness"),false);
  request_step="roles";const auto pair_bound=std::min<uint64_t>({b.files/2,uint64_t(request.bytes.size()/128),uint64_t(record_bound/8192)});require(pair_bound);
  std::vector<Selection> sources{cp,cw};std::set<std::string> roles,keys;require(j.at("roles").is_array()&&!j.at("roles").empty()&&j.at("roles").size()<=pair_bound);
  for(const auto& role:j.at("roles")){
   check();shape(role,{"role","keys"});require(role.at("role").is_string()&&!role.at("role").get_ref<const std::string&>().empty()&&role.at("role").get_ref<const std::string&>().size()<=1024&&roles.insert(role.at("role").get<std::string>()).second&&role.at("keys").is_array()&&!role.at("keys").empty()&&role.at("keys").size()<=pair_bound);
   for(const auto& key:role.at("keys")){check();shape(key,{"publicKey","index","primary","witness"});require(keys.size()<pair_bound&&keys.insert(hex(encoded(key.at("publicKey"),48))).second);if(!key.at("index").is_null())natural(key.at("index"));sources.push_back(selection(key.at("primary"),true));sources.push_back(selection(key.at("witness"),true));check();}
  }
  request_step="source-owners";std::vector<std::unique_ptr<rd::Directory>> source_owners;
  for(size_t a=0;a<sources.size();++a){
   check();source_owners.push_back(std::make_unique<rd::Directory>(sources[a].p));const auto& owner=*source_owners.back();separate(owner,scratch_owner);separate(owner,output_parent);
   for(const auto* input:{&request,&gi,&pi,&ci}){check();require(!physical_ancestor(owner,input->parent));}
   for(size_t z=0;z<a;++z){check();separate(owner,*source_owners[z]);}check();
  }
  request_step="source-inventories";const auto source_fence=[&]{for(size_t i=0;i<sources.size();++i){check();source_owners[i]->recheck();require(checkpoint_blob_gc_copy_inventory(sources[i].p,b.files,b.bytes,check)==sources[i].inventory);source_owners[i]->recheck();}check();};source_fence();
  request_step="request-complete";const ChainStoreMemoryPolicy policy{4*1024*1024,2,16*1024*1024,4*1024*1024};ScopedChainStoreMemoryPolicy memory(policy);
  Json identity={{"chainId",quantity(g.chain_id)},{"genesisHash",hex(g.block.header.hash())},{"consensus","beacon"}};if(!g.staking_contract.empty())identity["stakingContract"]=address(g.staking_contract);
  Json witnessed_identity={{"chainId",quantity(g.chain_id)},{"genesisHash",hex(g.block.header.hash())},{"consensus","nano-capella-recovery-v1"}};
  Json result_rows=Json::array();std::optional<Phase0ForkSlashingContext> first_context;
  // Two independent cold replica reads. Neither current DB token supplies its
  // own expected pin; both are fixed in the externally authenticated request.
  for(size_t pass=0;pass<2;++pass){
   check();stage="current-head";Reader primary(cp,scratch,b,check,identity),witness(cw,scratch,b,check,witnessed_identity);const auto p=primary.record(limits.envelope_bytes,false),w=witness.record(witness_bound,true);
   const auto primary_fetch=[&](const Bytes& id,size_t n){return primary.blob(id,n);};const auto primary_closure=authenticate_capella_checkpoint_blob_closure(p.snapshot,p.token.digest,rules,limits,primary_fetch,{},check);
   check();const auto context=primary_closure.head_context();check();require(context.registry.validators.size()<=maximum);const auto description=primary_closure.description();check();const auto& expected=core.at("expected");shape(expected,{"headRoot","identity","executionHead"});require(hex(Bytes(context.block_root.begin(),context.block_root.end()))==expected.at("headRoot").get<std::string>()&&description.at("identity")==expected.at("identity")&&description.at("executionHead")==expected.at("executionHead"));primary.coupled(p,description.at("executionHead"),g,limits.envelope_bytes);
   const auto intent=parse(w.snapshot,true,check);shape(intent,{"format","previous","next","snapshot","archive"});require(intent.at("format")=="nano-capella-recovery-v1"||intent.at("format")=="nano-capella-bootstrap-archive-recovery-v1");const auto next=token(intent.at("next"));require(intent.at("snapshot").is_string());
   require(next==p.token&&intent.at("snapshot").get_ref<const std::string&>()==p.snapshot);if(intent.at("previous").is_null())require(next.sequence==1);else{const auto previous=token(intent.at("previous"));require(previous.sequence<UINT64_MAX&&previous.sequence+1==next.sequence);}check();
   std::vector<CheckpointArchiveEntry> archive;require(intent.at("archive").is_array()&&intent.at("archive").size()<=1);
   for(const auto& row:intent.at("archive")){check();require(row.is_array()&&row.size()==3&&row.at(2).is_string());const auto& wire=row.at(2).get_ref<const std::string&>();require(limits.envelope_bytes<=(SIZE_MAX-2)/2&&wire.size()>2&&wire.size()<=2+2*limits.envelope_bytes&&wire.size()%2==0);auto bytes=unhex(wire);check();require(!bytes.empty()&&bytes.size()<=limits.envelope_bytes&&hex(bytes)==wire);archive.push_back({encoded(row.at(0),32),natural(row.at(1)),std::move(bytes)});check();}
   const auto witness_fetch=[&](const Bytes& id,size_t n){return witness.blob(id,n);};const auto witness_closure=authenticate_capella_checkpoint_blob_closure(intent.at("snapshot").get_ref<const std::string&>(),next.digest,rules,limits,witness_fetch,archive,check);
   check();const auto witness_description=witness_closure.description();require(witness_description.at("identity")==description.at("identity")&&witness_description.at("executionHead")==description.at("executionHead"));const auto witnessed_context=witness_closure.head_context();check();require(same_context(witnessed_context,context));
   if(first_context)require(same_context(*first_context,context));else{check();first_context=context;check();}
   primary.close();witness.close();Json observed=Json::array();
   stage="configured-journals";
   for(const auto& role:j.at("roles"))for(const auto& key:role.at("keys")){
    check();
    const auto jp=selection(key.at("primary"),true),jw=selection(key.at("witness"),true);Json namespace_primary={{"chainId",quantity(g.chain_id)},{"genesisHash",hex(g.block.header.hash())},{"consensus","nano-beacon-signing-v1"}},namespace_witness=namespace_primary;namespace_witness["consensus"]="nano-beacon-signing-witness-v1";
    Reader primary_journal(jp,scratch,b,check,namespace_primary),witness_journal(jw,scratch,b,check,namespace_witness);const auto pr=primary_journal.record(4096,true),wr=witness_journal.record(16384,true);
    auto pair=authenticate_beacon_signing_journal_pair({pr,wr,jp.current,jw.current,jp.floor,jw.floor},{encoded(key.at("publicKey"),48),context.genesis_root,rules.transition.consensus.epoch.ffg.slots_per_epoch},check);
    std::optional<uint64_t> index;if(!key.at("index").is_null())index=natural(key.at("index"));const auto registered=observe_beacon_journal_registration(pair,context.registry,index,size_t(maximum),check);
    const Json row={{"role",role.at("role")},{"publicKey",key.at("publicKey")},{"configuredIndex",key.at("index")},{"registeredIndex",registered.index?Json(quantity(Int(std::to_string(*registered.index)))):Json(nullptr)},{"primary",token_json(pair.primary_token())},{"witness",token_json(pair.witness_token())},{"disposition",pair.disposition()==BeaconJournalPairDisposition::Completed?"completed":"pending"},{"reservations",reservation(pair.primary())},{"candidateReservations",reservation(pair.candidate())},{"primaryFloorSupplied",pair.primary_floor().supplied},{"witnessFloorSupplied",pair.witness_floor().supplied}};
    check();require(row.dump().size()<=8192&&observed.size()<pair_bound);check();observed.push_back(row);check();
    primary_journal.close();witness_journal.close();if(pass==0){require(pairs<pair_bound);++pairs;}check();
   }
   check();if(pass==0)result_rows=std::move(observed);else require(result_rows==observed);scratch_empty();check();
  }
  const auto final_fence=[&]{check();output_parent.recheck();request.recheck(check);gi.recheck(check);pi.recheck(check);ci.recheck(check);source_fence();scratch_empty();check();};
  stage="closed-input-readback";final_fence();check();
  const Json completed={{"format","nano-configured-journal-observation-private-v1"},{"requestSha256",argv[3]},{"rows",result_rows},{"twoIndependentColdReads",true},{"currentHeadAuthenticated",true},{"floorConstraintsObserved",true},{"historicalDigestAncestryVerified",false},{"deliveredSignatureHistoryVerified",false},{"executionAccountNoncesVerified",false},{"runtimeDutiesAuthorized",false},{"newSignaturesCreated",0},{"productionQualified",false}};
  stage="publication";check();Publication publication(output_parent,output.filename().string(),check);publication.write(completed,record_bound,check);final_fence();publication.link(check);final_fence();publication.fence(check);check();
  const auto typed=Json{{"format","nano-configured-journal-observation-typed-v1"},{"status","READONLY_CONFIGURED_JOURNAL_RESERVATIONS_OBSERVED"},{"pairsObserved",pairs},{"independentColdPasses",2},{"maximumRssKiB",rss()},{"newSignaturesCreated",0},{"keysLoaded",false},{"runtimeStarted",false},{"runtimeDutiesAuthorized",false},{"productionQualified",false}}.dump();check();publication.fence(check);publication.accept();std::cout<<typed<<std::endl;return 0;
 }catch(...){std::cout<<Json{{"format","nano-configured-journal-observation-typed-v1"},{"status","REFUSED_NO_AUTOMATIC_RETRY"},{"refusedStage",stage},{"requestStep",request_step},{"pairsObserved",pairs},{"maximumRssKiB",rss()},{"newSignaturesCreated",0},{"keysLoaded",false},{"runtimeStarted",false},{"runtimeDutiesAuthorized",false},{"productionQualified",false}}.dump()<<std::endl;return 1;}
}
