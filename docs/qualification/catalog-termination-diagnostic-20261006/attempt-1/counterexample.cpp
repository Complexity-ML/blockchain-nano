#include "capella_catalog_relay.hpp"
#include <iostream>
using namespace std::chrono_literals;
static std::string root(char c){return "0x"+std::string(64,c);}
static void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(){try{
 const AltairWireProfile profile{"0x539",root('a'),"0x01020304",1024,256,256,256,root('b')};
 for(const std::string stop:{"export-unavailable","export-unconfirmed","entry-limit","catalog-exhausted","cancelled-or-deadline"}){
  asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(profile,budget);Json result;unsigned sends=0,releases=0,done=0;
  auto core=[&](std::string method,Json p,Reply reply){
   if(method=="nano_capellaCatalogGeneration")reply(root('e'));
   else if(method=="nano_capellaCatalogAcquire")reply(Json{{"handle",root('c')}});
   else if(method=="nano_capellaCatalogRelease"){++releases;reply(true);}
   else if(method=="nano_capellaCatalogPage"){
    const bool first=p[2].is_null();
    reply(Json{{"kind","block"},{"sequence","0x1"},{"digest",root('d')},{"entries",Json::array({root(first?'1':'2')})},{"next",first&&stop!="catalog-exhausted"?Json(root('1')):Json(nullptr)}});
   }else if(method=="nano_capellaExport"){
    if(p[2]==root('2')&&stop=="export-unconfirmed"){reply(nullptr);return;}
    if(p[2]==root('2')&&stop=="cancelled-or-deadline")return;
    reply(Json{{"available",!(p[2]==root('2')&&stop=="export-unavailable")},{"root",p[2]},{"envelope",{{"chainId",profile.chain_id},{"genesisHash",profile.genesis_hash},{"forkDigest",profile.fork_digest},{"consensusIdentity",profile.consensus_identity},{"kind","block"},{"ssz","0x0102"}}}});
   }else throw std::runtime_error("unexpected core method");
  };
  auto relay=std::make_shared<CapellaCatalogRelay>(io,core,[&](std::string,Json,Reply reply){++sends;reply(Json{{"accepted",false},{"reason","dependency-depth-limit"}});},"token","block",ingress,1,stop=="entry-limit"?1:2,[&](Json x){++done;result=std::move(x);});
  relay->start(20ms);io.run();
  check(done==1&&result.at("success")==false&&result.at("rejected")==1&&result.at("scanned")==1&&sends==1,"termination changed admission accounting");
  check(result.at("reason")=="dependency-depth-limit","first rejection lost");
  if(!result.contains("terminationReason")){std::cerr<<"MASKED TERMINATION: "<<stop<<" appears only as "<<result.at("reason")<<"; closed="<<result.at("closed")<<'\n';return 1;}
  check(result.at("terminationReason")==stop,"terminal reason does not describe stopping event");
  check(result.at("catalogExhausted")==(stop=="catalog-exhausted"),"cleanup acknowledgement confused with complete scan");
 }
 std::cout<<"5 catalog termination scenarios passed; no network qualification\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
