#include "nano/engine.hpp"
#include <iostream>
using namespace nano;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try{
 const std::string contract="0x000f3df6d732807ef1319fb7b8bb8522d0beac02",system="0xfffffffffffffffffffffffffffffffffffffffe";
 State initial;initial.get(contract).code=unhex("3373fffffffffffffffffffffffffffffffffffffffe14604d57602036146024575f5ffd5b5f35801560495762001fff810690815414603c575f5ffd5b62001fff01545f5260205ff35b5f5ffd5b62001fff42064281555f359062001fff015500");initial.get(contract).nonce=1;
 Block input;input.header.number=1;input.header.timestamp=8192;input.header.base_fee=0;input.header.excess_blob_gas=0;input.header.parent_beacon_block_root=bytes(Int(123),32);
 ExecutionEngine ethereum(1,{},EvmRevision::Cancun,ReplayPolicy::RequireChainId,ConsensusInputPolicy::EthereumValidated);
 auto state=initial;auto block=input;auto receipts=ethereum.execute_block(state,block,false);
 check(state.get_storage(contract,1)==8192&&state.get_storage(contract,8192)==123,"Beacon root/timestamp ring buffer differs");
 check(receipts.empty()&&block.header.gas_used==0&&block.header.logs_bloom==Bytes(256)&&!state.exists(system),"System call produced receipt, gas, logs or sender account");
 check(Header::from_json(block.header.to_json()).encode()==block.header.encode()&&Rlp::decode(block.header.encode()).items.size()==20,"Cancun header encoding/roundtrip differs");
 auto replay=initial;ethereum.verify_block(replay,block,false);check(replay.root()==state.root(),"Beacon block replay differs");
 auto native=initial;auto original=native.to_json();auto rejected_block=input;bool rejected=false;
 try{ExecutionEngine(1,{},EvmRevision::Cancun).execute_block(native,rejected_block,false);}catch(const std::invalid_argument& e){rejected=std::string(e.what())=="beacon root not authorized by Nano consensus";}
 check(rejected&&native.to_json()==original,"Native policy accepted external root or mutated state");
 State absent;auto missing=input;ethereum.execute_block(absent,missing,false);check(absent.to_json()==State{}.to_json(),"Absent beacon contract created accounts");
 State reverting;reverting.get(contract).code=unhex("600160005560006000fd");auto before=reverting.to_json();auto failed_call=input;ethereum.execute_block(reverting,failed_call,false);check(reverting.to_json()==before,"Failed system call leaked storage");
 auto genesis=input;genesis.header.number=0;genesis.header.parent_beacon_block_root=Bytes(32);auto genesis_state=initial;ethereum.execute_block(genesis_state,genesis,false);check(genesis_state.to_json()==initial.to_json(),"System call ran at genesis");
 auto key=bytes(Int(1),32);Transaction tx;tx.chain_id=2;tx.to=contract;tx.gas=21000;tx.gas_price=1;auto signed_tx=SignedTransaction::sign(tx,key);auto invalid=input;invalid.transactions.push_back(signed_tx);auto rollback=initial;rollback.get(signed_tx.sender()).balance=1000000;before=rollback.to_json();rejected=false;
 try{ethereum.execute_block(rollback,invalid,false);}catch(const std::invalid_argument& e){rejected=std::string(e.what())=="transaction chain id mismatch";}
 check(rejected&&rollback.to_json()==before,"Invalid later transaction did not roll back system call");
 auto bad_header=block.header;bad_header.parent_beacon_block_root=Bytes(31);rejected=false;try{bad_header.encode();}catch(const std::invalid_argument&){rejected=true;}check(rejected,"Wrong beacon root width accepted");
 std::cout<<"Beacon header, system call, absent code, revert, genesis, native policy and block rollback passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
