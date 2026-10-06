#include "capella_block_delivery.hpp"
#include <iostream>
using namespace std::chrono_literals;
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
std::string root(unsigned n){const char* h="0123456789abcdef";return "0x"+std::string(62,'0')+h[n>>4]+h[n&15];}
int main(){try{
 asio::io_context io;InboundBudget budget;AltairWireProfile profile{"0x539",root(200),"0x01020304",2048,512,256,256,root(201)};
 auto ingress=std::make_shared<AltairMessageIngress>(profile,budget);auto knowledge=std::make_shared<AltairRelayProgress>(32);unsigned exports=0,sends=0,done=0;Json result;
 auto core=[&](std::string method,Json p,Reply cb){check(method=="nano_capellaExport","unexpected RPC");unsigned n=std::stoul(p[2].get<std::string>().substr(64),nullptr,16);check(n>0&&n<=81,"walk escaped local history");++exports;
  std::vector<uint8_t> b(404);b[0]=100;b[180]=84;b[100]=n;b[147]=n-1;std::string wire="0x";for(auto v:b){wire+="0123456789abcdef"[v>>4];wire+="0123456789abcdef"[v&15];}
  cb(Json{{"root",root(n)},{"available",true},{"known",true},{"anchor",n==80},{"parentRoot",root(n-1)},{"envelope",{{"chainId",profile.chain_id},{"genesisHash",profile.genesis_hash},{"forkDigest",profile.fork_digest},{"consensusIdentity",profile.consensus_identity},{"kind","block"},{"ssz",wire}}}});
 };
 bool peer_has_anchor=true;auto send=[&](Json frame,Reply cb){++sends;unsigned n=std::stoul(frame["ssz"].get<std::string>().substr(202,2),nullptr,16);check(n==80||n==81,"sent unnecessary pre-anchor ancestor");cb(Json{{"accepted",peer_has_anchor&&(n==80||knowledge->acknowledged(root(80)))}});};
 auto d=std::make_shared<CapellaBlockDelivery>(io,core,send,"token",root(0),root(81),ingress,knowledge,8,32768,[&](Json value){++done;result=value;});d->start(100ms);io.run();
 check(done==1&&result==Json{{"accepted",true}}&&sends==2&&exports==2&&knowledge->acknowledged(root(80))&&knowledge->acknowledged(root(81)),"SIGNED ANCHOR STARVATION: shared authenticated anchor did not seed peer ACK before ancestor depth bound");
 std::cout<<"signed common-anchor modeled delivery passed; no consensus or WAN qualification\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
