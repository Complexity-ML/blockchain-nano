#include "nano/engine.hpp"
#include <iostream>
using namespace nano;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F> void rejects(F f,const std::string& message){bool rejected=false;try{f();}catch(const std::invalid_argument& e){rejected=message==e.what();}check(rejected,message.c_str());}
const std::string history="0x0000f90827f1c53a10cb7a02335b175320002935",withdrawal="0x00000961ef480eb55e80d19ad83579a64c007002",consolidation="0x0000bbddc7ce488642fb579f8b00f3a590007251";
Block input(){Block b;b.header.number=1;b.header.base_fee=0;b.header.parent_hash=bytes(Int(42),32);b.header.parent_beacon_block_root=Bytes(32);b.header.excess_blob_gas=0;return b;}
State initial(){State s;s.get(history).code=unhex("3373fffffffffffffffffffffffffffffffffffffffe14604657602036036042575f35600143038111604257611fff81430311604257611fff9006545f5260205ff35b5f5ffd5b5f35611fff60014303065500");s.get(history).nonce=1;s.get(withdrawal).code={0};s.get(consolidation).code={0};return s;}
}
int main(){try{
 const auto empty_hash=unhex("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
 check(execution_requests_hash({})==empty_hash&&execution_requests_hash({Bytes{0},Bytes{1},Bytes{2}})==empty_hash,"Empty requests commitment differs");
 rejects([]{execution_requests_hash({Bytes{1,2},Bytes{0,3}});},"execution requests not in type order");
 rejects([]{execution_requests_hash({Bytes{0},Bytes{0}});},"execution requests not in type order");
 rejects([]{execution_requests_hash({Bytes{}});},"execution requests not in type order");
 // Fixed-width ABI event decoding retains uint64 bytes in little-endian order.
 Log log;log.address="0x00000000219ab540356cbb839cbe05303d7705fa";log.topics={unhex("649bbc62d0e31342afea4e5cd82d4049e7e1ee912fc0889aa790803be39038c5")};log.data=Bytes(576);
 const size_t offsets[]={160,256,320,384,512},sizes[]={48,32,8,96,8};Bytes expected{0};
 for(size_t i=0;i<5;++i){auto offset=bytes(Int(offsets[i]),32),size=bytes(Int(sizes[i]),32);std::copy(offset.begin(),offset.end(),log.data.begin()+i*32);std::copy(size.begin(),size.end(),log.data.begin()+offsets[i]);for(size_t j=0;j<sizes[i];++j){uint8_t value=uint8_t(i*50+j);log.data[offsets[i]+32+j]=value;expected.push_back(value);}}
 check(deposit_requests({log})==expected,"Deposit bytes/order differ");
 auto unrelated=log;unrelated.address=history;unrelated.data={};check(deposit_requests({unrelated})==Bytes{0},"Unrelated log parsed as deposit");
 for(size_t position:{size_t(31),size_t(191)}){auto bad=log;bad.data[position]^=1;rejects([&]{deposit_requests({bad});},"invalid deposit event layout");}
 auto short_log=log;short_log.data.pop_back();rejects([&]{deposit_requests({short_log});},"invalid deposit event layout");
 ExecutionEngine engine(1,{},EvmRevision::Prague,ReplayPolicy::RequireChainId,ConsensusInputPolicy::EthereumValidated);
 auto s=initial();auto b=input();auto receipts=engine.execute_block(s,b,false);
 check(s.get_storage(history,0)==42,"Parent hash history was not updated");
 check(b.header.requests_hash==empty_hash&&receipts.empty()&&b.header.gas_used==0,"System calls changed receipt/gas/empty commitment");
 check(Rlp::decode(b.header.encode()).items.size()==21&&Header::from_json(b.header.to_json()).hash()==b.header.hash(),"Prague header roundtrip differs");
 auto replay=initial();engine.verify_block(replay,b,false);check(replay.root()==s.root(),"Prague block replay differs");
 auto bad=b;bad.header.requests_hash=Bytes(32);auto unchanged=initial();auto before=unchanged.to_json();rejects([&]{engine.verify_block(unchanged,bad,false);},"execution requests hash mismatch");check(unchanged.to_json()==before,"Bad commitment leaked system state");
 auto malformed=b.header;malformed.requests_hash=Bytes(31);rejects([&]{malformed.encode();},"invalid requests hash");
 auto native=input();native.header.requests_hash=empty_hash;native.header.parent_beacon_block_root.reset();State local;rejects([&]{ExecutionEngine(1,{},EvmRevision::Prague).execute_block(local,native,false);},"execution requests not authorized by Nano consensus");
 auto old=input();old.header.requests_hash=empty_hash;State old_state;rejects([&]{ExecutionEngine(1,{},EvmRevision::Cancun,ReplayPolicy::RequireChainId,ConsensusInputPolicy::EthereumValidated).execute_block(old_state,old,false);},"execution requests before Prague");
 // Required post-transaction calls must fail the entire block, including earlier history writes.
 for(const auto& target:{withdrawal,consolidation})for(bool absent:{false,true}){
  auto failing=initial();if(absent)failing.erase(target);else failing.get(target).code=unhex("600160005560006000fd");auto candidate=input();Transaction tx;tx.chain_id=1;tx.to="0x0000000000000000000000000000000000000011";tx.gas=21000;tx.gas_price=1;tx.value=7;auto signed_tx=SignedTransaction::sign(tx,bytes(Int(1),32));failing.get(signed_tx.sender()).balance=1000000;candidate.transactions.push_back(signed_tx);auto original=failing.to_json();
  rejects([&]{engine.execute_block(failing,candidate,false);},absent?"required system contract is empty":"required system contract call failed");check(failing.to_json()==original,"Required call failure leaked state");check(!candidate.header.requests_hash,"Failed block mutated header");
 }
 // Calls execute actual deployed bytecode, and their output is committed in type order.
 auto custom=initial();custom.get(withdrawal).code=unhex("60ab5f5360015ff3");custom.get(consolidation).code=unhex("60cd5f5360015ff3");auto custom_block=input();engine.execute_block(custom,custom_block,false);
 check(custom_block.header.requests_hash==execution_requests_hash({Bytes{0},Bytes{1,0xab},Bytes{2,0xcd}}),"System return data ignored");
 auto no_history=initial();no_history.erase(history);auto no_history_block=input();engine.execute_block(no_history,no_history_block,false);check(!no_history.exists(history),"Absent history contract created account");
 auto genesis=input();genesis.header.number=0;State empty;engine.execute_block(empty,genesis,false);check(empty.to_json()==State{}.to_json()&&genesis.header.requests_hash==empty_hash,"Genesis ran system calls");
 std::cout<<"Prague requests, ABI deposits, history, system calls, rollback and fork policy passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
