#pragma once
#include <rocksdb/db.h>
#include <memory>
#include <stdexcept>
namespace test {
// Fixture ownership only; supports both installed RocksDB Open signatures.
template<class Database=rocksdb::DB>
std::unique_ptr<Database> open_owned_fixture(const rocksdb::Options& options,const std::string& path){
 std::unique_ptr<Database> opened;rocksdb::Status status;
 if constexpr(requires { Database::Open(options,path,&opened); })status=Database::Open(options,path,&opened);
 else{Database* raw=nullptr;status=Database::Open(options,path,&raw);opened.reset(raw);}
 if(!status.ok())throw std::runtime_error("open owned legacy-format fixture: "+status.ToString());
 return opened;
}
}
