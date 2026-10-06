#include "nano/migration.hpp"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <unistd.h>
#ifdef __linux__
#include <fcntl.h>
#include <sys/syscall.h>
#endif
namespace nano {
namespace {
struct Source {
 std::filesystem::path root;
 std::map<std::filesystem::path,Bytes> hashes;
 static std::string read(const std::filesystem::path& path){
  std::ifstream file(path,std::ios::binary);if(!file)throw std::runtime_error("cannot read migration source: "+path.string());
  std::string data((std::istreambuf_iterator<char>(file)),{});if(file.bad())throw std::runtime_error("migration source read failed: "+path.string());return data;
 }
 Json load(const std::filesystem::path& relative){auto path=root/relative;auto data=read(path);hashes[path]=keccak(Bytes(data.begin(),data.end()));return Json::parse(data);}
 void verify()const{for(const auto& [path,hash]:hashes){auto data=read(path);if(keccak(Bytes(data.begin(),data.end()))!=hash)throw std::runtime_error("migration source changed during import: "+path.string());}}
};
std::string canonical_hash(const Json& value){auto hash=unhex(value.get<std::string>());if(hash.size()!=32)throw std::invalid_argument("migration requires a 32-byte hash");return hex(hash);}
struct Temporary {
 std::filesystem::path path;
 ~Temporary(){if(!path.empty()){std::error_code ignored;std::filesystem::remove_all(path,ignored);}}
};
void publish(const std::filesystem::path& source,const std::filesystem::path& destination){
 // Do not replace even an empty directory created by another process during replay.
#ifdef __APPLE__
 int result=renamex_np(source.c_str(),destination.c_str(),RENAME_EXCL);
#elif defined(__linux__)
 int result=int(syscall(SYS_renameat2,AT_FDCWD,source.c_str(),AT_FDCWD,destination.c_str(),1 /* RENAME_NOREPLACE */));
#else
 throw std::runtime_error("atomic migration publication is supported on macOS and Linux");
#endif
#if defined(__APPLE__) || defined(__linux__)
 if(result!=0)throw std::runtime_error(std::string("cannot publish migrated database: ")+std::strerror(errno));
#endif
}
}
Json migrate_chaindb(const Genesis& genesis,const std::filesystem::path& source,const std::filesystem::path& destination){
 if(!std::filesystem::is_directory(source))throw std::invalid_argument("migration source must be a Python ChainDB directory");
 auto target=std::filesystem::absolute(destination).lexically_normal();
 if(std::filesystem::symlink_status(target).type()!=std::filesystem::file_type::not_found)throw std::invalid_argument("migration destination already exists");
 if(!std::filesystem::is_directory(target.parent_path()))throw std::invalid_argument("migration destination parent must exist");
 // Keep all generated files outside the original ChainDB, including when its path is a symlink.
 auto root=std::filesystem::canonical(source);auto parent=std::filesystem::canonical(target.parent_path());
 auto relative=parent.lexically_relative(root);if(relative.empty()||(*relative.begin())!="..")throw std::invalid_argument("migration destination must be outside source directory");
 Source input{root,{}};auto metadata=input.load("meta.json");
 if(quantity(metadata.at("chain_id"))!=genesis.chain_id)throw std::invalid_argument("source chain ID does not match genesis");
 if(canonical_hash(metadata.at("genesis_hash"))!=hex(genesis.block.header.hash()))throw std::invalid_argument("source genesis hash does not match genesis");
 auto last=u64(quantity(metadata.at("latest_block")));if(last==UINT64_MAX)throw std::invalid_argument("source height exceeds supported range");
 auto initial=Block::from_json(input.load("blocks/0.json"));
 if(initial.header.hash()!=genesis.block.header.hash()||!initial.transactions.empty()||!initial.uncles.empty()||!initial.withdrawals.empty())throw std::invalid_argument("source genesis block mismatch");
 std::string pattern=(target.parent_path()/("."+target.filename().string()+".import-XXXXXX")).string();
 std::vector<char> buffer(pattern.begin(),pattern.end());buffer.push_back(0);if(!mkdtemp(buffer.data()))throw std::runtime_error("cannot create migration temporary directory");
 Temporary temporary{std::filesystem::path(buffer.data())};Json summary;
 {
  Blockchain chain(temporary.path,genesis);
  for(uint64_t number=1;number<=last;++number){
   try{
    auto data=input.load(std::filesystem::path("blocks")/(std::to_string(number)+".json"));
    for(auto& transaction:data.at("transactions")){
     std::optional<std::string> expected;
     if(transaction.is_string()){expected=canonical_hash(transaction);transaction=input.load(std::filesystem::path("transactions")/(*expected+".json"));}
     else if(transaction.contains("hash"))expected=canonical_hash(transaction["hash"]);
     SignedTransaction decoded{Transaction::from_json(transaction),quantity(transaction.at("v")),quantity(transaction.at("r")),quantity(transaction.at("s"))};
     if(expected&&*expected!=hex(decoded.hash()))throw std::invalid_argument("source transaction hash mismatch");
     if(transaction.contains("from")&&address(transaction["from"])!=address(decoded.sender()))throw std::invalid_argument("source transaction sender mismatch");
    }
    auto block=Block::from_json(data);if(block.header.number!=Int(std::to_string(number)))throw std::invalid_argument("source block number mismatch");
    // Historical replay validates timestamps relative to each block, not the wall clock.
    chain.import_block(block,block.header.timestamp);
   }catch(const std::exception& e){throw std::runtime_error("migration failed at block "+std::to_string(number)+": "+e.what());}
  }
  if(chain.store().total_difficulty()!=quantity(metadata.at("total_difficulty")))throw std::invalid_argument("source total difficulty mismatch");
  auto tip=chain.tip();summary={{"blocks",last+1},{"blockNumber",quantity(tip.header.number)},{"blockHash",hex(tip.header.hash())},{"stateRoot",hex(chain.state().root())},{"database",target.string()}};
 }
 // Reopen the finished database before it becomes visible at the requested path.
 {Blockchain reopened(temporary.path,genesis);if(hex(reopened.tip().header.hash())!=summary["blockHash"].get<std::string>()||hex(reopened.state().root())!=summary["stateRoot"].get<std::string>())throw std::runtime_error("migrated database failed reopen verification");}
 input.verify();publish(temporary.path,target);temporary.path.clear();return summary;
}
}
