// Offline accounting only: emits no keys/values and never opens a write handle.
#include <rocksdb/db.h>
#include <rocksdb/env.h>
#include <rocksdb/version.h>
#include "json.hpp"
#include <filesystem>
#include <iostream>
#include <memory>
#include <limits>
#include <cstdarg>
#include <map>
using Json=nlohmann::json;
class SilentLogger final:public rocksdb::Logger{public:void Logv(const char*,va_list)override{}};
void check(const rocksdb::Status& s){if(!s.ok())throw std::runtime_error(s.ToString());}
void add(uint64_t& a,uint64_t b){if(b>std::numeric_limits<uint64_t>::max()-a)throw std::overflow_error("accounting overflow");a+=b;}
template<class DB=rocksdb::DB>std::unique_ptr<DB> open(const std::filesystem::path& p){
 rocksdb::Options o;o.create_if_missing=false;o.paranoid_checks=true;o.info_log=std::make_shared<SilentLogger>();
 std::unique_ptr<DB> db;
 if constexpr(requires{DB::OpenForReadOnly(o,p.string(),&db);})check(DB::OpenForReadOnly(o,p.string(),&db));
 else{DB* raw=nullptr;auto s=DB::OpenForReadOnly(o,p.string(),&raw);db.reset(raw);check(s);}
 return db;
}
std::string category(std::string_view key){
 for(auto p:{"state/","block/","checkpoint-archive/v1/","checkpoint-archive-slot/v1/","fork-checkpoint/v1/","execution-checkpoint/","receipt/","tx/","hash/","meta/","consensus/","beacon/","signing/","recovery/"})if(key.starts_with(p))return p;
 return "other";
}
int main(int argc,char** argv){try{
 if(argc!=2)throw std::invalid_argument("Usage: storage-profile OFFLINE-NODE-DATA");
 if(std::filesystem::is_symlink(argv[1]))throw std::invalid_argument("Symlink root refused");
 auto root=std::filesystem::canonical(argv[1]);if(!std::filesystem::is_directory(root))throw std::invalid_argument("Node data directory required");
 Json result={{"schema",1},{"readOnly",true},{"pruningPerformed",false},{"logicalBytesAreCompressedDiskBytes",false},{"rocksdbVersion",std::to_string(ROCKSDB_MAJOR)+"."+std::to_string(ROCKSDB_MINOR)+"."+std::to_string(ROCKSDB_PATCH)},{"databases",Json::array()}};
 uint64_t totalLogical=0,totalFiles=0;
 for(const auto& entry:std::filesystem::directory_iterator(root)){
  if(entry.is_symlink())throw std::invalid_argument("Symlinks refused");
  if(!entry.is_directory()||!std::filesystem::is_regular_file(entry.path()/"CURRENT"))continue;
  for(const auto& f:std::filesystem::recursive_directory_iterator(entry.path()))if(f.is_symlink())throw std::invalid_argument("Symlinks refused");
  auto db=open(entry.path());std::string schema;check(db->Get(rocksdb::ReadOptions(),"meta/schema",&schema));if(schema!="nano/1")throw std::invalid_argument("Unknown Nano schema");
  struct Stats{uint64_t entries=0,keys=0,values=0,largest=0;};std::map<std::string,Stats> stats;
  rocksdb::ReadOptions ro;ro.verify_checksums=true;ro.fill_cache=false;
  std::unique_ptr<rocksdb::Iterator> it(db->NewIterator(ro));
  for(it->SeekToFirst();it->Valid();it->Next()){
   auto key=it->key(),value=it->value();auto& s=stats[category({key.data(),key.size()})];add(s.entries,1);add(s.keys,key.size());add(s.values,value.size());s.largest=std::max(s.largest,uint64_t(value.size()));add(totalLogical,key.size());add(totalLogical,value.size());
  }check(it->status());
  uint64_t files=0;for(const auto& f:std::filesystem::recursive_directory_iterator(entry.path())){
   if(f.is_symlink())throw std::invalid_argument("Symlinks refused");if(f.is_regular_file())add(files,f.file_size());
  }add(totalFiles,files);
  Json groups=Json::object();for(const auto& [k,s]:stats)groups[k]={{"entries",s.entries},{"keyBytes",s.keys},{"valueBytes",s.values},{"largestValueBytes",s.largest}};
  result["databases"].push_back({{"directory",entry.path().filename().string()},{"apparentFileBytes",files},{"categories",groups}});
 }
 if(result["databases"].empty())throw std::invalid_argument("No Nano databases found");
 result["totalLogicalKeyValueBytes"]=totalLogical;result["totalApparentDatabaseFileBytes"]=totalFiles;
 std::cout<<result.dump(2)<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
