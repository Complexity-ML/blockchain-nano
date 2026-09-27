#include "nano/consensus_execution_bridge.hpp"
#include <exception>
namespace nano {namespace {
template<class T> Bytes vector_bytes(const T& value){return {value.begin(),value.end()};}
Int uint64_int(uint64_t value){return Int(std::to_string(value));}
}
static BellatrixExecutionCandidate prepare_execution(
 const BellatrixExecutionPayload& p,const std::vector<CapellaWithdrawal>* withdrawals,
 const Header& parent,const State& state,uint64_t timestamp,const ConsensusRoot& randao,
 const Int& chain_id,ReplayPolicy replay_policy,
 const std::function<Bytes(const Int&)>& history,const std::function<void()>& check){
 if(check)check();
 if(!history)throw std::invalid_argument("authenticated execution history required");
 // Continuation within the selected post-merge fork; activation is separate.
 if(parent.difficulty!=0||!parent.base_fee||(bool(parent.withdrawals_root)!=bool(withdrawals))||parent.blob_gas_used||
    parent.excess_blob_gas||parent.parent_beacon_block_root||parent.requests_hash)
  throw std::invalid_argument("parent does not match post-merge execution profile");
 if(vector_bytes(p.parent_hash)!=parent.hash()||uint64_int(p.block_number)!=parent.number+1)
  throw std::invalid_argument("execution parent or block number mismatch");
 if(p.timestamp!=timestamp||uint64_int(p.timestamp)<=parent.timestamp||p.prev_randao!=randao)
  throw std::invalid_argument("execution timestamp or RANDAO mismatch");
 if(p.gas_limit>uint64_t(INT64_MAX))throw std::invalid_argument("execution gas limit exceeds int64");
 Block block;auto& h=block.header;
 h.parent_hash=vector_bytes(p.parent_hash);h.coinbase=hex(vector_bytes(p.fee_recipient));
 h.state_root=vector_bytes(p.state_root);h.receipts_root=vector_bytes(p.receipts_root);
 h.logs_bloom=vector_bytes(p.logs_bloom);h.mix_hash=vector_bytes(p.prev_randao);
 h.number=uint64_int(p.block_number);h.timestamp=uint64_int(p.timestamp);
 h.gas_limit=uint64_int(p.gas_limit);h.gas_used=uint64_int(p.gas_used);
 h.extra_data=p.extra_data;h.difficulty=0;
 h.base_fee=integer(Bytes(p.base_fee_per_gas.rbegin(),p.base_fee_per_gas.rend()));
 for(const auto& raw:p.transactions){
  if(check)check();
  if(raw.empty()||(raw[0]<0x80&&raw[0]!=1&&raw[0]!=2))
   throw std::invalid_argument("transaction envelope unavailable in Bellatrix");
  auto tx=SignedTransaction::decode(raw);
  if(tx.tx.type>2||tx.raw()!=raw)throw std::invalid_argument("noncanonical Bellatrix transaction");
  block.transactions.push_back(std::move(tx));
 }
 if(withdrawals){for(const auto& w:*withdrawals){if(check)check();block.withdrawals.push_back(w.to_json());}h.withdrawals_root=withdrawals_root(block.withdrawals);}
 h.transactions_root=transactions_root(block.transactions);
 if(h.hash()!=vector_bytes(p.block_hash))throw std::invalid_argument("execution block hash mismatch");
 validate_execution_header(h,parent);
 if(check)check();
 if(state.root()!=parent.state_root)throw std::invalid_argument("execution parent state mismatch");
 BellatrixExecutionCandidate candidate{std::move(block),state,{}};
 // Some EVM failure paths translate host exceptions into failed calls. Neither
 // cancellation nor unavailable/corrupt history may become a valid EVM REVERT.
 std::exception_ptr interrupted;
 auto execution_checkpoint=[&]{
  if(interrupted)std::rethrow_exception(interrupted);
  try{if(check)check();}catch(...){interrupted=std::current_exception();throw;}
 };
 auto execution_history=[&](const Int& number){
  if(interrupted)std::rethrow_exception(interrupted);
  try{auto hash=history(number);if(hash.size()!=32)throw std::invalid_argument("execution history hash width");return hash;}
  catch(...){interrupted=std::current_exception();throw;}
 };
 auto engine=ExecutionEngine(chain_id,execution_history,withdrawals?EvmRevision::Shanghai:EvmRevision::Paris,replay_policy,withdrawals?ConsensusInputPolicy::EthereumValidated:ConsensusInputPolicy::RejectExternalInputs).with_checkpoint(execution_checkpoint);
 try{candidate.receipts=engine.verify_block(candidate.state,candidate.block,parent,false);}
 catch(...){if(interrupted)std::rethrow_exception(interrupted);throw;}
 if(interrupted)std::rethrow_exception(interrupted);
 if(check)check();
 return candidate;
}
BellatrixExecutionCandidate prepare_bellatrix_execution(
 std::span<const uint8_t> wire,const ExecutionPayloadLimits& limits,size_t budget,
 const Header& parent,const State& state,uint64_t timestamp,const ConsensusRoot& randao,
 const Int& chain_id,ReplayPolicy replay,const std::function<Bytes(const Int&)>& history,const std::function<void()>& check){
 auto p=decode_bellatrix_execution_payload(wire,limits,budget,check);
 if(!p)throw std::invalid_argument("invalid Bellatrix payload SSZ or budget");
 return prepare_execution(*p,nullptr,parent,state,timestamp,randao,chain_id,replay,history,check);
}
BellatrixExecutionCandidate prepare_capella_execution(
 std::span<const uint8_t> wire,const ExecutionPayloadLimits& limits,uint64_t maximum_withdrawals,size_t budget,
 const Header& parent,const State& state,uint64_t timestamp,const ConsensusRoot& randao,
 const Int& chain_id,ReplayPolicy replay,const std::function<Bytes(const Int&)>& history,const std::function<void()>& check){
 auto p=decode_capella_execution_payload(wire,limits,maximum_withdrawals,budget,check);
 if(!p)throw std::invalid_argument("invalid Capella payload SSZ or budget");
 return prepare_execution(*p,&p->withdrawals,parent,state,timestamp,randao,chain_id,replay,history,check);
}

}
