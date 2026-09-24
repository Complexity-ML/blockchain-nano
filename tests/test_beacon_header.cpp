#include "support/phase0_minimal_ssz.hpp"
#include "nano/consensus_beacon_header.hpp"
#include "nano/consensus_beacon_body.hpp"
using namespace test;
using namespace test::phase0_minimal;
ConsensusRoot root(const Json& v){auto b=unhex(v.get<std::string>());check(b.size()==32,"root width");ConsensusRoot r{};std::copy(b.begin(),b.end(),r.begin());return r;}
ConsensusBeaconHeader mutate(ConsensusBeaconHeader h,unsigned field){switch(field){case 0:h.slot^=1;break;case 1:h.proposer_index^=1;break;case 2:h.parent_root[0]^=1;break;case 3:h.state_root[0]^=1;break;case 4:h.body_root[0]^=1;break;}return h;}
int main(int argc,char** argv){try{
 const fs::path fixtures=argc==2?fs::path(argv[1]):fs::path(NANO_TEST_SOURCE)/"tests/fixtures";
 const auto static_cases=load(fixtures/"consensus-beacon-header.json");unsigned roots=0,mutations=0,signatures=0,rejected=0;
 check(static_cases.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908","static revision");
 for(const auto& row:static_cases.at("cases")){
  const auto& v=row.at("value");ConsensusBeaconHeader h{std::stoull(v.at("slot").get<std::string>()),std::stoull(v.at("proposer_index").get<std::string>()),root(v.at("parent_root")),root(v.at("state_root")),root(v.at("body_root"))};
  const auto expected=root(row.at("root"));check(consensus_beacon_header_root(h)==expected,"official header SSZ root mismatch");++roots;
  for(unsigned field=0;field<5;++field){check(consensus_beacon_header_root(mutate(h,field))!=expected,"uncommitted header field");++mutations;}
 }
 const auto folder=fixtures/"consensus-randao-official";const auto manifest=load(folder/"manifest.json");
 check(manifest.at("commit")==static_cases.at("commit"),"block revision");
 for(const auto& entry:manifest.at("cases")){
  for(const auto& [path,info]:entry.at("files").items()){const auto b=binary(folder/path);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"fixture hash mismatch");}
  check(entry.at("blockCount")==1,"single-block fixture required for post header body root");
  const auto name=entry.at("name").get<std::string>();const auto pre=binary(folder/name/"pre.ssz"),post=binary(folder/name/"post.ssz"),b=binary(folder/name/"blocks_0.ssz");
  const auto registry=decode(pre);decode(post);
  check(b.size()>=404&&little(b,0,4)==100&&little(b,180,4)==84,"signed block layout");
  ConsensusBeaconHeader h;h.slot=little(b,100,8);h.proposer_index=little(b,108,8);
  check(little(post,64,8)==h.slot&&little(post,72,8)==h.proposer_index,"post header identity");
  std::copy_n(b.begin()+116,32,h.parent_root.begin());std::copy_n(b.begin()+148,32,h.state_root.begin());
  // Hash the received body independently, then compare to the official commitment.
  h.body_root=phase0_beacon_body_root(std::span<const uint8_t>(b).subspan(184),{16,2,128,16,16,2048});
  check(std::equal(h.body_root.begin(),h.body_root.end(),post.begin()+144),"official body commitment mismatch");
  ConsensusRoot genesis{};std::copy_n(pre.begin()+8,32,genesis.begin());
  ConsensusBytes4 version{};std::copy_n(pre.begin()+(h.slot/8<little(pre,56,8)?48:52),4,version.begin());
  const auto& key=registry.validators.at(h.proposer_index).public_key;const Bytes signature(b.begin()+4,b.begin()+100);
  check(verify_consensus_beacon_header(h,version,genesis,key,signature),"official block signature mismatch: "+name);++signatures;
  auto refuse=[&](const auto& header,const auto& fork,const auto& chain,const auto& pub,const auto& sig){check(!verify_consensus_beacon_header(header,fork,chain,pub,sig),"invalid header signature accepted");++rejected;};
  for(unsigned field=0;field<5;++field)refuse(mutate(h,field),version,genesis,key,signature);
  auto fork=version;fork[0]^=1;refuse(h,fork,genesis,key,signature);
  auto chain=genesis;chain[0]^=1;refuse(h,version,chain,key,signature);
  auto sig=signature;sig[0]^=1;refuse(h,version,genesis,key,sig);sig.pop_back();refuse(h,version,genesis,key,sig);
  refuse(h,version,genesis,registry.validators.at((h.proposer_index+1)%registry.validators.size()).public_key,signature);
  refuse(h,version,genesis,key,Bytes(b.begin()+184,b.begin()+280)); // RANDAO domain signature cannot sign a block.
 }
 check(roots==5&&signatures==3&&rejected==33,"corpus totals");
 std::cout<<Json{{"scenario","beacon-header-signature"},{"officialRoots",roots},{"fieldMutations",mutations},{"officialSignatures",signatures},{"rejected",rejected},{"fullBodyRootComputed",true},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
