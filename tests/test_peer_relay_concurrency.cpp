#include "support/process.hpp"
#include "../network/altair_archive_relay.hpp"
#include "../network/peer_relay_budget.hpp"
using namespace test;
int main(){try{
 const std::string root="0x"+std::string(64,'b'),epoch="0x"+std::string(64,'c');
 const AltairWireProfile profile{"0x539","0x"+std::string(64,'a'),"0x01020304",4096,1024,1024,1024};
 for(const size_t capacity:{1,2}){
  asio::io_context io;InboundBudget inbound;auto ingress=std::make_shared<AltairMessageIngress>(profile,inbound);
  PeerRelayBudget budget(capacity);std::map<std::string,std::shared_ptr<AltairRelayProgress>> progress;
  std::map<std::string,bool> completed;Reply delayed;
  auto core=[&](const std::string& method,Json,Reply reply){
   if(method=="nano_altairVoteInventory")reply(Json{{"catalogEpoch",epoch},{"votes",Json::array({Json{{"root",root}}})},{"missingSignedBlocks",0}});
   else reply(Json{{"available",true},{"root",root},{"envelope",{{"chainId",profile.chain_id},{"genesisHash",profile.genesis_hash},{"forkDigest",profile.fork_digest},{"kind","attestation"},{"sszBase64","AQI="}}}});
  };
  auto launch=[&](std::string peer){
   if(!budget.acquire(peer))return false;
   auto state=std::make_shared<AltairRelayProgress>();progress[peer]=state;
   auto send=[&,peer](Json,Reply reply){if(peer=="slow")delayed=reply;else reply(Json{{"accepted",true}});};
   auto done=[&,peer](Json result){check(!completed.contains(peer),"duplicate completion released newer permit");completed[peer]=result.at("success");budget.release(peer);};
   std::make_shared<AltairArchiveRelay>(io,core,send,"token",ingress,state,done,AltairWireKind::attestation)->start(std::chrono::milliseconds(20));
   return true;
  };
  check(launch("slow"),"slow peer not started");io.poll();check(bool(delayed),"slow peer never reached ack");
  check(!launch("slow"),"same peer admitted concurrently");
  check(launch("fast")== (capacity==2),"global bound ignored");
  if(capacity==2){
   check(!launch("overflow"),"global in-flight capacity exceeded");io.poll();
   check(completed.contains("fast")&&completed.at("fast")&&!completed.contains("slow"),"slow ack blocked independent peer");
   check(progress.at("fast")->acknowledged(root)&&!progress.at("slow")->acknowledged(root),"ack leaked between peers");
   check(launch("third"),"completed peer did not free capacity");io.poll();check(completed.at("third"),"third peer did not advance");
  }
  io.run();check(completed.contains("slow")&&!completed.at("slow")&&!budget.busy("slow"),"timeout failed to release peer");
  check(progress.at("slow")->cursor=="0x"&&!progress.at("slow")->acknowledged(root),"timeout acknowledged vote");
  check(budget.acquire("slow"),"same identity not reusable after timeout");
  delayed(Json{{"accepted",true}});io.restart();io.poll();
  check(budget.busy("slow")&&!progress.at("slow")->acknowledged(root),"late ack changed old state or released newer permit");
  budget.release("slow");
 }
 std::cout<<"Slow-peer isolation, serial negative control, global capacity, per-peer ordering, timeout and late acknowledgement verified\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
