#include "nano/engine.hpp"
namespace nano {
Receipt ExecutionEngine::apply_transaction(State& state,const SignedTransaction& signed_tx,const Header& block,const Int& index,const Int& cumulative)const{
 const auto& tx=signed_tx.tx;std::string sender=address(signed_tx.sender());const auto& account=static_cast<const State&>(state).get(sender);
 if(tx.chain_id!=chain_id_&&!(replay_policy_==ReplayPolicy::AllowLegacyUnprotected&&tx.type==0&&(signed_tx.v==27||signed_tx.v==28)))throw std::invalid_argument("transaction chain id mismatch");
 if(revision_==EvmRevision::Berlin&&tx.type>1)throw std::invalid_argument("transaction type unavailable in Berlin");
 if(revision_==EvmRevision::Paris&&!account.code.empty())throw std::invalid_argument("transaction sender has deployed code");
 if(tx.nonce!=account.nonce)throw std::invalid_argument("invalid transaction nonce");
 if(tx.value<0||tx.gas<0||index<0||cumulative<0)throw std::invalid_argument("negative transaction quantity");
 Int intrinsic=tx.intrinsic_gas();if(tx.gas<intrinsic)throw std::invalid_argument("intrinsic gas exceeds limit");
 if(cumulative+tx.gas>block.gas_limit)throw std::invalid_argument("block gas limit exceeded");
 Int base_fee=revision_==EvmRevision::Berlin?Int(0):block.base_fee.value_or(Int(0));Int cap=tx.type==2?tx.max_fee:tx.gas_price;
 if(cap<base_fee||(tx.type==2&&tx.priority_fee>tx.max_fee))throw std::invalid_argument("invalid transaction fee cap");
 Int price=tx.effective_gas_price(base_fee);if(account.balance<tx.value+tx.gas*cap)throw std::invalid_argument("insufficient transaction funds");
 uint64_t budget=u64(Int(tx.gas-intrinsic));address(block.coinbase);
 size_t transaction_snap=state.snapshot();
 try{
 state.get(sender).balance-=tx.gas*price;state.get(sender).nonce+=1;
 size_t execution_snap=state.snapshot();
 std::string destination=tx.to?address(*tx.to):create_address(sender,tx.nonce);
 ExecutionResult result;
 if(!tx.to&&(state.get(destination).nonce!=0||!state.get(destination).code.empty()||(revision_!=EvmRevision::NanoExperimental&&!state.get(destination).storage.empty()))){result.error="contract address collision";result.gas_used=budget;}
 else{
 if(!tx.to)state.get(destination).nonce=1;
 if(!state.transfer(sender,destination,tx.value))throw std::logic_error("validated transfer failed");
 ExecutionContext ctx;ctx.revision=revision_;ctx.access_list=tx.access_list;ctx.origin=sender;ctx.caller=sender;ctx.address=destination;ctx.gas_price=price;ctx.value=tx.value;ctx.gas=budget;ctx.data=tx.to?tx.data:Bytes{};ctx.code=tx.to?state.get(destination).code:tx.data;ctx.coinbase=block.coinbase;ctx.timestamp=block.timestamp;ctx.number=block.number;ctx.prevrandao=block.difficulty;ctx.gas_limit=block.gas_limit;ctx.chain_id=chain_id_;ctx.base_fee=base_fee;ctx.block_hash=block_hash_;
 EVM evm(state);result=evm.execute(ctx);
 if(result.success&&!tx.to){uint64_t deposit=result.return_data.size()*200;if(result.return_data.size()>24576||deposit>budget-result.gas_used||(revision_==EvmRevision::Paris&&!result.return_data.empty()&&result.return_data[0]==0xef)){result.success=false;result.error="contract code deposit out of gas";result.gas_used=budget;}else{result.gas_used+=deposit;state.get(destination).code=result.return_data;}}
 }
 if(result.success)state.commit(execution_snap);else{state.revert(execution_snap);result.logs.clear();result.gas_refund=0;}
 Int consumed=intrinsic+Int(std::to_string(result.gas_used));Int refund=std::min(result.gas_refund,Int(consumed/(revision_==EvmRevision::Berlin?2:5)));consumed-=std::max(Int(0),refund);
 if(consumed>tx.gas)throw std::logic_error("gas accounting overflow");
 state.get(sender).balance+=(tx.gas-consumed)*price;
 // Only the explicitly selected Paris rules burn base fees.
 state.get(block.coinbase).balance+=consumed*(revision_==EvmRevision::Paris?Int(price-base_fee):price);
 if(result.success)for(const auto& account:result.deleted_accounts)state.erase(account);
 Receipt receipt;receipt.type=tx.type;receipt.success=result.success;receipt.gas_used=consumed;receipt.cumulative_gas_used=cumulative+consumed;receipt.block_number=block.number;receipt.index=index;receipt.transaction_hash=signed_tx.hash();receipt.block_hash=block.hash();receipt.sender=sender;receipt.to=tx.to;if(!tx.to&&result.success)receipt.contract_address=destination;receipt.logs=std::move(result.logs);
 state.commit(transaction_snap);return receipt;
 }catch(...){state.revert(transaction_snap);throw;}
}
std::vector<Receipt> ExecutionEngine::execute_block(State& state,Block& block,bool reward)const{
 size_t snapshot=state.snapshot();Block candidate=block;std::vector<Receipt> receipts;
 try{Int cumulative=0;std::vector<Log> logs;for(size_t i=0;i<candidate.transactions.size();++i){auto receipt=apply_transaction(state,candidate.transactions[i],candidate.header,Int(i),cumulative);cumulative=receipt.cumulative_gas_used;logs.insert(logs.end(),receipt.logs.begin(),receipt.logs.end());receipts.push_back(std::move(receipt));}
 if(reward){Int amount("2000000000000000000");state.get(candidate.header.coinbase).balance+=amount;for(const auto& uncle:candidate.uncles){Int depth=candidate.header.number-uncle.number;if(depth<1||depth>6)throw std::invalid_argument("invalid uncle depth");state.get(uncle.coinbase).balance+=amount*(8-depth)/8;state.get(candidate.header.coinbase).balance+=amount/32;}}
 candidate.header.gas_used=cumulative;candidate.header.state_root=state.root();candidate.header.transactions_root=transactions_root(candidate.transactions);candidate.header.receipts_root=receipts_root(receipts);candidate.header.logs_bloom=logs_bloom(logs);std::vector<Rlp> uncles;for(const auto& h:candidate.uncles)uncles.push_back(Rlp::decode(h.encode()));candidate.header.uncle_hash=keccak(Rlp(uncles).encode());auto final_hash=candidate.header.hash();for(auto& r:receipts)r.block_hash=final_hash;block=std::move(candidate);state.commit(snapshot);return receipts;
 }catch(...){state.revert(snapshot);throw;}
}
std::vector<Receipt> ExecutionEngine::verify_block(State& state,const Block& advertised,bool reward)const{
 size_t snapshot=state.snapshot();try{Block replay=advertised;auto receipts=execute_block(state,replay,reward);if(replay.header.hash()!=advertised.header.hash())throw std::invalid_argument("block execution commitments mismatch");state.commit(snapshot);return receipts;}catch(...){state.revert(snapshot);throw;}
}
}
