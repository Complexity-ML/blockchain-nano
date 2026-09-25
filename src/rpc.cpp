#include "nano/rpc_timing.hpp"
#include "nano/rpc.hpp"
#include "../network/altair_rpc_service.hpp"
namespace nano {
namespace {
Json error_response(const Json& id,int code,const std::string& message,const Json& data=nullptr){Json error={{"code",code},{"message",message}};if(!data.is_null())error["data"]=data;return {{"jsonrpc","2.0"},{"id",id},{"error",error}};}
void arity(const Json& p,size_t min,size_t max){if(p.size()<min||p.size()>max)throw RpcError(-32602,"Invalid parameter count");}
Bytes hash_arg(const Json& j){auto value=unhex(j.get<std::string>());if(value.size()!=32)throw RpcError(-32602,"Expected 32-byte hash");return value;}
Json tx_json(const SignedTransaction& tx,const Block* block=nullptr,size_t index=0){auto result=tx.to_json();result["blockHash"]=block?Json(hex(block->header.hash())):Json(nullptr);result["blockNumber"]=block?Json(quantity(block->header.number)):Json(nullptr);result["transactionIndex"]=block?Json(quantity(Int(index))):Json(nullptr);return result;}
Json block_json(const Block& b,bool full){auto result=b.to_json(full);if(full)for(size_t i=0;i<b.transactions.size();++i)result["transactions"][i]=tx_json(b.transactions[i],&b,i);return result;}
Transaction simulation_transaction(Json call){
 // Nullable RPC fee fields are absent, not transaction-type selectors.
 for(const auto* name:{"gasPrice","maxFeePerGas","maxPriorityFeePerGas"})if(call.contains(name)&&call[name].is_null())call.erase(name);
 // Geth/Anvil simulations prefer input (including empty input) over data.
 for(const auto* name:{"data","input"})if(call.contains(name)){
  if(call[name].is_null())call.erase(name);
  else (void)unhex(call[name].get<std::string>());
 }
 if(call.contains("input"))call["data"]=call["input"];
 auto tx=Transaction::from_json(call);
 if(tx.type<2&&!call.contains("gasPrice"))tx.gas_price=0;
 return tx;
}
ExecutionResult simulate(const State& state,const Header& header,const Int& chain_id,const Json& call,uint64_t gas,const std::map<Int,Bytes>& ancestors,EvmRevision revision,const std::function<void()>& checkpoint){
 checkpoint();
 auto copy=state;Transaction tx=simulation_transaction(call);if(tx.type>2)throw RpcError(-32602,"Transaction type unavailable on Nano network");auto sender=address(call.value("from",std::string("0x0000000000000000000000000000000000000000")));
 Int intrinsic=transaction_intrinsic_gas(tx,revision);if(Int(std::to_string(gas))<intrinsic){ExecutionResult out;out.error="intrinsic gas exceeds limit";return out;}
 Int floor=transaction_floor_gas(tx,revision);if(Int(std::to_string(gas))<floor){ExecutionResult out;out.error="transaction gas below calldata floor";return out;}
 auto destination=tx.to?address(*tx.to):create_address(sender,state.get(sender).nonce);
 if(!tx.to&&(state.get(destination).nonce!=0||!state.get(destination).code.empty()||(revision!=EvmRevision::NanoExperimental&&!state.get(destination).storage.empty()))){
  ExecutionResult out;out.error="contract address collision";out.gas_used=gas;return out;
 }
 if(!copy.transfer(sender,destination,tx.value))throw RpcError(-32000,"Insufficient funds for call value");
 ExecutionContext ctx;ctx.checkpoint=checkpoint;ctx.revision=revision;ctx.access_list=tx.access_list;ctx.origin=sender;ctx.caller=sender;ctx.address=destination;ctx.value=tx.value;ctx.gas=gas-u64(intrinsic);ctx.chain_id=chain_id;ctx.gas_price=tx.effective_gas_price(header.base_fee.value_or(Int(0)));ctx.coinbase=header.coinbase;ctx.timestamp=header.timestamp;ctx.number=header.number;ctx.prevrandao=block_randomness(header,revision);ctx.gas_limit=header.gas_limit;ctx.base_fee=header.base_fee.value_or(Int(0));if(revision>=EvmRevision::Cancun)ctx.blob_base_fee=blob_base_fee(header.excess_blob_gas.value_or(Int(0)),revision);ctx.block_hash=[&ancestors](const Int& number){auto it=ancestors.find(number);if(it==ancestors.end())throw std::runtime_error("missing ancestor block");return it->second;};
 if(tx.to){ctx.data=tx.data;ctx.code=copy.get(destination).code;}else{ctx.code=tx.data;ctx.contract_creation=true;copy.get(destination).nonce=1;}
 if(revision>=EvmRevision::Prague&&tx.to)if(auto target=delegation_target(ctx.code)){ctx.authorization_warm_addresses.insert(*target);ctx.code=is_precompile(integer(unhex(*target)),revision)?Bytes{}:static_cast<const State&>(copy).get(*target).code;}
 EVM evm(copy);auto result=evm.execute(ctx);
 if(result.success&&!tx.to){uint64_t deposit=result.return_data.size()*200;if(result.return_data.size()>24576||deposit>ctx.gas-result.gas_used||((revision>=EvmRevision::Paris)&&!result.return_data.empty()&&result.return_data[0]==0xef)){result.success=false;result.error="contract code deposit out of gas";}else result.gas_used+=deposit;}
 result.gas_used+=u64(intrinsic);result.gas_used=std::max(result.gas_used,u64(floor));return result;
}
}
Json Rpc::network_block(const Block& block,bool full)const{
 auto result=full?block.to_transport_json():block.to_json(false);if(!full)result["transactionEncoding"]="signed-rlp-hashes-v1";result["uncleHeaders"]=Json::array();
 for(const auto& header:block.uncles)result["uncleHeaders"].push_back(header.to_json());
 auto certificate=chain_.store().certificate_at(block.header.number);
 if(certificate){
  Json votes=Json::array();for(const auto& vote:certificate->votes)votes.push_back(encode_vote_message(vote,chain_.genesis().chain_id,chain_.genesis().block.header.hash()));
  result["bftCertificate"]={{"version",1},{"round",quantity(Int(std::to_string(certificate->round)))},{"votes",std::move(votes)}};
 }else if(chain_.genesis().bft_version&&block.header.number!=0)throw RpcError(-32000,"Missing BFT certificate");
 return result;
}
Json Rpc::network_status()const{
 bool fresh=!network_report_.empty()&&std::chrono::steady_clock::now()-network_seen_<std::chrono::seconds(5);
 Json peers=fresh?network_report_["peers"]:Json::array();Int current=chain_.tip().header.number,highest=current;
 for(const auto& peer:peers)highest=std::max(highest,quantity(peer["bestBlock"]));
 return {{"syncing",highest>current},{"currentBlock",quantity(current)},{"highestBlock",quantity(highest)},{"peerCount",peers.size()},{"ready",highest==current},{"networkConnected",!peers.empty()},{"listening",fresh},{"peerId",fresh?network_report_["peerId"]:Json(nullptr)},{"addresses",fresh?network_report_["addresses"]:Json::array()},{"peers",peers},{"bootnodes",fresh?network_report_.value("bootnodes",Json::array()):Json::array()}};
}
std::optional<Block> Rpc::block(const Json& tag)const{
 if(!tag.is_string())throw RpcError(-32602,"Expected block tag");auto text=tag.get<std::string>();
 if(text=="latest")return chain_.tip();
 if(text=="earliest")return chain_.store().block_by_number(0);
 if(text=="pending"||text=="safe"||text=="finalized")throw RpcError(-32000,"Block tag is not available yet");
 return chain_.store().block_by_number(quantity(tag));
}
Int Rpc::development_timestamp()const{
 auto tip=chain_.tip();Int timestamp;
 if(next_timestamp_)timestamp=*next_timestamp_;
 else{Int now(std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()));timestamp=std::max(Int(now+time_offset_),Int(tip.header.timestamp+chain_.genesis().minimum_block_interval));}
 u64(timestamp);if(timestamp<=tip.header.timestamp)throw RpcError(-32602,"Next timestamp must exceed parent timestamp");return timestamp;
}
void Rpc::validator_tick(){
 std::lock_guard lock(mutex_);block_imports_.expire(admission_now_());auto now=std::chrono::steady_clock::now();
 if(bft_){try{Int wall(std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()));bft_->tick(now,wall);validator_error_.clear();}catch(const std::exception& error){validator_error_=error.what();}return;}
 if(!validating_||now<validator_due_)return;
 validator_due_=now+std::chrono::seconds(block_time_);
 try{
  if(!network_status()["ready"].get<bool>())return;
  auto selected=chain_.next_validator();if(!selected||address(*selected)!=coinbase_)return;
  Int timestamp(std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()));
  if(timestamp<chain_.tip().header.timestamp+chain_.genesis().minimum_block_interval)return;
  if(chain_.genesis().pos_signature_version){
   auto wallet=wallets_.get_wallet(coinbase_);if(!wallet)throw std::runtime_error("producer wallet is locked");
   auto block=chain_.block_template(coinbase_,timestamp);block=chain_.resume_signing_proposal(block,wallet->address());chain_.reserve_signature(block,wallet->address());wallet->sign_block(block,chain_.genesis().chain_id,chain_.genesis().block.header.hash());chain_.import_block(block,timestamp);
  }else chain_.validate(coinbase_,timestamp);
  validator_error_.clear();
 }catch(const std::exception& e){validator_error_=e.what();}
}
Json Rpc::dispatch(const std::string& method,const Json& p,const Estimate& estimate){
 if(method=="nano_bftManifest"){
  arity(p,0,1);if(!p.empty()&&!p[0].is_boolean())throw RpcError(-32602,"BFT page selector must be boolean");
  if(!bft_)return nullptr;auto result=bft_->snapshot_page(!p.empty()&&p[0].get<bool>(),true);
  if(result.dump().size()>8*1024*1024)throw RpcError(-32000,"BFT manifest requires further pagination");return result;
 }
 if(method=="nano_bftTransactionFragment"){
  arity(p,3,3);if(!bft_)throw RpcError(-32001,"BFT proposal unavailable");
  return bft_->proposal_fragment(hash_arg(p[0]),hash_arg(p[1]),u64(quantity(p[2])));
 }
 if(method=="nano_bftExchange"||method=="nano_bftExchangePage"||method=="nano_bftExchangeManifest"){
  const bool manifest=method=="nano_bftExchangeManifest";const bool paged=manifest||method=="nano_bftExchangePage";arity(p,0,paged?2:1);
  bool previous=false;if(p.size()==2){if(!p[1].is_boolean())throw RpcError(-32602,"BFT page selector must be boolean");previous=p[1].get<bool>();}
  if(!bft_)return nullptr;
  if(!p.empty()&&!p[0].is_null()){
   auto now=std::chrono::steady_clock::now();Int wall(std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()));
   auto incoming=p[0];
   if(manifest&&!incoming.at("proposal").is_null()){bft_->validate_manifest(incoming,now);incoming["proposal"]=nullptr;}
   bft_->receive(incoming,now,wall);
   // Include newly enabled local votes in this reply, rather than waiting for
   // the periodic tick and another relay exchange. Validation precedes signing.
   bft_->tick(now,wall);
  }
  auto result=paged?bft_->snapshot_page(previous,manifest):bft_->snapshot();
  if(manifest&&result.dump().size()>8*1024*1024)throw RpcError(-32000,"BFT manifest requires further pagination");return result;
 }

 if(method=="evm_mine"||method=="evm_increaseTime"||method=="evm_setNextBlockTimestamp"){
  if(!dev_mode_)throw RpcError(-32601,"Development mode is disabled");
  if(method=="evm_increaseTime"){
   arity(p,1,1);Int seconds=quantity(p[0]);Int now(std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()));u64(Int(now+time_offset_+seconds));time_offset_+=seconds;return quantity(seconds);
  }
  if(method=="evm_setNextBlockTimestamp"){
   arity(p,1,1);Int timestamp=quantity(p[0]);u64(timestamp);if(timestamp<=chain_.tip().header.timestamp)throw RpcError(-32602,"Next timestamp must exceed parent timestamp");next_timestamp_=timestamp;return quantity(timestamp);
  }
  arity(p,0,0);auto timestamp=development_timestamp();Block produced;
  if(chain_.genesis().consensus=="pow")produced=chain_.mine(coinbase_,timestamp);
  else{auto selected=chain_.next_validator();if(!selected)throw RpcError(-32000,"No active validator");if(chain_.genesis().pos_signature_version){auto wallet=wallets_.get_wallet(*selected);if(!wallet)throw RpcError(-32000,"selected producer wallet must be unlocked");produced=chain_.block_template(*selected,timestamp);produced=chain_.resume_signing_proposal(produced,wallet->address());chain_.reserve_signature(produced,wallet->address());wallet->sign_block(produced,chain_.genesis().chain_id,chain_.genesis().block.header.hash());chain_.import_block(produced,timestamp);}else produced=chain_.validate(*selected,timestamp);}
  next_timestamp_.reset();return hex(produced.header.hash());
 }
 if(method=="evm_snapshot"||method=="evm_revert"){
  if(!dev_mode_)throw RpcError(-32601,"Development mode is disabled");
  if(method=="evm_snapshot"){
   arity(p,0,0);if(snapshots_.size()>=1024)throw RpcError(-32005,"Too many development snapshots");
   auto tip=chain_.tip();auto id=next_snapshot_++;snapshots_.emplace(id,DevSnapshot{tip.header.number,tip.header.hash(),time_offset_,next_timestamp_});return quantity(Int(std::to_string(id)));
  }
  arity(p,1,1);auto id=u64(quantity(p[0]));auto it=snapshots_.find(id);if(it==snapshots_.end())return false;
  if(!chain_.restore(it->second.number,it->second.hash))return false;
  time_offset_=it->second.time_offset;next_timestamp_=it->second.next_timestamp;
  snapshots_.erase(it,snapshots_.end());filters_.clear();return true;
 }
 if(method=="web3_clientVersion"){arity(p,0,0);return "Nano/0.1.0/c++20";}
 if(method=="web3_sha3"){arity(p,1,1);return hex(keccak(unhex(p[0].get<std::string>())));}
 if(method=="eth_chainId"||method=="net_version"){arity(p,0,0);return method=="net_version"?chain_.genesis().chain_id.get_str():quantity(chain_.genesis().chain_id);}
 if(method=="eth_blockNumber"){arity(p,0,0);return quantity(chain_.tip().header.number);}
 if(method=="eth_mining"){arity(p,0,0);return false;}
 if(method=="net_listening"){arity(p,0,0);return network_status()["listening"];}
 if(method=="eth_syncing"){arity(p,0,0);auto status=network_status();if(!status["syncing"].get<bool>())return false;return Json{{"startingBlock","0x0"},{"currentBlock",status["currentBlock"]},{"highestBlock",status["highestBlock"]}};}
 if(method=="eth_hashrate"){arity(p,0,0);return "0x0";}
 if(method=="net_peerCount"){arity(p,0,0);return quantity(quantity(network_status()["peerCount"]));}
 if(method=="eth_protocolVersion"){arity(p,0,0);return "0x41";}
 if(method=="eth_feeHistory"){arity(p,2,3);return fee_history(p);}
 if(method=="eth_accounts"){arity(p,0,0);return wallets_.addresses();}
 if(method=="eth_coinbase"){arity(p,0,0);return coinbase_;}
 if(method=="eth_gasPrice"||method=="eth_maxPriorityFeePerGas"){arity(p,0,0);return quantity(method=="eth_gasPrice"?Int(chain_.tip().header.base_fee.value_or(Int(0))+1000000000):Int(1000000000));}
 if(method=="eth_getBalance"||method=="eth_getCode"||method=="eth_getTransactionCount"||method=="eth_getStorageAt"){
  bool storage=method=="eth_getStorageAt";arity(p,storage?2:1,storage?3:2);auto addr=address(p[0].get<std::string>());Json tag=p.size()>(storage?2:1)?p[storage?2:1]:Json("latest");
  if(method=="eth_getTransactionCount"&&tag=="pending")return quantity(chain_.pending_nonce(addr));
  auto b=block(tag);if(!b)throw RpcError(-32000,"Unknown block");auto state=chain_.store().state_at(b->header.number);if(!state)throw RpcError(-32000,"State unavailable");
  const auto& account=static_cast<const State&>(*state).get(addr);
  if(storage)return hex(bytes(state->get_storage(addr,quantity(p[1])),32));
  if(method=="eth_getCode")return hex(account.code);
  return quantity(method=="eth_getBalance"?account.balance:account.nonce);
 }
 if(method=="eth_getBlockByNumber"||method=="eth_getBlockByHash"||method=="eth_getBlockTransactionCountByNumber"||method=="eth_getBlockTransactionCountByHash"||method=="eth_getTransactionByBlockNumberAndIndex"||method=="eth_getTransactionByBlockHashAndIndex"){
  bool by_hash=method.find("Hash")!=std::string::npos;bool count=method.find("Count")!=std::string::npos;bool tx=method.ends_with("AndIndex");arity(p,tx?2:1,count?1:2);
  auto b=by_hash?chain_.store().block_by_hash(hash_arg(p[0])):block(p[0]);if(!b)return nullptr;
  if(count)return quantity(Int(b->transactions.size()));
  if(tx){auto index=quantity(p[1]);if(index>=Int(b->transactions.size()))return nullptr;return tx_json(b->transactions[size_t(u64(index))],&*b,size_t(u64(index)));}
  return block_json(*b,p.size()==2?p[1].get<bool>():false);
 }
 if(method=="eth_getTransactionByHash"){arity(p,1,1);auto hash=hash_arg(p[0]);auto mined=chain_.store().transaction(hash);if(mined)return *mined;auto pending=chain_.pending_transaction(hash);return pending?tx_json(*pending):Json(nullptr);}
 if(method=="eth_getTransactionReceipt"){
  arity(p,1,1);auto receipt=chain_.store().receipt(hash_arg(p[0]));if(!receipt)return nullptr;auto result=receipt->to_json();auto b=chain_.store().block_by_number(receipt->block_number);size_t offset=0;if(!b)throw std::runtime_error("receipt references missing block");
  for(size_t i=0;i<u64(receipt->index);++i){auto previous=chain_.store().receipt(b->transactions.at(i).hash());if(!previous)throw std::runtime_error("missing previous receipt");offset+=previous->logs.size();}
  for(size_t i=0;i<result["logs"].size();++i){auto& log=result["logs"][i];log["blockHash"]=hex(receipt->block_hash);log["blockNumber"]=quantity(receipt->block_number);log["transactionHash"]=hex(receipt->transaction_hash);log["transactionIndex"]=quantity(receipt->index);log["logIndex"]=quantity(Int(offset+i));log["removed"]=false;}
  result["effectiveGasPrice"]=quantity(b->transactions.at(size_t(u64(receipt->index))).tx.effective_gas_price(b->header.base_fee.value_or(Int(0))));return result;
 }
 if(method=="eth_sign"||method=="personal_sign"||method=="eth_signTypedData"||method=="eth_signTypedData_v4"||method=="eth_signTransaction"||method=="eth_sendTransaction")return account_request(method,p,estimate);
 if(method=="eth_sendRawTransaction"){arity(p,1,1);const auto& raw=p[0].get_ref<const std::string&>();if(raw.size()>2*1024*1024+2)throw RpcError(-32000,"transaction exceeds Nano pool byte limit");charge_transactions(1);auto tx=SignedTransaction::decode(unhex(raw));if(!chain_.submit(tx))throw RpcError(-32000,"Transaction rejected by pool");return hex(tx.hash());}
 if(method=="txpool_content"||method=="txpool_status"){arity(p,0,0);auto result=chain_.pool_info(method=="txpool_content");if(method=="txpool_status")for(const auto* field:{"pending","queued"})result[field]=quantity(Int(std::to_string(result.at(field).get<uint64_t>())));return result;}
 if(method=="nno_startValidating"||method=="nno_stopValidating"){
  arity(p,0,0);
  if(!wallets_.get_wallet(coinbase_))throw RpcError(-32000,"Configured coinbase wallet must be unlocked");
  if(method=="nno_stopValidating"){validating_=false;return true;}
  if(chain_.genesis().consensus!="pos")throw RpcError(-32000,"Validation requires Proof of Stake");
  if(validating_)return true;
  bool eligible=false;auto validators=chain_.validators();
  for(const auto& validator:validators)if(address(validator["address"])==coinbase_&&validator["is_active"].get<bool>())eligible=true;
  auto selected=chain_.next_validator();if(validators.empty()&&selected&&address(*selected)==coinbase_)eligible=true;
  if(!eligible)throw RpcError(-32000,"Configured coinbase is not an active validator");
  validating_=true;validator_error_.clear();validator_due_=std::chrono::steady_clock::now();return true;
 }
 if(method=="nno_registerValidator"||method=="nno_addStake"||method=="nno_withdrawStake"){
  bool withdraw=method=="nno_withdrawStake";arity(p,withdraw?1:2,withdraw?1:2);
  auto sender=address(p[0].get<std::string>());const auto& target=chain_.genesis().staking_contract;
  if(target.empty()||chain_.state().get(target).code.empty())throw RpcError(-32000,"Staking contract is not deployed");
  auto signature=withdraw?std::string("withdrawStake()"):method=="nno_addStake"?std::string("addStake()"):std::string("registerValidator()");
  auto selector=keccak(Bytes(signature.begin(),signature.end()));selector.resize(4);
  Int value=withdraw?Int(0):quantity(p[1]);if(!withdraw&&value==0)throw RpcError(-32602,"Stake amount must be positive");
  Json tx={{"from",sender},{"to",target},{"value",quantity(value)},{"data",hex(selector)}};
  // The inclusion block changes lockedUntil even if the estimate sees the same height.
  Int gas_estimate=quantity(estimate?estimate(tx):dispatch("eth_estimateGas",Json::array({tx})));
  tx["gas"]=quantity(std::min(chain_.tip().header.gas_limit,Int(gas_estimate+25000)));
  return account_request("eth_sendTransaction",Json::array({tx}),estimate);
 }
 if(method=="nno_getStake"){arity(p,1,1);auto addr=address(p[0].get<std::string>());try{return quantity(chain_.stake(addr));}catch(const std::exception&){throw RpcError(-32000,"Staking query failed");}}
 if(method=="nno_getValidators"){arity(p,0,0);return chain_.validators();}
 if(method=="eth_call"||method=="eth_estimateGas")return prepare_simulation(method,p)();
 if(method=="eth_getLogs"){arity(p,1,1);return logs(p[0]);}
 if(method=="eth_newFilter"||method=="eth_newBlockFilter"||method=="eth_newPendingTransactionFilter"){
  bool is_logs=method=="eth_newFilter";arity(p,is_logs?1:0,is_logs?1:0);if(filters_.size()>=1024)throw RpcError(-32005,"Too many filters");
  auto f=stream(is_logs?"logs":method=="eth_newBlockFilter"?"newHeads":"newPendingTransactions",is_logs?p[0]:Json::object());auto id=f.id;filters_.emplace(id,std::move(f));return id;
 }
 if(method=="eth_uninstallFilter"){arity(p,1,1);return filters_.erase(p[0].get<std::string>())!=0;}
 if(method=="eth_getFilterChanges"||method=="eth_getFilterLogs"){
  arity(p,1,1);auto it=filters_.find(p[0].get<std::string>());if(it==filters_.end())throw RpcError(-32000,"Filter not found");
  if(method=="eth_getFilterLogs"){if(it->second.type!="logs")throw RpcError(-32602,"Not a log filter");return logs(it->second.criteria);}
  auto changes=poll(it->second);if(it->second.type=="newHeads")for(auto& header:changes)header=header["hash"];return changes;
 }
 if(method=="nano_networkReport"){
  arity(p,2,2);if(network_token_.empty()||!p[0].is_string()||p[0].get<std::string>()!=network_token_)throw RpcError(-32001,"Unauthorized network report");
  const auto& report=p[1];if(!report.is_object()||!report.at("peerId").is_string()||report.at("peerId").get<std::string>().size()>128||!report.at("addresses").is_array()||report.at("addresses").size()>32||!report.at("peers").is_array()||report.at("peers").size()>50)throw RpcError(-32602,"Invalid network report");
  for(const auto& addr:report["addresses"])if(!addr.is_string()||addr.get<std::string>().size()>1024)throw RpcError(-32602,"Invalid listen address");
  for(const auto& peer:report["peers"]){if(!peer.at("peerId").is_string()||peer.at("peerId").get<std::string>().size()>128)throw RpcError(-32602,"Invalid peer");u64(quantity(peer.at("bestBlock")));}
  network_report_=report;network_seen_=std::chrono::steady_clock::now();return true;
 }
 if(method=="nno_getNodes"){
  arity(p,0,0);auto status=network_status();return Json{{"rpcNodes",Json::array()},{"bootnodes",status["bootnodes"]},{"connectedPeers",status["peers"]},{"totalPeers",status["peerCount"]}};
 }
 if(method=="nno_getNetworkInfo"){
  arity(p,0,0);auto status=network_status();auto tip=chain_.tip();return Json{{"network",{{"name","Nano"},{"chainId",u64(chain_.genesis().chain_id)},{"consensus",chain_.genesis().consensus},{"stakingContract",chain_.genesis().staking_contract.empty()?Json(nullptr):Json(chain_.genesis().staking_contract)}}},{"node",{{"version","0.1.0"},{"validating",validating_},{"validationError",validator_error_.empty()?Json(nullptr):Json(validator_error_)},{"peerId",status["peerId"]},{"listenAddresses",status["addresses"]},{"listening",status["listening"]}}},{"chain",{{"blockNumber",u64(tip.header.number)},{"latestBlockHash",hex(tip.header.hash())},{"gasLimit",u64(tip.header.gas_limit)}}},{"peers",{{"count",status["peerCount"]},{"maxPeers",50},{"connected",status["peers"]}}}};
 }
 if(method=="nano_networkStatus"){
  arity(p,0,0);auto tip=chain_.tip();return Json{{"chain_id",u64(chain_.genesis().chain_id)},{"network_id",u64(chain_.genesis().chain_id)},{"best_block",u64(tip.header.number)},{"best_hash",hex(tip.header.hash())},{"genesis_hash",hex(chain_.genesis().block.header.hash())},{"cursor",chain_.event_cursor()},{"gossip_epoch",chain_.event_epoch()},{"version","nano/1.0.0"}};
 }
 if(method=="nano_beginBftImport"||method=="nano_finishBftImport"){
  arity(p,2,2);if(network_token_.empty()||!p[0].is_string()||p[0].get<std::string>()!=network_token_)throw RpcError(-32001,"Unauthorized BFT import");
  if(!bft_)throw RpcError(-32001,"BFT reactor unavailable");auto now=std::chrono::steady_clock::now();
  try{
   if(method=="nano_beginBftImport"){
    if(p[1].dump().size()>8*1024*1024)throw BlockImportLimit("BFT manifest capacity exceeded");
    bft_->validate_manifest(p[1],now);return block_imports_.begin_proposal(p[1],admission_now_());
   }
   auto id=p[1].get<std::string>();if(id.size()!=34)throw RpcError(-32602,"Invalid BFT import identifier");
   auto envelope=block_imports_.finish_proposal(id,admission_now_());
   Int wall(std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()));
   // Assembly never grants vote/commit authority. Recheck the live height/round,
   // execute the complete proposal, and validate votes through the ordinary path.
   bft_->receive(envelope,now,wall);bft_->tick(now,wall);
   return Json{{"received",true},{"blockHash",envelope.at("proposal").at("hash")},{"height",envelope.at("height")},{"round",envelope.at("round")}};
  }catch(const BlockImportLimit& e){throw RpcError(-32005,e.what());}
 }
 if(method=="nano_beginBlockImport"||method=="nano_appendBlockImport"||method=="nano_finishBlockImport"||method=="nano_abortBlockImport"){
  arity(p,method=="nano_appendBlockImport"?4:2,method=="nano_appendBlockImport"?4:2);
  if(network_token_.empty()||!p[0].is_string()||p[0].get<std::string>()!=network_token_)throw RpcError(-32001,"Unauthorized block import");
  auto now=admission_now_();
  try{
   if(method=="nano_beginBlockImport"){
    const auto& manifest=p[1];auto header=Header::from_json(manifest.at("block"));auto tip=chain_.tip();
    if(header.parent_hash!=tip.header.hash()||header.number!=tip.header.number+1)throw RpcError(-32602,"Import must extend the current canonical tip");
    return block_imports_.begin(manifest,now);
   }
   auto id=p[1].get<std::string>();if(id.size()!=34)throw RpcError(-32602,"Invalid block import identifier");
   if(method=="nano_abortBlockImport")return block_imports_.abort(id,now);
   if(method=="nano_appendBlockImport")return block_imports_.append(id,u64(quantity(p[2])),p[3],now);
   auto block=block_imports_.finish(id,now);
   // The transfer has no authority of its own. Reuse all ordinary execution,
   // producer, root and certificate checks before committing the completed block.
   return dispatch("validator_submitBlock",Json::array({std::move(block)}));
  }catch(const BlockImportLimit& e){throw RpcError(-32005,e.what());}
 }
 if(method=="nano_networkBlockManifest"||method=="nano_networkBlockManifestAt"){
  arity(p,1,1);auto block=method=="nano_networkBlockManifest"?chain_.store().block_by_hash(hash_arg(p[0])):chain_.store().block_by_number(quantity(p[0]));if(!block)throw RpcError(-32001,"Unknown block");
  Json result={{"version",1},{"block",network_block(*block,false)}};
  if(result.dump().size()>8*1024*1024)throw RpcError(-32000,"Block manifest requires further pagination");return result;
 }
 if(method=="nano_networkTransactionFragment"){
  arity(p,3,3);auto block_hash=hash_arg(p[0]),tx_hash=hash_arg(p[1]);uint64_t offset;
  try{offset=u64(quantity(p[2]));}catch(const std::exception&){throw RpcError(-32602,"Invalid fragment offset");}
  constexpr size_t chunk_bytes=256*1024;
  if(offset%chunk_bytes)throw RpcError(-32602,"Fragment offset must be aligned");
  auto stored=chain_.store().transaction(tx_hash);if(!stored)throw RpcError(-32001,"Unknown transaction");
  if(stored->at("blockHash")!=hex(block_hash))throw RpcError(-32602,"Transaction does not belong to requested block");
  SignedTransaction tx{Transaction::from_json(*stored),quantity(stored->at("v")),quantity(stored->at("r")),quantity(stored->at("s"))};auto raw=tx.raw();
  if(tx.hash()!=tx_hash)throw RpcError(-32000,"Stored transaction hash mismatch");
  if(offset>=raw.size())throw RpcError(-32602,"Fragment offset outside transaction");
  const size_t end=static_cast<size_t>(offset)+std::min(chunk_bytes,raw.size()-static_cast<size_t>(offset));
  return Json{{"version",1},{"blockHash",hex(block_hash)},{"transactionHash",hex(tx_hash)},{"offset",quantity(Int(std::to_string(offset)))},{"total",quantity(Int(raw.size()))},{"next",end==raw.size()?Json(nullptr):Json(quantity(Int(end)))},{"data",hex(Bytes(raw.begin()+static_cast<size_t>(offset),raw.begin()+end))}};
 }
 if(method=="nano_networkBlocks"){
  arity(p,1,1);Int start=quantity(p[0]),height=chain_.tip().header.number;
  Json result={{"blocks",Json::array()},{"total",u64(height)},{"start",u64(Int(start+1))}};auto& blocks=result["blocks"];size_t size=result.dump().size();
  // Eight MiB is a batching target, not a reason to hide an existing first
  // block. Keep the complete peer page within the 16 MiB transport envelope.
  constexpr size_t target=8*1024*1024,maximum=16*1024*1024-1024;
  for(Int number=start+1;number<=height&&number<=start+50;++number){
   auto b=chain_.store().block_by_number(number);if(!b)throw RpcError(-32000,"Missing block");auto json=network_block(*b);auto addition=json.dump().size()+(blocks.empty()?0:1);
   if(!blocks.empty()&&addition>target-std::min(size,target))break;
   if(addition>maximum-size)throw RpcError(-32000,"Block requires transport beyond the single-page size limit");
   size+=addition;blocks.push_back(std::move(json));
  }
  return result;
 }
 if(method=="nano_gossipPage"){
  arity(p,4,4);return chain_.gossip_page(u64(quantity(p[0])),p[1].get<std::string>(),p[2].get<std::string>(),p[3].get<bool>());
 }
 if(method=="nano_receiveTransactions"){
  arity(p,1,1);const auto& batch=p[0];if(!batch.is_array()||batch.size()>50)throw RpcError(-32602,"gossip batch exceeds 50 transactions");
  size_t total=0;for(const auto& raw:batch){if(!raw.is_string()||raw.get_ref<const std::string&>().size()>2*1024*1024+2)throw RpcError(-32602,"invalid gossip transaction size");total+=raw.get_ref<const std::string&>().size();if(total>8*1024*1024)throw RpcError(-32602,"gossip batch exceeds byte limit");}
  charge_transactions(batch.size());
  Json accepted=Json::array();for(const auto& raw:batch){try{auto tx=SignedTransaction::decode(unhex(raw.get_ref<const std::string&>()));bool known=chain_.pending_transaction(tx.hash()).has_value()||chain_.store().transaction(tx.hash()).has_value();accepted.push_back(known||chain_.submit(tx));}catch(const std::invalid_argument&){accepted.push_back(false);}}
  return Json{{"processed",batch.size()},{"accepted",accepted}};
 }
 if(method=="nano_networkEventRefs"){
  arity(p,2,2);return chain_.block_reference_page(u64(quantity(p[0])),p[1].get<std::string>());
 }
 if(method=="nano_networkEvents"){
  arity(p,1,1);auto cursor=u64(quantity(p[0]));auto events=chain_.events_since(cursor);Json result=Json::array();size_t size=128;
  for(const auto& event:events){Json value;
   if(event.type=="block"){auto b=chain_.store().block_by_hash(unhex(event.value.at("hash")));if(b){value=network_block(*b);}}
   else{auto hash=unhex(event.value.get<std::string>());auto tx=chain_.pending_transaction(hash);if(tx)value=hex(tx->raw());else{auto stored=chain_.store().transaction(hash);if(stored){SignedTransaction mined{Transaction::from_json(*stored),quantity(stored->at("v")),quantity(stored->at("r")),quantity(stored->at("s"))};value=hex(mined.raw());}}}
   if(result.size()>=50)break;
   if(!value.is_null()){
    Json entry={{"type",event.type},{"value",std::move(value)}};auto addition=entry.dump().size()+(result.empty()?0:1);
    if(!result.empty()&&addition>8*1024*1024-std::min(size,size_t(8*1024*1024)))break;
    if(addition>16*1024*1024-1024-size)throw RpcError(-32000,"Event requires transport beyond the single-page size limit");
    size+=addition;result.push_back(std::move(entry));
   }
   cursor=event.sequence;
  }
  return Json{{"cursor",cursor},{"events",result}};
 }
 if(method=="validator_getBlockTemplate"){
  arity(p,1,1);auto coinbase=address(p[0].get<std::string>());auto now=Int(std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()));auto timestamp=std::max(now,Int(chain_.tip().header.timestamp+chain_.genesis().minimum_block_interval));
  if(timestamp>now+chain_.genesis().maximum_future_drift)throw RpcError(-32000,"Next permitted timestamp is too far in the future");
  auto candidate=chain_.block_template(coinbase,timestamp);auto result=candidate.to_json(true);result["blockNumber"]=quantity(candidate.header.number);result["coinbase"]=candidate.header.coinbase;result["posSignatureVersion"]=chain_.genesis().pos_signature_version;result["genesisHash"]=hex(chain_.genesis().block.header.hash());return result;
 }
 if(method=="validator_submitBlock"){
  arity(p,1,1);
  try{
   auto candidate=Block::from_json(p[0]);
   if(p[0].contains("coinbase")&&address(p[0]["coinbase"].get<std::string>())!=address(candidate.header.coinbase))throw std::invalid_argument("coinbase alias mismatch");
   if(p[0].contains("blockNumber")&&quantity(p[0]["blockNumber"])!=candidate.header.number)throw std::invalid_argument("block number alias mismatch");
   auto now=Int(std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()));
   if(candidate.header.number==0||transactions_root(candidate.transactions)!=candidate.header.transactions_root)throw std::invalid_argument("invalid submitted block");
   chain_.verify_authentication(candidate);
   auto existing=chain_.store().block_by_hash(candidate.header.hash());
   if(p[0].contains("bftCertificate")){
    const auto& proof=p[0].at("bftCertificate");
    if(!proof.is_object()||proof.size()!=3||!proof.at("version").is_number_integer()||proof.at("version")!=1||!proof.at("round").is_string()||proof.at("round").get_ref<const std::string&>().size()>18||!proof.at("votes").is_array()||proof.at("votes").empty())throw std::invalid_argument("invalid BFT certificate envelope");
    auto round=u64(quantity(proof.at("round")));if(quantity(Int(std::to_string(round)))!=proof.at("round").get_ref<const std::string&>())throw std::invalid_argument("noncanonical BFT round");
    auto validators=chain_.voting_set();
    if(existing){auto stored=chain_.store().certificate_at(candidate.header.number);if(!stored)throw std::invalid_argument("existing block has no certificate");validators=stored->validators;}
    if(proof.at("votes").size()>validators.size())throw std::invalid_argument("too many certificate votes");
    std::vector<FinalityVote> votes;
    for(const auto& encoded:proof.at("votes")){
     if(!encoded.is_string())throw std::invalid_argument("certificate vote must be a bounded wire string");
     votes.push_back(decode_vote_message(encoded.get_ref<const std::string&>(),validators,chain_.genesis().chain_id,chain_.genesis().block.header.hash(),u64(candidate.header.number),round));
    }
    if(!verify_commit_certificate(votes,validators,u64(candidate.header.number),round,candidate.header.hash(),chain_.genesis().chain_id,chain_.genesis().block.header.hash()))throw std::invalid_argument("invalid BFT commit certificate");
    if(!existing)chain_.import_certified_block(candidate,round,votes,now);
   }else if(chain_.genesis().bft_version)throw std::invalid_argument("BFT block requires certificate");
   else if(!existing)chain_.import_block(candidate,now);
   if(existing)candidate=std::move(*existing);
   Int fees=0;for(const auto& tx:candidate.transactions){auto receipt=chain_.store().receipt(tx.hash());if(!receipt)throw std::runtime_error("missing committed receipt");fees+=receipt->gas_used*tx.tx.effective_gas_price(candidate.header.base_fee.value_or(Int(0)));}
   return Json{{"success",true},{"blockHash",hex(candidate.header.hash())},{"blockNumber",quantity(candidate.header.number)},{"gasUsed",quantity(candidate.header.gas_used)},{"transactionCount",candidate.transactions.size()},{"blockReward",quantity(ProofOfStake::block_reward)},{"gasFees",quantity(fees)},{"totalReward",quantity(Int(ProofOfStake::block_reward+fees))}};
  }catch(const std::exception& e){return Json{{"success",false},{"error",e.what()}};}
 }
 if(method=="validator_isSelected"){
  arity(p,1,1);auto addr=address(p[0].get<std::string>());auto selected=chain_.next_validator();return Json{{"selected",selected&&address(*selected)==addr},{"selectedValidator",selected?Json(*selected):Json(nullptr)},{"blockNumber",quantity(chain_.tip().header.number)},{"yourAddress",addr}};
 }
 if(method=="validator_getPendingTransactions"){
  arity(p,0,1);Int limit=p.empty()?chain_.tip().header.gas_limit:quantity(p[0]);Json result=Json::array();for(const auto& tx:chain_.pending_transactions(limit))result.push_back(tx.to_json());return result;
 }
 if(method=="validator_getSyncStatus"){
  arity(p,0,0);return network_status();
 }
 throw RpcError(-32601,"Method not found");
}
std::function<Json()> Rpc::prepare_simulation(const std::string& method,const Json& p,const std::function<void()>& cancelled){
  arity(p,1,2);if(!p[0].is_object())throw RpcError(-32602,"Expected call object");
  const auto& args=p[0];auto supplied=[&](const char* name){return args.contains(name)&&!args[name].is_null();};
  if(supplied("chainId")&&quantity(args["chainId"])!=chain_.genesis().chain_id)throw RpcError(-32602,"chainId does not match node");
  if(supplied("type")&&quantity(args["type"])>2)throw RpcError(-32602,"Transaction type unavailable on Nano network");
  if(supplied("gasPrice")&&(supplied("maxFeePerGas")||supplied("maxPriorityFeePerGas")))throw RpcError(-32602,"Both legacy and dynamic fees specified");
  if(supplied("maxPriorityFeePerGas")){
   Int cap=supplied("maxFeePerGas")?quantity(args["maxFeePerGas"]):Int(0);
   if(quantity(args["maxPriorityFeePerGas"])>cap)throw RpcError(-32602,"Priority fee exceeds maximum fee");
  }
  auto limits=simulation_limits_;auto deadline=limits.now()+limits.timeout;
  auto checkpoint=[deadline,now=limits.now,cancelled]{if(cancelled)cancelled();if(now()>=deadline)throw ExecutionCancelled("RPC simulation execution timeout");};
  checkpoint();
  std::optional<Block> b;std::optional<State> state;
  std::map<Int,Bytes> ancestors;std::function<std::pair<Header,State>()> pending;
  // Estimates without an explicit block target the candidate used for inclusion.
  // eth_call and explicit latest/historical requests retain their selected context.
  if((p.size()==1&&method=="eth_estimateGas")||(p.size()==2&&p[1]=="pending")){
   Int now(std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()));
   if(dev_mode_)now=development_timestamp();
   auto snapshot=chain_.pending_snapshot(coinbase_,now,checkpoint);b=Block{};b->header=std::move(snapshot.header);ancestors=std::move(snapshot.ancestors);pending=std::move(snapshot.execute);
  }else{
   b=block(p.size()==2?p[1]:Json("latest"));if(!b)throw RpcError(-32000,"Unknown block");state=chain_.store().state_at(b->header.number);if(!state)throw RpcError(-32000,"State unavailable");
  }
  checkpoint();
  // Capture the full BLOCKHASH window before releasing the RPC lock. A later
  // block import or development rewind must not change this simulation.
  if(!pending)for(Int n=std::max(Int(0),Int(b->header.number-256));n<b->header.number;++n){checkpoint();auto ancestor=chain_.store().block_by_number(n);if(!ancestor)throw RpcError(-32000,"Missing simulation ancestor");ancestors.emplace(n,ancestor->header.hash());}
  auto chain_id=chain_.genesis().chain_id;auto revision=chain_.genesis().evm_revision;
  return [method,p,b=std::move(b),state=std::move(state),ancestors=std::move(ancestors),chain_id,revision,checkpoint,pending=std::move(pending)]() mutable -> Json {
  checkpoint();
  if(pending){auto candidate=pending();b->header=std::move(candidate.first);state=std::move(candidate.second);}
  Int requested=quantity(p[0].value("gas",Json(quantity(b->header.gas_limit))));
  // Geth treats sub-intrinsic gas as an unspecified estimate hint. Calls and
  // signed admission still enforce their actual gas budget.
  Int limit=method=="eth_estimateGas"&&requested<21000?b->header.gas_limit:std::min(b->header.gas_limit,requested);
  if(method=="eth_estimateGas"){
   auto tx=simulation_transaction(p[0]);Int cap=0;
   // Omitted/zero fees are simulation requests, not an invented wallet fee budget.
   if(p[0].contains("maxFeePerGas")&&!p[0]["maxFeePerGas"].is_null())cap=quantity(p[0]["maxFeePerGas"]);
   else if(p[0].contains("gasPrice")&&!p[0]["gasPrice"].is_null())cap=quantity(p[0]["gasPrice"]);
   if(cap>0){
    auto sender=address(p[0].value("from",std::string("0x0000000000000000000000000000000000000000")));
    Int balance=static_cast<const State&>(*state).get(sender).balance;
    if(tx.value>=balance)throw RpcError(-32000,"Insufficient funds for gas and value");
    limit=std::min(limit,Int((balance-tx.value)/cap));
    if(limit<std::max(transaction_intrinsic_gas(tx,revision),transaction_floor_gas(tx,revision)))throw RpcError(-32000,"Gas required exceeds available funds or gas limit");
   }
  }
  uint64_t high=u64(limit);auto result=simulate(*state,b->header,chain_id,p[0],high,ancestors,revision,checkpoint);
  if(!result.success)throw RpcError(3,"Execution failed: "+result.error,hex(result.return_data));
  if(method=="eth_call")return hex(result.return_data);
  auto tx=simulation_transaction(p[0]);
  uint64_t low=u64(std::max(transaction_intrinsic_gas(tx,revision),transaction_floor_gas(tx,revision)))-1;
  // A measured budget is returned only after a successful verification.
  // Nested EIP-150 calls can need headroom above their actual consumption.
  if(result.gas_used>low&&result.gas_used<high){
   auto trial=simulate(*state,b->header,chain_id,p[0],result.gas_used,ancestors,revision,checkpoint);
   if(trial.success)return quantity(Int(std::to_string(result.gas_used)));
   low=result.gas_used;
  }
  // Try a headroom candidate, but never exceed the caller/balance/block cap.
  // Use arbitrary-precision arithmetic before converting back to uint64.
  Int optimistic=(Int(std::to_string(result.gas_used))+2300)*64/63;
  if(optimistic>Int(std::to_string(low))&&optimistic<Int(std::to_string(high))){
   auto gas=u64(optimistic);auto trial=simulate(*state,b->header,chain_id,p[0],gas,ancestors,revision,checkpoint);
   if(trial.success)high=gas;else low=gas;
  }
  // Stop with a verified successful upper bound once the search interval is
  // within 1.5% of the quote. An RPC estimate need not be the exact minimum.
  while(high-low>1){
   if(Int(std::to_string(high-low))*1000<=Int(std::to_string(high))*15)break;
   auto mid=low+(high-low)/2;auto trial=simulate(*state,b->header,chain_id,p[0],mid,ancestors,revision,checkpoint);
   if(trial.success)high=mid;else low=mid;
  }
  return quantity(Int(std::to_string(high)));
  };
}
Json Rpc::one(const Json& request,const std::function<void()>& cancelled){
 if(!request.is_object()||request.value("jsonrpc",Json())!="2.0"||!request.contains("method")||!request["method"].is_string())return error_response(nullptr,-32600,"Invalid Request");
 bool notification=!request.contains("id");Json id=request.value("id",Json(nullptr));if(!id.is_null()&&!id.is_string()&&!id.is_number_integer())return error_response(nullptr,-32600,"Invalid Request ID");
 Json response;
 try{auto params=request.value("params",Json::array());if(!params.is_array())throw RpcError(-32602,"Expected positional parameters");
  if(cancelled)cancelled();
  auto lock_started=rpc_timing::Clock::now();std::unique_lock lock(mutex_);rpc_timing::record("rpc_lock",lock_started,rpc_timing::request);auto method=request["method"].get<std::string>();Json result;
  if(method=="nano_altairReceive"||method=="nano_altairStatus"||method=="nano_altairSubmit"||method=="nano_altairJob"||method=="nano_altairCancel"||method=="nano_altairForget"||method=="nano_altairJobs"||method=="nano_altairRotateSession"){
   if(!altair_)throw RpcError(-32601,"Method not found");
   const auto count=method=="nano_altairStatus"||method=="nano_altairJobs"?1:method=="nano_altairReceive"||method=="nano_altairRotateSession"?2:method=="nano_altairSubmit"?4:3;arity(params,count,count);
   if(network_token_.empty()||!params[0].is_string()||params[0].get<std::string>()!=network_token_)throw RpcError(-32001,"Unauthorized Altair request");
   uint64_t sequence=0;
   if(count>=3){if(!params[1].is_string()||!params[2].is_string())throw RpcError(-32602,"Expected session and sequence");sequence=u64(quantity(params[2]));if(!sequence||quantity(Int(std::to_string(sequence)))!=params[2].get<std::string>())throw RpcError(-32602,"Invalid admission sequence");}
   auto service=altair_;lock.unlock();
   if(method=="nano_altairStatus")result=service->status();
   else if(method=="nano_altairJobs")result=service->jobs();
   else if(method=="nano_altairRotateSession")result=service->rotate(params[1].get<std::string>());
   else if(method=="nano_altairReceive")result=service->receive(params[1].get_ref<const std::string&>(),cancelled);
   else if(method=="nano_altairSubmit")result=service->submit(params[1].get<std::string>(),sequence,params[3].get_ref<const std::string&>());
   else result=service->job(params[1].get<std::string>(),sequence,method=="nano_altairCancel",method=="nano_altairForget");
  }
  else if(method=="eth_call"||method=="eth_estimateGas"){
   std::function<Json()> simulation;{rpc_timing::Span preparation("prepare");simulation=prepare_simulation(method,params,cancelled);}
   lock.unlock();rpc_timing::Span execution(method=="eth_call"?"call_execute":"estimate_execute");result=simulation();
  }
  else {
   Estimate estimate=[&](const Json& transaction){
    auto simulation=prepare_simulation("eth_estimateGas",Json::array({transaction}),cancelled);
    lock.unlock();Json gas;
    try{gas=simulation();}catch(...){lock.lock();throw;}
    lock.lock();if(cancelled)cancelled();return gas;
   };
   result=dispatch(method,params,estimate);
  }
  response={{"jsonrpc","2.0"},{"id",id},{"result",std::move(result)}};
 }
 catch(const ExecutionCancelled& e){response=error_response(id,-32002,e.what());}
 catch(const RpcError& e){response=error_response(id,e.code,e.what(),e.data);}
 catch(const Json::exception&){response=error_response(id,-32602,"Invalid params");}
 catch(const std::invalid_argument& e){response=error_response(id,-32602,e.what());}
 catch(const std::overflow_error& e){response=error_response(id,-32602,e.what());}
 catch(const std::exception&){response=error_response(id,-32603,"Internal error");}
 return notification?Json(nullptr):response;
}
Json Rpc::handle(const Json& request,const std::function<void()>& cancelled){if(!request.is_array())return one(request,cancelled);if(request.empty())return error_response(nullptr,-32600,"Invalid Request");Json result=Json::array();for(const auto& item:request){auto response=one(item,cancelled);if(!response.is_null())result.push_back(std::move(response));}return result.empty()?Json(nullptr):result;}
Json Rpc::parse(const std::string& request){auto value=Json::parse(request,nullptr,false);if(value.is_discarded())return error_response(nullptr,-32700,"Parse error");return handle(value);}
}
