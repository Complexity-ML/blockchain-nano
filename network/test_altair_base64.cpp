#include "altair_message.hpp"
#include <iostream>
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try{
 for(const auto& [plain,encoded]:{std::pair{"",""},std::pair{"f","Zg=="},std::pair{"fo","Zm8="},std::pair{"foo","Zm9v"},std::pair{"foob","Zm9vYg=="},std::pair{"fooba","Zm9vYmE="},std::pair{"foobar","Zm9vYmFy"}}){std::string s=plain;std::vector<uint8_t> bytes(s.begin(),s.end());check(altair_base64(bytes)==encoded&&altair_unbase64(encoded,bytes.size())==bytes,"RFC 4648 vector mismatch");}
 for(const auto* malformed:{"Zg","Zg=","Zg===","Zh==","Zm9=","Zg==\n","Z g=","====","=AAA","AA=A","Zg--","Zg__"})check(!altair_unbase64(malformed,100),"noncanonical base64 accepted");
 check(!altair_unbase64("Zm9v",2),"decoded bound bypassed");
 const AltairWireProfile profile{"0x539","0x"+std::string(64,'a'),"0x01020304",16*1024*1024+1024,8*1024*1024,512,40000};InboundBudget budget;AltairMessageIngress ingress(profile,budget);
 std::vector<uint8_t> large(8*1024*1024);for(size_t i=0;i<large.size();++i)large[i]=uint8_t(i);
 nlohmann::json envelope{{"chainId",profile.chain_id},{"genesisHash",profile.genesis_hash},{"forkDigest",profile.fork_digest},{"kind","block"},{"sszBase64",altair_base64(large)}};
 const auto decoded=ingress.decode_compact(envelope);check(decoded&&decoded->ssz==large,"8 MiB binary payload changed");
 const auto request=nlohmann::json{{"jsonrpc","2.0"},{"id",1},{"method","nano_altairSubmit"},{"params",nlohmann::json::array({std::string(66,'a'),std::string(66,'b'),"0xffffffffffffffff",envelope})}}.dump();
 check(request.size()<=16*1024*1024,"full 8 MiB profile exceeds compact HTTP bound");
 check(!ingress.decode(envelope.dump()),"compact IPC encoding leaked into peer framing");
 envelope["kind"]="attestation";check(!ingress.decode_compact(envelope),"compact IPC kind limit bypassed");
 std::cout<<"Canonical base64 vectors, malformed inputs and 8 MiB IPC boundary passed; HTTP bytes="<<request.size()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
