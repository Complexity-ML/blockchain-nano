#include "nano/rpc.hpp"
#include "nano/storage_memory_policy.hpp"
#include <iostream>
#include <unistd.h>
using namespace nano;
int main(int argc,char**){if(argc!=1)return 2;auto text=(std::filesystem::temp_directory_path()/"nano-proof-baseline-XXXXXX").string();std::vector<char> name(text.begin(),text.end());name.push_back(0);auto created=::mkdtemp(name.data());if(!created)return 2;const std::filesystem::path path(created);struct Cleanup {std::filesystem::path path;~Cleanup(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}} cleanup{path};try{
 const ChainStoreMemoryPolicy policy{4*1024*1024,2,8*1024*1024,4*1024*1024};ScopedChainStoreMemoryPolicy memory(policy);
 const std::string account="0x00000000000000000000000000000000000000a1";auto genesis=Genesis::from_json({{"config",{{"chainId",1337},{"evmFork","Shanghai"}}},{"alloc",{{account,{{"balance","0x9"}}}}}});Blockchain chain(path/"chain",genesis);Rpc rpc(chain);auto reply=rpc.handle({{"jsonrpc","2.0"},{"id",1},{"method","eth_getProof"},{"params",Json::array({account,Json::array(),"earliest"})}});
 const bool passed=reply.contains("result")&&!reply.contains("error")&&reply["result"].value("balance",std::string())=="0x9";auto report=Json{{"scenario","get-proof-method-causal"},{"passed",passed},{"checks",1},{"lastRpcResponse",reply}};(passed?std::cout:std::cerr)<<report.dump()<<std::endl;return passed?0:1;
 }catch(const std::exception& error){std::cerr<<Json{{"scenario","get-proof-method-causal"},{"passed",false},{"preparationFailure",true},{"error",error.what()}}.dump()<<std::endl;return 2;}}
