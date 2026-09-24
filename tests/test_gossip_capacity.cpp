#include "support/process.hpp"
#include "nano/http.hpp"
using namespace test;
struct Running{HttpServer& server;std::thread thread;Running(HttpServer& s):server(s),thread([&s]{s.run();}){}~Running(){server.stop();thread.join();}};
int main(int argc,char** argv){try{
 check(argc==2,"relay binary required");Temp root;auto secret=bytes(Int(1),32);auto sender=public_address(public_key(secret));
 auto genesis=Genesis::from_json({{"config",{{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bootstrapValidator",sender},{"evmFork","Shanghai"}}},{"alloc",{{sender,{{"balance","0x3635c9adc5dea00000"}}}}}});
 Blockchain left(root/"a",genesis),right(root/"b",genesis);Rpc arpc(left),brpc(right);HttpServer as(arpc,"127.0.0.1",0),bs(brpc,"127.0.0.1",0);Running ra(as),rb(bs);
 Transaction tx;tx.chain_id=1337;tx.to=sender;tx.gas_price=2000000000;
 for(unsigned n=0;n<100;++n){tx.nonce=n;check(right.submit(SignedTransaction::sign(tx,secret)),"fill destination account capacity");}
 tx.nonce=100;auto target=SignedTransaction::sign(tx,secret);check(left.submit(target),"retain future transaction at source");check(!right.submit(target)&&right.pending_count()==100,"real account capacity rejects target");
 Env trace={{"NANO_TEST_NETWORK_TRACE","1"}};
 Process pa({argv[1],std::to_string(as.port()),(root/"a.key").string(),"/ip4/127.0.0.1/tcp/0"},trace);auto addr=pa.event("listen")["address"].get<std::string>();pa.event("ready");
 Process pb({argv[1],std::to_string(bs.port()),(root/"b.key").string(),"/ip4/127.0.0.1/tcp/0",addr},trace);pb.event("listen");pb.event("ready");
 auto refusal=pa.event("test-gossip-refused");check(left.pending_transaction(target.hash())&&!right.pending_transaction(target.hash()),"gossip refusal observed with source retaining target");
 auto epoch=right.event_epoch();auto first=right.validate_signed(secret,24);check(first.transactions.size()==100&&right.pending_count()==0&&right.state().get(sender).nonce==100,"signed block naturally releases account capacity");auto begin=std::chrono::steady_clock::now();
 until([&]{return left.tip().header.hash()==first.header.hash();},"Capacity-releasing block propagated");
 until([&]{return right.pending_transaction(target.hash()).has_value();},"Capacity-refused transaction recovered",90);
 auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();check(elapsed>=45,"recovery waits for periodic inventory");check(right.event_epoch()==epoch&&left.pending_count()==1&&right.pending_count()==1,"same session and exact pending sets");
 auto second=right.validate_signed(secret,48);check(second.transactions.size()==1&&second.transactions[0].hash()==target.hash(),"recovered target executes");until([&]{return left.tip().header.hash()==second.header.hash();},"Recovered transaction block propagated");
 check(left.state().root()==right.state().root()&&left.pending_count()==0&&right.pending_count()==0&&left.audit()["verified"]==true&&right.audit()["verified"]==true,"canonical agreement and audited histories");
 check(left.store().receipt(target.hash())->to_json()==right.store().receipt(target.hash())->to_json(),"recovered transaction receipts agree");
 std::cout<<Json{{"scenario","capacity-rejection-natural-recovery"},{"elapsedSeconds",elapsed},{"refusal",refusal},{"transactionHash",hex(target.hash())},{"transactionsInFirstBlock",100},{"blocks",2},{"sameCoreSession",true},{"controlledPoolClear",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
