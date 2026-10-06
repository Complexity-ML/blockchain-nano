#include <capella_head_catalog_relay.hpp>
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
 for(bool cancel:{false,true}){
  asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(profile,budget);auto knowledge=std::make_shared<AltairRelayProgress>(8);Json result;Reply late;unsigned done=0,sends=0;
  auto core=[&](std::string method,Json p,Reply reply){
   if(method=="nano_capellaCatalogGeneration")reply(root('e'));
   else if(method=="nano_capellaCatalogAcquire")reply(Json{{"handle",root('c')}});
   else if(method=="nano_capellaCatalogRelease")reply(true);
   else if(method=="nano_capellaCatalogPage")reply(Json{{"kind","block"},{"sequence","0x1"},{"digest",root('d')},{"entries",Json::array({root('1'),root('2')})},{"next",nullptr}});
   else if(method=="nano_capellaExport")reply(Json{{"available",true},{"root",p[2]},{"envelope",{{"chainId",profile.chain_id},{"genesisHash",profile.genesis_hash},{"forkDigest",profile.fork_digest},{"consensusIdentity",profile.consensus_identity},{"kind","block"},{"ssz","0x0102"}}}});
   else throw std::runtime_error("history timeout incorrectly requested source head");
  };
  auto relay=std::make_shared<CapellaHeadCatalogRelay>(io,core,[&](std::string id,Json,Reply reply){++sends;if(id==root('1'))reply(Json{{"accepted",false},{"reason","dependency-depth-limit"}});else late=reply;},[](Json,Reply){throw std::runtime_error("history timeout incorrectly sent head");},"token",root('0'),ingress,knowledge,4,8192,2,8,[&](Json x){++done;result=std::move(x);},[](const std::string&){return false;},nullptr,false,nullptr,true);
  relay->start(20ms);if(cancel){io.poll();check(bool(late),"cancel fixture did not reach second delivery");relay->cancel();io.restart();}io.run();
  check(done==1&&sends==2&&result.at("success")==false&&result.at("closed")==false,"wrapper claimed success or release after interrupted history");
  const auto& progress=result.at("catalogProgress");
  check(progress.at("scanned")==1&&progress.at("rejected")==1&&progress.at("exports")==2&&progress.at("payloadsAdmitted")==0&&progress.at("firstFailureReason")=="dependency-depth-limit"&&progress.at("inProgress")==true&&!progress.at("catalogExhausted").get<bool>(),"wrapper erased counters or reclassified rejected/unknown delivery");
  check(knowledge->retained()==0,"interrupted history invented ACK knowledge");late(Json{{"accepted",true}});io.restart();io.run();check(done==1&&knowledge->retained()==0,"late reply altered finished owner or ACK knowledge");
 }
 std::cout<<"7 catalog/wrapper termination scenarios passed; no network qualification\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
