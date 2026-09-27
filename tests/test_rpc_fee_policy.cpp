#include "nano/rpc.hpp"
#include "support/process.hpp"
using namespace nano;
int main(){try{
 test::Temp temporary;const auto key=bytes(Int(1),32),producer_key=bytes(Int(2),32);
 const auto sender=public_address(public_key(key)),producer=public_address(public_key(producer_key));
 const std::string receiver="0x0000000000000000000000000000000000000003";
 auto genesis=Genesis::from_json({{"config",{{"chainId",1337}}},{"alloc",{{sender,{{"balance","0x100000000000000000"}}},{producer,{{"balance","0x100000000000000000"}}}}}});
 Blockchain chain(temporary/"chain",genesis);Rpc rpc(chain);
 auto call=[&](const char* method,Json params=Json::array()){
  auto r=rpc.handle({{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",params}});
  test::check(!r.contains("error"),r.dump());return r.at("result");
 };
 auto tip=[&]{return quantity(call("eth_maxPriorityFeePerGas"));};
 auto policy=[&](Json p){rpc.load_wallets(Json{{"fee_policy",p}},"127.0.0.1");};
 auto submit=[&](const Bytes& signing_key,unsigned nonce,unsigned reward,bool typed=false){
  Transaction tx;tx.to=receiver;tx.nonce=nonce;tx.gas=21000;
  const Int base=next_base_fee(chain.tip().header);tx.gas_price=base+reward;
  if(typed){tx.type=2;tx.max_fee=base+reward;tx.priority_fee=reward+1000;}
  test::check(chain.submit(SignedTransaction::sign(tx,signing_key)),"fee fixture transaction rejected");
 };
 test::check(tip()==0,"empty history must not invent a fixed tip");
 test::check(quantity(call("eth_gasPrice"))==genesis.block.header.base_fee.value(),"empty history gas price includes base fee");
 submit(key,0,100);submit(key,1,200);submit(key,2,300,true);submit(producer_key,0,1);
 const auto first=chain.mine(producer,12);
 test::check(first.transactions.size()==4&&tip()==200,"percentile uses effective tips and excludes producer transfers");
 test::check(quantity(call("eth_gasPrice"))==first.header.base_fee.value()+200,"legacy gas quote and priority quote agree");
 policy({{"samples_per_block",1}});test::check(tip()==100,"per-block sample budget selects cheapest external transaction");
 policy({{"percentile",100}});test::check(tip()==300,"fee cap limits effective typed tip");
 policy({{"percentile",100},{"maximum_tip","0x96"}});test::check(tip()==150,"configured recommendation cap");
 for(const auto& bad:std::vector<Json>{{{"history_blocks",0}},{{"history_blocks",1025}},{{"history_blocks",-1}},{{"samples_per_block",0}},{{"samples_per_block",101}},{{"percentile",101}},{{"percentile",true}},{{"percentile",0.5}},{{"unknown",1}},{{"fallback_tip","-1"}},{{"maximum_tip",quantity(Int(1)<<256)}},{{"maximum_tip","0x1"},{"fallback_tip","0x2"}}}){
  bool rejected=false;try{policy(bad);}catch(const std::exception&){rejected=true;}
  test::check(rejected&&tip()==150,"invalid configuration must preserve previous policy and cache");
 }
 policy(Json::object());test::check(tip()==200,"policy reload invalidates cached percentile");
 test::check(chain.restore(0,genesis.block.header.hash()),"rewind fee fixture");
 submit(key,0,700,true);const auto replacement=chain.mine(producer,24);
 test::check(replacement.header.number==first.header.number&&replacement.header.hash()!=first.header.hash()&&tip()==700,"same-height replacement invalidates quote by hash");
 const auto empty=chain.mine(producer,36);
 policy({{"history_blocks",2}});test::check(tip()==700,"empty head retains observed ancestor within window");
 policy({{"history_blocks",1}});test::check(tip()==0,"outside-window fees cannot leak from cache");
 policy({{"history_blocks",1},{"fallback_tip","0x9"}});test::check(tip()==9,"explicit empty-window fallback");
 const auto wallet_file=temporary/"wallet.json";Wallet(key).save_keyfile(wallet_file,"fixture-password");setenv("NANO_FEE_POLICY_PASSWORD","fixture-password",1);
 Json wallet_config={{"fee_policy",{{"history_blocks",2}}},{"wallets",Json::array({{{"path",wallet_file.string()},{"password_env","NANO_FEE_POLICY_PASSWORD"}}})}};
 rpc.load_wallets(wallet_config,"127.0.0.1");
 auto sign=[&](Json fields){fields["from"]=sender;fields["to"]=receiver;fields["gas"]="0x5208";return SignedTransaction::decode(unhex(call("eth_signTransaction",Json::array({fields})).at("raw")));};
 test::check(sign({{"type","0x2"}}).tx.priority_fee==700,"wallet defaults use the RPC estimator");
 test::check(sign(Json::object()).tx.gas_price==empty.header.base_fee.value()+700,"legacy wallet uses the same fee policy");
 test::check(sign({{"type","0x2"},{"maxFeePerGas","0x32"}}).tx.priority_fee==50,"implicit tip respects explicit fee cap");
 test::check(sign({{"type","0x2"},{"maxPriorityFeePerGas","0x7"}}).tx.priority_fee==7,"explicit user priority fee is preserved");
 test::check(sign({{"gasPrice","0x58"}}).tx.gas_price==88,"explicit legacy price is preserved");
 const Int maximum=(Int(1)<<256)-1;
 wallet_config["fee_policy"]={{"history_blocks",1},{"fallback_tip",quantity(maximum)}};
 rpc.load_wallets(wallet_config,"127.0.0.1");
 test::check(tip()==maximum&&quantity(call("eth_gasPrice"))==maximum,"legacy recommendation cannot exceed uint256");
 const auto wide=sign({{"type","0x2"}});
 test::check(wide.tx.max_fee==maximum&&wide.tx.priority_fee==maximum,"implicit typed fee cap cannot overflow uint256");
 test::check(chain.pending_count()==0,"fee queries and signing do not submit transactions");
 unsetenv("NANO_FEE_POLICY_PASSWORD");
 std::cout<<"RPC fee policy: empty history, mixed fees, producer exclusion, limits, reload, reorg and wallet defaults passed\n";
 return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
