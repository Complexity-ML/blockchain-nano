#include "support/process.hpp"
#include "nano/chain.hpp"
#include "nano/staking.hpp"
using namespace test;
int main(){try{
 Temp root;auto artifact=load(std::string(NANO_TEST_SOURCE)+"/contracts/NanoStaking.json");
 std::vector<Bytes> keys;std::vector<std::string> accounts;Json alloc=Json::object();auto minimum=ProofOfStake::minimum_stake;
 for(unsigned i=1;i<=17;++i){keys.push_back(bytes(Int(i),32));accounts.push_back(public_address(public_key(keys.back())));alloc[accounts.back()]={{"balance",quantity(Int(minimum*10))}};}
 auto contract=create_address(accounts[0],0);Json config={{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bootstrapValidator",accounts[0]},{"stakingContract",contract},{"evmFork","Shanghai"}};
 Blockchain chain(root/"setup",Genesis::from_json({{"config",config},{"alloc",alloc}}));
 auto mine=[&](){auto proposer=chain.next_validator();auto index=std::find_if(accounts.begin(),accounts.end(),[&](const auto& a){return address(a)==address(*proposer);})-accounts.begin();check(index<keys.size(),"known proposer");return chain.validate_signed(keys[index],chain.tip().header.timestamp+12);};
 for(unsigned i=0;i<17;++i){Transaction tx;tx.chain_id=1337;tx.gas=3000000;tx.gas_price=2000000000;tx.value=minimum;
  if(i==0)tx.data=unhex(artifact["bytecode"]);else{tx.to=contract;tx.data=unhex(artifact["methodIdentifiers"]["registerValidator()"]);}
  auto signed_tx=SignedTransaction::sign(tx,keys[i]);check(chain.submit(signed_tx),"registration submitted");mine();check(chain.store().receipt(signed_tx.hash())->success,"registration executed");}
 auto state=chain.state();auto header=chain.tip().header;auto before=state.root();ProofOfStake previous;
 auto full=sync_staking_contract(state,header,contract,previous,1337,{},EvmRevision::Shanghai);
 for(uint64_t size:{1u,2u,8u,17u,32u}){auto paged=sync_staking_contract(state,header,contract,previous,1337,{},EvmRevision::Shanghai,{5000000,50000000,size});check(paged.to_json()==full.to_json(),"page sizes produce exact historical registry");}
 check(state.root()==before,"pagination does not mutate state");
 config["stakingQueryRules"]={{"version",2},{"perCallGas",5000000},{"syncGas",50000000},{"pageSize",8}};
 unsigned invalid_config=0;
 for(const Json& size:std::vector<Json>{0,-1,"64",1.5}){auto invalid=config;invalid["stakingQueryRules"]["pageSize"]=size;bool no=false;try{Genesis::from_json({{"config",invalid},{"alloc",state.to_json()}});}catch(const std::exception&){no=true;}check(no,"invalid page size accepted");++invalid_config;}
 auto genesis=Genesis::from_json({{"config",config},{"alloc",state.to_json()}});
 auto changed=config;changed["stakingQueryRules"]["pageSize"]=9;check(Genesis::from_json({{"config",changed},{"alloc",state.to_json()}}).block.header.hash()!=genesis.block.header.hash(),"page size bound to chain identity");
 {Blockchain paged(root/"paged",genesis);check(paged.validators().size()==17,"engine consumes paginated genesis registry");}
 {Blockchain reopened(root/"paged",genesis);check(reopened.validators().size()==17,"paginated registry survives reopen");}
 // Malformed scalar result must fail before any registry is published.
 auto bad=state;bad.get(contract).code=unhex("0x600160005360016000f3");bool refused=false;
 try{sync_staking_contract(bad,header,contract,previous,1337,{},EvmRevision::Shanghai,{5000000,50000000,8});}catch(const std::exception&){refused=true;}check(refused,"malformed count rejected");
 auto mock=[&](const Bytes& page,unsigned total){
  auto code=unhex("60003560e01c637071688a14601e57");
  code=concat(code,concat(unhex("61"),bytes(Int(page.size()),2)));
  code=concat(code,unhex("610029600039"));code=concat(code,concat(unhex("61"),bytes(Int(page.size()),2)));code=concat(code,unhex("6000f35b60"));code.push_back(static_cast<uint8_t>(total));code=concat(code,unhex("60005260206000f3"));check(code.size()==41,"mock data offset");return concat(code,page);
 };
 Bytes page;for(const Int& word:std::vector<Int>{32,1,2,minimum,1})page=concat(page,bytes(word,32));
 auto valid_mock=state;valid_mock.get(contract).code=mock(page,1);check(sync_staking_contract(valid_mock,header,contract,previous,1337,{},EvmRevision::Shanghai,{5000000,50000000,1}).to_json().at("validators").size()==1,"valid mock page control");
 unsigned invalid_pages=0;
 auto reject_page=[&](const Bytes& output,unsigned total){auto altered=state;altered.get(contract).code=mock(output,total);bool no=false;try{sync_staking_contract(altered,header,contract,previous,1337,{},EvmRevision::Shanghai,{5000000,50000000,1});}catch(const std::exception&){no=true;}check(no,"malformed or repeated page accepted");++invalid_pages;};
 auto malformed=page;malformed[31]=0;reject_page(malformed,1);
 malformed=page;malformed[63]=2;reject_page(malformed,1);
 malformed=page;malformed.pop_back();reject_page(malformed,1);
 malformed=page;malformed[159]=2;reject_page(malformed,1);
 malformed=page;malformed[64]=1;reject_page(malformed,1);
 reject_page(page,2); // Same address repeated across two otherwise valid pages.
 std::cout<<Json{{"scenario","staking-pagination"},{"registeredBySignedTransactions",17},{"pageSizes",{1,2,8,17,32}},{"identicalRegistry",true},{"engineGenesisAndReopen",true},{"malformedCountRejected",true},{"invalidPagesRejected",invalid_pages},{"invalidPageConfigurationsRejected",invalid_config}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
