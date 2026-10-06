#include "peer_status.hpp"
#include "altair_peer.hpp"
#include <iostream>
using Json=nlohmann::json;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try{
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
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
