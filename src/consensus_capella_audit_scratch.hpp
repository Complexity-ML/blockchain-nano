#pragma once
#include "nano/storage.hpp"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace nano::audit_detail {
inline bool audit_path_within(const std::filesystem::path& path,const std::filesystem::path& root){
 auto a=path.begin();for(auto b=root.begin();b!=root.end();++a,++b)if(a==path.end()||*a!=*b)return false;return true;
}
inline void require_audit_index_outside_database(const ChainStore& disk,const std::filesystem::path& index){
 if(!index.is_absolute()||index.lexically_normal()!=index||index.filename().empty()||index.filename()=="."||index.filename()=="..")throw std::invalid_argument("Capella audit NEW absolute index required");
 const auto database=std::filesystem::canonical(disk.directory());
 if(std::filesystem::canonical(index.parent_path())!=index.parent_path()||audit_path_within(index,database))throw std::invalid_argument("Capella audit private index must be outside database");
}
// Owns only a new owner0700 namespace. A nonempty namespace is retained, never
// deleted by name/mode: an O_EXCL refusal or later replacement is not ownership.
class TemporaryCanonicalAuditIndex {
 std::filesystem::path directory_,index_;int parent_=-1,directory_fd_=-1;struct stat identity_{};bool removed_=false,retention_reported_=false;
 static bool same_identity(const struct stat& a,const struct stat& b){return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_mode==b.st_mode&&a.st_uid==b.st_uid&&a.st_gid==b.st_gid;}
 bool cleanup_empty()noexcept{
  if(removed_)return true;if(directory_.empty())return true;
  struct stat a{},b{};if(parent_<0||directory_fd_<0||::fstat(directory_fd_,&a)!=0||::fstatat(parent_,directory_.filename().c_str(),&b,AT_SYMLINK_NOFOLLOW)!=0||!same_identity(a,identity_)||!same_identity(a,b)||!S_ISDIR(a.st_mode)||a.st_uid!=geteuid()||(a.st_mode&07777)!=0700)return false;
  // The kernel refuses every nonempty directory. No child is ever unlinked.
  if(::unlinkat(parent_,directory_.filename().c_str(),AT_REMOVEDIR)!=0)return false;
  removed_=true;return true;
 }
public:
 explicit TemporaryCanonicalAuditIndex(const ChainStore& disk){
  const auto base=std::filesystem::canonical(std::filesystem::temp_directory_path()),database=std::filesystem::canonical(disk.directory());
  if(audit_path_within(base,database))throw std::invalid_argument("Capella audit temporary parent is inside database");
  parent_=::open(base.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);if(parent_<0)throw std::runtime_error("Capella audit temporary parent open");
  try{
   auto pattern=(base/"nano-capella-canonical-audit-XXXXXX").string();std::vector<char> mutable_pattern(pattern.begin(),pattern.end());mutable_pattern.push_back(0);
   const auto created=::mkdtemp(mutable_pattern.data());if(!created)throw std::runtime_error("Capella audit private temporary directory create");directory_=created;index_=directory_/"canonical-index";
   directory_fd_=::open(directory_.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);struct stat named{};
   if(directory_fd_<0||::fstat(directory_fd_,&identity_)!=0||::fstatat(parent_,directory_.filename().c_str(),&named,AT_SYMLINK_NOFOLLOW)!=0||!same_identity(identity_,named)||!S_ISDIR(identity_.st_mode)||identity_.st_uid!=geteuid()||(identity_.st_mode&07777)!=0700)throw std::runtime_error("Capella audit private temporary identity");
   require_audit_index_outside_database(disk,index_);
  }catch(...){if(!cleanup_empty())std::fprintf(stderr,"Capella audit private scratch retained: %s\n",directory_.c_str());if(directory_fd_>=0)::close(directory_fd_);::close(parent_);throw;}
 }
 TemporaryCanonicalAuditIndex(const TemporaryCanonicalAuditIndex&)=delete;
 const std::filesystem::path& index()const{return index_;}
 // Call only after the returned audit JSON explicitly contains this locator.
 void retention_reported()noexcept{retention_reported_=true;}
 ~TemporaryCanonicalAuditIndex(){
  if(!cleanup_empty()&&!retention_reported_)std::fprintf(stderr,"Capella audit private scratch retained: %s\n",directory_.c_str());
  if(directory_fd_>=0)::close(directory_fd_);if(parent_>=0)::close(parent_);
 }
};
}
