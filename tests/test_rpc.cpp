#include "nano/rpc.hpp"
#include "nano/block_auth.hpp"
#include "nano/simulation_queue.hpp"
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <future>
#include <atomic>
#include <thread>
using namespace nano;
static int checks=0;
void check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
int main(){try{
 for(const auto* method:{"eth_signTransaction","eth_sendTransaction","nno_registerValidator","nno_addStake","nno_withdrawStake"}){
  Json item={{"method",method}};check(simulation_count(item)==1,"wallet estimation reserves simulation capacity");check(simulation_count(Json::array({item,item}))==2,"wallet batch counts every simulation");
 }
 auto budget_now=AdmissionBudget::Clock::time_point{};AdmissionBudget budget(budget_now);
 check(budget.consume(128,budget_now)&&!budget.consume(1,budget_now),"admission burst counts transactions");
 check(!budget.consume(1,budget_now-std::chrono::seconds(1)),"clock rollback cannot refill admission budget");
 check(!budget.consume(65,budget_now+std::chrono::seconds(1))&&budget.consume(64,budget_now+std::chrono::seconds(1)),"admission refill is 64 per second and failed requests do not charge");
 check(budget.consume(128,budget_now+std::chrono::hours(1))&&!budget.consume(1,budget_now+std::chrono::hours(1)),"idle credit is bounded at 128");
 auto key=bytes(Int(1),32);auto sender=public_address(public_key(key));std::string contract="0x00000000000000000000000000000000000000ab";
 Json alloc={{sender,{{"balance","0x100000000000000000"}}},{contract,{{"code","0x602a60005560005460005260206000f3"},{"balance","0x0"}}}};
 std::string hash_contract="0x00000000000000000000000000000000000000ac";alloc[hash_contract]={{"code","0x6000354060005260206000f3"}};
 std::string price_contract="0x00000000000000000000000000000000000000ad";
 std::string price_guard="0x00000000000000000000000000000000000000ae";
 alloc[price_contract]={{"code","0x3a60005260206000f3"}};
 alloc[price_guard]={{"code","0x6000353a14600d5760006000fd5b00"}};
 std::string context_contract="0x00000000000000000000000000000000000000af";
 alloc[context_contract]={{"code","0x436000554360005260206000f3"}};
 auto genesis=Genesis::from_json({{"config",{{"chainId",1337}}},{"alloc",alloc}});
 auto path=std::filesystem::temp_directory_path()/("nano-rpc-"+std::to_string(getpid()));
 {auto gossip_path=path.string()+"-gossip";{Blockchain gossip_chain(gossip_path,genesis);Rpc gossip_rpc(gossip_chain);
  auto receive=[&](Json batch){return gossip_rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method","nano_receiveTransactions"},{"params",Json::array({batch})}});};
  Transaction transaction;transaction.to=contract;transaction.gas=100000;transaction.gas_price=2000000000;auto signed_gossip=SignedTransaction::sign(transaction,key);auto raw=hex(signed_gossip.raw());
  check(receive(Json::array({raw,7})).contains("error")&&gossip_chain.pending_count()==0,"batch shape checked before partial admission");
  Json oversized=Json::array();for(unsigned i=0;i<51;++i)oversized.push_back(raw);check(receive(oversized).contains("error")&&gossip_chain.pending_count()==0,"gossip count limit before admission");
  check(receive(Json::array({std::string(2*1024*1024+3,'0')})).contains("error"),"gossip per transaction byte bound");
  Json bytes_batch=Json::array();for(unsigned i=0;i<5;++i)bytes_batch.push_back(std::string(2*1024*1024,'0'));check(receive(bytes_batch).contains("error"),"gossip aggregate byte bound");
  auto first=receive(Json::array({raw}));check(first.at("result").at("accepted")==Json::array({true})&&gossip_chain.pending_count()==1,"gossip batch admits signed transaction");
  auto cursor=gossip_chain.event_cursor();auto duplicate=receive(Json::array({raw,raw,"0x00"}));check(duplicate.at("result").at("processed")==3&&duplicate.at("result").at("accepted")==Json::array({true,true,false}),"lost acknowledgement replay accepts known transactions and identifies malformed item");
  check(gossip_chain.pending_count()==1&&gossip_chain.event_cursor()==cursor,"duplicates do not amplify gossip events");
  gossip_chain.mine(sender,24);auto root=gossip_chain.state().root();check(receive(Json::array({raw})).at("result").at("accepted")==Json::array({true})&&gossip_chain.state().root()==root&&gossip_chain.pending_count()==0,"mined transaction replay is idempotent");
 }std::filesystem::remove_all(gossip_path);}
 std::optional<Block> reopened_tip;Bytes reopened_root;
 {Blockchain chain(path,genesis);Rpc rpc(chain);
 auto request=[](const std::string& method,Json::initializer_list_t params={}){return Json{{"jsonrpc","2.0"},{"id",7},{"method",method},{"params",Json::array(params)}};};
 auto call=[&](const std::string& method,Json::initializer_list_t params={}){auto r=rpc.handle(request(method,params));if(r.contains("error"))throw std::runtime_error(method+": "+r.dump());return r["result"];};
 check(call("txpool_status")==Json{{"pending","0x0"},{"queued","0x0"}},"empty txpool status uses RPC quantities");
 Json context_call={{"from",sender},{"to",context_contract}};
 check(request("eth_call",{context_call})["params"]==Json::array({context_call}),"request builder preserves a single object as one positional parameter");
 check(request("eth_chainId")["params"]==Json::array(),"request builder emits an empty positional array");
 auto context_root=chain.state().root();
 auto default_quote=call("eth_estimateGas",{context_call});
 auto pending_quote=call("eth_estimateGas",{context_call,"pending"});
 auto latest_quote=call("eth_estimateGas",{context_call,"latest"});
 check(default_quote==pending_quote&&quantity(default_quote)>quantity(latest_quote),"unspecified estimate uses next-block storage cost");
 check(call("eth_call",{context_call})==hex(bytes(Int(0),32)),"unspecified call keeps latest block context");
 check(call("eth_call",{context_call,"pending"})==hex(bytes(Int(1),32)),"explicit pending call uses next block context");
 check(call("eth_estimateGas",{context_call,"0x0"})==latest_quote,"explicit historical estimate retains historical context");
 check(chain.state().root()==context_root&&chain.pending_count()==0,"default pending estimation leaves canonical state and pool untouched");

 Transaction blob;blob.type=3;blob.to=contract;blob.gas=100000;blob.max_fee=2000000000;blob.priority_fee=1;blob.max_fee_per_blob_gas=1;blob.blob_versioned_hashes={Bytes(32,0)};blob.blob_versioned_hashes[0][0]=1;
 auto blob_raw=SignedTransaction::sign(blob,key).raw();auto state_before_blob=chain.state().to_json();
 check(rpc.handle(request("eth_sendRawTransaction",{hex(blob_raw)})).contains("error"),"network rejects blob transaction before Cancun activation");
 for(const auto& method:{"eth_call","eth_estimateGas"})check(rpc.handle(request(method,{blob.to_json(),"latest"})).contains("error"),"network simulation rejects unsupported blob transaction");
 check(chain.state().to_json()==state_before_blob&&call("eth_getTransactionCount",{sender,"pending"})=="0x0","blob rejection preserves state and pending nonce");
 Transaction set_code=blob;set_code.type=4;set_code.blob_versioned_hashes.clear();Authorization auth;auth.target=contract;auth.chain_id=1337;auth.nonce=1;auto auth_sig=sign_hash(key,auth.signing_hash());auth.y_parity=auth_sig.v;auth.r=auth_sig.r;auth.s=auth_sig.s;set_code.authorization_list={auth};
 auto set_code_raw=SignedTransaction::sign(set_code,key).raw();
 auto rejected_set_code=rpc.handle(request("eth_sendRawTransaction",{hex(set_code_raw)}));
 check(rejected_set_code.contains("error")&&rejected_set_code.at("error").at("message")=="transaction type unavailable on Nano network","network rejects valid type-4 envelope before Prague activation");
 for(const auto& method:{"eth_call","eth_estimateGas"})check(rpc.handle(request(method,{set_code.to_json(),"latest"})).contains("error"),"network simulation rejects unsupported type-4 transaction");
 check(chain.state().to_json()==state_before_blob&&call("eth_getTransactionCount",{sender,"pending"})=="0x0","type-4 rejection preserves state and pending nonce");
 Rpc reported(chain,"test-only-link-token");
 reported.load_wallets(Json{{"coinbase",contract}},"127.0.0.1");
 check(reported.handle(request("eth_coinbase"))["result"]==contract,"coinbase RPC returns configured node reward address");

 auto report=Json{{"peerId","local-peer"},{"addresses",Json::array({"/ip4/127.0.0.1/tcp/30303"})},{"peers",Json::array({Json{{"peerId","remote-peer"},{"bestBlock",2}}})}};
 check(reported.handle(request("nano_networkReport",{"wrong",report}))["error"]["code"]==-32001,"network reports require link token");
 check(reported.handle(request("nano_networkReport",{"test-only-link-token",report}))["result"]==true,"authenticated network report accepted");
 auto remote_status=reported.handle(request("validator_getSyncStatus"))["result"];
 check(remote_status["syncing"]==true&&remote_status["highestBlock"]=="0x2"&&remote_status["ready"]==false,"peer height gates validator readiness");
 check(reported.handle(request("net_peerCount"))["result"]=="0x1"&&reported.handle(request("net_listening"))["result"]==true,"peer count and listener come from transport report");
 check(call("nno_getStake",{sender})=="0x0","unknown stake is zero");
 check(rpc.handle(request("nno_getStake"))["error"]["code"]==-32602,"stake requires address");
 check(rpc.handle(request("nno_getStake",{"0x12"}))["error"]["code"]==-32602,"stake rejects malformed address");
 check(call("eth_chainId")=="0x539"&&call("net_version")=="1337","network identity");
 check(call("eth_getBalance",{sender,"earliest"})=="0x100000000000000000","genesis account lookup");
 check(call("eth_getBlockByNumber",{"0x99",false}).is_null(),"missing block returns null");
 check(rpc.handle(request("eth_getBalance",{sender,"0x99"}))["error"]["code"]==-32000,"missing historical state returns error");
 check(rpc.parse("{")["error"]["code"]==-32700,"JSON parse error");
 check(rpc.handle(Json::array())["error"]["code"]==-32600,"empty batch rejected");
 check(rpc.handle(request("unknown"))["error"]["code"]==-32601,"unknown method error");
 check(rpc.handle(request("eth_getBalance",{12}))["error"]["code"]==-32602,"parameter types validated");
 check(rpc.handle(Json{{"jsonrpc","2.0"},{"method","eth_chainId"}}).is_null(),"notification has no response");
 auto batch=rpc.handle(Json::array({request("eth_chainId"),Json{{"jsonrpc","2.0"},{"method","eth_blockNumber"}},42}));check(batch.size()==2&&batch[0]["result"]=="0x539"&&batch[1]["error"]["code"]==-32600,"batch omits notification and reports invalid member");
 for(const auto& id:Json::array({nullptr,"client-id",-1,0,18446744073709551615ULL})){
  auto req=request("eth_chainId");req["id"]=id;auto reply=rpc.handle(req);
  check(reply["id"]==id&&reply["result"]=="0x539","RPC preserves accepted request ID");
 }
 for(const auto& id:Json::array({true,1.5,Json::object(),Json::array()})){
  auto req=request("eth_chainId");req["id"]=id;auto reply=rpc.handle(req);
  check(reply["id"].is_null()&&reply["error"]["code"]==-32600,"RPC rejects structured boolean and fractional IDs");
 }
 for(const auto& params:Json::array({nullptr,true,1,"bad",Json::object()})){
  auto req=request("eth_chainId");req["params"]=params;
  check(rpc.handle(req)["error"]["code"]==-32602,"RPC accepts only positional parameter arrays");
 }
 check(rpc.handle(Json::array({Json{{"jsonrpc","2.0"},{"method","unknown"}},Json{{"jsonrpc","2.0"},{"method","eth_getBalance"},{"params",Json::array({12})}}})).is_null(),"failed notifications produce no batch response");
 Json invocation={{"from",sender},{"to",contract}};
 check(call("eth_call",{invocation})==hex(bytes(Int(42),32)),"eth_call returns execution data");
 check(chain.state().get_storage(contract,0)==0,"eth_call does not change canonical state");
 auto estimate=quantity(call("eth_estimateGas",{invocation}));invocation["gas"]=quantity(estimate);check(call("eth_call",{invocation})==hex(bytes(Int(42),32)),"estimated gas actually executes");
 invocation["gas"]=quantity(Int(estimate-1));check(rpc.handle(request("eth_call",{invocation})).contains("error"),"estimate lower boundary fails");
 // GASPRICE must match execution for dynamic fees, including maxFee clipping.
 for(const auto& fees:std::vector<std::pair<Int,Int>>{{Int(3000000000),Int(200000000)},{Int(1100000000),Int(200000000)}}){
  Int price=std::min(fees.first,Int(genesis.block.header.base_fee.value_or(Int(0))+fees.second));
  Json dynamic={{"from",sender},{"to",price_contract},{"maxFeePerGas",quantity(fees.first)},{"maxPriorityFeePerGas",quantity(fees.second)}};
  check(call("eth_call",{dynamic})==hex(bytes(price,32)),"type 2 call uses effective GASPRICE");
  dynamic["to"]=price_guard;dynamic["data"]=hex(bytes(price,32));
  // This oracle executes at the genesis header, so request that same context explicitly.
  auto gas=quantity(call("eth_estimateGas",{dynamic,"latest"}));dynamic["gas"]=quantity(gas);
  auto simulated=call("eth_call",{dynamic});check(simulated=="0x","dynamic fee estimate executes price-dependent contract");
  auto copy=genesis.state;ExecutionEngine engine(genesis.chain_id);
  auto receipt=engine.apply_transaction(copy,SignedTransaction::sign(Transaction::from_json(dynamic),key),genesis.block.header,0,0);
  check(receipt.success&&receipt.gas_used==gas,"estimated dynamic fee gas agrees with signed transaction execution");
  check(chain.state().root()==genesis.state.root(),"fee simulation leaves canonical state unchanged");
 }
 check(call("eth_call",{Json{{"to",price_contract},{"gasPrice","0x7"}}})==hex(bytes(Int(7),32)),"legacy call preserves explicit GASPRICE");
 Transaction tx;tx.to=contract;tx.gas=100000;tx.gas_price=2000000000;auto signed_tx=SignedTransaction::sign(tx,key);
 check(call("eth_sendRawTransaction",{hex(signed_tx.raw())})==hex(signed_tx.hash()),"submit raw transaction");
 check(call("txpool_status")==Json{{"pending","0x1"},{"queued","0x0"}},"nonempty txpool status uses RPC quantities");
 check(call("eth_getTransactionCount",{sender,"pending"})=="0x1"&&call("eth_getTransactionCount",{sender,"latest"})=="0x0","pending and mined nonce differ");
 check(call("eth_getTransactionByHash",{hex(signed_tx.hash())})["blockHash"].is_null(),"pending transaction metadata");
 Json queued_call={{"from",sender},{"to",contract}};
 auto pool_root=chain.state().root();
 auto queued_default=call("eth_estimateGas",{queued_call});
 check(queued_default==call("eth_estimateGas",{queued_call,"pending"})&&quantity(queued_default)<quantity(call("eth_estimateGas",{queued_call,"latest"})),"default estimate incorporates pending transaction storage writes");
 check(chain.pending_count()==1&&chain.state().root()==pool_root&&call("eth_getTransactionCount",{sender,"latest"})=="0x0","default pending simulation preserves canonical nonce and queued transaction");
 auto mined=chain.mine(sender,24);
 check(reported.handle(request("eth_coinbase"))["result"]==contract,"coinbase remains local after another producer mines");
 bool bad_coinbase=false;try{reported.load_wallets(Json{{"coinbase","0x12"}},"127.0.0.1");}catch(const std::invalid_argument&){bad_coinbase=true;}
 check(bad_coinbase&&reported.handle(request("eth_coinbase"))["result"]==contract,"invalid configuration preserves previous coinbase");

 check(call("eth_getTransactionReceipt",{hex(signed_tx.hash())})["blockHash"]==hex(mined.header.hash()),"receipt lookup");
 check(call("eth_getStorageAt",{contract,"0x0","latest"})==hex(bytes(Int(42),32))&&call("eth_getStorageAt",{contract,"0x0","earliest"})==hex(Bytes(32)),"historical storage views");
 check(call("eth_getBlockByNumber",{"latest",true})["transactions"][0]["transactionIndex"]=="0x0","full block includes transaction metadata");
 check(call("eth_getTransactionByBlockHashAndIndex",{hex(mined.header.hash()),"0x0"})["hash"]==hex(signed_tx.hash()),"transaction index lookup");
 check(call("eth_getBlockTransactionCountByNumber",{"latest"})=="0x1","block transaction count");

 Json historical_price={{"to",price_contract},{"maxFeePerGas","0xb2d05e00"},{"maxPriorityFeePerGas","0xbebc200"}};
 check(call("eth_call",{historical_price,"earliest"})==hex(bytes(Int(genesis.block.header.base_fee.value_or(Int(0))+200000000),32)),"historical simulation uses genesis base fee");
 check(call("eth_call",{historical_price,"latest"})==hex(bytes(Int(mined.header.base_fee.value_or(Int(0))+200000000),32)),"latest simulation uses current base fee");
 Json hash_call={{"to",hash_contract},{"data",hex(bytes(Int(0),32))}};
 check(call("eth_call",{hash_call,"latest"})==hex(genesis.block.header.hash()),"RPC BLOCKHASH resolves historical ancestor");
 check(call("eth_call",{hash_call,"earliest"})==hex(Bytes(32)),"RPC historical context excludes its own block");
 auto hash_gas=call("eth_estimateGas",{hash_call});hash_call["gas"]=hash_gas;
 check(call("eth_call",{hash_call})==hex(genesis.block.header.hash()),"gas estimate executes BLOCKHASH with database context");
 auto history=call("eth_feeHistory",{"0x10","latest",Json::array({0,50,100})});
 check(history["oldestBlock"]=="0x0"&&history["baseFeePerGas"].size()==3&&history["gasUsedRatio"].size()==2,"fee history truncates range at genesis");
 check(history["reward"][0]==Json::array({"0x0","0x0","0x0"})&&history["gasUsedRatio"][0]==0.0,"empty block has zero rewards and utilization");
 Int tip=tx.gas_price-mined.header.base_fee.value();
 check(history["reward"][1]==Json::array({quantity(tip),quantity(tip),quantity(tip)})&&history["baseFeePerGas"][2]==quantity(next_base_fee(mined.header)),"fee history uses effective tip and projects next base fee");
 check(history["gasUsedRatio"][1].get<double>()==mined.header.gas_used.get_d()/mined.header.gas_limit.get_d(),"fee history reports actual gas utilization");
 check(!call("eth_feeHistory",{"0x1","earliest"}).contains("reward"),"omitted percentiles omit reward field");
 for(const auto& invalid:Json::array({Json::array({"0x0","latest"}),Json::array({"0x401","latest"}),Json::array({"0x1","latest",Json::array({50,10})}),Json::array({"0x1","latest",Json::array({101})}),Json::array({"0x1","latest",Json::array({"50"})})})){
  auto invalid_request=request("eth_feeHistory");invalid_request["params"]=invalid;
  check(rpc.handle(invalid_request)["error"]["code"]==-32602,"invalid fee history parameters rejected");
 }
 check(rpc.handle(request("eth_feeHistory",{"0x1","0x99"}))["error"]["code"]==-32000,"unknown fee history block rejected");
 Transaction cheap=tx;cheap.nonce=1;cheap.gas_price=2000000000;
 Transaction costly=tx;costly.nonce=2;costly.to=sender;costly.gas_price=3000000000;costly.type=2;costly.max_fee=3000000000;costly.priority_fee=2000000000;
 auto cheap_signed=SignedTransaction::sign(cheap,key),costly_signed=SignedTransaction::sign(costly,key);
 check(chain.submit(cheap_signed)&&chain.submit(costly_signed),"fee history mixed price transactions submitted");
 auto mixed=chain.mine(sender,36);
 check(mixed.transactions.size()==2&&chain.store().receipt(cheap_signed.hash())->gas_used>chain.store().receipt(costly_signed.hash())->gas_used,"weighted percentile fixture has unequal gas consumption");
 auto weighted=call("eth_feeHistory",{"0x1","latest",Json::array({0,50,100})});
 Int cheap_tip=cheap.gas_price-mixed.header.base_fee.value(),costly_tip=costly.effective_gas_price(mixed.header.base_fee.value())-mixed.header.base_fee.value();
 check(weighted["reward"][0]==Json::array({quantity(cheap_tip),quantity(cheap_tip),quantity(costly_tip)}),"reward percentiles are weighted by gas rather than transaction count");
 auto fractional=call("eth_feeHistory",{"0x1","latest",Json::array({0,0,12.5,99.9,100})});
 check(fractional["reward"][0]==Json::array({quantity(cheap_tip),quantity(cheap_tip),quantity(cheap_tip),quantity(costly_tip),quantity(costly_tip)}),"fractional and repeated percentiles preserve weighted ordering");
 check(!call("eth_feeHistory",{"0x1","latest",Json::array()}).contains("reward"),"empty percentiles omit reward field");
 check(call("eth_feeHistory",{"0x400","earliest",Json::array({100})})["gasUsedRatio"].size()==1,"maximum count is truncated at earliest block");
 for(const auto& percentiles:Json::array({Json::array({-0.1}),Json::array({true}),Json::array({nullptr}),Json::array({0.2,0.1}),Json(std::vector<int>(101,50))}))
  check(rpc.handle(request("eth_feeHistory",{"0x1","latest",percentiles}))["error"]["code"]==-32602,"malformed fractional or oversized percentile list rejected");
 check(call("eth_feeHistory",{"0x1","0x1",Json::array({50})})["reward"][0][0]==quantity(tip),"historical fee query stays pinned to requested block");
 check(call("eth_accounts").empty(),"no accounts unlocked by default");
 check(rpc.handle(request("eth_sign",{sender,"0x1234"}))["error"]["code"]==-32000,"locked account cannot sign");
 auto keyfile=path.string()+"-wallet.json";Wallet(key).save_keyfile(keyfile,"fixture-password");setenv("NANO_RPC_TEST_PASSWORD","fixture-password",1);
 Json config={{"coinbase",sender},{"wallets",Json::array({Json{{"path",keyfile},{"password_env","NANO_RPC_TEST_PASSWORD"},{"name","fixture"}}})}};
 bool denied=false;try{rpc.load_wallets(config,"0.0.0.0");}catch(const std::invalid_argument&){denied=true;}check(denied&&!rpc.has_wallets(),"public bind rejects unlocked accounts");
 rpc.load_wallets(config,"127.0.0.1");check(call("eth_accounts")==Json::array({sender}),"configured wallet appears in accounts");
 check(rpc.handle(request("nno_startValidating"))["error"]["code"]==-32000,"automatic PoS validation refuses PoW chain");
 check(rpc.handle(request("nno_stopValidating",{true}))["error"]["code"]==-32602,"validator control rejects unexpected params");
 auto signature=call("eth_sign",{sender,"0x1234"});check(recover_message(unhex("0x1234"),unhex(signature))==sender,"eth_sign signature recovers configured account");
 check(call("personal_sign",{"0x1234",sender})==signature,"personal_sign parameter order");
 Json unsigned_tx={{"from",sender},{"to",sender},{"value","0x1"}};
 auto signed_result=call("eth_signTransaction",{unsigned_tx});auto decoded=SignedTransaction::decode(unhex(signed_result["raw"]));
 check(decoded.sender()==sender&&decoded.tx.nonce==3&&decoded.tx.chain_id==1337&&decoded.tx.gas==21000&&chain.pending_count()==0,"signTransaction fills gas chain and nonce without submitting");
 auto first_sent=call("eth_sendTransaction",{unsigned_tx});unsigned_tx["type"]="0x2";auto second_sent=call("eth_sendTransaction",{unsigned_tx});
 check(call("eth_getTransactionByHash",{first_sent})["nonce"]=="0x3"&&call("eth_getTransactionByHash",{second_sent})["nonce"]=="0x4","sendTransaction uses contiguous pending nonces");
 auto local_block=chain.mine(sender,48);check(local_block.transactions.size()==2&&chain.state().get(sender).nonce==5,"RPC signed transactions execute in block");
 unsigned_tx["chainId"]="0x1";check(rpc.handle(request("eth_sendTransaction",{unsigned_tx}))["error"]["code"]==-32602,"wallet RPC rejects wrong chain");
 unsigned_tx.erase("chainId");unsigned_tx["type"]="0x100000000";check(rpc.handle(request("eth_signTransaction",{unsigned_tx}))["error"]["code"]==-32602,"wallet RPC rejects transaction type overflow");
 Json typed={{"types",{{"EIP712Domain",Json::array({Json{{"name","chainId"},{"type","uint256"}}})},{"Test",Json::array({Json{{"name","value"},{"type","uint256"}}})}}},{"primaryType","Test"},{"domain",{{"chainId",1337}}},{"message",{{"value",7}}}};
 check(call("eth_signTypedData_v4",{sender,typed.dump()})==hex(Wallet(key).sign_typed_data(typed)),"RPC signs structured data JSON strings");
 typed["domain"]["chainId"]=1;check(rpc.handle(request("eth_signTypedData",{sender,typed}))["error"]["code"]==-32602,"typed signatures reject foreign chain domain");
 setenv("NANO_RPC_TEST_PASSWORD","wrong",1);bool failed=false;try{rpc.load_wallets(config,"127.0.0.1");}catch(const std::exception&){failed=true;}check(failed&&call("eth_accounts")==Json::array({sender}),"failed wallet reload preserves loaded accounts");
 unsetenv("NANO_RPC_TEST_PASSWORD");std::filesystem::remove(keyfile);
 check(rpc.handle(request("evm_snapshot"))["error"]["code"]==-32601,"snapshots disabled by default");
 rpc.load_wallets(Json{{"dev_mode",true}},"127.0.0.1");auto snapshot=call("evm_snapshot");auto before=chain.tip();auto state_before=chain.state().root();auto difficulty_before=chain.store().total_difficulty();
 auto batch_snapshot=call("evm_snapshot");
 auto mutation_batch=rpc.handle(Json::array({
  Json{{"jsonrpc","2.0"},{"method","evm_setNextBlockTimestamp"},{"params",Json::array({1000})}},
  request("unknown"),request("evm_mine"),request("eth_blockNumber")}));
 check(mutation_batch.size()==3&&mutation_batch[0]["error"]["code"]==-32601,"batch failure does not discard subsequent requests");
 check(chain.tip().header.timestamp==1000&&mutation_batch[1]["result"]==hex(chain.tip().header.hash()),"notification executes before following batch mutation");
 check(mutation_batch[2]["result"]==quantity(chain.tip().header.number),"batch reads observe preceding included block");
 check(call("evm_revert",{batch_snapshot})==true&&chain.tip().header.hash()==before.header.hash(),"batch side effects can be reverted in development mode");
 auto filter=call("eth_newBlockFilter");auto subscription=rpc.stream("newHeads");
 Transaction after;after.nonce=5;after.to=contract;after.gas=100000;after.gas_price=2000000000;auto removed_tx=SignedTransaction::sign(after,key);check(chain.submit(removed_tx),"snapshot fixture transaction submitted");
 auto removed_block=chain.mine(sender,60);auto newer_snapshot=call("evm_snapshot");
 after.nonce=7;check(chain.submit(SignedTransaction::sign(after,key)),"pending transaction before revert");
 check(call("evm_revert",{snapshot})==true&&chain.tip().header.hash()==before.header.hash()&&chain.state().root()==state_before,"snapshot restores block and state commitments");
 check(chain.store().total_difficulty()==difficulty_before&&chain.store().count()==size_t(u64(before.header.number)+1)&&chain.pending_count()==0,"snapshot restores metadata and clears pending pool");
 check(!chain.store().block_by_hash(removed_block.header.hash())&&!chain.store().block_by_number(removed_block.header.number)&&!chain.store().transaction(removed_tx.hash())&&!chain.store().receipt(removed_tx.hash())&&!chain.store().state_at(removed_block.header.number),"revert removes every abandoned block index and receipt");
 check(call("evm_revert",{snapshot})==false&&call("evm_revert",{newer_snapshot})==false,"revert consumes snapshot and invalidates newer snapshots");
 check(rpc.handle(request("eth_getFilterChanges",{filter}))["error"]["code"]==-32000,"revert invalidates polling filters");
 bool reset=false;try{rpc.poll(subscription);}catch(const RpcError&){reset=true;}check(reset,"revert invalidates websocket stream cursors");
 check(!chain.restore(before.header.number,Bytes(32)),"restore rejects stale block identity");
 for(const auto& method:{"evm_mine","evm_increaseTime","evm_setNextBlockTimestamp"})check(reported.handle(request(method))["error"]["code"]==-32601,"time and production RPC disabled outside development mode");
 check(call("evm_setNextBlockTimestamp",{1000})=="0x3e8","set next development timestamp");auto timed_snapshot=call("evm_snapshot");
 auto first_hash=call("evm_mine");check(chain.tip().header.timestamp==1000&&hex(chain.tip().header.hash())==first_hash.get<std::string>(),"manual PoW block uses exact requested timestamp");
 check(rpc.handle(request("evm_setNextBlockTimestamp",{999}))["error"]["code"]==-32602,"backward timestamp rejected");
 check(call("evm_increaseTime",{3600})=="0xe10","increase time returns requested offset");
 auto wall=Int(std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()));call("evm_mine");check(chain.tip().header.timestamp>=wall+3600,"time offset affects next development block");
 check(call("evm_revert",{timed_snapshot})==true,"timed snapshot restored");
 check(call("evm_mine")==first_hash,"snapshot restores scheduled timestamp for identical replay");
 call("evm_mine");wall=Int(std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()));check(chain.tip().header.timestamp<=wall+1,"snapshot restores original time offset and scheduled timestamp is consumed");
 check(rpc.handle(request("evm_increaseTime",{"0xffffffffffffffff"}))["error"]["code"]==-32602,"time arithmetic overflow rejected");
 reopened_tip=chain.mine(sender,Int(chain.tip().header.timestamp+12));reopened_root=chain.state().root();check(reopened_tip->header.hash()!=removed_block.header.hash(),"new branch can replace reverted block height");



 }
 {Blockchain reopened(path,genesis);check(reopened.tip().header.hash()==reopened_tip->header.hash()&&reopened.state().root()==reopened_root,"reverted and rebuilt chain survives reopen");}
 std::filesystem::remove_all(path);
 // Contract creation simulations must agree with transaction collision rules.
 for(int occupied=0;occupied<3;++occupied){
  auto destination=create_address(sender,0);
  Json account={{"balance","0xa"}};
  if(occupied==1)account["code"]="0x00";
  if(occupied==2)account["nonce"]="0x1";
  auto creation_genesis=Genesis::from_json({{"config",{{"chainId",1337}}},{"alloc",{{sender,{{"balance","0x100000000000000000"}}},{destination,account}}}});
  Bytes before;
  {Blockchain chain(path,creation_genesis);Rpc rpc(chain);before=chain.state().root();
   Json invocation={{"from",sender},{"gas","0x186a0"},{"gasPrice","0x77359400"},{"value","0x5"},{"data","0x600060005360016000f3"}};
   auto invoke=[&](const std::string& method,const Json& data){return rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",Json::array({data})}});};
   auto result=invoke("eth_call",invocation),estimate=invoke("eth_estimateGas",invocation);
   auto copy=creation_genesis.state;ExecutionEngine engine(creation_genesis.chain_id);
   auto receipt=engine.apply_transaction(copy,SignedTransaction::sign(Transaction::from_json(invocation),key),creation_genesis.block.header,0,0);
   if(occupied){
    check(!receipt.success,"occupied creation fails in transaction engine");
    for(const auto& response:{result,estimate})check(response.contains("error")&&response["error"]["code"]==3&&response["error"]["message"].get<std::string>().find("collision")!=std::string::npos,"simulation rejects occupied contract address");
   }else{
    check(receipt.success&&result["result"]=="0x00","balance-only destination allows contract creation");
    check(estimate.contains("result")&&quantity(estimate["result"])==receipt.gas_used,"creation estimate includes runtime code deposit");
    invocation["gas"]=estimate["result"];check(invoke("eth_call",invocation)["result"]=="0x00","creation executes at estimated gas");
    invocation["gas"]=quantity(Int(receipt.gas_used-1));check(invoke("eth_call",invocation).contains("error"),"creation code deposit fails one gas below estimate");
   }
   check(chain.state().root()==before&&chain.store().state_at(0)->root()==before,"creation simulation never publishes state changes");
  }
  {Blockchain reopened(path,creation_genesis);check(reopened.state().root()==before,"creation simulation leaves reopened database unchanged");}
  std::filesystem::remove_all(path);
 }
 // A contract with no validators can still hold a deposit for a queried account.
 // Its getStake branch echoes the ABI address argument to verify its encoding.
 for(const auto& getter:{"60043560005260206000f3","600160005360016000f3","600160005500","60006000fd"}){
  Bytes code=unhex("60003560e01c63b7ab4db51461000057");auto branch=code.size()-3;
  auto payload=unhex(getter);code.insert(code.end(),payload.begin(),payload.end());
  code[branch]=uint8_t(code.size()>>8);code[branch+1]=uint8_t(code.size());
  payload=unhex("5b6020600052600060205260406000f3");code.insert(code.end(),payload.begin(),payload.end());
  auto staking=Genesis::from_json({{"config",{{"chainId",1337},{"consensus","pos"},{"stakingContract",contract}}},{"alloc",{{contract,{{"code",hex(code)}}}}}});
  {Blockchain chain(path,staking);Rpc rpc(chain);auto before=chain.state().root();
   auto response=rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method","nno_getStake"},{"params",Json::array({sender})}});
   if(std::string(getter)=="60043560005260206000f3")check(chain.validators().empty()&&response["result"]==quantity(integer(unhex(sender))),"contract stake reads unlisted account with correct ABI argument");
   else check(response["error"]["code"]==-32000,"malformed, mutating and reverting stake getters report failure");
   check(chain.state().root()==before&&chain.store().state_at(0)->root()==before,"stake query leaves canonical and durable state unchanged");
  }
  std::filesystem::remove_all(path);
 }
 {std::ifstream input("tests/fixtures/python-rpc.json");Json fixture;input>>fixture;
  check(fixture.at("vectors").size()==33,"Python read RPC fixture is complete");
  auto reference_genesis=Genesis::from_json(fixture.at("genesis"));
  {Blockchain chain(path,reference_genesis);Rpc rpc(chain);auto before=chain.state().root();
   for(const auto& vector:fixture.at("vectors")){
    auto actual=rpc.handle(vector.at("request"));
    auto name=vector.at("request").at("method").get<std::string>();
    auto expected=vector.at("response");
    // Keep the Python fixture intact; its integer status predates Ethereum
    // RPC quantity compatibility. Assert the replacement shape explicitly.
    if(name=="txpool_status")expected["result"]=Json{{"pending","0x0"},{"queued","0x0"}};
    check(actual==expected,("Historical read RPC response with explicit quantity correction: "+name).c_str());
   }
   check(chain.state().root()==before,"read RPC references do not mutate state");
  }
  std::filesystem::remove_all(path);
 }
 // Value is transferred on a simulation copy before CALLVALUE/SELFBALANCE execute.
 for(const auto& fork:{std::string("NanoExperimental"),std::string("Paris"),std::string("Shanghai")}){
  const std::string probe="0x00000000000000000000000000000000000030ab",reverter="0x00000000000000000000000000000000000030ac";
  Json config={{"chainId",1337}};if(fork!="NanoExperimental")config["evmFork"]=fork;
  auto value_genesis=Genesis::from_json({{"config",config},{"alloc",{{sender,{{"balance","0x100000000000000000"}}},{probe,{{"balance","0xa"},{"code","0x346000524760205260406000f3"}}},{reverter,{{"code","0x600160005560006000fd"}}}}}});
  {Blockchain chain(path,value_genesis);Rpc rpc(chain);auto original=chain.state().to_json();
   auto invoke=[&](const std::string& method,const Json& data){return rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",Json::array({data})}});};
   Json request={{"from",sender},{"to",probe},{"value","0x5"},{"gasPrice","0x77359400"},{"gas","0x186a0"},{"chainId","0x539"}};
   auto result=invoke("eth_call",request),estimate=invoke("eth_estimateGas",request);
   check(!result.contains("error")&&result["result"]==hex(concat(bytes(Int(5),32),bytes(Int(15),32))),"value call observes transferred balance");
   check(!estimate.contains("error"),"value call gas estimation succeeds");request["gas"]=estimate.at("result");
   check(invoke("eth_call",request).at("result")==result.at("result"),"value call executes at estimated gas");
   auto copy=value_genesis.state;auto tx=SignedTransaction::sign(Transaction::from_json(request),key);
   auto receipt=ExecutionEngine(1337,{},value_genesis.evm_revision).apply_transaction(copy,tx,value_genesis.block.header,0,0);
   check(receipt.success&&copy.get(probe).balance==15&&receipt.gas_used==quantity(estimate.at("result")),"signed value execution agrees with simulation estimate");
   for(const char* method:{"eth_call","eth_estimateGas"}){
    auto poor=request;poor["value"]=quantity(Int(value_genesis.state.get(sender).balance+1));check(invoke(method,poor).contains("error"),"unfunded value simulation rejected");
    auto revert=request;revert["to"]=reverter;revert["gas"]="0x186a0";check(invoke(method,revert).contains("error"),"value simulation propagates revert");
   }
   check(chain.state().to_json()==original,"successful/rejected/reverted value simulations leave canonical state unchanged");
  }
  std::filesystem::remove_all(path);
 }
 // Simulation argument validation must not silently discard fee or chain fields.
 for(const auto& fork:{std::string("Paris"),std::string("Shanghai")}){
  auto configured=Genesis::from_json({{"config",{{"chainId",1337},{"evmFork",fork}}},{"alloc",{{sender,{{"balance","0x100000000000000000"}}}}}});
  {Blockchain chain(path,configured);Rpc rpc(chain);auto before=chain.state().to_json();
   for(const auto& fields:std::vector<Json>{
     {{"gasPrice","0x77359400"},{"maxFeePerGas","0xb2d05e00"}},
     {{"gasPrice","0x77359400"},{"maxPriorityFeePerGas","0x1"}},
     {{"maxFeePerGas","0x1"},{"maxPriorityFeePerGas","0x2"}},
     {{"maxPriorityFeePerGas","0x1"}},{{"chainId","0x1"}},{{"type","0x100000000"}}}){
    Json request={{"from",sender},{"to",contract},{"gas","0x186a0"}};request.update(fields);
    for(const auto& method:{"eth_call","eth_estimateGas"}){
     auto result=rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",Json::array({request})}});
     check(result.contains("error"),"Simulation accepted inconsistent fee/chain/type arguments");
    }
   }
   check(chain.state().to_json()==before,"Invalid simulation arguments changed canonical state");
  }
  std::filesystem::remove_all(path);
 }
 // Missing/null RPC fees must not inject the signed transaction default GASPRICE.
 for(const auto& fork:{std::string("Paris"),std::string("Shanghai")}){
  auto configured=Genesis::from_json({{"config",{{"chainId",1337},{"evmFork",fork}}},{"alloc",{{sender,{{"balance","0x100000000000000000"}}},{price_contract,{{"code","0x3a60005260206000f3"}}},{price_guard,{{"code","0x6000353a14600d5760006000fd5b00"}}}}}});
  {Blockchain chain(path,configured);Rpc rpc(chain);auto before=chain.state().to_json();
   for(const auto& fields:std::vector<Json>{{{"gasPrice","0x77359400"},{"maxFeePerGas",nullptr}},{{"gasPrice","0x77359400"},{"maxPriorityFeePerGas",nullptr}},{{"gasPrice","0x77359400"},{"maxFeePerGas",nullptr},{"maxPriorityFeePerGas",nullptr}},{{"gasPrice",nullptr},{"maxFeePerGas",nullptr},{"maxPriorityFeePerGas",nullptr}},{{"maxFeePerGas",nullptr}},Json::object(),{{"gasPrice",nullptr}},{{"gasPrice","0x0"}},{{"maxFeePerGas","0x0"},{"maxPriorityFeePerGas","0x0"}}}){
    Json request={{"from",sender},{"to",price_contract},{"gas","0x186a0"}};request.update(fields);
    auto invoke=[&](const char* method){return rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",Json::array({request})}});};
    Int expected=fields.contains("gasPrice")&&!fields["gasPrice"].is_null()?quantity(fields["gasPrice"]):Int(0);
    auto result=invoke("eth_call");check(!result.contains("error")&&result["result"]==hex(bytes(expected,32)),"RPC nullable fee GASPRICE differs");
    request["to"]=price_guard;request["data"]=hex(bytes(expected,32));auto estimate=invoke("eth_estimateGas");check(!estimate.contains("error"),"RPC estimator misinterprets nullable fees");
    request["gas"]=estimate["result"];check(!invoke("eth_call").contains("error"),"Default price estimate cannot execute identical call");
   }
   check(chain.state().to_json()==before,"Fee default simulations changed state");
  }
  std::filesystem::remove_all(path);
 }
 // The simulation calldata selector follows Geth/Anvil, including null and empty aliases.
 for(const auto& fork:{std::string("Paris"),std::string("Shanghai")}){
  auto configured=Genesis::from_json({{"config",{{"chainId",1337},{"evmFork",fork}}},{"alloc",{{sender,{{"balance","0x100000000000000000"}}},{contract,{{"code","0x366000600037366000f3"}}}}}});
  {Blockchain chain(path,configured);Rpc rpc(chain);auto before=chain.state().to_json();
   std::vector<std::pair<Json,std::string>> cases={
    {Json::object(),"0x"},{{{"data","0x11"}},"0x11"},{{{"input","0x22"}},"0x22"},
    {{{"data","0x33"},{"input","0x33"}},"0x33"},{{{"data","0x11"},{"input","0x22"}},"0x22"},
    {{{"data",nullptr},{"input","0x44"}},"0x44"},{{{"data","0x55"},{"input",nullptr}},"0x55"},
    {{{"data",nullptr},{"input",nullptr}},"0x"},{{{"data","0x66"},{"input","0x"}},"0x"}};
   for(const auto& [fields,expected]:cases){
    Json request={{"from",sender},{"to",contract},{"gas","0x186a0"}};request.update(fields);
    auto invoke=[&](const char* method){return rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",Json::array({request})}});};
    auto result=invoke("eth_call");check(!result.contains("error")&&result["result"]==expected,"RPC calldata alias selects incorrect bytes");
    auto estimated=invoke("eth_estimateGas");check(!estimated.contains("error"),"RPC calldata alias estimation fails");request["gas"]=estimated["result"];
    result=invoke("eth_call");check(!result.contains("error")&&result["result"]==expected,"Estimate and call interpret calldata differently");
   }
   check(chain.state().to_json()==before,"Calldata alias simulation mutated state");
  }
  std::filesystem::remove_all(path);
 }
 // Estimation must honor the maximum fee cap, not merely the transferred value.
 for(const auto& fork:{std::string("Paris"),std::string("Shanghai")})for(unsigned type:{0u,1u,2u})for(int delta:{-1,0}){
  Int cap=type==2?Int(3000000000):Int(2000000000),balance=21000*cap+5+delta;
  auto funded=Genesis::from_json({{"config",{{"chainId",1337},{"evmFork",fork}}},{"alloc",{{sender,{{"balance",quantity(balance)}}}}}});
  {Blockchain chain(path,funded);Rpc rpc(chain);auto before=chain.state().to_json();
   Json tx={{"from",sender},{"to",contract},{"type",quantity(Int(type))},{"chainId","0x539"},{"value","0x5"},{"gas","0x186a0"}};
   if(type==2){tx["maxFeePerGas"]=quantity(cap);tx["maxPriorityFeePerGas"]="0x3b9aca00";}else tx["gasPrice"]=quantity(cap);
   auto estimate=[&](const Json& value){return rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method","eth_estimateGas"},{"params",Json::array({value})}});};
   auto result=estimate(tx);
   check(result.contains("error")==bool(delta<0),"Estimate ignores gas affordability at exact balance boundary");
   if(delta==0){check(result["result"]=="0x5208","Fundable transfer estimate differs");tx["gas"]=result["result"];auto replay=funded.state;auto receipt=ExecutionEngine(1337,{},funded.evm_revision).apply_transaction(replay,SignedTransaction::sign(Transaction::from_json(tx),key),funded.block.header,0,0);check(receipt.success,"Fundable estimate cannot execute signed transaction");}
   auto all_value=tx;all_value["value"]=quantity(balance);check(estimate(all_value).contains("error"),"Estimate reserves no gas funds when sending entire balance");
   auto free=Json{{"from",sender},{"to",contract},{"value","0x5"}};check(!estimate(free).contains("error"),"Omitted fee simulation should not invent an affordability cap");
   free["gasPrice"]="0x0";check(!estimate(free).contains("error"),"Zero fee simulation incorrectly capped");
   for(const auto& gas:{"0x0","0x1","0x5207"}){
    auto capped=tx;capped["gas"]=gas;auto estimated=estimate(capped);
    check(estimated.contains("error")==bool(delta<0),"Sub-intrinsic estimate gas differs from Geth");
    if(delta==0)check(estimated["result"]=="0x5208","Sub-intrinsic hint did not estimate actual required gas");
    auto call=rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method","eth_call"},{"params",Json::array({capped})}});check(call.contains("error"),"eth_call must retain explicit gas budget");
    auto signed_tx=SignedTransaction::sign(Transaction::from_json(capped),key);
    auto sent=rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method","eth_sendRawTransaction"},{"params",Json::array({hex(signed_tx.raw())})}});check(sent.contains("error"),"Signed transaction below intrinsic gas admitted");
   }
   check(chain.state().to_json()==before,"Fee affordability estimation mutated canonical state");
  }
  std::filesystem::remove_all(path);
 }
 // Diagnostic fork overrides below compare a supplied genesis execution context.
 // They do not activate modern-fork pending block construction on the network.
 // EIP-3541 rejection applies to every supported revision after Berlin.
 for(auto revision:{EvmRevision::NanoExperimental,EvmRevision::Berlin,EvmRevision::Paris,EvmRevision::Shanghai,EvmRevision::Cancun,EvmRevision::Prague,EvmRevision::Osaka}){
  auto creation_genesis=Genesis::from_json({{"config",{{"chainId",1337}}},{"alloc",{{sender,{{"balance","0x100000000000000000"}}}}}});
  creation_genesis.evm_revision=revision; // Diagnostic context, not a public genesis activation.
  {Blockchain chain(path,creation_genesis);Rpc rpc(chain);auto original=chain.state().to_json();
   for(const auto& prefix:{std::string("00"),std::string("ee"),std::string("ef")}){
    Json invocation={{"from",sender},{"chainId","0x539"},{"gasPrice","0x77359400"},{"gas","0x186a0"},{"data","0x60"+prefix+"60005360016000f3"}};
    auto invoke=[&](const char* method){return rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",Json::array({invocation,"latest"})}});};
    auto result=invoke("eth_call"),estimate=invoke("eth_estimateGas");auto copy=creation_genesis.state;
    auto receipt=ExecutionEngine(1337,{},revision).apply_transaction(copy,SignedTransaction::sign(Transaction::from_json(invocation),key),creation_genesis.block.header,0,0);
    bool forbidden=revision>=EvmRevision::Paris&&prefix=="ef";
    check(receipt.success==!forbidden,"signed creation prefix rule differs");
    check(result.contains("error")==forbidden,"RPC creation prefix rule differs from signed execution");
    if(estimate.contains("error")!=forbidden)throw std::runtime_error("RPC creation estimate prefix mismatch: revision="+std::to_string(static_cast<int>(revision))+" prefix="+prefix+" response="+estimate.dump());
    if(!forbidden)check(result.at("result")=="0x"+prefix,"creation simulation returns wrong runtime bytecode");
    check(chain.state().to_json()==original,"creation prefix simulation mutated canonical state");
   }
  }
  std::filesystem::remove_all(path);
 }
 // EIP-7623 is an admission floor, not execution gas charged before opcodes.
 for(auto revision:{EvmRevision::Cancun,EvmRevision::Prague,EvmRevision::Osaka}){
  auto floor_genesis=Genesis::from_json({{"config",{{"chainId",1337}}},{"alloc",{{sender,{{"balance","0x100000000000000000"}}}}}});floor_genesis.evm_revision=revision;
  const std::string gas_probe="0x00000000000000000000000000000000000040ab";floor_genesis.state.get(gas_probe).code=unhex("5a60005260206000f3");floor_genesis.block.header.state_root=floor_genesis.state.root();
  {Blockchain chain(path,floor_genesis);Rpc rpc(chain);auto original=chain.state().to_json();
   for(const auto& data:{Bytes(32,0),Bytes(32,255),concat(Bytes(16,0),Bytes(16,255))}){
    Int tokens=0,ordinary=21000;for(auto b:data){tokens+=b?4:1;ordinary+=b?16:4;}Int minimum=revision>=EvmRevision::Prague?Int(21000+10*tokens):ordinary;
    Json request={{"from",sender},{"to",contract},{"chainId","0x539"},{"gasPrice","0x77359400"},{"gas","0x186a0"},{"data",hex(data)}};
    auto invoke=[&](const char* method){return rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",Json::array({request,"latest"})}});};
    auto estimate=invoke("eth_estimateGas");check(!estimate.contains("error")&&quantity(estimate.at("result"))==minimum,"RPC calldata floor estimate differs");
    request["gas"]=quantity(minimum);check(!invoke("eth_call").contains("error"),"RPC rejects calldata floor boundary");
    auto copy=floor_genesis.state;auto receipt=ExecutionEngine(1337,{},revision).apply_transaction(copy,SignedTransaction::sign(Transaction::from_json(request),key),floor_genesis.block.header,0,0);
    check(receipt.success&&receipt.gas_used==minimum,"signed calldata floor differs from estimate");
    if(revision>=EvmRevision::Prague){request["to"]=gas_probe;auto budget=invoke("eth_call");check(!budget.contains("error")&&budget.at("result")==hex(bytes(Int(minimum-ordinary-2),32)),"calldata floor must not be prepaid as execution gas");request["to"]=contract;}
    request["gas"]=quantity(Int(minimum-1));check(invoke("eth_call").contains("error"),"RPC accepts gas below calldata floor");check(invoke("eth_estimateGas").contains("error"),"RPC estimate exceeds caller gas cap");
    check(chain.state().to_json()==original,"calldata floor simulations mutated canonical state");
   }
  }
  std::filesystem::remove_all(path);
 }
 // A delegated SELFDESTRUCT must distinguish a new account from an existing one.
 for(auto revision:{EvmRevision::Shanghai,EvmRevision::Cancun,EvmRevision::Prague,EvmRevision::Osaka}){
  const std::string helper="0x00000000000000000000000000000000000050ab",probe="0x00000000000000000000000000000000000050ac";
  const std::string init="0x600060006000600073"+helper.substr(2)+"620186a0f4504760005260206000f3";
  auto creation_genesis=Genesis::from_json({{"config",{{"chainId",1337}}},{"alloc",{{sender,{{"balance","0x100000000000000000"}}},{helper,{{"code","0x30ff"}}},{probe,{{"code",init}}}}}});creation_genesis.evm_revision=revision;
  {Blockchain chain(path,creation_genesis);Rpc rpc(chain);auto original=chain.state().to_json();
   for(bool creation:{false,true}){
    Json invocation={{"from",sender},{"chainId","0x539"},{"gasPrice","0x77359400"},{"gas","0x493e0"},{"value","0x5"}};if(creation)invocation["data"]=init;else invocation["to"]=probe;
    auto result=rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method","eth_call"},{"params",Json::array({invocation,"latest"})}});
    Int expected=creation||revision<EvmRevision::Cancun?0:5;
    check(!result.contains("error")&&result.at("result")==hex(bytes(expected,32)),"RPC created account SELFDESTRUCT context differs");
    auto copy=creation_genesis.state;auto receipt=ExecutionEngine(1337,{},revision).apply_transaction(copy,SignedTransaction::sign(Transaction::from_json(invocation),key),creation_genesis.block.header,0,0);
    auto target=creation?create_address(sender,0):probe;
    check(receipt.success&&static_cast<const State&>(copy).get(target).balance==expected,"signed delegated SELFDESTRUCT balance differs");
    if(creation||revision<EvmRevision::Cancun)check(!copy.exists(target),"destroyed account persists after signed execution");
    else check(copy.get(target).code==unhex(init),"existing account code deleted after Cancun");
    check(chain.state().to_json()==original,"SELFDESTRUCT simulation changed canonical state");
   }
  }
  std::filesystem::remove_all(path);
 }
 // Top-level calls to delegated accounts follow exactly one designation.
 for(auto revision:{EvmRevision::Cancun,EvmRevision::Prague,EvmRevision::Osaka})for(int mode=0;mode<5;++mode){
  const std::string owner="0x00000000000000000000000000000000000060ab",target="0x00000000000000000000000000000000000060ac";
  auto destination=mode==2?address("0x0000000000000000000000000000000000000004"):target;
  Json alloc={{sender,{{"balance","0x100000000000000000"}}},{owner,{{"code","0xef0100"+destination.substr(2)}}}};
  if(mode==0)alloc[target]={{"code","0x73"+target.substr(2)+"3150602a6001553060005260206000f3"}};
  if(mode==3)alloc[target]={{"code","0xef0100"+owner.substr(2)}};
  if(mode==4)alloc[target]={{"code","0x600160005560006000fd"}};
  auto delegated_genesis=Genesis::from_json({{"config",{{"chainId",1337}}},{"alloc",alloc}});delegated_genesis.evm_revision=revision;
  {Blockchain chain(path,delegated_genesis);Rpc rpc(chain);auto original=chain.state().to_json();
   Json invocation={{"from",sender},{"to",owner},{"chainId","0x539"},{"gasPrice","0x77359400"},{"gas","0x186a0"},{"data","0x1234"}};
   auto invoke=[&](const char* method){return rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",Json::array({invocation,"latest"})}});};
   auto result=invoke("eth_call"),estimate=invoke("eth_estimateGas");bool success=revision>=EvmRevision::Prague&&mode<3;
   check(result.contains("error")==!success,"RPC delegation status differs");check(estimate.contains("error")==!success,"RPC delegation estimate status differs");
   auto copy=delegated_genesis.state;auto receipt=ExecutionEngine(1337,{},revision).apply_transaction(copy,SignedTransaction::sign(Transaction::from_json(invocation),key),delegated_genesis.block.header,0,0);
   check(receipt.success==success,"signed delegation status differs");
   if(success){check(result.at("result")== (mode==0?hex(bytes(integer(unhex(owner)),32)):"0x"),"RPC delegation executes wrong code/address");check(quantity(estimate.at("result"))==receipt.gas_used,"RPC delegated gas/warm access differs");}
   check(copy.get_storage(owner,1)==(success&&mode==0?42:0)&&copy.get_storage(target,1)==0,"delegation uses wrong storage account");
   check(chain.state().to_json()==original,"delegated RPC changed canonical state");
  }
  std::filesystem::remove_all(path);
 }
 // A nested CALL can require more gas than it consumes because of EIP-150.
 // The measured-gas fast path must verify its quote and retain the fallback.
 for(const auto* fork:{"Paris","Shanghai"}){
  const std::string outer="0x00000000000000000000000000000000000070ab",inner="0x00000000000000000000000000000000000070ac";
  auto g=Genesis::from_json({{"config",{{"chainId",1337},{"evmFork",fork}}},{"alloc",{{sender,{{"balance","0x100000000000000000"}}},{outer,{{"code","0x6000600060006000600073"+inner.substr(2)+"5af160295760006000fd5b00"}}},{inner,{{"code","0x600160005500"}}}}}});
  {Blockchain chain(path,g);Rpc rpc(chain);auto root=chain.state().root();Json args={{"from",sender},{"to",outer},{"gasPrice","0x77359400"}};
   auto invoke=[&](const char* method){return rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",Json::array({args})}});};
   auto estimated=invoke("eth_estimateGas");check(!estimated.contains("error"),"nested EIP150 estimate succeeds");auto gas=quantity(estimated["result"]);
   Transaction tx;tx.to=outer;tx.gas=100000;tx.gas_price=2000000000;auto copy=g.state;
   ExecutionEngine engine(g.chain_id,{},g.evm_revision);auto observed=engine.apply_transaction(copy,SignedTransaction::sign(tx,key),g.block.header,0,0);
   check(observed.success&&gas>observed.gas_used,"nested fixture needs headroom above consumed gas");
   args["gas"]=quantity(observed.gas_used);check(invoke("eth_call").contains("error"),"consumed gas alone cannot execute nested call");
   args["gas"]=quantity(gas);check(invoke("eth_call")["result"]=="0x","verified fallback quote executes");
   // Independently locate the call's success boundary through eth_call.
   Int failing=observed.gas_used,passing=gas;
   while(passing-failing>1){Int mid=(passing+failing)/2;args["gas"]=quantity(mid);if(invoke("eth_call").contains("error"))failing=mid;else passing=mid;}
   check((gas-passing)*1000<=gas*15,"nested quote exceeds 1.5 percent of known call boundary");
   args["gas"]=quantity(passing);check(invoke("eth_estimateGas")["result"]==quantity(passing),"estimate honors a tight successful caller gas cap");
   copy=g.state;tx.gas=gas;auto receipt=engine.apply_transaction(copy,SignedTransaction::sign(tx,key),g.block.header,0,0);
   check(receipt.success&&copy.get_storage(inner,0)==1,"fallback gas executes signed transaction");check(chain.state().root()==root,"gas estimation does not commit nested writes");
  }
  std::filesystem::remove_all(path);
 }
 // Pending simulation uses next-block context and the executed pool, without committing it.
 for(const auto* fork:{"Paris","Shanghai"}){
  const std::string probe="0x00000000000000000000000000000000000040ad";
  auto g=Genesis::from_json({{"timestamp","0x1"},{"config",{{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bootstrapValidator",sender},{"evmFork",fork}}},{"alloc",{{sender,{{"balance","0x100000000000000000"}}},{probe,{{"code","0x6000544214601557426000556001546001016001555b00"},{"storage",{{"0x0","0x1"},{"0x1","0x1"}}}}}}}});
  {Blockchain chain(path,g);Rpc rpc(chain);Json args={{"from",sender},{"to",probe},{"gasPrice","0x77359400"}};
   auto invoke=[&](const char* method,const Json& input,const char* tag){return rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",Json::array({input,tag})}});};
   auto original=chain.state().root();auto cursor=chain.event_cursor();
   auto latest=invoke("eth_estimateGas",args,"latest"),pending=invoke("eth_estimateGas",args,"pending");
   check(!latest.contains("error")&&!pending.contains("error"),"timestamp probe estimates succeed");
   check(quantity(latest["result"])==23122&&quantity(pending["result"])==31039,"pending estimate accounts for next timestamp writes");
   args["gas"]=pending["result"];check(invoke("eth_call",args,"pending")["result"]=="0x","pending estimate executes in pending context");
   args["gas"]=latest["result"];check(invoke("eth_call",args,"pending").contains("error"),"latest estimate is insufficient in changed context");
   check(chain.state().root()==original&&chain.event_cursor()==cursor&&chain.pending_count()==0,"pending calls preserve canonical state events and pool");
   Transaction tx;tx.to=probe;tx.gas=quantity(pending["result"]);tx.gas_price=2000000000;
   check(chain.submit(SignedTransaction::sign(tx,key)),"admit timestamp probe");
   cursor=chain.event_cursor();auto candidate=chain.pending_execution(sender,10);
   check(candidate.first.number==1&&candidate.first.timestamp==10&&candidate.second.get_storage(probe,1)==2,"pending snapshot executes pool at candidate header");
   check(candidate.second.get(sender).nonce==1&&candidate.second.get_storage(probe,0)==10,"pending snapshot includes sender nonce and storage");
   args.erase("gas");auto after_pool=invoke("eth_estimateGas",args,"pending");
   check(after_pool["result"]==latest["result"],"pending simulation sees pool write at same candidate timestamp");
   check(chain.state().root()==original&&chain.event_cursor()==cursor&&chain.pending_count()==1,"candidate snapshot never consumes pending transactions");
   auto mined=chain.validate_signed(key,10);check(mined.transactions.size()==1&&chain.state().get_storage(probe,1)==2,"pending estimated transaction commits successfully");
  }
  std::filesystem::remove_all(path);
 }
 // Wallet-triggered estimates must release the RPC lock and honor cancellation.
 for(const auto* fork:{"Paris","Shanghai"}){
  const std::string probe="0x00000000000000000000000000000000000040ad";
  auto g=Genesis::from_json({{"config",{{"chainId",1337},{"evmFork",fork}}},{"alloc",{{sender,{{"balance","0x100000000000000000"}}},{probe,{{"code","0x60016000556000355b8015601557600190036008565b50602a60005260206000f3"}}}}}});
  auto keyfile=path.string()+"-estimate-wallet.json";Wallet(key).save_keyfile(keyfile,"fixture-password");setenv("NANO_RPC_ESTIMATE_PASSWORD","fixture-password",1);
  Json config={{"wallets",Json::array({Json{{"path",keyfile},{"password_env","NANO_RPC_ESTIMATE_PASSWORD"}}})}};
  {Blockchain chain(path,g);auto original=chain.state().root();
   for(const auto* method:{"eth_signTransaction","eth_sendTransaction"}){
    std::atomic<bool> paused=false,release=false;unsigned polls=0;
    RpcSimulationLimits limits;limits.now=[&]{if(++polls==20){paused=true;while(!release.load())std::this_thread::sleep_for(std::chrono::milliseconds(1));}return std::chrono::steady_clock::time_point{};};
    Rpc concurrent(chain,{},AdmissionBudget::Clock::now,limits);concurrent.load_wallets(config,"127.0.0.1");
    auto calculating=std::async(std::launch::async,[&]{return concurrent.handle({{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",Json::array({Json{{"from",sender},{"to",probe},{"data",hex(bytes(Int(6000),32))}}})}},[&]{if(release.load())throw ExecutionCancelled("wallet estimation cancelled");});});
    auto until=std::chrono::steady_clock::now()+std::chrono::seconds(2);while(!paused.load()&&std::chrono::steady_clock::now()<until)std::this_thread::sleep_for(std::chrono::milliseconds(1));
    auto reading=std::async(std::launch::async,[&]{return concurrent.handle({{"jsonrpc","2.0"},{"id",2},{"method","eth_blockNumber"}});});
    bool available=reading.wait_for(std::chrono::milliseconds(250))==std::future_status::ready;release=true;
    auto result=calculating.get(),read=reading.get();
    check(paused&&available&&read["result"]=="0x0","canonical RPC progresses during wallet estimate");
    check(result["error"]["code"]==-32002,"wallet estimate propagates cancellation");
    check(chain.state().root()==original&&chain.pending_count()==0,"cancelled wallet estimate never signs or submits a transaction");
   }
   for(bool revoke:{true,false}){
    std::atomic<bool> paused=false,release=false;unsigned polls=0;
    RpcSimulationLimits limits;limits.now=[&]{if(++polls==20){paused=true;while(!release.load())std::this_thread::sleep_for(std::chrono::milliseconds(1));}return std::chrono::steady_clock::time_point{};};
    Rpc concurrent(chain,{},AdmissionBudget::Clock::now,limits);concurrent.load_wallets(config,"127.0.0.1");
    auto calculating=std::async(std::launch::async,[&]{return concurrent.handle({{"jsonrpc","2.0"},{"id",3},{"method","eth_sendTransaction"},{"params",Json::array({Json{{"from",sender},{"to",probe},{"data",hex(bytes(Int(6000),32))}}})}});});
    auto until=std::chrono::steady_clock::now()+std::chrono::seconds(2);while(!paused.load()&&std::chrono::steady_clock::now()<until)std::this_thread::sleep_for(std::chrono::milliseconds(1));
    auto changing=std::async(std::launch::async,[&]()->Json{
     if(revoke){concurrent.load_wallets(Json::object(),"127.0.0.1");return true;}
     return concurrent.handle({{"jsonrpc","2.0"},{"id",4},{"method","eth_sendTransaction"},{"params",Json::array({Json{{"from",sender},{"to",sender},{"gas","0x5208"}}})}});
    });
    bool available=changing.wait_for(std::chrono::milliseconds(250))==std::future_status::ready;release=true;
    auto result=calculating.get(),changed=changing.get();check(paused&&available,"wallet reconfiguration and explicit-gas submission progress during estimate");
    if(revoke)check(result["error"]["code"]==-32000&&chain.pending_count()==0,"revoked wallet cannot sign after estimate");
    else{check(!result.contains("error")&&!changed.contains("error")&&chain.pending_count()==2,"concurrent wallet sends use distinct pending nonces");auto tx=chain.pending_transaction(unhex(result["result"].get<std::string>()));check(tx&&tx->tx.nonce==1,"implicit nonce is chosen after estimate");}
    check(chain.state().root()==original,"wallet estimation and admission leave canonical state unchanged");
   }
  }std::filesystem::remove_all(path);std::filesystem::remove(keyfile);unsetenv("NANO_RPC_ESTIMATE_PASSWORD");
 }
 // Pending preparation must share the host budget, including pooled EVM work.
 for(const auto* fork:{"Paris","Shanghai"}){
  const std::string probe="0x00000000000000000000000000000000000040ad";
  auto g=Genesis::from_json({{"config",{{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bootstrapValidator",sender},{"evmFork",fork}}},{"alloc",{{sender,{{"balance","0x100000000000000000"}}},{probe,{{"code","0x60016000555b600556"}}}}}});
  {Blockchain chain(path,g);Transaction tx;tx.to=probe;tx.gas=1000000;tx.gas_price=2000000000;
   check(chain.submit(SignedTransaction::sign(tx,key)),"admit costly pending transaction");
   auto original=chain.state().root();auto cursor=chain.event_cursor();unsigned ticks=0;
   RpcSimulationLimits limits;limits.timeout=std::chrono::milliseconds(20);limits.now=[&]{return std::chrono::steady_clock::time_point{}+std::chrono::milliseconds(ticks++);};
   Rpc rpc(chain,{},AdmissionBudget::Clock::now,limits);
   for(const auto* method:{"eth_call","eth_estimateGas"}){
    auto request=Json{{"jsonrpc","2.0"},{"id",44},{"method",method},{"params",Json::array({Json{{"to",sender}},"pending"})}};
    check(rpc.handle(request)["error"]["code"]==-32002,"pending pooled execution is covered by RPC deadline");
    check(chain.state().root()==original&&chain.event_cursor()==cursor&&chain.pending_count()==1,"pending cancellation preserves canonical state events and pool");
    request["params"][1]="latest";check(!rpc.handle(request).contains("error"),"latest request recovers after pending cancellation");
   }
   // Pause inside pooled EVM execution and require canonical reads to progress.
   std::atomic<bool> paused=false,release=false;unsigned clock_polls=0;
   RpcSimulationLimits paused_limits;paused_limits.now=[&]{if(++clock_polls==20){paused=true;while(!release.load())std::this_thread::sleep_for(std::chrono::milliseconds(1));}return std::chrono::steady_clock::time_point{};};
   Rpc concurrent(chain,{},AdmissionBudget::Clock::now,paused_limits);
   auto calculating=std::async(std::launch::async,[&]{return concurrent.handle({{"jsonrpc","2.0"},{"id",45},{"method","eth_call"},{"params",Json::array({Json{{"to",sender}},"pending"})}},[&]{if(release.load())throw ExecutionCancelled("released pending probe");});});
   auto until=std::chrono::steady_clock::now()+std::chrono::seconds(2);while(!paused.load()&&std::chrono::steady_clock::now()<until)std::this_thread::sleep_for(std::chrono::milliseconds(1));
   auto reading=std::async(std::launch::async,[&]{return concurrent.handle({{"jsonrpc","2.0"},{"id",46},{"method","eth_blockNumber"},{"params",Json::array()}});});
   bool available=reading.wait_for(std::chrono::milliseconds(250))==std::future_status::ready;release=true;
   auto cancelled_response=calculating.get(),read_response=reading.get();
   check(paused&&available&&read_response["result"]=="0x0","canonical RPC progresses while pending candidate EVM is paused");
   check(cancelled_response["error"]["code"]==-32002&&chain.state().root()==original&&chain.pending_count()==1,"isolated pending cancellation preserves canonical state and pool");
   State trial=chain.state();Header header;header.coinbase=sender;header.gas_limit=30000000;header.base_fee=1000000000;
   unsigned polls=0;ExecutionEngine engine(1337,{},fork==std::string("Paris")?EvmRevision::Paris:EvmRevision::Shanghai);
   auto bounded=engine.with_checkpoint([&]{if(++polls==10)throw ExecutionCancelled("pending test cancellation");});
   bool cancelled=false;try{bounded.apply_transaction(trial,SignedTransaction::sign(tx,key),header,0,0);}catch(const ExecutionCancelled&){cancelled=true;}
   check(cancelled&&trial.root()==original,"engine cancellation rolls back fees nonce and storage on actual trial state");
   Block candidate;candidate.header=header;candidate.transactions.push_back(SignedTransaction::sign(tx,key));auto before=candidate.to_json();polls=0;cancelled=false;
   try{bounded.execute_block(trial,candidate);}catch(const ExecutionCancelled&){cancelled=true;}
   check(cancelled&&trial.root()==original&&candidate.to_json()==before,"cancelled block execution is atomic for state and advertised block");
   auto mined=chain.validate_signed(key,10);check(mined.transactions.size()==1&&chain.pending_count()==0,"consensus production remains free of RPC cancellation");
   check(chain.state().get_storage(probe,0)==0,"ordinary OOG transaction rolls back storage");
  }
  std::filesystem::remove_all(path);
 }
 // A pending snapshot keeps its ancestor hashes even across a development rewind.
 for(const auto* fork:{"Paris","Shanghai"}){
  const std::string probe="0x00000000000000000000000000000000000040ad";
  auto g=Genesis::from_json({{"config",{{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bootstrapValidator",sender},{"evmFork",fork}}},{"alloc",{{sender,{{"balance","0x100000000000000000"}}},{probe,{{"code","0x60014060005500"}}}}}});
  {Blockchain chain(path,g);auto first=chain.validate_signed(key,10);Transaction tx;tx.to=probe;tx.gas=100000;tx.gas_price=2000000000;
   check(chain.submit(SignedTransaction::sign(tx,key)),"admit pending BLOCKHASH probe");auto snapshot=chain.pending_snapshot(sender,20);auto expected=snapshot.execute();
   check(expected.second.get_storage(probe,0)==integer(first.header.hash()),"pending candidate uses captured parent hash");
   check(chain.restore(0,g.block.header.hash()),"rewind pending snapshot fixture");
   // Build an externally supplied alternate block for this development-only rewind.
   // The local signer correctly resumes its durable proposal at this height.
   auto replacement=chain.block_template(sender,11);sign_block(replacement,key,1337,g.block.header.hash());chain.import_block(replacement,11);check(replacement.header.hash()!=first.header.hash(),"replacement parent differs");
   auto canonical=chain.state().root();auto cursor=chain.event_cursor();auto replay=snapshot.execute();
   check(replay.first.hash()==expected.first.hash()&&replay.second.root()==expected.second.root(),"pending snapshot remains deterministic after ancestor replacement");
   check(chain.state().root()==canonical&&chain.event_cursor()==cursor&&chain.pending_count()==0,"detached pending execution never reinserts transactions or mutates canonical state");
  }
  std::filesystem::remove_all(path);
 }
 // Deterministic host-clock cancellation: no sleeps or consensus timeouts.
 for(const auto& fork:{std::string("Paris"),std::string("Shanghai")}){
  auto cancel_path=std::filesystem::temp_directory_path()/("nano-rpc-cancel-"+fork+"-"+std::to_string(getpid()));
  const std::string leaf="0x00000000000000000000000000000000000040ad",outer="0x00000000000000000000000000000000000040ae",empty="0x00000000000000000000000000000000000040af";
  auto outer_code="0x60026001556000600060006000600073"+leaf.substr(2)+"5af150602a60005260206000f3";
  auto genesis=Genesis::from_json({{"config",{{"evmFork",fork}}},{"alloc",{{leaf,{{"code","0x60016000555b600556"}}},{outer,{{"code",outer_code}}}}}});
  {
   Blockchain chain(cancel_path,genesis);auto original=chain.state().root();auto events=chain.event_cursor();unsigned ticks=0;
   RpcSimulationLimits limits;limits.timeout=std::chrono::milliseconds(10);limits.now=[&]{return std::chrono::steady_clock::time_point{}+std::chrono::milliseconds(ticks++);};
   Rpc rpc(chain,{},AdmissionBudget::Clock::now,limits);
   for(const auto& target:{leaf,outer})for(const auto& method:{"eth_call","eth_estimateGas"})for(const auto& tag:{"latest","pending"}){
    auto response=rpc.handle({{"jsonrpc","2.0"},{"id",42},{"method",method},{"params",Json::array({Json{{"to",target},{"gas","0x1c9c380"}},tag})}});
    check(response["error"]["code"]==-32002&&response["id"]==42,"simulation timeout escapes nested CALL and estimation as host error");
    check(chain.state().root()==original&&chain.event_cursor()==events&&chain.pending_count()==0,"cancelled simulation preserves canonical state and pool");
   }
   // An outer contract deliberately ignores the child's failure. Cancellation
   // must still escape, restore both frames and never masquerade as EVM OOG.
   State state=chain.state();ExecutionContext ctx;ctx.revision=fork=="Paris"?EvmRevision::Paris:EvmRevision::Shanghai;ctx.address=outer;ctx.code=unhex(outer_code);ctx.gas=300000;
   unsigned checkpoints=0;ctx.checkpoint=[&]{if(++checkpoints==10)throw ExecutionCancelled("test cancellation");};bool cancelled=false;
   try{EVM evm(state);(void)evm.execute(ctx);}catch(const ExecutionCancelled&){cancelled=true;}
   check(cancelled&&state.root()==original,"cancellation restores nested frame writes on the actual execution state");
   ctx.checkpoint={};EVM unrestricted(state);auto normal=unrestricted.execute(ctx);
   check(normal.success&&normal.return_data==bytes(Int(42),32)&&state.get_storage(outer,1)==2&&state.get_storage(leaf,0)==0,"execution without host cancellation retains ordinary child-OOG semantics");
   // Preparation now contributes three clock checkpoints to this synthetic budget.
   limits.timeout=std::chrono::milliseconds(8);Rpc cumulative(chain,{},AdmissionBudget::Clock::now,limits);
   auto request=Json{{"jsonrpc","2.0"},{"id",43},{"method","eth_call"},{"params",Json::array({Json{{"to",empty},{"gas","0x1c9c380"}}})}};
   check(cumulative.handle(request)["result"]=="0x","one cheap simulation fits deterministic clock budget");
   request["method"]="eth_estimateGas";check(cumulative.handle(request)["error"]["code"]==-32002,"estimate retries share one deadline instead of resetting it");
   request["method"]="eth_call";check(cumulative.handle(request)["result"]=="0x","new request receives a fresh budget after cancellation");
  }
  std::filesystem::remove_all(cancel_path);
 }
 // Four nested EIP-150 forwarding layers should not repeatedly execute a
 // costly leaf merely to find headroom. Count deterministic checkpoints,
 // not wall time, while independently verifying the returned quote.
 for(const auto* fork:{"Paris","Shanghai"}){
  Json allocations={{sender,{{"balance","0x100000000000000000"}}}};
  std::string leaf="0x00000000000000000000000000000000000080ad",target=leaf;
  allocations[leaf]={{"code","0x60016000556000355b8015601557600190036008565b50602a60005260206000f3"}};
  for(unsigned level=1;level<=4;++level){auto outer=hex(bytes(Int(0x80ad+level),20));allocations[outer]={{"code","0x3660006000376020600060206000600073"+target.substr(2)+"5af1602f5760006000fd5b60206000f3"}};target=outer;}
  auto g=Genesis::from_json({{"config",{{"chainId",1337},{"evmFork",fork}}},{"alloc",allocations}});
  auto probe_path=path.string()+"-depth-estimate";
  {Blockchain chain(probe_path,g);auto original=chain.state().root();size_t clock_reads=0;
   RpcSimulationLimits limits;limits.now=[&]{++clock_reads;return std::chrono::steady_clock::time_point{};};Rpc rpc(chain,{},AdmissionBudget::Clock::now,limits);
   Json args={{"from",sender},{"to",target},{"gas","0x1c9c380"},{"data",hex(bytes(Int(40000),32))}};
   auto invoke=[&](const char* method){return rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",Json::array({args,"latest"})}});};
   auto called=invoke("eth_call");check(called.at("result")==hex(bytes(Int(42),32)),"deep estimate control executes");auto call_reads=clock_reads;clock_reads=0;
   auto quote=invoke("eth_estimateGas");check(quote.contains("result"),"deep estimate succeeds");auto estimate_reads=clock_reads;
   std::cout<<Json{{"scenario","deep-estimate-probe-count"},{"fork",fork},{"callCheckpoints",call_reads},{"estimateCheckpoints",estimate_reads}}.dump()<<std::endl;
   check(estimate_reads*2<=call_reads*11,"deep estimation repeats costly execution beyond bounded probe budget");
   auto gas=quantity(quote.at("result"));args["gas"]=quote.at("result");check(invoke("eth_call").at("result")==called.at("result"),"deep quote independently executes");
   Transaction tx;tx.chain_id=1337;tx.to=target;tx.gas=gas;tx.gas_price=2000000000;tx.data=bytes(Int(40000),32);auto state=g.state;ExecutionEngine engine(g.chain_id,{},g.evm_revision);auto receipt=engine.apply_transaction(state,SignedTransaction::sign(tx,key),g.block.header,0,0);
   check(receipt.success&&state.get_storage(leaf,0)==1&&chain.state().root()==original,"deep quote executes signed transaction without committing RPC writes");
   Int low=receipt.gas_used-1,high=gas;while(high-low>1){Int mid=(high+low)/2;args["gas"]=quantity(mid);if(invoke("eth_call").contains("error"))low=mid;else high=mid;}
   check((gas-high)*1000<=gas*15,"deep quote exceeds 1.5 percent of independently found boundary");
   State created_state=g.state;ExecutionContext creation;creation.origin=sender;creation.caller=sender;creation.address=target;creation.revision=g.evm_revision;creation.gas=300000;
   creation.code=unhex("0x3660006000373660006000f05060006000f3");
   creation.data=unhex("0x6000600060006000600073"+leaf.substr(2)+"5af15060006000f3");
   EVM created_vm(created_state);auto created=created_vm.execute(creation);
   check(created.success&&created.max_call_depth==2&&created_state.get_storage(leaf,0)==1,"observed depth includes CREATE constructor CALL without changing execution");
   std::cout<<Json{{"scenario","deep-estimate-checkpoints"},{"fork",fork},{"callCheckpoints",call_reads},{"estimateCheckpoints",estimate_reads},{"gasQuote",quantity(gas)},{"minimumVerified",quantity(high)}}.dump()<<'\n';
  }std::filesystem::remove_all(probe_path);
 }
 std::cout<<checks<<" RPC checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
