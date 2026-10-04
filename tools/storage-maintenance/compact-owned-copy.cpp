// Qualification helper: only an explicitly marked, offline owned chain copy.
#include <rocksdb/db.h>
#include <rocksdb/table.h>
#include <rocksdb/version.h>
#include <openssl/evp.h>
#include "json.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <limits>
#include <cstdarg>
#include <sys/stat.h>
using Json=nlohmann::json;namespace fs=std::filesystem;
class SilentLogger final:public rocksdb::Logger{public:void Logv(const char*,va_list)override{}};
void check(const rocksdb::Status& s){if(!s.ok())throw std::runtime_error(s.ToString());}
void add(uint64_t& a,uint64_t b){if(b>std::numeric_limits<uint64_t>::max()-a)throw std::overflow_error("accounting overflow");a+=b;}
Json files(const fs::path& p){uint64_t apparent=0,allocated=0,sst=0,wal=0,count=0;for(const auto& e:fs::recursive_directory_iterator(p)){if(e.is_symlink())throw std::invalid_argument("Symlinks refused");if(!e.is_regular_file())continue;struct stat st{};if(::stat(e.path().c_str(),&st)||st.st_blocks<0)throw std::runtime_error("File stat failed");const auto bytes=e.file_size();add(apparent,bytes);add(allocated,uint64_t(st.st_blocks)*512);add(count,1);if(e.path().extension()==".sst")add(sst,bytes);if(e.path().extension()==".log")add(wal,bytes);}return {{"files",count},{"apparentBytes",apparent},{"allocatedBytes",allocated},{"sstBytes",sst},{"walBytes",wal}};}
Json logical(rocksdb::DB& db){std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(),EVP_MD_CTX_free);if(!ctx||EVP_DigestInit_ex(ctx.get(),EVP_sha256(),nullptr)!=1)throw std::runtime_error("Digest init failed");auto update=[&](const void* p,size_t n){if(EVP_DigestUpdate(ctx.get(),p,n)!=1)throw std::runtime_error("Digest update failed");};auto framed=[&](rocksdb::Slice s){uint64_t n=s.size();unsigned char size[8];for(unsigned i=0;i<8;++i)size[7-i]=static_cast<unsigned char>(n>>(8*i));update(size,8);update(s.data(),s.size());};rocksdb::ReadOptions ro;ro.verify_checksums=true;ro.fill_cache=false;std::unique_ptr<rocksdb::Iterator> it(db.NewIterator(ro));uint64_t entries=0,bytes=0;for(it->SeekToFirst();it->Valid();it->Next()){framed(it->key());framed(it->value());add(entries,1);add(bytes,it->key().size());add(bytes,it->value().size());}check(it->status());unsigned char digest[EVP_MAX_MD_SIZE];unsigned n=0;if(EVP_DigestFinal_ex(ctx.get(),digest,&n)!=1||n!=32)throw std::runtime_error("Digest final failed");static constexpr char hex[]="0123456789abcdef";std::string value;for(unsigned i=0;i<n;++i){value+=hex[digest[i]>>4];value+=hex[digest[i]&15];}return {{"entries",entries},{"keyValueBytes",bytes},{"sha256FramedOrderedKeyValues",value}};}
int main(int argc,char** argv){try{
 if(argc!=3||std::string(argv[1])!="--offline-owned-copy")throw std::invalid_argument("Usage: compact-owned-copy --offline-owned-copy COPY/chain");
 fs::path input(argv[2]);for(auto p=input;!p.empty();p=p.parent_path()){if(fs::is_symlink(p))throw std::invalid_argument("Symlink path refused");if(p==p.parent_path())break;}
 const auto path=fs::canonical(input);if(path.filename()!="chain"||!fs::is_regular_file(path/"CURRENT"))throw std::invalid_argument("Owned chain copy required");std::ifstream marker(path.parent_path()/".nano-owned-compaction-copy");std::string flag;std::getline(marker,flag);if(flag!="owned-offline-qualification-copy-v1")throw std::invalid_argument("Explicit owned copy marker required");
 const auto beforeFiles=files(path);rocksdb::Options options;options.create_if_missing=false;options.paranoid_checks=true;options.allow_fallocate=false;options.info_log=std::make_shared<SilentLogger>();rocksdb::BlockBasedTableOptions tables;tables.format_version=6;options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(tables));rocksdb::DB* raw=nullptr;check(rocksdb::DB::Open(options,path.string(),&raw));std::unique_ptr<rocksdb::DB> db(raw);
 std::string schema;check(db->Get(rocksdb::ReadOptions(),"meta/schema",&schema));if(schema!="nano/1"&&schema!="nano/2-state-pruned")throw std::invalid_argument("Unknown Nano schema");const auto before=logical(*db);rocksdb::FlushOptions flush;flush.wait=true;check(db->Flush(flush));rocksdb::CompactRangeOptions compact;compact.exclusive_manual_compaction=true;check(db->CompactRange(compact,nullptr,nullptr));const auto after=logical(*db);if(before!=after)throw std::runtime_error("Compaction changed logical key/value content");db.reset();
 std::cout<<Json{{"schema",schema},{"rocksdbVersion",std::to_string(ROCKSDB_MAJOR)+"."+std::to_string(ROCKSDB_MINOR)+"."+std::to_string(ROCKSDB_PATCH)},{"ownedCopyOnly",true},{"beforeFiles",beforeFiles},{"afterFiles",files(path)},{"logicalBefore",before},{"logicalAfter",after},{"logicalContentUnchanged",true},{"physicalPowerLoss",false},{"productionMaintenanceQualified",false}}.dump(2)<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
