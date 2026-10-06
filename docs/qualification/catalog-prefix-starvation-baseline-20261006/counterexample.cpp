#include <capella_catalog_relay.hpp>
#include <iostream>
using namespace std::chrono_literals;
static std::string root(char c){return "0x"+std::string(64,c);}
static void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(){try{
 const AltairWireProfile profile{"0x539",root('a'),"0x01020304",1024,256,256,256,root('b')};
 unsigned target_admissions=0;Json observations=Json::array();
 // Each fresh inventory has valid roots 1..4. The first two are definitively
 // rejected; exporting 3 has an unknown outcome. Root 4 is independently valid.
 // No rejection/unknown response may become ACK knowledge. Across attempts,
 // this local transport failure must not permanently starve unrelated root 4.
 for(unsigned attempt=0;attempt<8;++attempt){
  asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(profile,budget);Json result,exported=Json::array();unsigned releases=0,done=0;
  auto core=[&](std::string method,Json p,Reply reply){
   if(method=="nano_capellaCatalogGeneration")reply(root('e'));
   else if(method=="nano_capellaCatalogAcquire")reply(Json{{"handle",root('c')}});
   else if(method=="nano_capellaCatalogRelease"){++releases;reply(true);}
   else if(method=="nano_capellaCatalogPage"){
    Json entries=Json::array();for(char c='1';c<='4';++c)if(p[2].is_null()||root(c)>p[2].get<std::string>())entries.push_back(root(c));
    reply(Json{{"kind","block"},{"sequence","0x1"},{"digest",root('d')},{"entries",entries},{"next",nullptr}});
   }else if(method=="nano_capellaExport"){
    exported.push_back(p[2]);if(p[2]==root('3')){reply(nullptr);return;}
    reply(Json{{"available",true},{"root",p[2]},{"envelope",{{"chainId",profile.chain_id},{"genesisHash",profile.genesis_hash},{"forkDigest",profile.fork_digest},{"consensusIdentity",profile.consensus_identity},{"kind","block"},{"ssz","0x0102"}}}});
   }else throw std::runtime_error("unexpected method");
  };
  auto relay=std::make_shared<CapellaCatalogRelay>(io,core,[&](std::string id,Json,Reply reply){if(id==root('4')){++target_admissions;reply(Json{{"accepted",true}});}else reply(Json{{"accepted",false}});},"token","block",ingress,4,8,[&](Json x){++done;result=std::move(x);},[](const auto&){return false;});
  relay->start(100ms);io.run();check(done==1&&releases==1,"attempt not terminal or snapshot release missing");
  observations.push_back(Json{{"attempt",attempt},{"exports",exported},{"result",result}});
 }
 std::cout<<Json{{"scenario","repeated-interrupted-catalog-prefix"},{"attempts",8},{"laterRootAdmissions",target_admissions},{"observations",observations},{"transportModeled",true},{"networkQualified",false}}.dump(2)<<'\n';
 check(target_admissions>0,"later valid root starved behind repeatedly interrupted unacknowledged prefix");
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
