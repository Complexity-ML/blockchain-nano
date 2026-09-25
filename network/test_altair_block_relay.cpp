#include "altair_archive_relay.hpp"
#include <iostream>
using namespace std::chrono_literals;
void check(bool b,const char* s){if(!b)throw std::runtime_error(s);}
int main(){try{
 const AltairWireProfile profile{"0x539","0x"+std::string(64,'a'),"0x01020304",1024,256,256,256};
 const std::string id="0x"+std::string(64,'b'),epoch="0x"+std::string(64,'c');unsigned cases=0;
 for(const bool vote:{false,true})for(const std::string mode:{"success","loss","rejected","duplicate","foreign-payload","incomplete","epoch","silent","empty"}){
  asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(profile,budget);auto progress=std::make_shared<AltairRelayProgress>();unsigned sends=0,done=0;bool success=false;
  if(mode=="epoch"){progress->cursor=id;progress->epoch="0x"+std::string(64,'d');}
  auto core=[&](const std::string& method,Json params,Reply cb){
   check(params[0]=="token","auth missing");if(mode=="silent")return;
   if(method==(vote?"nano_altairVoteInventory":"nano_altairInventory")){cb(Json{{"catalogEpoch",epoch},{"missingSignedBlocks",mode=="incomplete"?1:0},{vote?"votes":"blocks",mode=="empty"?Json::array():Json::array({Json{{"root",id}}})}});return;}
   check(method==(vote?"nano_altairVote":"nano_altairBlock")&&params[1]==id,"wrong export");cb(Json{{"available",true},{"root",id},{"envelope",{{"chainId",mode=="foreign-payload"?"0x1":profile.chain_id},{"genesisHash",profile.genesis_hash},{"forkDigest",profile.fork_digest},{"kind",vote?"attestation":"block"},{"sszBase64","AQI="}}}});
  };
  auto send=[&](Json frame,Reply cb){++sends;check(frame.at("ssz")=="0x0102"&&!frame.contains("sszBase64"),"peer encoding changed");if(mode=="loss")cb(nullptr);else cb(Json{{"accepted",mode!="rejected"}});if(mode=="duplicate")cb(Json{{"accepted",false}});};
  auto job=std::make_shared<AltairArchiveRelay>(io,core,send,"token",ingress,progress,[&](Json result){++done;success=result.at("success");},vote?AltairWireKind::attestation:AltairWireKind::block);job->start(50ms);io.run();
  const bool accepted=mode=="success"||mode=="duplicate";check(done==1&&success==(accepted||mode=="empty"),"bad relay completion");check(progress->cursor==(accepted?id:"0x"),"unacknowledged cursor advanced");
  if(mode=="foreign-payload"||mode=="incomplete"||mode=="epoch"||mode=="silent"||mode=="empty")check(sends==0,"unverified export sent");++cases;
 }
 std::cout<<cases<<" Altair relay attempts verified: loss/rejection never advance progress\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
