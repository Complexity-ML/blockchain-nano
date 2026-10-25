#include "support/process.hpp"
#include "nano/storage.hpp"
#include "nano/evm.hpp"
#include "nano/consensus_capella_cold_records.hpp"
#include "../src/checkpoint_blob_bounded_read.hpp"
#include <rocksdb/db.h>
#include <exception>

namespace {
using namespace nano;
using namespace test;
const Bytes genesis(32,0x11);
constexpr size_t commit_budget=1024*1024;
Bytes hash(std::string_view value){return sha256(Bytes(value.begin(),value.end()));}

template<class Exception,class Call>
void refuses(const Call& call,const char* label){
 bool rejected=false;
 try{call();}catch(const Exception&){rejected=true;}
 check(rejected,label);
}

void application_boundary(){
 const std::string text="fixed-public-checkpoint-blob";
 const auto expected=hash(text);
 size_t hashes=0,copies=0;
 auto consume=[&](std::string_view value,size_t bound,const Bytes& digest){
  return storage_detail::validated_checkpoint_blob_copy(value,digest,bound,
   [&](std::string_view bytes){++hashes;return hash(bytes);},
   [&](std::string_view bytes){++copies;return std::string(bytes);});
 };
 check(consume(text,text.size(),expected)==text&&hashes==1&&copies==1,
       "exact bound hashes once and copies once");
 hashes=copies=0;
 refuses<std::runtime_error>([&]{consume(text,text.size()-1,expected);},"bound+1 refused");
 check(hashes==0&&copies==0,"oversized view reached no hash/copy callback");
 refuses<std::runtime_error>([&]{consume({},text.size(),expected);},"empty view refused");
 check(hashes==0&&copies==0,"empty view reached no hash/copy callback");
 auto corrupted=text;corrupted[0]^=1;
 refuses<std::runtime_error>([&]{consume(corrupted,text.size(),expected);},"digest mismatch refused");
 check(hashes==1&&copies==0,"hash mismatch refused before result copy");
 const auto sentinel=std::make_exception_ptr(ExecutionCancelled("public hash boundary sentinel"));
 bool original=false;
 try{
  storage_detail::validated_checkpoint_blob_copy(text,expected,text.size(),
   [&](std::string_view)->Bytes{std::rethrow_exception(sentinel);},
   [&](std::string_view bytes){++copies;return std::string(bytes);});
 }catch(...){original=std::current_exception()==sentinel;}
 check(original&&copies==0,"hash exception propagated unchanged without result copy");
}

template<class Database=rocksdb::DB>
std::unique_ptr<Database> open_owned_corruption_fixture(const fs::path& path){
 rocksdb::Options options;options.create_if_missing=false;
 std::unique_ptr<Database> result;
 if constexpr(requires{Database::Open(options,path.string(),&result);}){
  check(Database::Open(options,path.string(),&result).ok(),"owned fixture open");
 }else{
  Database* raw=nullptr;const auto status=Database::Open(options,path.string(),&raw);
  result.reset(raw);check(status.ok(),"owned fixture open");
 }
 return result;
}

void actual_store_and_provider(){
 Temp directory;
 const auto path=directory/"checkpoint-blobs";
 const std::string small="public-small-blob",large(64*1024,'q');
 const auto small_hash=hash(small),large_hash=hash(large);
 ForkCheckpointToken token;
 {
  ChainStore store(path);store.bind_chain(1337,genesis,"checkpoint-blob-read-test");
  token=store.commit_fork_checkpoint_with_blobs(1337,genesis,{},"{}",commit_budget,
    {{small_hash,small},{large_hash,large}},2);
 }
 {
  ChainStore store(path,ChainStore::OpenMode::ReadOnly);
  store.require_existing_chain_binding(1337,genesis,"checkpoint-blob-read-test");
  check(store.checkpoint_blob(1337,genesis,small_hash,small.size())==small,
        "real ChainStore exact byte limit accepted");
  refuses<std::runtime_error>([&]{store.checkpoint_blob(1337,genesis,small_hash,small.size()-1);},
                             "real ChainStore bound+1 refused");
  refuses<std::runtime_error>([&]{store.checkpoint_blob(1337,genesis,large_hash,256);},
                             "large stored value refused at node-sized result bound");
  check(!store.checkpoint_blob(1337,genesis,Bytes(32,0x22),256),"missing value remains nullopt");
  refuses<std::invalid_argument>([&]{store.checkpoint_blob(1338,genesis,small_hash,256);},
                                "wrong chain refused");
  refuses<std::invalid_argument>([&]{store.checkpoint_blob(1337,Bytes(32,0x12),small_hash,256);},
                                "wrong genesis refused");
  refuses<std::invalid_argument>([&]{store.checkpoint_blob(1337,genesis,Bytes(31),256);},
                                "wrong digest width refused");
  refuses<std::invalid_argument>([&]{store.checkpoint_blob(1337,genesis,small_hash,0);},
                                "zero caller bound refused");
  size_t fetches=0;
  CapellaCheckpointProvider provider([&](const Bytes& digest,size_t maximum){
   ++fetches;return store.checkpoint_blob(1337,genesis,digest,maximum);
  });
  const auto reader=provider.reader();
  check(reader(small_hash,small.size())==small,"real provider preserves bounded reader result");
  refuses<std::runtime_error>([&]{reader(large_hash,256);},"provider forwards the node bound");
  check(fetches==2,"provider performed only requested reads");
  provider.close();
  refuses<std::runtime_error>([&]{reader(small_hash,256);},"closed provider refuses before fetch");
  check(fetches==2,"closed provider never reached the backing database");
  check(store.fork_checkpoint(1337,genesis,commit_budget,token).has_value(),
        "read/refusal paths preserved checkpoint commitment");
 }
 // Deliberately mutate only this fresh, closed public fixture. No primary or
 // campaign database is accepted as an argument by this test.
 {
  auto db=open_owned_corruption_fixture(path);rocksdb::WriteOptions write;write.sync=true;
  check(db->Put(write,"checkpoint-blob/v1/"+hex(small_hash),"").ok(),"inject empty fixture value");
  auto wrong=large;wrong[0]^=1;
  check(db->Put(write,"checkpoint-blob/v1/"+hex(large_hash),wrong).ok(),"inject corrupt fixture value");
 }
 {
  ChainStore store(path,ChainStore::OpenMode::ReadOnly);
  refuses<std::runtime_error>([&]{store.checkpoint_blob(1337,genesis,small_hash,256);},
                             "empty persisted value refused");
  refuses<std::runtime_error>([&]{store.checkpoint_blob(1337,genesis,large_hash,large.size());},
                             "same-size corrupt persisted value refused");
 }
}
}

int main(){try{
 application_boundary();
 actual_store_and_provider();
 std::cout<<Json{{"scenario","checkpoint-blob-application-copy-bound"},{"groups",2},
  {"passed",true},{"realStoreAndProviderExercised",true},
  {"oversizedViewHashCallbacks",0},{"oversizedViewCopyCallbacks",0},
  {"rocksDbInternalAllocationBounded",false},{"allAllocationsBounded",false},
  {"processRssQualified",false},{"persistedFormatChanged",false},
  {"productionQualified",false}}.dump()<<'\n';
 return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
