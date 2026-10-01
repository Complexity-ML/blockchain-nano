#include "capella_block_delivery.hpp"
#include <iostream>
using namespace std::chrono_literals;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
std::string root(char c){return "0x"+std::string(64,c);}
int main(){try{
 const AltairWireProfile profile{"0x539",root('a'),"0x01020304",2048,512,256,256,root('b')};unsigned cases=0;
 for(const std::string mode:{"normal","cached","duplicate","missing","bad-parent","backward-slot","bad-offset","foreign","depth","bytes","reject","lost-ack","cancel","new-anchor","anchor-data-present","unknown-anchor","anchor-root-mismatch","anchor-refused","anchor-target"}){
  asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(profile,budget);auto knowledge=std::make_shared<AltairRelayProgress>(8);if(mode=="cached")knowledge->acknowledge(root('2'));
  unsigned exports=0,sends=0,done=0;Json result;Reply late;
  auto core=[&](std::string method,Json params,Reply cb){check(method=="nano_capellaExport"&&params[0]=="token"&&params[1]=="block","bad export request");++exports;const auto id=params[2].get<std::string>();const unsigned n=unsigned(id[2]-'0');check(n>=1&&n<=3,"walk outside chain");
   if((n==2&&(mode=="new-anchor"||mode=="anchor-data-present"||mode=="unknown-anchor"||mode=="anchor-root-mismatch"||mode=="anchor-refused"))||(n==3&&mode=="anchor-target")){
    cb(Json{{"available",mode=="anchor-data-present"},{"root",mode=="anchor-root-mismatch"?root('f'):id},{"known",mode!="unknown-anchor"},{"anchor",true}});return;
   }
   if(mode=="missing"&&n==2){cb(Json{{"available",false},{"root",id}});return;}
   std::vector<uint8_t> bytes(404,0);bytes[0]=mode=="bad-offset"?99:100;bytes[180]=84;bytes[100]=mode=="backward-slot"?3:n;std::fill(bytes.begin()+116,bytes.begin()+148,uint8_t((n-1)*17));std::string wire="0x";const char* h="0123456789abcdef";for(auto b:bytes){wire+=h[b>>4];wire+=h[b&15];}
   Json value{{"available",true},{"root",id},{"parentRoot",root(mode=="bad-parent"?'f':char('0'+n-1))},{"envelope",{{"chainId",mode=="foreign"?"0x1":profile.chain_id},{"genesisHash",profile.genesis_hash},{"forkDigest",profile.fork_digest},{"consensusIdentity",profile.consensus_identity},{"kind","block"},{"ssz",wire}}}};
   cb(value);if(mode=="duplicate")cb(value);
  };
  auto send=[&](Json frame,Reply cb){++sends;const auto encoded=frame.at("ssz").get<std::string>();const unsigned slot=std::stoul(encoded.substr(202,2),nullptr,16);check(slot==((mode=="cached"||mode=="new-anchor"||mode=="anchor-data-present"||mode=="anchor-refused")?3:sends),"child delivered before ancestor");if(mode=="lost-ack"||mode=="cancel"){late=cb;return;}cb(Json{{"accepted",mode!="reject"&&mode!="anchor-refused"}});if(mode=="duplicate")cb(Json{{"accepted",true}});};
  auto delivery=std::make_shared<CapellaBlockDelivery>(io,core,send,"token",root('0'),root('3'),ingress,knowledge,mode=="depth"?2:4,mode=="bytes"?32:8192,[&](Json value){++done;result=value;});delivery->start(20ms);
  if(mode=="cancel"){io.poll();check(bool(late),"missing delayed ACK");delivery->cancel();late(Json{{"accepted",true}});io.restart();}io.run();
  const bool ok=mode=="normal"||mode=="cached"||mode=="duplicate"||mode=="new-anchor"||mode=="anchor-data-present";check(done==1&&result.at("accepted")==ok,"wrong completion");check(knowledge->acknowledged(root('3'))==ok,"cached unconfirmed target");if(ok)check(sends==((mode=="cached"||mode=="new-anchor"||mode=="anchor-data-present")?1:3),"wrong deliveries");if(mode=="missing"||mode=="depth"||mode=="bytes")check(sends==0,"partial send before preparation failure");if(mode=="new-anchor"||mode=="anchor-data-present"){check(exports==2&&!knowledge->acknowledged(root('2')),"inferred remote acknowledgement for local anchor");}++cases;
 }
 // Dependencies share the catalog budget; a local yield is not a target ACK.
 for(const std::string mode:{"resume","shared-targets","exact-boundary","rejected","lost-ack","cancel-late"}){
  auto knowledge=std::make_shared<AltairRelayProgress>(8);unsigned total=0;
  auto run=[&](char target,std::shared_ptr<CapellaDeliveryQuantum> quantum){
   asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(profile,budget);Json result;Reply late;unsigned done=0;
   auto core=[&](std::string method,Json params,Reply reply){
    check(method=="nano_capellaExport","unexpected quantum export");const auto id=params[2].get<std::string>();unsigned n=id[2]-'0';check(n>=1&&n<=3,"quantum walk escaped chain");
    std::vector<uint8_t> bytes(404,0);bytes[0]=100;bytes[180]=84;bytes[100]=n;std::fill(bytes.begin()+116,bytes.begin()+148,uint8_t((n-1)*17));std::string wire="0x";for(auto b:bytes){wire+="0123456789abcdef"[b>>4];wire+="0123456789abcdef"[b&15];}
    reply(Json{{"available",true},{"root",id},{"parentRoot",root(char('0'+n-1))},{"envelope",{{"chainId",profile.chain_id},{"genesisHash",profile.genesis_hash},{"forkDigest",profile.fork_digest},{"consensusIdentity",profile.consensus_identity},{"kind","block"},{"ssz",wire}}}});
   };
   auto send=[&](Json frame,Reply reply){unsigned slot=std::stoul(frame.at("ssz").get<std::string>().substr(202,2),nullptr,16);check(!knowledge->acknowledged(root(char('0'+slot))),"yield retransmitted acknowledged ancestor");++total;if(mode=="lost-ack"||mode=="cancel-late"){late=reply;return;}reply(Json{{"accepted",mode!="rejected"}});};
   auto d=std::make_shared<CapellaBlockDelivery>(io,core,send,"token",root('0'),root(target),ingress,knowledge,4,8192,[&](Json r){++done;result=std::move(r);},quantum);d->start(20ms);
   if(mode=="cancel-late"){io.poll();check(bool(late),"no pending quantum ACK");d->cancel();late(Json{{"accepted",true}});io.restart();}io.run();check(done==1,"quantum completion duplicated");return result;
  };
  auto quantum=std::make_shared<CapellaDeliveryQuantum>(CapellaDeliveryQuantum{mode=="exact-boundary"?3u:2u});
  if(mode=="shared-targets"){
   check(run('2',quantum)==Json{{"accepted",true}}&&quantum->admitted==2,"shared budget lost successful target");
   check(run('3',quantum)==Json{{"yielded",true}}&&total==2&&!knowledge->acknowledged(root('3')),"second target exceeded shared quota");
  }else{
   auto result=run('3',quantum);
   if(mode=="resume")check(result==Json{{"yielded",true}}&&quantum->admitted==2&&total==2&&!knowledge->acknowledged(root('3')),"dependency budget not enforced");
   else if(mode=="exact-boundary")check(result==Json{{"accepted",true}}&&quantum->admitted==3&&total==3,"exact completed target wrongly yielded");
   else check(result==Json{{"accepted",false}}&&quantum->admitted==0&&knowledge->retained()==0&&total==1,"unconfirmed/rejected/late ACK consumed quota or seeded knowledge");
  }
  if(mode=="resume"||mode=="shared-targets"){
   auto next=std::make_shared<CapellaDeliveryQuantum>(CapellaDeliveryQuantum{2});check(run('3',next)==Json{{"accepted",true}}&&next->admitted==1&&total==3&&knowledge->acknowledged(root('3')),"dependency yield failed to resume exactly once");
  }++cases;
 }
 std::cout<<cases<<" Capella ancestor delivery scenarios passed; mocked ACKs do not qualify consensus\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
