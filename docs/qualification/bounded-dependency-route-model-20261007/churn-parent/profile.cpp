#include "capella_head_catalog_relay.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
using namespace std::chrono_literals;
static void require(bool b,const char* m){if(!b)throw std::runtime_error(m);}
static std::string id(unsigned n){std::ostringstream out;out<<"0x"<<std::setfill('0')<<std::setw(64)<<std::hex<<n;return out.str();}
static std::string hex(const std::vector<uint8_t>& b){std::string s="0x";for(auto v:b){s+="0123456789abcdef"[v>>4];s+="0123456789abcdef"[v&15];}return s;}
int main(int argc,char** argv){try{
 const unsigned length=argc>1?std::stoul(argv[1]):513;require(length>=1&&length<=4096,"invalid fixture chain length");
 const AltairWireProfile p{"0x539",id(10000),"0x01020304",2048,512,256,256,id(10001)};
 auto knowledge=std::make_shared<AltairRelayProgress>(1024);auto preparation=std::make_shared<CapellaDependencyPreparation>();auto cache=std::make_shared<CapellaExportCache>(256,16*1024*1024);
 size_t cache_hits=0;size_t total_exports=0,total_sends=0,attempts=0,acknowledged=0;Json results=Json::array();
 for(;attempts<1024&&!knowledge->acknowledged(id(length));++attempts){
  for(unsigned i=0;i<256;++i)cache->remember(id(100000+attempts*256+i),"{}",id(0));
  asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(p,budget);Json last;unsigned completed=0,exports=0;
  auto core=[&](std::string method,Json args,Reply reply){Json v;
   if(method=="nano_capellaStatus")v={{"headRoot",id(length)},{"headSlot",uint64_t(length)}};
   else if(method=="nano_capellaExport"){
    ++exports;++total_exports;const auto root=args.at(2).get<std::string>();const auto n=std::stoul(root.substr(2),nullptr,16);require(n&&n<=length,"root escaped fixture");
    std::vector<uint8_t> bytes(404);bytes[0]=100;bytes[180]=84;for(unsigned i=0;i<8;++i)bytes[100+i]=uint8_t(uint64_t(n)>>(8*i));
    const auto parent=id(n-1);for(unsigned i=0;i<32;++i)bytes[116+i]=uint8_t(std::stoul(parent.substr(2+i*2,2),nullptr,16));
    v={{"root",root},{"available",true},{"parentRoot",parent},{"envelope",{{"chainId",p.chain_id},{"genesisHash",p.genesis_hash},{"forkDigest",p.fork_digest},{"consensusIdentity",p.consensus_identity},{"kind","block"},{"ssz",hex(bytes)}}}};
   }else if(method=="nano_capellaCatalogGeneration")v=id(10002);
   else if(method=="nano_capellaCatalogAcquire")v={{"handle",id(10003)}};
   else if(method=="nano_capellaCatalogPage")v={{"kind","block"},{"sequence","0x1"},{"digest",id(10004)},{"entries",Json::array()},{"next",nullptr}};
   else if(method=="nano_capellaCatalogRelease")v=true;
   else throw std::runtime_error("unexpected method");reply(v);
  };
  auto send=[&](Json frame,Reply reply){const auto wire=frame.at("ssz").get<std::string>();uint64_t n=0;for(unsigned i=0;i<8;++i)n|=uint64_t(std::stoul(wire.substr(202+i*2,2),nullptr,16))<<(8*i);require(n==acknowledged+1,"child sent before acknowledged parent");++acknowledged;++total_sends;reply(Json{{"accepted",true}});};
  auto quantum=std::make_shared<CapellaDeliveryQuantum>(CapellaDeliveryQuantum{8});
  auto relay=std::make_shared<CapellaHeadCatalogRelay>(io,core,[](std::string,Json,Reply){throw std::runtime_error("empty catalog sent");},send,"token",id(0),ingress,knowledge,256,16*1024*1024,8,1024,[&](Json x){last=std::move(x);++completed;},[&](const std::string& root){return knowledge->acknowledged(root);},quantum,true,cache,false,nullptr,preparation);
  relay->start(5s);io.run();require(completed==1,"completion missing");require(quantum->admitted<=8,"quantum enlarged");if(last.value("success",false))require(knowledge->acknowledged(id(length)),"false success");
  if(last.contains("dependencyProgress"))cache_hits+=last["dependencyProgress"].value("exportCacheHits",0u);
  results.push_back({{"turn",attempts+1},{"exports",exports},{"admitted",quantum->admitted},{"acknowledged",acknowledged}});
 }
 require(knowledge->acknowledged(id(length))&&acknowledged==length&&total_sends==length,"did not catch up with real fixture ACKs");
 std::cout<<Json{{"mode","modeled receiver ACKs with independent full export-cache churn between turns"},{"cacheHits",cache_hits},{"chainLength",length},{"dependencyBlocks",256},{"admissionQuantum",8},{"acknowledgedRoots",1024},{"maximumEntries",1024},{"exportCacheRecords",256},{"turns",attempts},{"exportsRequested",total_exports},{"payloadsAdmitted",acknowledged},{"perTurn",results},{"wanQualified",false},{"globalCauseProven",false}}.dump()<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
