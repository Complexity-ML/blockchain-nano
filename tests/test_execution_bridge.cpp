#include "nano/consensus_execution_bridge.hpp"
#include "nano/consensus_bellatrix_execution.hpp"
#include "support/phase0_minimal_ssz.hpp"
#include <iostream>
using namespace nano;
static void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class A> void copy_bytes(A& out,const Bytes& in){require(out.size()==in.size(),"fixture field width");std::copy(in.begin(),in.end(),out.begin());}
static BellatrixExecutionPayload payload(const Block& b){
 const auto& h=b.header;BellatrixExecutionPayload p;
 copy_bytes(p.parent_hash,h.parent_hash);copy_bytes(p.state_root,h.state_root);
 copy_bytes(p.receipts_root,h.receipts_root);copy_bytes(p.logs_bloom,h.logs_bloom);
 copy_bytes(p.fee_recipient,unhex(h.coinbase));copy_bytes(p.prev_randao,h.mix_hash);
 copy_bytes(p.block_hash,h.hash());auto fee=bytes(*h.base_fee,32);std::reverse(fee.begin(),fee.end());copy_bytes(p.base_fee_per_gas,fee);
 p.block_number=u64(h.number);p.timestamp=u64(h.timestamp);p.gas_limit=u64(h.gas_limit);p.gas_used=u64(h.gas_used);p.extra_data=h.extra_data;
 for(const auto& tx:b.transactions)p.transactions.push_back(tx.raw());return p;
}
int main(){try{
 const ExecutionPayloadLimits limits{32,1048576,1024};constexpr size_t budget=2*1048576;
 const auto key=bytes(Int(777),32);const std::string target="0x0000000000000000000000000000000000001234";
 State state;Block block;Transaction tx;tx.to=target;tx.gas=100000;tx.gas_price=2000000000;tx.max_fee=2000000000;tx.priority_fee=100000000;
 for(unsigned type=0;type<3;++type){tx.type=type;tx.nonce=type;block.transactions.push_back(SignedTransaction::sign(tx,key));}
 const auto sender=block.transactions.front().sender();state.get(sender).balance=Int("1000000000000000000");
 // PREVRANDAO and BLOCKHASH must come from the authenticated execution context.
 state.get(target).code=unhex("4460005560004060015500");
 const std::string reverter="0x0000000000000000000000000000000000001235";
 state.get(reverter).code=unhex("600160005560006000fd");
 auto reverting=tx;reverting.nonce=3;reverting.to=reverter;
 block.transactions.push_back(SignedTransaction::sign(reverting,key));
 Header parent;parent.difficulty=0;parent.state_root=state.root();parent.timestamp=100;
 const auto before=state.root();auto history=[&](const Int& number){require(number==0,"unexpected historical block");return parent.hash();};
 block.header.parent_hash=parent.hash();block.header.number=1;block.header.timestamp=112;
 block.header.base_fee=next_base_fee(parent);block.header.difficulty=0;block.header.mix_hash=bytes(Int(42),32);
 block.header.coinbase="0x0000000000000000000000000000000000009999";
 State expected=state;ExecutionEngine engine(1337,history,EvmRevision::Paris,ReplayPolicy::RequireChainId);
 const auto receipts=engine.execute_block(expected,block,false);const auto p=payload(block);
 auto run=[&](const BellatrixExecutionPayload& value,const std::function<void()>& check={}){
  return prepare_bellatrix_execution(encode_bellatrix_execution_payload(value,limits,budget),limits,budget,parent,state,112,p.prev_randao,1337,ReplayPolicy::RequireChainId,history,check);
 };
 size_t checkpoints=0;auto accepted=run(p,[&]{++checkpoints;});
 require(accepted.block.header.hash()==block.header.hash()&&accepted.state.root()==expected.root(),"valid payload differs from execution");
 require(accepted.receipts.size()==4&&accepted.receipts[0].success&&accepted.receipts[1].success&&accepted.receipts[2].success&&!accepted.receipts.back().success,"typed transactions or REVERT failed");
 require(accepted.state.get_storage(reverter,0)==0,"reverted contract retained storage");
 Int tips=0;for(size_t i=0;i<receipts.size();++i)tips+=receipts[i].gas_used*(block.transactions[i].tx.effective_gas_price(*block.header.base_fee)-*block.header.base_fee);
 require(accepted.state.get(block.header.coinbase).balance==tips,"PoW subsidy or incorrect base fee burn");
 require(accepted.state.get_storage(target,0)==42&&accepted.state.get_storage(target,1)==integer(parent.hash()),"execution context not bound");
 require(state.root()==before&&state.get_storage(target,0)==0,"parent state mutated");
 size_t rejected=0;
 auto deny=[&](const BellatrixExecutionPayload& value,const char* reason){bool refused=false;try{run(value);}catch(const std::invalid_argument& e){refused=std::string(e.what()).find(reason)!=std::string::npos;}require(refused,"invalid payload accepted or wrong rejection");require(state.root()==before,"invalid payload modified parent");++rejected;};
 auto changed=p;changed.parent_hash[0]^=1;deny(changed,"parent");
 changed=p;++changed.block_number;deny(changed,"number");
 changed=p;++changed.timestamp;deny(changed,"timestamp");
 changed=p;changed.prev_randao[0]^=1;deny(changed,"RANDAO");
 changed=p;changed.block_hash[0]^=1;deny(changed,"block hash");
 changed=p;changed.base_fee_per_gas[31]=1;deny(changed,"block hash");
 changed=p;changed.transactions[0]={};deny(changed,"envelope");
 changed=p;changed.transactions[0]={3};deny(changed,"envelope");
 changed=p;changed.transactions[0]={4};deny(changed,"envelope");
 // Correct advertised hashes do not make false execution commitments valid.
 for(unsigned field=0;field<4;++field){auto bad=block;if(field==0)bad.header.state_root[0]^=1;if(field==1)bad.header.receipts_root[0]^=1;if(field==2)bad.header.logs_bloom[0]^=1;if(field==3)++bad.header.gas_used;deny(payload(bad),"commitments");}
 {auto bad=block;++*bad.header.base_fee;deny(payload(bad),"base fee");}
 {auto bad=block;bad.header.gas_limit+=100000;deny(payload(bad),"gas limits");}
 {auto bad=block;auto wrong=reverting;wrong.chain_id=999;bad.transactions.back()=SignedTransaction::sign(wrong,key);bad.header.transactions_root=transactions_root(bad.transactions);deny(payload(bad),"chain id");}
 // Cancellation at every observed callback, including during/after EVM writes.
 for(size_t stop=1;stop<=checkpoints;++stop){size_t seen=0;bool aborted=false;try{run(p,[&]{if(++seen==stop)throw std::runtime_error("cancel bridge");});}catch(const std::runtime_error& e){aborted=std::string(e.what())=="cancel bridge";}require(aborted&&state.root()==before,"cancellation leaked state or was swallowed");}
 auto wire=encode_bellatrix_execution_payload(p,limits,budget);
 // Fork activation is one-way: Paris admission cannot follow Shanghai.
 {auto shanghai_parent=parent;shanghai_parent.withdrawals_root=withdrawals_root(Json::array());bool refused=false;
  try{prepare_bellatrix_execution(wire,limits,budget,shanghai_parent,state,112,p.prev_randao,1337,ReplayPolicy::RequireChainId,history);}
  catch(const std::invalid_argument& e){refused=std::string(e.what())=="parent does not match post-merge execution profile";}
  require(refused&&state.root()==before,"Shanghai to Paris downgrade accepted");++rejected;}
 bool wrong_state=false;try{State unrelated;prepare_bellatrix_execution(wire,limits,budget,parent,unrelated,112,p.prev_randao,1337,ReplayPolicy::RequireChainId,history);}catch(const std::invalid_argument& e){wrong_state=std::string(e.what()).find("parent state")!=std::string::npos;}require(wrong_state,"unauthenticated parent state accepted");
 bool oversize=false;try{prepare_bellatrix_execution(wire,limits,wire.size()-1,parent,state,112,p.prev_randao,1337,ReplayPolicy::RequireChainId,history);}catch(const std::invalid_argument& e){oversize=std::string(e.what()).find("budget")!=std::string::npos;}require(oversize,"transport budget ignored");
 // A producer can construct commitments matching an EVM that swallowed a host
 // history error. The adapter must still refuse, not accept a false REVERT/zero.
 for(bool missing:{false,true}){
  std::function<Bytes(const Int&)> broken_history=[&](const Int&)->Bytes{if(missing)return {};throw std::runtime_error("history unavailable");};
  auto false_block=block;State false_state=state;
  ExecutionEngine(1337,broken_history,EvmRevision::Paris,ReplayPolicy::RequireChainId).execute_block(false_state,false_block,false);
  bool failed=false;try{prepare_bellatrix_execution(encode_bellatrix_execution_payload(payload(false_block),limits,budget),limits,budget,parent,state,112,p.prev_randao,1337,ReplayPolicy::RequireChainId,broken_history);}catch(const std::exception& ex){failed=std::string(ex.what())==(missing?"execution history hash width":"history unavailable");}
  require(failed&&state.root()==before,"host history failure became valid execution");
 }
 // Isolated process_execution_payload operation: the static SSZ fixture is a
 // schema shell, not a lifecycle-valid or signed Beacon block transition.
 const auto folder=test::fs::path(NANO_TEST_SOURCE)/"tests/fixtures/consensus-bellatrix-containers-official";
 const auto preset=test::read(folder/"minimal-phase0.yaml");auto param=[&](const char* name){return test::phase0_minimal::parameter(preset,name);};
 const AltairStateLimits sl{param("SLOTS_PER_HISTORICAL_ROOT"),param("EPOCHS_PER_HISTORICAL_VECTOR"),param("EPOCHS_PER_SLASHINGS_VECTOR"),param("HISTORICAL_ROOTS_LIMIT"),param("EPOCHS_PER_ETH1_VOTING_PERIOD")*param("SLOTS_PER_EPOCH"),param("VALIDATOR_REGISTRY_LIMIT"),test::phase0_minimal::parameter(test::read(folder/"minimal-altair.yaml"),"SYNC_COMMITTEE_SIZE")};
 auto beacon=test::phase0_minimal::binary(folder/"BeaconState/case_0/serialized.ssz");
 const size_t randao_at=272+64*sl.slots_per_historical_root,header_field=randao_at+32*sl.epochs_per_historical_vector+8*sl.epochs_per_slashings_vector+229+96*sl.sync_committee_size;
 const size_t header_at=test::phase0_minimal::little(beacon,header_field,4);
 auto put64=[](Bytes& out,size_t at,uint64_t n){for(unsigned i=0;i<8;++i)out.at(at+i)=uint8_t(n>>(8*i));};
 put64(beacon,0,100);put64(beacon,40,1);std::copy(p.prev_randao.begin(),p.prev_randao.end(),beacon.begin()+randao_at);
 Block parent_block;parent_block.header=parent;
 const auto previous_header=encode_bellatrix_execution_header(bellatrix_execution_payload_header(payload(parent_block),limits),limits.extra_data_bytes,budget);
 beacon.resize(header_at);beacon.insert(beacon.end(),previous_header.begin(),previous_header.end());
 const auto authenticated=bellatrix_beacon_state_root(beacon,sl,limits,budget);
 auto paired=[&](const Bytes& input,const ConsensusRoot& root,const Bytes& value,size_t maximum,const std::function<void()>& cp={}){
  return prepare_bellatrix_payload_state(input,root,sl,limits,maximum,param("SLOTS_PER_EPOCH"),12,value,budget,parent,state,1337,ReplayPolicy::RequireChainId,history,cp);
 };
 size_t paired_calls=0;const auto bound=paired(beacon,authenticated,wire,budget,[&]{++paired_calls;});
 require(bound.execution.state.root()==expected.root(),"state-bound execution mismatch");
 require(std::equal(beacon.begin(),beacon.begin()+header_at,bound.consensus_state.begin()),"payload operation modified unrelated Beacon fields");
 const auto latest=decode_bellatrix_execution_header(std::span<const uint8_t>(bound.consensus_state).subspan(header_at),limits.extra_data_bytes,budget);
 require(latest==bellatrix_execution_payload_header(p,limits),"latest execution header not updated");
 require(bound.consensus_root==bellatrix_beacon_state_root(bound.consensus_state,sl,limits,budget)&&bound.consensus_root!=authenticated,"post Beacon root mismatch");
 size_t binding_rejections=0;
 auto deny_binding=[&](const Bytes& input,const ConsensusRoot& root,const Bytes& value,size_t maximum,const char* reason){bool refused=false;try{paired(input,root,value,maximum);}catch(const std::invalid_argument& ex){refused=std::string(ex.what()).find(reason)!=std::string::npos;}require(refused,"state binding accepted invalid input or wrong reason");++binding_rejections;};
 auto changed_root=authenticated;changed_root[0]^=1;deny_binding(beacon,changed_root,wire,budget,"authentication");
 auto changed_beacon=beacon;changed_beacon[header_at+472]^=1;deny_binding(changed_beacon,bellatrix_beacon_state_root(changed_beacon,sl,limits,budget),wire,budget,"execution parent");
 changed_beacon=beacon;changed_beacon[randao_at]^=1;deny_binding(changed_beacon,bellatrix_beacon_state_root(changed_beacon,sl,limits,budget),wire,budget,"RANDAO");
 changed_beacon=beacon;put64(changed_beacon,40,2);deny_binding(changed_beacon,bellatrix_beacon_state_root(changed_beacon,sl,limits,budget),wire,budget,"timestamp");
 changed_beacon=beacon;put64(changed_beacon,40,UINT64_MAX);deny_binding(changed_beacon,bellatrix_beacon_state_root(changed_beacon,sl,limits,budget),wire,budget,"overflow");
 auto longer=block;longer.header.extra_data=Bytes(32,7);const auto longer_wire=encode_bellatrix_execution_payload(payload(longer),limits,budget);
 deny_binding(beacon,authenticated,longer_wire,beacon.size(),"post-state budget");
 const std::set<size_t> paired_stops{1,paired_calls/2,paired_calls-1,paired_calls};
 for(size_t stop:paired_stops){size_t seen=0;bool aborted=false;try{paired(beacon,authenticated,wire,budget,[&]{if(++seen==stop)throw std::runtime_error("cancel paired");});}catch(const std::runtime_error& ex){aborted=std::string(ex.what())=="cancel paired";}require(aborted,"state-bound cancellation lost");}
 require(bellatrix_beacon_state_root(beacon,sl,limits,budget)==authenticated&&state.root()==before,"paired operation mutated parent states");
 std::cout<<Json{{"scenario","bellatrix-execution-candidate"},{"signedTransactions",4},{"invalidPayloads",rejected},{"cancellationPoints",checkpoints},{"beaconBindingRefusals",binding_rejections},{"pairedCancellationPoints",paired_stops.size()},{"contractRevertAccepted",true},{"parentUnchanged",state.root()==before},{"networkActivated",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
