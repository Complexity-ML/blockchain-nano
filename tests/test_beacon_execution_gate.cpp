#include "support/process.hpp"
using namespace test;
int main(){try{
 const Json config={{"config",{{"chainId",1337},{"evmFork","Paris"},{"consensus","beacon"}}}};
 const auto genesis=Genesis::from_json(config);
 check(genesis.block.header.difficulty==0&&genesis.block.header.nonce==Bytes(8),"post-merge genesis seal");
 auto rejected=[](auto action){try{action();return false;}catch(const std::exception&){return true;}};
 for(const char* field:{"stakingContract","posSignatureVersion","bootstrapValidator","bftVersion","consensusRules","stakingQueryRules"}){
  auto bad=config;bad["config"][field]=nullptr;check(rejected([&]{Genesis::from_json(bad);}),"reject legacy beacon fields");
 }
 for(const char* fork:{"Shanghai","NanoExperimental"}){auto bad=config;bad["config"]["evmFork"]=fork;check(rejected([&]{Genesis::from_json(bad);}),"explicit supported execution fork");}
 auto legacy=config;legacy["config"]["consensus"]="pow";legacy["difficulty"]="0x1";
 check(Genesis::from_json(legacy).block.header.hash()!=genesis.block.header.hash(),"consensus genesis domain separated");
 Temp root;
 {Blockchain chain(root/"legacy",Genesis::from_json(legacy));Rpc rpc(chain);const auto result=rpc.handle({{"jsonrpc","2.0"},{"id",1},{"method","eth_getBlockByNumber"},{"params",Json::array({"finalized",false})}});check(result.contains("error"),"legacy chain silently claims Beacon finality");}

 for(unsigned reopen=0;reopen<2;++reopen){
  Blockchain chain(root/"chain",genesis);
  check(rejected([&]{chain.require_execution_ready();}),"unverified checkpoint blocks readiness");
  check(rejected([&]{Rpc rpc(chain);}),"unverified checkpoint blocks RPC construction");
  check(rejected([&]{chain.finalized_block();}),"unverified checkpoint blocks finalized reads");
  check(rejected([&]{chain.tip();})&&rejected([&]{chain.state();}),"unverified checkpoint blocks canonical state reads");
  check(rejected([&]{chain.restore(0,genesis.block.header.hash());}),"legacy rewind cannot bypass fork choice");
  check(rejected([&]{chain.import_block(genesis.block,0);}),"legacy import cannot bypass signed consensus");
  check(rejected([&]{chain.validate("0x0000000000000000000000000000000000000001",12);}),"legacy production cannot bypass duty signing");
  check(rejected([&]{chain.block_template("0x0000000000000000000000000000000000000001",12);}),"legacy template cannot bypass Beacon context");
  check(rejected([&]{chain.audit();}),"legacy audit cannot claim beacon verification");
 }
 std::cout<<"Beacon startup verification gate and legacy mutation separation checks passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
