// Qualification helper: only an explicitly marked, offline owned chain copy.
#include <rocksdb/db.h>
#include "nano/checkpoint_blob_gc.hpp"
#include "rocksdb-owned-copy.hpp"
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
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <chrono>
#include <functional>
using Json=nlohmann::json;namespace fs=std::filesystem;
class SilentLogger final:public rocksdb::Logger{public:void Logv(const char*,va_list)override{}};
void check(const rocksdb::Status& s){if(!s.ok())throw std::runtime_error(s.ToString());}
void add(uint64_t& a,uint64_t b){if(b>std::numeric_limits<uint64_t>::max()-a)throw std::overflow_error("accounting overflow");a+=b;}
Json files(const fs::path& p,uint64_t maximum_files=UINT64_MAX,uint64_t maximum_bytes=UINT64_MAX,const std::function<void()>& checkpoint={}){
 uint64_t apparent=0,allocated=0,sst=0,wal=0,count=0,entries=0;
 for(const auto& e:fs::recursive_directory_iterator(p)){
  if(checkpoint)checkpoint();add(entries,1);if(entries>maximum_files)throw std::length_error("File statistics entry budget exceeded");
  struct stat st{};if(::lstat(e.path().c_str(),&st)||st.st_blocks<0||st.st_size<0)throw std::runtime_error("File stat failed");
  if(S_ISLNK(st.st_mode)||(!S_ISREG(st.st_mode)&&!S_ISDIR(st.st_mode)))throw std::invalid_argument("Statistics links/special files refused");if(!S_ISREG(st.st_mode))continue;
  const auto bytes=uint64_t(st.st_size);add(apparent,bytes);if(apparent>maximum_bytes)throw std::length_error("File statistics byte budget exceeded");
  add(allocated,uint64_t(st.st_blocks)*512);add(count,1);if(e.path().extension()==".sst")add(sst,bytes);if(e.path().extension()==".log")add(wal,bytes);
 }if(checkpoint)checkpoint();return {{"files",count},{"apparentBytes",apparent},{"allocatedBytes",allocated},{"sstBytes",sst},{"walBytes",wal}};
}
Json logical(rocksdb::DB& db){std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(),EVP_MD_CTX_free);if(!ctx||EVP_DigestInit_ex(ctx.get(),EVP_sha256(),nullptr)!=1)throw std::runtime_error("Digest init failed");auto update=[&](const void* p,size_t n){if(EVP_DigestUpdate(ctx.get(),p,n)!=1)throw std::runtime_error("Digest update failed");};auto framed=[&](rocksdb::Slice s){uint64_t n=s.size();unsigned char size[8];for(unsigned i=0;i<8;++i)size[7-i]=static_cast<unsigned char>(n>>(8*i));update(size,8);update(s.data(),s.size());};rocksdb::ReadOptions ro;ro.verify_checksums=true;ro.fill_cache=false;std::unique_ptr<rocksdb::Iterator> it(db.NewIterator(ro));uint64_t entries=0,bytes=0;for(it->SeekToFirst();it->Valid();it->Next()){framed(it->key());framed(it->value());add(entries,1);add(bytes,it->key().size());add(bytes,it->value().size());}check(it->status());unsigned char digest[EVP_MAX_MD_SIZE];unsigned n=0;if(EVP_DigestFinal_ex(ctx.get(),digest,&n)!=1||n!=32)throw std::runtime_error("Digest final failed");static constexpr char hex[]="0123456789abcdef";std::string value;for(unsigned i=0;i<n;++i){value+=hex[digest[i]>>4];value+=hex[digest[i]&15];}return {{"entries",entries},{"keyValueBytes",bytes},{"sha256FramedOrderedKeyValues",value}};}
std::string bounded_public_file(const fs::path& path,size_t maximum){
 if(fs::is_symlink(path)||!fs::is_regular_file(path)||fs::file_size(path)>maximum)throw std::invalid_argument("Bounded regular public input required");
 std::ifstream f(path,std::ios::binary);std::string out;char block[4096];while(f){f.read(block,sizeof(block));out.append(block,size_t(f.gcount()));if(out.size()>maximum)throw std::length_error("Public input grew past budget");}if(f.bad())throw std::runtime_error("Public input read failed");return out;
}
nano::Bytes input_sha(const char* text){const std::string value(text);if(value.size()!=64||value.find_first_not_of("0123456789abcdef")!=std::string::npos)throw std::invalid_argument("Exact SHA-256 without 0x required");return nano::unhex("0x"+value);}
std::string pinned_public_file(const fs::path& path,const nano::Bytes& digest,size_t maximum){const auto bytes=bounded_public_file(path,maximum);if(nano::sha256(nano::Bytes(bytes.begin(),bytes.end()))!=digest)throw std::invalid_argument("Public input SHA mismatch before parse");return bytes;}
struct PublicReceiptDestination {
 fs::path path;int parent=-1;dev_t device;ino_t inode;
 PublicReceiptDestination(const fs::path& value,const fs::path& primary,const fs::path& witness):path(value){
  if(!path.is_absolute()||path!=path.lexically_normal()||path.filename().empty())throw std::invalid_argument("Canonical absolute independent receipt path required");
  auto parent_path=path.parent_path();if(fs::canonical(parent_path)!=parent_path||fs::exists(path)||fs::is_symlink(path))throw std::invalid_argument("Fresh independent receipt path required");
  auto contains=[](const fs::path& a,const fs::path& b){auto i=a.begin(),j=b.begin();for(;i!=a.end()&&j!=b.end()&&*i==*j;++i,++j){}return i==a.end();};
  if(contains(primary,path)||contains(witness,path))throw std::invalid_argument("Receipt must remain outside both maintained copies");
  parent=::open(parent_path.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);struct stat st{};
  if(parent<0||::fstat(parent,&st)||st.st_uid!=::geteuid()||(st.st_mode&077)!=0){if(parent>=0)::close(parent);throw std::invalid_argument("Private owned receipt parent required");}device=st.st_dev;inode=st.st_ino;
 }
 ~PublicReceiptDestination(){if(parent>=0)::close(parent);}
 void publish(const Json& result){
  struct stat current{};if(::stat(path.parent_path().c_str(),&current)||current.st_dev!=device||current.st_ino!=inode)throw std::runtime_error("Receipt parent replaced");
  const auto temporary=path.filename().string()+".gc-staged-"+std::to_string(::getpid());
  int file=::openat(parent,temporary.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);if(file<0)throw std::runtime_error("Exclusive receipt staging failed");
  bool linked=false;try{const auto bytes=result.dump(2)+"\n";size_t at=0;while(at<bytes.size()){auto n=::write(file,bytes.data()+at,bytes.size()-at);if(n<0&&errno==EINTR)continue;if(n<=0)throw std::runtime_error("Receipt write failed");at+=size_t(n);}if(::fsync(file))throw std::runtime_error("Receipt fsync failed");if(::close(file))throw std::runtime_error("Receipt close failed");file=-1;
   // Atomic no-overwrite publication on Darwin and Linux. The temp link is then
   // removed, so the complete final artifact has one link and private mode0600.
   if(::linkat(parent,temporary.c_str(),parent,path.filename().c_str(),0))throw std::runtime_error("Exclusive receipt publication failed");linked=true;if(::unlinkat(parent,temporary.c_str(),0)||::fsync(parent))throw std::runtime_error("Receipt parent durability failed");
  }catch(...){if(file>=0)::close(file);if(!linked)::unlinkat(parent,temporary.c_str(),0);throw;}
 }
};
int checkpoint_gc(int argc,char** argv){
 if(argc!=9)throw std::invalid_argument("Usage: compact-owned-copy --checkpoint-blob-gc-owned-copies REQUEST.json REQUEST-SHA STOP-RECEIPT.json STOP-SHA PROFILE.json PROFILE-SHA RECEIPT.json");
 const auto request_sha=input_sha(argv[3]),stop_sha=input_sha(argv[5]),profile_sha=input_sha(argv[7]);
 const auto request=pinned_public_file(argv[2],request_sha,65536),stop=pinned_public_file(argv[4],stop_sha,65536),profile=pinned_public_file(argv[6],profile_sha,1048576);
 const auto j=Json::parse(request);if(j.at("profileSha256")!=std::string(argv[7]))throw std::invalid_argument("Profile SHA differs from externally pinned request");
 const auto budget=nano::checkpoint_blob_gc_budget(j.at("budget"));
 const auto started=std::chrono::steady_clock::now();const std::function<void()> deadline=[&]{const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-started).count();if(elapsed<0||uint64_t(elapsed)>=budget.duration_ms)throw std::runtime_error("Maintenance total deadline exhausted");};
 const auto limits=nano::checkpoint_blob_gc_snapshot_limits(j.at("limits"));
 const auto rules=nano::load_capella_fork_profile(profile,profile_sha,1048576);
 const auto primary=fs::path(j.at("primary").at("path").get<std::string>()),witness=fs::path(j.at("witness").at("path").get<std::string>());
 const auto source_primary=fs::path(j.at("primary").at("sourcePath").get<std::string>()),source_witness=fs::path(j.at("witness").at("sourcePath").get<std::string>());
 PublicReceiptDestination source_receipt_guard(argv[8],source_primary,source_witness);
 PublicReceiptDestination receipt(argv[8],primary,witness);
 auto copies=nano::CheckpointBlobGcCopies::open(request,request_sha,stop,stop_sha,budget,deadline);
 const auto before=Json::array({files(primary,budget.inventory_files,budget.inventory_bytes,deadline),files(witness,budget.inventory_files,budget.inventory_bytes,deadline)});
 const auto plan=copies->plan(rules,limits,budget,deadline);const auto result=copies->collect(plan,deadline);copies.reset();
 const Json report{{"format","nano-owned-checkpoint-gc-maintenance-receipt-v1"},{"requestSha256",std::string(argv[3])},{"stopReceiptSha256",std::string(argv[5])},{"profileSha256",std::string(argv[7])},{"plan",plan.description()},{"result",result},{"beforeFiles",before},{"afterFiles",Json::array({files(primary,budget.inventory_files,budget.inventory_bytes,deadline),files(witness,budget.inventory_files,budget.inventory_bytes,deadline)})},{"receiptPersistedSeparately",true},{"requiresSuccessfulTerminalExit",true},{"atomicExclusiveLinkPublication",true}};
 deadline();receipt.publish(report);const auto receipt_bytes=report.dump(2)+"\n";std::cout<<Json{{"completed",true},{"receiptSha256",nano::hex(nano::sha256(nano::Bytes(receipt_bytes.begin(),receipt_bytes.end()))).substr(2)},{"productionQualified",false}}.dump()<<'\n';
 return 0;
}
int main(int argc,char** argv){try{
 if(argc>1&&std::string(argv[1])=="--checkpoint-blob-gc-owned-copies")return checkpoint_gc(argc,argv);

 if(argc!=3||std::string(argv[1])!="--offline-owned-copy")throw std::invalid_argument("Usage: compact-owned-copy --offline-owned-copy COPY/chain");
 fs::path input(argv[2]);for(auto p=input;!p.empty();p=p.parent_path()){if(fs::is_symlink(p))throw std::invalid_argument("Symlink path refused");if(p==p.parent_path())break;}
 const auto path=fs::canonical(input);if(path.filename()!="chain"||!fs::is_regular_file(path/"CURRENT"))throw std::invalid_argument("Owned chain copy required");nano::maintenance::require_owned_marker(path.parent_path()/".nano-owned-compaction-copy","owned-offline-qualification-copy-v1");
 const auto beforeFiles=files(path);rocksdb::Options options;options.create_if_missing=false;options.paranoid_checks=true;options.allow_fallocate=false;options.info_log=std::make_shared<SilentLogger>();rocksdb::BlockBasedTableOptions tables;tables.format_version=6;options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(tables));auto db=nano::maintenance::open_copy(options,path.string());
 std::string schema;check(db->Get(rocksdb::ReadOptions(),"meta/schema",&schema));if(schema!="nano/1"&&schema!="nano/2-state-pruned")throw std::invalid_argument("Unknown Nano schema");const auto before=logical(*db);rocksdb::FlushOptions flush;flush.wait=true;check(db->Flush(flush));rocksdb::CompactRangeOptions compact;compact.exclusive_manual_compaction=true;check(db->CompactRange(compact,nullptr,nullptr));const auto after=logical(*db);if(before!=after)throw std::runtime_error("Compaction changed logical key/value content");db.reset();
 std::cout<<Json{{"schema",schema},{"rocksdbVersion",std::to_string(ROCKSDB_MAJOR)+"."+std::to_string(ROCKSDB_MINOR)+"."+std::to_string(ROCKSDB_PATCH)},{"ownedCopyOnly",true},{"beforeFiles",beforeFiles},{"afterFiles",files(path)},{"logicalBefore",before},{"logicalAfter",after},{"logicalContentUnchanged",true},{"physicalPowerLoss",false},{"productionMaintenanceQualified",false}}.dump(2)<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
