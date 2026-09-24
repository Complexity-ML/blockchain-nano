#include "nano/txpool.hpp"
#include <iostream>
#include <stdexcept>
using namespace nano;
void check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
int main(){try{
 auto key=bytes(Int(1),32);auto sender=public_address(public_key(key));
 auto make=[&](unsigned nonce,unsigned fee,size_t payload){Transaction tx;tx.chain_id=1337;tx.nonce=nonce;tx.to=sender;tx.gas=100000;tx.gas_price=fee;tx.data=Bytes(payload,1);return SignedTransaction::sign(tx,key);};
 auto a=make(0,100,0),b=make(1,200,0),c=make(2,300,0);auto sa=a.raw().size(),sb=b.raw().size(),sc=c.raw().size();
 {TxPool p(10,10,1,sa+sb+sc);check(p.add(a)&&p.add(b)&&p.add(c),"fill exact byte boundary");check(p.serialized_bytes()==sa+sb+sc,"initial byte accounting");
  auto low=make(3,250,(sa+sb)/2+20);check(low.raw().size()>sa+sb&&low.raw().size()<sa+sb+sc,"multiple victim fixture size");
  check(!p.add(low)&&p.size()==3&&p.get(hex(a.hash()))&&p.get(hex(b.hash()))&&p.get(hex(c.hash()))&&p.serialized_bytes()==sa+sb+sc,"failed multi-eviction leaves all entries and bytes unchanged");
  auto high=make(3,400,(sa+sb)/2+20);check(p.add(high)&&p.size()==1&&p.serialized_bytes()==high.raw().size(),"higher fee candidate may evict several smaller entries");
  check(!p.add(high)&&p.serialized_bytes()==high.raw().size(),"duplicate does not charge bytes");p.remove_mined({high});p.remove_mined({high});check(p.serialized_bytes()==0&&p.size()==0,"mining frees bytes once");}
 {TxPool p(10,10,1,sa+sb);check(p.add(a)&&p.add(b),"replacement byte fixture");auto low=make(0,110,50);check(low.raw().size()<sa+sb&&low.raw().size()>sa,"replacement growth fixture");
  check(!p.add(low)&&p.get(hex(a.hash()))&&p.get(hex(b.hash()))&&p.serialized_bytes()==sa+sb,"rejected replacement preserves original and other entry");
  auto high=make(0,300,50);check(p.add(high)&&p.size()==1&&p.serialized_bytes()==high.raw().size(),"replacement growth accounts for old bytes and another eviction");auto small=make(0,330,0);check(p.add(small)&&p.serialized_bytes()==small.raw().size(),"smaller replacement releases excess bytes");
  check(p.remove(hex(small.hash()))&&!p.remove(hex(small.hash()))&&p.serialized_bytes()==0,"explicit removal releases bytes once");check(p.add(a),"readmission after removal");p.clear();check(p.serialized_bytes()==0&&p.size()==0,"clear resets byte accounting");}
 {TxPool p(2,10,1,10000);check(p.add(a)&&p.add(b)&&p.add(c)&&p.serialized_bytes()==sb+sc,"count eviction also updates bytes");}
 {TxPool p(2,10,1,1);check(!p.add(a)&&p.serialized_bytes()==0&&p.size()==0,"single oversized transaction refused without mutation");}
 {TxPool p(10,10,1,sa+sb);check(p.add(a)&&p.add(b),"stale nonce byte fixture");State state;state.get(sender).balance=1000000000;state.get(sender).nonce=1;p.remove_stale(state);check(p.size()==1&&!p.get(hex(a.hash()))&&p.get(hex(b.hash()))&&p.serialized_bytes()==sb,"stale cleanup frees only obsolete bytes");p.remove_stale(state);check(p.serialized_bytes()==sb,"repeated stale cleanup is idempotent");}
 {auto dynamic=[&](unsigned fee,unsigned tip){Transaction t;t.type=2;t.chain_id=1337;t.to=sender;t.max_fee=fee;t.priority_fee=tip;return SignedTransaction::sign(t,key);};TxPool p;auto original=dynamic(101,11);check(p.add(original),"dynamic replacement original");auto used=p.serialized_bytes();check(!p.add(dynamic(111,11))&&!p.add(dynamic(101,12))&&p.get(hex(original.hash()))&&p.serialized_bytes()==used,"single-cap bump rejected without mutation");check(p.add(dynamic(111,12)),"both caps meet floored threshold");}
 {TxPool p(10,10,1,sa+sb);check(p.add(a)&&p.add(b),"balance refresh byte fixture");State state;state.get(sender).balance=15000000;p.remove_stale(state);check(p.size()==1&&p.get(hex(a.hash()))&&!p.get(hex(b.hash()))&&p.serialized_bytes()==sa,"balance refresh releases only unaffordable bytes");p.remove_stale(state);check(p.serialized_bytes()==sa,"balance refresh idempotent accounting");}
 bool invalid=false;try{TxPool p(2,10,1,0);}catch(const std::invalid_argument&){invalid=true;}check(invalid,"zero byte budget rejected");
 TxPool pool;
 for(unsigned i=0;i<65;++i){Transaction tx;tx.chain_id=1337;tx.nonce=i;tx.to=sender;tx.gas=30000000;tx.gas_price=100;tx.data=Bytes(1024*1024-256,1);check(pool.add(SignedTransaction::sign(tx,key)),"large valid fixture admitted or replaces an older equal-fee entry");}
 size_t total=0,count=0;std::string after;
 do{auto page=pool.page(after);for(const auto& raw:page.at("transactions")){total+=(raw.get_ref<const std::string&>().size()-2)/2;++count;}if(page.at("next").is_null())break;after=page.at("next");}while(true);
 check(total==pool.serialized_bytes(),"accounted bytes match independent inventory serialization");
 check(total<=64*1024*1024,"default pool exceeds 64 MiB serialized transaction budget");
 std::cout<<Json{{"scenario","pool-byte-bound"},{"serializedBytes",total},{"entries",count},{"limit",64*1024*1024}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
