#include "support/process.hpp"
#include "../network/altair_archive_relay.hpp"
using namespace test;
int main(){try{
 asio::io_context io;InboundBudget budget;
 const std::string first="0x"+std::string(64,'1'),second="0x"+std::string(64,'2'),epoch="0x"+std::string(64,'3');
 const AltairWireProfile profile{"0x539","0x"+std::string(64,'a'),"0x12345678",4096,1024,1024,1024};
 auto ingress=std::make_shared<AltairMessageIngress>(profile,budget);
 auto progress=std::make_shared<AltairRelayProgress>();bool permit_first=false,uncertain=false;unsigned sent=0;
 auto call=[&](const std::string& method,Json params,Reply reply){
  const bool proofs=method=="nano_altairSlashingInventory"||method=="nano_altairSlashing";
  const bool votes=method=="nano_altairVoteInventory"||method=="nano_altairVote";
  if(method=="nano_altairVoteInventory"||method=="nano_altairInventory"||method=="nano_altairSlashingInventory"){
   auto entries=Json::array();const auto cursor=params.at(1).get<std::string>();
   if(cursor=="0x")entries.push_back({{"root",first}});else if(cursor==first)entries.push_back({{"root",second}});
   reply(Json{{"catalogEpoch",epoch},{proofs?"slashings":votes?"votes":"blocks",entries},{"missingSignedBlocks",0}});
  }else{
   const auto root=params.at(1).get<std::string>();const std::vector<uint8_t> data{uint8_t(root==first?1:2)};
   reply(Json{{"available",true},{"root",root},{"envelope",{{"chainId",profile.chain_id},{"genesisHash",profile.genesis_hash},{"forkDigest",profile.fork_digest},{"kind",proofs?"attester_slashing":votes?"attestation":"block"},{"sszBase64",altair_base64(data)}}}});
  }
 };
 auto send=[&](Json frame,Reply reply){++sent;if(uncertain)reply(nullptr);else reply(Json{{"accepted",permit_first||frame.at("ssz")=="0x02"}});};
 auto attempt=[&](AltairWireKind kind=AltairWireKind::attestation){
  io.restart();std::optional<Json> done;
  auto relay=std::make_shared<AltairArchiveRelay>(io,call,send,"fixture",ingress,progress,[&](Json result){done=std::move(result);},kind);
  relay->start(std::chrono::seconds(1));io.run();check(done.has_value(),"relay callback missing");return done->at("success").get<bool>();
 };
 check(!attempt()&&progress->cursor==first&&progress->retry_votes,"explicit rejection blocks newer votes or is falsely acknowledged");
 check(attempt()&&progress->cursor==second,"later eligible vote starved behind rejected vote");
 const auto before=sent;check(attempt()&&sent==before&&progress->cursor=="0x"&&!progress->retry_votes,"deferred vote pass was not scheduled");
 permit_first=true;check(attempt()&&progress->cursor==first,"deferred vote not retried after becoming admissible");
 progress=std::make_shared<AltairRelayProgress>();uncertain=true;
 check(!attempt()&&progress->cursor=="0x"&&!progress->retry_votes,"lost response advanced delivery cursor");
 uncertain=false;permit_first=false;
 check(!attempt(AltairWireKind::block)&&progress->cursor=="0x","block dependency ordering bypassed on rejection");
 progress=std::make_shared<AltairRelayProgress>();
 check(!attempt(AltairWireKind::attester_slashing)&&progress->cursor==first&&progress->retry_votes,"rejected proof blocks independent successor");
 check(attempt(AltairWireKind::attester_slashing)&&progress->cursor==second,"valid proof successor not relayed");
 check(attempt(AltairWireKind::attester_slashing)&&progress->cursor=="0x","deferred proof not scheduled");
 uncertain=true;check(!attempt(AltairWireKind::attester_slashing)&&progress->cursor=="0x","lost proof reply advanced cursor");
 std::cout<<"Rejected votes do not starve eligible successors; deferred retry, lost response and block ordering checks passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
