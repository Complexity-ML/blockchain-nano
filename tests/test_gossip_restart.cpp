#include "support/process.hpp"
using namespace test;
int main(int argc,char** argv){try{
 check(argc==3||(argc==4&&std::string(argv[3])=="--live-pages"),"core and relay required");bool live=argc==4;unsigned count=live?75:1;Temp root;std::string core=argv[1],network=argv[2];
 auto secret=bytes(Int(1),32);auto sender=public_address(public_key(secret));
 auto genesis=root/"genesis.json";save(genesis,{{"config",{{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bootstrapValidator",sender},{"evmFork","Shanghai"}}},{"alloc",{{sender,{{"balance","0x3635c9adc5dea00000"}}}}}});
 Process left({core,"serve",genesis.string(),(root/"a").string(),"0"}),right({core,"serve",genesis.string(),(root/"b").string(),"0"});int a=left.port(),b=right.port();
 Env trace={{"NANO_TEST_NETWORK_TRACE","1"}};
 Process pa({network,std::to_string(a),(root/"a.key").string(),"/ip4/127.0.0.1/tcp/0"},trace);auto addr=pa.event("listen")["address"].get<std::string>();pa.event("ready");
 Process pb({network,std::to_string(b),(root/"b.key").string(),"/ip4/127.0.0.1/tcp/0",addr},trace);auto baddr=pb.event("listen")["address"].get<std::string>();pb.event("ready");pa.event("peer");pb.event("peer");
 std::vector<std::string> hashes;for(unsigned n=0;n<count;++n){Transaction tx;tx.chain_id=1337;tx.gas_price=2000000000;tx.to=sender;tx.nonce=n;auto signed_tx=SignedTransaction::sign(tx,secret);auto hash=hex(signed_tx.hash());check(rpc(a,"eth_sendRawTransaction",{hex(signed_tx.raw())})==hash,"admitted hash");hashes.push_back(hash);}
 until([&]{return rpc(b,"txpool_status")["pending"]==quantity(Int(count));},"Initial gossip delivered",45);
 auto old=rpc(b,"nano_networkStatus")["gossip_epoch"];
 // Freeze both relays only during restart to observe the empty volatile pool
 // before retransmission, retaining the exact relay processes and identities.
 if(!live){pa.signal(SIGSTOP);pb.signal(SIGSTOP);}right.signal(SIGKILL);check(right.wait().code==-SIGKILL,"core crash confirmed");
 Process restarted({core,"serve",genesis.string(),(root/"b").string(),std::to_string(b)});check(restarted.port()==b,"same RPC endpoint");auto fresh=rpc(b,"nano_networkStatus")["gossip_epoch"];check(old!=fresh,"new process session");if(!live)check(rpc(b,"txpool_status")["pending"]=="0x0","pool empty before relay resume");
 if(!live){pa.signal(SIGCONT);pb.signal(SIGCONT);}
 until([&]{return rpc(b,"txpool_status")["pending"]==quantity(Int(count));},"Pending transactions retransmitted after core restart",45);
 auto reset=pa.event("test-gossip-remote-session-reset");check(reset["oldEpoch"]==old&&reset["newEpoch"]==fresh,"observed remote epoch transition");check(reset["peerId"]==baddr.substr(baddr.rfind("/p2p/")+5),"same destination P2P identity");
 check(pa.running()&&pb.running(),"original relay processes survived");check(rpc(a,"eth_blockNumber")=="0x0"&&rpc(b,"eth_blockNumber")=="0x0","no block inclusion substituted for pending recovery");
 check(rpc(b,"txpool_status")["pending"]==quantity(Int(count)),"exact pending count after replay");
 for(const auto& hash:hashes){auto tx=rpc(b,"eth_getTransactionByHash",{hash});check(!tx.is_null()&&tx["hash"]==hash&&tx["blockHash"].is_null(),"every original transaction recovered pending");}
 std::cout<<Json{{"scenario","remote-core-restart-same-relays"},{"transactions",count},{"reset",reset},{"pendingRecovered",true},{"relayPauseDuringRestart",!live}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
