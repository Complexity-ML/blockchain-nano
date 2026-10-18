#pragma once
// Private maintenance capability. Never opens a source DB or promotes a copy
// into protocol authority. RO readers may use only a sealed independent tree.
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <dirent.h>
#include <fcntl.h>
#include <filesystem>
#include <functional>
#include <memory>
#include <iostream>
#include <locale>
#include <mutex>
#include <source_location>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>
namespace nano::maintenance {
namespace readonly_replica_detail {
namespace fs=std::filesystem;
// Failure-only diagnostic. The caller's already captured values are the only
// stat inputs; this code never opens, reads, restats, repairs or reseals a tree.
namespace diagnostic_v4_detail {
struct Budget {std::mutex mutex;size_t records=0,bytes=0;};
inline Budget& budget(){static Budget value;return value;}
inline void captured_stat(std::ostream& out,const struct stat& s){
#ifdef __APPLE__
 const auto mt=s.st_mtimespec,ct=s.st_ctimespec;
#else
 const auto mt=s.st_mtim,ct=s.st_ctim;
#endif
 out<<"{\"st_dev\":\""<<std::uintmax_t(s.st_dev)<<"\",\"st_ino\":\""<<std::uintmax_t(s.st_ino)
    <<"\",\"st_mode\":\""<<std::uintmax_t(s.st_mode)<<"\",\"st_nlink\":\""<<std::uintmax_t(s.st_nlink)
    <<"\",\"st_uid\":\""<<std::uintmax_t(s.st_uid)<<"\",\"st_gid\":\""<<std::uintmax_t(s.st_gid)
    <<"\",\"st_size\":\""<<std::intmax_t(s.st_size)
    <<"\",\"st_mtime\":{\"seconds\":\""<<std::intmax_t(mt.tv_sec)<<"\",\"nanoseconds\":\""<<std::intmax_t(mt.tv_nsec)
    <<"\"},\"st_ctime\":{\"seconds\":\""<<std::intmax_t(ct.tv_sec)<<"\",\"nanoseconds\":\""<<std::intmax_t(ct.tv_nsec)<<"\"}}";
}
inline void failure(const char* kind,const std::source_location& where,
                    const struct stat* before=nullptr,const struct stat* after=nullptr,const std::uintmax_t* expected_euid=nullptr)noexcept{
 try{
  auto& state=budget();std::lock_guard lock(state.mutex);
  constexpr size_t maximum_records=32,maximum_bytes=1024*1024;
  if(state.records>=maximum_records||state.bytes>=maximum_bytes)return;
  std::ostringstream out;out.imbue(std::locale::classic());
  out<<"{\"diagnostic\":\"readonly-replica-source-failure-v4\",\"kind\":\""<<kind
     <<"\",\"callerLine\":\""<<where.line()<<"\",\"result\":false";
  if(before&&after){out<<",\"before\":";captured_stat(out,*before);out<<",\"after\":";captured_stat(out,*after);}
  if(expected_euid)out<<",\"expectedEuid\":\""<<*expected_euid<<"\"";
  out<<"}\n";if(!out)return;const auto text=out.str();
  if(text.size()>maximum_bytes-state.bytes)return;
  ++state.records;state.bytes+=text.size();
  std::cerr.write(text.data(),std::streamsize(text.size()));std::cerr.flush();
 }catch(...){/* The original predicate/refusal, outside this block, remains. */}
}
} // diagnostic_v4_detail
inline void need(bool ok,const std::source_location& where=std::source_location::current()){
 if(!ok){diagnostic_v4_detail::failure("need",where);throw std::invalid_argument("Readonly replica refused");}
}
inline void io(bool ok,const std::source_location& where=std::source_location::current()){
 if(!ok){diagnostic_v4_detail::failure("io",where);throw std::runtime_error("Readonly replica I/O refused");}
}
struct Fd{
 int n=-1;explicit Fd(int fd):n(fd){io(n>=0);}~Fd(){if(n>=0)::close(n);}
 Fd(const Fd&)=delete;Fd& operator=(const Fd&)=delete;
 Fd(Fd&& x)noexcept:n(x.n){x.n=-1;}
};
inline bool identity(const struct stat& a,const struct stat& b,const std::source_location& where=std::source_location::current()){
 const bool result=a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_uid==b.st_uid&&a.st_gid==b.st_gid&&a.st_mode==b.st_mode&&a.st_nlink==b.st_nlink;
 if(!result)diagnostic_v4_detail::failure("identity",where,&a,&b);return result;
}
inline bool directory_identity(const struct stat& a,const struct stat& b,const std::source_location& where=std::source_location::current()){
 const bool result=a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_uid==b.st_uid&&a.st_gid==b.st_gid&&a.st_mode==b.st_mode;
 if(!result)diagnostic_v4_detail::failure("directory_identity",where,&a,&b);return result;
}
inline bool stable(const struct stat& a,const struct stat& b,const std::source_location& where=std::source_location::current()){
#ifdef __APPLE__
 const bool times=a.st_mtimespec.tv_sec==b.st_mtimespec.tv_sec&&a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec&&a.st_ctimespec.tv_sec==b.st_ctimespec.tv_sec&&a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
 const bool times=a.st_mtim.tv_sec==b.st_mtim.tv_sec&&a.st_mtim.tv_nsec==b.st_mtim.tv_nsec&&a.st_ctim.tv_sec==b.st_ctim.tv_sec&&a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
 const bool result=identity(a,b,where)&&a.st_size==b.st_size&&times;
 if(!result)diagnostic_v4_detail::failure("stable",where,&a,&b);return result;
}
inline std::string hexadecimal(const unsigned char* p,size_t n){std::string out;out.reserve(2*n);for(size_t i=0;i<n;++i){out.push_back("0123456789abcdef"[p[i]>>4]);out.push_back("0123456789abcdef"[p[i]&15]);}return out;}
struct Hash{
 std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)> c{EVP_MD_CTX_new(),EVP_MD_CTX_free};
 Hash(){io(bool(c)&&EVP_DigestInit_ex(c.get(),EVP_sha256(),nullptr)==1);}
 void add(const void* p,size_t n){io(EVP_DigestUpdate(c.get(),p,n)==1);}
 std::string finish(){std::array<unsigned char,32> out{};unsigned n=0;io(EVP_DigestFinal_ex(c.get(),out.data(),&n)==1&&n==32);return hexadecimal(out.data(),out.size());}
};
inline void sync(int fd){int n;do{n=::fsync(fd);}while(n<0&&errno==EINTR);io(n==0);}
inline void check(const std::function<void()>& f){if(f)f();}
struct Directory{
 fs::path path;std::vector<Fd> chain;std::vector<std::string> names;std::vector<struct stat> stamps;
 explicit Directory(fs::path p):path(std::move(p)){
  need(path.is_absolute()&&path==path.lexically_normal()&&path.string().size()<=4096&&path.string().find('\0')==std::string::npos&&path!=path.root_path());
  chain.emplace_back(::open("/",O_RDONLY|O_DIRECTORY|O_CLOEXEC));names.emplace_back();struct stat root{};io(::fstat(chain.back().n,&root)==0);stamps.push_back(root);
  for(const auto& component:path.relative_path()){
   const auto name=component.string();need(!name.empty()&&name!="."&&name!="..");
   chain.emplace_back(::openat(chain.back().n,name.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));names.push_back(name);struct stat s{};io(::fstat(chain.back().n,&s)==0);stamps.push_back(s);
  }
  const auto& leaf=stamps.back();
  if(!(S_ISDIR(leaf.st_mode)&&leaf.st_uid==::geteuid()&&(leaf.st_mode&0777)==0700)){
   const std::uintmax_t expected_euid=::geteuid();
   diagnostic_v4_detail::failure("directory_leaf_policy",std::source_location::current(),&leaf,&leaf,&expected_euid);
  }
  need(S_ISDIR(leaf.st_mode)&&leaf.st_uid==::geteuid()&&(leaf.st_mode&0777)==0700);recheck();
 }
 int fd()const{return chain.back().n;}
 void recheck()const{
  for(size_t i=1;i<chain.size();++i){struct stat a{},b{};io(::fstat(chain[i].n,&a)==0&&::fstatat(chain[i-1].n,names[i].c_str(),&b,AT_SYMLINK_NOFOLLOW)==0);need(directory_identity(stamps[i],a)&&directory_identity(stamps[i],b));}
 }
};
struct Entry{std::string name;struct stat stamp{};std::string sha;};
inline Fd relative_fd(int root,const fs::path& relative,int leaf_flags){
 need(!relative.empty()&&!relative.is_absolute()&&relative==relative.lexically_normal());Fd at(::openat(root,".",O_RDONLY|O_DIRECTORY|O_CLOEXEC));
 for(const auto& component:relative.parent_path()){
  const auto name=component.string();need(name!="."&&name!=".."&&!name.empty());
  Fd next(::openat(at.n,name.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));std::swap(at.n,next.n);
 }
 return Fd(::openat(at.n,relative.filename().c_str(),leaf_flags|O_NOFOLLOW|O_CLOEXEC));
}
inline Fd relative_parent(int root,const fs::path& relative){
 if(relative.parent_path().empty())return Fd(::openat(root,".",O_RDONLY|O_DIRECTORY|O_CLOEXEC));
 return relative_fd(root,relative.parent_path(),O_RDONLY|O_DIRECTORY);
}
inline std::string file_hash(int fd,const struct stat& expected,const std::function<void()>& checkpoint){
 struct stat before{},after{};io(::fstat(fd,&before)==0);need(stable(expected,before)&&S_ISREG(before.st_mode)&&before.st_nlink==1&&before.st_size>=0);
 Hash hash;std::array<unsigned char,65536> buffer{};uint64_t at=0;
 while(at<uint64_t(before.st_size)){check(checkpoint);const size_t count=size_t(std::min<uint64_t>(buffer.size(),uint64_t(before.st_size)-at));const auto n=::pread(fd,buffer.data(),count,off_t(at));if(n<0&&errno==EINTR)continue;io(n>0);hash.add(buffer.data(),size_t(n));at+=uint64_t(n);}
 check(checkpoint);io(::fstat(fd,&after)==0);need(stable(before,after));return hash.finish();
}
inline void scan(int dir,const std::string& prefix,uint64_t maximum_entries,uint64_t maximum_bytes,
                 uint64_t& total,std::vector<Entry>& out,const std::function<void()>& checkpoint,size_t depth=0){
 need(depth<=128);check(checkpoint);struct stat before{},after{};io(::fstat(dir,&before)==0);need(S_ISDIR(before.st_mode)&&before.st_uid==::geteuid()&&(before.st_mode&0022)==0);
 if(prefix.empty())out.push_back({"",before,{}});
 Fd listing(::openat(dir,".",O_RDONLY|O_DIRECTORY|O_CLOEXEC));DIR* raw=::fdopendir(listing.n);io(raw!=nullptr);listing.n=-1;std::unique_ptr<DIR,decltype(&::closedir)> owned(raw,&::closedir);std::vector<std::string> names;
 for(;;){check(checkpoint);errno=0;auto* entry=::readdir(owned.get());if(!entry){io(errno==0);break;}std::string name(entry->d_name);if(name=="."||name=="..")continue;need(names.size()<maximum_entries&&name.find('/')==std::string::npos);names.push_back(std::move(name));}
 std::sort(names.begin(),names.end());
 for(const auto& name:names){
  check(checkpoint);need(out.size()-1<maximum_entries);struct stat s{};io(::fstatat(dir,name.c_str(),&s,AT_SYMLINK_NOFOLLOW)==0);need(s.st_uid==::geteuid()&&(s.st_mode&0022)==0&&(S_ISDIR(s.st_mode)||S_ISREG(s.st_mode)));
  const auto relative=prefix.empty()?name:prefix+"/"+name;need(relative.size()<=4096);const size_t index=out.size();out.push_back({relative,s,{}});
  Fd leaf(::openat(dir,name.c_str(),O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC|(S_ISDIR(s.st_mode)?O_DIRECTORY:0)));struct stat held{},linked{};io(::fstat(leaf.n,&held)==0&&::fstatat(dir,name.c_str(),&linked,AT_SYMLINK_NOFOLLOW)==0);need(stable(s,held)&&stable(s,linked));
  if(S_ISDIR(s.st_mode))scan(leaf.n,relative,maximum_entries,maximum_bytes,total,out,checkpoint,depth+1);
  else{need(s.st_nlink==1&&s.st_size>=0&&uint64_t(s.st_size)<=maximum_bytes-total);total+=uint64_t(s.st_size);out[index].sha=file_hash(leaf.n,s,checkpoint);}
  io(::fstat(leaf.n,&held)==0&&::fstatat(dir,name.c_str(),&linked,AT_SYMLINK_NOFOLLOW)==0);need(stable(s,held)&&stable(s,linked));
 }
 io(::fstat(dir,&after)==0);need(stable(before,after));
}
inline std::vector<Entry> capture(int root,uint64_t maximum_entries,uint64_t maximum_bytes,const std::function<void()>& checkpoint){
 need(maximum_entries&&maximum_bytes);uint64_t total=0;std::vector<Entry> entries;scan(root,"",maximum_entries,maximum_bytes,total,entries,checkpoint);return entries;
}
inline bool same_entries(const std::vector<Entry>& a,const std::vector<Entry>& b){if(a.size()!=b.size())return false;for(size_t i=0;i<a.size();++i)if(a[i].name!=b[i].name||a[i].sha!=b[i].sha||!stable(a[i].stamp,b[i].stamp))return false;return true;}
inline bool same_contents(const std::vector<Entry>& source,const std::vector<Entry>& copied){
 if(source.size()!=copied.size())return false;
 for(size_t i=0;i<source.size();++i){const auto& a=source[i];const auto& b=copied[i];if(a.name!=b.name||a.sha!=b.sha||S_ISDIR(a.stamp.st_mode)!=S_ISDIR(b.stamp.st_mode))return false;
  if(a.stamp.st_dev==b.stamp.st_dev&&a.stamp.st_ino==b.stamp.st_ino)return false;
  if(S_ISREG(a.stamp.st_mode)&&(a.stamp.st_size!=b.stamp.st_size||(b.stamp.st_mode&0777)!=0600||b.stamp.st_nlink!=1))return false;
  if(S_ISDIR(b.stamp.st_mode)&&(b.stamp.st_mode&0777)!=0700)return false;
 }
 return true;
}
} // readonly_replica_detail
class SealedReadonlyReplica{
 struct State;
 std::unique_ptr<State> state_;
 explicit SealedReadonlyReplica(std::unique_ptr<State> state);
public:
 SealedReadonlyReplica(const SealedReadonlyReplica&)=delete;SealedReadonlyReplica& operator=(const SealedReadonlyReplica&)=delete;
 SealedReadonlyReplica(SealedReadonlyReplica&&)noexcept;
 ~SealedReadonlyReplica();
 static SealedReadonlyReplica capture(const std::filesystem::path& source,const std::filesystem::path& owned_output,
  uint64_t maximum_entries,uint64_t maximum_bytes,std::function<void()> checkpoint={});
 std::filesystem::path read_only_path()const;
 void recheck()const;
 void close();
};
struct SealedReadonlyReplica::State{
 using Entry=readonly_replica_detail::Entry;
 readonly_replica_detail::Directory source,parent;
 std::string leaf;std::filesystem::path path;std::unique_ptr<readonly_replica_detail::Fd> root;
 struct stat created{};uint64_t maximum_entries,maximum_bytes;std::function<void()> checkpoint;
 std::vector<Entry> source_entries,sealed_entries,created_entries;bool closed=false;
 State(const std::filesystem::path& from,const std::filesystem::path& output,uint64_t entries,uint64_t bytes,std::function<void()> check):source(from),parent(output),maximum_entries(entries),maximum_bytes(bytes),checkpoint(std::move(check)){}
 void root_binding()const{using namespace readonly_replica_detail;parent.recheck();struct stat held{},linked{};io(::fstat(root->n,&held)==0&&::fstatat(parent.fd(),leaf.c_str(),&linked,AT_SYMLINK_NOFOLLOW)==0);need(directory_identity(created,held)&&directory_identity(created,linked));}
 bool cleanup()noexcept{
  if(closed)return true;
  if(!root)return leaf.empty();
  try{using namespace readonly_replica_detail;root_binding();
   // Check every owned inode before the first unlink; no discovery/deletion of
   // names which this capture did not create. Never follow a substituted path.
   for(const auto& e:created_entries){auto at=relative_parent(root->n,std::filesystem::path(e.name));struct stat now{};io(::fstatat(at.n,std::filesystem::path(e.name).filename().c_str(),&now,AT_SYMLINK_NOFOLLOW)==0);need(directory_identity(e.stamp,now)&&(!S_ISREG(now.st_mode)||now.st_nlink==1));}
   for(auto it=created_entries.rbegin();it!=created_entries.rend();++it){auto at=relative_parent(root->n,std::filesystem::path(it->name));struct stat now{};io(::fstatat(at.n,std::filesystem::path(it->name).filename().c_str(),&now,AT_SYMLINK_NOFOLLOW)==0);need(directory_identity(it->stamp,now));io(::unlinkat(at.n,std::filesystem::path(it->name).filename().c_str(),S_ISDIR(now.st_mode)?AT_REMOVEDIR:0)==0);}
   root_binding();root.reset();io(::unlinkat(parent.fd(),leaf.c_str(),AT_REMOVEDIR)==0);sync(parent.fd());closed=true;return true;
  }catch(...){return false;}
 }
 ~State(){(void)cleanup();}
};
inline SealedReadonlyReplica::~SealedReadonlyReplica()=default;
inline SealedReadonlyReplica::SealedReadonlyReplica(std::unique_ptr<State> state):state_(std::move(state)){}
inline SealedReadonlyReplica::SealedReadonlyReplica(SealedReadonlyReplica&&)noexcept=default;
inline SealedReadonlyReplica SealedReadonlyReplica::capture(const std::filesystem::path& source,const std::filesystem::path& output,uint64_t maximum_entries,uint64_t maximum_bytes,std::function<void()> checkpoint){
 using namespace readonly_replica_detail;need(maximum_entries&&maximum_bytes);
 auto inside=[](const std::filesystem::path& a,const std::filesystem::path& b){auto i=a.begin(),j=b.begin();for(;i!=a.end()&&j!=b.end()&&*i==*j;++i,++j){}return i==a.end();};
 need(!inside(source,output)&&!inside(output,source));auto state=std::make_unique<State>(source,output,maximum_entries,maximum_bytes,checkpoint);auto same_inode=[](const struct stat& a,const struct stat& b){return a.st_dev==b.st_dev&&a.st_ino==b.st_ino;};for(const auto& ancestor:state->source.stamps)need(!same_inode(ancestor,state->parent.stamps.back()));for(const auto& ancestor:state->parent.stamps)need(!same_inode(ancestor,state->source.stamps.back()));state->source_entries=readonly_replica_detail::capture(state->source.fd(),maximum_entries,maximum_bytes,checkpoint);
 for(unsigned tries=0;;++tries){need(tries<16);std::array<unsigned char,16> random{};io(RAND_bytes(random.data(),int(random.size()))==1);state->leaf="readonly-replica-"+hexadecimal(random.data(),random.size());if(::mkdirat(state->parent.fd(),state->leaf.c_str(),0700)==0)break;io(errno==EEXIST);}
 state->path=state->parent.path/state->leaf;state->root=std::make_unique<Fd>(::openat(state->parent.fd(),state->leaf.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));io(::fstat(state->root->n,&state->created)==0);need((state->created.st_mode&0777)==0700&&state->created.st_uid==::geteuid());sync(state->parent.fd());state->root_binding();
 for(size_t i=1;i<state->source_entries.size();++i){const auto& entry=state->source_entries[i];check(checkpoint);auto parent=relative_parent(state->root->n,std::filesystem::path(entry.name));const auto name=std::filesystem::path(entry.name).filename().string();
  if(S_ISDIR(entry.stamp.st_mode)){io(::mkdirat(parent.n,name.c_str(),0700)==0);struct stat created{};io(::fstatat(parent.n,name.c_str(),&created,AT_SYMLINK_NOFOLLOW)==0);need((created.st_mode&0777)==0700);state->created_entries.push_back({entry.name,created,{}});continue;}
  auto src=relative_fd(state->source.fd(),std::filesystem::path(entry.name),O_RDONLY|O_NONBLOCK);struct stat source_before{};io(::fstat(src.n,&source_before)==0);need(stable(source_before,entry.stamp));Fd dst(::openat(parent.n,name.c_str(),O_RDWR|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600));struct stat created{};io(::fstat(dst.n,&created)==0);need(S_ISREG(created.st_mode)&&(created.st_mode&0777)==0600&&created.st_nlink==1&&created.st_uid==::geteuid());state->created_entries.push_back({entry.name,created,{}});
  Hash content;std::array<unsigned char,65536> buffer{};uint64_t at=0;
  while(at<uint64_t(entry.stamp.st_size)){check(checkpoint);const auto count=size_t(std::min<uint64_t>(buffer.size(),uint64_t(entry.stamp.st_size)-at));const auto n=::pread(src.n,buffer.data(),count,off_t(at));if(n<0&&errno==EINTR)continue;io(n>0);content.add(buffer.data(),size_t(n));size_t written=0;while(written<size_t(n)){check(checkpoint);const auto k=::write(dst.n,buffer.data()+written,size_t(n)-written);if(k<0&&errno==EINTR)continue;io(k>0);written+=size_t(k);}at+=uint64_t(n);}
  struct stat source_after{},copied{};io(::fstat(src.n,&source_after)==0);need(stable(source_before,source_after)&&content.finish()==entry.sha);sync(dst.n);io(::fstat(dst.n,&copied)==0);need(uint64_t(copied.st_size)==uint64_t(entry.stamp.st_size)&&file_hash(dst.n,copied,checkpoint)==entry.sha);state->created_entries.back().stamp=copied;
 }
 for(auto it=state->created_entries.rbegin();it!=state->created_entries.rend();++it)if(S_ISDIR(it->stamp.st_mode)){auto dir=relative_fd(state->root->n,std::filesystem::path(it->name),O_RDONLY|O_DIRECTORY);sync(dir.n);}sync(state->root->n);state->source.recheck();state->root_binding();
 need(same_entries(state->source_entries,readonly_replica_detail::capture(state->source.fd(),maximum_entries,maximum_bytes,checkpoint)));state->sealed_entries=readonly_replica_detail::capture(state->root->n,maximum_entries,maximum_bytes,checkpoint);need(same_contents(state->source_entries,state->sealed_entries));SealedReadonlyReplica result(std::move(state));result.recheck();return result;
}
inline void SealedReadonlyReplica::recheck()const{
 using namespace readonly_replica_detail;need(bool(state_)&&!state_->closed);state_->source.recheck();state_->root_binding();need(same_entries(state_->source_entries,readonly_replica_detail::capture(state_->source.fd(),state_->maximum_entries,state_->maximum_bytes,state_->checkpoint)));need(same_entries(state_->sealed_entries,readonly_replica_detail::capture(state_->root->n,state_->maximum_entries,state_->maximum_bytes,state_->checkpoint)));state_->source.recheck();state_->root_binding();
}
inline std::filesystem::path SealedReadonlyReplica::read_only_path()const{recheck();return state_->path;}
inline void SealedReadonlyReplica::close(){readonly_replica_detail::need(bool(state_));readonly_replica_detail::need(state_->cleanup());}
} // nano::maintenance
