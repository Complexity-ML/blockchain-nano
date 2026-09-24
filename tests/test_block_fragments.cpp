#include "support/process.hpp"
#include "nano/http.hpp"
#include "support/persistent_rpc.hpp"
#include "nano/consensus_round.hpp"
#include "nano/block_auth.hpp"
#include "../network/json_frame.hpp"
using namespace test;
struct RunningServer {HttpServer& server;std::thread thread;explicit RunningServer(HttpServer& s):server(s),thread([&s]{s.run();}){}~RunningServer(){server.stop();thread.join();}};
int main(){try{
 Temp root;auto key=bytes(Int(1),32);auto signer=public_address(public_key(key));
 std::string fork=std::getenv("NANO_TEST_EVM_FORK")?std::getenv("NANO_TEST_EVM_FORK"):"Paris";
 check(fork=="Paris"||fork=="Shanghai","explicit network profile");
 auto genesis=Genesis::from_json({{"gasLimit","0x2625a00"},{"config",{{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bftVersion",1},{"bootstrapValidator",signer},{"evmFork",fork}}},{"alloc",{{signer,{{"balance","0x100000000000000000"}}}}}});
 Blockchain chain(root/"producer",genesis);ChainStore journal(root/"journal");
 for(unsigned nonce=0;nonce<10;++nonce){Transaction tx;tx.chain_id=1337;tx.nonce=nonce;tx.gas=4000000;tx.gas_price=2000000000;tx.to="0x00000000000000000000000000000000000000b0";tx.value=1;tx.data=Bytes(900000,0);auto signed_tx=SignedTransaction::sign(tx,key);check(signed_tx.raw().size()<1024*1024,"transaction below pool byte ceiling");check(chain.submit(signed_tx),"transaction admitted");}
 auto block=chain.block_template(signer,12,0);check(block.transactions.size()==10,"all ten fit gas budget");sign_block(block,key,1337,genesis.block.header.hash());chain.verify_authentication(block);chain.verify_proposal(block,12);

 ConsensusRound round(chain,journal,key,0,ConsensusRound::Clock::time_point{},1s,1s,1s);round.propose(block,12);check(round.advance(ConsensusRound::Clock::time_point{},12).committed,"real local journal-backed finality");
 Rpc rpc(chain);HttpServer server(rpc,"127.0.0.1",0);RunningServer running(server);PersistentRpc client(server.port());
 auto call=[&](const std::string& method,Json params)->Json{try{return {{"jsonrpc","2.0"},{"id",1},{"result",client.call(method,params)}};}catch(const PersistentRpcError& error){return {{"jsonrpc","2.0"},{"id",1},{"error",{{"code",error.code},{"message",error.what()}}}};}};
 const auto block_hash=hex(block.header.hash());auto manifest_reply=call("nano_networkBlockManifest",Json::array({block_hash}));auto manifest=manifest_reply.at("result");check(manifest.at("version")==1,"manifest version");auto assembled=manifest.at("block");auto hashes=assembled.at("transactions");check(hashes.size()==10&&assembled.at("transactionEncoding")=="signed-rlp-hashes-v1","manifest identifies ordered transaction hashes");
 bool refused=false;try{Block::from_json(assembled);}catch(const std::exception&){refused=true;}check(refused,"manifest alone must not be treated as executable block");
 assembled["transactionEncoding"]="signed-rlp-v1";assembled["transactions"]=Json::array();unsigned fragments=0;size_t maximum_reply=manifest_reply.dump().size();
 for(size_t index=0;index<hashes.size();++index){Bytes raw;uint64_t offset=0;size_t pages=0;
  for(;;){auto response=call("nano_networkTransactionFragment",Json::array({block_hash,hashes[index],quantity(Int(std::to_string(offset)))}));auto size=response.dump().size();maximum_reply=std::max(maximum_reply,size);check(size<=512*1024+1024,"bounded serialized fragment reply");JsonFrame frame;check(frame.feed(response.dump()),"fragment fits actual libp2p frame parser");auto part=response.at("result");check(part.at("version")==1&&part.at("blockHash")==block_hash&&part.at("transactionHash")==hashes[index]&&u64(quantity(part.at("offset")))==offset,"fragment identity and offset");auto data=unhex(part.at("data"));check(data.size()<=256*1024&&!data.empty(),"bounded nonempty fragment");raw.insert(raw.end(),data.begin(),data.end());++pages;++fragments;
   if(index==0&&offset==0)check(call("nano_networkTransactionFragment",Json::array({block_hash,hashes[index],"0x0"}))==response,"repeated fragment is byte-identical");
   if(part.at("next").is_null()){check(raw.size()==u64(quantity(part.at("total"))),"complete transaction length");break;}
   auto next=u64(quantity(part.at("next")));check(next==offset+data.size()&&next>offset,"fragment cursor advances exactly");offset=next;check(pages<8,"bounded fixture fragment traversal");
  }
  check(raw==block.transactions[index].raw()&&hex(keccak(raw))==hashes[index].get<std::string>(),"exact signed transaction reconstruction");assembled["transactions"].push_back(hex(raw));
 }
 auto restored=Block::from_json(assembled);check(restored.header.hash()==block.header.hash()&&transactions_root(restored.transactions)==block.header.transactions_root,"reconstructed block commitments");check(restored.to_transport_json()==block.to_transport_json(),"complete reconstructed block bytes");
 unsigned invalid=0;const auto zero=hex(Bytes(32,0));
 for(const auto& params:std::vector<Json>{Json::array({block_hash,hashes[0],1}),Json::array({block_hash,hashes[0],-1}),Json::array({block_hash,hashes[0],"0x10000000000000000"}),Json::array({block_hash,hashes[0],"0x100000"}),Json::array({zero,hashes[0],0}),Json::array({block_hash,zero,0}),Json::array({block_hash,hashes[0]})}){check(call("nano_networkTransactionFragment",params).contains("error"),"invalid fragment query rejected");++invalid;}
 check(call("nano_networkBlockManifest",Json::array({zero})).contains("error"),"unknown manifest refused");++invalid;
 Blockchain follower(root/"follower",genesis);Rpc receiver(follower);unsigned invalid_blocks=0;
 for(unsigned mode=0;mode<3;++mode){auto bad=assembled;if(mode==0)bad["transactions"].erase(bad["transactions"].end()-1);if(mode==1)std::swap(bad["transactions"][0],bad["transactions"][1]);if(mode==2){auto data=bad["transactions"][0].get<std::string>();data.back()=data.back()=='0'?'1':'0';bad["transactions"][0]=data;}
  auto response=receiver.handle({{"jsonrpc","2.0"},{"id",2},{"method","validator_submitBlock"},{"params",Json::array({bad})}});check((response.contains("error")||response.at("result").at("success")==false)&&follower.tip().header.number==0,"incomplete reordered or corrupt block cannot import");++invalid_blocks;
 }
 auto imported=receiver.handle({{"jsonrpc","2.0"},{"id",3},{"method","validator_submitBlock"},{"params",Json::array({assembled})}});check(imported.at("result").at("success")==true&&follower.tip().header.hash()==block.header.hash()&&follower.state().root()==chain.state().root()&&follower.audit().at("verified")==true,"complete reconstructed block imports and audits through direct RPC");
 check(chain.tip().header.hash()==block.header.hash()&&chain.audit().at("verified")==true,"fragment reads preserve canonical producer state");
 std::cout<<Json{{"httpExport",true},{"scenario","block-fragment-export"},{"fork",fork},{"fragments",fragments},{"maximumReplyBytes",maximum_reply},{"manifestBytes",manifest_reply.dump().size()},{"compactBlockBytes",block.to_transport_json().dump().size()},{"invalidQueriesRejected",invalid},{"invalidAssembliesRejected",invalid_blocks},{"exactReconstruction",true},{"directRpcImportAudited",true},{"relayIntegrated",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
