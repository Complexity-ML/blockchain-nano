#include "support/process.hpp"
#include "../network/altair_archive_relay.hpp"
using namespace test;
int main(){try{
 asio::io_context io;InboundBudget budget;
 const std::string first="0x"+std::string(64,'1'),second="0x"+std::string(64,'2'),epoch="0x"+std::string(64,'3');
 const AltairWireProfile profile{"0x539","0x"+std::string(64,'a'),"0x12345678",4096,1024,1024,1024};
 auto ingress=std::make_shared<AltairMessageIngress>(profile,budget);
 auto progress=std::make_shared<AltairRelayProgress>();bool permit_first=false,uncertain=false;unsigned sent=0;
 std::string catalogue_epoch=epoch;std::vector<std::string> backlog;unsigned inventory_calls=0;bool duplicate_page=false,oversized_page=false;
 auto call=[&](const std::string& method,Json params,Reply reply){
  const bool proofs=method=="nano_altairSlashingInventory"||method=="nano_altairSlashing";
  const bool votes=method=="nano_altairVoteInventory"||method=="nano_altairVote";
  if(method=="nano_altairVoteInventory"||method=="nano_altairInventory"||method=="nano_altairSlashingInventory"){
   auto entries=Json::array();const auto cursor=params.at(1).get<std::string>();
   ++inventory_calls;
   if(!backlog.empty()){
    const auto at=cursor=="0x"?backlog.begin():std::find(backlog.begin(),backlog.end(),cursor)+1;
    const auto limit=std::stoul(params.at(2).get<std::string>(),nullptr,16);
    for(auto item=at;item!=backlog.end()&&entries.size()<limit;++item)entries.push_back({{"root",*item}});
   }else if(cursor=="0x")entries.push_back({{"root",first}});else if(cursor==first)entries.push_back({{"root",second}});
   if(duplicate_page&&!entries.empty())entries.back()=entries.front();
   if(oversized_page)while(entries.size()<=32)entries.push_back({{"root",first}});
   reply(Json{{"catalogEpoch",catalogue_epoch},{proofs?"slashings":votes?"votes":"blocks",entries},{"missingSignedBlocks",0}});
  }else{
   const auto root=params.at(1).get<std::string>();const std::vector<uint8_t> data{uint8_t(root==first?1:2)};
   reply(Json{{"available",true},{"root",root},{"envelope",{{"chainId",profile.chain_id},{"genesisHash",profile.genesis_hash},{"forkDigest",profile.fork_digest},{"kind",proofs?"attester_slashing":votes?"attestation":"block"},{"sszBase64",altair_base64(data)}}}});
  }
 };
 auto send=[&](Json frame,Reply reply){++sent;if(uncertain)reply(nullptr);else reply(Json{{"accepted",permit_first||frame.at("ssz")=="0x02"}});};
 auto attempt=[&](AltairWireKind kind=AltairWireKind::attestation){
  io.restart();std::optional<Json> done;
  auto relay=std::make_shared<AltairArchiveRelay>(io,call,send,"fixture",ingress,progress,[&](Json result){done=std::move(result);},kind);
  relay->start(std::chrono::seconds(1));io.run();check(done.has_value(),"relay callback missing");
  const auto& timings=done->at("stageUs");check(timings.is_object()&&timings.contains("inventory"),"missing inventory timing");
  int64_t sum=0;for(const auto& value:timings){const auto us=value.get<int64_t>();check(us>=0,"negative relay timing");sum+=us;}
  const auto total=done->at("elapsedMs").get<int64_t>()*1000;
  check(sum<=total+1000,"stage timings do not account for relay duration");
  if(done->at("stage")=="ack")check(timings.contains("payload")&&timings.contains("ack"),"missing payload or acknowledgement timing");
  return done->at("success").get<bool>();
 };
 check(!attempt()&&progress->cursor==first&&progress->retry_votes,"explicit rejection blocks newer votes or is falsely acknowledged");
 check(attempt()&&progress->cursor==second,"later eligible vote starved behind rejected vote");
 const auto before=sent;check(attempt()&&sent==before&&progress->cursor=="0x"&&!progress->retry_votes,"deferred vote pass was not scheduled");
 permit_first=true;check(attempt()&&progress->cursor==first,"deferred vote not retried after becoming admissible");
 const auto delivered=sent;
 check(attempt()&&progress->cursor==second&&sent==delivered,"acknowledged successor retransmitted in deferred pass");
 catalogue_epoch="0x"+std::string(64,'4');
 check(!attempt(),"catalogue change did not reset scan");
 check(attempt()&&sent==delivered,"catalogue rotation lost peer acknowledgement");
 progress=std::make_shared<AltairRelayProgress>();
 check(attempt()&&sent==delivered+1,"new peer session incorrectly inherited acknowledgements");
 // A tiny resource budget exercises eviction, not a limit on delivery.
 progress=std::make_shared<AltairRelayProgress>(1);
 check(attempt()&&attempt()&&progress->retained()==1,"acknowledgement cache exceeded its budget");
 progress->cursor="0x";const auto evicted=sent;
 check(attempt()&&sent==evicted+1,"evicted vote could not be delivered again");
 progress=std::make_shared<AltairRelayProgress>();uncertain=true;
 check(!attempt()&&progress->cursor=="0x"&&!progress->retry_votes,"lost response advanced delivery cursor");
 uncertain=false;permit_first=false;
 check(!attempt(AltairWireKind::block)&&progress->cursor=="0x","block dependency ordering bypassed on rejection");
 progress=std::make_shared<AltairRelayProgress>();
 check(!attempt(AltairWireKind::attester_slashing)&&progress->cursor==first&&progress->retry_votes,"rejected proof blocks independent successor");
 check(attempt(AltairWireKind::attester_slashing)&&progress->cursor==second,"valid proof successor not relayed");
 check(attempt(AltairWireKind::attester_slashing)&&progress->cursor=="0x","deferred proof not scheduled");
 uncertain=true;check(!attempt(AltairWireKind::attester_slashing)&&progress->cursor=="0x","lost proof reply advanced cursor");
 // A pruned catalogue can contain dozens of previously acknowledged entries.
 // They must not consume one scheduler interval each before a fresh vote.
 progress=std::make_shared<AltairRelayProgress>();uncertain=false;permit_first=true;
 for(unsigned i=0;i<70;++i){auto id=std::string(64,'0');const char* digits="0123456789abcdef";id[62]=digits[(i+1)/16];id[63]=digits[(i+1)%16];backlog.push_back("0x"+id);progress->acknowledge(backlog.back());}
 backlog.push_back(second);const auto sent_before=sent,calls_before=inventory_calls;
 check(attempt()&&sent==sent_before&&inventory_calls-calls_before==1,"cached scan did not yield at its budget");
 check(attempt()&&sent==sent_before&&inventory_calls-calls_before==2,"second cached scan lost bounded progress");
 check(attempt()&&sent==sent_before+1&&progress->cursor==second&&inventory_calls-calls_before==3,"fresh vote remained behind acknowledged backlog");
 // Unknown delivery still obeys the original retry contract after a fast scan.
 progress=std::make_shared<AltairRelayProgress>();for(size_t i=0;i<70;++i)progress->acknowledge(backlog[i]);uncertain=true;
 check(attempt()&&attempt(),"cached prefix lost progress");
 check(!attempt()&&progress->cursor==backlog[69]&&!progress->acknowledged(second),"lost tail reply was treated as acknowledged");
 uncertain=false;check(attempt()&&progress->cursor==second,"fresh tail was not retried after reply loss");
 // Corrupt pages cannot turn unknown entries into acknowledged progress.
 for(const bool oversized:{false,true}){
  progress=std::make_shared<AltairRelayProgress>();for(size_t i=0;i<70;++i)progress->acknowledge(backlog[i]);
  duplicate_page=!oversized;oversized_page=oversized;const auto prior=sent;
  check(!attempt()&&progress->cursor=="0x"&&sent==prior,"malformed page advanced cursor or sent payload");
 }
 duplicate_page=false;oversized_page=false;
 std::cout<<"Rejected votes do not starve eligible successors; deferred retry, lost response and block ordering checks passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
