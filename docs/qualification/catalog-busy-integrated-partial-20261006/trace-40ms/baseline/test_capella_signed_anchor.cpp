#include "capella_block_delivery.hpp"
#include <iostream>
#include <set>
using namespace std::chrono_literals;
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
std::string root(unsigned n){const char* h="0123456789abcdef";return "0x"+std::string(62,'0')+h[n>>4]+h[n&15];}
int main(){try{
 unsigned cases=0;
 for(const std::string mode:{"shared","cached","probe-lost","probe-cancel","refused-bounded","fallback","fallback-anchor-refused","quantum-resume","bad-anchor-frame","cache-refused","unknown-anchor-metadata"}){
  asio::io_context io;InboundBudget budget;AltairWireProfile profile{"0x539",root(200),"0x01020304",2048,512,256,256,root(201)};auto ingress=std::make_shared<AltairMessageIngress>(profile,budget);
  auto knowledge=std::make_shared<AltairRelayProgress>(128);auto cache=std::make_shared<CapellaExportCache>(128,262144);const bool small=mode=="fallback"||mode=="fallback-anchor-refused";const unsigned anchor=small?2:80,target=anchor+1;
  std::set<unsigned> peer{0};if(mode!="refused-bounded"&&mode!="fallback"&&mode!="fallback-anchor-refused"&&mode!="cache-refused")peer.insert(anchor);
  unsigned exports=0,done=0;std::vector<unsigned> sent;Json result;Reply late;
  auto value=[&](unsigned n){std::vector<uint8_t> b(404);b[0]=100;b[180]=84;b[100]=n;b[147]=n-1;std::string wire="0x";for(auto v:b){wire+="0123456789abcdef"[v>>4];wire+="0123456789abcdef"[v&15];}
   return Json{{"root",root(n)},{"available",true},{"known",mode!="unknown-anchor-metadata"},{"anchor",n==anchor},{"parentRoot",root(n-1)},{"envelope",{{"chainId",mode=="bad-anchor-frame"&&n==anchor?"0x1":profile.chain_id},{"genesisHash",profile.genesis_hash},{"forkDigest",profile.fork_digest},{"consensusIdentity",profile.consensus_identity},{"kind","block"},{"ssz",wire}}}};};
  if(mode=="cached"||mode=="cache-refused"){auto v=value(anchor);cache->remember(root(anchor),v["envelope"].dump(),root(anchor-1),true);}
  auto core=[&](std::string method,Json p,Reply cb){check(method=="nano_capellaExport","unexpected RPC");unsigned n=std::stoul(p[2].get<std::string>().substr(64),nullptr,16);check(n&&n<=target,"walk outside history");++exports;cb(value(n));};
  auto send=[&](Json frame,Reply cb){unsigned n=std::stoul(frame["ssz"].get<std::string>().substr(202,2),nullptr,16);sent.push_back(n);
   if(mode=="probe-lost"){cb(nullptr);return;}if(mode=="probe-cancel"){late=cb;return;}
   const bool accepted=(peer.contains(n)||peer.contains(n-1))&&!(mode=="fallback-anchor-refused"&&n==anchor);if(accepted)peer.insert(n);cb(Json{{"accepted",accepted}});
  };
  auto quantum=std::make_shared<CapellaDeliveryQuantum>(CapellaDeliveryQuantum{mode=="quantum-resume"?1u:128u});
  auto run=[&]{auto d=std::make_shared<CapellaBlockDelivery>(io,core,send,"token",root(0),root(target),ingress,knowledge,8,32768,[&](Json v){++done;result=v;},quantum,cache);d->start(100ms);return d;};auto d=run();
  if(mode=="probe-cancel"){io.poll();check(bool(late),"probe ACK not delayed");d->cancel();late(Json{{"accepted",true}});io.restart();}io.run();check(done==1,"duplicate completion");
  if(mode=="shared"||mode=="cached")check(result==Json{{"accepted",true}}&&sent==std::vector<unsigned>{anchor,target}&&knowledge->acknowledged(root(anchor))&&knowledge->acknowledged(root(target))&&exports==(mode=="cached"?1u:2u),"shared/cached anchor skipped required ACK");
  else if(mode=="fallback")check(result==Json{{"accepted",true}}&&sent==std::vector<unsigned>{2,1,2,3}&&quantum->admitted==3&&knowledge->acknowledged(root(target)),"definitive refusal did not prepare older parents");
  else if(mode=="fallback-anchor-refused")check(result==Json{{"accepted",false}}&&sent==std::vector<unsigned>{2,1,2}&&!knowledge->acknowledged(root(anchor))&&!knowledge->acknowledged(root(target))&&quantum->admitted==1,"rejected probe forged ACK or repeated forever");
  else if(mode=="quantum-resume"){
   check(result==Json{{"yielded",true}}&&knowledge->acknowledged(root(anchor))&&!knowledge->acknowledged(root(target))&&sent.size()==1&&quantum->admitted==1,"anchor bypassed shared quantum");
   quantum=std::make_shared<CapellaDeliveryQuantum>(CapellaDeliveryQuantum{1});io.restart();d=run();io.run();check(done==2&&result==Json{{"accepted",true}}&&sent==std::vector<unsigned>{anchor,target}&&quantum->admitted==1,"yield lost anchor ACK or retried it");
  }else{
   check(result["accepted"]==false&&!knowledge->acknowledged(root(anchor))&&!knowledge->acknowledged(root(target))&&quantum->admitted==0,"failed/unconfirmed probe seeded knowledge");
   if(mode=="probe-lost"||mode=="probe-cancel")check(sent.size()==1&&exports==2,"ambiguous ACK retried probe or parents");
   if(mode=="refused-bounded"||mode=="cache-refused")check(sent.size()==1&&exports<=8,"refused probe escaped preparation bound");
   if(mode=="bad-anchor-frame"||mode=="unknown-anchor-metadata")check(sent.empty(),"invalid/unproven anchor sent");
  }++cases;
 }
 std::cout<<cases<<" signed-anchor ACK/fallback/cache/quota scenarios passed; modeled peer admission, not WAN qualification\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
