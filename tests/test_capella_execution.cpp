#include "nano/consensus_capella_execution.hpp"
#include "support/phase0_minimal_ssz.hpp"
using namespace nano;
static void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class A> void copy_bytes(A& out,const Bytes& in){require(out.size()==in.size(),"fixture field width");std::copy(in.begin(),in.end(),out.begin());}
static CapellaExecutionPayload payload(const Block& b){
 const auto& h=b.header;CapellaExecutionPayload p;
 copy_bytes(p.parent_hash,h.parent_hash);copy_bytes(p.state_root,h.state_root);
 copy_bytes(p.receipts_root,h.receipts_root);copy_bytes(p.logs_bloom,h.logs_bloom);
 copy_bytes(p.fee_recipient,unhex(h.coinbase));copy_bytes(p.prev_randao,h.mix_hash);
 copy_bytes(p.block_hash,h.hash());auto fee=bytes(*h.base_fee,32);std::reverse(fee.begin(),fee.end());copy_bytes(p.base_fee_per_gas,fee);
 p.block_number=u64(h.number);p.timestamp=u64(h.timestamp);p.gas_limit=u64(h.gas_limit);p.gas_used=u64(h.gas_used);p.extra_data=h.extra_data;
 for(const auto& tx:b.transactions)p.transactions.push_back(tx.raw());return p;
}
int main(){try{
 using namespace test;using namespace test::phase0_minimal;
 const auto fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures",preset_folder=fixtures/"consensus-bellatrix-containers-official";
 const auto phase0=read(preset_folder/"minimal-phase0.yaml"),altair=read(preset_folder/"minimal-altair.yaml");auto par=[&](const char* k){return parameter(phase0,k);};
 const AltairStateLimits sl{par("SLOTS_PER_HISTORICAL_ROOT"),par("EPOCHS_PER_HISTORICAL_VECTOR"),par("EPOCHS_PER_SLASHINGS_VECTOR"),par("HISTORICAL_ROOTS_LIMIT"),par("EPOCHS_PER_ETH1_VOTING_PERIOD")*par("SLOTS_PER_EPOCH"),par("VALIDATOR_REGISTRY_LIMIT"),parameter(altair,"SYNC_COMMITTEE_SIZE")};
 const ExecutionPayloadLimits limits{32,1048576,1024};constexpr size_t budget=4*1048576;
 const auto capella=read(fixtures/"consensus-capella-withdrawals-official/minimal-capella.yaml");
 const CapellaWithdrawalRules rules{par("MAX_EFFECTIVE_BALANCE"),parameter(capella,"MAX_WITHDRAWALS_PER_PAYLOAD"),parameter(capella,"MAX_VALIDATORS_PER_WITHDRAWALS_SWEEP")};
 const auto fixture=fixtures/"consensus-capella-withdrawals-official/all_withdrawal";
 auto beacon=binary(fixture/"pre.ssz");const auto raw=decode_capella_execution_payload(binary(fixture/"execution_payload.ssz"),limits,rules.maximum_per_payload,budget);require(raw.has_value(),"withdrawal fixture");
 auto put=[](Bytes& b,size_t at,uint64_t n,unsigned width=8){for(unsigned i=0;i<width;++i)b.at(at+i)=uint8_t(n>>(8*i));};
 put(beacon,0,100);const auto slot=little(beacon,40,8),timestamp=100+slot*6;
 const size_t randao_at=272+64*sl.slots_per_historical_root,field=randao_at+32*sl.epochs_per_historical_vector+8*sl.epochs_per_slashings_vector+229+96*sl.sync_committee_size;
 const size_t header_at=little(beacon,field,4);ConsensusRoot randao{};std::copy_n(beacon.begin()+randao_at+32*((slot/par("SLOTS_PER_EPOCH"))%sl.epochs_per_historical_vector),32,randao.begin());
 State state;const std::string target="0x0000000000000000000000000000000000001234";
 Transaction tx;tx.to=target;tx.gas=100000;tx.gas_price=0;const auto signed_tx=SignedTransaction::sign(tx,bytes(Int(777),32));state.get(signed_tx.sender()).balance=1000000;state.get(target).code=unhex("602a5f5500");
 Header parent;parent.difficulty=0;parent.base_fee=0;parent.withdrawals_root=withdrawals_root(Json::array());parent.state_root=state.root();parent.timestamp=0;
 Block previous;previous.header=parent;const auto header=encode_capella_execution_header(capella_execution_payload_header(payload(previous),limits,rules.maximum_per_payload),limits.extra_data_bytes,budget);
 beacon.resize(header_at);beacon.insert(beacon.end(),header.begin(),header.end());put(beacon,field+20,beacon.size(),4);const Bytes summaries(64,42);beacon.insert(beacon.end(),summaries.begin(),summaries.end());
 const auto root=capella_beacon_state_root(beacon,sl,limits,budget),before=root;const auto original=beacon;const auto evm_before=state.to_json();
 auto history=[&](const Int&){return parent.hash();};
 Block block;block.transactions.push_back(signed_tx);block.header.parent_hash=parent.hash();block.header.number=1;block.header.timestamp=Int(std::to_string(timestamp));block.header.base_fee=0;block.header.difficulty=0;block.header.mix_hash=Bytes(randao.begin(),randao.end());block.header.extra_data={1,2,3};
 for(const auto& w:raw->withdrawals)block.withdrawals.push_back(w.to_json());
 State expected=state;ExecutionEngine(1337,history,EvmRevision::Shanghai,ReplayPolicy::RequireChainId,ConsensusInputPolicy::EthereumValidated).execute_block(expected,block,false);
 auto p=payload(block);p.withdrawals=raw->withdrawals;
 auto run=[&](const CapellaExecutionPayload& value,const std::function<void()>& check={}){return prepare_capella_payload_state(beacon,root,sl,limits,budget,par("SLOTS_PER_EPOCH"),6,rules,encode_capella_execution_payload(value,limits,rules.maximum_per_payload,budget),budget,parent,state,1337,ReplayPolicy::RequireChainId,history,check);};
 unsigned callbacks=0;const auto result=run(p,[&]{++callbacks;});
 require(result.execution.receipts.size()==1&&result.execution.receipts[0].success&&result.execution.state.get_storage(target,0)==42,"Shanghai PUSH0 execution missing");
 require(result.execution.state.root()==expected.root()&&result.execution.block.header.hash()==block.header.hash(),"execution differs");
 const auto debited=prepare_capella_withdrawal_state(beacon,root,sl,limits,budget,par("SLOTS_PER_EPOCH"),rules,p.withdrawals);require(debited.has_value(),"debit failed");
 auto expected_ssz=debited->state;const auto next_header=encode_capella_execution_header(capella_execution_payload_header(p,limits,rules.maximum_per_payload),limits.extra_data_bytes,budget);expected_ssz.resize(header_at);expected_ssz.insert(expected_ssz.end(),next_header.begin(),next_header.end());put(expected_ssz,field+20,expected_ssz.size(),4);expected_ssz.insert(expected_ssz.end(),summaries.begin(),summaries.end());
 require(result.consensus_state==expected_ssz&&result.consensus_root==capella_beacon_state_root(expected_ssz,sl,limits,budget),"paired consensus or summaries differ");
 unsigned rejected=0;auto deny=[&](const CapellaExecutionPayload& value){bool bad=false;try{run(value);}catch(const std::invalid_argument&){bad=true;}require(bad&&beacon==original&&state.to_json()==evm_before,"invalid payload changed parent");++rejected;};
 auto bad=p;bad.withdrawals[0].amount_gwei^=1;deny(bad);bad=p;bad.parent_hash[0]^=1;deny(bad);bad=p;++bad.timestamp;deny(bad);bad=p;bad.prev_randao[0]^=1;deny(bad);bad=p;bad.block_hash[0]^=1;deny(bad);
 auto false_block=block;false_block.header.state_root[0]^=1;bad=payload(false_block);bad.withdrawals=p.withdrawals;deny(bad);
 // A self-consistent EVM block cannot authorize a larger consensus withdrawal.
 auto forged=block;auto forged_withdrawals=p.withdrawals;++forged_withdrawals[0].amount_gwei;forged.withdrawals=Json::array();for(const auto& w:forged_withdrawals)forged.withdrawals.push_back(w.to_json());
 State forged_state=state;ExecutionEngine(1337,history,EvmRevision::Shanghai,ReplayPolicy::RequireChainId,ConsensusInputPolicy::EthereumValidated).execute_block(forged_state,forged,false);
 auto forged_payload=payload(forged);forged_payload.withdrawals=forged_withdrawals;
 const auto independently_valid=prepare_capella_execution(encode_capella_execution_payload(forged_payload,limits,rules.maximum_per_payload,budget),limits,rules.maximum_per_payload,budget,parent,state,timestamp,randao,1337,ReplayPolicy::RequireChainId,history);
 require(independently_valid.state.root()==forged_state.root(),"forged withdrawal control not EVM-valid");deny(forged_payload);
 unsigned cancellations=0;for(unsigned stop:{1U,callbacks/2,callbacks}){unsigned n=0;bool stopped=false;try{run(p,[&]{if(++n==stop)throw ExecutionCancelled("cancel");});}catch(const ExecutionCancelled&){stopped=true;}require(stopped&&beacon==original&&state.to_json()==evm_before,"cancel leaked state");++cancellations;}
 require(before==capella_beacon_state_root(beacon,sl,limits,budget),"authenticated parent changed");
 std::cout<<Json{{"scenario","capella-payload-paired-state"},{"rejected",rejected},{"cancellations",cancellations},{"withdrawals",p.withdrawals.size()},{"historicalSummariesPreserved",true},{"networkActivated",false}}.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
