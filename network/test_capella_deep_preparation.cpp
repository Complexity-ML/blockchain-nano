#include "capella_head_catalog_relay.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <set>
using namespace std::chrono_literals;
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
std::string root(uint64_t n){std::ostringstream s;s<<"0x"<<std::hex<<std::setfill('0')<<std::setw(64)<<n;return s.str();}
std::string hex(const std::vector<uint8_t>& b){std::string s="0x";for(auto v:b){s+="0123456789abcdef"[v>>4];s+="0123456789abcdef"[v&15];}return s;}
int main(int argc,char** argv){try{
 const std::string mode=argc>1?argv[1]:"static";
 check(mode=="static"||mode=="moving"||mode=="reset"||mode=="refused"||mode=="anchor","invalid mode");
 const AltairWireProfile p{"0x539",root(100000),"0x01020304",2048,512,256,256,root(100001)};
 auto knowledge=std::make_shared<AltairRelayProgress>(1024);
 auto preparation=std::make_shared<CapellaDependencyPreparation>();
 auto cache=std::make_shared<CapellaExportCache>(256,16*1024*1024);
 auto selection=std::make_shared<CapellaCatalogSelection>();
 // Receiver state is independent of the bounded sender ACK cache.
 std::set<uint64_t> receiver{0};uint64_t head=mode=="anchor"?0:4096,total_exports=0,total_sends=0,max_exports=0;unsigned attempts=0,resets=0,refusals=0;bool reached=false;
 for(;attempts<4096&&!reached;++attempts){
  if(mode=="moving")++head;
  if(mode=="reset"&&attempts==200){knowledge=std::make_shared<AltairRelayProgress>(1024);preparation=std::make_shared<CapellaDependencyPreparation>();cache=std::make_shared<CapellaExportCache>(256,16*1024*1024);selection=std::make_shared<CapellaCatalogSelection>();++resets;}
  asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(p,budget);
  unsigned exports=0,sends=0,done=0;Json result;
  auto core=[&](std::string method,Json args,Reply reply){Json v;
   if(method=="nano_capellaStatus")v={{"headRoot",root(head)},{"headSlot",head}};
   else if(method=="nano_capellaExport"){
    ++exports;++total_exports;const auto id=args.at(2).get<std::string>();const auto n=std::stoull(id.substr(2),nullptr,16);check(n>0&&n<=head,"export escaped authenticated chain");
    std::vector<uint8_t> b(404);b[0]=100;b[180]=84;for(unsigned i=0;i<8;++i){b[100+i]=uint8_t(n>>(8*i));b[147-i]=uint8_t((n-1)>>(8*i));}
    v={{"root",id},{"available",true},{"parentRoot",root(n-1)},{"envelope",{{"chainId",p.chain_id},{"genesisHash",p.genesis_hash},{"forkDigest",p.fork_digest},{"consensusIdentity",p.consensus_identity},{"kind","block"},{"ssz",hex(b)}}}};
   }else if(method=="nano_capellaCatalogGeneration")v=root(100002);
   else if(method=="nano_capellaCatalogAcquire")v={{"handle",root(100003)}};
   else if(method=="nano_capellaCatalogPage")v={{"kind","block"},{"sequence","0x1"},{"digest",root(100004)},{"entries",Json::array()},{"next",nullptr}};
   else if(method=="nano_capellaCatalogRelease")v=true;
   else throw std::runtime_error("unexpected core operation");
   reply(v);
  };
  auto send=[&](Json frame,Reply reply){auto wire=frame.at("ssz").get<std::string>();uint64_t n=0;for(unsigned i=0;i<8;++i)n|=std::stoull(wire.substr(202+2*i,2),nullptr,16)<<(8*i);check(receiver.contains(n-1),"sent child before receiver admitted parent");++sends;++total_sends;
   if(mode=="refused"&&!refusals){++refusals;reply(Json{{"accepted",false}});return;}
   receiver.insert(n);reply(Json{{"accepted",true}});
  };
  auto quantum=std::make_shared<CapellaDeliveryQuantum>(CapellaDeliveryQuantum{8});
  auto relay=std::make_shared<CapellaHeadCatalogRelay>(io,core,[](std::string,Json,Reply){throw std::runtime_error("empty catalog sent payload");},send,"token",root(0),ingress,knowledge,256,16*1024*1024,8,1024,[&](Json r){++done;result=std::move(r);},[&](const std::string& id){return knowledge->acknowledged(id);},quantum,true,cache,attempts%2==1,selection,preparation);
  relay->start(10s);io.run();check(done==1,"completion duplicated or missing");check(exports<=256&&sends<=8,"bounded turn exceeded traversal/admission limit");check(knowledge->retained()<=1024,"ACK cache exceeded bound");check(cache->retained()<=256&&cache->charged_bytes()<=16*1024*1024,"authenticated export cache exceeded bounds");check(!result.value("success",false)||head==0||knowledge->acknowledged(root(head)),"claimed current head without actual ACK");if(result.contains("dependencyProgress")){const auto& d=result.at("dependencyProgress");check(d.at("exportsRequested").get<unsigned>()==exports&&d.at("sendAttempts").get<unsigned>()==sends,"dependency diagnostics disagree with independent callbacks");check(d.at("payloadsAdmitted")==quantum->admitted&&d.at("pendingHighWater").get<unsigned>()<=256,"dependency diagnostics ACK or queue count incorrect");}check(result.at("selectedHeadAcknowledged")==knowledge->acknowledged(root(head)),"head diagnostic invented ACK");max_exports=std::max<uint64_t>(max_exports,exports);check(result.at("selectedHeadIsConfiguredAnchor")== (head==0),"configured anchor diagnostic incorrect");reached=head==0||knowledge->acknowledged(root(head));
 }
 std::cout<<Json{{"mode",mode},{"attempts",attempts},{"head",head},{"exports",total_exports},{"sends",total_sends},{"uniqueReceiverBlocks",receiver.size()-1},{"maximumExportsPerTurn",max_exports},{"acknowledgedRoots",knowledge->retained()},{"sessionResets",resets},{"refusals",refusals},{"currentHeadAcknowledged",knowledge->acknowledged(root(head))},{"configuredAnchor",head==0},{"scope","modeled receiver; empty historical catalog; not WAN qualification"}}.dump()<<'\n';
 check(reached,"bounded repeated turns did not catch moving or deep head");check(receiver.size()-1==head,"receiver missing ancestral blocks");
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
