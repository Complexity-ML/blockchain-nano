#include "peer_status.hpp"
#include "altair_peer.hpp"
#include <iostream>
using Json=nlohmann::json;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}

// Dependent lookup keeps this new test compilable against the parent header:
// its malformed best_hash case fails first, rather than failing at compilation.
template<class Status> void status_projection_case(const Status& status){
 if constexpr(requires { project_peer_status(status); }){
  auto projected=project_peer_status(status),expected=status;expected.erase("futureExtension");
  check(projected==expected&&!projected.contains("futureExtension"),"unknown status extension retained");
  for(const auto* field:{"altair","bellatrix","capella"}){
   const auto protocol="/nano/"+std::string(field)+"/1.0.0";
   check(compatible_altair_peer(projected,status,field,protocol),"projection changed advertised consensus compatibility");
  }
  auto ambiguous=status;ambiguous["capella"]["extra"]=true;auto retained=project_peer_status(ambiguous);
  check(retained["capella"]==ambiguous["capella"]&&!compatible_altair_peer(retained,projected,"capella","/nano/capella/1.0.0"),"projection sanitized an ambiguous advertisement into compatibility");
 }else check(false,"status projection unavailable");
}
void status_bounds_cases(){
 auto local=Json::parse(R"({"chain_id":1337,"best_block":0})");local["genesis_hash"]="0x"+std::string(64,'a');
 for(const auto& value:std::vector<Json>{std::string(8192,'a'),nullptr,true,0,Json::object(),Json::array(),"0x",std::string(66,'g'),"0x"+std::string(64,'A')}){
  auto remote=local;remote["best_hash"]=value;
  check(!valid_peer_status(remote,local)&&!valid_peer_status(local,remote),"malformed best_hash accepted");
 }
 check(valid_peer_status(local,local),"legacy status without best_hash rejected");
 auto remote=local;remote["best_hash"]="0x"+std::string(64,'b');remote["gossip_epoch"]="0x"+std::string(32,'c');
 remote["network_id"]=uint64_t(1337);remote["best_block"]=uint64_t(-1);remote["cursor"]=uint64_t(-1);remote["version"]="nano/1.0.0";
 remote["altairIpc"]=false;remote["bellatrixIpc"]=false;remote["capellaIpc"]=true;
 remote["beacon_head_slot"]=uint64_t(-1);remote["capellaSession"]="0x"+std::string(64,'d');remote["capellaStreamProtocol"]="/nano/capella/session/1.0.0";
 for(const auto* field:{"altair","bellatrix","capella"})remote[field]={{"protocol","/nano/"+std::string(field)+"/1.0.0"},{"consensusIdentity","0x"+std::string(64,'e')},{"forkDigest","0x01020304"}};
 remote["futureExtension"]={{"padding",std::string(1024,'z')}};
 check(remote.dump().size()<4096&&valid_peer_status(remote,local),"bounded future extension or canonical best_hash rejected");
 status_projection_case(remote);
 std::cout<<Json{{"scenario","status-shape-and-projection"},{"passed",true},{"malformedBestHashes",9},{"unknownFieldsRetained",0}}.dump()<<'\n';
}
int main(int argc,char** argv){try{
 if(argc==2&&std::string(argv[1])=="--status-bounds-self-test"){status_bounds_cases();return 0;}
 check(argc==1,"unknown peer status test selector");
 auto local=Json::parse(R"({"chain_id":1337,"best_block":0})");local["genesis_hash"]="0x"+std::string(64,'a');
 check(valid_peer_status(local,local),"valid matching status");
 auto remote=local;remote["best_block"]=uint64_t(100);check(valid_peer_status(remote,local),"peer height may differ");
 for(const auto& field:{"chain_id","best_block","genesis_hash"}){remote=local;remote.erase(field);check(!valid_peer_status(remote,local)&&!valid_peer_status(local,remote),"missing local or remote identity field rejected");}
 for(const auto& value:std::vector<Json>{nullptr,true,-1,1.5,"1337",Json::object(),Json::array()}){remote=local;remote["chain_id"]=value;check(!valid_peer_status(remote,local),"invalid chain ID type");remote=local;remote["best_block"]=value;check(!valid_peer_status(remote,local),"invalid height type");}
 remote=local;remote["chain_id"]=uint64_t(1338);check(!valid_peer_status(remote,local),"foreign chain ID");
 remote=local;remote["genesis_hash"]="0x"+std::string(64,'b');check(!valid_peer_status(remote,local),"same chain ID foreign genesis");
 for(const auto& value:std::vector<Json>{nullptr,0,"0x",std::string(66,'g'),"0x"+std::string(64,'A')}){remote=local;remote["genesis_hash"]=value;check(!valid_peer_status(remote,local),"malformed genesis hash");}
 // The gossip session is optional for legacy peers, but malformed supplied
 // sessions must not reach cursor-reset handling or throw on JSON conversion.
 remote=local;remote["gossip_epoch"]="0x"+std::string(32,'a');
 check(valid_peer_status(remote,local)&&valid_peer_status(local,remote),"legacy and session-aware peers interoperate");
 auto session_local=remote;remote["gossip_epoch"]="0x"+std::string(32,'b');
 check(valid_peer_status(remote,session_local),"different process epochs are valid on the same chain");
 unsigned rejected_epochs=0;
 for(const auto& value:std::vector<Json>{nullptr,true,0,1.5,Json::array(),Json::object(),"","0x","0x"+std::string(31,'a'),"0x"+std::string(33,'a'),"0X"+std::string(32,'a'),"0x"+std::string(32,'A'),"0x"+std::string(31,'a')+"g","0x"+std::string(31,'a')+std::string(1,'\0')}){
  remote=local;remote["gossip_epoch"]=value;
  check(!valid_peer_status(remote,session_local)&&!valid_peer_status(session_local,remote),"malformed local or remote gossip epoch rejected");
  ++rejected_epochs;
 }
 std::cout<<rejected_epochs<<" malformed gossip epoch cases rejected on both sides\n";
 check(!valid_peer_status(Json::object(),Json::object()),"two absent identities do not match");
 auto advertised=local;advertised["altair"]={{"protocol",altair_protocol},{"consensusIdentity","0x"+std::string(64,'c')},{"forkDigest","0x01020304"}};
 check(compatible_altair_peer(advertised,advertised),"matching Altair profile refused");
 auto session_advertised=advertised;session_advertised["capellaStreamProtocol"]="/nano/capella/session/1.0.0";
 check(compatible_altair_peer(session_advertised,advertised)&&compatible_altair_peer(advertised,session_advertised),"optional stream capability broke legacy profile compatibility");
 check(!compatible_altair_peer(local,advertised)&&!compatible_altair_peer(advertised,local),"absent Altair profile accepted");
 for(const auto& key:{"protocol","consensusIdentity","forkDigest"}){auto different=advertised;different["altair"][key]="wrong";check(!compatible_altair_peer(different,advertised)&&!compatible_altair_peer(advertised,different),"incompatible Altair profile accepted");}
 auto other=advertised;other["altair"]["consensusIdentity"]="0x"+std::string(64,'d');check(!compatible_altair_peer(other,advertised),"same fork digest hid different consensus rules");
 other=advertised;other["altair"]["extra"]=true;check(!compatible_altair_peer(other,advertised),"ambiguous Altair advertisement accepted");
 std::cout<<"Strict peer chain and genesis identity passed\n";
 status_bounds_cases();
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
