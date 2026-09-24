#include "support/process.hpp"
#include "nano/http.hpp"
#include "nano/consensus_round.hpp"
#include "nano/block_auth.hpp"
#include "../network/block_reference.hpp"
using namespace test;
struct Running {HttpServer& server;std::thread thread;explicit Running(HttpServer& s):server(s),thread([&s]{s.run();}){}~Running(){server.stop();thread.join();}};
int main(int argc,char** argv){try{
 check(argc==2,"relay binary required");Temp root;auto private_key=bytes(Int(1),32);auto signer=public_address(public_key(private_key));
 std::string fork=std::getenv("NANO_TEST_EVM_FORK")?std::getenv("NANO_TEST_EVM_FORK"):"Paris";check(fork=="Paris"||fork=="Shanghai","explicit fork");
 Json alloc={{signer,{{"balance","0x100000000000000000"}}}};std::vector<Bytes> event_keys;for(unsigned k=10;k<110;++k){event_keys.push_back(bytes(Int(k),32));alloc[public_address(public_key(event_keys.back()))]={{"balance","0x100000000000000000"}};}
 auto genesis=Genesis::from_json({{"gasLimit","0x2625a00"},{"config",{{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bftVersion",1},{"bootstrapValidator",signer},{"evmFork",fork}}},{"alloc",alloc}});
 Bytes target_hash;std::string old_epoch;size_t compact=0,page_bytes=0;unsigned rejected=0;Json sent,received;
 {
 Blockchain source(root/"source",genesis),follower(root/"follower",genesis);ChainStore journal(root/"journal");Rpc source_rpc(source,"event-source"),follower_rpc(follower,"event-follower");
 auto call=[&](const std::string& method,Json params){return source_rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",std::move(params)}});};
 {
 HttpServer source_server(source_rpc,"127.0.0.1",0),follower_server(follower_rpc,"127.0.0.1",0);Running source_running(source_server),follower_running(follower_server);
 Process source_peer({argv[1],std::to_string(source_server.port()),(root/"source-peer").string(),"/ip4/127.0.0.1/tcp/0"},Env{{"NANO_NETWORK_TOKEN","event-source"},{"NANO_TEST_NETWORK_TRACE","1"}});auto address=source_peer.event("listen").at("address").get<std::string>();source_peer.event("ready");
 Process follower_peer({argv[1],std::to_string(follower_server.port()),(root/"follower-peer").string(),"/ip4/127.0.0.1/tcp/0",address},Env{{"NANO_NETWORK_TOKEN","event-follower"},{"NANO_TEST_NETWORK_TRACE","1"}});follower_peer.event("ready");source_peer.event("peer");follower_peer.event("peer");
 for(unsigned n=0;n<10;++n){Transaction tx;tx.chain_id=1337;tx.nonce=n;tx.to="0x00000000000000000000000000000000000000b0";tx.value=1;tx.gas=4000000;tx.gas_price=2000000000;tx.data=Bytes(900000,0);check(source.submit(SignedTransaction::sign(tx,private_key)),"large transaction admitted");}
 auto block=source.block_template(signer,12,0);check(block.transactions.size()==10,"ten transactions in block");sign_block(block,private_key,1337,genesis.block.header.hash());auto now=ConsensusRound::Clock::now();ConsensusRound round(source,journal,private_key,0,now,1s,1s,1s);round.propose(block,12);check(round.advance(now,12).committed,"source commits through journal-backed quorum");target_hash=block.header.hash();compact=block.to_transport_json().dump().size();check(compact>16*1024*1024,"legacy frame cannot carry target block");
 sent=source_peer.event("test-block-reference-sent");received=follower_peer.event("test-block-reference-received");check(sent.at("reference")==received.at("reference")&&sent.at("reference").at("hash")==hex(target_hash)&&valid_block_reference(sent.at("reference")),"real libp2p reference matches committed block");
 until([&]{source_peer.pump(0);follower_peer.pump(0);return rpc(follower_server.port(),"eth_blockNumber")=="0x1";},"Follower imports referenced oversized block",80);
 auto imported=follower_peer.event("test-block-fragments-imported");check(imported.at("blockHash")==hex(target_hash)&&imported.at("fragments")==40,"canonical catch-up uses forty fragments");
 for(const auto& tx:block.transactions)check(rpc(source_server.port(),"eth_getTransactionReceipt",{hex(tx.hash())})==rpc(follower_server.port(),"eth_getTransactionReceipt",{hex(tx.hash())}),"all receipts match");check(rpc(follower_server.port(),"eth_getBalance",{"0x00000000000000000000000000000000000000b0","latest"})=="0xa","exactly-once referenced transfer");
 check(source_peer.stop().code==0&&follower_peer.stop().code==0,"relays stop cleanly");
 }
 old_epoch=source.event_epoch();auto page=call("nano_networkEventRefs",{0,old_epoch}).at("result");check(page.at("reset")==false&&page.at("events").size()==1&&page.at("events").at(0).at("value")==sent.at("reference")&&page.at("cursor")==source.event_cursor(),"small reference page crosses large block event");check(page.dump().size()<1024,"single-block reference page stays below one KiB");check(call("nano_networkEvents",{10}).contains("error"),"legacy event route still exposes old oversized boundary");
 check(call("nano_networkEventRefs",{0,old_epoch}).at("result")==page,"reference query repeats identically");
 for(unsigned mode=0;mode<7;++mode){auto bad=sent.at("reference");if(mode==0)bad["version"]=2;if(mode==1)bad["hash"]="0x01";if(mode==2)bad["hash"]=std::string(66,'g');if(mode==3)bad["number"]="0x01";if(mode==4)bad["number"]="0x10000000000000000";if(mode==5)bad["extra"]=true;if(mode==6)bad["version"]=1.0;check(!valid_block_reference(bad),"invalid peer reference rejected");++rejected;}
 Transaction marker;marker.chain_id=1337;marker.nonce=10;marker.to=signer;marker.gas_price=2000000000;check(source.submit(SignedTransaction::sign(marker,private_key)),"post-block transaction admitted");
 for(unsigned height=2;height<=52;++height){auto block=source.block_template(signer,Int(height*12),0);sign_block(block,private_key,1337,genesis.block.header.hash());auto now=ConsensusRound::Clock::now();ConsensusRound next(source,journal,private_key,0,now,1s,1s,1s);next.propose(block,Int(height*12));check(next.advance(now,Int(height*12)).committed,"pagination block commits");}
 auto first=call("nano_networkEventRefs",{0,old_epoch}).at("result");auto second=call("nano_networkEventRefs",{first.at("cursor"),old_epoch}).at("result");check(first.at("events").size()==50&&second.at("events").size()==2&&second.at("cursor")==source.event_cursor(),"fifty-entry pages preserve following events");check(call("nano_networkEventRefs",{second.at("cursor"),old_epoch}).at("result").at("events").empty(),"drained cursor yields no duplicates");page_bytes=std::max(first.dump().size(),second.dump().size());check(page_bytes<16384,"reference page byte bound");
 for(const auto& params:std::vector<Json>{{0,""},{source.event_cursor()+1,old_epoch}}){auto reset=call("nano_networkEventRefs",params).at("result");check(reset.at("reset")==true&&reset.at("cursor")==source.event_cursor()&&reset.at("events").size()==1&&reset.at("events").at(0).at("value").at("hash")==hex(source.tip().header.hash()),"epoch/future cursor reset gives canonical tip");}
 check(call("nano_networkEventRefs",{0,std::string(35,'x')}).contains("error"),"oversized epoch rejected");
 // Bounded native event-ring exhaustion, with no network traffic or mining.
 for(const auto& key:event_keys)for(unsigned n=0;n<100;++n){marker.nonce=n;check(source.submit(SignedTransaction::sign(marker,key)),"event-ring transaction admitted");}check(source.pending_count()==10000,"event ring exercised with full pool across one hundred accounts");
 auto reset=call("nano_networkEventRefs",{0,old_epoch}).at("result");check(reset.at("reset")==true&&reset.at("cursor")==source.event_cursor()&&reset.at("events").at(0).at("value").at("number")=="0x34","expired history explicitly resets to current canonical tip");
 check(source.audit().at("verified")==true&&follower.audit().at("verified")==true,"histories audit after reference queries");
 }
 for(const auto& name:{"source","follower"}){Blockchain reopened(root/name,genesis);check(reopened.store().block_by_number(1)->header.hash()==target_hash&&reopened.audit().at("verified")==true,"referenced block survives reopening");Rpc rpc(reopened);auto reset=rpc.handle({{"jsonrpc","2.0"},{"id",1},{"method","nano_networkEventRefs"},{"params",Json::array({0,old_epoch})}}).at("result");check(reset.at("reset")==true&&reset.at("events").at(0).at("value").at("hash")==hex(reopened.tip().header.hash()),"reopened epoch reconciles canonical tip");}
 std::cout<<Json{{"scenario","block-event-references"},{"fork",fork},{"compactBlockBytes",compact},{"maximumPageBytes",page_bytes},{"referenceSent",sent},{"referenceReceived",received},{"fragments",40},{"receiptCount",10},{"paginatedBlocks",52},{"expiredHistoryEvents",10000},{"invalidReferencesRejected",rejected},{"reopenedAudits",2},{"coreProcesses",1},{"relayProcesses",2},{"legacyOversizedEventRefused",true}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
