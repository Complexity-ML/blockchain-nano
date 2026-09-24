#include "support/phase0_transition_rules.hpp"
#include "nano/consensus_snapshot.hpp"
#include "nano/chain.hpp"
extern "C" void nano_test_arm_sync_fault(const char*);
extern "C" unsigned nano_test_sync_failures();
using namespace test;
using namespace test::phase0_minimal;
int main(int argc,char** argv){try{
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures";
 const auto rules=transition_rules(fixtures/"consensus-slashing-official");const auto corpus=fixtures/"consensus-full-blocks-official";const auto manifest=load(corpus/"manifest.json");
 for(const auto& row:manifest.at("cases"))if(row.at("name")=="attestation")for(const auto& [path,info]:row.at("files").items())check(hex(sha256(binary(corpus/path))).substr(2)==info.at("sha256").get<std::string>(),"fault fixture digest");
 const auto folder=corpus/"attestation";Phase0SlotState initial{binary(folder/"pre.ssz"),0};check(little(initial.ssz,40,8)==8,"trusted checkpoint import");
 const auto first=binary(folder/"blocks_0.ssz"),second=binary(folder/"blocks_1.ssz");const auto mid=prepare_phase0_state_transition(initial,first,rules,64);check(bool(mid),"first transition");const auto end=prepare_phase0_state_transition(*mid,second,rules,64);check(end&&end->ssz==binary(folder/"post.ssz"),"expected end state");
 const auto root0=phase0_beacon_state_root(initial.ssz,rules.state),root1=root_at(first,148),root2=root_at(second,148);
 const auto before=phase0_slot_snapshot(initial,rules),middle=phase0_slot_snapshot(*mid,rules);
 const auto genesis=Genesis::from_json(Json{{"config",{{"chainId",1337},{"consensus","pow"}}},{"alloc",Json::object()}});
 auto block1=genesis.block;block1.header.number=1;block1.header.timestamp=1;block1.header.parent_hash=genesis.block.header.hash();block1.header.transactions_root=transactions_root({});block1.header.receipts_root=receipts_root({});
 auto block2=block1;block2.header.number=2;block2.header.timestamp=2;block2.header.parent_hash=block1.header.hash();
 if(argc==3&&std::string(argv[1])=="--fail"){
  ChainStore store(argv[2]);store.bind_chain(genesis.chain_id,genesis.block.header.hash(),"pow");store.commit(genesis.block,genesis.state,{},before);
  fs::path wal;for(const auto& entry:fs::directory_iterator(argv[2]))if(entry.path().extension()==".log"&&(wal.empty()||entry.path().filename()>wal.filename()))wal=entry.path();check(!wal.empty(),"missing WAL");const auto bytes_before=fs::file_size(wal);nano_test_arm_sync_fault(wal.c_str());
  std::string error;try{store.commit(block1,genesis.state,{},middle);}catch(const std::runtime_error& e){error=e.what();}
  const auto failures=nano_test_sync_failures();check(failures>0&&error.starts_with("RocksDB:")&&fs::file_size(wal)>bytes_before,"sync error not exercised after append");
  check(store.latest()->header.number==0&&store.count()==1&&!store.consensus_at(1),"failed sync published partial live commit");
  std::cout<<Json{{"wal",wal.filename().string()},{"before",bytes_before},{"after",fs::file_size(wal)},{"syncFailures",failures},{"error",error}}.dump()<<std::endl;_exit(0);
 }
 check(argc==1,"unexpected arguments");Json outcomes=Json::array();
 for(const auto mode:{"retained","lost","torn"}){
  Temp temp;const auto db=temp/"db";const auto failed=success(run({argv[0],"--fail",db.string()}));check(failed.at("syncFailures").get<unsigned>()>0,"missing observed sync fault");
  const auto name=failed.at("wal").get<std::string>();check(fs::path(name).filename()==name,"WAL basename");const auto wal=db/name;const auto old_size=failed.at("before").get<uint64_t>(),new_size=failed.at("after").get<uint64_t>();check(fs::file_size(wal)==new_size,"unexpected WAL change");
  uint64_t kept=new_size-old_size;
  if(std::string(mode)!="retained"){
   const auto length=std::string(mode)=="lost"?old_size:old_size+(new_size-old_size)/2;
   kept=length-old_size;
   int fd=open(wal.c_str(),O_WRONLY);check(fd>=0,"open owned WAL");const bool ok=ftruncate(fd,static_cast<off_t>(length))==0&&fsync(fd)==0;close(fd);check(ok,"simulate unsynced tail loss");
  }
  uint64_t reopened=0;
  {
   ChainStore store(db);store.bind_chain(genesis.chain_id,genesis.block.header.hash(),"pow");reopened=u64(store.latest()->header.number);
   check(reopened==(std::string(mode)=="retained"?1:0),"unexpected WAL recovery outcome");
   check(store.count()==reopened+1&&store.state_at(Int(std::to_string(reopened)))->root()==genesis.state.root()&&!store.consensus_at(Int(std::to_string(reopened+1))),"mixed recovered commit");
   const auto recovered=phase0_slot_from_snapshot(*store.consensus_at(Int(std::to_string(reopened))),rules,reopened?root1:root0,1024*1024);check(recovered==(reopened?*mid:initial),"mixed SSZ/marker recovery");
   Phase0SlotState current=recovered;
   if(!reopened){const auto next=prepare_phase0_state_transition(current,first,rules,64);check(next&&*next==*mid,"reapply lost commit");current=*next;store.commit(block1,genesis.state,{},phase0_slot_snapshot(current,rules));}
   check(!prepare_phase0_state_transition(current,first,rules,64),"duplicate first block after recovery");
   const auto next=prepare_phase0_state_transition(current,second,rules,64);check(next&&*next==*end,"resumed state differs from official");store.commit(block2,genesis.state,{},phase0_slot_snapshot(*next,rules));
  }
  {
   ChainStore store(db);check(store.count()==3&&store.latest()->header.hash()==block2.header.hash()&&phase0_slot_from_snapshot(*store.consensus_at(2),rules,root2,1024*1024)==*end,"second recovery changed committed state");
  }
  outcomes.push_back({{"mode",mode},{"reopenedHeight",reopened},{"syncFailures",failed.at("syncFailures")},{"consistent",true},{"resumedOfficialState",true},{"walAppendedBytes",new_size-old_size},{"walSuffixBytesRetained",kept}});
 }
 std::cout<<Json{{"scenario","phase0-snapshot-sync-failure"},{"outcomes",outcomes},{"physicalPowerLossQualified",false},{"executionConsensusBindingQualified",false},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
