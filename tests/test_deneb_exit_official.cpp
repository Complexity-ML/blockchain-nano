#include "support/deneb_minimal_ssz.hpp"
#include "nano/consensus_exit.hpp"
using namespace test;
using test::phase0_minimal::binary;
using test::phase0_minimal::little;
using test::phase0_minimal::parameter;
int main(int argc,char** argv){try{
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures";const fs::path root=argc==2?fs::path(argv[1]):fixtures/"consensus-deneb-exit-official";
 const auto manifest=load(root/"manifest.json");check(manifest.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908","fixture revision");
 const auto refs=fixtures/"consensus-slashing-official";const auto reference_manifest=load(refs/"manifest.json");for(const auto& [path,info]:reference_manifest.at("referenceFiles").items())check(hex(sha256(binary(refs/path))).substr(2)==info.at("sha256").get<std::string>(),"config digest");
 const auto preset=read(refs/"minimal-phase0.yaml"),config=read(refs/"minimal-config.yaml");auto p=[&](const char* key){return parameter(preset,key);};auto c=[&](const char* key){return parameter(config,key);};
 ConsensusBytes4 capella{};std::istringstream lines(config);std::string line;bool found=false;while(std::getline(lines,line))if(line.starts_with("CAPELLA_FORK_VERSION:")){std::string value;std::istringstream(line.substr(line.find(':')+1))>>value;auto raw=unhex(value);check(raw.size()==4,"capella version width");std::copy(raw.begin(),raw.end(),capella.begin());found=true;}check(found,"missing Capella version");
 const Phase0ExitRules rules{c("MIN_PER_EPOCH_CHURN_LIMIT"),c("CHURN_LIMIT_QUOTIENT"),p("MAX_SEED_LOOKAHEAD"),c("MIN_VALIDATOR_WITHDRAWABILITY_DELAY"),c("SHARD_COMMITTEE_PERIOD")};
 unsigned valid=0,invalid=0,future=0,replay=0,local=0,profile_differences=0,invalid_contexts=0;size_t records=0;std::set<std::string> names;
 for(const auto& row:manifest.at("cases")){
  const auto name=row.at("name").get<std::string>();check(names.insert(name).second,"duplicate case");for(const auto& [path,info]:row.at("files").items()){auto b=binary(root/path);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");}
  const auto pre=binary(root/name/"pre.ssz"),operation=binary(root/name/"voluntary_exit.ssz");check(operation.size()==112,"signed exit width");SignedConsensusVoluntaryExit exit{little(operation,0,8),little(operation,8,8),Bytes(operation.begin()+16,operation.end())};
  auto state=deneb_minimal::decode(pre);const auto saved=state;VoluntaryExitDomainContext context{};context.profile=VoluntaryExitProfile::Deneb;context.capella_version=capella;context.fork_epoch=little(pre,56,8);std::copy_n(pre.begin()+48,4,context.previous_version.begin());std::copy_n(pre.begin()+52,4,context.current_version.begin());std::copy_n(pre.begin()+8,32,context.genesis_root.begin());
  const auto accepted=process_consensus_voluntary_exit(state,rules,exit,context);
  if(row.at("valid").get<bool>()){
   check(accepted,"valid Deneb exit rejected: "+name);const auto expected=deneb_minimal::decode(binary(root/name/"post.ssz"));check(state==expected,"official Deneb exit state mismatch: "+name);records+=state.validators.size();++valid;
   const auto after=state;check(!process_consensus_voluntary_exit(state,rules,exit,context)&&state==after,"Deneb exit replay changed registry");++replay;
   // Simulate replacement of both state fork versions without changing Capella.
   auto advanced=context;advanced.previous_version[0]=15;advanced.current_version[0]=16;advanced.fork_epoch=saved.epoch;auto candidate=saved;
   check(process_consensus_voluntary_exit(candidate,rules,exit,advanced)&&candidate==expected,"Capella exit expired with newer state fork versions");++future;
   for(bool change_genesis:{false,true}){auto wrong=context;if(change_genesis)wrong.genesis_root[0]^=1;else wrong.capella_version[0]^=1;auto copy=saved;check(!process_consensus_voluntary_exit(copy,rules,exit,wrong)&&copy==saved,"wrong frozen domain accepted");++local;}
   auto wrong=context;wrong.profile=static_cast<VoluntaryExitProfile>(999);candidate=saved;bool threw=false;try{process_consensus_voluntary_exit(candidate,rules,exit,wrong);}catch(const std::invalid_argument&){threw=true;}check(threw&&candidate==saved,"unknown profile not rejected");++invalid_contexts;
   wrong=context;wrong.fork_epoch=saved.epoch+1;candidate=saved;threw=false;try{process_consensus_voluntary_exit(candidate,rules,exit,wrong);}catch(const std::invalid_argument&){threw=true;}check(threw&&candidate==saved,"future fork context accepted");++invalid_contexts;
  }else{check(!accepted&&state==saved,"invalid Deneb exit accepted: "+name);++invalid;}
  if(name=="voluntary_exit_with_previous_fork_version_not_is_before_fork_epoch"||name=="invalid_voluntary_exit_with_current_fork_version_not_is_before_fork_epoch"){
   auto legacy=context;legacy.profile=VoluntaryExitProfile::Phase0;auto copy=saved;const auto previous_behavior=process_consensus_voluntary_exit(copy,rules,exit,legacy);check(previous_behavior!=accepted,"EIP7044 domain rule has no effect");++profile_differences;
  }
 }
 check(valid==6&&invalid==10&&names.size()==16&&profile_differences==2,"Deneb corpus coverage");
 std::cout<<Json{{"scenario","official-deneb-voluntary-exit"},{"valid",valid},{"invalid",invalid},{"validatorsCompared",records},{"futureForkDomainChecks",future},{"replayRejections",replay},{"localDomainRejections",local},{"configurationRejections",invalid_contexts},{"legacyProfileDifferences",profile_differences},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
