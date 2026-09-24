#include "support/process.hpp"
#include "nano/http.hpp"
using namespace test;
struct Running{HttpServer& server;std::thread thread;Running(HttpServer& s):server(s),thread([&s]{s.run();}){}~Running(){server.stop();thread.join();}};
int main(int argc,char** argv){try{
 check(argc==2,"relay binary required");Temp root;auto secret=bytes(Int(1),32);auto sender=public_address(public_key(secret));
 auto genesis=Genesis::from_json({{"config",{{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bootstrapValidator",sender},{"evmFork","Shanghai"}}},{"alloc",{{sender,{{"balance","0x3635c9adc5dea00000"}}}}}});
 Blockchain left(root/"a",genesis),right(root/"b",genesis);Rpc arpc(left),brpc(right);HttpServer as(arpc,"127.0.0.1",0),bs(brpc,"127.0.0.1",0);Running ra(as),rb(bs);
 Transaction tx;tx.chain_id=1337;tx.to=sender;tx.gas_price=2000000000;auto low=SignedTransaction::sign(tx,secret);tx.gas_price=2100000000;auto high=SignedTransaction::sign(tx,secret);
 check(left.submit(low)&&right.submit(high),"competing transactions admitted locally");
 Env trace={{"NANO_TEST_NETWORK_TRACE","1"}};
 Process pa({argv[1],std::to_string(as.port()),(root/"a.key").string(),"/ip4/127.0.0.1/tcp/0"},trace);auto addr=pa.event("listen")["address"].get<std::string>();pa.event("ready");
 Process pb({argv[1],std::to_string(bs.port()),(root/"b.key").string(),"/ip4/127.0.0.1/tcp/0",addr},trace);pb.event("listen");pb.event("ready");
 auto refusal=pa.event("test-gossip-refused");pb.event("test-gossip-refused");check(left.pending_transaction(low.hash())&&!right.pending_transaction(low.hash())&&right.pending_transaction(high.hash()),"real replacement policy rejects both directions");
 // Controlled fixture withdrawal: same canonical tip and core session, no
 // production mutation RPC, no restart and no new source submission.
 auto before=rpc(bs.port(),"nano_networkStatus");auto tip=right.tip();check(right.restore(tip.header.number,tip.header.hash()),"controlled destination pool clear");check(right.pending_count()==0,"destination obstacle removed");auto begin=std::chrono::steady_clock::now();
 until([&]{return right.pending_transaction(low.hash()).has_value();},"Previously refused transaction recovered by periodic inventory",90);
 auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();check(elapsed>=50,"recovery used delayed reconciliation, not immediate retry");
 check(rpc(bs.port(),"nano_networkStatus")["gossip_epoch"]==before["gossip_epoch"],"no epoch reset substituted for timer");check(left.pending_count()==1&&right.pending_count()==1&&!right.pending_transaction(high.hash()),"exact final pools");check(left.tip().header.number==0&&right.tip().header.number==0&&pa.running()&&pb.running(),"no block or process restart substituted for retry");
 std::cout<<Json{{"scenario","temporary-replacement-refusal-recovery"},{"elapsedSeconds",elapsed},{"refusal",refusal},{"transactionHash",hex(low.hash())},{"sameCoreSession",true},{"controlledPoolClear",true}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
