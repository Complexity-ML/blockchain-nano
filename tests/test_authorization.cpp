#include "nano/engine.hpp"
#include <iostream>
using namespace nano;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
Authorization authorize(const Bytes& key,const std::string& target,Int nonce=0,Int chain=1337){Authorization a;a.chain_id=chain;a.target=target;a.nonce=nonce;auto sig=sign_hash(key,a.signing_hash());a.y_parity=sig.v;a.r=sig.r;a.s=sig.s;return a;}
int main(){try{
 auto payer_key=bytes(Int(1),32),owner_key=bytes(Int(2),32);auto payer=public_address(public_key(payer_key)),owner=public_address(public_key(owner_key));auto target=address("0x0000000000000000000000000000000000001000");
 Transaction tx;tx.type=4;tx.chain_id=1337;tx.to=owner;tx.gas=100000;tx.max_fee=20;tx.priority_fee=4;tx.authorization_list={authorize(owner_key,target)};
 auto signed_tx=SignedTransaction::sign(tx,payer_key);check(SignedTransaction::decode(signed_tx.raw()).raw()==signed_tx.raw(),"Type-4 RLP round-trip failed");check(Transaction::from_json(tx.to_json()).signing_hash()==tx.signing_hash(),"Type-4 JSON round-trip failed");
 check(tx.authorization_list[0].authority()==owner,"Authorization recovered wrong authority");
 Header header;header.base_fee=10;ExecutionEngine engine(1337,{},EvmRevision::Prague);
 for(bool revert:{false,true})for(bool existing:{false,true}){
  State state;state.get(payer).balance=1000000000;if(existing)state.get(owner).balance=1;
  state.get(target).code=unhex(revert?"5f5ffd":"602a60015500");
  auto receipt=engine.apply_transaction(state,signed_tx,header,0,0);
  check(receipt.success!=revert,"Delegated execution status differs");
  check(delegation_target(state.get(owner).code)==target&&state.get(owner).nonce==1,"Authorization rolled back with execution");
  check(state.get_storage(owner,1)==(revert?0:42)&&state.get_storage(target,1)==0,"Delegated storage used wrong account");
  if(revert)check(receipt.gas_used==(existing?36804:46004),"Authorization refund lost on revert");
  check(Receipt::from_json(receipt.to_json()).encode()==receipt.encode(),"Type-4 receipt round-trip failed");
  // A later signed authorization can clear the designation, retaining storage.
  auto revoke=tx;revoke.nonce=1;revoke.authorization_list={authorize(owner_key,address("0x0000000000000000000000000000000000000000"),1)};
  check(engine.apply_transaction(state,SignedTransaction::sign(revoke,payer_key),header,0,0).success,"Revocation failed");
  check(state.get(owner).code.empty()&&state.get(owner).nonce==2&&state.get_storage(owner,1)==(revert?0:42),"Revocation changed storage or nonce incorrectly");
 }
 // The pinned Anvil memory backend represents absent accounts as existing empty
 // accounts. EIP-7702 refunds only nonempty authorities; map insertion is not funding.
 for(auto revision:{EvmRevision::Prague,EvmRevision::Osaka})for(unsigned mode=0;mode<3;++mode){
  State probe;probe.get(payer).balance=1000000000;
  if(mode==1)(void)probe.get(owner);
  if(mode==2)probe.get(owner).balance=1;
  probe.get(target).code=unhex("602a5f5d5f5c5f5260205f60205e60205160015500");
  auto receipt=ExecutionEngine(1337,{},revision).apply_transaction(probe,signed_tx,header,0,0);
  Int expected_gas=mode==2?55841:68341;
  check(receipt.success&&receipt.gas_used==expected_gas,"Empty authority incorrectly received nonempty-account refund");
  check(probe.get(payer).balance==Int(1000000000)-expected_gas*14,"Authorization fee balance mismatch");
  check(probe.get(owner).balance==(mode==2?1:0)&&probe.get(owner).nonce==1,"Authority balance or nonce changed incorrectly");
  check(probe.get_storage(owner,1)==42&&delegation_target(probe.get(owner).code)==target,"Authorization probe did not execute delegated code");
 }
 for(unsigned invalid=0;invalid<3;++invalid){
  auto malformed=tx;if(invalid==0)malformed.authorization_list[0]=authorize(owner_key,target,0,1);
  if(invalid==1)malformed.authorization_list[0]=authorize(owner_key,target,1);
  if(invalid==2){auto& a=malformed.authorization_list[0];a.s=Int("fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141",16)-a.s;a.y_parity=1-a.y_parity;}
  State state;state.get(payer).balance=1000000000;auto receipt=engine.apply_transaction(state,SignedTransaction::sign(malformed,payer_key),header,0,0);
  check(receipt.success&&state.get(owner).code.empty()&&state.get(owner).nonce==0,"Invalid authorization changed authority");
 }
 {
  State warm;warm.get(payer).balance=1000000000;
  warm.get(target).code=concat(concat(Bytes{0x73},unhex(owner)),Bytes{0x31,0x50,0x00});
  auto probe=tx;probe.to=target;
  auto receipt=engine.apply_transaction(warm,SignedTransaction::sign(probe,payer_key),header,0,0);
  check(receipt.success&&receipt.gas_used==46105,"Recovered checksum address was not warmed canonically");
 }
 State state;state.get(payer).balance=1000000000;auto before=state.to_json();bool rejected=false;
 try{ExecutionEngine(1337,{},EvmRevision::Cancun).apply_transaction(state,signed_tx,header,0,0);}catch(const std::invalid_argument& e){rejected=std::string(e.what())=="transaction type 4 unavailable before Prague";}
 check(rejected&&state.to_json()==before,"Type-4 pre-fork rejection mutated state");
 auto wrong_chain=tx;wrong_chain.chain_id=1338;wrong_chain.authorization_list.clear();rejected=false;
 try{engine.apply_transaction(state,SignedTransaction::sign(wrong_chain,payer_key),header,0,0);}catch(const std::invalid_argument& e){rejected=std::string(e.what())=="transaction chain id mismatch";}
 check(rejected&&state.to_json()==before,"Chain ID validation must precede empty authorization rejection");
 auto oversized=tx;oversized.authorization_list[0].nonce=Int(1)<<64;rejected=false;try{SignedTransaction::sign(oversized,payer_key);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"Out-of-bounds authorization nonce accepted");
 std::cout<<"Authorization signing, execution, revert persistence, refund, revocation and fork gates passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
