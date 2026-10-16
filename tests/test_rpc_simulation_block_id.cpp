#include "nano/rpc.hpp"
#include <rocksdb/db.h>
#include <array>
#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <memory>
#include <thread>
#include <unistd.h>
using namespace nano;
namespace {
size_t checks=0,refusals=0,groups=0;
std::string stage="arguments";
void check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
// Detect the RocksDB output API, as storage.cpp does, without a version guess.
template<class Database>
rocksdb::Status open_fixture_database(const rocksdb::Options& options,const std::string& path,std::unique_ptr<Database>& opened){
 if constexpr(requires { Database::Open(options,path,&opened); })return Database::Open(options,path,&opened);
 else{Database* raw=nullptr;auto status=Database::Open(options,path,&raw);opened.reset(raw);return status;}
}
struct OwnedDirectory {
 std::filesystem::path path=std::filesystem::temp_directory_path()/("nano-rpc-eip1898-"+std::to_string(getpid()));
 OwnedDirectory(){check(std::filesystem::create_directory(path),"new fixture namespace already exists");}
 ~OwnedDirectory(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
Json request(const char* method,const Json& call,const Json& selector){return {{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",Json::array({call,selector})}};}
Json value(Rpc& rpc,const char* method,const Json& call,const Json& selector){auto response=rpc.handle(request(method,call,selector));check(response.contains("result")&&!response.contains("error"),"selected simulation should succeed");return response.at("result");}
void refused(Rpc& rpc,const char* method,const Json& call,const Json& selector,int code){auto response=rpc.handle(request(method,call,selector));check(response.contains("error")&&!response.contains("result")&&response.at("error").at("code")==code,"selected simulation should refuse with the expected code");++refusals;}
void emit(const char* scenario){++groups;std::cout<<Json{{"scenario",scenario},{"passed",true},{"checks",checks},{"expectedRefusals",refusals}}.dump()<<std::endl;}

// Exercise the real execution-retention writer without replaying the selector suite.
int pruning_boundary_self_test(){try{
 constexpr size_t checkpoint_budget=8*1024*1024;
 OwnedDirectory owned;const auto path=owned.path/"retention-boundaries";
 const Bytes fixture_key=bytes(Int(777),32); // Existing public unit-test key, never a campaign key.
 Transaction transfer;transfer.chain_id=1337;transfer.to="0x00000000000000000000000000000000000000b1";transfer.gas=21000;transfer.gas_price=2000000000;
 const auto sender=public_address(public_key(fixture_key));
 const std::string target="0x00000000000000000000000000000000000000a1",slow="0x00000000000000000000000000000000000000a2";
 Bytes slow_code(16384,0x5b);const auto context_code=unhex("0x600240600052436020524760405260606000f3");slow_code.insert(slow_code.end(),context_code.begin(),context_code.end());
 auto genesis=Genesis::from_json({{"config",{{"chainId",1337},{"evmFork","Shanghai"}}},{"alloc",{{sender,{{"balance","0x100000000000000000000"}}},{target,{{"code","0x436000554360005260206000f3"}}},{slow,{{"code",hex(slow_code)}}}}}});
 const Json call={{"from",sender},{"to",target},{"gas","0x493e0"}};
 const std::array<const char*,3> methods{"eth_call","eth_estimateGas","eth_createAccessList"};
 std::vector<Block> blocks{genesis.block};std::vector<Bytes> transactions;std::vector<CheckpointArchiveEntry> archives;
 auto chain=std::make_unique<Blockchain>(path,genesis);
 stage="retention-fixture";
 for(unsigned number=1;number<=6;++number){
  transfer.nonce=Int(number-1);transfer.value=Int(number);auto signed_transfer=SignedTransaction::sign(transfer,fixture_key);
  check(chain->submit(signed_transfer),"public synthetic transfer must enter the fixture pool");
  blocks.push_back(chain->mine(slow,Int(number*12)));transactions.push_back(signed_transfer.hash());
  const auto receipt=chain->store().receipt(transactions.back());
  check(receipt&&receipt->block_hash==blocks.back().header.hash()&&receipt->block_number==Int(number),"fixture must contain a real committed receipt at every height");
  const auto encoded=blocks.back().to_json(true).dump();archives.push_back({blocks.back().header.hash(),number,Bytes(encoded.begin(),encoded.end())});
 }
 const auto tip=blocks.back().header.hash();check(chain->tip().header.number==6,"retention fixture must stop at height six");
 const auto view=[&]{return Json{{"head",hex(chain->tip().header.hash())},{"stateRoot",hex(chain->state().root())},{"pool",chain->pool_info(true)},{"events",chain->event_cursor()}};};
 const auto original=view();const auto snapshot=Json{{"format","synthetic-rpc-retention-checkpoint-v1"},{"head",hex(tip)},{"stateRoot",original.at("stateRoot")}}.dump();
 ForkCheckpointToken token;
 const auto checkpoint=[&](const Int& common,size_t maximum_blocks,const std::optional<ExecutionStateRetentionAnchor>& retention){
  // The underlying fixture owner is non-const. Empty suffix/common six never
  // changes its published head/state; no production visibility hook is added.
  auto& disk=const_cast<ChainStore&>(chain->store());
  return disk.commit_execution_checkpoint(1337,genesis.block.header.hash(),token,tip,common,blocks.at(u64(common)).header.hash(),{},snapshot,checkpoint_budget,maximum_blocks,{},retention);
 };
 {
  auto& disk=const_cast<ChainStore&>(chain->store());
  token=disk.commit_execution_checkpoint(1337,genesis.block.header.hash(),{},tip,6,tip,{},snapshot,checkpoint_budget,8,archives);
 }
 check(!chain->store().state_pruning()&&chain->store().state_at(1)&&chain->store().state_at(2),"coupling alone must retain historical states");
 const auto coupled_token=token;
 token=checkpoint(6,2,ExecutionStateRetentionAnchor{6,tip});
 auto marker=chain->store().state_pruning();
 check(marker&&marker->before==3&&marker->anchor.number==6&&marker->anchor.hash==tip,"partial pruning must distinguish before three from anchor six");
 check(chain->store().state_at(0)&&!chain->store().state_at(2)&&chain->store().state_at(3)&&chain->store().state_at(6),"first retention batch must keep genesis and the available frontier");
 check(token.sequence==coupled_token.sequence+1&&chain->store().fork_checkpoint(1337,genesis.block.header.hash(),checkpoint_budget,token)->snapshot==snapshot,"retention marker and externally pinned checkpoint must advance together");
 check(view()==original,"retention must not change the in-memory head/state/pool/events");
 auto unavailable=[&](Rpc& rpc,const char* method,const Json& selector){
  const auto response=rpc.handle(request(method,call,selector));
  check(response.contains("error")&&!response.contains("result")&&response.at("error").at("code")==-32000&&response.at("error").at("message")=="State unavailable","known pruned block must refuse as unavailable state, never unknown/latest");++refusals;
 };
 std::array<Json,3> latest,pending;
 {
  Rpc rpc(*chain);stage="retention-rpc-frontiers";
  for(size_t i=0;i<methods.size();++i){
   const auto* method=methods[i];
   unavailable(rpc,method,Json{{"blockNumber","0x2"}});unavailable(rpc,method,Json{{"blockHash",hex(blocks[2].header.hash())},{"requireCanonical",true}});
   const auto retained=value(rpc,method,call,"0x3");
   check(value(rpc,method,call,Json{{"blockHash",hex(blocks[3].header.hash())},{"requireCanonical",true}})==retained,"retained state below the rollback anchor must remain queryable by hash");
   latest[i]=value(rpc,method,call,"latest");pending[i]=value(rpc,method,call,"pending");
  }
  check(value(rpc,"eth_call",call,"0x3")==hex(bytes(Int(3),32)),"retained historical call must execute at height three");
  check(value(rpc,"eth_call",call,"latest")==hex(bytes(Int(6),32)),"latest call must use the unchanged head");
  check(value(rpc,"eth_call",call,"pending")==hex(bytes(Int(7),32)),"pending call must retain the next-block context");
  check(view()==original,"boundary refusals and simulations must not publish state");
 }
 emit("real-retention-before-versus-anchor-rpc");
 {
  stage="retention-during-detached-call";Rpc rpc(*chain);auto slow_call=call;slow_call["to"]=slow;
  const Json selected={{"blockHash",hex(blocks[3].header.hash())},{"requireCanonical",true}};
  const auto expected=value(rpc,"eth_call",slow_call,selected);
  Bytes context=blocks[2].header.hash();const auto number=bytes(Int(3),32),balance=bytes(chain->store().state_at(3)->get(slow).balance,32);context.insert(context.end(),number.begin(),number.end());context.insert(context.end(),balance.begin(),balance.end());
  check(expected==hex(context),"captured context must bind BLOCKHASH, NUMBER and historical account balance");
  std::atomic<bool> paused{false},release{false};unsigned polls=0;
  RpcSimulationLimits limits;limits.now=[&]{
   if(++polls==20){paused.store(true);const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(2);while(!release.load()){if(std::chrono::steady_clock::now()>=end)throw ExecutionCancelled("bounded retention rendezvous expired");std::this_thread::sleep_for(std::chrono::milliseconds(1));}}
   return std::chrono::steady_clock::time_point{};
  };
  Rpc concurrent(*chain,{},AdmissionBudget::Clock::now,limits);std::future<Json> calculating,reading;
  struct Release {std::atomic<bool>& flag;~Release(){flag.store(true);}} release_guard{release};
  calculating=std::async(std::launch::async,[&]{return concurrent.handle(request("eth_call",slow_call,selected));});
  const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(2);while(!paused.load()&&std::chrono::steady_clock::now()<end)std::this_thread::sleep_for(std::chrono::milliseconds(1));
  check(paused.load(),"selected call must reach the bounded detached pause");
  reading=std::async(std::launch::async,[&]{return concurrent.handle(Json{{"jsonrpc","2.0"},{"id",2},{"method","eth_blockNumber"},{"params",Json::array()}});});
  check(reading.wait_for(std::chrono::milliseconds(250))==std::future_status::ready,"identity RPC must remain available before retention advances");
  check(reading.get().at("result")=="0x6","concurrent read must observe height six");
  token=checkpoint(6,2,ExecutionStateRetentionAnchor{6,tip});marker=chain->store().state_pruning();
  check(marker&&marker->before==5&&marker->anchor.number==6&&marker->anchor.hash==tip,"second bounded retention batch must stop at before five");
  check(chain->store().state_at(0)&&!chain->store().state_at(3)&&!chain->store().state_at(4)&&chain->store().state_at(5),"captured state must be physically unavailable to new storage reads");
  check(view()==original,"pruning while a call is detached must not change the current chain");
  release.store(true);const auto completed=calculating.get();
  check(completed.contains("result")&&!completed.contains("error")&&completed.at("result")==expected,"detached execution must retain its captured historical state and ancestor window");
  for(size_t i=0;i<methods.size();++i){
   unavailable(rpc,methods[i],Json{{"blockNumber","0x3"}});unavailable(rpc,methods[i],selected);
   check(value(rpc,methods[i],call,"latest")==latest[i],"fresh latest simulation must remain unchanged after retention");
   check(value(rpc,methods[i],call,"pending")==pending[i],"fresh pending simulation must remain unchanged after retention");
  }
  check(view()==original,"detached completion and fresh refusals must not republish deleted state");
  for(size_t i=1;i<blocks.size();++i){const auto block=chain->store().block_by_number(Int(i));const auto receipt=chain->store().receipt(transactions[i-1]);const auto transaction=chain->store().transaction(transactions[i-1]);const auto archive=chain->store().checkpoint_archive(blocks[i].header.hash(),checkpoint_budget);
   check(block&&block->header.hash()==blocks[i].header.hash()&&receipt&&receipt->block_hash==blocks[i].header.hash()&&transaction&&archive&&archive->payload==archives[i-1].payload,"retention must preserve every canonical block/transaction/receipt/archive");}
  check(chain->store().fork_checkpoint(1337,genesis.block.header.hash(),checkpoint_budget,token)->snapshot==snapshot,"second retention checkpoint must retain its exact external token and bytes");
 }
 emit("captured-rpc-state-survives-real-retention");
 stage="retention-reopen-and-rollback";chain.reset();
 {
  ChainStore disk(path,ChainStore::OpenMode::ReadOnly);const auto retained=disk.state_pruning();
  check(retained&&retained->before==5&&retained->anchor.number==6&&retained->anchor.hash==tip&&disk.fork_checkpoint(1337,genesis.block.header.hash(),checkpoint_budget,token)->snapshot==snapshot,"read-only reopen must retain the exact floor/anchor/checkpoint");
  for(unsigned number=0;number<=6;++number)check(bool(disk.state_at(Int(number)))==(number==0||number>=5),"reopen must not recreate pruned states or discard the retained range");
  bool stale_refused=false;try{(void)disk.fork_checkpoint(1337,genesis.block.header.hash(),checkpoint_budget,coupled_token);}catch(const std::runtime_error& error){stale_refused=std::string(error.what())=="fork checkpoint differs from external commitment";}check(stale_refused,"cold reopen must reject an independently retained obsolete checkpoint commitment");++refusals;
  for(size_t i=1;i<blocks.size();++i){const auto block=disk.block_by_number(Int(i));const auto receipt=disk.receipt(transactions[i-1]);const auto archive=disk.checkpoint_archive(blocks[i].header.hash(),checkpoint_budget);check(block&&block->header.hash()==blocks[i].header.hash()&&receipt&&receipt->block_hash==blocks[i].header.hash()&&archive&&archive->payload==archives[i-1].payload,"cold reopen must retain canonical block/receipt/archive history");}
 }
 chain=std::make_unique<Blockchain>(path,genesis,ChainStore::OpenMode::ReadWriteExisting);
 {
  Rpc rpc(*chain);
  for(const auto* method:methods){unavailable(rpc,method,Json{{"blockNumber","0x3"}});unavailable(rpc,method,Json{{"blockHash",hex(blocks[3].header.hash())},{"requireCanonical",true}});}
  const auto before=token;bool rejected=false;
  try{(void)checkpoint(5,2,std::nullopt);}catch(const std::invalid_argument& error){rejected=std::string(error.what())=="replacement below retained execution anchor";}
  check(rejected,"rollback below the retained anchor must refuse for the correct invariant");++refusals;
  marker=chain->store().state_pruning();
  check(marker&&marker->before==5&&chain->store().fork_checkpoint(1337,genesis.block.header.hash(),checkpoint_budget,before)->snapshot==snapshot&&chain->tip().header.hash()==tip,"rollback refusal must not advance token or mutate the frontier/head");
  token=checkpoint(6,2,std::nullopt);marker=chain->store().state_pruning();
  check(token.sequence==before.sequence+1&&token.digest==before.digest&&marker&&marker->before==5&&chain->store().fork_checkpoint(1337,genesis.block.header.hash(),checkpoint_budget,token)->snapshot==snapshot,"checkpoint at the retained anchor must remain writable without changing its pruning frontier");
  check(value(rpc,"eth_call",call,"latest")==hex(bytes(Int(6),32)),"reopened latest call must retain height six");
  check(value(rpc,"eth_call",call,"pending")==hex(bytes(Int(7),32)),"reopened pending call must retain height seven");
 }
 chain.reset();{ChainStore disk(path,ChainStore::OpenMode::ReadOnly);check(disk.state_pruning()->before==5&&disk.fork_checkpoint(1337,genesis.block.header.hash(),checkpoint_budget,token)->snapshot==snapshot,"accepted anchor checkpoint must cold-reopen with the exact commitment");}
 emit("retention-cold-reopen-and-rollback-floor");
 check(groups==3&&refusals==20&&checks==111,"retention-specific coverage counters must remain exact");
 std::cout<<Json{{"format","nano-rpc-pruning-retention-boundary-v1"},{"passed",true},{"groups",groups},{"checks",checks},{"expectedRefusals",refusals},{"fixtureTransactions",6},{"fixtureKeyPublic",true},{"retentionBefore",5},{"retentionAnchor",6},{"oldSelectorSuiteExecuted",false},{"coreSourceModified",false},{"networkStarted",false},{"runtimeDutiesResumed",false},{"productionQualified",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<std::endl;std::cout<<Json{{"format","nano-rpc-pruning-retention-boundary-v1"},{"passed",false},{"stage",stage},{"checks",checks},{"expectedRefusals",refusals},{"productionQualified",false}}.dump()<<std::endl;return 1;}}
}
int main(int argc,char** argv){try{
 if(argc==2&&std::string(argv[1])=="--pruning-boundary-self-test")return pruning_boundary_self_test();
 check(argc==2&&std::string(argv[1])=="--eip1898-self-test","usage: nano-rpc-simulation-block-id-tests --eip1898-self-test");
 OwnedDirectory owned;
 const std::string sender="0x0000000000000000000000000000000000000101",target="0x00000000000000000000000000000000000000a1",slow="0x00000000000000000000000000000000000000a2";
 // The first contract writes only its simulation copy and returns NUMBER.
 // A bounded JUMPDEST prefix lets the second pause after the RPC lock is released.
 Bytes slow_code(16384,0x5b);auto context_code=unhex("0x600140600052436020524760405260606000f3");slow_code.insert(slow_code.end(),context_code.begin(),context_code.end());
 auto genesis=Genesis::from_json({{"config",{{"chainId",1337},{"evmFork","Shanghai"}}},{"alloc",{{sender,{{"balance","0x100000000000000000000"}}},{target,{{"code","0x436000554360005260206000f3"}}},{slow,{{"code",hex(slow_code)}}}}}});
 const Json call={{"from",sender},{"to",target},{"gas","0x493e0"}};
 const std::array<const char*,3> methods{"eth_call","eth_estimateGas","eth_createAccessList"};
 {
  Blockchain chain(owned.path/"contexts",genesis);Rpc rpc(chain);
  const auto first=chain.mine(slow,12),second=chain.mine(slow,24);
  const auto genesis_hash=hex(genesis.block.header.hash());
  auto before=[&]{return Json{{"root",hex(chain.state().root())},{"head",hex(chain.tip().header.hash())},{"pool",chain.pool_info(true)},{"events",chain.event_cursor()}};};
  stage="canonical-selectors";const auto initial=before();
  for(const auto* method:methods){
   const auto expected=value(rpc,method,call,"0x0");
   for(const auto& selector:std::vector<Json>{{{"blockHash",genesis_hash}},{{"blockHash",genesis_hash},{"requireCanonical",true}},{{"blockHash",genesis_hash},{"requireCanonical",false}},{{"blockNumber","0x0"}}})
    check(value(rpc,method,call,selector)==expected,"hash and number objects must use the same historical context");
  }
  check(value(rpc,"eth_call",call,Json{{"blockHash",genesis_hash}})==hex(bytes(Int(0),32)),"genesis hash call must return genesis NUMBER");
  check(value(rpc,"eth_estimateGas",call,Json{{"blockHash",hex(first.header.hash())}})!=value(rpc,"eth_estimateGas",call,Json{{"blockNumber","0x0"}}),"historical header must affect SSTORE gas estimation");
  check(value(rpc,"eth_call",call,"pending")==hex(bytes(Int(3),32)),"explicit pending call must retain next-block context");
  const auto omitted=rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method","eth_estimateGas"},{"params",Json::array({call})}});
  check(omitted.contains("result")&&omitted.at("result")==value(rpc,"eth_estimateGas",call,"pending"),"omitted estimate must retain pending semantics");
  check(value(rpc,"eth_createAccessList",call,genesis_hash)==value(rpc,"eth_createAccessList",call,"0x0"),"access-list legacy DATA selector must remain unchanged");
  check(before()==initial,"successful simulations must preserve canonical state, head, pool and events");
  emit("canonical-hash-number-and-pending");

  stage="malformed-and-missing-selectors";
  std::string invalid_hash=genesis_hash;invalid_hash.back()='z';
  const std::vector<Json> invalid={Json::object(),{{"blockNumber","0x0"},{"blockHash",genesis_hash}},{{"blockHash",genesis_hash},{"extra",false}},{{"blockNumber","latest"}},{{"blockNumber","0x00"}},{{"blockNumber",0}},{{"blockNumber","0x10000000000000000"}},{{"blockHash","0x01"}},{{"blockHash",7}},{{"blockHash",invalid_hash}},{{"blockNumber","0x0"},{"requireCanonical",true}},{{"blockHash",genesis_hash},{"requireCanonical","true"}}};
  check(invalid.size()==12,"malformed-selector matrix must stay explicit");
  for(const auto* method:methods)for(const auto& selector:invalid)refused(rpc,method,call,selector,-32602);
  const std::string unknown_hash="0x"+std::string(64,'f');
  for(const auto* method:methods){
   const int missing_code=std::string(method)=="eth_createAccessList"?-32000:-32001;
   for(bool canonical:{false,true})refused(rpc,method,call,Json{{"blockHash",unknown_hash},{"requireCanonical",canonical}},missing_code);
   refused(rpc,method,call,Json{{"blockNumber","0xff"}},missing_code);
  }
  check(before()==initial,"invalid or missing selectors must not change canonical state");
  emit("malformed-and-unknown-resource-precedence");

  stage="captured-context-and-obsolete-resolution";
  auto slow_call=call;slow_call["to"]=slow;
  const auto old_hash=hex(second.header.hash());
  const auto old_result=value(rpc,"eth_call",slow_call,Json{{"blockHash",old_hash},{"requireCanonical",true}});
  Bytes expected_old=first.header.hash();auto append_word=[&](const Int& n){auto word=bytes(n,32);expected_old.insert(expected_old.end(),word.begin(),word.end());};append_word(2);append_word(chain.state().get(slow).balance);
  check(old_result==hex(expected_old),"hash-selected call must expose its ancestor, NUMBER and selected account balance");
  std::atomic<bool> paused{false},release{false};unsigned polls=0;
  RpcSimulationLimits pause_limits;pause_limits.now=[&]{
   if(++polls==20){paused.store(true);const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(2);while(!release.load()){if(std::chrono::steady_clock::now()>=until)throw ExecutionCancelled("bounded test rendezvous expired");std::this_thread::sleep_for(std::chrono::milliseconds(1));}}
   return std::chrono::steady_clock::time_point{};
  };
  Rpc concurrent(chain,{},AdmissionBudget::Clock::now,pause_limits);
  std::future<Json> calculating,reading;
  struct Release {std::atomic<bool>& flag;~Release(){flag.store(true);}} release_guard{release};
  calculating=std::async(std::launch::async,[&]{return concurrent.handle(request("eth_call",slow_call,Json{{"blockHash",old_hash},{"requireCanonical",true}}));});
  const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(2);while(!paused.load()&&std::chrono::steady_clock::now()<until)std::this_thread::sleep_for(std::chrono::milliseconds(1));
  check(paused.load(),"hash-selected execution must reach the bounded detached pause");
  reading=std::async(std::launch::async,[&]{return concurrent.handle(Json{{"jsonrpc","2.0"},{"id",2},{"method","eth_blockNumber"},{"params",Json::array()}});});
  check(reading.wait_for(std::chrono::milliseconds(250))==std::future_status::ready,"identity RPC must remain available while simulation is detached");
  check(reading.get().at("result")=="0x2","concurrent identity RPC must observe the original head");
  check(chain.restore(0,genesis.block.header.hash()),"controlled fixture rewind must succeed");chain.mine(sender,13);chain.mine(sender,25);
  const auto replaced=before();check(hex(chain.tip().header.hash())!=old_hash,"replacement fixture must have a distinct hash");
  release.store(true);const auto completed=calculating.get();
  check(completed.contains("result")&&completed.at("result")==old_result,"detached simulation must retain the selected state, header and ancestors across rewind");
  check(before()==replaced,"detached completion must not write its former state into the replacement chain");
  for(const auto* method:methods)for(bool canonical:{false,true})refused(rpc,method,slow_call,Json{{"blockHash",old_hash},{"requireCanonical",canonical}},std::string(method)=="eth_createAccessList"?-32000:-32001);
  check(value(rpc,"eth_call",slow_call,Json{{"blockHash",hex(chain.tip().header.hash())}})!=old_result,"fresh hash lookup must resolve the replacement context, not a cached old state");
  emit("immutable-context-and-stale-hash-refusal");

  stage="hash-cancellation-deadline";const auto stable=before();const Json selected={{"blockHash",hex(chain.tip().header.hash())},{"requireCanonical",true}};
  for(const auto* method:{"eth_call","eth_estimateGas"}){
   unsigned checkpoints=0;auto cancelled=rpc.handle(request(method,slow_call,selected),[&]{if(++checkpoints==20)throw ExecutionCancelled("public hash-selector test cancellation");});
   check(cancelled.contains("error")&&cancelled.at("error").at("code")==-32002&&checkpoints==20,"hash-selected simulation must preserve top-level execution cancellation");++refusals;
   unsigned ticks=0;RpcSimulationLimits deadline;deadline.timeout=std::chrono::milliseconds(8);deadline.now=[&]{return std::chrono::steady_clock::time_point{}+std::chrono::milliseconds(ticks++);};Rpc bounded(chain,{},AdmissionBudget::Clock::now,deadline);
   auto expired=bounded.handle(request(method,slow_call,selected));check(expired.contains("error")&&expired.at("error").at("code")==-32002,"hash-selected simulation must retain the shared deadline");++refusals;
  }
  check(before()==stable,"cancelled or timed-out simulations must preserve canonical state");
  emit("hash-selector-cancellation-and-deadline");
 }

 stage="unavailable-historical-state";const auto missing_path=owned.path/"missing-state";Bytes historical_hash;
 {Blockchain chain(missing_path,genesis);historical_hash=chain.mine(sender,12).header.hash();chain.mine(sender,24);}
 // Remove only an owned synthetic historical state, retaining its known block.
 {rocksdb::Options options;options.create_if_missing=false;std::unique_ptr<rocksdb::DB> db;check(open_fixture_database(options,missing_path.string(),db).ok(),"owned synthetic database must reopen for missing-state fixture");rocksdb::WriteOptions writes;writes.sync=true;check(db->Delete(writes,"state/"+hex(bytes(Int(1),8))).ok(),"owned fixture historical state removal must succeed");}
 {Blockchain chain(missing_path,genesis,ChainStore::OpenMode::ReadWriteExisting);Rpc rpc(chain);const auto root=chain.state().root();
  for(const auto* method:methods)refused(rpc,method,call,Json{{"blockHash",hex(historical_hash)},{"requireCanonical",false}},-32000);
  check(value(rpc,"eth_call",call,"latest")==hex(bytes(Int(2),32)),"missing historical state must not make a valid latest state unavailable");check(chain.state().root()==root,"unavailable historical state refusal must preserve the tip");
 }
 emit("known-header-without-historical-state");
 check(groups==5&&refusals==58,"new selector coverage counts must remain exact");
 std::cout<<Json{{"format","nano-rpc-simulation-eip1898-targeted-v1"},{"passed",true},{"groups",groups},{"checks",checks},{"expectedRefusals",refusals},{"oldSuiteExecuted",false},{"networkStarted",false},{"productionQualified",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<std::endl;std::cout<<Json{{"format","nano-rpc-simulation-eip1898-targeted-v1"},{"passed",false},{"stage",stage},{"checks",checks},{"expectedRefusals",refusals},{"productionQualified",false}}.dump()<<std::endl;return 1;}}
