// Public owned fixtures. Header-selection work is distinct from state decoding;
// neither this test nor its derived header index authenticates network finality.
#include "nano/rpc.hpp"
#include "support/durable_owned_fixture.hpp"
#include "nano/storage_memory_policy.hpp"
#include <rocksdb/db.h>
#include <future>
#include <map>
#include <memory>
#include <iostream>
#include <unistd.h>
using namespace nano;

namespace {
using Check=std::function<void(bool,const char*)>;
namespace du=nano::test::durable_owned;
template<class Database=rocksdb::DB>
std::unique_ptr<Database> raw_database(const std::filesystem::path& path,bool read_only){
 rocksdb::Options options;options.create_if_missing=false;apply_chain_store_memory_policy(options);
 std::unique_ptr<Database> opened;rocksdb::Status status;
 if(read_only){
  if constexpr(requires{Database::OpenForReadOnly(options,path.string(),&opened);})status=Database::OpenForReadOnly(options,path.string(),&opened);
  else{Database* raw=nullptr;status=Database::OpenForReadOnly(options,path.string(),&raw);opened.reset(raw);}
 }else{
  if constexpr(requires{Database::Open(options,path.string(),&opened);})status=Database::Open(options,path.string(),&opened);
  else{Database* raw=nullptr;status=Database::Open(options,path.string(),&raw);opened.reset(raw);}
 }
 if(!status.ok())throw std::runtime_error("owned closed fixture open: "+status.ToString());return opened;
}
void put_owned(const std::filesystem::path& path,const std::string& key,const std::optional<std::string>& value){
 // Every caller has closed its sole ChainStore/Blockchain before this open.
 auto db=raw_database(path,false);rocksdb::WriteOptions options;options.sync=true;
 const auto status=value?db->Put(options,key,*value):db->Delete(options,key);
 if(!status.ok())throw std::runtime_error("owned fixture mutation: "+status.ToString());
}
std::map<std::string,std::string> records(const std::filesystem::path& path){
 auto db=raw_database(path,true);std::map<std::string,std::string> out;
 std::unique_ptr<rocksdb::Iterator> it(db->NewIterator(rocksdb::ReadOptions()));
 for(it->SeekToFirst();it->Valid();it->Next())out.emplace(it->key().ToString(),it->value().ToString());
 if(!it->status().ok())throw std::runtime_error("owned fixture read: "+it->status().ToString());return out;
}
template<class Error,class Action> bool raises(Action&& action){try{action();}catch(const Error&){return true;}return false;}
void header_cost_cases(const std::filesystem::path& root,const Check& check,size_t& refusals,std::string& stage,Json& last){
 constexpr size_t cap=4*1024*1024;
 const auto genesis=Genesis::from_json({{"config",{{"chainId",1337},{"evmFork","Shanghai"}}},{"alloc",Json::object()}});
 const auto path=root/"header-body-causal";{Blockchain owner(path,genesis);}
 const auto height=hex(bytes(Int(0),8)),body_key="block/"+height,index_key="execution-header/v1/"+height;
 const auto original=records(path);check(original.contains(index_key),"ordinary Genesis commit must publish its derived header index");
 const auto original_body=original.at(body_key),original_header=original.at(index_key);
 check(original_header==genesis.block.header.to_json().dump(),"derived index is exactly the existing Header JSON codec");
 stage="indexed-body-independent";
 put_owned(path,body_key,std::string("{not valid block JSON"));
 const auto before=records(path);
 {
  // ChainStore only: Blockchain startup/global auditing still decodes a body.
  ChainStore disk(path,ChainStore::OpenMode::ReadOnly);
  const auto by_number=disk.header_by_number(0,cap),by_hash=disk.header_by_hash(genesis.block.header.hash(),cap);
  check(by_number&&by_hash&&by_number->to_json()==genesis.block.header.to_json()&&by_hash->to_json()==by_number->to_json(),"indexed selection must not parse even an unreadable block body");
  check(raises<Json::exception>([&]{(void)disk.block_by_number(0);}),"the same malformed body must still fail the full block decoder");++refusals;
  check(raises<std::length_error>([&]{(void)disk.header_by_number(0,1);}),"indexed header bytes must obey the existing snapshot cap");++refusals;
  check(!disk.header_by_number(1,cap)&&!disk.header_by_hash(Bytes(32,0xee),cap),"absent indexed/legacy blocks stay absent");
  for(const auto limit:{size_t(0),size_t(256*1024*1024)+1}){check(raises<std::invalid_argument>([&]{(void)disk.header_by_number(0,limit);}),"invalid header caps are rejected before reading");++refusals;}
  check(raises<std::invalid_argument>([&]{(void)disk.header_by_number(Int(1)<<64,cap);})&&raises<std::invalid_argument>([&]{(void)disk.header_by_hash(Bytes(31),cap);}),"height/hash API widths must be validated");++refusals;
 }
 check(records(path)==before,"read-only indexed header reads perform no logical writes or migration");
 put_owned(path,body_key,original_body);
 put_owned(path,index_key,std::string("{"));
 {ChainStore disk(path,ChainStore::OpenMode::ReadOnly);check(raises<std::runtime_error>([&]{(void)disk.header_by_number(0,cap);})&&raises<std::runtime_error>([&]{(void)disk.header_by_hash(genesis.block.header.hash(),cap);}),"present malformed header index must fail closed instead of falling back to a valid body");++refusals;}
 auto wrong=genesis.block.header;wrong.number=1;put_owned(path,index_key,wrong.to_json().dump());
 {ChainStore disk(path,ChainStore::OpenMode::ReadOnly);check(raises<std::runtime_error>([&]{(void)disk.header_by_number(0,cap);}),"derived header height must match its requested key");++refusals;}
 put_owned(path,index_key,original_header);const auto reverse_key="hash/"+hex(genesis.block.header.hash());
 put_owned(path,reverse_key,hex(bytes(Int(1),8)));
 {ChainStore disk(path,ChainStore::OpenMode::ReadOnly);check(raises<std::runtime_error>([&]{(void)disk.header_by_number(0,cap);}),"indexed header must remain bound to the canonical reverse index");++refusals;}
 put_owned(path,reverse_key,height);
 stage="bounded-legacy-header-long-token";
 auto body=Json::parse(original_body);body["aaaLegacyPadding"]=std::string(2*1024*1024,'x');const auto legacy=body.dump();
 put_owned(path,body_key,legacy);put_owned(path,index_key,std::nullopt);const auto legacy_before=records(path);
 {
  ChainStore disk(path,ChainStore::OpenMode::ReadOnly);
  const auto by_number=disk.header_by_number(0,cap),by_hash=disk.header_by_hash(genesis.block.header.hash(),cap);
  check(by_number&&by_hash&&by_number->to_json()==genesis.block.header.to_json()&&by_hash->to_json()==by_number->to_json(),"bounded legacy projection must preserve the exact header without reconstructing the transaction body");
  check(raises<std::length_error>([&]{(void)disk.header_by_number(0,16384);}),"legacy body over cap is refused before projection");++refusals;
  unsigned polls=0;bool cancelled=false;
  try{(void)disk.header_by_hash(genesis.block.header.hash(),cap,[&]{if(++polls==12)throw ExecutionCancelled("synthetic long-token header cancellation");});}catch(const ExecutionCancelled& error){cancelled=std::string(error.what())=="synthetic long-token header cancellation";}
  // by_hash: six setup checks, then a check every 4096 scanned bytes. The
  // alphabetically first 2 MiB value is one token: poll 12 is inside it.
  check(cancelled&&polls==12,"legacy scan must poll inside a single long JSON string, not only between SAX events");++refusals;
 }
 check(records(path)==legacy_before&&!records(path).contains(index_key),"legacy read-only projection never backfills the header index");
 put_owned(path,body_key,std::string("{\"aaaLegacyPadding\":\"")+std::string(65536,'x'));
 {ChainStore disk(path,ChainStore::OpenMode::ReadOnly);check(raises<std::runtime_error>([&]{(void)disk.header_by_number(0,cap);}),"truncated legacy JSON cannot produce a header");++refusals;}

 stage="legacy-selection-cheap-rpc-and-real-commit";
 // Unknown JSON members are valid in the historical Block codec. This owned
 // legacy body therefore remains a valid full block for ordinary startup/mine.
 put_owned(path,body_key,legacy);
 Blockchain chain(path,genesis,ChainStore::OpenMode::ReadWriteExisting);Rpc rpc(chain);
 std::promise<void> entered,release;auto arrived=entered.get_future();auto gate=release.get_future().share();
 std::future<Json> read,cheap;std::future<Block> writer;
 struct Release {std::promise<void>& value;bool done=false;void go(){if(!done){done=true;value.set_value();}}~Release(){go();}} guard{release};
 unsigned polls=0;
 read=std::async(std::launch::async,[&]{return rpc.handle({{"jsonrpc","2.0"},{"id",1},{"method","eth_getBalance"},{"params",Json::array({"0x00000000000000000000000000000000000000a1",Json{{"blockHash",hex(genesis.block.header.hash())},{"requireCanonical",true}}})}},[&]{if(++polls==32){entered.set_value();if(gate.wait_for(std::chrono::seconds(10))!=std::future_status::ready)throw ExecutionCancelled("synthetic legacy selection gate deadline");}});});
 // Poll32 is still inside that first long token, before the selected header
 // and state snapshot exist. Both operations must finish before releasing it.
 const bool paused=arrived.wait_for(std::chrono::seconds(5))==std::future_status::ready;
 if(paused){cheap=std::async(std::launch::async,[&]{return rpc.handle({{"jsonrpc","2.0"},{"id",2},{"method","eth_chainId"},{"params",Json::array()}});});writer=std::async(std::launch::async,[&]{return chain.mine("0x00000000000000000000000000000000000000cc",12);});}
 const bool cheap_progress=paused&&cheap.wait_for(std::chrono::seconds(2))==std::future_status::ready;
 const bool writer_progress=paused&&writer.wait_for(std::chrono::seconds(2))==std::future_status::ready;
 guard.go();last=read.get();check(paused&&cheap_progress&&writer_progress,"cheap RPC and a real block commit must progress while legacy header projection is paused outside Rpc/Chain/Store locks");
 const auto committed=writer.get();check(cheap.get().at("result")=="0x539"&&last.at("result")=="0x0"&&committed.header.number==1&&chain.tip().header.hash()==committed.header.hash(),"resumed request keeps Genesis selection while the real writer publishes its next block");
 check(chain.store().header_by_number(1,cap)->hash()==committed.header.hash(),"the concurrent ordinary commit must publish its derived header index too");
}



struct ReadyBeacon {
 const du::Fixture& fixture;du::Temp directory;
 std::unique_ptr<Blockchain> owner;
 std::unique_ptr<ChainStore> witness;
 std::unique_ptr<CapellaDurableReactor> reactor;
 explicit ReadyBeacon(const du::Fixture& f):fixture(f),owner(std::make_unique<Blockchain>(directory.path/"primary",f.genesis)){}
 ~ReadyBeacon(){close();}
 void start(){witness=std::make_unique<ChainStore>(directory.path/"witness");reactor=std::make_unique<CapellaDurableReactor>(*owner,fixture.rules,fixture.limits,fixture.initial,du::checkpoint,witness.get());}
 void close(){reactor.reset();witness.reset();owner.reset();}
 void at(uint64_t slot){reactor->tick(fixture.initial.timed.clock.genesis_time_ms+slot*fixture.rules.slot_duration_ms,du::checkpoint);}
 void admit(const Bytes& wire){
  bool accepted=false;for(size_t attempts=0;attempts<8&&!accepted;++attempts){const auto value=reactor->block_admission(wire,du::checkpoint);du::check(value!=CapellaBlockAdmission::rejected,"public ready fixture child must not be rejected");accepted=value==CapellaBlockAdmission::accepted;}
  du::check(accepted,"bounded real ready fixture admission");
 }
};
Bytes ready_child(ReadyBeacon& producer,uint64_t slot){
 du::check(du::test_signatures<=30,"at most32 authorized public test signatures");
 producer.at(slot);const auto& f=producer.fixture;const auto& t=f.rules.transition;
 for(size_t tries=0;!producer.reactor->prepare_current_duties(slot,du::checkpoint);++tries)du::check(tries<8,"finite public preparation");
 const auto index=producer.reactor->current_proposer(slot,du::checkpoint);du::check(bool(index),"public synthetic proposer");
 const auto current=producer.reactor->current();const auto parent=capella_fork_head(*current.state,f.rules,du::checkpoint);
 const auto context=capella_fork_block_context(parent->state,parent->header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state,du::checkpoint);
 const auto epoch=slot/t.consensus.epoch.ffg.slots_per_epoch;const auto version=epoch<context.fork.epoch?context.fork.previous_version:context.fork.current_version;
 du::TestKey key(*index);du::check(key.public_key==Bytes(context.registry.validators.at(*index).public_key.begin(),context.registry.validators.at(*index).public_key.end()),"synthetic key actually selected");
 ConsensusRoot epoch_root{};for(size_t i=0;i<8;++i)epoch_root[i]=uint8_t(epoch>>(8*i));
 const auto reveal=key.sign(consensus_signing_root(epoch_root,consensus_domain({2,0,0,0},version,context.genesis_root)));
 const auto body=producer.reactor->prepare_local_body(*index,slot,reveal,parent->execution_block.header.coinbase,current.token,{},1,64,du::checkpoint);
 du::check(bool(body),"normal complete public body");const auto parent_root=consensus_beacon_header_root(parent->header);
 auto history=[&](const Int& n){return capella_execution_history(*current.state,parent_root,n,du::checkpoint);};
 auto candidate=prepare_capella_unsigned_block(parent->state,slot,*index,parent_root,*body,t,parent->execution_block.header,parent->execution_state,history,f.limits.replay_slots,f.limits.sync_trials,du::checkpoint);
 du::check(bool(candidate),"independent complete child transition");const auto& message=candidate->message_ssz;
 const auto body_root=capella_beacon_body_root(std::span<const uint8_t>(message).subspan(84),t.consensus.body,t.consensus.state.sync_committee_size,t.execution,t.withdrawals.maximum_per_payload,t.maximum_bls_changes,t.consensus.maximum_block_bytes,du::checkpoint);
 const ConsensusBeaconHeader header{du::test_little(message,0),du::test_little(message,8),du::test_root(message,16),du::test_root(message,48),body_root};
 const auto signature=key.sign(consensus_signing_root(consensus_beacon_header_root(header),consensus_domain({0,0,0,0},version,context.genesis_root)));
 Bytes wire(100);du::test_put(wire,0,100,4);std::copy(signature.begin(),signature.end(),wire.begin()+4);wire.insert(wire.end(),message.begin(),message.end());producer.admit(wire);return wire;
}
void beacon_context_cases(const Check& check,size_t& refusals,std::string& stage,Json& last){
 stage="public-beacon-ready-fixture";const auto f=du::fixture();std::vector<Bytes> wires;std::vector<Header> headers;
 {ReadyBeacon producer(f);producer.start();for(uint64_t slot=2;slot<=6;++slot){wires.push_back(ready_child(producer,slot));headers.push_back(producer.owner->tip_header());}}
 check(wires.size()==5&&du::test_signatures==12,"exact public fixture bootstrap plus five genuine signed children, bounded below32 signatures");
 ReadyBeacon receiver(f);bool unready=false;
 try{(void)receiver.owner->state_read_context(65536);}catch(const std::runtime_error& error){unready=std::string(error.what())=="Beacon execution unavailable until checkpoint verification";}
 check(unready,"header-only context must preserve the exact Beacon not-ready refusal");++refusals;
 check(raises<std::runtime_error>([&]{Rpc refused_rpc(*receiver.owner);}),"the existing public Rpc constructor must also refuse an unadmitted Beacon owner");++refusals;
 receiver.start();const auto initial_tip=receiver.owner->tip_header();const auto finalized=receiver.owner->finalized_block();check(bool(finalized),"real admitted fixture has a finalized execution header");
 const auto context=receiver.owner->state_read_context(65536);check(context.tip.to_json()==initial_tip.to_json()&&context.finalized&&context.finalized->to_json()==finalized->header.to_json(),"ready context returns the exact real reactor head and finalized header");
 auto rpc=std::make_unique<Rpc>(*receiver.owner);const std::string account="0x00000000000000000000000000000000000000ff";
 for(const auto* tag:{"latest","finalized"}){
  const auto header=std::string(tag)=="latest"?initial_tip:finalized->header;const auto state=receiver.owner->store().state_at(header.number);check(bool(state),"the selected ready state must really exist");
  last=rpc->handle({{"jsonrpc","2.0"},{"id",1},{"method","eth_getBalance"},{"params",Json::array({account,tag})}});check(last.contains("result")&&!last.contains("error")&&last.at("result")==quantity(state->get(account).balance),"ready latest/finalized balance is exactly the selected persisted state");
 }
 receiver.at(6);stage="beacon-header-context-real-publication-retry";unsigned polls=0;bool changed=false;
 const auto retried=receiver.owner->state_read_context(65536,[&]{if(++polls==2){receiver.admit(wires[0]);changed=true;}});
 check(changed&&retried.tip.to_json()==headers[0].to_json()&&retried.finalized&&retried.finalized->to_json()==finalized->header.to_json(),"one genuine child publication after capture retries to its exact new header with unchanged finality");
 stage="beacon-header-context-bounded-real-publications";polls=0;unsigned changes=0;bool cancelled=false;
 try{(void)receiver.owner->state_read_context(65536,[&]{++polls;if(polls==2||polls==15||polls==28||polls==41){receiver.admit(wires.at(1+changes));++changes;}});}catch(const ExecutionCancelled& error){cancelled=std::string(error.what())=="RPC state-read head changed during capture";}
 // Each indexed Beacon attempt has13 callbacks: capture1/2, durable header
 // 3..12, then13 before cache/event comparison. No fixture header exceeds4096.
 check(cancelled&&changes==4&&polls==52,"four genuine coherent child publications exhaust the fixed context retry bound");++refusals;
 const auto stable=receiver.owner->state_read_context(65536);check(stable.tip.to_json()==headers.back().to_json()&&stable.finalized&&stable.finalized->hash()==finalized->header.hash(),"bounded activity refusal leaves the next stable ready read usable");
 // A full A-to-B-to-A Beacon fork-choice case is not claimed by this linear
 // child fixture. The production event-sequence comparison has source review.
 stage="beacon-header-context-stable-cache-durable-mismatch";
 const auto primary=receiver.owner->store().fork_checkpoint(f.genesis.chain_id,f.genesis.block.header.hash(),f.limits.envelope_bytes);check(bool(primary),"independent primary checkpoint token before the intentional mismatch");
 auto& disk=const_cast<ChainStore&>(receiver.owner->store());
 (void)disk.commit_execution_checkpoint(f.genesis.chain_id,f.genesis.block.header.hash(),primary->token,headers.back().hash(),headers[3].number,headers[3].hash(),{},primary->snapshot,f.limits.envelope_bytes,f.limits.blocks);
 bool mismatch=false;try{(void)receiver.owner->state_read_context(65536);}catch(const std::runtime_error& error){mismatch=std::string(error.what())=="Beacon canonical cache differs from durable head";}
 check(mismatch,"a stable cached/durable head contradiction remains a fatal readiness refusal");++refusals;
 last=rpc->handle({{"jsonrpc","2.0"},{"id",1},{"method","eth_getBalance"},{"params",Json::array({account,"latest"})}});check(last.contains("error")&&!last.contains("result")&&last.at("error").at("code")==-32603,"corrupt ready context returns an internal error without a stale result");++refusals;
 check(raises<std::runtime_error>([&]{receiver.owner->require_execution_ready();}),"the existing global body/readiness guard still rejects that same contradiction");++refusals;
 rpc.reset();receiver.close();stage="beacon-header-context-legacy-certificate-refusal";
 // This fixture is terminal and all owners are closed. No future duty/reopen
 // is attempted with its intentionally mismatched primary/witness checkpoints.
 const auto path=receiver.directory.path/"primary";put_owned(path,"meta/certified-tip",hex(bytes(Int(0),8)));
 {ChainStore disk(path,ChainStore::OpenMode::ReadOnly);check(raises<std::runtime_error>([&]{(void)disk.uncertified_tip_header(65536);}),"header-only Beacon path refuses legacy certified-tip metadata");++refusals;}
 put_owned(path,"meta/certified-tip",std::nullopt);put_owned(path,"certificate/"+hex(bytes(Int(0),8)),std::string("{}"));
 {ChainStore disk(path,ChainStore::OpenMode::ReadOnly);check(raises<std::runtime_error>([&]{(void)disk.uncertified_tip_header(65536);}),"header-only Beacon path also refuses orphan legacy certificate records");++refusals;}
 check(du::test_signatures==12,"real ready/mismatch cases create no additional test signatures");
}

}

int main(int argc,char** argv){size_t checks=0,refusals=0;Json last;std::string stage="arguments";try{
 if(argc!=2||std::string(argv[1])!="--state-read-cost-self-test")throw std::invalid_argument("expected --state-read-cost-self-test");
 const auto check=[&](bool value,const char* why){++checks;if(!value)throw std::runtime_error(why);};
 auto text=(std::filesystem::temp_directory_path()/"nano-state-read-cost-XXXXXX").string();std::vector<char> name(text.begin(),text.end());name.push_back(0);const auto made=::mkdtemp(name.data());if(!made)throw std::runtime_error("cannot create owned namespace");struct Clean {std::filesystem::path path;~Clean(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}} clean{made};
 const ChainStoreMemoryPolicy policy{4*1024*1024,2,8*1024*1024,4*1024*1024};ScopedChainStoreMemoryPolicy memory(policy);
 Json alloc=Json::object();for(unsigned i=0;i<128;++i){Json slots=Json::object();for(unsigned k=0;k<16;++k)slots[quantity(Int(k))]=quantity(Int(k+7));alloc[address(hex(bytes(Int(i+4096),20))) ]={{"nonce","0x1"},{"balance","0x1"},{"code","0x00"},{"storage",slots}};}
 auto genesis=Genesis::from_json({{"config",{{"chainId",1337},{"evmFork","Shanghai"}}},{"alloc",alloc}});Blockchain chain(clean.path/"chain",genesis);Rpc rpc(chain);const auto target=address(hex(bytes(Int(4096),20)));const auto before=chain.state().to_json();
 for(const auto* method:{"eth_getBalance","eth_getCode","eth_getTransactionCount","eth_getStorageAt"}){stage=std::string("cancel-")+method;Json params=Json::array({target});if(std::string(method)=="eth_getStorageAt")params.push_back("0x0");params.push_back("latest");unsigned polls=0;last=rpc.handle({{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",params}},[&]{if(++polls==64)throw ExecutionCancelled("synthetic bounded state decode cancellation");});check(last.contains("error")&&last["error"]["code"]==-32002&&last["error"]["message"]=="synthetic bounded state decode cancellation"&&polls==64,"each old state reader must now cancel inside bounded decode/root work");++refusals;}
 stage="mutex-release-during-decode";std::promise<void> entered,release;auto entered_future=entered.get_future();auto gate=release.get_future().share();bool signaled=false;unsigned polls=0;std::future<Json> read,cheap;
 // Release guard is declared after futures, so it releases before future joins
 // during unwinding. Both futures finish before Rpc/Blockchain destruction.
 struct Release {std::promise<void>& value;bool done=false;void go(){if(!done){done=true;value.set_value();}}~Release(){go();}} guard{release};
 read=std::async(std::launch::async,[&]{return rpc.handle({{"jsonrpc","2.0"},{"id",1},{"method","eth_getBalance"},{"params",Json::array({target,"latest"})}},[&]{if(++polls==64){signaled=true;entered.set_value();if(gate.wait_for(std::chrono::seconds(5))!=std::future_status::ready)throw ExecutionCancelled("synthetic gate deadline");}});});
 const bool paused=entered_future.wait_for(std::chrono::seconds(5))==std::future_status::ready;if(paused)cheap=std::async(std::launch::async,[&]{return rpc.handle({{"jsonrpc","2.0"},{"id",2},{"method","eth_chainId"},{"params",Json::array()}});});
 const bool progressed=paused&&cheap.wait_for(std::chrono::seconds(2))==std::future_status::ready;guard.go();last=read.get();check(paused&&signaled&&progressed,"cheap RPC must finish while a captured state decode is paused outside Rpc mutex");check(cheap.get().at("result")=="0x539"&&last.at("result")=="0x1","both cheap and resumed captured reads must return exact fixture values");check(chain.state().to_json()==before,"cancellation and captured reads do not mutate canonical state");
 header_cost_cases(clean.path,check,refusals,stage,last);
 beacon_context_cases(check,refusals,stage,last);du::checkpoint();
 std::cout<<Json{{"scenario","state-read-cost-summary"},{"passed",true},{"groups",1},{"checks",checks},{"expectedRefusals",refusals},{"accounts",128},{"slotsPerAccount",16},{"headerSelectionCases",true},{"beaconReadyCases",true},{"fullBeaconAbaQualified",false},{"newTestOnlyBlsSignatures",du::test_signatures},{"maximumTestBlsSignatures",32},{"actualPeakRssKiB",du::public_peak_rss_kib()},{"maximumRssKiB",131072},{"runtimeDutiesAuthorized",false},{"privateInputsRead",false},{"externalNetwork",false}}.dump()<<std::endl;return 0;
 }catch(const std::exception& error){std::cerr<<Json{{"scenario","state-read-cost-summary"},{"passed",false},{"stage",stage},{"checks",checks},{"expectedRefusals",refusals},{"lastRpcResponse",last},{"error",error.what()}}.dump()<<std::endl;return 1;}}
