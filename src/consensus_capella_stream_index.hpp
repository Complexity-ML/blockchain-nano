#pragma once
// Private shared implementation: Core replay and its existing index self-tests.
#include "nano/consensus_capella_canonical_stream.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <climits>
#include <fcntl.h>
#include <limits>
#include <sys/stat.h>
#include <unistd.h>

namespace nano {
namespace canonical_stream_detail {
inline void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
template<class T>Bytes bytes(const T& value){return {value.begin(),value.end()};}
inline uint64_t little(std::span<const uint8_t> b,size_t at,size_t width=8){require(at<=b.size()&&width<=b.size()-at,"stream truncated integer");uint64_t n=0;for(size_t i=0;i<width;++i)n|=uint64_t(b[at+i])<<(8*i);return n;}
inline ConsensusRoot root(std::span<const uint8_t> b,size_t at){require(at<=b.size()&&32<=b.size()-at,"stream truncated root");ConsensusRoot out{};std::copy_n(b.begin()+at,32,out.begin());return out;}
inline bool same(const struct stat& a,const struct stat& b){
#ifdef __APPLE__
 const auto am=a.st_mtimespec,bm=b.st_mtimespec,ac=a.st_ctimespec,bc=b.st_ctimespec;
#else
 const auto am=a.st_mtim,bm=b.st_mtim,ac=a.st_ctim,bc=b.st_ctim;
#endif
 return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_mode==b.st_mode&&a.st_uid==b.st_uid&&a.st_gid==b.st_gid&&a.st_nlink==b.st_nlink&&a.st_size==b.st_size&&am.tv_sec==bm.tv_sec&&am.tv_nsec==bm.tv_nsec&&ac.tv_sec==bc.tv_sec&&ac.tv_nsec==bc.tv_nsec;
}
struct Fd{int fd=-1;explicit Fd(int f):fd(f){}Fd(const Fd&)=delete;~Fd(){if(fd>=0)::close(fd);}};
inline constexpr size_t record_bytes=canonical_stream_index_record_bytes;
using Record=std::array<uint8_t,record_bytes>;
inline void put(Record& r,size_t at,uint64_t n){for(size_t i=0;i<8;++i)r[at+i]=uint8_t(n>>(8*i));}
inline Record record(const ConsensusRoot& id,const CheckpointArchiveEntry& e){Record r{};std::copy(id.begin(),id.end(),r.begin());const auto parent=root(e.payload,116);std::copy(parent.begin(),parent.end(),r.begin()+32);put(r,64,e.slot);put(r,72,e.payload.size());const auto hash=sha256(e.payload);std::copy(hash.begin(),hash.end(),r.begin()+80);return r;}
inline Bytes roll(const Bytes& previous,const Record& row){Bytes b=previous;b.insert(b.end(),row.begin(),row.end());return sha256(b);}

// Write only while NEW. One successful seal freezes known ordered bytes/size
// AND a full-nine-field FD/path authority; it is never refreshed or resealed.
// No raw wire enters the index. Caller uses private OS tmp/owned-volume output.
class Index {
 Fd parent_,file_;std::filesystem::path path_;struct stat identity_{},sealed_material_{};bool seal_attempted_=false,sealed_=false;
 uint64_t count_=0,maximum_;Bytes commitment_=Bytes(32,0);
 void identity(const struct stat& s)const{require(S_ISREG(s.st_mode)&&s.st_nlink==1&&s.st_uid==geteuid()&&(s.st_mode&07777)==0600&&s.st_dev==identity_.st_dev&&s.st_ino==identity_.st_ino&&s.st_gid==identity_.st_gid&&s.st_size>=0&&uint64_t(s.st_size)==count_*record_bytes,"stream private index identity/size changed");}
 void sealed_fence()const{require(sealed_&&std::filesystem::canonical(path_.parent_path())==path_.parent_path(),"stream sealed index parent/authority required");struct stat a{},b{};require(::fstat(file_.fd,&a)==0&&::lstat(path_.c_str(),&b)==0&&same(a,b)&&same(a,sealed_material_),"stream sealed index material changed");identity(a);}
public:
 Index(const std::filesystem::path& p,uint64_t maximum):parent_(-1),file_(-1),path_(p),maximum_(maximum){
  require(p.is_absolute()&&p.lexically_normal()==p&&p.filename()!="."&&p.filename()!=".."&&!p.filename().empty(),"stream canonical new index required");
  const auto dir=p.parent_path();require(std::filesystem::canonical(dir)==dir,"stream linked index parent refused");
  parent_.fd=::open(dir.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);require(parent_.fd>=0,"stream private index parent open");struct stat d{},path{};require(::fstat(parent_.fd,&d)==0&&::lstat(dir.c_str(),&path)==0&&same(d,path)&&S_ISDIR(d.st_mode)&&d.st_uid==geteuid()&&(d.st_mode&07777)==0700,"stream owner0700 index parent required");
  file_.fd=::openat(parent_.fd,p.filename().c_str(),O_RDWR|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);require(file_.fd>=0,"stream exclusive index create");
  require(::fstat(file_.fd,&identity_)==0&&::fstatat(parent_.fd,p.filename().c_str(),&path,AT_SYMLINK_NOFOLLOW)==0&&same(identity_,path),"stream private index creation identity");identity(identity_);require(::fsync(parent_.fd)==0,"stream index directory fsync");
 }
 void append(const Record& r,const std::function<void()>& check){
  require(!seal_attempted_,"stream index append after seal refused");require(count_<=maximum_/record_bytes&&record_bytes<=maximum_-count_*record_bytes,"stream private index byte bound");size_t at=0;while(at<r.size()){check();const auto n=::write(file_.fd,r.data()+at,r.size()-at);if(n<0&&errno==EINTR)continue;require(n>0,"stream private index write");at+=size_t(n);}++count_;commitment_=roll(commitment_,r);
 }
 void seal(){require(!seal_attempted_,"stream index reseal refused");seal_attempted_=true;require(::fsync(file_.fd)==0,"stream private index fsync");struct stat before{},after{},path{};require(::fstat(file_.fd,&before)==0&&::lstat(path_.c_str(),&path)==0&&same(before,path),"stream index seal FD/path differs");identity(before);require(::fstat(file_.fd,&after)==0&&::lstat(path_.c_str(),&path)==0&&same(before,after)&&same(after,path),"stream index seal material unstable");sealed_material_=after;sealed_=true;}
 Record read(uint64_t ordinal,const std::function<void()>& check)const{
  check();require(sealed_&&ordinal<count_&&ordinal<=uint64_t(std::numeric_limits<off_t>::max())/record_bytes,"stream sealed index read bound");require(std::filesystem::canonical(path_.parent_path())==path_.parent_path(),"stream linked index parent refused");struct stat before{},after{},path{};require(::fstat(file_.fd,&before)==0&&::lstat(path_.c_str(),&path)==0&&same(before,path)&&same(before,sealed_material_),"stream sealed index material changed");identity(before);
  Record r{};size_t at=0;while(at<r.size()){check();const auto n=::pread(file_.fd,r.data()+at,r.size()-at,off_t(ordinal*record_bytes+at));if(n<0&&errno==EINTR)continue;require(n>0,"stream index short read");at+=size_t(n);}
  require(::fstat(file_.fd,&after)==0&&::lstat(path_.c_str(),&path)==0&&same(before,after)&&same(after,path)&&same(after,sealed_material_),"stream sealed index changed during FD read");return r;
 }
 void verify(const std::function<void()>& check)const{check();sealed_fence();Bytes digest(32,0);for(uint64_t i=0;i<count_;++i)digest=roll(digest,read(i,check));require(digest==commitment_,"stream known private index bytes changed");check();sealed_fence();}
 uint64_t count()const{return count_;}
};
}
}
