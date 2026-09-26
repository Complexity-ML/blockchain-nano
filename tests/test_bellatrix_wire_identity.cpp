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
 auto exit=frame;exit["kind"]="voluntary_exit";exit["ssz"]=hex(Bytes(112));
 check(beacon.decode(exit.dump()).has_value(),"signed exit envelope refused");
 auto foreign=exit;foreign["consensusIdentity"]="0x"+std::string(64,'c');check(!beacon.decode(foreign.dump()),"foreign exit identity admitted");
 auto legacy_exit=exit;legacy_exit.erase("consensusIdentity");check(!legacy.decode(legacy_exit.dump()),"exit admitted on legacy protocol");
 for(size_t width:{0,111,113}){auto malformed=exit;malformed["ssz"]=hex(Bytes(width));check(!beacon.decode(malformed.dump()),"invalid exit width admitted");malformed.erase("ssz");malformed["sszBase64"]=altair_base64(Bytes(width));check(!beacon.decode_compact(malformed),"invalid compact exit width admitted");}
 auto compact_exit=exit;compact_exit.erase("ssz");compact_exit["sszBase64"]=altair_base64(Bytes(112));check(beacon.decode_compact(compact_exit).has_value(),"valid compact exit refused");
 const auto text=frame.dump();auto duplicate=text;duplicate.insert(1,"\"consensusIdentity\":\""+identity+"\",");check(!beacon.decode(duplicate),"duplicate identity key refused");
 std::cout<<"Bellatrix full identity and legacy/compact envelope separation checks passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
