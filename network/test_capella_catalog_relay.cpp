#include "capella_catalog_relay.hpp"
#include <iostream>
using namespace std::chrono_literals;
void check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
std::string root(char c){return "0x"+std::string(64,c);}
int main(){try{
 const AltairWireProfile profile{"0x539",root('a'),"0x01020304",1024,256,256,256,root('b')};unsigned scenarios=0;
 for(const std::string mode:{"success","lost-open","duplicate","empty","changed-snapshot","wrong-next","unsorted","unavailable","foreign","rejected","lost-ack","lost-close","close-applied-reply-lost","close-request-lost","close-unreachable","close-refused","limit","expired"}){
  asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(profile,budget);unsigned pages=0,sends=0,closes=0,done=0;Json result;bool allocated=false;unsigned removals=0;
  auto core=[&](std::string method,Json p,Reply cb){check(p[0]=="token","missing authentication");Json value;
   if(method=="nano_capellaCatalogGeneration"){cb(root('e'));return;}
   if(method=="nano_capellaCatalogAcquire"){allocated=true;if(mode=="lost-open"){cb(nullptr);return;}value={{"handle",root('c')}};}
   else if(method=="nano_capellaCatalogRelease"){++closes;check(p[1]==root('e')&&p[2].get<std::string>().size()==66,"wrong release request");if(mode=="lost-close")return;
    if(mode=="close-unreachable"||(mode=="close-request-lost"&&closes==1)){cb(nullptr);return;}
    if(mode=="close-refused"){cb(false);return;}
    if(allocated){allocated=false;++removals;}
    if(mode=="close-applied-reply-lost"&&closes==1){cb(nullptr);return;}value=true;}
   else if(method=="nano_capellaCatalogPage"){
    ++pages;check(p[1]==root('c'),"wrong page handle");if(mode=="expired"){cb(nullptr);return;}
    check((pages==1&&p[2].is_null())||(pages==2&&p[2]==root('2')),"cursor did not follow acknowledged page");
    value={{"sequence",mode=="changed-snapshot"&&pages==2?"0x2":"0x1"},{"digest",root('d')},{"kind","block"},{"entries",pages==1?Json::array({root('1'),root('2')}):Json::array({root('3')})},{"next",pages==1?Json(root('2')):Json(nullptr)}};
    if(mode=="empty"){value["entries"]=Json::array();value["next"]=nullptr;}if(mode=="wrong-next")value["next"]=root('4');if(mode=="unsorted")value["entries"]=Json::array({root('2'),root('1')});
   }else if(method=="nano_capellaExport")value={{"available",mode!="unavailable"},{"root",p[2]},{"envelope",{{"chainId",mode=="foreign"?"0x1":profile.chain_id},{"genesisHash",profile.genesis_hash},{"forkDigest",profile.fork_digest},{"consensusIdentity",profile.consensus_identity},{"kind","block"},{"ssz","0x0102"}}}};
   else throw std::runtime_error("unexpected RPC");cb(value);if(mode=="duplicate")cb(value);
  };
  auto deliver=[&](std::string id,Json envelope,Reply cb){++sends;check(id==root(char('0'+sends))&&envelope["ssz"]=="0x0102","wrong payload");if(mode=="lost-ack")return;cb(Json{{"accepted",mode!="rejected"}});if(mode=="duplicate")cb(Json{{"accepted",true}});};
  auto relay=std::make_shared<CapellaCatalogRelay>(io,core,deliver,"token","block",ingress,2,mode=="limit"?2:4,[&](Json value){++done;result=std::move(value);});relay->start(20ms);io.run();
  const bool success=mode=="success"||mode=="duplicate"||mode=="empty"||mode=="close-applied-reply-lost"||mode=="close-request-lost";const bool retried=mode=="close-applied-reply-lost"||mode=="close-request-lost"||mode=="close-unreachable";check(done==1&&result["success"]==success&&closes==(retried?2:1),"incorrect completion/cleanup");
  if(mode=="success"||mode=="duplicate")check(sends==3&&pages==2&&result["closed"]==true,"pagination failed");
  if(mode=="lost-open")check(removals==1&&!allocated&&sends==0&&result["closed"]==true,"lost open reply leaked snapshot");
  if(mode=="close-applied-reply-lost"||mode=="close-request-lost")check(removals==1&&!allocated&&result["closed"]==true&&sends==3,"close retry leaked or replayed delivery");
  if(mode=="close-unreachable"||mode=="close-refused")check(allocated&&result["closed"]==false,"unconfirmed close reported success");
  if(mode=="limit")check(sends==2&&pages==1,"entry limit exceeded");if(mode=="lost-ack")check(result["delivered"]==0,"unacknowledged delivery counted");++scenarios;
 }
 // Open may allocate a handle after cancellation: close the late handle once.
 for(const bool late_open:{true,false}){
  asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(profile,budget);Reply delayed;unsigned closes=0,done=0;Json result;
  auto core=[&](std::string method,Json,Reply cb){if(method=="nano_capellaCatalogGeneration"){cb(root('e'));return;}
   if(method=="nano_capellaCatalogAcquire"){if(late_open)delayed=cb;else cb(Json{{"handle",root('c')}});}else if(method=="nano_capellaCatalogRelease"||method=="nano_capellaCatalogClose"){++closes;cb(true);}else delayed=cb;};
  auto relay=std::make_shared<CapellaCatalogRelay>(io,core,[](auto,auto,auto){throw std::runtime_error("delivery after cancellation");},"token","block",ingress,1,2,[&](Json value){++done;result=value;});relay->start(20ms);io.poll();relay->cancel();check(bool(delayed),"missing delayed call");delayed(Json{{"handle",root('c')}});delayed(Json{{"handle",root('c')}});io.restart();io.run();check(done==1&&result["success"]==false&&closes==(late_open?2:1),"late callback resurrected/leaked catalog");++scenarios;
 }
 for(bool sync:{false,true})for(bool wrong_id:{false,true}){
  asio::io_context io;InboundBudget budget;auto capable=profile;capable.bls_changes=true;capable.sync_messages=true;const std::string kind=sync?"sync_committee_message":"bls_to_execution_change";auto ingress=std::make_shared<AltairMessageIngress>(capable,budget);std::vector<uint8_t> bytes(sync?144:172,7);unsigned char digest[32];SHA256(bytes.data(),bytes.size(),digest);std::string id="0x",wire="0x";for(auto b:digest){id+="0123456789abcdef"[b>>4];id+="0123456789abcdef"[b&15];}for(auto b:bytes){wire+="0123456789abcdef"[b>>4];wire+="0123456789abcdef"[b&15];}if(wrong_id)id=root('1');unsigned delivered=0,closed=0;Json result;
  auto core=[&](std::string method,Json args,Reply reply){if(method=="nano_capellaCatalogGeneration")reply(root('e'));else if(method=="nano_capellaCatalogAcquire"){check(args[1]==kind,"wrong BLS catalog requested");reply(Json{{"handle",root('c')}});}else if(method=="nano_capellaCatalogPage")reply(Json{{"kind",kind},{"sequence","0x1"},{"digest",root('d')},{"entries",Json::array({id})},{"next",nullptr}});else if(method=="nano_capellaExport")reply(Json{{"available",true},{"root",id},{"envelope",{{"chainId",capable.chain_id},{"genesisHash",capable.genesis_hash},{"forkDigest",capable.fork_digest},{"consensusIdentity",capable.consensus_identity},{"kind",kind},{"ssz",wire}}}});else if(method=="nano_capellaCatalogRelease"){++closed;reply(true);}else throw std::runtime_error("unexpected BLS catalog method");};
  auto deliver=[&](std::string actual,Json frame,Reply reply){check(actual==id&&frame.at("ssz")==wire,"changed BLS catalog payload");++delivered;reply(Json{{"accepted",true}});};auto relay=std::make_shared<CapellaCatalogRelay>(io,core,deliver,"token",kind,ingress,1,1,[&](Json value){result=std::move(value);});relay->start(1s);io.run();check(result.at("success")==!wrong_id&&delivered==(wrong_id?0:1)&&closed==1,"BLS catalog identity or cleanup failed");++scenarios;
 }
 std::cout<<scenarios<<" Capella catalog relay scenarios passed; no consensus validity inferred from mocked ACKs\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
