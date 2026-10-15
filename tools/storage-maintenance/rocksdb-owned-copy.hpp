#pragma once
#include <rocksdb/db.h>
#include <array>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <cerrno>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
namespace nano::maintenance {
inline void require_ok(const rocksdb::Status& status){if(!status.ok())throw std::runtime_error(status.ToString());}
namespace owned_copy_detail {
struct Fd {
 int value=-1;
 explicit Fd(int fd):value(fd){if(fd<0)throw std::runtime_error("Owned copy physical admission I/O failed");}
 ~Fd(){if(value>=0)::close(value);}
 Fd(const Fd&)=delete;Fd& operator=(const Fd&)=delete;
 Fd(Fd&& other)noexcept:value(other.release()){}
 int release(){const int result=value;value=-1;return result;}
};
inline void io(bool ok){if(!ok)throw std::runtime_error("Owned copy physical admission I/O failed");}
inline bool same(const struct stat& a,const struct stat& b){
 if(a.st_dev!=b.st_dev||a.st_ino!=b.st_ino||a.st_uid!=b.st_uid||a.st_gid!=b.st_gid||a.st_mode!=b.st_mode||a.st_nlink!=b.st_nlink||a.st_size!=b.st_size)return false;
#ifdef __APPLE__
 return a.st_mtimespec.tv_sec==b.st_mtimespec.tv_sec&&a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec&&a.st_ctimespec.tv_sec==b.st_ctimespec.tv_sec&&a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
 return a.st_mtim.tv_sec==b.st_mtim.tv_sec&&a.st_mtim.tv_nsec==b.st_mtim.tv_nsec&&a.st_ctim.tv_sec==b.st_ctim.tv_sec&&a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
}
inline void stable(bool ok){if(!ok)throw std::invalid_argument("Owned copy physical admission changed");}
struct Directory {
 std::unique_ptr<DIR,decltype(&::closedir)> stream{nullptr,&::closedir};
 struct stat initial{};int parent=-1;std::string name;
 Directory(Fd owned,int parent_fd,std::string leaf):parent(parent_fd),name(std::move(leaf)){
  io(::fstat(owned.value,&initial)==0);stable(S_ISDIR(initial.st_mode));
  auto* raw=::fdopendir(owned.value);io(raw!=nullptr);owned.release();stream.reset(raw);
 }
 Directory(Directory&&)=default;Directory& operator=(Directory&&)=default;
 Directory(const Directory&)=delete;Directory& operator=(const Directory&)=delete;
 int fd()const{return ::dirfd(stream.get());}
};
}
// A stopped copy must have independent regular files before a writable SDK
// open. This is an admission check, not a filesystem lock: the operator must
// keep its namespace quiescent, including against other processes of this UID,
// through DB destruction. No RocksDB Env or race-proof ownership is implied.
inline void require_independent_copy_files(const std::string& path){
 using namespace owned_copy_detail;
 const int root=::open(path.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);
 // Preserve the generic wrapper's create_if_missing behavior. The maintenance
 // CLIs separately require an existing CURRENT and their explicit marker.
 if(root<0){if(errno==ENOENT)return;throw std::invalid_argument("Owned copy directory admission refused");}
 Fd owned_root(root);std::vector<Directory> pending;pending.emplace_back(std::move(owned_root),-1,std::string{});
 while(!pending.empty()){
  auto& current=pending.back();errno=0;auto* entry=::readdir(current.stream.get());
  if(!entry){
   io(errno==0);struct stat held{},linked{};io(::fstat(current.fd(),&held)==0);
   if(current.parent<0)io(::lstat(path.c_str(),&linked)==0);
   else io(::fstatat(current.parent,current.name.c_str(),&linked,AT_SYMLINK_NOFOLLOW)==0);
   stable(same(current.initial,held)&&same(held,linked));pending.pop_back();continue;
  }
  const std::string name(entry->d_name);if(name=="."||name=="..")continue;
  const int parent=current.fd();struct stat before{};io(::fstatat(parent,name.c_str(),&before,AT_SYMLINK_NOFOLLOW)==0);
  if(!S_ISREG(before.st_mode)&&!S_ISDIR(before.st_mode))throw std::invalid_argument("Owned copy symlink or special entry refused");
  if(S_ISREG(before.st_mode)&&before.st_nlink!=1)throw std::invalid_argument("Owned copy hard-linked database file refused");
  Fd leaf(::openat(parent,name.c_str(),O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC|(S_ISDIR(before.st_mode)?O_DIRECTORY:0)));
  struct stat held{},linked{};io(::fstat(leaf.value,&held)==0&&::fstatat(parent,name.c_str(),&linked,AT_SYMLINK_NOFOLLOW)==0);stable(same(before,held)&&same(held,linked));
  if(S_ISDIR(held.st_mode))pending.emplace_back(std::move(leaf),parent,name);
 }
}
// API detection supports installed RocksDB versions without guessing a version boundary.
template<class Database=rocksdb::DB>
std::unique_ptr<Database> open_copy(const rocksdb::Options& options,const std::string& path){
 require_independent_copy_files(path);
 std::unique_ptr<Database> opened;
 if constexpr(requires { Database::Open(options,path,&opened); })require_ok(Database::Open(options,path,&opened));
 else{Database* raw=nullptr;auto status=Database::Open(options,path,&raw);opened.reset(raw);require_ok(status);}
 return opened;
}
template<class Database=rocksdb::DB>
std::unique_ptr<Database> open_read_only_copy(const rocksdb::Options& options,const std::string& path){
 std::unique_ptr<Database> opened;
 if constexpr(requires { Database::OpenForReadOnly(options,path,&opened); })require_ok(Database::OpenForReadOnly(options,path,&opened));
 else{Database* raw=nullptr;auto status=Database::OpenForReadOnly(options,path,&raw);opened.reset(raw);require_ok(status);}
 return opened;
}
inline void require_owned_marker(const std::filesystem::path& marker,const std::string& expected){
 const int raw=::open(marker.c_str(),O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);
 if(raw<0)throw std::invalid_argument("Bounded regular owned-copy marker required");
 owned_copy_detail::Fd input(raw);struct stat before{};owned_copy_detail::io(::fstat(input.value,&before)==0);
 if(!S_ISREG(before.st_mode)||before.st_size<0||before.st_size>96)throw std::invalid_argument("Bounded regular owned-copy marker required");
 if(before.st_nlink!=1)throw std::invalid_argument("Owned-copy marker hard link refused");
 std::array<char,97> bytes{};size_t count=0;
 for(;;){const auto n=::read(input.value,bytes.data()+count,bytes.size()-count);if(n<0&&errno==EINTR)continue;if(n<0)throw std::invalid_argument("Owned-copy marker read failed or grew");if(!n)break;count+=size_t(n);if(count>96)throw std::invalid_argument("Owned-copy marker read failed or grew");}
 struct stat held{},linked{};owned_copy_detail::io(::fstat(input.value,&held)==0&&::lstat(marker.c_str(),&linked)==0);owned_copy_detail::stable(owned_copy_detail::same(before,held)&&owned_copy_detail::same(held,linked));
 const std::string value(bytes.data(),count);
 if(value!=expected&&value!=expected+"\n")throw std::invalid_argument("Explicit owned-copy marker required");
}
}
