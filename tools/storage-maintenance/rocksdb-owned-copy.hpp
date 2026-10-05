#pragma once
#include <rocksdb/db.h>
#include <array>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
namespace nano::maintenance {
inline void require_ok(const rocksdb::Status& status){if(!status.ok())throw std::runtime_error(status.ToString());}
// API detection supports installed RocksDB versions without guessing a version boundary.
template<class Database=rocksdb::DB>
std::unique_ptr<Database> open_copy(const rocksdb::Options& options,const std::string& path){
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
 if(std::filesystem::is_symlink(marker)||!std::filesystem::is_regular_file(marker)||std::filesystem::file_size(marker)>96)throw std::invalid_argument("Bounded regular owned-copy marker required");
 std::ifstream input(marker,std::ios::binary);std::array<char,97> bytes{};input.read(bytes.data(),bytes.size());
 if(input.bad()||!input.eof()||input.gcount()>96)throw std::invalid_argument("Owned-copy marker read failed or grew");
 const std::string value(bytes.data(),size_t(input.gcount()));
 if(value!=expected&&value!=expected+"\n")throw std::invalid_argument("Explicit owned-copy marker required");
}
}
