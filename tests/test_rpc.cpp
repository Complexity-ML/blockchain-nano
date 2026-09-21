#include "nano/rpc.hpp"
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
using namespace nano;
static int checks=0;
void check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
int main(){try{
 auto key=bytes(Int(1),32);auto sender=public_address(public_key(key));std::string contract="0x00000000000000000000000000000000000000ab";
 Json alloc={{sender,{{"balance","0x100000000000000000"}}},{contract,{{"code","0x602a60005560005460005260206000f3"},{"balance","0x0"}}}};
 std::string hash_contract="0x00000000000000000000000000000000000000ac";alloc[hash_contract]={{"code","0x6000354060005260206000f3"}};
 std::string price_contract="0x00000000000000000000000000000000000000ad";
 std::string price_guard="0x00000000000000000000000000000000000000ae";
 alloc[price_contract]={{"code","0x3a60005260206000f3"}};
 alloc[price_guard]={{"code","0x6000353a14600d5760006000fd5b00"}};
 auto genesis=Genesis::from_json({{"config",{{"chainId",1337}}},{"alloc",alloc}});
 auto path=std::filesystem::temp_directory_path()/("nano-rpc-"+std::to_string(getpid()));
 std::optional<Block> reopened_tip;Bytes reopened_root;
 {Blockchain chain(path,genesis);Rpc rpc(chain);
 auto request=[](const std::string& method,Json params=Json::array()){return Json{{"jsonrpc","2.0"},{"id",7},{"method",method},{"params",params}};};
 auto call=[&](const std::string& method,Json params=Json::array()){auto r=rpc.handle(request(method,params));if(r.contains("error"))throw std::runtime_error(r.dump());return r["result"];};
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
  auto gas=quantity(call("eth_estimateGas",{dynamic}));dynamic["gas"]=quantity(gas);
  auto simulated=call("eth_call",{dynamic});check(simulated=="0x","dynamic fee estimate executes price-dependent contract");
  auto copy=genesis.state;ExecutionEngine engine(genesis.chain_id);
  auto receipt=engine.apply_transaction(copy,SignedTransaction::sign(Transaction::from_json(dynamic),key),genesis.block.header,0,0);
  check(receipt.success&&receipt.gas_used==gas,"estimated dynamic fee gas agrees with signed transaction execution");
  check(chain.state().root()==genesis.state.root(),"fee simulation leaves canonical state unchanged");
 }
 check(call("eth_call",{Json{{"to",price_contract},{"gasPrice","0x7"}}})==hex(bytes(Int(7),32)),"legacy call preserves explicit GASPRICE");
 Transaction tx;tx.to=contract;tx.gas=100000;tx.gas_price=2000000000;auto signed_tx=SignedTransaction::sign(tx,key);
 check(call("eth_sendRawTransaction",{hex(signed_tx.raw())})==hex(signed_tx.hash()),"submit raw transaction");
 check(call("eth_getTransactionCount",{sender,"pending"})=="0x1"&&call("eth_getTransactionCount",{sender,"latest"})=="0x0","pending and mined nonce differ");
 check(call("eth_getTransactionByHash",{hex(signed_tx.hash())})["blockHash"].is_null(),"pending transaction metadata");
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
 for(const auto& invalid:Json::array({Json::array({"0x0","latest"}),Json::array({"0x401","latest"}),Json::array({"0x1","latest",Json::array({50,10})}),Json::array({"0x1","latest",Json::array({101})}),Json::array({"0x1","latest",Json::array({"50"})})}))
  check(rpc.handle(request("eth_feeHistory",invalid))["error"]["code"]==-32602,"invalid fee history parameters rejected");
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
    check(actual==vector.at("response"),("Python read RPC response: "+name).c_str());
   }
   check(chain.state().root()==before,"read RPC references do not mutate state");
  }
  std::filesystem::remove_all(path);
 }
 std::cout<<checks<<" RPC checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
