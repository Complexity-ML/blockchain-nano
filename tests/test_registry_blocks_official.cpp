#include "support/phase0_minimal_ssz.hpp"
#include "nano/consensus_registry_operations.hpp"
using namespace test;
using namespace test::phase0_minimal;
namespace {
ConsensusRandaoState randao_state(const Bytes& b){ConsensusRandaoState r;r.epoch=little(b,40,8)/8;r.mixes.resize(64);for(size_t i=0;i<64;++i)std::copy_n(b.begin()+4368+i*32,32,r.mixes[i].begin());r.last_reveal_slot=little(b,64,8);return r;}
}
int main(int argc,char** argv){try{
 const fs::path root=argc==2?fs::path(argv[1]):fs::path(NANO_TEST_SOURCE)/"tests/fixtures/consensus-registry-blocks-official";
 const auto manifest=load(root/"manifest.json");check(manifest.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908","fixture revision");
 const Phase0BodyLimits limits{16,2,128,16,16,2048};const ConsensusRandaoRules randao_rules{8,1};const Phase0SlashingRules slash_rules{2,32,4,256,64,64,512,8,1000000000,2};
 const Phase0DepositRules deposit_rules{1000000000,32000000000ULL,uint64_t(1)<<40,{0,0,0,1}};const Phase0ExitRules exit_rules{2,32,4,256,64};
 unsigned valid=0,invalid=0,operations=0,deposits=0,exits=0,deferred_epochs=0;size_t balances=0;std::set<std::string> names;
 for(const auto& row:manifest.at("cases")){
  auto name=row.at("name").get<std::string>();check(names.insert(name).second,"duplicate case");
  for(const auto& [path,info]:row.at("files").items()){auto b=binary(root/path);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"fixture digest mismatch");}
  if(row.at("blockCount").get<size_t>()>1){
   const auto first=binary(root/name/"blocks_0.ssz"),last=binary(root/name/("blocks_"+std::to_string(row.at("blockCount").get<size_t>()-1)+".ssz"));
   check(little(first,100,8)/8!=little(last,100,8)/8,"deferred corpus no longer requires epoch processing");++deferred_epochs;continue;
  }
  const auto pre=binary(root/name/"pre.ssz"),block=binary(root/name/"blocks_0.ssz");auto registry=decode(pre);auto randao=randao_state(pre);const auto old_registry=registry;const auto old_randao=randao;
  Phase0StateFork fork{};std::copy_n(pre.begin()+48,4,fork.previous_version.begin());std::copy_n(pre.begin()+52,4,fork.current_version.begin());fork.epoch=little(pre,56,8);
  Phase0BlockContext context{};context.slot=little(block,100,8);context.fork_version=context.slot/8<fork.epoch?fork.previous_version:fork.current_version;context.maximum_effective_balance=32000000000ULL;context.shuffle_rounds=10;context.maximum_serialized_bytes=1024*1024;std::copy_n(pre.begin()+8,32,context.genesis_root.begin());
  check(context.slot/8==registry.epoch,"selected corpus must not skip epoch processing");
  // Invalid-fixture parent supplied as context so only the late operation fails.
  std::copy_n(block.begin()+116,32,context.parent_root.begin());
  if(row.at("valid").get<bool>()){const auto post=binary(root/name/"post.ssz");check(context.slot>0&&std::equal(context.parent_root.begin(),context.parent_root.end(),post.begin()+176+((context.slot-1)%64)*32),"official parent commitment mismatch");}
  Phase0DepositState state{registry,{},little(pre,4308,8),little(pre,4352,8)};std::copy_n(pre.begin()+4276,32,state.deposit_root.begin());const auto original=state;
  auto result=prepare_phase0_registry_operations(block,context,limits,randao_rules,slash_rules,deposit_rules,exit_rules,fork,state,randao);
  check(state==original,"official registry pipeline mutated source");
  check(registry==old_registry&&randao==old_randao,"official processing mutated source");
  if(row.at("valid").get<bool>()){
   check(std::holds_alternative<Phase0RegistryOperationsCandidate>(result),"valid official block operations rejected: "+name);const auto& candidate=std::get<Phase0RegistryOperationsCandidate>(result);const auto post=binary(root/name/"post.ssz");
   check(candidate.deposits_after.registry==decode(post),"official block registry mismatch: "+name);
   check(candidate.authenticated.randao_after==randao_state(post),"official block RANDAO mismatch: "+name);
   check(candidate.deposits_after.next_deposit_index==little(post,4352,8)&&candidate.deposits_after.deposit_count==little(post,4308,8)&&std::equal(candidate.deposits_after.deposit_root.begin(),candidate.deposits_after.deposit_root.end(),post.begin()+4276),"official deposit cursor/root mismatch");
   check(std::equal(candidate.authenticated.header.body_root.begin(),candidate.authenticated.header.body_root.end(),post.begin()+144),"official block body root mismatch");
   operations+=candidate.proposer_slashings_processed;deposits+=candidate.deposits_processed;exits+=candidate.exits_processed;balances+=candidate.deposits_after.registry.validators.size();++valid;
  }else{
   // Prove it reached the operation gate, rather than failing envelope auth.
   check(bool(authenticate_phase0_block(block,context,limits,randao_rules,registry,randao)),"invalid operation fixture failed early block authentication");
   check(std::holds_alternative<InvalidPhase0RegistryBlock>(result),"invalid official operations accepted: "+name);++invalid;
  }
 }
 check(valid==3&&invalid==3&&deferred_epochs==2&&names.size()==8,"official selection count");
 std::cout<<Json{{"scenario","official-phase0-registry-block-operations"},{"valid",valid},{"invalid",invalid},{"deferredEpochSequences",deferred_epochs},{"proposerSlashings",operations},{"deposits",deposits},{"exits",exits},{"balancesCompared",balances},{"canonicalInputsUnchanged",true},{"fullStateTransition",false},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
