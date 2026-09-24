#include "support/process.hpp"
using namespace test;
int main(){try{
 auto key=bytes(Int(1),32);auto sender=public_address(public_key(key));
 auto make=[&](unsigned nonce,unsigned fee){Transaction tx;tx.chain_id=1337;tx.nonce=nonce;tx.to=sender;tx.gas_price=fee;return SignedTransaction::sign(tx,key);};
 auto a=make(0,100),b=make(1,200),c=make(2,300),d=make(3,400);
 {TxPool pool(2,10);check(pool.add(a)&&pool.add(b)&&pool.add(c),"bounded pool populated");check(!pool.get(hex(a.hash())),"lowest fee evicted");check(!pool.add(a)&&pool.get(hex(b.hash()))&&pool.get(hex(c.hash())),"lower fee readmission cannot evict better transactions from full pool");check(pool.remove(hex(b.hash()))&&pool.add(a),"evicted transaction can return when room is available");check(!pool.add(a),"active duplicate remains rejected");}
 {TxPool pool(2,10);auto replacement=make(0,110);check(pool.add(a)&&pool.add(replacement),"replacement admitted");check(!pool.add(a),"fee rule rejects older replacement while current transaction exists");check(pool.remove(hex(replacement.hash()))&&pool.add(a),"replaced transaction may return after removal");}
 {TxPool pool(2,10);check(pool.add(a),"first mined fixture");pool.remove_mined({a});check(!pool.add(a),"recent mined duplicate rejected");pool.remove_mined({b,c});check(pool.add(a),"mined cache forgets oldest after capacity");check(!pool.add(b)&&!pool.add(c),"recent mined cache entries retained");
  for(unsigned i=0;i<1000;++i)pool.remove_mined({b});pool.remove_mined({d});check(pool.add(b),"repeated mined input cannot inflate or refresh FIFO history");check(!pool.add(a),"history eviction never forgets active pool entries");pool.clear();check(pool.add(c),"clear resets bounded history");}
 // Persistent canonical validation must remain authoritative after cache reset.
 Temp root;auto genesis=Genesis::from_json({{"config",{{"chainId",1337}}},{"alloc",{{sender,{{"balance","0x100000000000000000"}}}}}});
 {Blockchain chain(root/"chain",genesis);auto tx=make(0,2000000000);check(chain.submit(tx),"canonical fixture submitted");auto block=chain.mine(sender,24);auto state=chain.state().root();check(chain.restore(block.header.number,block.header.hash()),"same-tip cache reset");bool rejected=false;try{chain.submit(tx);}catch(const std::invalid_argument& e){rejected=std::string(e.what())=="transaction nonce already mined";}check(rejected&&chain.pending_count()==0&&chain.state().root()==state,"persistent nonce rejects mined replay after cache reset");check(chain.audit().at("verified")==true,"canonical history valid after cache reset");}
 std::cout<<"Eviction readmission, bounded mined history, duplicates and canonical replay checks passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
