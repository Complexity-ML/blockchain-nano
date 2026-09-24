#include "support/phase0_minimal_ssz.hpp"
#include "nano/consensus_exit.hpp"
using namespace test;
using namespace test::phase0_minimal;
int main(int argc,char** argv){try{
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures";const fs::path root=argc==2?fs::path(argv[1]):fixtures/"consensus-voluntary-exit-official";
 const auto manifest=load(root/"manifest.json");check(manifest.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908","fixture revision");
 const auto references=fixtures/"consensus-slashing-official";const auto ref_manifest=load(references/"manifest.json");for(const auto& [name,entry]:ref_manifest.at("referenceFiles").items())check(hex(sha256(binary(references/name))).substr(2)==entry.at("sha256").get<std::string>(),"reference digest");
 const auto preset=read(references/"minimal-phase0.yaml"),config=read(references/"minimal-config.yaml");auto p=[&](const char* key){return parameter(preset,key);};auto c=[&](const char* key){return parameter(config,key);};
 const Phase0ExitRules rules{c("MIN_PER_EPOCH_CHURN_LIMIT"),c("CHURN_LIMIT_QUOTIENT"),p("MAX_SEED_LOOKAHEAD"),c("MIN_VALIDATOR_WITHDRAWABILITY_DELAY"),c("SHARD_COMMITTEE_PERIOD")};
 unsigned valid=0,invalid=0,replays=0,local=0,overflows=0;size_t validators=0;std::set<std::string> names;
 for(const auto& row:manifest.at("cases")){
  const auto name=row.at("name").get<std::string>();check(names.insert(name).second,"duplicate fixture");
  for(const auto& [path,info]:row.at("files").items()){auto b=binary(root/path);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");}
  const auto pre=binary(root/name/"pre.ssz"),operation=binary(root/name/"voluntary_exit.ssz");check(operation.size()==112,"exit SSZ width");
  const SignedConsensusVoluntaryExit exit{little(operation,0,8),little(operation,8,8),Bytes(operation.begin()+16,operation.end())};
  auto state=decode(pre);const auto initial=state;ConsensusRoot genesis{};std::copy_n(pre.begin()+8,32,genesis.begin());ConsensusBytes4 version{};std::copy_n(pre.begin()+(exit.epoch<little(pre,56,8)?48:52),4,version.begin());
  const bool accepted=process_phase0_voluntary_exit(state,rules,exit,version,genesis);
  if(row.at("valid").get<bool>()){
   check(accepted,"valid exit rejected: "+name);const auto expected=decode(binary(root/name/"post.ssz"));check(state==expected,"official exit state mismatch: "+name);validators+=state.validators.size();++valid;
   for(const auto& [index,v]:state.validators)check(v.balance==initial.validators.at(index).balance,"exit debited balance");
   const auto after=state;check(!process_phase0_voluntary_exit(state,rules,exit,version,genesis)&&state==after,"exit replay changed state");++replays;
   auto refuse=[&](const auto& request,const auto& fork,const auto& chain){auto next=initial;check(!process_phase0_voluntary_exit(next,rules,request,fork,chain)&&next==initial,"invalid exit changed state");++local;};
   auto bad_version=version;bad_version[0]^=1;refuse(exit,bad_version,genesis);auto bad_genesis=genesis;bad_genesis[0]^=1;refuse(exit,version,bad_genesis);
   auto bad=exit;bad.signature.pop_back();refuse(bad,version,genesis);bad=exit;bad.signature[0]^=1;refuse(bad,version,genesis);bad=exit;bad.epoch=UINT64_MAX;refuse(bad,version,genesis);bad=exit;bad.validator_index=UINT64_MAX;refuse(bad,version,genesis);
   // Signature remains valid but processing near the epoch limit cannot wrap.
   auto high=initial;high.epoch=UINT64_MAX-1;for(auto& [index,v]:high.validators)if(index!=exit.validator_index){v.exit_epoch=UINT64_MAX;v.withdrawable_epoch=UINT64_MAX;}
   const auto before=high;bool overflow=false;try{process_phase0_voluntary_exit(high,rules,exit,version,genesis);}catch(const std::overflow_error&){overflow=true;}check(overflow&&high==before,"overflow exit partially changed state");++overflows;
  }else{check(!accepted&&state==initial,"invalid exit accepted: "+name);++invalid;}
 }
 check(valid==4&&invalid==6&&names.size()==10,"official exit corpus coverage");
 std::cout<<Json{{"scenario","official-voluntary-exit"},{"valid",valid},{"invalid",invalid},{"validatorsCompared",validators},{"replayRejections",replays},{"localRejections",local},{"atomicOverflowRejections",overflows},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
