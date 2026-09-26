#include "nano/consensus_capella_withdrawals.hpp"
#include <iostream>
using namespace nano;
namespace {
void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
template<class F>void refuses(F f){bool bad=false;try{f();}catch(const std::invalid_argument&){bad=true;}catch(const std::overflow_error&){bad=true;}check(bad,"invalid withdrawal accepted");}
ConsensusSlashingValidator member(uint64_t balance,uint64_t effective,uint64_t withdrawable,uint8_t prefix=1){ConsensusSlashingValidator v;v.balance=balance;v.effective_balance=effective;v.withdrawable_epoch=withdrawable;v.withdrawal_credentials[0]=prefix;v.withdrawal_credentials[31]=42;return v;}
}
int main(){try{
 const CapellaWithdrawalRules rules{32000000000,2,5};CapellaWithdrawalState initial{};initial.registry.epoch=10;initial.next_withdrawal_index=7;
 initial.registry.validators={{0,member(35000000000,32000000000,UINT64_MAX)},{1,member(17000000000,16000000000,10)},{2,member(40000000000,32000000000,9,0)},{3,member(40000000000,31000000000,11)},{4,member(0,0,9)}};
 const auto expected=expected_capella_withdrawals(initial,rules);check(expected.size()==2&&expected[0].index==7&&expected[0].validator_index==0&&expected[0].amount_gwei==3000000000&&expected[1].index==8&&expected[1].validator_index==1&&expected[1].amount_gwei==17000000000,"full/partial priority or withdrawal order");
 const auto result=prepare_capella_withdrawals(initial,rules,expected);check(result&&result->registry.validators.at(0).balance==32000000000&&result->registry.validators.at(1).balance==0&&result->next_withdrawal_index==9&&result->next_validator_index==2,"withdrawal accounting/cursors");check(!prepare_capella_withdrawals(*result,rules,expected),"duplicate debit accepted");
 for(unsigned mutation=0;mutation<5;++mutation){auto bad=expected;if(mutation==0)bad[0].address[0]^=1;if(mutation==1)++bad[0].amount_gwei;if(mutation==2)++bad[0].index;if(mutation==3)std::swap(bad[0],bad[1]);if(mutation==4)bad.pop_back();check(!prepare_capella_withdrawals(initial,rules,bad),"altered consensus withdrawal list accepted");}
 auto wrap=initial;wrap.next_validator_index=4;auto scan=rules;scan.maximum_per_payload=3;scan.maximum_validators_per_sweep=UINT64_MAX;const auto wrapped=prepare_capella_withdrawals(wrap,scan,expected_capella_withdrawals(wrap,scan));check(wrapped&&wrapped->next_validator_index==4,"overflow-safe sweep modulo differs");
 auto empty=initial;for(auto& [_,v]:empty.registry.validators)v.withdrawal_credentials[0]=0;scan.maximum_validators_per_sweep=2;const auto no=prepare_capella_withdrawals(empty,scan,{});check(no&&no->next_withdrawal_index==7&&no->next_validator_index==2,"empty sweep cursor");
 auto overflow=initial;overflow.next_withdrawal_index=UINT64_MAX;refuses([&]{expected_capella_withdrawals(overflow,rules);});auto malformed=initial;malformed.registry.validators.erase(2);refuses([&]{expected_capella_withdrawals(malformed,rules);});
 // Same recipient for full and partial withdrawal; no EVM callback is invoked.
 const std::string recipient=expected[0].to_json().at("address");State evm;evm.get(recipient).balance=13;evm.get(recipient).code={0xfe};Header parent;parent.difficulty=0;parent.base_fee=0;parent.state_root=evm.root();parent.withdrawals_root=withdrawals_root(Json::array());
 Block block;block.header.parent_hash=parent.hash();block.header.number=1;block.header.timestamp=12;block.header.difficulty=0;block.header.base_fee=0;for(const auto& w:expected)block.withdrawals.push_back(w.to_json());
 auto built=evm;ExecutionEngine ethereum(1337,{},EvmRevision::Shanghai,ReplayPolicy::RequireChainId,ConsensusInputPolicy::EthereumValidated);ethereum.execute_block(built,block,false);
 const auto paid=verify_capella_withdrawal_execution(initial,rules,parent,evm,block,1337,ReplayPolicy::RequireChainId);
 check(paid.consensus==*result&&paid.execution.root()==built.root()&&paid.execution.get(recipient).balance-evm.get(recipient).balance==Int("20000000000000000000")&&paid.receipts.empty()&&block.header.gas_used==0,"coupled withdrawal debit/credit mismatch");
 const auto old_consensus=initial;const auto old_evm=evm.to_json();auto corrupted=block;corrupted.header.state_root[0]^=1;refuses([&]{verify_capella_withdrawal_execution(initial,rules,parent,evm,corrupted,1337,ReplayPolicy::RequireChainId);});check(initial==old_consensus&&evm.to_json()==old_evm,"late EVM failure leaked consensus debit");
 corrupted=block;corrupted.withdrawals[0]["amount"]="0x1";refuses([&]{verify_capella_withdrawal_execution(initial,rules,parent,evm,corrupted,1337,ReplayPolicy::RequireChainId);});
 bool cancelled=false;try{prepare_capella_withdrawals(initial,rules,expected,[]{throw ExecutionCancelled("cancel");});}catch(const ExecutionCancelled&){cancelled=true;}check(cancelled&&initial==old_consensus,"cancelled withdrawal mutated input");
 std::cout<<"Capella sweep, debit/credit, ordering, cursor wrap, duplicate, malformed and rollback checks passed; runtime not activated\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
