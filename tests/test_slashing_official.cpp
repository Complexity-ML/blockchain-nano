#include "support/phase0_minimal_ssz.hpp"
#include "nano/consensus_slashing.hpp"
using namespace test;
using namespace test::phase0_minimal;

int main(int argc,char** argv){try{
 const fs::path root=argc==2?argv[1]:fs::path(NANO_TEST_SOURCE)/"tests/fixtures/consensus-slashing-official";
 auto manifest=load(root/"manifest.json");check(manifest.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908","unexpected fixture revision");
 for(const auto& [name,entry]:manifest.at("referenceFiles").items())check(hex(sha256(binary(root/name))).substr(2)==entry.at("sha256").get<std::string>(),"reference hash mismatch");
 const auto preset=read(root/"minimal-phase0.yaml"),config=read(root/"minimal-config.yaml");
 auto p=[&](const char* key){return parameter(preset,key);};auto c=[&](const char* key){return parameter(config,key);};
 check(p("SLOTS_PER_EPOCH")==8&&p("SLOTS_PER_HISTORICAL_ROOT")==64&&p("EPOCHS_PER_HISTORICAL_VECTOR")==64&&p("EPOCHS_PER_SLASHINGS_VECTOR")==64,"unsupported fixture SSZ preset");
 Phase0SlashingRules rules{c("MIN_PER_EPOCH_CHURN_LIMIT"),c("CHURN_LIMIT_QUOTIENT"),p("MAX_SEED_LOOKAHEAD"),c("MIN_VALIDATOR_WITHDRAWABILITY_DELAY"),p("EPOCHS_PER_SLASHINGS_VECTOR"),p("MIN_SLASHING_PENALTY_QUOTIENT"),p("WHISTLEBLOWER_REWARD_QUOTIENT"),p("PROPOSER_REWARD_QUOTIENT"),p("EFFECTIVE_BALANCE_INCREMENT"),p("PROPORTIONAL_SLASHING_MULTIPLIER")};
 size_t cases=0,balances=0,changed=0;std::set<std::string> names;
 for(const auto& entry:manifest.at("cases")){
  auto name=entry.at("name").get<std::string>();check(names.insert(name).second,"duplicate fixture case");
  for(const auto& [path,info]:entry.at("files").items()){auto b=binary(root/path);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"fixture hash mismatch "+path);}
  auto pre=decode(binary(root/name/"pre.ssz")),expected=decode(binary(root/name/"post.ssz")),actual=pre;
  for(const auto& [index,debit]:phase0_slashing_penalties(pre,rules))actual.validators.at(index).balance-=debit;
  for(const auto& [index,v]:expected.validators){
   check(actual.validators.at(index).balance==v.balance,"official balance mismatch: "+name+" index="+std::to_string(index));
   ++balances;if(v.balance!=pre.validators.at(index).balance)++changed;
  }
  // Compare the unmodified official post-state projection. Epoch rotation is
  // separately tested through the validated wrapper, not this component corpus.
  check(actual==expected,"official slashing state mismatch: "+name);++cases;
 }
 check(names==std::set<std::string>{"low_penalty","max_penalties","minimal_penalty","scaled_penalties","slashings_with_random_state"},"incomplete official fixture selection");
 std::cout<<Json{{"scenario","official-phase0-slashings"},{"cases",cases},{"balancesCompared",balances},{"balancesChanged",changed},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
