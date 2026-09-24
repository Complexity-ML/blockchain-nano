#include "support/phase0_minimal_ssz.hpp"
#include "nano/consensus_randao.hpp"
#include "nano/consensus_proposer.hpp"
using namespace test;
using namespace test::phase0_minimal;
namespace {
ConsensusRandaoState randao(const Bytes& b){
 auto registry=decode(b); // Validate the minimal BeaconState layout first.
 ConsensusRandaoState state;state.epoch=registry.epoch;state.mixes.resize(64);
 for(size_t i=0;i<64;++i)std::copy_n(b.begin()+4368+i*32,32,state.mixes[i].begin());
 state.last_reveal_slot=little(b,64,8); // latest_block_header.slot
 return state;
}
}
int main(int argc,char** argv){try{
 const fs::path root=argc==2?argv[1]:fs::path(NANO_TEST_SOURCE)/"tests/fixtures/consensus-randao-official";
 const auto manifest=load(root/"manifest.json");check(manifest.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908","fixture revision mismatch");
 const ConsensusRandaoRules rules{8,1}; // Official minimal preset (same SSZ schema).
 size_t cases=0,reveals=0,rejected=0,mixes=0,selected_rejections=0;std::set<std::string> names;
 for(const auto& entry:manifest.at("cases")){
  const auto name=entry.at("name").get<std::string>();check(names.insert(name).second,"duplicate fixture");
  for(const auto& [path,info]:entry.at("files").items()){
   auto b=binary(root/path);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"fixture hash mismatch "+path);
  }
  const auto pre=binary(root/name/"pre.ssz"),post=binary(root/name/"post.ssz");
  auto state=randao(pre),expected=randao(post);const auto registry=decode(pre);
  ConsensusRoot genesis{};std::copy_n(pre.begin()+8,32,genesis.begin());
  ConsensusBytes4 previous{},current{};std::copy_n(pre.begin()+48,4,previous.begin());std::copy_n(pre.begin()+52,4,current.begin());
  const auto fork_epoch=little(pre,56,8);
  for(size_t i=0;i<entry.at("blockCount").get<size_t>();++i){
   const auto b=binary(root/name/("blocks_"+std::to_string(i)+".ssz"));
   check(b.size()>=404&&little(b,0,4)==100,"invalid signed block fixed section");
   const size_t message=100;
   check(little(b,message+80,4)==84,"invalid block body offset");
   const auto slot=little(b,message,8),proposer=little(b,message+8,8);
   check(slot/8>=state.epoch&&slot/8-state.epoch<=64,"fixture epoch gap outside bounded test");
   while(state.epoch<slot/8)advance_consensus_randao_epoch(state,rules,state.epoch);
   std::vector<Phase0ProposerValidator> candidates;
   for(const auto& [index,v]:registry.validators)candidates.push_back({index,v.effective_balance,v.activation_epoch,v.exit_epoch});
   check(phase0_proposer_index(candidates,state,rules,slot,32000000000ULL,10)==proposer,"official proposer selection mismatch: "+name);
   const auto version=state.epoch<fork_epoch?previous:current;
   const auto& key=registry.validators.at(proposer).public_key;
   const Bytes reveal(b.begin()+184,b.begin()+280);
   auto refuse=[&](const auto& sig,const auto& v,const auto& g,const auto& k){auto candidate=state;check(!process_consensus_randao(candidate,rules,slot,v,g,k,sig)&&candidate==state,"mutated official reveal accepted");++rejected;};
   auto bad=reveal;bad[0]^=1;refuse(bad,version,genesis,key);
   auto wrong_fork=version;wrong_fork[0]^=1;refuse(reveal,wrong_fork,genesis,key);
   auto wrong_genesis=genesis;wrong_genesis[0]^=1;refuse(reveal,version,wrong_genesis,key);
   const auto& other_key=registry.validators.at((proposer+1)%registry.validators.size()).public_key;refuse(reveal,version,genesis,other_key);
   auto duty_registry=registry;duty_registry.epoch=state.epoch;
   // These selected empty-block fixtures preserve every proposer-relevant field.
   const auto post_registry=decode(post);
   for(const auto& [index,v]:registry.validators){const auto& w=post_registry.validators.at(index);check(v.public_key==w.public_key&&v.effective_balance==w.effective_balance&&v.activation_epoch==w.activation_epoch&&v.exit_epoch==w.exit_epoch&&v.slashed==w.slashed,"fixture changed proposer registry across epoch");}
   auto candidate=state;
   check(!process_phase0_selected_randao(candidate,duty_registry,rules,slot,(proposer+1)%registry.validators.size(),32000000000ULL,10,version,genesis,reveal)&&candidate==state,"wrong selected proposer accepted");++selected_rejections;
   auto slashed=duty_registry;slashed.validators.at(proposer).slashed=true;
   check(!process_phase0_selected_randao(candidate,slashed,rules,slot,proposer,32000000000ULL,10,version,genesis,reveal)&&candidate==state,"slashed selected proposer accepted");++selected_rejections;
   check(process_phase0_selected_randao(state,duty_registry,rules,slot,proposer,32000000000ULL,10,version,genesis,reveal),"official reveal rejected: "+name);++reveals;
   auto saved=state;check(!process_consensus_randao(state,rules,slot,version,genesis,key,reveal)&&state==saved,"official block slot replay accepted");++rejected;
  }
  check(state==expected,"official RANDAO post-state mismatch: "+name);mixes+=state.mixes.size();++cases;
 }
 check(names==std::set<std::string>{"empty_block_transition","empty_epoch_transition","skipped_slots"},"fixture selection mismatch");
 std::cout<<Json{{"scenario","official-randao-projection"},{"cases",cases},{"revealsVerified",reveals},{"proposersMatched",reveals},{"selectedProposerRejections",selected_rejections},{"mixesCompared",mixes},{"localMutationsRejected",rejected},{"fullBlockValidation",false},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
