#include "support/altair_transition_rules.hpp"
using namespace test;using namespace test::phase0_minimal;
int main(){try{
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures",root=fixtures/"consensus-altair-full-blocks-official";const auto r=altair_transition_rules(fixtures);const auto m=load(root/"manifest.json");check(m.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908"&&m.at("cases").size()==52,"Altair full block corpus");
 unsigned valid=0,invalid=0;size_t accepted=0,bytes_compared=0,constructed=0,invalid_construction=0,reconstructed_envelopes=0;
 for(const auto& row:m.at("cases")){
  const auto name=row.at("name").get<std::string>();for(const auto& [path,info]:row.at("files").items()){const auto b=binary(root/path);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");}
  AltairSlotState state{binary(root/name/"pre.ssz"),std::nullopt};bool rejected=false;
  for(size_t i=0;i<row.at("blockCount").get<size_t>();++i){const auto before=state;const auto b=binary(root/name/("blocks_"+std::to_string(i)+".ssz"));std::optional<AltairSlotState> candidate;
   try{candidate=prepare_altair_state_transition(state,b,r,4096,100000);}catch(const std::exception& e){throw std::runtime_error("Altair block exception "+name+" #"+std::to_string(i)+": "+e.what());}check(state==before,"block input mutation");if(!candidate){
    auto local=prepare_altair_unsigned_block(state,little(b,100,8),little(b,108,8),root_at(b,116),std::span<const uint8_t>(b).subspan(184),r,4096,100000);
    const bool envelope_only=name=="invalid_all_zeroed_sig"||name=="invalid_incorrect_block_sig"||name=="invalid_incorrect_state_root";
    check(bool(local)==envelope_only&&state==before,"invalid official body construction outcome: "+name);
    if(envelope_only)++reconstructed_envelopes;else ++invalid_construction;
    rejected=true;break;
   }auto built=prepare_altair_unsigned_block(state,little(b,100,8),little(b,108,8),root_at(b,116),std::span<const uint8_t>(b).subspan(184),r,4096,100000);
   check(built&&built->post_state==*candidate&&built->message_ssz==Bytes(b.begin()+100,b.end())&&state==before,"constructed official block differs: "+name);++constructed;
   state=std::move(*candidate);++accepted;
  }
  if(row.at("valid").get<bool>()){check(!rejected,"valid Altair block rejected: "+name);const auto expected=binary(root/name/"post.ssz");check(state.ssz==expected,"official Altair state mismatch: "+name);bytes_compared+=expected.size();++valid;}else{check(rejected,"invalid Altair block accepted: "+name);++invalid;}
 }
 check(valid==37&&invalid==15,"Altair block outcomes");const AltairSlotState input{binary(root/"empty_block_transition/pre.ssz"),std::nullopt};const auto block=binary(root/"empty_block_transition/blocks_0.ssz");const auto before=input;
 size_t calls=0;const auto result=prepare_altair_state_transition(input,block,r,4096,100000,[&]{++calls;});check(bool(result),"local positive block");check(!prepare_altair_state_transition(*result,block,r,4096,100000),"same-slot replay accepted");
 std::set<size_t> stops{1,2,calls/4,calls/2,3*calls/4,calls-1,calls};for(size_t stop:stops){size_t n=0;bool aborted=false;try{prepare_altair_state_transition(input,block,r,4096,100000,[&]{if(++n==stop)throw std::runtime_error("cancelled");});}catch(const std::runtime_error& e){aborted=std::string(e.what())=="cancelled";}check(aborted&&input==before,"cancelled block changed input");}
 unsigned mutations=0;for(size_t at:{size_t(4),size_t(116),size_t(148),size_t(184),size_t(384),size_t(404)}){auto b=block;b[at]^=1;check(!prepare_altair_state_transition(input,b,r,4096,100000)&&input==before,"altered block accepted");++mutations;}for(size_t cut:{size_t(0),size_t(99),size_t(183),size_t(403),size_t(503)}){check(!prepare_altair_state_transition(input,Bytes(block.begin(),block.begin()+cut),r,4096,100000),"truncated block accepted");++mutations;}
 // Local construction still validates the body, selected proposer and parent.
 auto build=[&](const Bytes& b,const std::function<void()>& checkpoint={}){return prepare_altair_unsigned_block(input,little(b,100,8),little(b,108,8),root_at(b,116),std::span<const uint8_t>(b).subspan(184),r,4096,100000,checkpoint);};
 unsigned construction_rejections=0;
 for(size_t at:{size_t(108),size_t(116),size_t(184),size_t(384),size_t(404)}){auto b=block;b[at]^=1;check(!build(b)&&input==before,"invalid local candidate accepted");++construction_rejections;}
 auto built=build(block);check(built.has_value(),"local block construction failed");
 Bytes unsigned_wire(100,0);unsigned_wire[0]=100;unsigned_wire.insert(unsigned_wire.end(),built->message_ssz.begin(),built->message_ssz.end());
 check(!prepare_altair_state_transition(input,unsigned_wire,r,4096,100000),"unsigned constructed block admitted by receive path");
 size_t construction_calls=0;check(build(block,[&]{++construction_calls;}).has_value(),"counted construction failed");
 std::set<size_t> construction_stops{1,construction_calls/2,construction_calls};
 for(auto stop:construction_stops){size_t n=0;bool cancelled=false;try{build(block,[&]{if(++n==stop)throw std::runtime_error("construction cancelled");});}catch(const std::runtime_error& e){cancelled=std::string(e.what())=="construction cancelled";}check(cancelled&&input==before,"construction cancellation changed parent");}
 unsigned config_rejections=0;auto refuse=[&](const AltairTransitionRules& cfg,uint64_t budget){bool threw=false;try{prepare_altair_state_transition(input,block,cfg,budget,100000);}catch(const std::exception&){threw=true;}check(threw&&input==before,"invalid rules/budget accepted");++config_rejections;};refuse(r,0);auto cfg=r;cfg.sync.proposer_weight++;refuse(cfg,4096);cfg=r;cfg.attestation.base_reward_factor++;refuse(cfg,4096);cfg=r;cfg.deposit.validator_registry_limit++;refuse(cfg,4096);cfg=r;cfg.epoch.registry.minimum_churn_limit++;refuse(cfg,4096);
 // Verify the wrong-root fixture's signature over the advertised root. It must
 // not be rejected merely because that root was changed without re-signing.
 const auto wrong=binary(root/"invalid_incorrect_state_root/blocks_0.ssz");const auto pre=binary(root/"invalid_incorrect_state_root/pre.ssz");const auto advanced=prepare_altair_slots({pre,std::nullopt},r.epoch,r.state,little(wrong,100,8),4096,100000);const auto& s=advanced.ssz;
 const ConsensusBeaconHeader h{little(wrong,100,8),little(wrong,108,8),root_at(wrong,116),root_at(wrong,148),altair_beacon_body_root(std::span<const uint8_t>(wrong).subspan(184),r.body,r.state.sync_committee_size)};ConsensusBytes4 version{};std::copy_n(s.begin()+(h.slot/r.attestation.duties.slots_per_epoch<little(s,56,8)?48:52),4,version.begin());const auto validators=little(s,4360,4);const Bytes key(s.begin()+validators+121*h.proposer_index,s.begin()+validators+121*h.proposer_index+48);check(verify_consensus_beacon_header(h,version,root_at(s,8),key,std::span<const uint8_t>(wrong).subspan(4,96)),"wrong-root fixture signature invalid");check(!prepare_altair_state_transition({pre,std::nullopt},wrong,r,4096,100000),"signed wrong root accepted");
 std::cout<<Json{{"scenario","official-altair-full-blocks"},{"valid",valid},{"invalid",invalid},{"acceptedBlocks",accepted},{"constructedExactBlocks",constructed},{"officialInvalidConstructionRejected",invalid_construction},{"envelopeOnlyReconstructed",reconstructed_envelopes},{"constructionRejections",construction_rejections},{"constructionCancellations",construction_stops.size()},{"serializedBytesCompared",bytes_compared},{"mutations",mutations},{"configurationRejections",config_rejections},{"cancellations",stops.size()},{"stateRootsVerified",true},{"networkActive",false}}.dump()<<'\n';
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
