#include "support/process.hpp"
#include "../network/altair_archive_relay.hpp"
#include "../network/altair_vote_delivery.hpp"
using namespace test;
int main(){try{
 asio::io_context io;InboundBudget budget;
 const std::string first="0x"+std::string(64,'1'),second="0x"+std::string(64,'2'),epoch="0x"+std::string(64,'3');
 const AltairWireProfile profile{"0x539","0x"+std::string(64,'a'),"0x12345678",4096,1024,1024,1024};
 auto ingress=std::make_shared<AltairMessageIngress>(profile,budget);
 auto progress=std::make_shared<AltairRelayProgress>();bool permit_first=false,uncertain=false;unsigned sent=0;
 std::string catalogue_epoch=epoch;std::vector<std::string> backlog;unsigned inventory_calls=0;bool duplicate_page=false,oversized_page=false,missing_payload=false;
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
   if(missing_payload){reply(Json{{"available",false}});return;}
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
 // A page of unknown votes must amortize inventory RPCs too, while retaining
 // per-item durable export and ACK semantics across distinct relay attempts.
 progress=std::make_shared<AltairRelayProgress>();backlog.resize(32);permit_first=true;
 const auto page_calls=inventory_calls,page_sent=sent;
 for(size_t i=0;i<backlog.size();++i)check(attempt()&&progress->cursor==backlog[i],"cached unknown page lost ordered delivery");
 check(inventory_calls==page_calls+1&&sent==page_sent+32,"unknown page fetched repeatedly or payload ACKs skipped");
 // A lost ACK cannot consume the rest of a cached page or skip its item.
 progress=std::make_shared<AltairRelayProgress>();check(attempt(),"page head missing");
 uncertain=true;const auto cursor=progress->cursor;
 check(!attempt()&&progress->cursor==cursor&&!progress->acknowledged(backlog[1]),"cached page advanced after lost ACK");
 uncertain=false;check(attempt()&&progress->cursor==backlog[1],"lost cached item was not retried");
 // Pruning between inventory and export invalidates the cached item: do
 // not acknowledge it, and refresh the catalogue before trying it again.
 progress=std::make_shared<AltairRelayProgress>();check(attempt(),"prune fixture head missing");
 missing_payload=true;const auto prune_sent=sent,prune_calls=inventory_calls;
 check(!attempt()&&progress->cursor==backlog[0]&&sent==prune_sent,"missing cached payload was delivered");
 missing_payload=false;catalogue_epoch="0x"+std::string(64,'5');
 check(!attempt()&&progress->cursor=="0x"&&inventory_calls==prune_calls+1,"stale cached page did not refresh catalogue");
 check(attempt()&&progress->cursor==backlog[1]&&sent==prune_sent+1,"refreshed page lost acknowledgement or successor");
 // Inbound possession knowledge is gated by admission and session identity.
 auto sender=std::make_shared<AltairRelayProgress>(),other=std::make_shared<AltairRelayProgress>();
 AltairWork::Done admitted;bool enqueue_ok=true;unsigned completions=0;
 auto receive=remember_vote_sender([&](AltairWireMessage,AltairWork::Cancellation,AltairWork::Done done){if(!enqueue_ok)return false;admitted=std::move(done);return true;},sender);
 const auto inbound_root=nano::hex(nano::sha256(nano::Bytes{1,2}));
 auto inbound=[&](AltairWireKind kind=AltairWireKind::attestation){AltairWireMessage message;message.kind=kind;message.ssz={1,2};return receive(std::move(message),std::make_shared<std::atomic<bool>>(false),[&](auto){++completions;});};
 check(inbound()&&!sender->acknowledged(inbound_root),"unadmitted inbound vote became known");admitted(AltairWork::Decision::rejected);
 check(!sender->acknowledged(inbound_root),"rejected inbound vote became known");
 check(inbound(),"inbound retry refused");admitted(AltairWork::Decision::local_failure);
 check(!sender->acknowledged(inbound_root),"failed inbound vote became known");
 check(inbound(AltairWireKind::block),"block dispatch refused");admitted(AltairWork::Decision::accepted);
 check(!sender->acknowledged(inbound_root),"block seeded vote cache");
 enqueue_ok=false;check(!inbound()&&!sender->acknowledged(inbound_root),"failed enqueue seeded vote cache");enqueue_ok=true;
 check(inbound(),"accepted vote dispatch refused");auto fresh_session=std::make_shared<AltairRelayProgress>();admitted(AltairWork::Decision::accepted);
 check(sender->acknowledged(inbound_root)&&sender->cursor=="0x"&&!other->acknowledged(inbound_root)&&!fresh_session->acknowledged(inbound_root)&&completions==4,"inbound knowledge escaped its session or changed delivery progress");
 // Run the actual outbound relay: possession suppresses only the echo,
 // while an independent peer still receives and acknowledges this vote.
 backlog={inbound_root};progress=sender;const auto echo_sent=sent;
 check(attempt()&&sent==echo_sent,"accepted inbound vote echoed to its sender");
 progress=other;check(attempt()&&sent==echo_sent+1&&other->acknowledged(inbound_root),"sender knowledge suppressed independent peer delivery");
 std::cout<<"Rejected votes do not starve eligible successors; deferred retry, lost response and block ordering checks passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
