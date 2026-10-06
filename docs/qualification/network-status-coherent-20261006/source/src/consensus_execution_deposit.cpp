#include "nano/consensus_execution_deposit.hpp"
#include <algorithm>
namespace nano {
ExecutionDepositBatch replay_execution_deposits(const ExecutionEngine& engine,
 const State& parent_state,const Header& parent,const Block& block,
 const Bytes& expected_block_hash,const Bytes& pinned_contract_code_hash,
 uint64_t next_index,size_t maximum_deposits,const std::function<void()>& checkpoint){
 auto check=[&]{if(checkpoint)checkpoint();};check();
 if(expected_block_hash.size()!=32||block.header.hash()!=expected_block_hash)
  throw std::invalid_argument("unauthenticated deposit execution block");
 if(parent_state.root()!=parent.state_root)throw std::invalid_argument("deposit parent state mismatch");
 auto check_code=[&](const State& state){const auto& code=state.get(ethereum_deposit_contract).code;
  if(pinned_contract_code_hash.size()!=32||code.empty()||keccak(code)!=pinned_contract_code_hash)
   throw std::invalid_argument("deposit contract code mismatch");};
 check_code(parent_state);
 // The canonical contract admits at most 2**32 - 1 deposits.
 constexpr uint64_t limit=(uint64_t(1)<<32)-1;
 if(next_index>limit)throw std::invalid_argument("deposit index outside contract tree");
 ExecutionDepositBatch result{parent_state,{}, {},next_index};
 result.receipts=engine.with_checkpoint(checkpoint).verify_block(result.state,block,parent,false);
 check_code(result.state);
 for(const auto& receipt:result.receipts){check();for(const auto& log:receipt.logs){
  check();const auto encoded=deposit_requests({log});if(encoded.size()==1)continue;
  if(!receipt.success||encoded.size()!=193)throw std::invalid_argument("invalid executed deposit event");
  auto number=[&](size_t at){uint64_t value=0;for(unsigned i=0;i<8;++i)value|=uint64_t(encoded[at+i])<<(8*i);return value;};
  if(number(185)!=result.next_deposit_index||result.next_deposit_index==limit)
   throw std::invalid_argument("nonconsecutive execution deposit index");
  if(result.deposits.size()>=maximum_deposits)throw std::length_error("execution deposit batch capacity");
  Phase0Deposit deposit{};
  std::copy_n(encoded.begin()+1,48,deposit.message.public_key.begin());
  std::copy_n(encoded.begin()+49,32,deposit.message.withdrawal_credentials.begin());
  deposit.message.amount_gwei=number(81);
  std::copy_n(encoded.begin()+89,96,deposit.signature.begin());
  result.deposits.push_back(deposit);++result.next_deposit_index;
 }}
 check();return result;
}
}
