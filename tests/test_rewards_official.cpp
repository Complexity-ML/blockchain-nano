#include "support/phase0_ffg_ssz.hpp"
#include "nano/consensus_rewards.hpp"
using namespace test;
using namespace test::phase0_minimal;
int main(int argc,char** argv){try{
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures";
 const auto root=argc==2?fs::path(argv[1]):fixtures/"consensus-rewards-official";
 const auto m=load(root/"manifest.json");check(m.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908"&&m.at("cases").size()==19,"official coverage");
 const auto refs=fixtures/"consensus-slashing-official";const auto refm=load(refs/"manifest.json");for(const auto& [name,info]:refm.at("referenceFiles").items())check(hex(sha256(binary(refs/name))).substr(2)==info.at("sha256").get<std::string>(),"config digest");
 const auto preset=read(refs/"minimal-phase0.yaml");auto p=[&](const char* k){return parameter(preset,k);};
 const Phase0RewardRules rules{{p("SLOTS_PER_EPOCH"),p("MIN_SEED_LOOKAHEAD"),p("TARGET_COMMITTEE_SIZE"),p("MAX_COMMITTEES_PER_SLOT"),p("EFFECTIVE_BALANCE_INCREMENT"),p("MAX_EFFECTIVE_BALANCE"),uint16_t(p("SHUFFLE_ROUND_COUNT"))},p("BASE_REWARD_FACTOR"),4,p("PROPOSER_REWARD_QUOTIENT"),p("MIN_EPOCHS_TO_INACTIVITY_PENALTY"),p("INACTIVITY_PENALTY_QUOTIENT")}; // Phase0 BASE_REWARDS_PER_EPOCH constant = 4.
 unsigned cases=0,leaks=0,genesis=0,duplicates=0;size_t records=0,changed=0;
 for(const auto& row:m.at("cases")){
  check(row.at("valid")==true,"unexpected invalid fixture");const auto name=row.at("name").get<std::string>();
  for(const auto& [path,info]:row.at("files").items()){const auto b=binary(root/path);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");}
  const auto pre=binary(root/name/"pre.ssz");const auto s=decode(pre),before=s;const auto f=ffg_state(pre),fbefore=f;const auto r=ffg_randao(pre),rbefore=r;
  const auto expected=decode(binary(root/name/"post.ssz"));const auto result=prepare_phase0_rewards(f,s,r,rules);
  check(result.registry_after==expected,"official reward mismatch: "+name);check(s==before&&f==fbefore&&r==rbefore,"reward source mutation");
  for(const auto& [i,v]:expected.validators)changed+=v.balance!=s.validators.at(i).balance;
  ++cases;records+=expected.validators.size();leaks+=result.inactivity_leak;genesis+=s.epoch==0;
  auto duplicate=f;duplicate.previous_attestations.insert(duplicate.previous_attestations.end(),f.previous_attestations.begin(),f.previous_attestations.end());
  const auto again=prepare_phase0_rewards(duplicate,s,r,rules);check(again.registry_after==expected&&again.rewards==result.rewards&&again.penalties==result.penalties,"duplicate participation reward");++duplicates;
 }
 size_t cancellations=0;unsigned rejections=0,overflows=0,saturations=0;
 const auto pre=binary(root/"full_attestation_participation/pre.ssz");const auto ledger=decode(pre);const auto f=ffg_state(pre);const auto randao=ffg_randao(pre);
 size_t calls=0;prepare_phase0_rewards(f,ledger,randao,rules,[&]{++calls;});
 for(size_t stop=1;stop<=calls;++stop){size_t n=0;bool threw=false;try{prepare_phase0_rewards(f,ledger,randao,rules,[&]{if(++n==stop)throw std::runtime_error("cancelled");});}catch(const std::runtime_error& e){threw=std::string(e.what())=="cancelled";}check(threw&&ledger==decode(pre)&&f==ffg_state(pre)&&randao==ffg_randao(pre),"cancelled rewards mutated source");++cancellations;}
 auto refuse=[&](auto state,auto registry,auto r){const auto original=registry;bool threw=false;try{prepare_phase0_rewards(state,registry,randao,r);}catch(const std::exception&){threw=true;}check(threw&&registry==original,"invalid rewards accepted");++rejections;};
 auto r=rules;r.base_rewards_per_epoch=0;refuse(f,ledger,r);r=rules;r.proposer_reward_quotient=0;refuse(f,ledger,r);r=rules;r.inactivity_penalty_quotient=0;refuse(f,ledger,r);r=rules;r.duties.effective_balance_increment=0;refuse(f,ledger,r);
 auto s=f;s.checkpoints.finalized.epoch=ledger.epoch;refuse(s,ledger,rules);s=f;s.previous_attestations.at(0).inclusion_delay=0;refuse(s,ledger,rules);s=f;s.previous_attestations.at(0).proposer_index=ledger.validators.size();refuse(s,ledger,rules);s=f;s.previous_attestations.at(0).aggregation_bits.push_back(true);refuse(s,ledger,rules);s=f;s.block_roots.clear();refuse(s,ledger,rules);
 auto l=ledger;l.validators.at(1).public_key=l.validators.at(0).public_key;refuse(f,l,rules);l=ledger;l.validators.erase(0);refuse(f,l,rules);
 const auto baseline=prepare_phase0_rewards(f,ledger,randao,rules);
 l=ledger;size_t last=baseline.rewards.size();while(last>0&&!baseline.rewards[last-1])--last;check(last>0,"missing rewarded overflow candidate");l.validators.at(last-1).balance=UINT64_MAX;const auto original=l;
 bool overflow=false;try{prepare_phase0_rewards(f,l,randao,rules);}catch(const std::overflow_error&){overflow=true;}check(overflow&&l==original,"late reward overflow atomicity");++overflows;
 // Penalties saturate at zero, never wrap into newly created funds.
 s=f;s.previous_attestations.clear();l=ledger;for(auto& [i,v]:l.validators)v.balance=1;
 const auto drained=prepare_phase0_rewards(s,l,randao,rules);for(const auto& [i,v]:drained.registry_after.validators){check(v.balance==0&&drained.rewards[i]==0&&drained.penalties[i]>1,"penalty saturation");++saturations;}
 std::cout<<Json{{"scenario","official-phase0-rewards"},{"cases",cases},{"validatorsCompared",records},{"changedBalances",changed},{"leakCases",leaks},{"genesisCases",genesis},{"duplicateControls",duplicates},{"cancellations",cancellations},{"localRejections",rejections},{"atomicOverflows",overflows},{"saturations",saturations},{"fullEpochTransition",false},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
