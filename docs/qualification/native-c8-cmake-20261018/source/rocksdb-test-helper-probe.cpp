#include "tests/support/rocksdb.hpp"
#include <cerrno>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace {
namespace fs=std::filesystem;
using Clock=std::chrono::steady_clock;
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
struct OwnedRoot {
 fs::path path; struct stat identity{}; bool created=false,identified=false,removed=false;
 void create(const fs::path& requested){
  need(requested.is_absolute()&&requested.lexically_normal()==requested,"absolute normalized root required");
  const auto name=requested.filename().string(),prefix=std::string("synthetic-rocksdb-helper-");
  need(name.starts_with(prefix)&&name.size()>prefix.size()&&name.size()<=96,"synthetic root basename required");
  for(const unsigned char c:name)need((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_',"plain synthetic root basename required");
  need(fs::canonical(requested.parent_path())==requested.parent_path(),"physical existing parent required");
  struct stat before{};errno=0;need(::lstat(requested.c_str(),&before)!=0&&errno==ENOENT,"root must be absent");
  path=requested;need(::mkdir(path.c_str(),0700)==0,"exclusive root mkdir failed");created=true;
  need(::lstat(path.c_str(),&identity)==0,"created root stat failed");identified=true;
  need(S_ISDIR(identity.st_mode)&&(identity.st_mode&07777)==0700&&identity.st_uid==::geteuid()&&identity.st_gid==::getegid(),"created root identity/mode/owner invalid");
 }
 void cleanup(){
  if(!created||removed)return;
  struct stat now{};need(identified&&::lstat(path.c_str(),&now)==0,"owned root identity unavailable");
  need(fs::canonical(path.parent_path())==path.parent_path()&&S_ISDIR(now.st_mode)&&now.st_dev==identity.st_dev&&now.st_ino==identity.st_ino&&(now.st_mode&07777)==0700&&now.st_uid==identity.st_uid&&now.st_gid==identity.st_gid,"owned root changed before cleanup");
  fs::remove_all(path);errno=0;need(::lstat(path.c_str(),&now)!=0&&errno==ENOENT,"owned root cleanup incomplete");removed=true;
 }
 ~OwnedRoot(){if(created&&!removed)try{cleanup();}catch(...){} }
};
struct Counts {int open_attempts=0,opens=0,refusals=0,puts=0,gets=0,closed=0;bool failure_null=false,reopened=false,missing_current=false;};
}
int main(int argc,char** argv){
 const auto began=Clock::now();OwnedRoot root;Counts c;std::string first;
 const auto elapsed=[&](){return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-began).count();};
 const auto budget=[&](){need(elapsed()<30000,"probe deadline exceeded");};
 try{
  need(argc==2,"usage: rocksdb-test-helper-probe ABSENT_SYNTHETIC_ROOT");root.create(argv[1]);budget();
  const auto database=(root.path/"database").string();
  const std::string key("fixture-key"),value("fixture-value");
  {
   rocksdb::Options options;options.create_if_missing=true;options.error_if_exists=true;
   std::unique_ptr<rocksdb::DB> db;++c.open_attempts;
   const auto opened=test::open_rocksdb(options,database,db);need(opened.ok()&&db!=nullptr,"fresh RW create refused");++c.opens;budget();
   rocksdb::WriteOptions durable;durable.sync=true;need(db->Put(durable,key,value).ok(),"synthetic Put refused");++c.puts;budget();
   std::string found;need(db->Get(rocksdb::ReadOptions(),key,&found).ok()&&found==value,"synthetic Get differs");++c.gets;
   db.reset();++c.closed;budget();
  }
  {
   rocksdb::Options options;options.create_if_missing=false;
   std::unique_ptr<rocksdb::DB> db;++c.open_attempts;
   const auto opened=test::open_rocksdb(options,database,db);need(opened.ok()&&db!=nullptr,"existing RW reopen refused");++c.opens;budget();
   std::string found;need(db->Get(rocksdb::ReadOptions(),key,&found).ok()&&found==value,"reopened Get differs");++c.gets;c.reopened=true;
   db.reset();++c.closed;budget();
  }
  {
   const auto missing=root.path/"missing";need(!fs::exists(missing),"missing case was not initially absent");
   rocksdb::Options options;options.create_if_missing=false;
   std::unique_ptr<rocksdb::DB> db;++c.open_attempts;
   const auto refused=test::open_rocksdb(options,missing.string(),db);
   need(!refused.ok(),"missing DB unexpectedly opened");++c.refusals;c.failure_null=db==nullptr;
   need(c.failure_null,"failed Open retained an owner");c.missing_current=!fs::exists(missing/"CURRENT");
   need(c.missing_current,"failed Open initialized a CURRENT file");budget();
  }
  need(c.open_attempts==3&&c.opens==2&&c.refusals==1&&c.puts==1&&c.gets==2&&c.closed==2&&c.reopened,"incomplete source-defined operation counts");
 }catch(const std::exception& error){first=error.what();}catch(...){first="non-standard probe exception";}
 try{root.cleanup();}catch(const std::exception& error){if(first.empty())first=error.what();else std::cerr<<"Secondary cleanup failure: "<<std::string(error.what()).substr(0,512)<<'\n';}catch(...){if(first.empty())first="non-standard cleanup exception";else std::cerr<<"Secondary cleanup failure\n";}
 const auto milliseconds=elapsed();if(first.empty()&&milliseconds>=30000)first="probe deadline exceeded during closure";
 const bool passed=first.empty()&&root.removed;
 std::cout<<"{\"scenario\":\"rocksdb-test-helper-api\",\"result\":\""<<(passed?"PASS":"FAIL")
  <<"\",\"openAttempts\":"<<c.open_attempts<<",\"successfulOpens\":"<<c.opens<<",\"expectedOpenRefusals\":"<<c.refusals
  <<",\"successfulPuts\":"<<c.puts<<",\"successfulGets\":"<<c.gets<<",\"ownersClosed\":"<<c.closed
  <<",\"failureOwnerNull\":"<<(c.failure_null?"true":"false")<<",\"reopenedValueExact\":"<<(c.reopened?"true":"false")
  <<",\"missingCurrentAbsent\":"<<(c.missing_current?"true":"false")<<",\"ownedRootRemoved\":"<<(root.removed?"true":"false")
  <<",\"readOnlyOpens\":0,\"newSignaturesCreated\":0,\"networkStarted\":false,\"oldDataOpened\":false,\"productionQualified\":false,\"elapsedMs\":"<<milliseconds<<"}\n";
 if(!passed)std::cerr<<"RocksDB helper probe refused: "<<first.substr(0,512)<<'\n';
 return passed?0:1;
}
