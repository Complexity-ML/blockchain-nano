#include "support/phase0_minimal_ssz.hpp"
#include "nano/consensus_effective_balance.hpp"
using namespace test;
using namespace test::phase0_minimal;
int main(int argc,char** argv){try{
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures";
 const auto root=argc==2?fs::path(argv[1]):fixtures/"consensus-effective-balances-official";
 const auto manifest=load(root/"manifest.json");
 check(manifest.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908"&&manifest.at("cases").size()==1,"fixture coverage/revision");
 const auto refs=fixtures/"consensus-slashing-official";
 const auto ref_manifest=load(refs/"manifest.json");
 for(const auto& [name,info]:ref_manifest.at("referenceFiles").items())
  check(hex(sha256(binary(refs/name))).substr(2)==info.at("sha256").get<std::string>(),"config digest");
 const auto preset=read(refs/"minimal-phase0.yaml");auto p=[&](const char* k){return parameter(preset,k);};
 const Phase0EffectiveBalanceRules rules{p("EFFECTIVE_BALANCE_INCREMENT"),p("MAX_EFFECTIVE_BALANCE"),p("HYSTERESIS_QUOTIENT"),p("HYSTERESIS_DOWNWARD_MULTIPLIER"),p("HYSTERESIS_UPWARD_MULTIPLIER")};
 const auto row=manifest.at("cases").at(0);check(row.at("name")=="effective_balance_hysteresis"&&row.at("valid")==true,"official case");
 for(const auto& [path,info]:row.at("files").items()){auto b=binary(root/path);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");}
 const auto initial=decode(binary(root/"effective_balance_hysteresis/pre.ssz")),before=initial;
 const auto expected=decode(binary(root/"effective_balance_hysteresis/post.ssz"));
 check(prepare_phase0_effective_balances(initial,rules)==expected&&initial==before,"official effective balance mismatch");
 size_t changed=0;for(const auto& [i,v]:expected.validators)changed+=v.effective_balance!=initial.validators.at(i).effective_balance;
 size_t calls=0;prepare_phase0_effective_balances(initial,rules,[&]{++calls;});
 for(size_t stop=1;stop<=calls;++stop){size_t n=0;bool failed=false;try{prepare_phase0_effective_balances(initial,rules,[&]{if(++n==stop)throw std::runtime_error("cancelled");});}catch(const std::runtime_error& e){failed=std::string(e.what())=="cancelled";}check(failed&&initial==before,"cancellation changed input");}
 // Explicit oracle values around strict thresholds, cap, zero and uint64 limit.
 const std::vector<std::array<uint64_t,3>> edges{{{30000000000,29750000000,30000000000}},{{30000000000,29749999999,29000000000}},{{30000000000,31250000000,30000000000}},{{30000000000,31250000001,31000000000}},{{32000000000,UINT64_MAX,32000000000}},{{1000000000,0,0}},{{0,1250000000,0}},{{0,1250000001,1000000000}}};
 for(const auto& e:edges){auto s=initial;s.validators.at(0).effective_balance=e[0];s.validators.at(0).balance=e[1];auto want=s;want.validators.at(0).effective_balance=e[2];s.validators.erase(std::next(s.validators.begin()),s.validators.end());want.validators.erase(std::next(want.validators.begin()),want.validators.end());check(prepare_phase0_effective_balances(s,rules)==want,"threshold boundary or metadata changed");}
 unsigned rejected=0;auto refuse=[&](auto s,auto r){const auto old=s;bool threw=false;try{prepare_phase0_effective_balances(s,r);}catch(const std::exception&){threw=true;}check(threw&&s==old,"invalid input accepted");++rejected;};
 auto r=rules;r.increment=0;refuse(initial,r);r=rules;r.hysteresis_quotient=0;refuse(initial,r);r=rules;r.downward_multiplier=UINT64_MAX;refuse(initial,r);r=rules;r.upward_multiplier=UINT64_MAX;refuse(initial,r);r=rules;r.maximum++;refuse(initial,r);
 auto s=initial;s.validators.at(0).effective_balance=rules.maximum+rules.increment;refuse(s,rules);s=initial;s.validators.at(0).effective_balance=1;refuse(s,rules);s=initial;s.validators.erase(0);refuse(s,rules);
 std::cout<<Json{{"scenario","official-phase0-effective-balances"},{"cases",1},{"validatorsCompared",expected.validators.size()},{"changed",changed},{"boundaries",edges.size()},{"cancellations",calls},{"rejections",rejected},{"fullEpochTransition",false},{"activeInConsensus",false}}.dump()<<std::endl;
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
