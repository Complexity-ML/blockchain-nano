#include "support/process.hpp"
using namespace test;
int main(){try{
 const auto key=bytes(Int(1),32);const auto sender=public_address(public_key(key));
 auto make=[&](unsigned nonce,unsigned fee){Transaction tx;tx.chain_id=1337;tx.nonce=nonce;tx.to=sender;tx.gas_price=fee;return SignedTransaction::sign(tx,key);};
 const auto a=make(0,100),b=make(1,200),c=make(2,300),expensive=make(3,1000000);
 const auto state=State::from_json({{sender,{{"balance","0x10000000"}}}});
 // Preparation must not publish if a disk commit is cancelled or fails.
 {TxPool pool(2,10);check(pool.add(a)&&pool.add(b),"seed rollback");const auto bytes=pool.serialized_bytes();
  {auto discard=pool.prepare_canonical_change(state,{a},{c});}
  check(pool.get(hex(a.hash()))&&pool.get(hex(b.hash()))&&!pool.get(hex(c.hash()))&&pool.serialized_bytes()==bytes,"discard preserves entries and bytes");
  {auto publish=pool.prepare_canonical_change(state,{a},{c});publish();publish();}
  check(!pool.get(hex(a.hash()))&&pool.get(hex(b.hash()))&&pool.get(hex(c.hash())),"publish once removes included and restores orphan");
  check(!pool.add(a),"published mined cache rejects duplicate");
 }
 // A transaction present in both branches must never evict pending entries.
 {TxPool pool(2,10);check(pool.add(a)&&pool.add(b),"seed overlap");pool.remove_mined({c});
  {auto publish=pool.prepare_canonical_change(state,{c},{c});publish();}
  check(pool.get(hex(a.hash()))&&pool.get(hex(b.hash()))&&!pool.get(hex(c.hash())),"overlapping mined transaction cannot evict pending");
 }
 // An expensive orphan invalid under the new state must not evict then vanish.
 {TxPool pool(2,10);check(pool.add(a)&&pool.add(b),"seed unaffordable");
  {auto publish=pool.prepare_canonical_change(state,{}, {expensive});publish();}
  check(pool.get(hex(a.hash()))&&pool.get(hex(b.hash()))&&!pool.get(hex(expensive.hash())),"unaffordable orphan leaves useful entries intact");
 }
 // Reorg removes the old mined marker, allowing a valid orphan to re-enter.
 {TxPool pool(2,10);pool.remove_mined({a});check(!pool.add(a),"old branch mined marker");
  {auto publish=pool.prepare_canonical_change(state,{}, {a});publish();}
  check(pool.get(hex(a.hash())).has_value(),"orphan restored despite previous mined marker");
 }
 // Reorg restoration or hash-ordered P2P reception can put a nonce prefix last.
 // The former Capella truncation then spent every trial on future nonces.
 {TxPool pool(128,128);for(unsigned nonce=1;nonce<96;++nonce)check(pool.add(make(nonce,100)),"future nonce admission");check(pool.add(a),"late nonce zero");
  auto legacy=pool.pending_for_block(30000000);legacy.resize(16);check(std::none_of(legacy.begin(),legacy.end(),[](const auto& tx){return tx.tx.nonce==0;}),"counterexample no longer reproduces old prefix selection");
  auto ready=pool.ready_for_block(state,30000000,16);check(ready.size()==16,"ready trial count");for(unsigned i=0;i<16;++i)check(ready[i].tx.nonce==i,"ready nonce dependency order");
  check(pool.size()==96,"candidate selection mutated pool");
  bool cancelled=false;try{pool.ready_for_block(state,30000000,16,[]{throw std::runtime_error("cancelled");});}catch(const std::runtime_error&){cancelled=true;}check(cancelled&&pool.size()==96,"cancelled selection mutated pool");
  auto advanced=state;advanced.get(sender).nonce=16;ready=pool.ready_for_block(advanced,30000000,16);check(ready.size()==16&&ready.front().tx.nonce==16&&ready.back().tx.nonce==31,"canonical nonce advancement ignored");
 }
 {TxPool pool(128,128);for(unsigned nonce=16;nonce<32;++nonce)check(pool.add(make(nonce,100)),"seed reorg pending");
  std::vector<SignedTransaction> orphaned;for(unsigned nonce=0;nonce<16;++nonce)orphaned.push_back(make(nonce,100));pool.remove_mined(orphaned);
  {auto publish=pool.prepare_canonical_change(state,{},orphaned);publish();}
  auto ready=pool.ready_for_block(state,30000000,16);check(ready.size()==16&&ready.front().tx.nonce==0&&ready.back().tx.nonce==15,"restored orphans starved by future entries");
 }
 {TxPool pool;check(pool.add(a)&&pool.add(c),"nonce gap seed");auto ready=pool.ready_for_block(state,30000000,16);check(ready.size()==1&&ready[0].tx.nonce==0,"nonce gap crossed");check(pool.ready_for_block(state,20999,16).empty(),"gas budget bypassed");}
 {const auto other_key=bytes(Int(2),32);const auto other_sender=public_address(public_key(other_key));auto funded=state;funded.get(other_sender).balance=100000000;
  Transaction transaction;transaction.chain_id=1337;transaction.to=sender;transaction.gas_price=150;const auto other=SignedTransaction::sign(transaction,other_key);
  TxPool pool;check(pool.add(b)&&pool.add(other)&&pool.add(a),"multi-account seed");auto ready=pool.ready_for_block(funded,30000000,3);
  check(ready.size()==3&&ready[0].hash()==other.hash()&&ready[1].hash()==a.hash()&&ready[2].hash()==b.hash(),"fee priority bypassed sender dependencies");
  auto large=a.tx;large.gas=50000;TxPool constrained;check(constrained.add(SignedTransaction::sign(large,key))&&constrained.add(b)&&constrained.add(other),"gas constrained seed");
  ready=constrained.ready_for_block(funded,21000,16);check(ready.size()==1&&ready[0].hash()==other.hash(),"oversized account head blocked unrelated account or advanced its own nonce");
 }
 std::cout<<"Canonical pool rollback, orphan restoration and bounded nonce-ready selection checks passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
