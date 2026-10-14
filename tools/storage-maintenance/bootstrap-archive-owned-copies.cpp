// Offline owned copies only. No signer, journals, clock, runtime or raw Put API.
#include "nano/chain.hpp"
#include "nano/checkpoint_blob_gc.hpp"
#include "nano/consensus_capella_bootstrap_input.hpp"
#include "nano/consensus_capella_reactor.hpp"
#include "nano/consensus_beacon_header.hpp"
#include "rocksdb-owned-copy.hpp"
#include <rocksdb/db.h>
#include <openssl/evp.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <filesystem>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>
using namespace nano;
namespace fs=std::filesystem;
namespace {
const char* stage="arguments";
void require(bool ok){if(!ok)throw std::invalid_argument("Bootstrap maintenance refused");}
void io(bool ok){if(!ok)throw std::runtime_error("Bootstrap maintenance I/O refused");}
void shape(const Json& j,std::initializer_list<const char*> keys){require(j.is_object()&&j.size()==keys.size());for(auto k:keys)require(j.contains(k));}
std::string digest(const Bytes& b){return hex(sha256(b)).substr(2);}
Bytes raw(std::string_view s){return {s.begin(),s.end()};}
std::string sha_text(const Json& j){require(j.is_string());const auto s=j.get<std::string>();require(s.size()==64&&s.find_first_not_of("0123456789abcdef")==std::string::npos);return s;}
uint64_t quantity_u64(const Json& j){require(j.is_string());const auto n=quantity(j);const auto v=u64(n);require(v&&quantity(n)==j.get<std::string>());return v;}
ForkCheckpointToken token(const Json& j){shape(j,{"sequence","digest"});const auto s=quantity_u64(j.at("sequence"));require(j.at("digest").is_string());const auto d=unhex(j.at("digest").get<std::string>());require(d.size()==32&&hex(d)==j.at("digest").get<std::string>());return {s,d};}
Json encoded(const ForkCheckpointToken& t){return {{"sequence",quantity(Int(std::to_string(t.sequence)))},{"digest",hex(t.digest)}};}
Json parse(std::string_view text){std::vector<std::set<std::string>> keys;return Json::parse(text,[&](int depth,Json::parse_event_t e,Json& v){require(depth<=32);if(e==Json::parse_event_t::object_start)keys.emplace_back();else if(e==Json::parse_event_t::object_end)keys.pop_back();else if(e==Json::parse_event_t::key)require(keys.back().insert(v.get<std::string>()).second);return true;});}
void sync_fd(int fd){int n;do{n=::fsync(fd);}while(n&&errno==EINTR);io(n==0);}
struct Fd{int n;explicit Fd(int value):n(value){io(n>=0);}~Fd(){if(n>=0)::close(n);}Fd(const Fd&)=delete;Fd& operator=(const Fd&)=delete;Fd(Fd&& other)noexcept:n(other.n){other.n=-1;}};
bool identity(const struct stat& a,const struct stat& b){return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_uid==b.st_uid&&a.st_gid==b.st_gid&&a.st_mode==b.st_mode&&a.st_nlink==b.st_nlink;}
// Directory link counts can change when this tool creates its owned output.
// Retain inode/owner/mode/path binding; file identities still include nlink.
bool directory_identity(const struct stat& a,const struct stat& b){return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_uid==b.st_uid&&a.st_gid==b.st_gid&&a.st_mode==b.st_mode;}
bool same(const struct stat& a,const struct stat& b){
#ifdef __APPLE__
 const bool times=a.st_mtimespec.tv_sec==b.st_mtimespec.tv_sec&&a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec&&a.st_ctimespec.tv_sec==b.st_ctimespec.tv_sec&&a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
 const bool times=a.st_mtim.tv_sec==b.st_mtim.tv_sec&&a.st_mtim.tv_nsec==b.st_mtim.tv_nsec&&a.st_ctim.tv_sec==b.st_ctim.tv_sec&&a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
 return identity(a,b)&&a.st_size==b.st_size&&times;
}
fs::path path(const Json& j){require(j.is_string());const auto text=j.get<std::string>();require(!text.empty()&&text.size()<=4096&&text.find('\0')==std::string::npos);const fs::path p(text);require(p.is_absolute()&&text==p.lexically_normal().string()&&!p.filename().empty()&&text[1]!='/');for(auto at=p;!at.empty();at=at.parent_path()){require(!fs::is_symlink(fs::symlink_status(at)));if(at==at.parent_path())break;}return p;}
bool inside(const fs::path& a,const fs::path& b){auto i=a.begin(),j=b.begin();for(;i!=a.end()&&j!=b.end()&&*i==*j;++i,++j){}return i==a.end();}
struct Directory{
 fs::path p;Fd fd;struct stat before{};
 explicit Directory(fs::path value):p(std::move(value)),fd(::open(p.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC)){
  io(::fstat(fd.n,&before)==0);require(S_ISDIR(before.st_mode)&&before.st_uid==::geteuid()&&(before.st_mode&077)==0&&fs::canonical(p)==p);recheck();
 }
 void recheck()const{struct stat a{},b{};io(::fstat(fd.n,&a)==0&&::lstat(p.c_str(),&b)==0);require(directory_identity(before,a)&&directory_identity(before,b));require(path(Json(p.string()))==p);}
 Json binding()const{return {{"path",p.string()},{"device",std::to_string(before.st_dev)},{"inode",std::to_string(before.st_ino)}};}
};
// Held FD, pinned owned buffer. Inputs are never logged and checked again later.
class Input{
 std::vector<std::unique_ptr<Fd>> dirs;std::vector<std::string> names;std::vector<struct stat> parent_stats;Fd file;struct stat before{};std::string leaf;
 static int open_leaf(const fs::path& p,std::vector<std::unique_ptr<Fd>>& ds,std::vector<std::string>& ns,std::vector<struct stat>& stamps,const std::function<void()>& check){
  ds.push_back(std::make_unique<Fd>(::open("/",O_RDONLY|O_DIRECTORY|O_CLOEXEC)));ns.push_back("");struct stat root{};io(::fstat(ds.back()->n,&root)==0);stamps.push_back(root);
  for(const auto& part:p.parent_path().relative_path()){check();const auto name=part.string();ds.push_back(std::make_unique<Fd>(::openat(ds.back()->n,name.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC)));ns.push_back(name);struct stat st{};io(::fstat(ds.back()->n,&st)==0);stamps.push_back(st);}
  return ::openat(ds.back()->n,p.filename().c_str(),O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);
 }
 void metadata()const{
  for(size_t i=1;i<dirs.size();++i){struct stat a{},b{};io(::fstat(dirs[i]->n,&a)==0&&::fstatat(dirs[i-1]->n,names[i].c_str(),&b,AT_SYMLINK_NOFOLLOW)==0);require(S_ISDIR(a.st_mode)&&directory_identity(parent_stats[i],a)&&directory_identity(parent_stats[i],b));}
  struct stat a{},b{};io(::fstat(file.n,&a)==0&&::fstatat(dirs.back()->n,leaf.c_str(),&b,AT_SYMLINK_NOFOLLOW)==0);require(same(before,a)&&same(before,b));
 }
public:
 fs::path p;Bytes bytes;std::string sha;
 Input(const fs::path& value,const std::string& expected,size_t maximum,const std::function<void()>& check):file(open_leaf(value,dirs,names,parent_stats,check)),leaf(value.filename().string()),p(value),sha(expected){
  sha_text(Json(expected));io(::fstat(file.n,&before)==0);require(S_ISREG(before.st_mode)&&before.st_nlink==1&&before.st_uid==::geteuid()&&(before.st_mode&0022)==0&&before.st_size>0&&uint64_t(before.st_size)<=maximum);metadata();bytes.resize(size_t(before.st_size));size_t at=0;
  while(at<bytes.size()){check();const auto n=::pread(file.n,bytes.data()+at,std::min<size_t>(65536,bytes.size()-at),off_t(at));if(n<0&&errno==EINTR)continue;io(n>0);at+=size_t(n);}metadata();require(digest(bytes)==expected);check();metadata();
 }
 void recheck(const std::function<void()>& check)const{metadata();std::array<uint8_t,65536> b{};size_t at=0;while(at<bytes.size()){check();const auto n=::pread(file.n,b.data(),std::min(b.size(),bytes.size()-at),off_t(at));if(n<0&&errno==EINTR)continue;io(n>0);require(std::equal(b.begin(),b.begin()+n,bytes.begin()+at));at+=size_t(n);}metadata();}
 void private_receipt()const{require((before.st_mode&0777)==0600);metadata();}
 Json json()const{return parse(std::string_view(reinterpret_cast<const char*>(bytes.data()),bytes.size()));}
};
struct Artifact{fs::path p;std::string sha;size_t bytes;};
Artifact artifact(const Json& j,size_t maximum){shape(j,{"path","sha256","bytes"});require(j.at("bytes").is_number_integer());uint64_t n=0;if(j.at("bytes").is_number_unsigned())n=j.at("bytes").get<uint64_t>();else{const auto v=j.at("bytes").get<int64_t>();require(v>0);n=uint64_t(v);}require(n&&n<=maximum);return {path(j.at("path")),sha_text(j.at("sha256")),size_t(n)};}
struct Budget{uint64_t files,physical,records,logical,duration;};
Budget budget(const Json& j){shape(j,{"inventoryFiles","inventoryBytes","logicalRecords","logicalBytes","durationMs"});Budget b{quantity_u64(j.at("inventoryFiles")),quantity_u64(j.at("inventoryBytes")),quantity_u64(j.at("logicalRecords")),quantity_u64(j.at("logicalBytes")),quantity_u64(j.at("durationMs"))};require(b.duration<=uint64_t(INT64_MAX));return b;}
struct StoreInput{fs::path p,source;std::string inventory,source_inventory;ForkCheckpointToken pin;};
StoreInput store(const Json& j){shape(j,{"path","sourcePath","inventorySha256","sourceInventorySha256","token"});return {path(j.at("path")),path(j.at("sourcePath")),sha_text(j.at("inventorySha256")),sha_text(j.at("sourceInventorySha256")),token(j.at("token"))};}
class Copies{
 const Budget b;const std::function<void()> check;
public:
 StoreInput p,w;std::array<std::unique_ptr<Directory>,4> d;
 Copies(StoreInput primary,StoreInput witness,Budget bounds,std::function<void()> callback):b(bounds),check(std::move(callback)),p(std::move(primary)),w(std::move(witness)){
  const std::array<fs::path,4> paths{p.p,w.p,p.source,w.source};for(size_t i=0;i<4;++i)d[i]=std::make_unique<Directory>(paths[i]);
  for(size_t i=0;i<4;++i)for(size_t j=i+1;j<4;++j)require((d[i]->before.st_dev!=d[j]->before.st_dev||d[i]->before.st_ino!=d[j]->before.st_ino)&&!inside(paths[i],paths[j])&&!inside(paths[j],paths[i]));
  for(size_t i=0;i<2;++i){Directory marker_parent(d[i]->p.parent_path());const auto marker=d[i]->p.parent_path()/".nano-owned-bootstrap-archive-copy";require(path(Json(marker.string()))==marker);struct stat s{};io(::lstat(marker.c_str(),&s)==0);require(S_ISREG(s.st_mode)&&s.st_uid==::geteuid()&&s.st_nlink==1&&(s.st_mode&077)==0);nano::maintenance::require_owned_marker(marker,"owned-offline-bootstrap-archive-v1");require(fs::is_regular_file(d[i]->p/"CURRENT"));}
  original_sources();
 }
 void original_sources()const{for(size_t i=0;i<4;++i)d[i]->recheck();require(checkpoint_blob_gc_copy_inventory(p.source,b.files,b.physical,check)==p.source_inventory&&checkpoint_blob_gc_copy_inventory(w.source,b.files,b.physical,check)==w.source_inventory);}
 std::array<std::string,2> inventories()const{return {checkpoint_blob_gc_copy_inventory(p.p,b.files,b.physical,check),checkpoint_blob_gc_copy_inventory(w.p,b.files,b.physical,check)};}
 Json bindings()const{return Json::array({d[0]->binding(),d[1]->binding()});}
 void outside(const fs::path& value)const{for(const auto& dir:d)require(!inside(dir->p,value)&&!inside(value,dir->p));}
 void initial()const{const auto now=inventories();require(now[0]==p.inventory&&now[1]==w.inventory&&p.inventory==p.source_inventory&&w.inventory==w.source_inventory);}
};
class Output{
 std::unique_ptr<Directory> parent;std::unique_ptr<Fd> owned;struct stat created{};
public:
 fs::path p;
 Output(const fs::path& output,const Copies& copies):p(output){copies.outside(p);require(!fs::exists(fs::symlink_status(p)));parent=std::make_unique<Directory>(p.parent_path());io(::mkdirat(parent->fd.n,p.filename().c_str(),0700)==0);sync_fd(parent->fd.n);owned=std::make_unique<Fd>(::openat(parent->fd.n,p.filename().c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));io(::fstat(owned->n,&created)==0);require(S_ISDIR(created.st_mode)&&created.st_uid==::geteuid()&&(created.st_mode&0777)==0700);parent->recheck();}
 void write(const char* name,const Json& j){parent->recheck();struct stat a{},entry{};io(::fstat(owned->n,&a)==0&&::fstatat(parent->fd.n,p.filename().c_str(),&entry,AT_SYMLINK_NOFOLLOW)==0);require(directory_identity(created,a)&&directory_identity(created,entry));const auto data=j.dump()+"\n";Fd fd(::openat(owned->n,name,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600));struct stat created_file{};io(::fstat(fd.n,&created_file)==0);require(S_ISREG(created_file.st_mode)&&created_file.st_uid==::geteuid()&&created_file.st_nlink==1&&(created_file.st_mode&0777)==0600);size_t at=0;while(at<data.size()){const auto n=::write(fd.n,data.data()+at,data.size()-at);if(n<0&&errno==EINTR)continue;io(n>0);at+=size_t(n);}sync_fd(fd.n);sync_fd(owned->n);}
};
struct Hash{
 std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)> c{EVP_MD_CTX_new(),EVP_MD_CTX_free};
 Hash(){io(bool(c)&&EVP_DigestInit_ex(c.get(),EVP_sha256(),nullptr)==1);}
 void frame(const rocksdb::Slice& s){std::array<unsigned char,8> n{};uint64_t z=s.size();for(unsigned i=0;i<8;++i)n[7-i]=uint8_t(z>>(8*i));io(EVP_DigestUpdate(c.get(),n.data(),n.size())==1&&EVP_DigestUpdate(c.get(),s.data(),s.size())==1);}
 std::string finish(){Bytes out(32);unsigned n=0;io(EVP_DigestFinal_ex(c.get(),out.data(),&n)==1&&n==32);return hex(out).substr(2);}
};
std::string invariant(const fs::path& p,const std::set<std::string>& allowed,const Budget& b,const std::function<void()>& check){
 rocksdb::Options o;o.create_if_missing=false;o.paranoid_checks=true;auto db=nano::maintenance::open_read_only_copy(o,p.string());rocksdb::ReadOptions r;r.verify_checksums=true;r.fill_cache=false;std::unique_ptr<rocksdb::Iterator> it(db->NewIterator(r));Hash h;uint64_t records=0,bytes=0;
 for(it->SeekToFirst();it->Valid();it->Next()){check();require(records<b.records);++records;require(it->key().size()<=UINT64_MAX-it->value().size());const uint64_t n=it->key().size()+it->value().size();require(bytes<=b.logical&&n<=b.logical-bytes);bytes+=n;if(!allowed.contains(it->key().ToString())){h.frame(it->key());h.frame(it->value());}}io(it->status().ok());return h.finish();
}
void stopped(const Json& s,const Copies& c){shape(s,{"format","scope","allWritersStopped","allReadersStopped","stoppedAtMs","copies"});require(s.at("format")=="nano-stopped-checkpoint-copy-receipt-v1"&&s.at("scope")=="isolated-owned-copies"&&s.at("allWritersStopped").is_boolean()&&s.at("allReadersStopped").is_boolean()&&s.at("allWritersStopped")==true&&s.at("allReadersStopped")==true);quantity_u64(s.at("stoppedAtMs"));require(s.at("copies").is_array()&&s.at("copies").size()==2);unsigned i=0;for(const auto* role:{"primary","witness"}){const auto& x=s.at("copies")[i];const auto& v=i++?c.w:c.p;shape(x,{"role","path","inventorySha256","sourcePath","sourceInventorySha256"});require(x.at("role")==role&&x.at("path")==v.p.string()&&x.at("sourcePath")==v.source.string()&&x.at("inventorySha256")==v.inventory&&x.at("sourceInventorySha256")==v.source_inventory);}}
struct Pair{
 std::unique_ptr<Blockchain> owner;std::unique_ptr<ChainStore> witness;std::unique_ptr<CapellaDurableReactor> reactor;
 Pair(const Copies& c,const Genesis& g,const CapellaForkBlockRules& rules,CapellaStoreSnapshotLimits limits,const CapellaSignedBootstrapArchive& proof,const std::function<void()>& check,bool recover,ForkCheckpointToken np,ForkCheckpointToken nw){
  owner=std::make_unique<Blockchain>(c.p.p,g,ChainStore::OpenMode::ReadWriteExisting);witness=std::make_unique<ChainStore>(c.w.p,ChainStore::OpenMode::ReadWriteExisting);witness->require_existing_chain_binding(g.chain_id,g.block.header.hash(),"nano-capella-recovery-v1");
  if(recover){reactor=std::make_unique<CapellaDurableReactor>(*owner,rules,limits,*witness,proof,np,nw,check);return;}
  const auto a=owner->store().fork_checkpoint(g.chain_id,g.block.header.hash(),limits.envelope_bytes,c.p.pin);const auto maximum=2048+8*limits.envelope_bytes;const auto b=witness->fork_checkpoint(g.chain_id,g.block.header.hash(),maximum,c.w.pin);require(a&&b);const auto intent=parse(b->snapshot);shape(intent,{"format","previous","next","snapshot","archive"});require(intent.dump()==b->snapshot&&token(intent.at("next"))==c.p.pin&&intent.at("snapshot")==a->snapshot);
  if(intent.at("format")=="nano-capella-bootstrap-archive-recovery-v1")reactor=std::make_unique<CapellaDurableReactor>(*owner,rules,limits,*witness,proof,c.p.pin,c.w.pin,check);
  else{require(intent.at("format")=="nano-capella-recovery-v1");reactor=std::make_unique<CapellaDurableReactor>(*owner,rules,limits,*witness,check,c.p.pin);}
 }
};
Json retained(const char* role,const std::string& request,const Copies& c,bool work,const ForkCheckpointToken& next,const std::array<std::string,2>& hashes,const std::array<std::string,2>& inv){return {{"format","nano-bootstrap-archive-retained-next-v1"},{"role",role},{"requestSha256",request},{"currentPrimary",encoded(c.p.pin)},{"currentWitness",encoded(c.w.pin)},{"next",encoded(next)},{"hasWork",work},{"copies",c.bindings()},{"invariants",Json::array({hashes[0],hashes[1]})},{"preparedInventories",Json::array({inv[0],inv[1]})}};}
void retained_relation(bool work,const ForkCheckpointToken& current_p,const ForkCheckpointToken& current_w,const ForkCheckpointToken& next_p,const ForkCheckpointToken& next_w){
 if(!work){require(next_p==current_p&&next_w==current_w);return;}
 require(current_p.sequence<UINT64_MAX&&current_w.sequence<UINT64_MAX&&next_p.sequence==current_p.sequence+1&&next_p.digest==current_p.digest&&next_w.sequence==current_w.sequence+1);
}
void retained_shape(const Json& j,const char* role,const std::string& request,const Copies& c){shape(j,{"format","role","requestSha256","currentPrimary","currentWitness","next","hasWork","copies","invariants","preparedInventories"});require(j.at("format")=="nano-bootstrap-archive-retained-next-v1"&&j.at("role")==role&&j.at("requestSha256")==request&&token(j.at("currentPrimary"))==c.p.pin&&token(j.at("currentWitness"))==c.w.pin&&j.at("hasWork").is_boolean()&&j.at("copies")==c.bindings());require(j.at("invariants").is_array()&&j.at("invariants").size()==2&&j.at("preparedInventories").is_array()&&j.at("preparedInventories").size()==2);for(unsigned i=0;i<2;++i){sha_text(j.at("invariants")[i]);sha_text(j.at("preparedInventories")[i]);}token(j.at("next"));}
int run(int argc,char** argv){
 require(argc==5||argc==9);const std::string op=argv[1];require((op=="plan"&&argc==5)||((op=="apply"||op=="recover")&&argc==9));const auto began=std::chrono::steady_clock::now();const std::function<void()> initial_deadline=[&]{require(std::chrono::steady_clock::now()-began<std::chrono::seconds(10));};
 stage="pinned-request";Input input(path(Json(argv[2])),sha_text(Json(argv[3])),65536,initial_deadline);const auto request=input.json();shape(request,{"format","genesis","profile","authority","stopReceipt","primary","witness","limits","budget"});require(request.at("format")=="nano-owned-bootstrap-archive-maintenance-v1"&&request.dump()==std::string(input.bytes.begin(),input.bytes.end()));const auto b=budget(request.at("budget"));const std::function<void()> check=[&]{const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-began).count();require(elapsed>=0&&uint64_t(elapsed)<b.duration);};check();
 const auto ga=artifact(request.at("genesis"),16*1024*1024),pa=artifact(request.at("profile"),1024*1024),sa=artifact(request.at("stopReceipt"),65536);Input gi(ga.p,ga.sha,ga.bytes,check),pi(pa.p,pa.sha,pa.bytes,check),si(sa.p,sa.sha,sa.bytes,check);require(gi.bytes.size()==ga.bytes&&pi.bytes.size()==pa.bytes&&si.bytes.size()==sa.bytes);const auto g=Genesis::from_json(gi.json());const auto rules=load_capella_fork_profile(std::string(pi.bytes.begin(),pi.bytes.end()),unhex("0x"+pa.sha),1024*1024);const auto limits=checkpoint_blob_gc_snapshot_limits(request.at("limits"));require(limits.envelope_bytes<=(SIZE_MAX-2048)/8);
 stage="signed-authority";const auto proof=load_capella_signed_bootstrap_proof(g,rules,limits,request.at("authority"),check);const auto header=proof.header();const auto anchor=consensus_beacon_header_root(header);const auto root=hex(Bytes(anchor.begin(),anchor.end()));const auto wire=proof.signed_wire();
 stage="owned-stopped-copies";Copies copies(store(request.at("primary")),store(request.at("witness")),b,check);stopped(si.json(),copies);for(const auto* a:{&gi,&pi,&si,&input})copies.outside(a->p);for(const auto* kind:{"genesisState","signedBlock"})copies.outside(path(request.at("authority").at(kind).at("path")));
 Output out(path(Json(argv[4])),copies);out.write("operation-intent.json",Json{{"format","nano-bootstrap-maintenance-operation-intent-v1"},{"operation",op},{"requestSha256",input.sha},{"currentPrimary",encoded(copies.p.pin)},{"currentWitness",encoded(copies.w.pin)}});
 const std::set<std::string> primary_allowed{"fork-checkpoint/v1/token","checkpoint-archive/v1/"+root,"checkpoint-archive-slot/v1/0x0000000000000001/"+root};const std::set<std::string> witness_allowed{"fork-checkpoint/v1/token","fork-checkpoint/v1/payload"};
 std::array<std::string,2> hashes{invariant(copies.p.p,primary_allowed,b,check),invariant(copies.w.p,witness_allowed,b,check)};
 std::unique_ptr<Input> pfile,wfile;Json rp,rw;ForkCheckpointToken np=copies.p.pin,nw=copies.w.pin;bool work=false;
 if(op=="plan")copies.initial();else{
  stage="retained-two-commitments";pfile=std::make_unique<Input>(path(Json(argv[5])),sha_text(Json(argv[6])),65536,check);wfile=std::make_unique<Input>(path(Json(argv[7])),sha_text(Json(argv[8])),65536,check);pfile->private_receipt();wfile->private_receipt();copies.outside(pfile->p);copies.outside(wfile->p);require(pfile->p!=wfile->p);rp=pfile->json();rw=wfile->json();retained_shape(rp,"primary",input.sha,copies);retained_shape(rw,"witness",input.sha,copies);auto x=rp,y=rw;x.erase("role");x.erase("next");y.erase("role");y.erase("next");require(x==y);np=token(rp.at("next"));nw=token(rw.at("next"));work=rp.at("hasWork").get<bool>();retained_relation(work,copies.p.pin,copies.w.pin,np,nw);require(rp.at("invariants")==Json::array({hashes[0],hashes[1]}));if(op=="apply"){const auto prepared=copies.inventories();require(rp.at("preparedInventories")==Json::array({prepared[0],prepared[1]}));}else require(work);
 }
 stage="pre-publication-retention";input.recheck(check);gi.recheck(check);pi.recheck(check);si.recheck(check);copies.original_sources();if(pfile){pfile->recheck(check);wfile->recheck(check);out.write("primary-next.json",rp);out.write("witness-next.json",rw);}
 stage="reconstructed-process-local-plan";
 {Pair pair(copies,g,rules,limits,proof,check,op=="recover",np,nw);
  if(op!="recover"){
   auto plan=pair.reactor->prepare_bootstrap_archive(proof,copies.p.pin,copies.w.pin,check);if(op=="plan"){work=plan.has_work();np=plan.next_primary();nw=plan.next_witness();}else{require(work==plan.has_work()&&np==plan.next_primary()&&nw==plan.next_witness());input.recheck(check);gi.recheck(check);pi.recheck(check);si.recheck(check);pfile->recheck(check);wfile->recheck(check);copies.original_sources();stage="explicit-archive-publication";const auto result=pair.reactor->apply_bootstrap_archive(std::move(plan),check);require(result.published==work&&result.primary==np&&result.witness==nw);}
  }else require(pair.reactor->current().token==np);
 }
 stage="closed-cold-readback";copies.original_sources();require(invariant(copies.p.p,primary_allowed,b,check)==hashes[0]&&invariant(copies.w.p,witness_allowed,b,check)==hashes[1]);
 {ChainStore p(copies.p.p,ChainStore::OpenMode::ReadOnly),w(copies.w.p,ChainStore::OpenMode::ReadOnly);p.require_existing_chain_binding(g.chain_id,g.block.header.hash(),g.consensus,g.staking_contract);w.require_existing_chain_binding(g.chain_id,g.block.header.hash(),"nano-capella-recovery-v1");const auto a=p.fork_checkpoint(g.chain_id,g.block.header.hash(),limits.envelope_bytes,op=="plan"?copies.p.pin:np);const auto z=w.fork_checkpoint(g.chain_id,g.block.header.hash(),2048+8*limits.envelope_bytes,op=="plan"?copies.w.pin:nw);require(a&&z);if(op!="plan"){
   const auto archive=p.checkpoint_archive(Bytes(anchor.begin(),anchor.end()),limits.envelope_bytes);require(archive&&archive->slot==1&&archive->payload==wire);
  }}
 if(op!="plan"){
  // Reconstruct through typed recovery once more to verify the archive/index,
  // coupled tip/receipts and independently read cold/fragment references.
  Pair reopened(copies,g,rules,limits,proof,check,work,np,nw);require(reopened.reactor->current().token==np);const auto noop=reopened.reactor->prepare_bootstrap_archive(proof,np,nw,check);require(!noop.has_work());
 }
 input.recheck(check);gi.recheck(check);pi.recheck(check);si.recheck(check);if(pfile){pfile->recheck(check);wfile->recheck(check);}copies.original_sources();require(invariant(copies.p.p,primary_allowed,b,check)==hashes[0]&&invariant(copies.w.p,witness_allowed,b,check)==hashes[1]);const auto inv=copies.inventories();
 stage="independent-private-receipts";retained_relation(work,copies.p.pin,copies.w.pin,np,nw);if(op=="plan"){
  out.write("primary-next.json",retained("primary",input.sha,copies,work,np,hashes,inv));out.write("witness-next.json",retained("witness",input.sha,copies,work,nw,hashes,inv));
 }
 out.write("complete.json",Json{{"format","nano-bootstrap-archive-maintenance-completed-v1"},{"operation",op},{"requestSha256",input.sha},{"hasWork",work},{"primary",encoded(np)},{"witness",encoded(nw)},{"copies",copies.bindings()},{"allDatabaseHandlesClosedBeforeReceipt",true},{"originalSourcesUnchanged",true},{"rawSnapshotAndUnrelatedNamespacesPreserved",true},{"operatorHostOnly",true},{"newSignatures",0},{"runtimeStarted",false},{"productionQualified",false}});
 std::cout<<Json{{"completed",true},{"operation",op},{"hasWork",work},{"stores",2},{"newSignatures",0},{"runtimeStarted",false},{"productionQualified",false}}.dump()<<std::endl;return 0;
}
}
int main(int argc,char** argv){try{return run(argc,argv);}catch(...){std::cout<<Json{{"completed",false},{"refusedStage",stage},{"rawErrorsSuppressed",true},{"runtimeStarted",false},{"productionQualified",false}}.dump()<<std::endl;return 1;}}
