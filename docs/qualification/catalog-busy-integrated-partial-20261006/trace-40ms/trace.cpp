#include "capella_head_catalog_relay.hpp"
#include <iostream>
using namespace std::chrono_literals;
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
std::string root(char c){return "0x"+std::string(64,c);}
std::string hex(const std::vector<uint8_t>& b){std::string s="0x";for(auto v:b){s+="0123456789abcdef"[v>>4];s+="0123456789abcdef"[v&15];}return s;}
int main(int argc,char** argv){try{
 const std::string mode=argc>1?argv[1]:"normal";bool injected=false;
 const AltairWireProfile p{"0x539",root('a'),"0x01020304",2048,512,256,256,root('b')};
 auto knowledge=std::make_shared<AltairRelayProgress>(16);
#ifdef NANO_TEST_DEPENDENCY_PREPARATION
 auto preparation=std::make_shared<CapellaDependencyPreparation>();
#endif
 unsigned total_exports=0,total_sends=0,attempts=0;
 for(;attempts<20&&!knowledge->acknowledged(root('9'));++attempts){
  const auto started_cursor=preparation->cursor;
  asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(p,budget);
  unsigned exports=0,sends=0,done=0;Json result;Reply late;std::string cursor_at_done;std::shared_ptr<CapellaHeadCatalogRelay> relay;
  auto core=[&](std::string method,Json args,Reply reply){Json v;
   if(method=="nano_capellaStatus")v={{"headRoot",root('9')},{"headSlot",uint64_t(9)}};
   else if(method=="nano_capellaExport"){
    ++exports;++total_exports;const auto id=args.at(2).get<std::string>();const unsigned n=id.at(2)-'0';check(n>=1&&n<=9,"export escaped chain");
    if((mode=="unavailable"||mode=="unavailable-anchor")&&!injected){injected=true;Json unavailable{{"root",id},{"available",false}};if(mode=="unavailable-anchor"){unavailable["known"]=true;unavailable["anchor"]=true;}reply(unavailable);return;}
    std::vector<uint8_t> bytes(404);bytes[0]=100;bytes[180]=84;bytes[100]=n;std::fill(bytes.begin()+116,bytes.begin()+148,uint8_t((n-1)*17));
    v={{"root",id},{"available",true},{"parentRoot",root(char('0'+n-1))},{"envelope",{{"chainId",p.chain_id},{"genesisHash",p.genesis_hash},{"forkDigest",p.fork_digest},{"consensusIdentity",p.consensus_identity},{"kind","block"},{"ssz",hex(bytes)}}}};
   }else if(method=="nano_capellaCatalogGeneration")v=root('e');
   else if(method=="nano_capellaCatalogAcquire")v={{"handle",root('c')}};
   else if(method=="nano_capellaCatalogPage")v={{"kind","block"},{"sequence","0x1"},{"digest",root('d')},{"entries",Json::array()},{"next",nullptr}};
   else if(method=="nano_capellaCatalogRelease")v=true;
   else throw std::runtime_error("unexpected core operation");
   if(method=="nano_capellaExport"&&(mode=="slow"||mode=="slow-duplicate")){
    auto timer=std::make_shared<asio::steady_timer>(io);timer->expires_after(40ms);timer->async_wait([timer,reply,v,mode](auto e){if(!e){reply(v);if(mode=="slow-duplicate")reply(v);}});return;
   }
   reply(v);if(mode=="duplicate")reply(v);
  };
  auto send=[&](Json frame,Reply reply){const auto n=std::stoul(frame.at("ssz").get<std::string>().substr(202,2),nullptr,16);check(n==1||knowledge->acknowledged(root(char('0'+n-1))),"sent descendant without admitted parent");check(!knowledge->acknowledged(root(char('0'+n))),"retransmitted admitted payload");++sends;++total_sends;
   if(!injected&&(mode=="refused"||mode=="lost"||mode=="cancel")){injected=true;if(mode=="refused")reply(Json{{"accepted",false}});else{late=reply;if(mode=="cancel")relay->cancel();}return;}
   reply(Json{{"accepted",true}});if(mode=="duplicate")reply(Json{{"accepted",true}});};
  auto quantum=std::make_shared<CapellaDeliveryQuantum>(CapellaDeliveryQuantum{2});
  relay=std::make_shared<CapellaHeadCatalogRelay>(io,core,[](std::string,Json,Reply){throw std::runtime_error("empty catalog sent payload");},send,"token",root('0'),ingress,knowledge,3,8192,2,8,[&](Json r){++done;result=std::move(r);cursor_at_done=preparation->cursor;},[&](const std::string& id){return knowledge->acknowledged(id);},quantum,true,nullptr,false,nullptr
#ifdef NANO_TEST_DEPENDENCY_PREPARATION
   ,preparation
#endif
  );relay->start(100ms);io.run();if(late){const auto before=knowledge->retained();late(Json{{"accepted",true}});io.restart();io.run();check(knowledge->retained()==before,"late unknown ACK seeded peer knowledge");}check(preparation->cursor==cursor_at_done,"late export mutated preparation after completion");check(done==1,"completion duplicated");check(exports<=3&&sends<=2,"attempt exceeded unchanged preparation or admission bound");std::cout<<Json{{"attempt",attempts},{"cursorBefore",started_cursor},{"cursorAfter",preparation->cursor},{"exports",exports},{"sends",sends},{"result",result}}.dump()<<std::endl;check(!result.value("success",false)||knowledge->acknowledged(root('9')),"claimed recent head without actual ACK");
 }
 std::cout<<"mode="<<mode<<" attempts="<<attempts<<" exports="<<total_exports<<" sends="<<total_sends<<" targetAcknowledged="<<knowledge->acknowledged(root('9'))<<"; modeled receiver, not WAN qualification\n";
 check(knowledge->acknowledged(root('9'))&&total_sends==(mode=="refused"||mode=="lost"||mode=="cancel"?10u:9u),"repeated bounded turns never admitted deep target");
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
