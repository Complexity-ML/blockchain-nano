#include "support/process.hpp"
#include "../network/altair_message.hpp"
#include "../network/altair_peer.hpp"
using namespace test;
int main(){try{
 const std::string identity="0x"+std::string(64,'a');
 AltairWireProfile profile{"0x539","0x"+std::string(64,'b'),"0x01020304",4096,1024,1024,1024,identity};
 InboundBudget budget;AltairMessageIngress beacon(profile,budget);
 Json frame={{"chainId",profile.chain_id},{"genesisHash",profile.genesis_hash},{"forkDigest",profile.fork_digest},{"consensusIdentity",identity},{"kind","block"},{"ssz","0x010203"}};
 auto accepted=beacon.decode(frame.dump());check(accepted&&accepted->ssz==Bytes({1,2,3}),"complete Bellatrix identity admitted");
 auto missing=frame;missing.erase("consensusIdentity");check(!beacon.decode(missing.dump()),"legacy envelope refused by Bellatrix");
 auto wrong=frame;wrong["consensusIdentity"]="0x"+std::string(64,'c');check(!beacon.decode(wrong.dump()),"same fork digest cannot mask foreign full identity");
 auto legacy_profile=profile;legacy_profile.consensus_identity.clear();AltairMessageIngress legacy(legacy_profile,budget);
 check(!legacy.decode(frame.dump())&&legacy.decode(missing.dump()).has_value(),"protocol schemas remain separate");
 auto compact=frame;compact.erase("ssz");compact["sszBase64"]=altair_base64(Bytes({1,2,3}));
 check(beacon.decode_compact(compact).has_value(),"compact IPC carries identity");compact["consensusIdentity"]="0x"+std::string(64,'c');check(!beacon.decode_compact(compact),"compact foreign identity refused");
 const auto text=frame.dump();auto duplicate=text;duplicate.insert(1,"\"consensusIdentity\":\""+identity+"\",");check(!beacon.decode(duplicate),"duplicate identity key refused");
 std::cout<<"Bellatrix full identity and legacy/compact envelope separation checks passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
