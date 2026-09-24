#include "nano/engine.hpp"
#include "nano/rpc.hpp"
#include "nano/staking.hpp"
#include <unistd.h>
#include <iostream>
using namespace nano;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F> void rejects(F action,const std::string& message){try{action();}catch(const std::invalid_argument& e){check(e.what()==message,"Unexpected rejection reason");return;}throw std::runtime_error("Expected rejection");}
int main(){try{
 for(auto revision:{EvmRevision::Cancun,EvmRevision::Prague,EvmRevision::Osaka}){
  const std::string probe="0x0000000000000000000000000000000000003000";
  Header h;h.excess_blob_gas=5007716;State state;state.get(probe).code=unhex("4a60005260206000f3");auto before=state.to_json();
  Int expected=revision==EvmRevision::Cancun?4:2;
  check(staking_contract_stake(state,h,probe,probe,1337,{},revision)==expected,"Staking BLOBBASEFEE context differs");
  check(state.to_json()==before,"Staking blob fee probe mutated state");
  // In-memory diagnostic profile only; production genesis parsing remains unchanged.
  auto genesis=Genesis::from_json({{"config",{{"chainId",1337}}},{"alloc",{{probe,{{"code","0x4a60005260206000f3"}}}}}});
  genesis.evm_revision=revision;genesis.block.header.withdrawals_root=keccak(Rlp(Bytes{}).encode());genesis.block.header.blob_gas_used=0;genesis.block.header.excess_blob_gas=5007716;
  auto path=std::filesystem::temp_directory_path()/("nano-blob-context-"+std::to_string(getpid())+"-"+std::to_string(int(revision)));
  {Blockchain chain(path,genesis);Rpc rpc(chain);auto original=chain.state().to_json();
   auto response=rpc.handle({{"jsonrpc","2.0"},{"id",1},{"method","eth_call"},{"params",Json::array({Json{{"to",probe}},"latest"})}});
   check(!response.contains("error")&&response.at("result")==hex(bytes(expected,32)),"RPC BLOBBASEFEE context differs");
   check(chain.state().to_json()==original,"RPC blob fee probe mutated state");}
  std::filesystem::remove_all(path);
 }
 Bytes key(32);key.back()=1;
 Transaction tx;tx.type=3;tx.chain_id=1337;tx.gas=100000;tx.max_fee=20;tx.priority_fee=4;tx.max_fee_per_blob_gas=5;
 tx.to="0x0000000000000000000000000000000000001000";tx.blob_versioned_hashes={Bytes(32,0)};tx.blob_versioned_hashes[0][0]=1;
 auto signed_tx=SignedTransaction::sign(tx,key);auto sender=signed_tx.sender();
 check(SignedTransaction::decode(signed_tx.raw()).raw()==signed_tx.raw(),"Blob signed envelope round-trip failed");
 check(Transaction::from_json(tx.to_json()).signing_hash()==tx.signing_hash(),"Blob JSON round-trip failed");
 Header header;header.base_fee=10;header.withdrawals_root=keccak(Rlp(Bytes{}).encode());header.blob_gas_used=131072;header.excess_blob_gas=0;
 check(Header::from_json(header.to_json()).encode()==header.encode(),"Blob header round-trip failed");
 check(blob_base_fee(0)==1&&blob_base_fee(3338477)==2,"Blob fee exponential differs");
 ExecutionEngine engine(1337,{},EvmRevision::Cancun);
 for(bool revert:{false,true}){
  State state;Int initial=1000000000;state.get(sender).balance=initial;
  state.get(*tx.to).code=unhex(revert?"5f5ffd":"5f496001554a60025500");
  auto receipt=engine.apply_transaction(state,signed_tx,header,0,0);
  check(receipt.success!=revert,"Wrong blob transaction status");
  check(state.get(sender).balance==initial-receipt.gas_used*14-131072,"Blob fee burn/refund differs");
  check(state.get(header.coinbase).balance==receipt.gas_used*4,"Blob fees paid to producer");
  if(!revert){check(state.get_storage(*tx.to,1)==integer(tx.blob_versioned_hashes[0]),"BLOBHASH differs");check(state.get_storage(*tx.to,2)==1,"BLOBBASEFEE differs");}
  check(Receipt::from_json(receipt.to_json()).encode()==receipt.encode(),"Blob receipt round-trip failed");
 }
 State state;state.get(sender).balance=1000000000;auto before=state.to_json();
 rejects([&]{ExecutionEngine(1337,{},EvmRevision::Shanghai).apply_transaction(state,signed_tx,header,0,0);},"transaction type 3 unavailable before Cancun");
 check(state.to_json()==before,"Pre-Cancun rejection mutated state");
 tx.blob_versioned_hashes.resize(4,tx.blob_versioned_hashes[0]);tx.nonce=0;auto first=SignedTransaction::sign(tx,key);tx.nonce=1;auto second=SignedTransaction::sign(tx,key);
 Block block;block.header=header;block.transactions={first,second};auto original=block.to_json(true);
 rejects([&]{engine.execute_block(state,block,false);},"block blob gas limit exceeded");
 check(state.to_json()==before&&block.to_json(true)==original,"Blob block rejection was not atomic");
 block.transactions={first};auto receipts=engine.execute_block(state,block,false);
 check(receipts.size()==1&&block.header.blob_gas_used==Int(4*131072),"Block blob gas total differs");
 // The same signed eight-blob block is invalid in Cancun and valid in Prague.
 State prague_state;prague_state.get(sender).balance=1000000000;
 block.transactions={first,second};ExecutionEngine prague(1337,{},EvmRevision::Prague);
 auto prague_receipts=prague.execute_block(prague_state,block,false);
 check(prague_receipts.size()==2&&block.header.blob_gas_used==Int(8*131072),"Prague eight-blob block failed");
 check(blob_base_fee(5007716,EvmRevision::Prague)==2&&blob_base_fee(5007716,EvmRevision::Cancun)==4,"Fork-specific blob fee differs");
 for(unsigned count:{9u,10u}){
  State fresh;fresh.get(sender).balance=1000000000;auto initial=fresh.to_json();
  tx.nonce=0;tx.blob_versioned_hashes.resize(count,tx.blob_versioned_hashes[0]);
  auto signed_blob=SignedTransaction::sign(tx,key);
  if(count==9)check(prague.apply_transaction(fresh,signed_blob,header,0,0).success,"Prague nine-blob transaction rejected");
  else{rejects([&]{prague.apply_transaction(fresh,signed_blob,header,0,0);},"blob transaction exceeds block blob limit");check(fresh.to_json()==initial,"Prague blob rejection mutated state");}
 }
 // Parent excess accounting is fork-versioned and checked before state mutation.
 for(auto revision:{EvmRevision::Cancun,EvmRevision::Prague})for(int offset:{-1,0,1}){
  Int target=revision==EvmRevision::Cancun?393216:786432;Header parent;parent.withdrawals_root=withdrawals_root(Json::array());parent.base_fee=10;parent.gas_used=parent.gas_limit/2;parent.blob_gas_used=0;parent.excess_blob_gas=target+offset;
  Block empty;empty.header.base_fee=10;empty.header.number=1;empty.header.parent_hash=parent.hash();empty.header.withdrawals_root=withdrawals_root(Json::array());empty.header.blob_gas_used=0;empty.header.excess_blob_gas=offset>0?1:0;
  ExecutionEngine versioned(1337,{},revision);State expected;versioned.execute_block(expected,empty,false);State replay;versioned.verify_block(replay,empty,parent,false);check(replay.root()==expected.root(),"Parent excess boundary replay differs");
  auto bad=empty;*bad.header.excess_blob_gas+=1;State unchanged;auto before_bad=unchanged.to_json();rejects([&]{versioned.verify_block(unchanged,bad,parent,false);},"incorrect excess blob gas");check(unchanged.to_json()==before_bad,"Excess rejection mutated state");
  bad=empty;bad.header.blob_gas_used=131072;rejects([&]{versioned.verify_block(unchanged,bad,parent,false);},"incorrect blob gas used");check(unchanged.to_json()==before_bad,"Blob gas commitment rejection mutated state");
 }
 // Floor gas is charged after execution/refunds; it is not subtracted from the EVM budget.
 Transaction data_tx;data_tx.chain_id=1337;data_tx.to=tx.to;data_tx.data=Bytes(100,1);data_tx.gas_price=20;data_tx.gas=25000;
 for(auto revision:{EvmRevision::Cancun,EvmRevision::Prague}){
  State fresh;fresh.get(sender).balance=1000000000;fresh.get(*tx.to).code=unhex("602a60005260206000f3");
  auto result=ExecutionEngine(1337,{},revision).apply_transaction(fresh,SignedTransaction::sign(data_tx,key),header,0,0);
  check(result.success,"Calldata floor consumed execution budget");
  check(result.gas_used==(revision==EvmRevision::Prague?25000:22618),"Calldata floor fee differs across forks");
 }
 data_tx.gas=24999;State underfunded;underfunded.get(sender).balance=1000000000;auto initial=underfunded.to_json();
 rejects([&]{prague.apply_transaction(underfunded,SignedTransaction::sign(data_tx,key),header,0,0);},"transaction gas below calldata floor");
 check(underfunded.to_json()==initial,"Calldata floor rejection mutated state");
 std::cout<<"Blob encoding, fees, revert, pre-fork rejection and block limit passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
