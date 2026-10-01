#pragma once
#include <rocksdb/db.h>
#include <memory>
#include <string>

namespace test {
// Use the DB** API shared by supported RocksDB versions. The fixture retains
// RAII ownership on both success and failure; no runtime storage code changes.
inline rocksdb::Status open_rocksdb(const rocksdb::Options& options,
                                   const std::string& path,
                                   std::unique_ptr<rocksdb::DB>& owner) {
 rocksdb::DB* raw=nullptr;
 auto status=rocksdb::DB::Open(options,path,&raw);
 owner.reset(raw);
 return status;
}
}
