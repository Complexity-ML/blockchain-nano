#include "support/altair_transition_rules.hpp"
#include "nano/consensus_altair_snapshot.hpp"
#include "nano/chain.hpp"
using namespace test;
using namespace test::phase0_minimal;
int main(int argc,char** argv){try{
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures";
 const auto rules=altair_transition_rules(fixtures);const auto corpus=fixtures/"consensus-altair-full-blocks-official";const auto m=load(corpus/"manifest.json");
 for(const auto& row:m.at("cases"))if(row.at("name")=="attestation")for(const auto& [path,info]:row.at("files").items())check(hex(sha256(binary(corpus/path))).substr(2)==info.at("sha256").get<std::string>(),"recovery fixture digest");
 const auto folder=corpus/"attestation";AltairSlotState initial{binary(folder/"pre.ssz"),std::nullopt};const auto initial_epoch=little(initial.ssz,40,8)/rules.epoch.history.slots_per_epoch;if(initial_epoch)initial.last_processed_epoch=initial_epoch-1; // Explicit trusted fixture checkpoint import.
const auto first=binary(folder/"blocks_0.ssz"),second=binary(folder/"blocks_1.ssz");
 const auto mid=prepare_altair_state_transition(initial,first,rules,64,100000);check(bool(mid),"first authenticated block");const auto end=prepare_altair_state_transition(*mid,second,rules,64,100000);check(end&&end->ssz==binary(folder/"post.ssz"),"uninterrupted official end state");
 const auto root0=altair_beacon_state_root(initial.ssz,rules.state),root1=root_at(first,148),root2=root_at(second,148);
 const auto before=altair_slot_snapshot(initial,rules),middle=altair_slot_snapshot(*mid,rules),after=altair_slot_snapshot(*end,rules);
 const auto genesis=Genesis::from_json(Json{{"config",{{"chainId",1337},{"consensus","pow"}}},{"alloc",Json::object()}});
 auto execution_block=[&](uint64_t height,const Bytes& parent){auto b=genesis.block;b.header.number=Int(std::to_string(height));b.header.timestamp=Int(std::to_string(height));b.header.parent_hash=parent;b.header.transactions_root=transactions_root({});b.header.receipts_root=receipts_root({});return b;};
 const auto block1=execution_block(1,genesis.block.header.hash()),block2=execution_block(2,block1.header.hash());
 if(argc==3&&std::string(argv[1])=="--commit-and-exit"){
  ChainStore store(argv[2]);store.bind_chain(genesis.chain_id,genesis.block.header.hash(),"pow");store.commit(genesis.block,genesis.state,{},before);store.commit(block1,genesis.state,{},middle);_exit(0);
 }
 unsigned checks=0,rejections=0;
 check(altair_slot_from_snapshot(Json::parse(middle.dump()),rules,root1,1024*1024)==*mid,"canonical state snapshot roundtrip");++checks;
 auto refuse=[&](const Json& bad,const ConsensusRoot& root,size_t max=1024*1024){bool threw=false;try{altair_slot_from_snapshot(bad,rules,root,max);}catch(const std::exception&){threw=true;}check(threw,"invalid snapshot accepted");++rejections;};
 auto bad=middle;bad["format"]="nano-altair-slot-v0";refuse(bad,root1);bad=middle;bad["extra"]=0;refuse(bad,root1);bad=middle;bad.erase("stateRoot");refuse(bad,root1);
 for(const auto& [key,value]:middle.at("profile").items()){if(value.is_array()){for(size_t i=0;i<value.size();++i){bad=middle;bad["profile"][key][i]=quantity(quantity(value[i])+1);refuse(bad,root1);}}else{bad=middle;bad["profile"][key]="0xffffffff";refuse(bad,root1);}}
 bad=middle;bad["lastProcessedEpoch"]=nullptr;refuse(bad,root1);bad=middle;bad["lastProcessedEpoch"]="0x1";refuse(bad,root1);bad=middle;bad["lastProcessedEpoch"]="0x00";refuse(bad,root1);bad=middle;bad["lastProcessedEpoch"]=0;refuse(bad,root1);
 bad=middle;auto payload=mid->ssz;payload[0]^=1;bad["ssz"]=hex(payload);refuse(bad,root1);
 const auto forged=altair_beacon_state_root(payload,rules.state);bad["stateRoot"]=hex(Bytes(forged.begin(),forged.end()));refuse(bad,root1);
 bad=middle;bad["ssz"]="0x0";refuse(bad,root1);refuse(middle,root1,mid->ssz.size()-1);refuse(middle,root0);
 bad=before;bad["lastProcessedEpoch"]="0xffff";refuse(bad,root0);
 auto missing=*mid;missing.last_processed_epoch.reset();bool rejected=false;try{altair_slot_snapshot(missing,rules);}catch(const std::exception&){rejected=true;}check(rejected,"writer accepted missing epoch marker");++checks;
 Temp location;const auto db=location/"chain";Process child({argv[0],"--commit-and-exit",db.string()});const auto status=child.wait(30);check(status.code==0,"abrupt snapshot writer: "+status.err);++checks;
 {
  ChainStore store(db);store.bind_chain(genesis.chain_id,genesis.block.header.hash(),"pow");check(store.count()==2&&store.latest()->header.hash()==block1.header.hash(),"recovered coherent tip");++checks;
  check(altair_slot_from_snapshot(*store.consensus_at(0),rules,root0,1024*1024)==initial,"historical checkpoint snapshot");++checks;
  const auto recovered=altair_slot_from_snapshot(*store.consensus_at(1),rules,root1,1024*1024);check(recovered==*mid,"recovered bytes and epoch marker");++checks;
  check(!prepare_altair_state_transition(recovered,first,rules,64,100000),"replayed block after reopen");++checks;
  const auto resumed=prepare_altair_state_transition(recovered,second,rules,64,100000);check(resumed&&*resumed==*end,"recovered continuation differs from uninterrupted execution");++checks;
  store.commit(block2,genesis.state,{},altair_slot_snapshot(*resumed,rules));
  auto invalid=execution_block(3,block2.header.hash());invalid.header.state_root=Bytes(32);bool failed=false;try{store.commit(invalid,genesis.state,{},before);}catch(const std::exception&){failed=true;}check(failed&&!store.consensus_at(3)&&!store.block_by_number(3),"rejected execution commit persisted snapshot");++checks;
 }
 {
  ChainStore store(db);const auto recovered=altair_slot_from_snapshot(*store.consensus_at(2),rules,root2,1024*1024);check(recovered==*end&&recovered.ssz==binary(folder/"post.ssz")&&store.count()==3&&!store.consensus_at(3),"second reopen exact final state");++checks;
  check(!prepare_altair_state_transition(recovered,second,rules,64,100000),"repeated final block after reopen");++checks;
  bool failed=false;try{store.bind_chain(999,genesis.block.header.hash(),"pow");}catch(const std::exception&){failed=true;}check(failed,"cross-chain storage binding");++checks;
 }
 std::cout<<Json{{"scenario","altair-slot-snapshot-recovery"},{"checks",checks},{"rejectedSnapshots",rejections},{"abruptExitAfterCommit",true},{"resumedStateMatchesOfficial",true},{"powerLossQualified",false},{"executionConsensusBindingQualified",false},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
