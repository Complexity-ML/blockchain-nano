#include "peer_status.hpp"
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
 check(!valid_peer_status(Json::object(),Json::object()),"two absent identities do not match");
 std::cout<<"Strict peer chain and genesis identity passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
