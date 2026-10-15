#include "nano/http.hpp"
#include "nano/simulation_queue.hpp"
#include "nano/rpc_timing.hpp"
#include "support/process.hpp"
#include "support/persistent_rpc.hpp"
#include <atomic>
#include <cctype>
#include <future>
#include <sys/resource.h>
using namespace nano;
using namespace std::chrono_literals;
namespace {
unsigned groups=0,signatures=0;std::string stage="start";
void require(bool value,const std::string& message){if(!value)throw std::runtime_error(message);}
uint64_t peak(){rusage r{};require(getrusage(RUSAGE_SELF,&r)==0,"getrusage");
#ifdef __APPLE__
 return uint64_t(r.ru_maxrss)/1024;
#else
 return uint64_t(r.ru_maxrss);
#endif
}
void emit(const char* name,Json fields){++groups;fields["scenario"]=name;fields["fork"]="Shanghai";fields["privateInputsRead"]=false;fields["productionQualified"]=false;fields["actualPeakRssKiB"]=peak();std::cout<<fields.dump()<<std::endl;}
std::string a(unsigned n){return hex(bytes(Int(n),20));}
std::string k(unsigned n){return hex(bytes(Int(n),32));}
Json row(const std::string& address,std::initializer_list<unsigned> keys={}){Json out={{"address",address},{"storageKeys",Json::array()}};for(auto key:keys)out["storageKeys"].push_back(k(key));return out;}
Json request(const char* method,Json params){return {{"jsonrpc","2.0"},{"id",17},{"method",method},{"params",std::move(params)}};}
Json result(Rpc& rpc,Json call,Json tag="latest",Json overrides=nullptr){
 auto params=Json::array({std::move(call),std::move(tag)});if(!overrides.is_null())params.push_back(std::move(overrides));auto response=rpc.handle(request("eth_createAccessList",std::move(params)));
 require(response.contains("result"),"access-list RPC must exist and execute: "+response.dump());auto out=response.at("result");
 require(out.is_object()&&out.contains("accessList")&&out.at("accessList").is_array()&&out.contains("gasUsed")&&out.at("gasUsed").is_string(),"access-list result shape");
 require(quantity(out.at("gasUsed"))>0,"access-list gasUsed is positive");return out;
}
void top_error(Rpc& rpc,Json call,int code,Json tag="latest"){auto r=rpc.handle(request("eth_createAccessList",Json::array({std::move(call),std::move(tag)})));require(r.contains("error")&&r.at("error").at("code")==code&&!r.contains("result"),"expected top-level error: "+r.dump());}
Json sorted(Json list){std::map<std::string,std::set<std::string>> keys;for(const auto& r:list){auto address=r.at("address").get<std::string>();for(char& c:address)c=char(std::tolower(static_cast<unsigned char>(c)));auto& s=keys[address];for(const auto& v:r.at("storageKeys"))s.insert(v.get<std::string>());}Json out=Json::array();for(const auto& [address,s]:keys)out.push_back({{"address",address},{"storageKeys",s}});return out;}
const std::string read_two="0x600054506001545000",write_one="0x602a6000556000545000";
const std::string revert_write="0x602a6000556007545060006000fd";
// GAS > 77000 chooses slot 1 on the first replay; adding its intrinsic
// access-list cost selects slot 0. A replacement-only collector oscillates.
const std::string gas_branch="0x5a62012cc810600e5760005450005b6001545000";
struct Fixture {
 test::Temp tmp;Bytes key=bytes(Int(101),32);std::string sender=public_address(public_key(key));
 std::string storage=a(0xa1),writer=a(0xa2),reverting=a(0xa3),accounts=a(0xa4),outer=a(0xa5),branch=a(0xa6),create=a(0xa7),refund=a(0xa8),create2=a(0xa9),long_running=a(0xab);
 Genesis genesis;std::unique_ptr<Blockchain> chain;std::unique_ptr<Rpc> rpc;
 Fixture(){
  Json alloc={{sender,{{"balance","0x100000000000000000000"}}},{storage,{{"code",read_two}}},{writer,{{"code",write_one}}},{reverting,{{"code",revert_write}}},{branch,{{"code",gas_branch}}},{refund,{{"code","0x600060005500"},{"storage",{{k(0),k(1)}}}}}};
  alloc[accounts]={{"code","0x73"+a(0xb1).substr(2)+"315073"+a(0xb2).substr(2)+"3b5073"+a(0xb3).substr(2)+"3f5000"}};
  // Child failure is ignored by the parent. Its slot-7 touch must survive
  // tracing even though both its state and warm-set changes are reverted.
  alloc[outer]={{"code","0x6000600060006000600073"+reverting.substr(2)+"5af1506003545000"}};
  // Copy nine init-code bytes at offset 16; CREATE returns no runtime code.
  alloc[create]={{"code","0x60096010600039600960006000f050006000545060006000f3"}};
  alloc[create2]={{"code","0x600960126000396000600960006000f550006000545060006000f3"}};
  std::string long_code="0x";for(unsigned i=0;i<32768;++i)long_code+="5b";long_code+=read_two.substr(2);alloc[long_running]={{"code",long_code}};
  genesis=Genesis::from_json({{"config",{{"chainId",1337},{"evmFork","Shanghai"}}},{"alloc",alloc}});
  chain=std::make_unique<Blockchain>(tmp/"db",genesis);rpc=std::make_unique<Rpc>(*chain);
 }
 Json call(const std::string& target,uint64_t gas=100000)const{return {{"from",sender},{"to",target},{"gas",quantity(Int(std::to_string(gas)))},{"gasPrice","0x77359400"}};}
 Json before()const{return {{"root",hex(chain->state().root())},{"head",hex(chain->tip().header.hash())},{"pool",chain->pool_info(true)},{"events",chain->event_cursor()}};}
 void unchanged(const Json& prior)const{require(before()==prior,"access-list simulation changed canonical state/head/pool/events");}
 Receipt execute_copy(Json call,const Json& access){call["accessList"]=access;auto tx=Transaction::from_json(call);auto state=chain->state();auto signed_tx=SignedTransaction::sign(tx,key);++signatures;return ExecutionEngine(genesis.chain_id,{},EvmRevision::Shanghai).apply_transaction(state,signed_tx,chain->tip().header,0,0);}
};
void discovery(Fixture& f){stage="discovery";const auto before=f.before();
 auto legacy=[&](const char* method){auto response=f.rpc->handle(request(method,Json::array({f.call(f.storage),"latest"})));require(response.contains("result"),"existing call/estimate path must keep working");return response.at("result");};auto old_call=legacy("eth_call"),old_estimate=legacy("eth_estimateGas");require(old_call=="0x"&&quantity(old_estimate)>0,"existing call/estimate fixture oracle");
 auto r=result(*f.rpc,f.call(f.storage));require(r.at("accessList")==Json::array({row(f.storage,{0,1})})&&!r.contains("error"),"recipient storage discovers two sorted keys");
 require(f.execute_copy(f.call(f.storage),r.at("accessList")).gas_used==quantity(r.at("gasUsed")),"gasUsed equals actual Shanghai execution with stabilized list");
 auto accounts=result(*f.rpc,f.call(f.accounts));require(accounts.at("accessList")==Json::array({row(a(0xb1)),row(a(0xb2)),row(a(0xb3))}),"BALANCE/EXTCODESIZE/EXTCODEHASH discover addresses");
 auto writes=result(*f.rpc,f.call(f.writer));require(writes.at("accessList")==Json::array({row(f.writer,{0})}),"SSTORE discovery retained");
 auto refund=result(*f.rpc,f.call(f.refund));auto receipt=f.execute_copy(f.call(f.refund),refund.at("accessList"));require(receipt.success&&receipt.gas_used==quantity(refund.at("gasUsed")),"gasUsed includes Shanghai SSTORE refund cap");
 require(legacy("eth_call")==old_call&&legacy("eth_estimateGas")==old_estimate,"access-list discovery must not alter existing call/estimate semantics");f.unchanged(before);emit("access-list-discovery-and-gas",{{"cases",4},{"executionReceiptComparisons",2},{"statePoolHeadEventsUnchanged",true},{"gasIncludesFinalListAndRefund",true},{"existingCallAndEstimateUnchanged",true}});
}
void normalization(Fixture& f){stage="normalization-fixpoint";const auto before=f.before();auto call=f.call(f.storage);call["accessList"]=Json::array({row(a(0xc2),{3,3}),row(f.storage,{1}),row(a(0xc2),{2}),row(f.sender),row(a(1))});
 auto r=result(*f.rpc,call);require(r.at("accessList")==Json::array({row(f.storage,{0,1}),row(a(0xc2),{2,3})}),"dedup/sort seeds, preserve unused seed, exclude sender/precompile address-only seeds");
 auto repeat=call;repeat["accessList"]=r.at("accessList");require(result(*f.rpc,repeat)==r,"stable normalized list is a deterministic fixed point");
 auto excluded=f.call(f.storage);excluded["accessList"]=Json::array({row(f.sender,{8}),row(a(1),{7})});require(result(*f.rpc,excluded).at("accessList")==sorted(Json::array({row(f.storage,{0,1}),row(f.sender,{8}),row(a(1),{7})})),"explicit seeded storage on excluded accounts is retained");
 auto branch=result(*f.rpc,f.call(f.branch));require(branch.at("accessList")==Json::array({row(f.branch,{0,1})}),"gas-dependent replays grow monotonically across paths");
 auto again=f.call(f.branch);again["accessList"]=branch.at("accessList");require(result(*f.rpc,again)==branch,"gas-dependent final list and gas stabilize");
 f.unchanged(before);emit("access-list-normalized-monotone-fixed-point",{{"seedRows",5},{"deduplicatedUnusedSeedPreserved",true},{"gasDependentBothBranchesRetained",true},{"repeatedResultByteExact",true},{"statePoolHeadEventsUnchanged",true}});
}
void failed_and_create(Fixture& f){stage="revert-create";const auto before=f.before();auto failed=result(*f.rpc,f.call(f.reverting));
 require(failed.contains("error")&&failed.at("error").is_string()&&failed.at("error").get<std::string>().find("revert")!=std::string::npos,"VM revert is result.error, not JSON-RPC error");
 require(failed.at("accessList")==Json::array({row(f.reverting,{0,7})}),"top-level reverted accesses retained");
 auto receipt=f.execute_copy(f.call(f.reverting),failed.at("accessList"));require(!receipt.success&&receipt.gas_used==quantity(failed.at("gasUsed")),"reverted gasUsed matches actual receipt");
 auto nested=result(*f.rpc,f.call(f.outer));require(!nested.contains("error")&&nested.at("accessList")==Json::array({row(f.reverting,{0,7}),row(f.outer,{3})}),"nested reverted accesses survive observer while parent succeeds");
 auto created=create_address(f.create,0);auto creation=result(*f.rpc,f.call(f.create,250000));require(creation.at("accessList")==Json::array({row(created,{0})}),"CREATE child storage trace uses created address");
 auto created2=create2_address(f.create2,Bytes(32,0),keccak(unhex("0x6000545060006000f3")));require(result(*f.rpc,f.call(f.create2,250000)).at("accessList")==Json::array({row(created2,{0})}),"CREATE2 child storage trace uses derived address");
 auto top=f.call(f.create,250000);top.erase("to");top["data"]="0x6000545060006000f3";auto top_created=create_address(f.sender,0);auto top_result=result(*f.rpc,top);require(top_result.at("accessList")==Json::array({row(top_created,{0})}),"top-level init-code storage traces under derived address");
 top["nonce"]="0x9";require(result(*f.rpc,top).at("accessList")==Json::array({row(top_created,{0})}),"explicit caller nonce does not replace state nonce used by VM CREATE");require(result(*f.rpc,top,"latest",Json{{f.sender,{{"nonce","0x9"}}}}).at("accessList")==Json::array({row(create_address(f.sender,9),{0})}),"state override nonce changes actual creation address before simulation defaults");
 f.unchanged(before);emit("access-list-revert-and-create",{{"cases",5},{"vmErrorInsideResult",true},{"nestedRevertTouchesRetained",true},{"creationStorageAddressExact",true},{"statePoolHeadEventsUnchanged",true}});
}
void contexts(Fixture& f){stage="contexts-fees";const std::string target=a(0xd1);
 // Nonempty calldata sets slot 0 to 1. An empty call reads slot 0 and the
 // storage key selected by that value, so pending/history differ observably.
 auto genesis=Genesis::from_json({{"config",{{"chainId",1337},{"evmFork","Shanghai"}}},{"alloc",{{f.sender,{{"balance","0x100000000000000000000"}}},{target,{{"code","0x3615600b576001600055005b600054545000"}}}}}});
 Blockchain chain(f.tmp/"contexts",genesis);Rpc rpc(chain);auto c=f.call(target);auto original=result(rpc,c);auto default_response=rpc.handle(request("eth_createAccessList",Json::array({c})));require(default_response.at("result")==original,"omitted selector means latest");
 Transaction tx;tx.to=target;tx.data={1};tx.gas=100000;tx.gas_price=2000000000;require(chain.submit(SignedTransaction::sign(tx,f.key)),"pending fixture signed tx");++signatures;
 const auto root=chain.state().root();const auto events=chain.event_cursor();const auto pool=chain.pool_info(true);auto pending=result(rpc,c,"pending");
 require(original.at("accessList")==Json::array({row(target,{0})})&&pending.at("accessList")==Json::array({row(target,{0,1})}),"pending pool execution changes accessed slot while latest remains genesis");
 require(chain.state().root()==root&&chain.event_cursor()==events&&chain.pool_info(true)==pool,"pending access-list replay does not commit pool execution");
 chain.mine(f.sender,12);auto historical=result(rpc,c,"0x0");require(historical==original,"historical state and gas are stable after import");auto latest=result(rpc,c);require(latest.at("accessList")==Json::array({row(target,{0,1})}),"latest uses committed storage context");
 require(result(rpc,c,Json{{"blockNumber","0x0"}})==historical&&result(rpc,c,Json{{"blockHash",hex(genesis.block.header.hash())},{"requireCanonical",true}})==historical,"EIP-1898 selects the same canonical historical context");
 auto override=Json{{target,{{"stateDiff",{{k(0),k(7)}}}}}};require(result(rpc,c,"0x0",override).at("accessList")==Json::array({row(target,{0,7})}),"state overrides apply only to selected historical copy");require(result(rpc,c,"0x0")==historical,"state override leaves original historical view unchanged");
 require(result(rpc,c,"latest",Json{{target,{{"state",Json::object()}}}}).at("accessList")==Json::array({row(target,{0})}),"full state override replaces rather than merges storage");require(result(rpc,c,"latest",Json{{target,{{"code","0x6009545000"}}}}).at("accessList")==Json::array({row(target,{9})}),"code override is used only for this execution copy");auto funded=c;funded["from"]=a(0xffff);require(!result(rpc,funded,"latest",Json{{a(0xffff),{{"balance","0x100000000000000000000"}}}}).contains("error"),"balance override participates in copied affordability");require(result(rpc,c)==latest,"code/full-state/balance overrides do not alter latest state");
 auto dynamic=c;dynamic.erase("gasPrice");dynamic["maxFeePerGas"]="0xb2d05e00";dynamic["maxPriorityFeePerGas"]="0x7";require(!result(rpc,dynamic).contains("error"),"dynamic fee request accepted");
 auto bad=dynamic;bad["gasPrice"]="0x1";top_error(rpc,bad,-32602);bad=dynamic;bad["maxPriorityFeePerGas"]="0xb2d05e01";top_error(rpc,bad,-32602);bad=c;bad["chainId"]="0x1";top_error(rpc,bad,-32602);bad=c;bad["from"]=a(0xffff);bad["value"]="0x1";top_error(rpc,bad,-32000);top_error(rpc,c,-32000,"0x99");bad=c;bad["gasPrice"]="0x0";top_error(rpc,bad,-32602);bad=dynamic;bad["maxFeePerGas"]="0x0";bad["maxPriorityFeePerGas"]="0x0";top_error(rpc,bad,-32602);
 emit("access-list-contexts-and-fees",{{"omittedSelectorLatest",true},{"pendingExecutedOnCopy",true},{"historicStableAfterImport",true},{"dynamicFeesValidated",true},{"supportedStateOverrideFields",5},{"invalidRequestsTopLevel",7}});
}
void quotas_and_cancel(Fixture& f){stage="quotas-cancellation";const auto before=f.before();
 for(unsigned kind=0;kind<3;++kind){RpcSimulationLimits l;if(kind==0)l.maximum_access_list_iterations=1;if(kind==1)l.maximum_access_list_addresses=1;if(kind==2)l.maximum_access_list_storage_keys=1;Rpc limited(*f.chain,{},AdmissionBudget::Clock::now,l);top_error(limited,f.call(kind==1?f.accounts:f.storage),-32005);}
 RpcSimulationLimits seed_limit;seed_limit.maximum_access_list_addresses=1;Rpc seed_bounded(*f.chain,{},AdmissionBudget::Clock::now,seed_limit);auto seeded=f.call(f.storage);seeded["accessList"]=Json::array({row(a(0xc1)),row(a(0xc2))});top_error(seed_bounded,seeded,-32005);
 const auto parsed=RpcSimulationLimits::from_json({{"timeout_ms",1234},{"maximum_access_list_iterations",3},{"maximum_access_list_addresses",4},{"maximum_access_list_storage_keys",5}});require(parsed.timeout==1234ms&&parsed.maximum_access_list_iterations==3&&parsed.maximum_access_list_addresses==4&&parsed.maximum_access_list_storage_keys==5,"startup limits parse exact bounded integers");
 for(const auto& bad:std::vector<Json>{{{"timeout_ms",0}},{{"timeout_ms",60001}},{{"maximum_access_list_iterations",true}},{{"maximum_access_list_addresses",1.5}},{{"maximum_access_list_storage_keys",-1}},{{"unexpected",1}}}){bool refused=false;try{(void)RpcSimulationLimits::from_json(bad);}catch(const std::invalid_argument&){refused=true;}require(refused,"invalid startup limits refuse without narrowing/truncation");}
 unsigned checkpoints=0;auto cancelled=f.rpc->handle(request("eth_createAccessList",Json::array({f.call(f.long_running,200000)})),[&]{if(++checkpoints==20)throw ExecutionCancelled("public access-list cancellation");});require(cancelled.at("error").at("code")==-32002&&checkpoints==20,"cancellation during execution is not VM error or successful empty list");
 RpcSimulationLimits deadline;unsigned ticks=0;const auto now=std::chrono::steady_clock::time_point{};deadline.timeout=1ms;deadline.now=[&]{return now+std::chrono::milliseconds(ticks++);};Rpc expired(*f.chain,{},AdmissionBudget::Clock::now,deadline);top_error(expired,f.call(f.storage),-32002);
 for(auto malformed:Json::array({Json::array({{{"address","0x12"},{"storageKeys",Json::array()}}}),Json::array({{{"address",f.storage},{"storageKeys",Json::array({"0x00"})}}})})){
  auto c=f.call(f.storage);c["accessList"]=malformed;top_error(*f.rpc,c,-32602);
 }
 for(const auto& override:std::vector<Json>{{{f.storage,{{"state",Json::object()},{"stateDiff",Json::object()}}}},{{f.storage,{{"nonce","0x10000000000000000"}}}},{{f.storage,{{"movePrecompileTo",a(2)}}}},{{f.storage,{{"stateDiff",{{"0x00",k(0)}}}}}}}){auto response=f.rpc->handle(request("eth_createAccessList",Json::array({f.call(f.storage),"latest",override})));require(response.contains("error")&&response.at("error").at("code")==-32602,"invalid state override is a top-level parameter error");}
 RpcSimulationLimits code_limit;code_limit.maximum_access_list_override_code_bytes=1;Rpc code_bounded(*f.chain,{},AdmissionBudget::Clock::now,code_limit);auto code_error=code_bounded.handle(request("eth_createAccessList",Json::array({f.call(f.storage),"latest",Json{{f.storage,{{"code","0x0000"}}}}})));require(code_error.contains("error")&&code_error.at("error").at("code")==-32005,"override code aggregate cap is enforced");
 f.unchanged(before);emit("access-list-budgets-and-cancellation",{{"quotaRefusals",5},{"startupConfigRefusals",6},{"cancellationTopLevel",true},{"deadlineTopLevel",true},{"malformedListsRefused",2},{"malformedOverridesRefused",4},{"statePoolHeadEventsUnchanged",true}});
}
void native_trace(Fixture& f){stage="native-preopcode-observer";State state;ExecutionContext context;context.revision=EvmRevision::Shanghai;context.origin=f.sender;context.caller=f.sender;context.address=f.storage;context.gas=5;context.code=unhex("0x600054");
 const std::set<std::string> excluded={f.sender,f.storage,a(1)};EvmAccessListTrace low({},excluded,8,8);auto failed=EVM(state).execute(context,low);require(!failed.success&&low.access_list().size()==1&&low.access_list()[0].address==f.storage&&low.access_list()[0].keys==std::vector<Bytes>{bytes(Int(0),32)},"pre-opcode SLOAD touch remains visible when dynamic/base gas fails");
 context.gas=100000;context.code=unhex("0x600055");EvmAccessListTrace underflow({},excluded,8,8);failed=EVM(state).execute(context,underflow);require(!failed.success&&underflow.storage_key_count()==1,"Geth SSTORE observer needs one key even when value underflows");
 context.code=unhex("0x60006000600073"+a(0xb4).substr(2)+"61fffff1");EvmAccessListTrace call({},excluded,8,8);failed=EVM(state).execute(context,call);require(!failed.success&&call.address_count()==1&&call.access_list()[0].address==a(0xb4),"CALL observer uses five-stack threshold before execution underflow");
 std::vector<Access> seed={{f.sender,{bytes(Int(8),32)}},{a(1),{bytes(Int(7),32)}}};EvmAccessListTrace seeded(seed,excluded,8,8);require(seeded.address_count()==2&&seeded.storage_key_count()==2,"excluded seeded accounts keep explicit storage keys in pinned Geth version");
 context.code=unhex("0x41315000");context.coinbase=a(0xb5);EvmAccessListTrace coinbase({},excluded,8,8);require(EVM(state).execute(context,coinbase).success&&coinbase.address_count()==1&&coinbase.access_list()[0].address==context.coinbase,"Shanghai warm coinbase is not a tracer exclusion");
 bool refused=false;try{EvmAccessListTrace bounded(seed,excluded,1,8);}catch(const EvmAccessListLimitExceeded&){refused=true;}require(refused,"native observer enforces seed budget before execution");
 context.code=unhex(read_two);EvmAccessListTrace sticky({},excluded,8,1);bool interrupted=false,partial_refused=false;try{(void)EVM(state).execute(context,sticky);}catch(const EvmAccessListLimitExceeded&){interrupted=true;}try{(void)sticky.access_list();}catch(const EvmAccessListLimitExceeded&){partial_refused=true;}require(interrupted&&partial_refused,"quota is a host exception and sticky observer cannot publish a partial list");
 emit("access-list-native-preopcode-and-seeds",{{"cases",7},{"oogAndUnderflowTouchesRetained",true},{"excludedSeedStorageRetained",true},{"warmCoinbaseObserved",true},{"nativeQuotaRefused",true},{"partialListAfterLimitRefused",true}});
}
struct Running {HttpServer& server;std::thread worker;explicit Running(HttpServer& s):server(s),worker([&]{server.run();}){}~Running(){server.stop();worker.join();}};
void transport(Fixture& f){stage="server-unlock-and-queue";const auto before=f.before();
 require(simulation_count(request("eth_createAccessList",Json::array({f.call(f.storage)})))==1,"access-list method counted as simulation");Json oversized=Json::array();for(unsigned i=0;i<9;++i){auto item=request("eth_createAccessList",Json::array({f.call(f.storage)}));if(i%2)item.erase("id");oversized.push_back(item);}require(simulation_count(oversized)==9,"notifications counted before batch dispatch");
 Json alloc={{f.sender,{{"balance","0x100000000000000000000"}}},{f.storage,{{"code",read_two}}}};auto genesis=Genesis::from_json({{"config",{{"chainId",1337},{"evmFork","Shanghai"}}},{"alloc",alloc}});Blockchain local(f.tmp/"transport",genesis);
 auto local_snapshot=[&]{return Json{{"root",hex(local.state().root())},{"head",hex(local.tip().header.hash())},{"pool",local.pool_info(true)},{"events",local.event_cursor()}};};const auto local_before=local_snapshot();
 const auto timing_dir=f.tmp/"timing";require(std::filesystem::create_directory(timing_dir),"fresh owned timing directory");require(setenv("NANO_RPC_TIMING_DIR",timing_dir.c_str(),1)==0,"enable public per-request timing");require(rpc_timing::directory()==timing_dir.string(),"timing directory was not initialized before fixture selection");const auto timing_file=timing_dir/("rpc-"+std::to_string(getpid())+".jsonl");
 std::atomic<bool> paused{false},release{false};RpcSimulationLimits limits;limits.timeout=10s;limits.now=[&]{
  // A completed round with this HTTP worker request id proves the current
  // calculation entered its unlocked executor. No magic callback count.
  if(!paused.load()&&rpc_timing::request){std::ifstream input(timing_file);std::string line;size_t bytes=0;while(std::getline(input,line)){bytes+=line.size();require(bytes<=2*1024*1024,"bounded public timing log");auto event=Json::parse(line,nullptr,false);if(event.is_object()&&event.value("request",uint64_t(0))==rpc_timing::request&&event.value("phase",std::string{})=="access_list_round"){paused=true;while(!release.load())std::this_thread::sleep_for(1ms);break;}}}return std::chrono::steady_clock::now();};Rpc rpc(local,{},AdmissionBudget::Clock::now,limits);HttpServer server(rpc,"127.0.0.1",0);Running running(server);
 std::string error;Json reply;std::jthread work([&]{try{PersistentRpc client(server.port());reply=client.call("eth_createAccessList",Json::array({f.call(f.storage)}));}catch(const std::exception& e){error=e.what();}});
 struct Unblock {std::atomic<bool>& flag;~Unblock(){flag=true;}} unblock{release};auto until=std::chrono::steady_clock::now()+5s;while(!paused.load()&&std::chrono::steady_clock::now()<until)std::this_thread::sleep_for(1ms);bool reached=paused;
 // A second client must use the event loop and Rpc lock while calculation
 // is deliberately paused; this fails if createAccessList stays inline.
 auto identity=std::async(std::launch::async,[&]{PersistentRpc client(server.port());return client.call("eth_chainId");});bool responsive=identity.wait_for(1s)==std::future_status::ready;release=true;work.join();auto chain_id=identity.get();
 boost::asio::io_context io;boost::beast::tcp_stream socket(io);socket.connect({boost::asio::ip::make_address("127.0.0.1"),server.port()});namespace http=boost::beast::http;http::request<http::string_body> batch(http::verb::post,"/",11);batch.set(http::field::host,"localhost");batch.body()=oversized.dump();batch.prepare_payload();http::write(socket,batch);boost::beast::flat_buffer buffer;http::response<http::string_body> batch_reply;http::read(socket,buffer,batch_reply);require(batch_reply.result_int()==503,"nine access-list requests including notifications refuse before dispatch");
 require(reached&&responsive&&chain_id=="0x539"&&error.empty()&&reply.contains("accessList"),"access-list calculation blocks identity/event-loop or RPC lock");require(local_snapshot()==local_before,"transport access-list job mutated served canonical state/head/pool/events");f.unchanged(before);
 emit("access-list-server-queue-and-unlock",{{"simulationClassification",true},{"notificationBatchReservations",9},{"oversizedBatchHttpStatus",503},{"identityWhileExecutionPaused",true},{"statePoolHeadEventsUnchanged",true}});
}
}
int main(int argc,char** argv){try{require(argc==2&&std::string(argv[1])=="--self-test","expected --self-test");Fixture fixture;discovery(fixture);normalization(fixture);failed_and_create(fixture);contexts(fixture);quotas_and_cancel(fixture);native_trace(fixture);transport(fixture);require(groups==7&&signatures<=32&&peak()<=131072,"bounded access-list consumer summary");std::cout<<Json{{"scenario","access-list-summary"},{"semanticGroups",groups},{"newSyntheticSignatures",signatures},{"actualPeakRssKiB",peak()},{"oldScenarioGroupsExecuted",0},{"privateInputsRead",false},{"runtimeDutiesAuthorized",false},{"productionQualified",false}}.dump()<<std::endl;return 0;}catch(const std::exception& e){std::cerr<<Json{{"stage",stage},{"reason",e.what()},{"privateInputsRead",false},{"productionQualified",false}}.dump()<<std::endl;return 1;}}
