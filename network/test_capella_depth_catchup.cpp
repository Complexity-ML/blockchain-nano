#include "capella_head_catalog_relay.hpp"
#include <iostream>
using namespace std::chrono_literals;
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
std::string root(char c){return "0x"+std::string(64,c);}
std::string id(unsigned slot){return root(slot==1?'f':slot==2?'2':slot==3?'1':'0');}
std::string hex(const std::vector<uint8_t>& b){std::string s="0x";for(auto v:b){s+="0123456789abcdef"[v>>4];s+="0123456789abcdef"[v&15];}return s;}
int main(){try{
 const AltairWireProfile p{"0x539",root('a'),"0x01020304",2048,512,256,256,root('b')};
 for(bool reject:{false,true}){
 asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(p,budget);auto knowledge=std::make_shared<AltairRelayProgress>(8);
 unsigned pages=0,sends=0,admitted=0;Json last;unsigned done=0;
 auto frame=[&](unsigned n){std::vector<uint8_t> b(404,0);b[0]=100;b[180]=84;b[100]=n;const auto parent=n==1?0:n==2?255:34;std::fill(b.begin()+116,b.begin()+148,uint8_t(parent));return Json{{"chainId",p.chain_id},{"genesisHash",p.genesis_hash},{"forkDigest",p.fork_digest},{"consensusIdentity",p.consensus_identity},{"kind","block"},{"ssz",hex(b)}};};
 auto core=[&](std::string method,Json args,Reply reply){check(args[0]=="token","missing authentication");Json v;
 if(method=="nano_capellaStatus")v={{"headRoot",id(3)},{"headSlot",uint64_t(3)}};
 else if(method=="nano_capellaExport"){const auto r=args[2].get<std::string>();const unsigned n=r==id(1)?1:r==id(2)?2:r==id(3)?3:0;check(n!=0,"bad local export");v={{"root",r},{"available",true},{"parentRoot",id(n-1)},{"envelope",frame(n)}};}
 else if(method=="nano_capellaCatalogGeneration")v=root('e');
 else if(method=="nano_capellaCatalogAcquire")v={{"handle",root('c')}};
 else if(method=="nano_capellaCatalogPage"){++pages;v={{"kind","block"},{"sequence","0x1"},{"digest",root('d')},{"entries",Json::array({id(3),id(2),id(1)})},{"next",nullptr}};}
 else if(method=="nano_capellaCatalogRelease"||method=="nano_capellaCatalogClose")v=true;
 else throw std::runtime_error("unexpected method");reply(v);};
 auto send=[&](Json f,Reply reply){++sends;const unsigned n=std::stoul(f["ssz"].get<std::string>().substr(202,2),nullptr,16);check(n==admitted+1,"child sent before acknowledged parent");if(!reject)++admitted;reply(Json{{"accepted",!reject}});};
 for(unsigned attempt=0;attempt<(reject?1:2);attempt++){
  auto q=std::make_shared<CapellaDeliveryQuantum>(CapellaDeliveryQuantum{4});
  auto deliver=[&](std::string r,Json,Reply reply){auto d=std::make_shared<CapellaBlockDelivery>(io,core,send,"token",id(0),r,ingress,knowledge,2,8192,std::move(reply),q);d->start(100ms);};
  auto h=std::make_shared<CapellaHeadCatalogRelay>(io,core,deliver,send,"token",id(0),ingress,knowledge,2,8192,4,8,[&](Json r){++done;last=std::move(r);},[&](const std::string& r){return knowledge->acknowledged(r);},q,true);h->start(100ms);io.restart();io.run();
  if(attempt==0){check(last["success"]==false&&last["headPriority"]=="dependency-depth-catalog-fallback","head failure hid fallback diagnostics");check(!knowledge->acknowledged(id(3)),"unconfirmed deep head was acknowledged");}
 }
 if(reject){check(pages==1&&sends==2&&admitted==0&&knowledge->retained()==0,"remote rejection advanced catch-up");}
 else {check(done==2&&last["success"]==true&&pages==2&&sends==3&&admitted==3&&knowledge->acknowledged(id(3)),"fresh session did not progressively catch up with fixed bound");}
 }
 std::cout<<"Progressive depth-bound catch-up and actual-rejection guards passed; modeled transport only\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
