#include "support/process.hpp"
#include "nano/rpc.hpp"
using namespace test;
int main(){try{
 Temp root;const auto secret=bytes(Int(1),32);const auto sender=public_address(public_key(secret));
 auto genesis=Genesis::from_json({{"config",{{"chainId",1337}}},{"alloc",{{sender,{{"balance","1000000000000000000000000"}}}}}});
 Blockchain chain(root/"db",genesis);auto now=AdmissionBudget::Clock::time_point{};
 Rpc rpc(chain,{},[&]{return now;});const auto original=chain.state().root();const auto cursor=chain.event_cursor();
 unsigned id=0,foreign=0;auto send_raw=[&](const Bytes& raw){return rpc.handle(Json{{"jsonrpc","2.0"},{"id",++id},{"method","eth_sendRawTransaction"},{"params",Json::array({hex(raw)})}});};
 auto send=[&](const SignedTransaction& tx){return send_raw(tx.raw());};
 Transaction tx;tx.to="0x00000000000000000000000000000000000000ab";tx.gas=21000;tx.gas_price=2000000000;tx.max_fee=2000000000;tx.priority_fee=1000000000;
 for(unsigned type=0;type<3;++type)for(unsigned nonce=0;nonce<128;++nonce){
  tx.type=type;tx.nonce=nonce;tx.chain_id=7331;auto result=send(SignedTransaction::sign(tx,secret));
  check(result.contains("error")&&result.at("error").at("message")=="wrong chain ID","foreign chain consumed local admission budget");++foreign;
 }
 auto nested_raw=[&](const Int& chain_id){auto typed=tx;typed.type=1;typed.nonce=0;typed.chain_id=chain_id;auto raw=SignedTransaction::sign(typed,secret).raw();auto fields=Rlp::decode(Bytes(raw.begin()+1,raw.end()));Rlp nested(Bytes{});for(unsigned depth=0;depth<70;++depth)nested=Rlp(std::vector<Rlp>{std::move(nested)});fields.items[7]=std::move(nested);auto encoded=fields.encode();encoded.insert(encoded.begin(),1);return encoded;};
 const auto foreign_nested=nested_raw(7331);
 for(unsigned i=0;i<128;++i){auto result=send_raw(foreign_nested);check(result.contains("error")&&result.at("error").at("message")=="wrong chain ID","foreign nested payload reached the recursive decoder before admission");++foreign;}
 check(chain.state().root()==original&&chain.event_cursor()==cursor&&chain.pending_count()==0,"foreign envelopes changed canonical state events or pool");
 tx.type=0;tx.nonce=0;tx.chain_id=1337;auto healthy=SignedTransaction::sign(tx,secret);auto admitted=send(healthy);
 check(admitted.contains("result")&&admitted.at("result")==hex(healthy.hash())&&chain.pending_count()==1,"healthy transaction starved by foreign envelopes");
 auto bad=healthy;bad.s=Int("fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364140",16);
 for(unsigned i=0;i<63;++i){auto result=send(bad);check(result.contains("error")&&result.at("error").at("message")=="noncanonical high-s signature","local signature verification budget was bypassed");}
 const auto local_nested=nested_raw(1337);
 for(unsigned i=0;i<64;++i){auto result=send_raw(local_nested);check(result.contains("error")&&result.at("error").at("message")=="invalid RLP nesting/length","local nested payload bypassed canonical decoding");}
 tx.nonce=1;auto second=SignedTransaction::sign(tx,secret);auto limited=send(second);
 check(limited.contains("error")&&limited.at("error").at("code")==-32005,"local signature work escaped the admission bound");
 check(chain.state().root()==original&&chain.pending_count()==1,"invalid signatures or rate refusal mutated canonical state or pool");
 now+=1s;check(send(second).contains("result")&&chain.pending_count()==2,"local admission did not recover after refill");
 // Unknown envelopes are bounded local work, including failures before the
 // full decoder. A foreign envelope remains distinguishable at exhaustion.
 Blockchain malformed_chain(root/"malformed",genesis);auto malformed_now=AdmissionBudget::Clock::time_point{};
 Rpc malformed(malformed_chain,{},[&]{return malformed_now;});
 auto malformed_send=[&](const std::string& raw){return malformed.handle(Json{{"jsonrpc","2.0"},{"id",++id},{"method","eth_sendRawTransaction"},{"params",Json::array({raw})}});};
 for(const auto& raw:{"0xzz","0x00","0x02f8ff","0xc0"})for(unsigned i=0;i<32;++i){
  auto response=malformed_send(raw);check(response.contains("error")&&response.at("error").at("code")!=-32005,"malformed burst refused before its bounded capacity");
 }
 check(malformed_send(hex(healthy.raw())).at("error").at("code")==-32005,"malformed raw envelopes escaped local admission budget");
 check(malformed_send(hex(foreign_nested)).at("error").at("message")=="wrong chain ID","exhausted malformed budget rejected proven foreign envelope");
 check(malformed_send("0xzz").at("error").at("code")==-32005,"malformed hex bypasses exhausted local admission");
 check(malformed_chain.state().root()==original&&malformed_chain.pending_count()==0&&malformed_chain.event_cursor()==0,"malformed admission changed canonical state events or pool");
 malformed_now+=1s;check(malformed_send(hex(healthy.raw())).contains("result")&&malformed_chain.pending_count()==1,"malformed raw admission did not recover on refill");
 // Gossip must reserve the entire local portion before changing the pool.
 // Foreign items preserve their result positions without spending local work.
 Blockchain gossip_chain(root/"gossip",genesis);auto gossip_now=AdmissionBudget::Clock::time_point{};
 Rpc gossip(gossip_chain,{},[&]{return gossip_now;});
 auto receive=[&](const Json& batch){return gossip.handle(Json{{"jsonrpc","2.0"},{"id",++id},{"method","nano_receiveTransactions"},{"params",Json::array({batch})}});};
 tx.nonce=0;tx.chain_id=7331;auto foreign_raw=hex(SignedTransaction::sign(tx,secret).raw());
 Json foreign_batch=Json::array();for(unsigned i=0;i<50;++i)foreign_batch.push_back(i%2?hex(foreign_nested):foreign_raw);
 for(unsigned i=0;i<10;++i){auto result=receive(foreign_batch);check(result.contains("result")&&result.at("result").at("accepted")==Json(std::vector<bool>(50,false)),"foreign gossip consumed local admission budget");}
 check(gossip_chain.pending_count()==0&&gossip_chain.event_cursor()==0&&gossip_chain.state().root()==original,"foreign gossip mutated canonical state or events");
 check(receive(Json::array({hex(healthy.raw()),7})).contains("error")&&gossip_chain.pending_count()==0,"mixed gossip shape partially admitted");
 auto mixed=receive(Json::array({foreign_raw,hex(healthy.raw()),hex(foreign_nested),hex(bad.raw()),hex(local_nested)}));
 check(mixed.contains("result")&&mixed.at("result").at("processed")==5&&mixed.at("result").at("accepted")==Json::array({false,true,false,false,false}),"mixed gossip lost order or local validation");
 for(unsigned i=0;i<124;++i){auto result=receive(Json::array({hex(bad.raw())}));check(result.contains("result")&&result.at("result").at("accepted")==Json::array({false}),"invalid local gossip bypassed bounded signature work");}
 tx.chain_id=1337;tx.nonce=2;auto third=SignedTransaction::sign(tx,secret);auto before=gossip_chain.event_cursor();
 auto refused=receive(Json::array({foreign_raw,hex(second.raw()),hex(third.raw())}));
 check(refused.contains("error")&&refused.at("error").at("code")==-32005&&gossip_chain.pending_count()==1&&gossip_chain.event_cursor()==before,"insufficient gossip batch budget partially mutated pool or events");
 check(receive(Json::array({hex(second.raw())})).at("result").at("accepted")==Json::array({true}),"rejected gossip reservation consumed remaining local capacity");
 check(receive(foreign_batch).contains("result"),"foreign gossip rejected by exhausted local budget");
 check(receive(Json::array({"0x00"})).at("error").at("code")==-32005,"malformed gossip escaped local budget");
 gossip_now+=1s;check(receive(Json::array({hex(third.raw())})).at("result").at("accepted")==Json::array({true})&&gossip_chain.pending_count()==3&&gossip_chain.state().root()==original,"gossip refill or canonical isolation failed");
 std::cout<<Json{{"foreignRejected",foreign},{"foreignNestedRejectedBeforeDecode",128},{"localNestedRejectedWithinBudget",64},{"localHighSRejectedWithinBudget",63},{"healthyAdmissionPreserved",true},{"localSignatureBudgetBounded",true},{"refillVerified",true},{"canonicalStateUnchanged",chain.state().root()==original},{"foreignGossipRejected",550},{"mixedGossipOrderPreserved",true},{"gossipReservationAtomic",true},{"malformedGossipBudgetBounded",true},{"gossipRefillVerified",true}}.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
