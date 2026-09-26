#include <rocksdb/db.h>
#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>
#include <sys/stat.h>
#include <cstdlib>
namespace fs=std::filesystem;
int main(){try{char pattern[]="/tmp/nano-footprint-XXXXXX";const auto* created=mkdtemp(pattern);if(!created)throw std::runtime_error("mkdtemp");const fs::path base(created);for(int mode=0;mode<3;++mode){
 auto root=base/std::to_string(mode);
 fs::create_directories(root);
 rocksdb::Options o;o.create_if_missing=true;o.paranoid_checks=true;
 if(mode==1)o.allow_fallocate=false;
 if(mode==2)o.write_buffer_size=4*1024*1024;
 std::vector<std::unique_ptr<rocksdb::DB>> dbs;
 for(int i=0;i<66;++i){rocksdb::DB* p=nullptr;auto s=rocksdb::DB::Open(o,(root/std::to_string(i)).string(),&p);if(!s.ok())throw std::runtime_error(s.ToString());dbs.emplace_back(p);rocksdb::WriteOptions w;w.sync=true;s=p->Put(w,"binding",std::string(100,'x'));if(!s.ok())throw std::runtime_error(s.ToString());}
 uint64_t allocated=0,logical=0,wal=0;
 for(auto& e:fs::recursive_directory_iterator(root)){if(!e.is_regular_file())continue;struct stat st{};if(stat(e.path().c_str(),&st))throw std::runtime_error("stat");allocated+=uint64_t(st.st_blocks)*512;logical+=st.st_size;if(e.path().extension()==".log")wal+=uint64_t(st.st_blocks)*512;}
 std::cout<<"{\"mode\":"<<mode<<",\"databases\":66,\"logicalBytes\":"<<logical<<",\"allocatedBytes\":"<<allocated<<",\"walAllocatedBytes\":"<<wal<<"}"<<std::endl;
 dbs.clear();
 for(int i=0;i<66;++i){rocksdb::DB* p=nullptr;auto status=rocksdb::DB::Open(o,(root/std::to_string(i)).string(),&p);std::unique_ptr<rocksdb::DB> db(p);if(!status.ok())throw std::runtime_error(status.ToString());std::string value;status=p->Get(rocksdb::ReadOptions(),"binding",&value);if(!status.ok()||value!=std::string(100,'x'))throw std::runtime_error("reopen lost value");}
 fs::remove_all(root);
 }fs::remove_all(base);std::cout<<"{\"reopenedValuesVerified\":198}"<<std::endl;}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
