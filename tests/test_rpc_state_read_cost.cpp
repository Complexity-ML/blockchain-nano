// Deliberately uses only the pre-existing Rpc constructor/layout contract. Root
// compiles a separate copy with old headers for the causal baseline, never links
// an object compiled with the new RpcSimulationLimits layout to an old Core.
#include "nano/rpc.hpp"
#include "nano/storage_memory_policy.hpp"
#include <future>
#include <iostream>
#include <unistd.h>
using namespace nano;
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
 std::cout<<Json{{"scenario","state-read-cost-summary"},{"passed",true},{"groups",1},{"checks",checks},{"expectedRefusals",refusals},{"accounts",128},{"slotsPerAccount",16},{"externalNetwork",false}}.dump()<<std::endl;return 0;
 }catch(const std::exception& error){std::cerr<<Json{{"scenario","state-read-cost-summary"},{"passed",false},{"stage",stage},{"checks",checks},{"expectedRefusals",refusals},{"lastRpcResponse",last},{"error",error.what()}}.dump()<<std::endl;return 1;}}
