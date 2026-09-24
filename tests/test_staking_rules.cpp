#include "support/process.hpp"
#include "nano/chain.hpp"
#include "nano/staking.hpp"
using namespace test;
int main(){try{
 Json data={{"config",{{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bootstrapValidator",account},{"stakingContract",bootstrap},{"evmFork","Shanghai"}}}};
 auto legacy=Genesis::from_json(data);check(legacy.staking_query_version==0&&legacy.staking_query_limits.per_call_gas==5000000&&legacy.staking_query_limits.sync_gas==50000000,"legacy defaults unchanged");
 data["config"]["stakingQueryRules"]={{"version",1},{"perCallGas",1000000},{"syncGas",10000000}};
 auto configured=Genesis::from_json(data);check(configured.block.header.hash()!=legacy.block.header.hash(),"query rules bound to genesis");
 auto modified=data;modified["config"]["stakingQueryRules"]["syncGas"]=10000001;check(Genesis::from_json(modified).block.header.hash()!=configured.block.header.hash(),"different budgets different chain identity");
 unsigned refused=0;auto reject=[&](const Json& d){bool no=false;try{Genesis::from_json(d);}catch(const std::exception&){no=true;}check(no,"invalid query rules accepted");++refused;};
 for(const auto& value:std::vector<Json>{0,-1,"1000",1.5}){auto d=data;d["config"]["stakingQueryRules"]["perCallGas"]=value;reject(d);}
 for(const auto& value:std::vector<Json>{0,-1,999999}){auto d=data;d["config"]["stakingQueryRules"]["syncGas"]=value;reject(d);}
 auto d=data;d["config"]["stakingQueryRules"]["version"]=2;reject(d);d=data;d["config"]["stakingQueryRules"]["extra"]=1;reject(d);d=data;d["config"].erase("stakingContract");reject(d);
 Temp root;{Blockchain chain(root/"db",configured);}bool mismatch=false;try{Blockchain chain(root/"db",Genesis::from_json(modified));}catch(const std::exception&){mismatch=true;}check(mismatch,"database accepted changed query rules");
 // Pure static getter: controlled gas changes success without mutating input.
 auto state=State::from_json({{bootstrap,{{"code","0x602a60005260206000f3"}}}});auto before=state.root();
 check(staking_contract_stake(state,configured.block.header,bootstrap,account,1337,{},EvmRevision::Shanghai,{100,100})==42,"configured getter budget");
 bool out=false;try{staking_contract_stake(state,configured.block.header,bootstrap,account,1337,{},EvmRevision::Shanghai,{1,1});}catch(const std::exception&){out=true;}check(out&&state.root()==before,"gas exhaustion leaves state intact");
 // One-address registry with cheap scalar getters: each call fits 100 gas,
 // while all three calls together exceed the aggregate budget.
 auto registry=State::from_json({{bootstrap,{{"code","0x5f3560e01c63b7ab4db51460165760015f5260205ff35b60205f526001602052600260405260605ff3"}}}});
 ExecutionContext ctx;ctx.revision=EvmRevision::Shanghai;ctx.address=bootstrap;ctx.code=registry.get(bootstrap).code;ctx.data=unhex("b7ab4db5");ctx.is_static=true;ctx.gas=100;auto scratch=registry;check(EVM(scratch).execute(ctx).success,"list fits per-call budget");
 ProofOfStake prior;auto before_registry=registry.root();
 auto result=sync_staking_contract(registry,configured.block.header,bootstrap,prior,1337,{},EvmRevision::Shanghai,{100,1000});
 check(result.to_json().at("validators").size()==1,"registry synchronized with sufficient total budget");
 bool exhausted=false;try{sync_staking_contract(registry,configured.block.header,bootstrap,prior,1337,{},EvmRevision::Shanghai,{100,100});}catch(const std::exception&){exhausted=true;}
 check(exhausted&&registry.root()==before_registry&&prior.to_json().at("validators").empty(),"total budget enforced without partial registry publication");
 std::cout<<Json{{"scenario","staking-query-rules"},{"invalidConfigurationsRejected",refused},{"identityBound",true},{"reopenMismatchRejected",true},{"gasBudgetEnforced",true}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
