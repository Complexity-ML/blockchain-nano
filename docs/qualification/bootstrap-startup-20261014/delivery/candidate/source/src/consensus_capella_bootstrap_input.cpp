#include "nano/consensus_capella_bootstrap_input.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <filesystem>
#include <fcntl.h>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>
namespace nano {
namespace {
namespace fs=std::filesystem;
void require(bool ok){if(!ok)throw std::invalid_argument("Invalid Capella bootstrap proof input");}
void io_require(bool ok){if(!ok)throw std::system_error(errno,std::generic_category(),"Capella bootstrap proof read");}
void check_now(const std::function<void()>& check){if(check)check();}
struct Fd {
 int value;
 explicit Fd(int fd):value(fd){io_require(value>=0);}
 ~Fd(){if(value>=0)::close(value);}
 Fd(const Fd&)=delete;Fd& operator=(const Fd&)=delete;
 Fd(Fd&& other)noexcept:value(other.value){other.value=-1;}
};
std::unique_ptr<Fd> own_fd(int value){Fd owned(value);return std::make_unique<Fd>(std::move(owned));}
bool identity(const struct stat& a,const struct stat& b){
 return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_uid==b.st_uid&&
  a.st_gid==b.st_gid&&a.st_mode==b.st_mode&&a.st_nlink==b.st_nlink;
}
bool same_file(const struct stat& a,const struct stat& b){
#ifdef __APPLE__
 const bool time=a.st_mtimespec.tv_sec==b.st_mtimespec.tv_sec&&a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec&&
  a.st_ctimespec.tv_sec==b.st_ctimespec.tv_sec&&a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
 const bool time=a.st_mtim.tv_sec==b.st_mtim.tv_sec&&a.st_mtim.tv_nsec==b.st_mtim.tv_nsec&&
  a.st_ctim.tv_sec==b.st_ctim.tv_sec&&a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
 return identity(a,b)&&a.st_size==b.st_size&&time;
}
void shape(const Json& value,std::initializer_list<const char*> keys){
 require(value.is_object()&&value.size()==keys.size());for(const auto* key:keys)require(value.contains(key));
}
uint64_t positive_size(const Json& value){
 require(value.is_number_integer());
 if(value.is_number_unsigned()){const auto n=value.get<uint64_t>();require(n>0);return n;}
 const auto n=value.get<int64_t>();require(n>0);return uint64_t(n);
}
struct Artifact {fs::path path;std::string sha;size_t bytes;};
Artifact artifact(const Json& value,size_t maximum){
 shape(value,{"path","sha256","bytes"});require(maximum>0&&value.at("path").is_string()&&value.at("sha256").is_string());
 const auto path=value.at("path").get<std::string>();const auto sha=value.at("sha256").get<std::string>();
 require(!path.empty()&&path.size()<=4096&&path.find('\0')==std::string::npos);
 const fs::path p(path);require(p.is_absolute()&&path==p.lexically_normal().string()&&!p.filename().empty()&&path[1]!='/');
 for(const auto& part:p.relative_path())require(!part.empty()&&part!="."&&part!="..");
 require(sha.size()==64&&std::all_of(sha.begin(),sha.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');}));
 const auto count=positive_size(value.at("bytes"));
 require(count<=maximum&&count<=uint64_t(std::numeric_limits<size_t>::max())&&count<=uint64_t(std::numeric_limits<off_t>::max()));
 return {p,sha,size_t(count)};
}
struct Directory {
 std::string name;std::unique_ptr<Fd> fd;struct stat observed{};
 Directory(std::string n,std::unique_ptr<Fd> owned):name(std::move(n)),fd(std::move(owned)){
  io_require(::fstat(fd->value,&observed)==0);require(S_ISDIR(observed.st_mode));
 }
};
class PublicFile {
 std::vector<std::unique_ptr<Directory>> parents_;
 std::string name_;std::unique_ptr<Fd> fd_;struct stat observed_{};
 void metadata()const{
  struct stat now{},entry{};
  for(size_t i=0;i<parents_.size();++i){
   io_require(::fstat(parents_[i]->fd->value,&now)==0);require(identity(parents_[i]->observed,now));
   if(i){io_require(::fstatat(parents_[i-1]->fd->value,parents_[i]->name.c_str(),&entry,AT_SYMLINK_NOFOLLOW)==0);
    require(identity(parents_[i]->observed,entry)&&S_ISDIR(entry.st_mode));}
  }
  io_require(::fstat(fd_->value,&now)==0);
  io_require(::fstatat(parents_.back()->fd->value,name_.c_str(),&entry,AT_SYMLINK_NOFOLLOW)==0);
  require(S_ISREG(now.st_mode)&&S_ISREG(entry.st_mode)&&same_file(observed_,now)&&same_file(observed_,entry));
 }
 size_t read_at(uint8_t* output,size_t count,size_t offset)const{
  ssize_t n;do{n=::pread(fd_->value,output,count,off_t(offset));}while(n<0&&errno==EINTR);
  io_require(n>=0);require(n>0&&size_t(n)<=count);return size_t(n);
 }
public:
 Bytes bytes;
 PublicFile(const Artifact& a,const std::function<void()>& check):name_(a.path.filename().string()){
  parents_.push_back(std::make_unique<Directory>("",own_fd(::open("/",O_RDONLY|O_DIRECTORY|O_CLOEXEC))));
  for(const auto& part:a.path.parent_path().relative_path()){
   check_now(check);const auto name=part.string();
   auto opened=own_fd(::openat(parents_.back()->fd->value,name.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));
   parents_.push_back(std::make_unique<Directory>(name,std::move(opened)));
  }
  fd_=own_fd(::openat(parents_.back()->fd->value,name_.c_str(),O_RDONLY|O_NOFOLLOW|O_CLOEXEC|O_NONBLOCK));
  io_require(::fstat(fd_->value,&observed_)==0);
  require(S_ISREG(observed_.st_mode)&&observed_.st_size>0&&uint64_t(observed_.st_size)==a.bytes);
  metadata();bytes.resize(a.bytes);
  size_t at=0;while(at<bytes.size()){check_now(check);at+=read_at(bytes.data()+at,std::min<size_t>(65536,bytes.size()-at),at);}
  metadata();check_now(check);require(hex(sha256(bytes)).substr(2)==a.sha);check_now(check);metadata();
 }
 void readback(const std::function<void()>& check)const{
  metadata();std::array<uint8_t,65536> chunk{};size_t at=0;
  while(at<bytes.size()){
   check_now(check);const auto n=read_at(chunk.data(),std::min(chunk.size(),bytes.size()-at),at);
   require(std::equal(chunk.begin(),chunk.begin()+n,bytes.begin()+at));at+=n;
  }
  // Exact positive length is bound by unchanged fstat/path size before/after.
  metadata();
 }
 void final_metadata()const{metadata();}
};
}
CapellaSignedBootstrapArchive load_capella_signed_bootstrap_proof(
 Genesis genesis,CapellaForkBlockRules rules,CapellaStoreSnapshotLimits limits,Json descriptor,
 const std::function<void()>& checkpoint){
 // Copy the callback too; moving the caller's callback during a callback cannot
 // invalidate this load. No callbacks borrow caller Genesis/rules/descriptor.
 const std::function<void()> check=checkpoint;
 shape(descriptor,{"format","genesisState","signedBlock"});
 require(descriptor.at("format").is_string()&&descriptor.at("format").get<std::string>()=="nano-capella-signed-bootstrap-proof-inputs-v1");
 require(limits.replay_slots>0&&limits.sync_trials>0);
 const auto state=artifact(descriptor.at("genesisState"),std::min({rules.transition.maximum_state_bytes,limits.state_bytes,limits.total_state_bytes}));
 const auto block=artifact(descriptor.at("signedBlock"),std::min(rules.transition.consensus.maximum_block_bytes,limits.envelope_bytes));
 check_now(check);PublicFile pre(state,check),wire(block,check);
 auto proof=prepare_capella_signed_bootstrap_archive(std::move(genesis),{pre.bytes,std::nullopt},wire.bytes,
  std::move(rules),limits.replay_slots,limits.sync_trials,check);
 require(bool(proof));
 pre.readback(check);wire.readback(check);check_now(check);
 pre.final_metadata();wire.final_metadata();return std::move(*proof);
}
}
