#include "support/process.hpp"
#include "nano/rpc.hpp"
#include "nano/consensus_round.hpp"
#include "nano/block_auth.hpp"
#include "../network/json_frame.hpp"
using namespace test;
int main(){try{
 Temp root;auto key=bytes(Int(1),32);auto signer=public_address(public_key(key));
 std::string fork=std::getenv("NANO_TEST_EVM_FORK")?std::getenv("NANO_TEST_EVM_FORK"):"Paris";
 check(fork=="Paris"||fork=="Shanghai","explicit network profile");
 auto genesis=Genesis::from_json({{"gasLimit","0x2625a00"},{"config",{{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bftVersion",1},{"bootstrapValidator",signer},{"evmFork",fork}}},{"alloc",{{signer,{{"balance","0x100000000000000000"}}}}}});
 Blockchain chain(root/"producer",genesis);ChainStore journal(root/"journal");
 for(unsigned nonce=0;nonce<10;++nonce){Transaction tx;tx.chain_id=1337;tx.nonce=nonce;tx.gas=4000000;tx.gas_price=2000000000;tx.to="0x00000000000000000000000000000000000000b0";tx.value=1;tx.data=Bytes(900000,0);auto signed_tx=SignedTransaction::sign(tx,key);check(signed_tx.raw().size()<1024*1024,"transaction below pool byte ceiling");check(chain.submit(signed_tx),"transaction admitted");}
 auto block=chain.block_template(signer,12,0);check(block.transactions.size()==10,"all ten fit gas budget");sign_block(block,key,1337,genesis.block.header.hash());chain.verify_authentication(block);chain.verify_proposal(block,12);
 auto wire=block.to_transport_json().dump();check(wire.size()>JsonFrame::maximum_size,"compact proposal must exceed frame cap");
 ConsensusRound round(chain,journal,key,0,ConsensusRound::Clock::time_point{},1s,1s,1s);
 round.propose(block,12);check(round.advance(ConsensusRound::Clock::time_point{},12).committed,"real journal-backed oversized block finalizes");
 check(chain.tip().header.hash()==block.header.hash()&&chain.audit().at("verified")==true,"oversized canonical history audits");
 Rpc rpc(chain);auto call=[&](const std::string& method,Json params){return rpc.handle({{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",params}});};
 auto page=call("nano_networkBlocks",Json::array({0}));check(page.contains("error")&&page.at("error").at("message").get<std::string>().find("single-page size limit")!=std::string::npos,"legacy aggregate export retains frame bound");
 bool oversized_refused=false;try{JsonFrame frame;frame.feed(wire);}catch(const std::invalid_argument&){oversized_refused=true;}check(oversized_refused,"oversized single frame remains forbidden");
 auto refs=call("nano_networkEventRefs",Json::array({0,""}));check(refs.contains("result"),"bounded block references export");
 bool announced=false;for(const auto& event:refs.at("result").at("events"))if(event.at("type")=="block"&&event.at("value").at("hash")==hex(block.header.hash()))announced=true;
 check(announced,"committed oversized block is announced through reference route");
 auto manifest_reply=call("nano_networkBlockManifest",Json::array({hex(block.header.hash())}));check(manifest_reply.contains("result"),"oversized block manifest export");auto manifest=manifest_reply.at("result");
 auto framed=[](const Json& value){JsonFrame frame;check(frame.feed(value.dump()),"each transport document fits unchanged frame bound");};framed(refs);framed(manifest_reply);
 Blockchain follower(root/"follower",genesis);const std::string token="oversized-lifecycle-token";Rpc receiver(follower,token);
 auto receive=[&](const std::string& method,Json params){auto reply=receiver.handle({{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",params}});check(reply.contains("result"),"authenticated fragmented import succeeds");return reply.at("result");};
 auto id=receive("nano_beginBlockImport",Json::array({token,manifest})).at("id");size_t fragments=0;
 for(size_t index=0;index<block.transactions.size();++index){Json offset="0x0";do{
  auto reply=call("nano_networkTransactionFragment",Json::array({hex(block.header.hash()),hex(block.transactions[index].hash()),offset}));check(reply.contains("result"),"transaction fragment export");framed(reply);auto fragment=reply.at("result");
  receive("nano_appendBlockImport",Json::array({token,id,quantity(Int(index)),fragment}));++fragments;offset=fragment.at("next");
 }while(!offset.is_null());}
 auto imported=receive("nano_finishBlockImport",Json::array({token,id}));check(imported.at("success")==true,"certified fragmented import commits");
 check(fragments==40&&follower.tip().header.hash()==block.header.hash()&&follower.state().root()==chain.state().root()&&follower.audit().at("verified")==true,"all forty fragments reconstruct exact audited canonical state");
 check(follower.state().get("0x00000000000000000000000000000000000000b0").balance==10,"transfers applied exactly once");
 std::cout<<Json{{"scenario","oversized-valid-block"},{"fork",fork},{"compactBytes",wire.size()},{"maximumFrameBytes",JsonFrame::maximum_size},{"liveJournalCommitted",true},{"canonicalAudit",true},{"referenceAnnounced",true},{"fragments",fragments},{"legacyAggregateRefused",true},{"oversizedFrameRefused",true},{"fragmentedImportAudited",true},{"libp2pIntegrated",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
