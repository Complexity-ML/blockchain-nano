#include "nano/rpc.hpp"
namespace nano {
namespace {
Json error_response(const Json& id,int code,const std::string& message,const Json& data=nullptr){Json error={{"code",code},{"message",message}};if(!data.is_null())error["data"]=data;return {{"jsonrpc","2.0"},{"id",id},{"error",error}};}
void arity(const Json& p,size_t min,size_t max){if(p.size()<min||p.size()>max)throw RpcError(-32602,"Invalid parameter count");}
Bytes hash_arg(const Json& j){auto value=unhex(j.get<std::string>());if(value.size()!=32)throw RpcError(-32602,"Expected 32-byte hash");return value;}
Json tx_json(const SignedTransaction& tx,const Block* block=nullptr,size_t index=0){auto result=tx.to_json();result["blockHash"]=block?Json(hex(block->header.hash())):Json(nullptr);result["blockNumber"]=block?Json(quantity(block->header.number)):Json(nullptr);result["transactionIndex"]=block?Json(quantity(Int(index))):Json(nullptr);return result;}
Json block_json(const Block& b,bool full){auto result=b.to_json(full);if(full)for(size_t i=0;i<b.transactions.size();++i)result["transactions"][i]=tx_json(b.transactions[i],&b,i);return result;}
ExecutionResult simulate(const State& state,const Header& header,const Int& chain_id,const Json& call,uint64_t gas,const ChainStore& store,EvmRevision revision){
 auto copy=state;Transaction tx=Transaction::from_json(call);auto sender=address(call.value("from",std::string("0x0000000000000000000000000000000000000000")));
 Int intrinsic=tx.intrinsic_gas();if(Int(std::to_string(gas))<intrinsic){ExecutionResult out;out.error="intrinsic gas exceeds limit";return out;}
 auto destination=tx.to?address(*tx.to):create_address(sender,state.get(sender).nonce);
 if(!tx.to&&(state.get(destination).nonce!=0||!state.get(destination).code.empty()||(revision!=EvmRevision::NanoExperimental&&!state.get(destination).storage.empty()))){
  ExecutionResult out;out.error="contract address collision";out.gas_used=gas;return out;
 }
 if(!copy.transfer(sender,destination,tx.value))throw RpcError(-32000,"Insufficient funds for call value");
 ExecutionContext ctx;ctx.revision=revision;ctx.access_list=tx.access_list;ctx.origin=sender;ctx.caller=sender;ctx.address=destination;ctx.value=tx.value;ctx.gas=gas-u64(intrinsic);ctx.chain_id=chain_id;ctx.gas_price=tx.effective_gas_price(header.base_fee.value_or(Int(0)));ctx.coinbase=header.coinbase;ctx.timestamp=header.timestamp;ctx.number=header.number;ctx.prevrandao=header.difficulty;ctx.gas_limit=header.gas_limit;ctx.base_fee=header.base_fee.value_or(Int(0));ctx.block_hash=[&store](const Int& number){auto b=store.block_by_number(number);if(!b)throw std::runtime_error("missing ancestor block");return b->header.hash();};
 if(tx.to){ctx.data=tx.data;ctx.code=copy.get(destination).code;}else{ctx.code=tx.data;copy.get(destination).nonce=1;}
 EVM evm(copy);auto result=evm.execute(ctx);
 if(result.success&&!tx.to){uint64_t deposit=result.return_data.size()*200;if(result.return_data.size()>24576||deposit>ctx.gas-result.gas_used||(revision==EvmRevision::Paris&&!result.return_data.empty()&&result.return_data[0]==0xef)){result.success=false;result.error="contract code deposit out of gas";}else result.gas_used+=deposit;}
 result.gas_used+=u64(intrinsic);return result;
}
}
Json Rpc::network_block(const Block& block)const{
 auto result=block.to_json(true);result["uncleHeaders"]=Json::array();
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
 std::lock_guard lock(mutex_);auto now=std::chrono::steady_clock::now();
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
Json Rpc::dispatch(const std::string& method,const Json& p){
 if(method=="nano_bftExchange"){
  arity(p,0,1);if(!bft_)return nullptr;
  if(!p.empty()&&!p[0].is_null()){
   auto now=std::chrono::steady_clock::now();Int wall(std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()));
   bft_->receive(p[0],now,wall);
  }
  return bft_->snapshot();
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
 if(method=="eth_sign"||method=="personal_sign"||method=="eth_signTypedData"||method=="eth_signTypedData_v4"||method=="eth_signTransaction"||method=="eth_sendTransaction")return account_request(method,p);
 if(method=="eth_sendRawTransaction"){arity(p,1,1);auto tx=SignedTransaction::decode(unhex(p[0].get<std::string>()));if(!chain_.submit(tx))throw RpcError(-32000,"Transaction rejected by pool");return hex(tx.hash());}
 if(method=="txpool_content"||method=="txpool_status"){arity(p,0,0);return chain_.pool_info(method=="txpool_content");}
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
  Int estimate=quantity(dispatch("eth_estimateGas",Json::array({tx})));
  tx["gas"]=quantity(std::min(chain_.tip().header.gas_limit,Int(estimate+25000)));
  return account_request("eth_sendTransaction",Json::array({tx}));
 }
 if(method=="nno_getStake"){arity(p,1,1);auto addr=address(p[0].get<std::string>());try{return quantity(chain_.stake(addr));}catch(const std::exception&){throw RpcError(-32000,"Staking query failed");}}
 if(method=="nno_getValidators"){arity(p,0,0);return chain_.validators();}
 if(method=="eth_call"||method=="eth_estimateGas"){
  arity(p,1,2);if(!p[0].is_object())throw RpcError(-32602,"Expected call object");auto b=block(p.size()==2?p[1]:Json("latest"));if(!b)throw RpcError(-32000,"Unknown block");auto state=chain_.store().state_at(b->header.number);if(!state)throw RpcError(-32000,"State unavailable");
  Int limit=std::min(b->header.gas_limit,quantity(p[0].value("gas",Json(quantity(b->header.gas_limit)))));uint64_t high=u64(limit);auto result=simulate(*state,b->header,chain_.genesis().chain_id,p[0],high,chain_.store(),chain_.genesis().evm_revision);
  if(!result.success)throw RpcError(3,"Execution failed: "+result.error,hex(result.return_data));
  if(method=="eth_call")return hex(result.return_data);
  uint64_t low=u64(Transaction::from_json(p[0]).intrinsic_gas())-1;
  while(high-low>1){auto mid=low+(high-low)/2;auto trial=simulate(*state,b->header,chain_.genesis().chain_id,p[0],mid,chain_.store(),chain_.genesis().evm_revision);if(trial.success)high=mid;else low=mid;}
  return quantity(Int(std::to_string(high)));
 }
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
  arity(p,0,0);auto tip=chain_.tip();return Json{{"chain_id",u64(chain_.genesis().chain_id)},{"network_id",u64(chain_.genesis().chain_id)},{"best_block",u64(tip.header.number)},{"best_hash",hex(tip.header.hash())},{"genesis_hash",hex(chain_.genesis().block.header.hash())},{"cursor",chain_.event_cursor()},{"version","nano/1.0.0"}};
 }
 if(method=="nano_networkBlocks"){
  arity(p,1,1);Int start=quantity(p[0]),height=chain_.tip().header.number;Json blocks=Json::array();size_t size=0;
  for(Int number=start+1;number<=height&&number<=start+50;++number){auto b=chain_.store().block_by_number(number);if(!b)throw RpcError(-32000,"Missing block");auto json=network_block(*b);size+=json.dump().size();if(size>8*1024*1024)break;blocks.push_back(std::move(json));}
  return Json{{"blocks",blocks},{"total",u64(height)},{"start",u64(Int(start+1))}};
 }
 if(method=="nano_networkEvents"){
  arity(p,1,1);auto cursor=u64(quantity(p[0]));auto events=chain_.events_since(cursor);Json result=Json::array();size_t size=0;
  for(const auto& event:events){Json value;
   if(event.type=="block"){auto b=chain_.store().block_by_hash(unhex(event.value.at("hash")));if(b){value=network_block(*b);}}
   else{auto hash=unhex(event.value.get<std::string>());auto tx=chain_.pending_transaction(hash);if(tx)value=hex(tx->raw());else{auto stored=chain_.store().transaction(hash);if(stored){SignedTransaction mined{Transaction::from_json(*stored),quantity(stored->at("v")),quantity(stored->at("r")),quantity(stored->at("s"))};value=hex(mined.raw());}}}
   size+=value.dump().size();if(size>8*1024*1024||result.size()>=50)break;
   cursor=event.sequence;if(!value.is_null())result.push_back({{"type",event.type},{"value",std::move(value)}});
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
    if(!proof.is_object()||proof.size()!=3||!proof.at("version").is_number_integer()||proof.at("version")!=1||!proof.at("round").is_string()||proof.at("round").get_ref<const std::string&>().size()>18||!proof.at("votes").is_array()||proof.at("votes").empty()||proof.at("votes").size()>1000)throw std::invalid_argument("invalid BFT certificate envelope");
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
Json Rpc::one(const Json& request){
 if(!request.is_object()||request.value("jsonrpc",Json())!="2.0"||!request.contains("method")||!request["method"].is_string())return error_response(nullptr,-32600,"Invalid Request");
 bool notification=!request.contains("id");Json id=request.value("id",Json(nullptr));if(!id.is_null()&&!id.is_string()&&!id.is_number_integer())return error_response(nullptr,-32600,"Invalid Request ID");
 Json response;
 try{auto params=request.value("params",Json::array());if(!params.is_array())throw RpcError(-32602,"Expected positional parameters");response={{"jsonrpc","2.0"},{"id",id},{"result",dispatch(request["method"].get<std::string>(),params)}};}
 catch(const RpcError& e){response=error_response(id,e.code,e.what(),e.data);}
 catch(const Json::exception&){response=error_response(id,-32602,"Invalid params");}
 catch(const std::invalid_argument& e){response=error_response(id,-32602,e.what());}
 catch(const std::overflow_error& e){response=error_response(id,-32602,e.what());}
 catch(const std::exception&){response=error_response(id,-32603,"Internal error");}
 return notification?Json(nullptr):response;
}
Json Rpc::handle(const Json& request){std::lock_guard lock(mutex_);if(!request.is_array())return one(request);if(request.empty())return error_response(nullptr,-32600,"Invalid Request");Json result=Json::array();for(const auto& item:request){auto response=one(item);if(!response.is_null())result.push_back(std::move(response));}return result.empty()?Json(nullptr):result;}
Json Rpc::parse(const std::string& request){auto value=Json::parse(request,nullptr,false);if(value.is_discarded())return error_response(nullptr,-32700,"Parse error");return handle(value);}
}
