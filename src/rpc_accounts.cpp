#include "nano/rpc.hpp"
#include <cstdlib>
namespace nano {
void Rpc::load_wallets(const Json& config,const std::string& bind){
 std::lock_guard lock(mutex_);auto entries=config.value("wallets",Json::array());
 auto fees=RpcFeePolicy::from_json(config.value("fee_policy",Json::object()));
 if(!entries.is_array()||entries.size()>100)throw std::invalid_argument("wallets must be an array of at most 100 accounts");
 bool dev=config.value("dev_mode",false);
 if((dev||!entries.empty())&&bind!="127.0.0.1"&&bind!="::1"&&bind!="localhost")throw std::invalid_argument("development and unlocked wallet RPC require a loopback bind address");
 auto coinbase=address(config.value("coinbase",std::string("0x0000000000000000000000000000000000000000")));
 auto interval=u64(quantity(config.value("block_time",Json(12))));if(interval<1||interval>3600)throw std::invalid_argument("block_time must be between 1 and 3600 seconds");
 KeyStore candidate;
 for(const auto& entry:entries){
  auto variable=entry.at("password_env").get<std::string>();if(variable.empty())throw std::invalid_argument("wallet password_env is required");
  const char* password=std::getenv(variable.c_str());if(!password)throw std::invalid_argument("wallet password environment variable is missing");
  candidate.add_wallet(Wallet::from_keyfile(entry.at("path").get<std::string>(),password),entry.value("name",std::string{}));
 }
 std::unique_ptr<BftReactor> bft;
 if(config.contains("bft")){
  const auto& b=config.at("bft");auto variable=b.at("key_env").get<std::string>();const char* key=std::getenv(variable.c_str());
  if(!key)throw std::invalid_argument("BFT key environment variable is missing");
  bft=std::make_unique<BftReactor>(chain_,b.at("journal").get<std::string>(),unhex(key));
 }
 bft_=std::move(bft);
 fee_policy_=std::move(fees);fee_head_.reset();
 validating_=false;validator_error_.clear();block_time_=interval;
 wallets_=std::move(candidate);dev_mode_=dev;coinbase_=coinbase;if(!dev_mode_){snapshots_.clear();time_offset_=0;next_timestamp_.reset();}
}
Json Rpc::account_request(const std::string& method,const Json& params,const Estimate& estimate){
 bool transaction=method=="eth_sendTransaction"||method=="eth_signTransaction";
 if(params.size()!=(transaction?1:2))throw RpcError(-32602,"Invalid parameter count");
 std::string sender;
 if(transaction){if(!params[0].is_object()||!params[0].contains("from"))throw RpcError(-32602,"Transaction from is required");sender=address(params[0]["from"].get<std::string>());}
 else sender=address(params[method=="personal_sign"?1:0].get<std::string>());
 auto wallet=wallets_.get_wallet(sender);if(!wallet)throw RpcError(-32000,"Account is not unlocked");
 if(method=="eth_sign"||method=="personal_sign")return hex(wallet->sign_message(unhex(params[method=="personal_sign"?0:1].get<std::string>())));
 if(method=="eth_signTypedData"||method=="eth_signTypedData_v4"){
  Json data=params[1].is_string()?Json::parse(params[1].get<std::string>()):params[1];
  if(data.at("domain").contains("chainId")&&quantity(data["domain"]["chainId"])!=chain_.genesis().chain_id)throw RpcError(-32602,"Typed data chain ID mismatch");
  return hex(wallet->sign_typed_data(data));
 }
 if(method=="eth_sendTransaction")charge_transactions(1);
 Json data=params[0];
 if(data.contains("chainId")&&quantity(data["chainId"])!=chain_.genesis().chain_id)throw RpcError(-32602,"Transaction chain ID mismatch");
 data["chainId"]=quantity(chain_.genesis().chain_id);
 if(data.contains("data")&&data.contains("input")&&unhex(data["data"].get<std::string>())!=unhex(data["input"].get<std::string>()))throw RpcError(-32602,"Conflicting transaction data and input");
 auto type=u64(quantity(data.value("type",Json(data.contains("maxFeePerGas")||data.contains("maxPriorityFeePerGas")?2:data.contains("accessList")?1:0))));
 if(type>2)throw RpcError(-32602,"Unsupported transaction type");data["type"]=quantity(Int(std::to_string(type)));
 if(type==2){
  if(data.contains("gasPrice"))throw RpcError(-32602,"Type 2 cannot specify gasPrice");
  if(!data.contains("maxPriorityFeePerGas")){
   Int tip=suggested_tip(chain_.tip_header());
   if(data.contains("maxFeePerGas"))tip=std::min(tip,quantity(data["maxFeePerGas"]));
   data["maxPriorityFeePerGas"]=quantity(tip);
  }
  if(!data.contains("maxFeePerGas"))data["maxFeePerGas"]=quantity(std::min(Int((Int(1)<<256)-1),Int(chain_.tip_header().base_fee.value_or(Int(0))*2+quantity(data["maxPriorityFeePerGas"]))));
  if(quantity(data["maxPriorityFeePerGas"])>quantity(data["maxFeePerGas"]))throw RpcError(-32602,"Priority fee exceeds fee cap");
 }else{
  if(data.contains("maxFeePerGas")||data.contains("maxPriorityFeePerGas"))throw RpcError(-32602,"Dynamic fees require type 2");
  if(!data.contains("gasPrice"))data["gasPrice"]=dispatch("eth_gasPrice",Json::array());
 }
 if(!data.contains("gas"))data["gas"]=estimate?estimate(data):dispatch("eth_estimateGas",Json::array({data}));
 // Estimation may release the RPC lock. Recheck signing authority and choose
 // the implicit nonce only after reacquiring it, immediately before submission.
 wallet=wallets_.get_wallet(sender);if(!wallet)throw RpcError(-32000,"Account is not unlocked");
 if(!data.contains("nonce"))data["nonce"]=quantity(chain_.pending_nonce(sender));
 auto signed_tx=wallet->sign_transaction(Transaction::from_json(data));
 if(method=="eth_signTransaction")return Json{{"raw",hex(signed_tx.raw())},{"tx",signed_tx.to_json()}};
 if(!chain_.submit(signed_tx))throw RpcError(-32000,"Transaction rejected by pool");return hex(signed_tx.hash());
}
}
