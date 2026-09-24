#include "nano/engine.hpp"
#include <iostream>
using namespace nano;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try{
 const std::string recipient="0x0000000000000000000000000000000000002000";
 auto item=[&](Int index,Int amount){return Json{{"index",quantity(index)},{"validatorIndex","0x7"},{"address",recipient},{"amount",quantity(amount)}};};
 Block candidate;candidate.header.base_fee=0;candidate.withdrawals=Json::array({item(100,1),item(200,2),item(300,0)});
 State initial;initial.get(recipient).code=Bytes{0xfe};initial.get(recipient).balance=10;auto before=initial.to_json();
 ExecutionEngine native(1,{},EvmRevision::Shanghai);auto state=initial;bool refused=false;
 try{native.execute_block(state,candidate,false);}catch(const std::invalid_argument& e){refused=std::string(e.what())=="withdrawals not authorized by Nano consensus";}
 check(refused&&state.to_json()==before&&!candidate.header.withdrawals_root,"Native withdrawals accepted or rejection mutated state/block");
 ExecutionEngine ethereum(1,{},EvmRevision::Shanghai,ReplayPolicy::RequireChainId,ConsensusInputPolicy::EthereumValidated);
 auto receipts=ethereum.execute_block(state,candidate,false);
 check(receipts.empty()&&candidate.header.gas_used==0&&state.get(recipient).balance==Int("3000000010")&&state.get(recipient).code==Bytes{0xfe},"Withdrawal invoked EVM, charged gas or lost credits");
 check(candidate.header.withdrawals_root==withdrawals_root(candidate.withdrawals),"Withdrawal commitment mismatch");
 auto replay=initial;ethereum.verify_block(replay,candidate,false);check(replay.root()==state.root(),"Withdrawal replay differs");
 auto bad=candidate;(*bad.header.withdrawals_root)[0]^=1;replay=initial;refused=false;
 try{ethereum.verify_block(replay,bad,false);}catch(const std::invalid_argument& e){refused=std::string(e.what())=="withdrawals root mismatch";}
 check(refused&&replay.to_json()==before,"Bad withdrawal root changed state");
 std::map<Bytes,Bytes> entries;for(size_t i=0;i<candidate.withdrawals.size();++i)entries[Rlp(Int(i)).encode()]=withdrawal_payload(candidate.withdrawals[i]).encode();
 check(withdrawals_root(candidate.withdrawals)==trie_root(entries),"Root must use position, not global withdrawal index");
 for(const auto* field:{"index","validatorIndex","amount"}){auto invalid=item(0,1);invalid[field]=quantity(Int(1)<<64);refused=false;try{withdrawal_payload(invalid);}catch(const std::invalid_argument&){refused=true;}check(refused,"Withdrawal uint64 overflow accepted");}
 for(auto invalid_address:{"0x01","0x000000000000000000000000000000000000000000"}){auto invalid=item(0,1);invalid["address"]=invalid_address;refused=false;try{withdrawal_payload(invalid);}catch(const std::invalid_argument&){refused=true;}check(refused,"Wrong withdrawal address width accepted");}
 State zero;Block empty;empty.header.base_fee=0;empty.withdrawals=Json::array({item(0,0)});ethereum.execute_block(zero,empty,false);check(zero.to_json()==State{}.to_json(),"Zero withdrawal created an account");
 ExecutionEngine paris(1,{},EvmRevision::Paris,ReplayPolicy::RequireChainId,ConsensusInputPolicy::EthereumValidated);auto pre=initial;auto preblock=candidate;refused=false;try{paris.execute_block(pre,preblock,false);}catch(const std::invalid_argument&){refused=true;}check(refused&&pre.to_json()==before,"Withdrawals activated before Shanghai");
 std::cout<<"Withdrawals: explicit consensus policy, atomic rejection, commitments, no EVM/gas, zero amounts and bounds passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
