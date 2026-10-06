#include "support/process.hpp"
#include "../src/consensus_capella_checkpoint_fragments.hpp"
using namespace test;
using namespace nano::capella_checkpoint_detail;
int main(){try{
 constexpr size_t budget=1024*1024;const Bytes genesis(32,7);Temp temp;const auto db=temp/"db";
 Json rows=Json::array();for(unsigned index:{1u,2u,3u})rows.push_back(Json::array({hex(Bytes(32,index)),Json::array({quantity(Int(index)),"header"}),Json::array({hex(Bytes(4096,index)),nullptr}),"0x0","0x0",Json::array({"execution",Json{{"account","value"}},Json::array()})}));
 const Json store{{"format","nano-capella-store-v1"},{"blocks",rows},{"clock","preserved"},{"messages",Json::array({"unchanged-vote"})}};
 const auto canonical=Json{{"format","nano-capella-durable-v4"},{"signedVotes",Json::array({"signed bytes"})},{"signedSlashings",Json::array({"proof"})},{"blsChanges",Json::array({"change"})},{"signedExits",Json::array({"exit"})},{"store",store}}.dump();
 const auto prepared=fragment_checkpoint(canonical,budget,3);check(prepared.manifest.size()<canonical.size()/4,"manifest does not replace full records");ForkCheckpointToken token;
 {
  ChainStore disk(db);disk.bind_chain(1337,genesis,"fragment-manifest-test");token=disk.commit_fork_checkpoint_with_blobs(1337,genesis,{},prepared.manifest,budget,prepared.blobs,3);
  auto fetch=[&](const Bytes& digest,size_t bytes){return disk.checkpoint_blob(1337,genesis,digest,bytes);};
  check(expand_fragment_checkpoint(prepared.manifest,token.digest,budget,3,fetch)==canonical,"exact metadata/records roundtrip failed");
  const auto standalone=store.dump();const auto single=fragment_checkpoint(standalone,budget,3);auto own=[&](const Bytes& digest,size_t bound)->std::optional<std::string>{for(const auto& b:single.blobs)if(b.digest==digest&&b.payload.size()<=bound)return b.payload;return {};};check(expand_fragment_checkpoint(single.manifest,sha256(Bytes(single.manifest.begin(),single.manifest.end())),budget,3,own)==standalone,"legacy store-only roundtrip failed");
  unsigned fetched=0,rejected=0;auto refuse=[&](auto action){bool bad=false;try{action();}catch(const std::exception&){bad=true;}check(bad,"malformed fragment manifest accepted");++rejected;};
  auto counted=[&](const Bytes& digest,size_t bytes){++fetched;return fetch(digest,bytes);};auto wrong=token.digest;wrong[0]^=1;refuse([&]{expand_fragment_checkpoint(prepared.manifest,wrong,budget,3,counted);});check(fetched==0,"untrusted manifest fetched records before digest verification");
  for(unsigned attack=0;attack<10;++attack){auto altered=Json::parse(prepared.manifest);auto& refs=altered["snapshot"]["store"]["blocks"];
   if(attack==0)altered["extra"]=true;
   if(attack==1)altered["expandedBytes"]="0x10000000000000000";
   if(attack==2)altered["expandedDigest"]=hex(Bytes(32,0xab));
   if(attack==3)refs[0][1]=refs[1][1];
   if(attack==4)refs[0][2]="0x0";
   if(attack==5)refs[0][2]="0x10000000000000000";
   if(attack==6)refs[1][0]=refs[0][0];
   if(attack==7)std::swap(refs[0],refs[1]);
   if(attack==8)refs[0][1]=hex(Bytes(32,0xee));
   if(attack==9)refs[0][2]="0x01";
   const auto bytes=altered.dump();refuse([&]{expand_fragment_checkpoint(bytes,sha256(Bytes(bytes.begin(),bytes.end())),budget,3,fetch);});
  }
  auto absent=[](const Bytes&,size_t)->std::optional<std::string>{return {};};refuse([&]{expand_fragment_checkpoint(prepared.manifest,token.digest,budget,3,absent);});
  auto tampered=[&](const Bytes& digest,size_t bytes){auto value=fetch(digest,bytes);if(value)(*value)[0]^=1;return value;};refuse([&]{expand_fragment_checkpoint(prepared.manifest,token.digest,budget,3,tampered);});
  refuse([&]{fragment_checkpoint(canonical,budget,2);});refuse([&]{expand_fragment_checkpoint(prepared.manifest,token.digest,budget,2,fetch);});
  bool interrupted=false;try{expand_fragment_checkpoint(prepared.manifest,token.digest,budget,3,fetch,[&]{throw std::runtime_error("cancel");});}catch(const std::runtime_error&){interrupted=true;}check(interrupted,"cancellation swallowed");
  check(disk.fork_checkpoint(1337,genesis,budget,token)->snapshot==prepared.manifest,"diagnostic refusals changed persisted checkpoint");
  std::cout<<Json{{"scenario","capella-checkpoint-fragment-manifest"},{"canonicalBytes",canonical.size()},{"manifestBytes",prepared.manifest.size()},{"records",prepared.blobs.size()},{"exactRoundtrip",true},{"metadataAndOperationsPreserved",true},{"adversarialRejections",rejected},{"untrustedDigestFetches",0},{"runtimeIntegration",false},{"consensusValidationQualified",false},{"productionQualified",false}}.dump()<<'\n';
 }
 {ChainStore disk(db,ChainStore::OpenMode::ReadOnly);const auto saved=disk.fork_checkpoint(1337,genesis,budget,token);check(saved&&expand_fragment_checkpoint(saved->snapshot,token.digest,budget,3,[&](const Bytes& digest,size_t bytes){return disk.checkpoint_blob(1337,genesis,digest,bytes);})==canonical,"persisted manifest reopening failed");}
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
