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
 std::cout<<"Canonical pool rollback, publication, branch overlap and orphan policy checks passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
