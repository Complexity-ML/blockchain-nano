#pragma once
#include <rocksdb/db.h>
#include <memory>
#include <string>

namespace test {
// Detect the installed Open signature; keep fixture RAII ownership on success
// and failure without changing runtime storage or any test assertion.
template<class Database=rocksdb::DB>
inline rocksdb::Status open_rocksdb(const rocksdb::Options& options,
                                   const std::string& path,
                                   std::unique_ptr<Database>& owner) {
 if constexpr(requires { Database::Open(options,path,&owner); }){
  return Database::Open(options,path,&owner);
 }else{
  Database* raw=nullptr;
  auto status=Database::Open(options,path,&raw);
  owner.reset(raw);
  return status;
 }
}
}
