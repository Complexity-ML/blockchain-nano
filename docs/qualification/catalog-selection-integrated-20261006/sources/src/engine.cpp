#include "nano/engine.hpp"
namespace nano {
void validate_execution_block_size(size_t encoded_size,EvmRevision revision){
 if(revision>=EvmRevision::Osaka&&encoded_size>8388608)throw std::invalid_argument("execution block RLP size limit exceeded");
}
Int next_excess_blob_gas(const Header& parent,EvmRevision revision){
 Int excess=parent.excess_blob_gas.value_or(Int(0)),used=parent.blob_gas_used.value_or(Int(0));
 Int target=revision>=EvmRevision::Prague?786432:393216;
 if(excess+used<target)return 0;
 if(revision>=EvmRevision::Osaka&&8192*parent.base_fee.value_or(Int(0))>131072*blob_base_fee(excess,revision))return excess+used/3;
 return excess+used-target;
}
Int blob_base_fee(const Int& excess,EvmRevision revision){
 if(excess<0||excess>Int("18446744073709551615"))throw std::invalid_argument("invalid excess blob gas");
 // EIP-7691 changes the exponential denominator at Prague.
 const Int denominator=revision>=EvmRevision::Prague?5007716:3338477;Int term=denominator,total=0,i=1;
 while(term>0){total+=term;if(total>=(Int(1)<<256)*denominator)throw std::invalid_argument("blob base fee exceeds uint256");term=term*excess/(denominator*i);++i;}
 return Int(total/denominator);
}

// EIP-7623 reserves a calldata floor, applied after execution and refunds.
Int transaction_floor_gas(const Transaction& tx,EvmRevision revision){
 if(revision<EvmRevision::Prague)return 0;
 Int tokens=0;for(auto byte:tx.data)tokens+=byte==0?1:4;
 return 21000+10*tokens;
}

Int transaction_intrinsic_gas(const Transaction& tx,EvmRevision revision){
 if(revision>=EvmRevision::Shanghai&&!tx.to&&tx.data.size()>49152)throw std::invalid_argument("initcode size exceeds limit");
 Int gas=tx.intrinsic_gas();if(revision>=EvmRevision::Shanghai&&!tx.to)gas+=2*((tx.data.size()+31)/32);return gas;
}
Receipt ExecutionEngine::apply_transaction(State& state,const SignedTransaction& signed_tx,const Header& block,const Int& index,const Int& cumulative)const{
 if(checkpoint_)checkpoint_();
 const auto& tx=signed_tx.tx;if(tx.type==4&&revision_<EvmRevision::Prague)throw std::invalid_argument("transaction type 4 unavailable before Prague");if(tx.type>4)throw std::invalid_argument("unsupported transaction type");if(tx.type==3&&revision_<EvmRevision::Cancun)throw std::invalid_argument("transaction type 3 unavailable before Cancun");std::string sender=address(signed_tx.sender());const auto& account=static_cast<const State&>(state).get(sender);
 if(tx.chain_id!=chain_id_&&!(replay_policy_==ReplayPolicy::AllowLegacyUnprotected&&tx.type==0&&(signed_tx.v==27||signed_tx.v==28)))throw std::invalid_argument("transaction chain id mismatch");
 if(tx.type==4){if(!tx.to)throw std::invalid_argument("set-code transaction cannot create contract");if(tx.authorization_list.empty())throw std::invalid_argument("set-code transaction has no authorizations");for(const auto& a:tx.authorization_list)a.payload();}
 if(revision_==EvmRevision::Berlin&&tx.type>1)throw std::invalid_argument("transaction type unavailable in Berlin");
 if((revision_==EvmRevision::Paris||revision_>=EvmRevision::Shanghai)&&!account.code.empty()&&!(revision_>=EvmRevision::Prague&&delegation_target(account.code)))throw std::invalid_argument("transaction sender has deployed code");
 if(revision_>=EvmRevision::Osaka&&tx.gas>16777216)throw std::invalid_argument("transaction gas limit exceeds Osaka maximum");
 if(tx.nonce>=Int("18446744073709551615"))throw std::invalid_argument("transaction nonce at or above maximum");
 if(tx.nonce<account.nonce)throw std::invalid_argument("transaction nonce too low");
 if(tx.nonce>account.nonce)throw std::invalid_argument("transaction nonce too high");
 if(tx.value<0||tx.gas<0||index<0||cumulative<0)throw std::invalid_argument("negative transaction quantity");
 Int intrinsic=transaction_intrinsic_gas(tx,revision_);if(tx.gas<intrinsic)throw std::invalid_argument("intrinsic gas exceeds limit");
 Int floor=transaction_floor_gas(tx,revision_);if(tx.gas<floor)throw std::invalid_argument("transaction gas below calldata floor");
 if(cumulative+tx.gas>block.gas_limit)throw std::invalid_argument("block gas limit exceeded");
 Int base_fee=revision_==EvmRevision::Berlin?Int(0):block.base_fee.value_or(Int(0));Int cap=tx.type>=2?tx.max_fee:tx.gas_price;
 if(cap<base_fee)throw std::invalid_argument("invalid transaction fee cap");if(tx.type>=2&&tx.priority_fee>tx.max_fee)throw std::invalid_argument("priority fee exceeds fee cap");
 Int blob_price=revision_>=EvmRevision::Cancun?blob_base_fee(block.excess_blob_gas.value_or(Int(0)),revision_):Int(0),blob_gas=0;
 if(tx.type==3){
  if(!tx.to)throw std::invalid_argument("blob transaction cannot create contract");
  if(tx.blob_versioned_hashes.empty())throw std::invalid_argument("blob transaction has no blobs");
  if(revision_>=EvmRevision::Osaka&&tx.blob_versioned_hashes.size()>9)throw std::invalid_argument("blob transaction exceeds maximum blob gas allowance");
  if(revision_>=EvmRevision::Osaka&&tx.blob_versioned_hashes.size()>6)throw std::invalid_argument("blob transaction exceeds Osaka transaction blob limit");
  if(tx.blob_versioned_hashes.size()>(revision_>=EvmRevision::Prague?9u:6u))throw std::invalid_argument("blob transaction exceeds block blob limit");
  for(const auto& hash:tx.blob_versioned_hashes)if(hash.size()!=32||hash[0]!=1)throw std::invalid_argument("invalid blob versioned hash");
  if(tx.max_fee_per_blob_gas<blob_price)throw std::invalid_argument("blob fee cap below base fee");
  blob_gas=Int(tx.blob_versioned_hashes.size())*131072;
 }
 Int price=tx.effective_gas_price(base_fee);if(account.balance<tx.value+tx.gas*cap+blob_gas*tx.max_fee_per_blob_gas)throw std::invalid_argument("insufficient transaction funds");
 uint64_t budget=u64(Int(tx.gas-intrinsic));address(block.coinbase);
 size_t transaction_snap=state.snapshot();
 try{
 state.get(sender).balance-=tx.gas*price+blob_gas*blob_price;state.get(sender).nonce+=1;
 Int authorization_refund=0;std::set<std::string> authorization_warm;
 if(tx.type==4)for(const auto& auth:tx.authorization_list){
  if((auth.chain_id!=0&&auth.chain_id!=chain_id_)||auth.nonce>=Int("18446744073709551615"))continue;
  std::string authority;try{authority=address(auth.authority());}catch(const std::invalid_argument&){continue;}
  authorization_warm.insert(authority);
  const auto& current=static_cast<const State&>(state).get(authority);
  if((!current.code.empty()&&!delegation_target(current.code))||current.nonce!=auth.nonce)continue;
  if(!current.empty())authorization_refund+=12500;
  auto& account=state.get(authority);auto target=unhex(address(auth.target));
  account.code=integer(target)==0?Bytes{}:concat(Bytes{0xef,1,0},target);account.nonce+=1;
 }
 size_t execution_snap=state.snapshot();
 std::string destination=tx.to?address(*tx.to):create_address(sender,tx.nonce);
 ExecutionResult result;
 if(!tx.to&&(state.get(destination).nonce!=0||!state.get(destination).code.empty()||(revision_!=EvmRevision::NanoExperimental&&!state.get(destination).storage.empty()))){result.error="contract address collision";result.gas_used=budget;}
 else{
 if(!tx.to)state.get(destination).nonce=1;
 if(!state.transfer(sender,destination,tx.value))throw std::logic_error("validated transfer failed");
 ExecutionContext ctx;ctx.checkpoint=checkpoint_;ctx.revision=revision_;ctx.access_list=tx.access_list;ctx.origin=sender;ctx.caller=sender;ctx.address=destination;ctx.gas_price=price;ctx.value=tx.value;ctx.gas=budget;ctx.data=tx.to?tx.data:Bytes{};ctx.code=tx.to?state.get(destination).code:tx.data;ctx.coinbase=block.coinbase;ctx.timestamp=block.timestamp;ctx.number=block.number;ctx.prevrandao=block_randomness(block,revision_);ctx.gas_limit=block.gas_limit;ctx.chain_id=chain_id_;ctx.base_fee=base_fee;ctx.block_hash=block_hash_;ctx.contract_creation=!tx.to;ctx.blob_base_fee=blob_price;ctx.blob_hashes=tx.blob_versioned_hashes;
 ctx.authorization_warm_addresses=authorization_warm;
 if(revision_>=EvmRevision::Prague&&tx.to)if(auto target=delegation_target(ctx.code)){
  ctx.authorization_warm_addresses.insert(*target);Int number=integer(unhex(*target));
  ctx.code=is_precompile(number,revision_)?Bytes{}:static_cast<const State&>(state).get(*target).code;
 }
 EVM evm(state);result=evm.execute(ctx);
 if(result.success&&!tx.to){uint64_t deposit=result.return_data.size()*200;if(result.return_data.size()>24576||deposit>budget-result.gas_used||((revision_==EvmRevision::Paris||revision_>=EvmRevision::Shanghai)&&!result.return_data.empty()&&result.return_data[0]==0xef)){result.success=false;result.error="contract code deposit out of gas";result.gas_used=budget;}else{result.gas_used+=deposit;state.get(destination).code=result.return_data;}}
 }
 if(result.success)state.commit(execution_snap);else{state.revert(execution_snap);result.logs.clear();result.gas_refund=0;}
 result.gas_refund+=authorization_refund;
 Int consumed=intrinsic+Int(std::to_string(result.gas_used));Int refund=std::min(result.gas_refund,Int(consumed/(revision_==EvmRevision::Berlin?2:5)));consumed-=std::max(Int(0),refund);consumed=std::max(consumed,floor);
 if(consumed>tx.gas)throw std::logic_error("gas accounting overflow");
 state.get(sender).balance+=(tx.gas-consumed)*price;
 // Explicit Paris and Shanghai rules burn base fees.
 state.get(block.coinbase).balance+=consumed*((revision_==EvmRevision::Paris||revision_>=EvmRevision::Shanghai)?Int(price-base_fee):price);
 if(result.success)for(const auto& account:result.deleted_accounts)state.erase(account);
 Receipt receipt;receipt.type=tx.type;receipt.success=result.success;receipt.gas_used=consumed;receipt.cumulative_gas_used=cumulative+consumed;receipt.block_number=block.number;receipt.index=index;receipt.transaction_hash=signed_tx.hash();receipt.block_hash=block.hash();receipt.sender=sender;receipt.to=tx.to;if(!tx.to&&result.success)receipt.contract_address=destination;receipt.logs=std::move(result.logs);
 if(checkpoint_)checkpoint_();
 state.commit(transaction_snap);return receipt;
 }catch(...){state.revert(transaction_snap);throw;}
}
namespace {
Int block_blob_gas(const Block& block,EvmRevision revision){
 Int total=0;for(const auto& signed_tx:block.transactions){const auto& tx=signed_tx.tx;if(tx.type!=3)continue;auto count=tx.blob_versioned_hashes.size();
  if(revision>=EvmRevision::Osaka&&count>9)throw std::invalid_argument("blob transaction exceeds maximum blob gas allowance");
  if(revision>=EvmRevision::Osaka&&count>6)throw std::invalid_argument("blob transaction exceeds Osaka transaction blob limit");
  if(count>(revision>=EvmRevision::Prague?9u:6u))throw std::invalid_argument("blob transaction exceeds block blob limit");
  total+=Int(count)*131072;
 }
 if(total>Int(revision>=EvmRevision::Prague?1179648:786432))throw std::invalid_argument("block blob gas limit exceeded");return total;
}
}
std::vector<Receipt> ExecutionEngine::execute_block(State& state,Block& block,bool reward)const{
 if(checkpoint_)checkpoint_();
 size_t snapshot=state.snapshot();Block candidate=block;std::vector<Receipt> receipts;
 try{
 if(!candidate.withdrawals.is_array())throw std::invalid_argument("withdrawals must be an array");
 if(consensus_input_policy_==ConsensusInputPolicy::RejectExternalInputs&&!candidate.withdrawals.empty())throw std::invalid_argument("withdrawals not authorized by Nano consensus");
 if(consensus_input_policy_==ConsensusInputPolicy::RejectExternalInputs&&candidate.header.parent_beacon_block_root)throw std::invalid_argument("beacon root not authorized by Nano consensus");
 if(consensus_input_policy_==ConsensusInputPolicy::RejectExternalInputs&&candidate.header.requests_hash)throw std::invalid_argument("execution requests not authorized by Nano consensus");
 if(revision_<EvmRevision::Prague&&candidate.header.requests_hash)throw std::invalid_argument("execution requests before Prague");
 if(revision_<EvmRevision::Cancun&&candidate.header.parent_beacon_block_root)throw std::invalid_argument("beacon root before Cancun");
 if(consensus_input_policy_==ConsensusInputPolicy::EthereumValidated&&revision_>=EvmRevision::Cancun&&(!candidate.header.parent_beacon_block_root||candidate.header.parent_beacon_block_root->size()!=32))throw std::invalid_argument("missing Cancun parent beacon block root");
 if(revision_<EvmRevision::Shanghai&&(!candidate.withdrawals.empty()||candidate.header.withdrawals_root))throw std::invalid_argument("withdrawals before Shanghai");
 if(consensus_input_policy_==ConsensusInputPolicy::EthereumValidated&&revision_>=EvmRevision::Shanghai)candidate.header.withdrawals_root=withdrawals_root(candidate.withdrawals);
 if(revision_<EvmRevision::Cancun&&(candidate.header.blob_gas_used||candidate.header.excess_blob_gas))throw std::invalid_argument("blob header fields before Cancun");if(revision_>=EvmRevision::Cancun){Int total_blob_gas=block_blob_gas(candidate,revision_);if(!candidate.header.excess_blob_gas||!candidate.header.withdrawals_root)throw std::invalid_argument("missing Cancun header fields");candidate.header.blob_gas_used=total_blob_gas;}if(consensus_input_policy_==ConsensusInputPolicy::EthereumValidated&&revision_>=EvmRevision::Cancun&&candidate.header.number!=0){
  (void)system_call(state,candidate.header,"0x000f3df6d732807ef1319fb7b8bb8522d0beac02",*candidate.header.parent_beacon_block_root);
 }
 if(consensus_input_policy_==ConsensusInputPolicy::EthereumValidated&&revision_>=EvmRevision::Prague&&candidate.header.number!=0)
  (void)system_call(state,candidate.header,"0x0000f90827f1c53a10cb7a02335b175320002935",candidate.header.parent_hash);

 Int cumulative=0;std::vector<Log> logs;for(size_t i=0;i<candidate.transactions.size();++i){auto receipt=apply_transaction(state,candidate.transactions[i],candidate.header,Int(i),cumulative);cumulative=receipt.cumulative_gas_used;logs.insert(logs.end(),receipt.logs.begin(),receipt.logs.end());receipts.push_back(std::move(receipt));}
 if(reward){Int amount("2000000000000000000");state.get(candidate.header.coinbase).balance+=amount;for(const auto& uncle:candidate.uncles){Int depth=candidate.header.number-uncle.number;if(depth<1||depth>6)throw std::invalid_argument("invalid uncle depth");state.get(uncle.coinbase).balance+=amount*(8-depth)/8;state.get(candidate.header.coinbase).balance+=amount/32;}}
 if(consensus_input_policy_==ConsensusInputPolicy::EthereumValidated)for(const auto& withdrawal:candidate.withdrawals){auto fields=withdrawal_payload(withdrawal);Int amount=fields[3].number();if(amount!=0)state.get(hex(fields[2].blob())).balance+=amount*1000000000;}
 if(consensus_input_policy_==ConsensusInputPolicy::EthereumValidated&&revision_>=EvmRevision::Prague){
  std::vector<Bytes> requests{deposit_requests(logs)};
  if(candidate.header.number!=0){
   for(const auto& [type,target]:std::vector<std::pair<uint8_t,std::string>>{{1,"0x00000961ef480eb55e80d19ad83579a64c007002"},{2,"0x0000bbddc7ce488642fb579f8b00f3a590007251"}}){
    auto data=system_call(state,candidate.header,target,{},true);Bytes request{type};request.insert(request.end(),data.begin(),data.end());requests.push_back(std::move(request));
   }
  }
  candidate.header.requests_hash=execution_requests_hash(requests);
 }
 candidate.header.gas_used=cumulative;candidate.header.state_root=state.root();candidate.header.transactions_root=transactions_root(candidate.transactions);candidate.header.receipts_root=receipts_root(receipts);candidate.header.logs_bloom=logs_bloom(logs);std::vector<Rlp> uncles;for(const auto& h:candidate.uncles)uncles.push_back(Rlp::decode(h.encode()));candidate.header.uncle_hash=keccak(Rlp(uncles).encode());if(revision_>=EvmRevision::Osaka)validate_execution_block_size(execution_rlp_size(candidate),revision_);auto final_hash=candidate.header.hash();for(auto& r:receipts)r.block_hash=final_hash;if(checkpoint_)checkpoint_();block=std::move(candidate);state.commit(snapshot);return receipts;
 }catch(...){state.revert(snapshot);throw;}
}
std::vector<Receipt> ExecutionEngine::verify_block(State& state,const Block& advertised,bool reward)const{
 size_t snapshot=state.snapshot();try{
 if(revision_>=EvmRevision::Osaka)validate_execution_block_size(execution_rlp_size(advertised),revision_);
 if(consensus_input_policy_==ConsensusInputPolicy::EthereumValidated&&revision_>=EvmRevision::Shanghai){if(!advertised.header.withdrawals_root||*advertised.header.withdrawals_root!=withdrawals_root(advertised.withdrawals))throw std::invalid_argument("withdrawals root mismatch");}
 if(revision_>=EvmRevision::Cancun){auto total=block_blob_gas(advertised,revision_);if(!advertised.header.blob_gas_used||*advertised.header.blob_gas_used!=total)throw std::invalid_argument("incorrect blob gas used");}
 Block replay=advertised;auto receipts=execute_block(state,replay,reward);if(replay.header.requests_hash!=advertised.header.requests_hash)throw std::invalid_argument("execution requests hash mismatch");if(replay.header.hash()!=advertised.header.hash())throw std::invalid_argument("block execution commitments mismatch");state.commit(snapshot);return receipts;}catch(...){state.revert(snapshot);throw;}
}
std::vector<Receipt> ExecutionEngine::verify_block(State& state,const Block& block,const Header& parent,bool reward)const{
 validate_execution_header(block.header,parent);
 if(revision_>=EvmRevision::Cancun){
  Int expected=next_excess_blob_gas(parent,revision_);
  if(!block.header.excess_blob_gas||*block.header.excess_blob_gas!=expected)throw std::invalid_argument("incorrect excess blob gas");
 }
 return verify_block(state,block,reward);
}

}
