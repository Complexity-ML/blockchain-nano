#include "support/phase0_ffg_ssz.hpp"
#include "nano/consensus_attestation_inclusion.hpp"
using namespace test;
using namespace test::phase0_minimal;
namespace {
Phase0SignedAttestation attestation(const Bytes& b){
 check(b.size()>=229&&little(b,0,4)==228&&b.back()!=0,"attestation SSZ encoding");Phase0SignedAttestation a;auto& d=a.data;d.slot=little(b,4,8);d.index=little(b,12,8);d.beacon_block_root=root_at(b,20);d.source=checkpoint_at(b,52);d.target=checkpoint_at(b,92);a.signature=Bytes(b.begin()+132,b.begin()+228);
 const auto n=(b.size()-229)*8+std::bit_width(unsigned(b.back()))-1;check(n<=2048,"attestation bitlist bound");for(size_t i=0;i<n;++i)a.aggregation_bits.push_back(bool((b[228+i/8]>>(i%8))&1));return a;
}
}
int main(int argc,char** argv){try{
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures";const fs::path root=argc==2?fs::path(argv[1]):fixtures/"consensus-attestation-inclusion-official";
 const auto manifest=load(root/"manifest.json");check(manifest.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908","fixture revision");
 const auto refs=fixtures/"consensus-slashing-official";const auto reference=load(refs/"manifest.json");for(const auto& [name,info]:reference.at("referenceFiles").items())check(hex(sha256(binary(refs/name))).substr(2)==info.at("sha256").get<std::string>(),"config digest");const auto preset=read(refs/"minimal-phase0.yaml");auto p=[&](const char* key){return parameter(preset,key);};
 const Phase0FfgRules duties{p("SLOTS_PER_EPOCH"),p("MIN_SEED_LOOKAHEAD"),p("TARGET_COMMITTEE_SIZE"),p("MAX_COMMITTEES_PER_SLOT"),p("EFFECTIVE_BALANCE_INCREMENT"),p("MAX_EFFECTIVE_BALANCE"),uint16_t(p("SHUFFLE_ROUND_COUNT"))};const Phase0AttestationInclusionRules rules{duties,p("MIN_ATTESTATION_INCLUSION_DELAY"),p("MAX_ATTESTATIONS")*duties.slots_per_epoch};
 unsigned valid=0,invalid=0,mutations=0,duplicates=0,capacity=0,cancelled=0,configurations=0,deferred=0;size_t appended=0;std::set<std::string> names;
 for(const auto& row:manifest.at("cases")){
  const auto name=row.at("name").get<std::string>();check(names.insert(name).second,"duplicate case");for(const auto& [path,info]:row.at("files").items()){auto b=binary(root/path);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");}
  const auto pre=binary(root/name/"pre.ssz");auto state=ffg_state(pre);const auto initial=state;const auto registry=decode(pre),original_registry=registry;const auto randao=ffg_randao(pre),original_randao=randao;const auto att=attestation(binary(root/name/"attestation.ssz"));const auto genesis=root_at(pre,8);
  Phase0StateFork fork{};std::copy_n(pre.begin()+48,4,fork.previous_version.begin());std::copy_n(pre.begin()+52,4,fork.current_version.begin());fork.epoch=little(pre,56,8);
  const auto accepted=process_phase0_attestation(state,registry,randao,rules,fork,genesis,att);
  check(registry==original_registry&&randao==original_randao,"attestation changed registry/RANDAO");
  if(row.at("valid").get<bool>()){
   check(accepted,"valid official attestation rejected: "+name);const auto post=binary(root/name/"post.ssz");check(state==ffg_state(post),"official pending state mismatch: "+name);check(decode(post)==registry&&ffg_randao(post)==randao,"unexpected official non-pending mutation");++valid;
   const auto total=state.previous_attestations.size()+state.current_attestations.size();check(total==initial.previous_attestations.size()+initial.current_attestations.size()+1,"single inclusion count");++appended;
   auto duplicate=state;check(process_phase0_attestation(duplicate,registry,randao,rules,fork,genesis,att),"Phase0 repeat inclusion unexpectedly forbidden");check(duplicate.previous_attestations.size()+duplicate.current_attestations.size()==total+1,"duplicate append lost");++duplicates;
   auto limited=rules;limited.maximum_pending_attestations=att.data.target.epoch==registry.epoch?state.current_attestations.size():state.previous_attestations.size();auto full=state;check(!process_phase0_attestation(full,registry,randao,limited,fork,genesis,att)&&full==state,"pending capacity changed state");++capacity;
   auto refuse=[&](const Phase0SignedAttestation& a,const ConsensusRoot& g){auto s=initial;check(!process_phase0_attestation(s,registry,randao,rules,fork,g,a)&&s==initial,"invalid attestation published");++mutations;};
   auto bad=att;bad.signature[0]^=1;refuse(bad,genesis);bad=att;bad.data.source.root[0]^=1;refuse(bad,genesis);bad=att;bad.aggregation_bits.push_back(false);refuse(bad,genesis);bad=att;std::fill(bad.aggregation_bits.begin(),bad.aggregation_bits.end(),false);refuse(bad,genesis);auto wrong_genesis=genesis;wrong_genesis[0]^=1;refuse(att,wrong_genesis);
   if(name=="one_basic_attestation"){
    size_t total_calls=0;auto s=initial;check(process_phase0_attestation(s,registry,randao,rules,fork,genesis,att,[&]{++total_calls;}),"cancellation control");
    auto invalid_context=[&](const Phase0AttestationInclusionRules& cfg,const ConsensusSlashingState& reg,const ConsensusRandaoState& r,const Phase0StateFork& f){auto copy=initial;bool threw=false;try{process_phase0_attestation(copy,reg,r,cfg,f,genesis,att);}catch(const std::invalid_argument&){threw=true;}check(threw&&copy==initial,"bad context mutated pending state");++configurations;};
    auto cfg=rules;cfg.minimum_inclusion_delay=0;invalid_context(cfg,registry,randao,fork);cfg=rules;cfg.minimum_inclusion_delay=cfg.duties.slots_per_epoch+1;invalid_context(cfg,registry,randao,fork);cfg=rules;cfg.maximum_pending_attestations=0;invalid_context(cfg,registry,randao,fork);cfg=rules;cfg.duties.slots_per_epoch=0;invalid_context(cfg,registry,randao,fork);cfg=rules;cfg.duties.target_committee_size=0;invalid_context(cfg,registry,randao,fork);
    auto r=randao;r.epoch+=1;invalid_context(rules,registry,r,fork);auto f=fork;f.epoch=registry.epoch+1;invalid_context(rules,registry,randao,f);auto reg=registry;reg.validators.at(1).public_key=reg.validators.at(0).public_key;invalid_context(rules,reg,randao,fork);
    for(size_t stop=1;stop<=total_calls;++stop){s=initial;size_t calls=0;bool threw=false;try{process_phase0_attestation(s,registry,randao,rules,fork,genesis,att,[&]{if(++calls==stop)throw std::runtime_error("cancelled");});}catch(const std::runtime_error& e){threw=std::string(e.what())=="cancelled";}check(threw&&s==initial,"cancelled admission appended");++cancelled;}
   }
  }else{check(!accepted&&state==initial,"invalid official attestation accepted: "+name);++invalid;}
 }
 const auto blocks=fixtures/"consensus-attestation-blocks-official";const auto block_manifest=load(blocks/"manifest.json");check(block_manifest.at("commit")==manifest.at("commit"),"block corpus revision");
 for(const auto& row:block_manifest.at("cases")){check(row.at("blockCount")==2,"deferred sequence changed");for(const auto& [path,info]:row.at("files").items()){auto b=binary(blocks/path);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"deferred block digest");}const auto dir=blocks/row.at("name").get<std::string>();check(little(binary(dir/"blocks_0.ssz"),100,8)/duties.slots_per_epoch!=little(binary(dir/"blocks_1.ssz"),100,8)/duties.slots_per_epoch,"deferred sequence does not cross epoch");++deferred;}
 check(deferred==2,"deferred block count");
 check(names.size()==41&&valid==17&&invalid==24,"official admission coverage");
 std::cout<<Json{{"scenario","official-phase0-attestation-inclusion"},{"valid",valid},{"invalid",invalid},{"pendingAppended",appended},{"mutationRejections",mutations},{"duplicateInclusions",duplicates},{"capacityRejections",capacity},{"cancellations",cancelled},{"configurationRejections",configurations},{"deferredBlockSequences",deferred},{"activeInConsensus",false},{"fullBlockTransition",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
