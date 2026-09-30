#include "nano/consensus_capella_audit.hpp"
#include "nano/chain.hpp"
#include <rocksdb/db.h>
#include <fstream>
#include <iostream>
#include <unistd.h>
using namespace nano;
static void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
static Json load(const std::filesystem::path& p){std::ifstream f(p);Json j;f>>j;return j;}
static auto hashes(const std::filesystem::path& root){std::map<std::string,Bytes> out;for(const auto& e:std::filesystem::recursive_directory_iterator(root))if(e.is_regular_file()){std::ifstream f(e.path(),std::ios::binary);Bytes b((std::istreambuf_iterator<char>(f)),{});check(!f.bad(),"hash read");out[e.path().lexically_relative(root).string()]=sha256(b);}return out;}
template<class DB=rocksdb::DB>static std::unique_ptr<DB> open(const std::filesystem::path& path){
 rocksdb::Options options;std::unique_ptr<DB> db;
 if constexpr(requires{DB::Open(options,path.string(),&db);}){check(DB::Open(options,path.string(),&db).ok(),"open mutation copy");}
 else{DB* raw=nullptr;const auto s=DB::Open(options,path.string(),&raw);db.reset(raw);check(s.ok(),"open mutation copy");}return db;
}
int main(int argc,char** argv){try{
 check(argc==4,"usage: nano-capella-audit-corruption-tests genesis.json stopped-database audit-request.json");
 const auto genesis=Genesis::from_json(load(argv[1]));const std::filesystem::path source=argv[2];const auto request=load(argv[3]);const auto before=hashes(source);
 const auto baseline=audit_capella_database(genesis,source,request);check(baseline.at("canonicalHistoryVerified")==true,"baseline failed");
 Bytes historical_tx;
 {ChainStore store(source,ChainStore::OpenMode::ReadOnly);for(Int height=quantity(baseline.at("firstExecutionBlock"))+1;height<quantity(baseline.at("lastExecutionBlock"));++height){const auto b=store.block_by_number(height);check(bool(b),"fixture block missing");if(!b->transactions.empty()){historical_tx=b->transactions.front().hash();break;}}}
 check(!historical_tx.empty(),"fixture needs non-anchor non-tip transaction");
 char pattern[]="/tmp/nano-audit-corruption-XXXXXX";auto root=mkdtemp(pattern);check(root,"temporary directory");struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{root};
 Json results=Json::array();
 for(const std::string mode:{"missing-archive","invalid-signature","receipt","transaction-index"}){
  const auto copy=cleanup.p/mode;std::filesystem::copy(source,copy,std::filesystem::copy_options::recursive);
  {
   auto db=open(copy);rocksdb::WriteOptions options;options.sync=true;
   const std::string key=mode=="receipt"?"receipt/"+hex(historical_tx):mode=="transaction-index"?"tx/"+hex(historical_tx):"checkpoint-archive/v1/"+baseline.at("head").get<std::string>();
   std::string value;check(db->Get(rocksdb::ReadOptions(),key,&value).ok(),"mutation target missing");
   if(mode=="missing-archive")check(db->Delete(options,key).ok(),"delete archive");
   else{
    auto changed=Json::parse(value);
    if(mode=="invalid-signature"){
     auto bytes=unhex(changed.at("payload").get<std::string>());check(bytes.size()>100,"archive too short");bytes[4]^=1;
     changed["payload"]=hex(bytes);changed["digest"]=hex(sha256(bytes));
    }else if(mode=="receipt")changed["gasUsed"]="0x0";
    else changed["blockNumber"]="0x0";
    check(db->Put(options,key,changed.dump()).ok(),"write mutation");
   }
  }
  const auto copy_before=hashes(copy);std::string error;
  try{audit_capella_database(genesis,copy,request);}catch(const std::exception& e){error=e.what();}
  check(!error.empty(),"corruption accepted");check(hashes(copy)==copy_before,"audit modified corrupted copy");results.push_back({{"case",mode},{"rejected",true},{"error",error},{"filesUnchanged",true}});
 }
 check(hashes(source)==before,"audit modified original source");
 std::cout<<Json{{"baseline",baseline},{"cases",results},{"sourceFilesUnchanged",true}}.dump(2)<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
