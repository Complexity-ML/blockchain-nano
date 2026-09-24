#include "support/phase0_minimal_ssz.hpp"
#include "nano/consensus_registry_epoch.hpp"
using namespace test;
using namespace test::phase0_minimal;
int main(int argc,char** argv){try{
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures";
 const fs::path root=argc==2?fs::path(argv[1]):fixtures/"consensus-registry-updates-official";
 const auto manifest=load(root/"manifest.json");check(manifest.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908","fixture revision");
 const auto refs=fixtures/"consensus-slashing-official";const auto ref_manifest=load(refs/"manifest.json");for(const auto& [name,info]:ref_manifest.at("referenceFiles").items())check(hex(sha256(binary(refs/name))).substr(2)==info.at("sha256").get<std::string>(),"config digest");
 const auto preset=read(refs/"minimal-phase0.yaml"),config=read(refs/"minimal-config.yaml");auto p=[&](const char* key){return parameter(preset,key);};auto c=[&](const char* key){return parameter(config,key);};
 const Phase0RegistryUpdateRules rules{c("MIN_PER_EPOCH_CHURN_LIMIT"),c("CHURN_LIMIT_QUOTIENT"),p("MAX_SEED_LOOKAHEAD"),c("MIN_VALIDATOR_WITHDRAWABILITY_DELAY"),p("MAX_EFFECTIVE_BALANCE"),c("EJECTION_BALANCE")};
 const Phase0SlashingRules snapshot_rules{rules.minimum_churn_limit,rules.churn_limit_quotient,rules.max_seed_lookahead,rules.minimum_withdrawability_delay,p("EPOCHS_PER_SLASHINGS_VECTOR"),p("MIN_SLASHING_PENALTY_QUOTIENT"),p("WHISTLEBLOWER_REWARD_QUOTIENT"),p("PROPOSER_REWARD_QUOTIENT"),p("EFFECTIVE_BALANCE_INCREMENT"),p("PROPORTIONAL_SLASHING_MULTIPLIER")};
 unsigned valid=0,invalid=0,eligible=0,activated=0,ejected=0,snapshots=0,cancelled=0,rejected=0;size_t records=0;std::set<std::string> names;
 for(const auto& row:manifest.at("cases")){
  const auto name=row.at("name").get<std::string>();check(names.insert(name).second,"duplicate case");for(const auto& [path,info]:row.at("files").items()){auto bytes=binary(root/path);check(bytes.size()==info.at("bytes")&&hex(sha256(bytes)).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");}
  const auto pre=binary(root/name/"pre.ssz");const auto initial=decode(pre),before=initial;const auto finalized=little(pre,7017,8);
  if(!row.at("valid").get<bool>()){
   bool threw=false;try{prepare_phase0_registry_updates(initial,rules,finalized);}catch(const std::overflow_error&){threw=true;}check(threw&&initial==before,"invalid official update must overflow atomically");++invalid;continue;
  }
  auto result=prepare_phase0_registry_updates(initial,rules,finalized);const auto expected=decode(binary(root/name/"post.ssz"));check(result.registry_after==expected,"official registry update mismatch: "+name);check(initial==before,"source changed");++valid;records+=expected.validators.size();
  std::vector<uint64_t> expected_eligible,expected_ejected;std::vector<std::pair<uint64_t,uint64_t>> expected_activated;
  for(const auto& [index,after]:expected.validators){const auto& old=initial.validators.at(index);if(old.activation_eligibility_epoch!=after.activation_eligibility_epoch)expected_eligible.push_back(index);if(old.exit_epoch!=after.exit_epoch)expected_ejected.push_back(index);if(old.activation_epoch!=after.activation_epoch)expected_activated.emplace_back(old.activation_eligibility_epoch,index);}
  std::sort(expected_activated.begin(),expected_activated.end());std::vector<uint64_t> order;for(const auto& [epoch,index]:expected_activated)order.push_back(index);
  check(result.newly_eligible==expected_eligible&&result.ejected==expected_ejected&&result.activated==order,"update outcome/order mismatch");eligible+=result.newly_eligible.size();activated+=result.activated.size();ejected+=result.ejected.size();
  auto recovered=phase0_slashing_from_snapshot(Json::parse(phase0_slashing_snapshot(result.registry_after,snapshot_rules).dump()),snapshot_rules);check(recovered==expected,"registry lifecycle snapshot mismatch");++snapshots;
  if(name=="activation_queue_activation_and_ejection__1"){
   size_t total=0;prepare_phase0_registry_updates(initial,rules,finalized,[&]{++total;});
   for(size_t stop=1;stop<=total;++stop){size_t calls=0;bool aborted=false;try{prepare_phase0_registry_updates(initial,rules,finalized,[&]{if(++calls==stop)throw std::runtime_error("cancelled");});}catch(const std::runtime_error& e){aborted=std::string(e.what())=="cancelled";}check(aborted&&initial==before,"cancelled update changed state");++cancelled;}
   auto refuse=[&](const ConsensusSlashingState& s,const Phase0RegistryUpdateRules& r,uint64_t f){const auto original=s;bool threw=false;try{prepare_phase0_registry_updates(s,r,f);}catch(const std::exception&){threw=true;}check(threw&&s==original,"bad update accepted or changed state");++rejected;};
   auto r=rules;r.minimum_churn_limit=0;refuse(initial,r,finalized);r=rules;r.churn_limit_quotient=0;refuse(initial,r,finalized);r=rules;r.ejection_balance=r.maximum_effective_balance+1;refuse(initial,r,finalized);refuse(initial,rules,initial.epoch+1);
   auto s=initial;s.epoch=UINT64_MAX;refuse(s,rules,finalized);s=initial;s.validators.at(1).public_key=s.validators.at(0).public_key;refuse(s,rules,finalized);s=initial;s.validators.erase(1);refuse(s,rules,finalized);s=initial;s.validators.at(0).effective_balance=rules.maximum_effective_balance+1;refuse(s,rules,finalized);
   r=rules;r.minimum_withdrawability_delay=UINT64_MAX;refuse(initial,r,finalized);r=rules;r.max_seed_lookahead=UINT64_MAX;refuse(initial,r,finalized);
  }
 }
 check(valid==14&&invalid==1&&names.size()==15,"official registry update coverage");
 std::cout<<Json{{"scenario","official-phase0-registry-updates"},{"valid",valid},{"invalid",invalid},{"validatorsCompared",records},{"eligible",eligible},{"activated",activated},{"ejected",ejected},{"snapshotRoundtrips",snapshots},{"cancellations",cancelled},{"localRejections",rejected},{"fullEpochTransition",false},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
