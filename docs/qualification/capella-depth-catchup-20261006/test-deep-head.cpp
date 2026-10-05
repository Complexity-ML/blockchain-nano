#include "capella_head_catalog_relay.hpp"
#include <iostream>
#include <thread>
using namespace std::chrono_literals;
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
std::string root(char c){return "0x"+std::string(64,c);}
std::string encode(const std::vector<uint8_t>& b){std::string s="0x";for(auto v:b){s+="0123456789abcdef"[v>>4];s+="0123456789abcdef"[v&15];}return s;}
int main(){try{
 const AltairWireProfile p{"0x539",root('a'),"0x01020304",2048,512,256,256,root('b')};unsigned cases=0;
 for(const std::string mode:{"deep-head"}){
  asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(p,budget);auto knowledge=std::make_shared<AltairRelayProgress>(8);if(mode=="cached")knowledge->acknowledge(root('3'));
  unsigned done=0,pages=0,head_sends=0,historical_sends=0;Json result;Reply late;
  auto frame=[&](unsigned n){std::vector<uint8_t> b(404,0);b[0]=100;b[180]=84;b[100]=n;const auto parent=n==4?0:n-1;std::fill(b.begin()+116,b.begin()+148,uint8_t(parent*17));return Json{{"chainId",p.chain_id},{"genesisHash",p.genesis_hash},{"forkDigest",p.fork_digest},{"consensusIdentity",p.consensus_identity},{"kind","block"},{"ssz",encode(b)}};};
  auto core=[&](std::string method,Json params,Reply reply){check(params[0]=="token","authentication missing");Json value;
   if(method=="nano_capellaStatus"){
    if(mode=="cancel-status"||mode=="timeout-status"){late=reply;return;}
    value=mode=="status-null"?Json(nullptr):Json{{"headRoot",mode=="invalid-root"?"0xBAD":mode=="anchor"?root('0'):root('3')},{"headSlot",uint64_t(3)}};if(mode=="bad-slot")value["headSlot"]=-1;
   }else if(method=="nano_capellaExport"){
    const auto id=params[2].get<std::string>();const unsigned n=id[2]-'0';check(n>0&&n<5,"invalid export root");value={{"root",id},{"available",mode!="missing-export"},{"parentRoot",root(char('0'+(n==4?0:n-1)))},{"envelope",frame(n)}};
   }else if(method=="nano_capellaCatalogGeneration"){value=root('e');}
   else if(method=="nano_capellaCatalogAcquire")value={{"handle",root('c')}};
   else if(method=="nano_capellaCatalogPage"){++pages;value={{"kind","block"},{"sequence","0x1"},{"digest",root('d')},{"entries",Json::array({root('1'),root('2'),root('3'),root('4')})},{"next",nullptr}};}
   else if(method=="nano_capellaCatalogRelease"||method=="nano_capellaCatalogClose")value=true;
   else throw std::runtime_error("unexpected core method");reply(value);if(mode=="duplicate")reply(value);
  };
  auto send=[&](Json wire,Reply reply){++head_sends;check(pages==0,"priority dependency sent after history");if(mode=="cancel-head"||mode=="timeout-head"){late=reply;return;}reply(Json{{"accepted",mode!="reject-head"}});if(mode=="duplicate")reply(Json{{"accepted",true}});};
  std::shared_ptr<CapellaDeliveryQuantum> q;
  auto deliver=[&](std::string id,Json,Reply reply){++historical_sends;if(mode=="shared-quantum"){auto historical=std::make_shared<CapellaBlockDelivery>(io,core,send,"token",root('0'),id,ingress,knowledge,4,8192,std::move(reply),q);historical->start(20ms);return;}if(mode=="timeout-catalog"){late=reply;return;}if(mode=="late-completion")std::this_thread::sleep_for(25ms);knowledge->acknowledge(id);reply(Json{{"accepted",true}});};
  q=std::make_shared<CapellaDeliveryQuantum>(CapellaDeliveryQuantum{mode=="yield"?2u:mode=="shared-quantum"?3u:8u});
  auto d=std::make_shared<CapellaHeadCatalogRelay>(io,core,deliver,send,"token",root('0'),ingress,knowledge,2,8192,4,8,[&](Json r){++done;result=std::move(r);},[&](const std::string& id){return knowledge->acknowledged(id);},q,true);d->start(20ms);
  if(mode=="cancel-status"||mode=="cancel-head"){io.poll();check(bool(late),"cancel fixture not reached");d->cancel();late(Json{{"accepted",true}});io.restart();}io.run();
  std::cout<<Json{{"result",result},{"pages",pages},{"headSends",head_sends},{"historicalSends",historical_sends},{"acknowledged",knowledge->retained()}}.dump()<<std::endl;
  check(done==1,"missing terminal callback");
  check(pages>0,"deep head blocks all historical catalog progress on a fresh peer");
  ++cases;
 }
 std::cout<<cases<<" head-priority scenarios passed; modeled transport is not WAN qualification\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
