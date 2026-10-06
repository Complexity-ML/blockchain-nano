#include "capella_attestation_delivery.hpp"
#include <iostream>
using namespace std::chrono_literals;
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
std::string root(char c){return "0x"+std::string(64,c);}
std::string encode(const std::vector<uint8_t>& b){std::string out="0x";for(auto v:b){out+="0123456789abcdef"[v>>4];out+="0123456789abcdef"[v&15];}return out;}
int main(){try{
 const AltairWireProfile p{"0x539",root('a'),"0x01020304",2048,512,256,256,root('b')};unsigned cases=0;
 for(const std::string mode:{"normal","cached","reject-block","reject-vote","missing","yield","resume","cancel-block","cancel-vote","lost-vote","duplicate","foreign","short","bad-offset","depth","lost-block"}){
  for(unsigned observer=0;observer<3;++observer){
  std::vector<Json> traces;asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(p,budget);auto blocks=std::make_shared<AltairRelayProgress>(8);
  if(mode=="cached")blocks->acknowledge(root('3'));unsigned block_sends=0,vote_sends=0,exports=0,done=0;Json result;Reply late;
  auto frame=[&](const char* kind,const std::vector<uint8_t>& bytes){return Json{{"chainId",mode=="foreign"?"0x1":p.chain_id},{"genesisHash",p.genesis_hash},{"forkDigest",p.fork_digest},{"consensusIdentity",p.consensus_identity},{"kind",kind},{"ssz",encode(bytes)}};};
  std::vector<uint8_t> vote(mode=="short"?20:229,0);vote[0]=mode=="bad-offset"?227:228;if(vote.size()>52)std::fill(vote.begin()+20,vote.begin()+52,0x33);auto attestation=frame("attestation",vote);
  auto core=[&](std::string method,Json params,Reply reply){check(method=="nano_capellaExport"&&params[1]=="block","unexpected dependency export");++exports;const auto id=params[2].get<std::string>();unsigned n=id[2]-'0';check(n>=1&&n<=3,"invalid dependency walk");if(mode=="missing"){reply(Json{{"root",id},{"available",false}});return;}
   std::vector<uint8_t> b(404,0);b[0]=100;b[180]=84;b[100]=n;std::fill(b.begin()+116,b.begin()+148,uint8_t((n-1)*17));reply(Json{{"root",id},{"available",true},{"parentRoot",root(char('0'+n-1))},{"envelope",frame("block",b)}});
  };
  auto send=[&](Json wire,Reply reply){if(wire["kind"]=="attestation"){
   ++vote_sends;check(blocks->acknowledged(root('3')),"vote sent before head admission");
   if(mode=="cancel-vote"||mode=="lost-vote"){late=reply;return;}reply(Json{{"accepted",mode!="reject-vote"}});if(mode=="duplicate")reply(Json{{"accepted",true}});
  }else{if(mode=="lost-block"){++block_sends;reply(nullptr);return;}++block_sends;const auto encoded=wire.at("ssz").get<std::string>();const auto slot=std::stoul(encoded.substr(202,2),nullptr,16);check(blocks->acknowledged(root(char('0'+slot-1)))||slot==1,"child sent before parent");if(mode=="cancel-block"){late=reply;return;}reply(Json{{"accepted",mode!="reject-block"}});if(mode=="duplicate")reply(Json{{"accepted",true}});}};
  auto quantum=std::make_shared<CapellaDeliveryQuantum>(CapellaDeliveryQuantum{mode=="yield"||mode=="resume"?2u:8u});
  auto launch=[&]{auto d=std::make_shared<CapellaAttestationDelivery>(io,core,send,"token",root('0'),attestation,ingress,blocks,mode=="depth"?2:4,8192,[&](Json r){++done;result=std::move(r);},quantum,nullptr,observer?CapellaAttestationDelivery::Trace{[&](const Json& row){traces.push_back(row);if(observer==2)throw std::runtime_error("PRIVATE_OBSERVER");}}:CapellaAttestationDelivery::Trace{});d->start(20ms);return d;};
  auto d=launch();if(mode=="cancel-block"||mode=="cancel-vote"){io.poll();check(bool(late),"cancel requires pending ACK");d->cancel();late(Json{{"accepted",true}});io.restart();}io.run();
  const bool success=mode=="normal"||mode=="cached"||mode=="duplicate";
  check(done==1,"duplicate completion");check(result==(mode=="yield"||mode=="resume"?Json{{"yielded",true}}:Json{{"accepted",success}}),"wrong outcome");
  if(mode=="yield"||mode=="resume"){check(vote_sends==0&&block_sends==2&&blocks->acknowledged(root('2'))&&!blocks->acknowledged(root('3')),"dependency quota bypass");}
  if(mode=="resume"){quantum=std::make_shared<CapellaDeliveryQuantum>(CapellaDeliveryQuantum{2});io.restart();auto resumed=launch();io.run();check(done==2&&result==Json{{"accepted",true}}&&vote_sends==1&&block_sends==3,"resume repeated dependencies or lost vote");}
  if(mode=="missing"||mode=="foreign"||mode=="short"||mode=="bad-offset")check(block_sends==0&&vote_sends==0,"invalid export sent data");
  if(mode=="reject-block"||mode=="cancel-block")check(vote_sends==0&&!blocks->acknowledged(root('1')),"unconfirmed block authorized vote");
  if(mode=="cached")check(exports==0&&block_sends==0&&vote_sends==1,"cached head retransmitted");
  if(observer){
   check(!traces.empty(),"missing observer result");
   bool peer_refusal=false,dependency_failure=false;
   for(const auto& row:traces){check(row.size()==3&&row.contains("stage")&&row.contains("outcome")&&row.contains("dependencyReason"),"unsafe trace shape");const auto text=row.dump();check(text.find("token")==std::string::npos&&text.find("PRIVATE_")==std::string::npos&&text.find(root('3'))==std::string::npos,"trace leaked payload/auth/root");peer_refusal|=row.at("stage")=="vote-ack"&&row.at("outcome")=="peer-rejected";dependency_failure|=row.at("stage")=="block-dependency"&&row.at("outcome")=="not-admitted";}
   check(peer_refusal==(mode=="reject-vote"),"local failure confused with remote vote refusal");
   if(mode=="depth")check(block_sends==0&&vote_sends==0&&traces.back()["dependencyReason"]=="dependency-depth-limit","depth failure disguised or sent");
   if(mode=="lost-block")check(block_sends==1&&vote_sends==0&&!blocks->acknowledged(root('1')),"unconfirmed dependency acknowledged");
   if(mode=="reject-block"||mode=="missing"||mode=="depth"||mode=="lost-block")check(dependency_failure&&vote_sends==0,"dependency failure not isolated");
  }else check(traces.empty(),"disabled observer emitted");++cases;
  }
 }
 std::cout<<cases<<" Capella attestation dependency scenarios passed; mock ACKs do not qualify WAN finality\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
