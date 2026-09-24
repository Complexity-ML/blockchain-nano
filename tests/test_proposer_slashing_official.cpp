#include "support/phase0_minimal_ssz.hpp"
#include "nano/consensus_proposer.hpp"
using namespace test;
using namespace test::phase0_minimal;
namespace {
SignedConsensusBeaconHeader header(const Bytes& bytes,size_t at){
 check(at<=bytes.size()&&bytes.size()-at>=208,"signed header size");SignedConsensusBeaconHeader out;out.message.slot=little(bytes,at,8);out.message.proposer_index=little(bytes,at+8,8);
 std::copy_n(bytes.begin()+at+16,32,out.message.parent_root.begin());std::copy_n(bytes.begin()+at+48,32,out.message.state_root.begin());std::copy_n(bytes.begin()+at+80,32,out.message.body_root.begin());out.signature=Bytes(bytes.begin()+at+112,bytes.begin()+at+208);return out;
}
}
int main(int argc,char** argv){try{
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures";
 const fs::path root=argc==2?fs::path(argv[1]):fixtures/"consensus-proposer-slashing-official";
 const auto manifest=load(root/"manifest.json");check(manifest.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908","fixture revision");
 const auto references=fixtures/"consensus-slashing-official";const auto ref_manifest=load(references/"manifest.json");
 for(const auto& [name,entry]:ref_manifest.at("referenceFiles").items())check(hex(sha256(binary(references/name))).substr(2)==entry.at("sha256").get<std::string>(),"reference file digest");
 const auto preset=read(references/"minimal-phase0.yaml"),config=read(references/"minimal-config.yaml");auto p=[&](const char* key){return parameter(preset,key);};auto c=[&](const char* key){return parameter(config,key);};
 const Phase0SlashingRules rules{c("MIN_PER_EPOCH_CHURN_LIMIT"),c("CHURN_LIMIT_QUOTIENT"),p("MAX_SEED_LOOKAHEAD"),c("MIN_VALIDATOR_WITHDRAWABILITY_DELAY"),p("EPOCHS_PER_SLASHINGS_VECTOR"),p("MIN_SLASHING_PENALTY_QUOTIENT"),p("WHISTLEBLOWER_REWARD_QUOTIENT"),p("PROPOSER_REWARD_QUOTIENT"),p("EFFECTIVE_BALANCE_INCREMENT"),p("PROPORTIONAL_SLASHING_MULTIPLIER")};
 unsigned valid=0,invalid=0,replays=0,local_rejections=0,overflows=0,invalid_registries=0;size_t balances=0;std::set<std::string> names;
 for(const auto& row:manifest.at("cases")){
  const auto name=row.at("name").get<std::string>();check(names.insert(name).second,"duplicate fixture");
  for(const auto& [path,info]:row.at("files").items()){auto b=binary(root/path);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"fixture digest: "+path);}
  const auto pre=binary(root/name/"pre.ssz"),operation=binary(root/name/"proposer_slashing.ssz");check(operation.size()==416,"proposer proof SSZ size");const auto first=header(operation,0),second=header(operation,208);
  auto state=decode(pre);const auto saved=state;
  ConsensusRandaoState randao;randao.epoch=state.epoch;randao.mixes.resize(64);for(size_t i=0;i<64;++i)std::copy_n(pre.begin()+4368+i*32,32,randao.mixes[i].begin());
  std::vector<Phase0ProposerValidator> candidates;for(const auto& [index,v]:state.validators)candidates.push_back({index,v.effective_balance,v.activation_epoch,v.exit_epoch});
  const auto proposer=phase0_proposer_index(candidates,randao,{8,1},little(pre,40,8),p("MAX_EFFECTIVE_BALANCE"),p("SHUFFLE_ROUND_COUNT"));
  ConsensusRoot genesis{};std::copy_n(pre.begin()+8,32,genesis.begin());ConsensusBytes4 version{};std::copy_n(pre.begin()+(first.message.slot/8<little(pre,56,8)?48:52),4,version.begin());
  std::optional<std::vector<uint64_t>> result;
  try{result=process_phase0_proposer_slashing(state,rules,first,second,version,genesis,proposer,proposer);}catch(const std::invalid_argument& e){
   // Upstream intentionally sets withdrawable_epoch=0 while exit_epoch remains
   // FAR_FUTURE in this invalid component fixture. Keep strict registry checks.
   check(name=="invalid_proposer_is_withdrawn"&&!row.at("valid").get<bool>()&&std::string(e.what())=="invalid slashing registry","unexpected registry rejection: "+name);++invalid_registries;
  }
  if(row.at("valid").get<bool>()){
   check(result&&*result==std::vector<uint64_t>{first.message.proposer_index},"valid proof rejected: "+name);
   const auto expected=decode(binary(root/name/"post.ssz"));check(state==expected,"official proposer slashing transition mismatch: "+name);balances+=state.validators.size();++valid;
   const auto after=state;check(!process_phase0_proposer_slashing(state,rules,first,second,version,genesis,proposer,proposer)&&state==after,"proof replay double charge");++replays;
   auto restored=phase0_slashing_from_snapshot(phase0_slashing_snapshot(state,rules),rules);check(!process_phase0_proposer_slashing(restored,rules,first,second,version,genesis,proposer,proposer)&&restored==after,"snapshot replay double charge");++replays;
   auto reject=[&](const auto& a,const auto& b,const auto& fork,const auto& chain){auto candidate=saved;check(!process_phase0_proposer_slashing(candidate,rules,a,b,fork,chain,proposer,proposer)&&candidate==saved,"local invalid proof changed state");++local_rejections;};
   auto wrong_version=version;wrong_version[0]^=1;reject(first,second,wrong_version,genesis);auto wrong_genesis=genesis;wrong_genesis[0]^=1;reject(first,second,version,wrong_genesis);
   auto short_first=first;short_first.signature.pop_back();reject(short_first,second,version,genesis);reject(first,first,version,genesis);
   auto overflow=saved;overflow.slashings.at(state.epoch%rules.epochs_per_slashings_vector)=UINT64_MAX;const auto before=overflow;bool threw=false;try{process_phase0_proposer_slashing(overflow,rules,first,second,version,genesis,proposer,proposer);}catch(const std::overflow_error&){threw=true;}check(threw&&overflow==before,"overflow partially slashed proposer");++overflows;
  }else{check(!result&&state==saved,"invalid official proof accepted or state changed: "+name);++invalid;}
 }
 check(names.size()==15&&valid==3&&invalid==12&&invalid_registries==1,"official corpus completeness");
 std::cout<<Json{{"scenario","official-proposer-slashing"},{"valid",valid},{"invalid",invalid},{"invalidRegistries",invalid_registries},{"balancesCompared",balances},{"replayRejections",replays},{"localRejections",local_rejections},{"atomicOverflowRejections",overflows},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
