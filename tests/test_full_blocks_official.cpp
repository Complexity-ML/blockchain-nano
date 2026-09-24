#include "support/phase0_epoch_ssz.hpp"
#include "nano/consensus_transition.hpp"
using namespace test;
using namespace test::phase0_minimal;
int main(int argc,char** argv){try{
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures";
 const auto root=argc==2?fs::path(argv[1]):fixtures/"consensus-full-blocks-official";
 const auto epoch=epoch_rules(fixtures/"consensus-slashing-official");const auto preset=read(fixtures/"consensus-slashing-official/minimal-phase0.yaml"),config=read(fixtures/"consensus-slashing-official/minimal-config.yaml");auto p=[&](const char* k){return parameter(preset,k);};
 const Phase0StateLimits limits{epoch.history.slots_per_historical_root,epoch.epochs_per_randao_vector,epoch.slashing.epochs_per_slashings_vector,epoch.history.historical_roots_limit,epoch.epochs_per_eth1_voting_period*epoch.history.slots_per_epoch,p("VALIDATOR_REGISTRY_LIMIT"),epoch.maximum_pending_attestations,p("MAX_VALIDATORS_PER_COMMITTEE")};
 const Phase0TransitionRules rules{epoch,limits,{uint32_t(p("MAX_PROPOSER_SLASHINGS")),uint32_t(p("MAX_ATTESTER_SLASHINGS")),uint32_t(p("MAX_ATTESTATIONS")),uint32_t(p("MAX_DEPOSITS")),uint32_t(p("MAX_VOLUNTARY_EXITS")),uint32_t(p("MAX_VALIDATORS_PER_COMMITTEE"))},{p("EFFECTIVE_BALANCE_INCREMENT"),p("MAX_EFFECTIVE_BALANCE"),p("VALIDATOR_REGISTRY_LIMIT"),{0,0,0,1}},{epoch.registry.minimum_churn_limit,epoch.registry.churn_limit_quotient,epoch.registry.max_seed_lookahead,epoch.registry.minimum_withdrawability_delay,parameter(config,"SHARD_COMMITTEE_PERIOD")},p("MIN_ATTESTATION_INCLUSION_DELAY"),8*1024*1024};
 const auto m=load(root/"manifest.json");check(m.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908"&&m.at("cases").size()==45,"full block corpus");
 unsigned valid=0,invalid=0;size_t accepted=0,bytes_compared=0;
 for(const auto& row:m.at("cases")){
  const auto name=row.at("name").get<std::string>();
  for(const auto& [path,info]:row.at("files").items()){const auto b=binary(root/path);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");}
  Phase0SlotState state{binary(root/name/"pre.ssz"),std::nullopt};bool rejected=false;
  for(size_t i=0;i<row.at("blockCount").get<size_t>();++i){const auto before=state;const auto b=binary(root/name/("blocks_"+std::to_string(i)+".ssz"));
   std::optional<Phase0SlotState> candidate;
   try{candidate=prepare_phase0_state_transition(state,b,rules,4096);}catch(const std::exception& e){throw std::runtime_error("full block exception "+name+" #"+std::to_string(i)+": "+e.what());}
   check(state==before,"block input mutation");
   if(!candidate){rejected=true;break;}state=std::move(*candidate);++accepted;
  }
  if(row.at("valid").get<bool>()){
   check(!rejected,"valid full block rejected: "+name);const auto expected=binary(root/name/"post.ssz");check(state.ssz==expected,"official full block state mismatch: "+name);bytes_compared+=expected.size();++valid;
  }else{check(rejected,"invalid full block accepted: "+name);++invalid;}
 }
 const Phase0SlotState input{binary(root/"empty_block_transition/pre.ssz"),std::nullopt};const auto block=binary(root/"empty_block_transition/blocks_0.ssz");const auto before=input;
 size_t calls=0;check(bool(prepare_phase0_state_transition(input,block,rules,4096,[&]{++calls;})),"local positive block");
 std::set<size_t> stops{1,2,calls/4,calls/2,3*calls/4,calls-1,calls};
 for(size_t stop:stops){size_t n=0;bool aborted=false;try{prepare_phase0_state_transition(input,block,rules,4096,[&]{if(++n==stop)throw std::runtime_error("cancelled");});}catch(const std::runtime_error& e){aborted=std::string(e.what())=="cancelled";}check(aborted&&input==before,"cancelled full block changed input");}
 unsigned mutations=0;for(size_t at:{size_t(4),size_t(116),size_t(148),size_t(184)}){auto b=block;b[at]^=1;check(!prepare_phase0_state_transition(input,b,rules,4096)&&input==before,"altered signed block accepted");++mutations;}
 for(size_t cut:{size_t(0),size_t(99),size_t(183),size_t(403)}){check(!prepare_phase0_state_transition(input,Bytes(block.begin(),block.begin()+cut),rules,4096),"truncated block accepted");++mutations;}
 unsigned config_rejects=0;auto reject=[&](auto cfg,uint64_t budget){bool threw=false;try{prepare_phase0_state_transition(input,block,cfg,budget);}catch(const std::exception&){threw=true;}check(threw&&input==before,"bad block config accepted");++config_rejects;};
 reject(rules,0);auto bad=rules;bad.deposit.validator_registry_limit++;reject(bad,4096);bad=rules;bad.body.validators_per_committee++;reject(bad,4096);bad=rules;bad.maximum_block_bytes=403;reject(bad,4096);
 // The official wrong-state-root block has a valid proposer signature and
 // RANDAO; prove its rejection is not merely an envelope signature failure.
 const Phase0SlotState wrong_pre{binary(root/"invalid_incorrect_state_root/pre.ssz"),std::nullopt};const auto wrong_block=binary(root/"invalid_incorrect_state_root/blocks_0.ssz");
 const auto advanced=prepare_phase0_slots(wrong_pre,epoch,limits,little(wrong_block,100,8),4096);const auto projected=epoch_state(advanced.ssz);
 const ConsensusBeaconHeader h{little(advanced.ssz,64,8),little(advanced.ssz,72,8),root_at(advanced.ssz,80),root_at(advanced.ssz,112),root_at(advanced.ssz,144)};
 ConsensusBytes4 version{};std::copy_n(advanced.ssz.begin()+52,4,version.begin());
 const Phase0BlockContext context{little(wrong_block,100,8),consensus_beacon_header_root(h),version,root_at(advanced.ssz,8),p("MAX_EFFECTIVE_BALANCE"),uint16_t(p("SHUFFLE_ROUND_COUNT")),rules.maximum_block_bytes};
 check(bool(authenticate_phase0_block(wrong_block,context,rules.body,{p("SLOTS_PER_EPOCH"),p("MIN_SEED_LOOKAHEAD")},projected.registry,projected.randao)),"wrong-root fixture fails envelope authentication");
 check(!prepare_phase0_state_transition(wrong_pre,wrong_block,rules,4096),"authenticated wrong state root accepted");
 std::cout<<Json{{"scenario","official-phase0-full-blocks"},{"valid",valid},{"invalid",invalid},{"acceptedBlocks",accepted},{"serializedBytesCompared",bytes_compared},{"stateRootsVerified",true},{"authenticatedWrongRootRejections",1},{"cancellationPoints",stops.size()},{"mutations",mutations},{"configurationRejections",config_rejects},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
