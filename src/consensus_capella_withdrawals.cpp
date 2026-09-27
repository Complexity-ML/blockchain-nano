#include "nano/consensus_capella_withdrawals.hpp"
namespace nano {
namespace {
void validate(const CapellaWithdrawalState& s,const CapellaWithdrawalRules& r){
 const auto n=s.registry.validators.size();
 if(!r.maximum_effective_balance||!r.maximum_per_payload||!r.maximum_validators_per_sweep||!n||s.next_validator_index>=n||s.registry.validators.begin()->first!=0||s.registry.validators.rbegin()->first!=n-1)
  throw std::invalid_argument("invalid Capella withdrawal state or rules");
}
uint64_t advance(uint64_t index,uint64_t amount,uint64_t size){amount%=size;return index>=size-amount?index-(size-amount):index+amount;}
std::string q(uint64_t n){return quantity(Int(std::to_string(n)));}
}
Json CapellaWithdrawal::to_json()const{return {{"index",q(index)},{"validatorIndex",q(validator_index)},{"address",hex(Bytes(address.begin(),address.end()))},{"amount",q(amount_gwei)}};}
std::vector<CapellaWithdrawal> expected_capella_withdrawals(const CapellaWithdrawalState& state,const CapellaWithdrawalRules& rules,const std::function<void()>& check){
 if(check)check();validate(state,rules);const auto count=uint64_t(state.registry.validators.size());
 auto cursor=state.next_validator_index,index=state.next_withdrawal_index;std::vector<CapellaWithdrawal> out;
 const auto limit=std::min(count,rules.maximum_validators_per_sweep);
 for(uint64_t visited=0;visited<limit&&out.size()<rules.maximum_per_payload;++visited){
  if(check)check();const auto& v=state.registry.validators.at(cursor);uint64_t amount=0;
  if(v.withdrawal_credentials[0]==1){
   if(v.withdrawable_epoch<=state.registry.epoch&&v.balance)amount=v.balance;
   else if(v.effective_balance==rules.maximum_effective_balance&&v.balance>rules.maximum_effective_balance)amount=v.balance-rules.maximum_effective_balance;
  }
  if(amount){
   if(index==UINT64_MAX)throw std::overflow_error("Capella withdrawal index overflow");
   CapellaWithdrawal withdrawal{index++,cursor,{},amount};std::copy_n(v.withdrawal_credentials.begin()+12,20,withdrawal.address.begin());out.push_back(withdrawal);
  }
  cursor=advance(cursor,1,count);
 }
 return out;
}
std::optional<CapellaWithdrawalState> prepare_capella_withdrawals(const CapellaWithdrawalState& state,const CapellaWithdrawalRules& rules,const std::vector<CapellaWithdrawal>& advertised,const std::function<void()>& check){
 const auto expected=expected_capella_withdrawals(state,rules,check);if(expected!=advertised)return std::nullopt;
 if(check)check();auto next=state;
 for(const auto& withdrawal:expected){if(check)check();next.registry.validators.at(withdrawal.validator_index).balance-=withdrawal.amount_gwei;}
 if(!expected.empty())next.next_withdrawal_index=expected.back().index+1;
 next.next_validator_index=expected.size()==rules.maximum_per_payload?advance(expected.back().validator_index,1,state.registry.validators.size()):advance(state.next_validator_index,rules.maximum_validators_per_sweep,state.registry.validators.size());
 if(check)check();return next;
}
CapellaWithdrawalExecutionCandidate verify_capella_withdrawal_execution(const CapellaWithdrawalState& consensus,const CapellaWithdrawalRules& rules,const Header& parent,const State& state,const Block& block,const Int& chain_id,ReplayPolicy replay,const std::function<Bytes(const Int&)>& history,const std::function<void()>& check){
 if(check)check();if(state.root()!=parent.state_root)throw std::invalid_argument("Capella execution parent state mismatch");
 const auto expected=expected_capella_withdrawals(consensus,rules,check);
 if(!block.withdrawals.is_array()||block.withdrawals.size()!=expected.size())throw std::invalid_argument("unexpected Capella withdrawals");
 for(size_t i=0;i<expected.size();++i){if(check)check();if(withdrawal_payload(block.withdrawals[i]).encode()!=withdrawal_payload(expected[i].to_json()).encode())throw std::invalid_argument("unexpected Capella withdrawals");}
 auto after=prepare_capella_withdrawals(consensus,rules,expected,check);if(!after)throw std::logic_error("Capella withdrawal preparation mismatch");
 CapellaWithdrawalExecutionCandidate candidate{std::move(*after),state,{}};
 ExecutionEngine engine(chain_id,history,EvmRevision::Shanghai,replay,ConsensusInputPolicy::EthereumValidated);
 candidate.receipts=engine.with_checkpoint(check).verify_block(candidate.execution,block,parent,false);
 if(check)check();return candidate;
}
CapellaWithdrawalState read_capella_withdrawal_state(std::span<const uint8_t> state,
 const ConsensusRoot& authenticated_root,const AltairStateLimits& l,const ExecutionPayloadLimits& execution,
 size_t budget,uint64_t slots,const std::function<void()>& check){
 if(!slots)throw std::invalid_argument("invalid Capella epoch length");
 if(capella_beacon_state_root(state,l,execution,budget,check)!=authenticated_root)throw std::invalid_argument("Capella withdrawal state authentication failed");
 auto number=[&](size_t at,size_t width=8){uint64_t n=0;for(size_t i=0;i<width;++i)n|=uint64_t(state[at+i])<<(8*i);return n;};
 const size_t historical=176+64*l.slots_per_historical_root,randao=historical+96;
 const size_t slashings=randao+32*l.epochs_per_historical_vector,previous=slashings+8*l.epochs_per_slashings_vector;
 const size_t execution_field=previous+229+96*l.sync_committee_size;
 const size_t start=number(historical+88,4),balances=number(historical+92,4),end=number(previous,4);
 const size_t count=(balances-start)/121;
 const size_t current=number(previous+4,4),inactivity=number(previous+129,4),header=number(execution_field,4);
 if((end-balances)/8!=count||current-end!=count||inactivity-current!=count||(header-inactivity)/8!=count)throw std::invalid_argument("Capella registry cardinality mismatch");
 CapellaWithdrawalState projected;projected.registry.epoch=number(40)/slots;
 projected.next_withdrawal_index=number(execution_field+4);projected.next_validator_index=number(execution_field+12);
 for(size_t i=0;i<count;++i){
  if(check)check();const size_t at=start+121*i;ConsensusSlashingValidator v;
  std::copy_n(state.begin()+at,48,v.public_key.begin());std::copy_n(state.begin()+at+48,32,v.withdrawal_credentials.begin());
  v.effective_balance=number(at+80);v.slashed=state[at+88];v.activation_eligibility_epoch=number(at+89);v.activation_epoch=number(at+97);v.exit_epoch=number(at+105);v.withdrawable_epoch=number(at+113);v.balance=number(balances+8*i);projected.registry.validators.emplace(i,v);
 }
 return projected;
}
std::optional<CapellaWithdrawalSszCandidate> prepare_capella_withdrawal_state(
 std::span<const uint8_t> state,const ConsensusRoot& authenticated_root,
 const AltairStateLimits& l,const ExecutionPayloadLimits& execution,size_t budget,
 uint64_t slots,const CapellaWithdrawalRules& rules,const std::vector<CapellaWithdrawal>& advertised,
 const std::function<void()>& check){
 const auto projected=read_capella_withdrawal_state(state,authenticated_root,l,execution,budget,slots,check);
 const size_t historical=176+64*l.slots_per_historical_root,randao=historical+96;
 const size_t execution_field=randao+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector+229+96*l.sync_committee_size;
 size_t balances=0;for(unsigned i=0;i<4;++i)balances|=size_t(state[historical+92+i])<<(8*i);
 const auto after=prepare_capella_withdrawals(projected,rules,advertised,check);if(!after)return std::nullopt;
 Bytes post(state.begin(),state.end());auto put=[&](size_t at,uint64_t n){for(unsigned i=0;i<8;++i)post[at+i]=uint8_t(n>>(8*i));};
 for(const auto& w:advertised){if(check)check();put(balances+8*w.validator_index,after->registry.validators.at(w.validator_index).balance);}
 put(execution_field+4,after->next_withdrawal_index);put(execution_field+12,after->next_validator_index);
 const auto root=capella_beacon_state_root(post,l,execution,budget,check);if(check)check();return CapellaWithdrawalSszCandidate{std::move(post),root};
}
}
