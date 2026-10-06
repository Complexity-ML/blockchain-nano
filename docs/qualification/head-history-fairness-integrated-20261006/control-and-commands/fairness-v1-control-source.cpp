#include "capella_head_catalog_relay.hpp"
#include "capella_relay_schedule.hpp"
#include <iostream>
using namespace std::chrono_literals;
void check(bool value,const char* reason){if(!value)throw std::runtime_error(reason);}
std::string root(char n){return "0x"+std::string(64,n);}
std::string encode(const std::vector<uint8_t>& b){std::string s="0x";for(auto v:b){s+="0123456789abcdef"[v>>4];s+="0123456789abcdef"[v&15];}return s;}
int main(){try{
 const AltairWireProfile p{"0x539",root('a'),"0x01020304",2048,512,256,256,root('b')};unsigned cases=0;
 for(const std::string mode:{"slow-head","status-unavailable","missing-head","head-in-history","catalog-timeout","cancel-history","status-timeout","duplicate-replies"}){
  asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(p,budget);auto knowledge=std::make_shared<AltairRelayProgress>(8);CapellaRelaySchedule schedule;
  unsigned done=0,pages=0,history_sent=0,head_sent=0,status_calls=0;Json result;std::vector<Reply> late;
  auto frame=[&](unsigned n){std::vector<uint8_t> b(404);b[0]=100;b[180]=84;b[100]=n;return Json{{"chainId",p.chain_id},{"genesisHash",p.genesis_hash},{"forkDigest",p.fork_digest},{"consensusIdentity",p.consensus_identity},{"kind","block"},{"ssz",encode(b)}};};
  auto core=[&](std::string method,Json args,Reply reply){check(args[0]=="token","missing source authentication");Json value;
   if(method=="nano_capellaStatus"){++status_calls;if(mode=="status-timeout"){late.push_back(reply);return;}value=mode=="status-unavailable"?Json(nullptr):Json{{"headRoot",root('3')},{"headSlot",uint64_t(3)}};}
   else if(method=="nano_capellaExport"){auto id=args[2].get<std::string>();auto n=unsigned(id[2]-'0');value={{"root",id},{"available",true},{"parentRoot",root('0')},{"envelope",frame(n)}};}
   else if(method=="nano_capellaCatalogGeneration")value=root('e');
   else if(method=="nano_capellaCatalogAcquire")value={{"handle",root('c')}};
   else if(method=="nano_capellaCatalogPage"){++pages;value={{"kind","block"},{"sequence","0x1"},{"digest",root('d')},{"entries",mode=="head-in-history"?Json::array({root('3'),root('4')}):Json::array({root('4')})},{"next",nullptr}};}
   else if(method=="nano_capellaCatalogRelease"||method=="nano_capellaCatalogClose")value=true;
   else throw std::runtime_error("unexpected core method");reply(value);if(mode=="duplicate-replies")reply(value);
  };
  auto send=[&](Json,Reply reply){++head_sent;late.push_back(std::move(reply));};
  auto deliver=[&](std::string id,Json,Reply reply){++history_sent;
   if(mode=="catalog-timeout"||mode=="cancel-history"){late.push_back(std::move(reply));return;}
   // This is the exact positive receiver ACK, not a source inventory hint.
   knowledge->acknowledge(id);reply(Json{{"accepted",true}});if(mode=="duplicate-replies")reply(Json{{"accepted",true}});
  };
  for(unsigned attempt=0;attempt<2;++attempt){
   bool historical=false;
#ifdef NANO_HEAD_HISTORY_FIRST
   // The real peer scheduler is used; other category turns cannot alter this bit.
   while(schedule.next()!=0){}historical=schedule.next_block_history_first();
#endif
   auto q=std::make_shared<CapellaDeliveryQuantum>(CapellaDeliveryQuantum{8});
   auto relay=std::make_shared<CapellaHeadCatalogRelay>(io,core,deliver,send,"token",root('0'),ingress,knowledge,4,8192,4,8,[&](Json r){++done;result=std::move(r);},[&](const std::string& id){return knowledge->acknowledged(id);},q,true
#ifdef NANO_HEAD_HISTORY_FIRST
    ,nullptr,historical
#endif
   );relay->start(30ms);io.restart();
   if(mode=="cancel-history"&&historical){io.poll();relay->cancel();io.restart();}
   io.run();check(done==attempt+1,"duplicate attempt completion");
  }
  if(mode=="catalog-timeout"||mode=="cancel-history")check(pages==1&&!knowledge->acknowledged(root('4'))&&result["success"]==false,"uncertain historical delivery invented an ACK");
  else check(pages==1&&knowledge->acknowledged(root('4'))&&history_sent>0,"HISTORY STARVATION: unconfirmed head consumed both historical opportunities");
  if(mode=="head-in-history")check(result["success"]==true&&knowledge->acknowledged(root('3')),"actual historical head ACK failed qualification");
  else check(result["success"]==false&&!knowledge->acknowledged(root('3')),"historical turn invented selected-head ACK");
  if(mode=="status-unavailable")check(result["reason"]=="head-reference-unavailable"&&result["closed"]==true,"lost source status erased history release/result");
  if(mode=="status-timeout")check(result["reason"]=="head-or-catalog-deadline"&&result["delivered"]==1,"late status erased delivered history/deadline");
  for(auto& reply:late)reply(Json{{"accepted",true}});io.restart();io.run();check(done==2,"late response completed attempt twice");if(mode!="head-in-history")check(!knowledge->acknowledged(root('3')),"late head ACK bypassed finished attempt");
  ++cases;
 }
 std::cout<<cases<<" historical/head fairness scenarios passed; modeled transport is not WAN qualification\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
