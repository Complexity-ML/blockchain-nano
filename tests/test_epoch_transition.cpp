#include "support/phase0_epoch_ssz.hpp"
using namespace test;
using namespace test::phase0_minimal;
int main(int argc,char** argv){try{
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures";
 const auto root=argc==2?fs::path(argv[1]):fixtures/"consensus-slots-official";
 const auto rules=epoch_rules(fixtures/"consensus-slashing-official");
 const auto manifest=load(root/"manifest.json");check(manifest.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908"&&manifest.at("cases").size()==7,"slot fixture coverage");
 unsigned projections=0,deferred=0,epochs=0,replays=0;size_t compared=0;
 for(const auto& row:manifest.at("cases")){
  const auto name=row.at("name").get<std::string>();check(row.at("valid")==true,"invalid slot fixture");
  for(const auto& [path,info]:row.at("files").items()){const auto b=binary(root/path);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");}
  if(name=="historical_accumulator"){++deferred;continue;}
  auto s=epoch_state(binary(root/name/"pre.ssz"));const auto expected=epoch_state(binary(root/name/"post.ssz"));
  check(s.ffg.previous_attestations.empty()&&s.ffg.current_attestations.empty(),"projection requires empty pending records");
  // Projection only: cached per-slot roots are not calculated here. No history
  // accumulator boundary is allowed, so no expected output is fed as input.
  const auto slots=std::stoull(read(root/name/"slots.yaml"));check(slots<=UINT64_MAX-s.ffg.slot,"slot count overflow");const auto target=s.ffg.slot+slots;check(target==expected.ffg.slot,"official target slot mismatch");
  while(s.ffg.slot<target){
   if((s.ffg.slot+1)%rules.history.slots_per_epoch==0){
    check((s.registry.epoch+1)%(rules.history.slots_per_historical_root/rules.history.slots_per_epoch)!=0,"projection crosses history boundary");
    const auto old=s;s=prepare_phase0_epoch(s,rules,s.registry.epoch);check(old.ffg.slot==s.ffg.slot,"process_epoch advanced slot");++epochs;
    bool rejected=false;try{prepare_phase0_epoch(s,rules,s.registry.epoch);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"epoch replay accepted");++replays;
   }
   ++s.ffg.slot;s.registry.epoch=s.ffg.slot/rules.history.slots_per_epoch;s.randao.epoch=s.registry.epoch;
  }
  check(s.registry==expected.registry&&s.randao==expected.randao&&s.ffg.checkpoints==expected.ffg.checkpoints&&s.ffg.previous_attestations==expected.ffg.previous_attestations&&s.ffg.current_attestations==expected.ffg.current_attestations&&s.eth1_votes==expected.eth1_votes&&s.historical_roots==expected.historical_roots,"official epoch projection mismatch: "+name);
  ++projections;compared+=s.registry.validators.size();
 }
 auto s=epoch_state(binary(root/"balance_change_affects_proposer/pre.ssz"));
 // Force the correlation penalty to consume the pre-update effective weight.
 // Resetting slashings or lowering effective balance early leaves funds behind.
 auto& v=s.registry.validators.at(0);v.slashed=true;v.activation_epoch=0;v.exit_epoch=s.registry.epoch+1;v.withdrawable_epoch=s.registry.epoch+rules.slashing.epochs_per_slashings_vector/2;v.balance=32000000000;v.effective_balance=32000000000;
 s.registry.slashings[(s.registry.epoch+1)%s.registry.slashings.size()]=2048000000000;
 const auto before=s;const auto result=prepare_phase0_epoch(s,rules,s.registry.epoch);
 check(s==before&&result.registry.validators.at(0).balance==0&&result.registry.validators.at(0).effective_balance==0&&result.registry.slashings[(s.registry.epoch+1)%s.registry.slashings.size()]==0,"epoch ordering/atomicity");
 size_t calls=0;prepare_phase0_epoch(s,rules,s.registry.epoch,[&]{++calls;});
 for(size_t stop=1;stop<=calls;++stop){size_t n=0;bool aborted=false;try{prepare_phase0_epoch(s,rules,s.registry.epoch,[&]{if(++n==stop)throw std::runtime_error("cancelled");});}catch(const std::runtime_error& e){aborted=std::string(e.what())=="cancelled";}check(aborted&&s==before,"cancelled epoch mutated input");}
 unsigned rejects=0;auto refuse=[&](auto state,auto config,uint64_t epoch){const auto old=state;bool threw=false;try{prepare_phase0_epoch(state,config,epoch);}catch(const std::exception&){threw=true;}check(threw&&state==old,"invalid epoch accepted");++rejects;};
 refuse(s,rules,s.registry.epoch+1);auto bad=s;--bad.ffg.slot;refuse(bad,rules,s.registry.epoch);bad=s;bad.last_processed_epoch=s.registry.epoch;refuse(bad,rules,s.registry.epoch);bad=s;bad.randao.epoch++;refuse(bad,rules,s.registry.epoch);bad=s;bad.ffg.slot=UINT64_MAX;refuse(bad,rules,s.registry.epoch);
 auto r=rules;r.effective.increment++;refuse(s,r,s.registry.epoch);r=rules;r.slashing.minimum_churn_limit++;refuse(s,r,s.registry.epoch);r=rules;r.rewards.proposer_reward_quotient++;refuse(s,r,s.registry.epoch);r=rules;r.history.slots_per_epoch++;refuse(s,r,s.registry.epoch);r=rules;r.epochs_per_randao_vector++;refuse(s,r,s.registry.epoch);r=rules;r.epochs_per_eth1_voting_period=0;refuse(s,r,s.registry.epoch);r=rules;r.maximum_pending_attestations=0;refuse(s,r,s.registry.epoch);
 // Nonempty authenticated records exercise FFG -> rewards -> rotation.
 const auto ffolder=fixtures/"consensus-ffg-official";const auto fm=load(ffolder/"manifest.json");
 for(const auto& row:fm.at("cases"))if(row.at("name")=="123_ok_support")for(const auto& [path,info]:row.at("files").items())check(hex(sha256(binary(ffolder/path))).substr(2)==info.at("sha256").get<std::string>(),"FFG composition fixture digest");
 const auto voting=epoch_state(binary(ffolder/"123_ok_support/pre.ssz"));const auto voted=prepare_phase0_epoch(voting,rules,voting.registry.epoch);
 check(voted.ffg.checkpoints==ffg_state(binary(ffolder/"123_ok_support/post.ssz")).checkpoints&&voted.ffg.previous_attestations==voting.ffg.current_attestations&&voted.ffg.current_attestations.empty(),"FFG reward rotation composition");
 // A late history capacity error must discard prior rewards/registry/sanctions.
 bad=s;bad.ffg.slot=63;bad.registry.epoch=7;bad.randao.epoch=7;bad.historical_roots.resize(1);r=rules;r.history.historical_roots_limit=1;
 const auto original=bad;bool late=false;try{prepare_phase0_epoch(bad,r,7);}catch(const std::overflow_error& e){late=std::string(e.what())=="historical roots list full";}check(late&&bad==original,"late history overflow atomicity");++rejects;
 check(projections==6&&deferred==1,"projection/deferred accounting");
 std::cout<<Json{{"scenario","phase0-ordered-epoch"},{"officialProjections",projections},{"deferredHistorySequences",deferred},{"epochs",epochs},{"registryEntriesCompared",compared},{"replayRejections",replays},{"orderingControls",1},{"votingComposition",1},{"cancellations",calls},{"localRejections",rejects},{"fullSlotTransition",false},{"stateRootVerified",false},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
