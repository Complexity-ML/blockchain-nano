#include "support/process.hpp"
#include "nano/consensus_committee.hpp"
#include "nano/consensus_attestation.hpp"
#include "nano/consensus_slashing.hpp"
#include "blst.h"
using namespace test;
namespace {
Bytes sign(const blst_scalar& sk,const ConsensusRoot& root){
 static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";
 blst_p2 h,s;blst_hash_to_g2(&h,root.data(),root.size(),reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);
 blst_sign_pk_in_g1(&s,&h,&sk);Bytes out(96);blst_p2_compress(out.data(),&s);return out;
}
Bytes aggregate(const std::vector<Bytes>& signatures){
 blst_p2 sum{};bool first=true;
 for(const auto& bytes:signatures){blst_p2_affine p;check(blst_p2_uncompress(&p,bytes.data())==BLST_SUCCESS,"decode test signature");if(first){blst_p2_from_affine(&sum,&p);first=false;}else blst_p2_add_or_double_affine(&sum,&sum,&p);}
 Bytes out(96);blst_p2_compress(out.data(),&sum);return out;
}
}
int main(){try{
 ConsensusBytes4 version{1,0,0,0};ConsensusRoot genesis{},object{};genesis[0]=7;ConsensusAttestationData vote;vote.slot=64;vote.index=1;vote.beacon_block_root[0]=42;vote.source.epoch=1;vote.target.epoch=2;object=consensus_attestation_root(vote);
 auto domain=consensus_domain({1,0,0,0},version,genesis);
 std::vector<ConsensusCommitteeMember> members;std::vector<blst_scalar> secrets(3);
 for(size_t i=0;i<3;++i){
  std::array<uint8_t,32> seed{};seed[0]=uint8_t(i+1);blst_keygen(&secrets[i],seed.data(),seed.size(),nullptr,0);
  blst_p1 pk;blst_sk_to_pk_in_g1(&pk,&secrets[i]);ConsensusCommitteeMember m;m.validator_index=10+i*10;m.effective_weight=i==2?UINT64_MAX:10;
  blst_p1_compress(m.possession_message.public_key.data(),&pk);m.possession_message.amount_gwei=32'000'000'000;
  auto root=consensus_signing_root(consensus_deposit_root(m.possession_message),consensus_domain({3,0,0,0},version,{}));m.possession_signature=sign(secrets[i],root);members.push_back(m);
 }
 ConsensusCommittee committee(members,version,domain);
 auto signed_vote=[&](ConsensusAttestationData data,std::vector<uint64_t> indices,const ConsensusRoot& signing_domain){
  std::vector<Bytes> signatures;for(auto index:indices)signatures.push_back(sign(secrets.at((index-10)/10),consensus_signing_root(consensus_attestation_root(data),signing_domain)));
  return IndexedConsensusAttestation{data,std::move(indices),aggregate(signatures)};
 };
 auto other=vote;other.beacon_block_root[0]^=1;

 auto first=signed_vote(vote,{10,20},domain),second=signed_vote(other,{20,30},domain);
 Phase0SlashingRules rules{1,65536,1,2,8,32,512,8,1000,1};
 ConsensusSlashingState initial;initial.epoch=3;initial.slashings.resize(8);
 for(const auto& member:members){ConsensusSlashingValidator v;v.public_key=member.possession_message.public_key;v.balance=v.effective_balance=32000;v.activation_epoch=0;initial.validators.emplace(member.validator_index,v);}

 unsigned checks=0,rejected=0;
 auto state=initial;
 check(process_phase0_attester_slashing(state,rules,first,committee,second,committee,10,30).has_value(),"signed sanction fixture");
 for(uint64_t epoch=3;epoch<7;++epoch){auto debits=advance_phase0_slashing_epoch(state,rules,epoch);check(debits.empty()&&state.validators.at(20).balance==31000,"premature correlation penalty");}
 auto due=state;auto debits=advance_phase0_slashing_epoch(state,rules,7);
 check(debits==std::map<uint64_t,uint64_t>{{20,16000}}&&state.validators.at(20).balance==15000&&state.epoch==8,"delayed penalty at midpoint");++checks;
 auto after=state;
 auto reject=[&](ConsensusSlashingState candidate,const Phase0SlashingRules& config,uint64_t epoch){auto saved=candidate;bool failed=false;try{advance_phase0_slashing_epoch(candidate,config,epoch);}catch(const std::exception&){failed=true;}check(failed&&candidate==saved,"epoch failure mutated state");++rejected;};
 reject(state,rules,7);reject(state,rules,9);
 auto restored=phase0_slashing_from_snapshot(phase0_slashing_snapshot(state,rules),rules);
 check(advance_phase0_slashing_epoch(restored,rules,8).empty()&&restored.validators.at(20).balance==15000,"reopened snapshot repeated penalty");++checks;
 for(uint64_t epoch=9;epoch<=11;++epoch)advance_phase0_slashing_epoch(restored,rules,epoch);
 check(restored.slashings==std::vector<uint64_t>(8)&&restored.validators.at(20).balance==15000,"expired bucket cleared without repeated debit");++checks;
 state=initial;auto& v=state.validators.at(20);v.slashed=true;v.exit_epoch=6;v.withdrawable_epoch=7;state.slashings[4]=32000;
 auto rounding=state;debits=advance_phase0_slashing_epoch(rounding,rules,3);
 check(debits.at(20)==10000&&rounding.validators.at(20).balance==22000&&rounding.slashings[4]==0,"division order and penalty before bucket reset");++checks;
 auto different=rules;different.proportional_slashing_multiplier=3;auto saturated=state;debits=advance_phase0_slashing_epoch(saturated,different,3);
 check(debits.at(20)==32000&&saturated.validators.at(20).balance==0,"correlation multiplier cap");++checks;
 auto low=state;low.validators.at(20).balance=1;debits=advance_phase0_slashing_epoch(low,rules,3);check(debits.at(20)==1&&low.validators.at(20).balance==0,"balance floor");++checks;
 auto empty=state;empty.slashings.assign(8,0);debits=advance_phase0_slashing_epoch(empty,rules,3);check(debits.at(20)==0&&empty.validators.at(20).balance==32000,"zero slashings");++checks;
 auto not_due=state;not_due.validators.at(20).withdrawable_epoch=8;check(advance_phase0_slashing_epoch(not_due,rules,3).empty()&&not_due.validators.at(20).balance==32000,"wrong midpoint");++checks;
 auto clean=state;clean.validators.at(20).slashed=false;check(advance_phase0_slashing_epoch(clean,rules,3).empty()&&clean.validators.at(20).balance==32000,"unslashed validator untouched");++checks;
 auto no_active=state;for(auto& [index,w]:no_active.validators)w.exit_epoch=2;debits=advance_phase0_slashing_epoch(no_active,rules,3);check(debits.at(20)==32000,"minimum denominator when no active stake");++checks;
 auto wide=state;for(auto& [index,w]:wide.validators){w.balance=UINT64_MAX;w.effective_balance=UINT64_MAX;}wide.slashings[0]=UINT64_MAX;wide.slashings[4]=UINT64_MAX;
 auto wide_rules=rules;wide_rules.effective_balance_increment=1;wide_rules.proportional_slashing_multiplier=UINT64_MAX;
 debits=advance_phase0_slashing_epoch(wide,wide_rules,3);check(debits.at(20)==UINT64_MAX&&wide.validators.at(20).balance==0,"wide aggregate arithmetic");++checks;
 auto invalid=state;invalid.epoch=UINT64_MAX;reject(invalid,rules,UINT64_MAX);
 invalid=state;invalid.slashings.pop_back();reject(invalid,rules,3);
 different=rules;different.effective_balance_increment=0;reject(state,different,3);
 different=rules;different.proportional_slashing_multiplier=0;reject(state,different,3);
 invalid=state;invalid.validators.at(20).public_key=invalid.validators.at(10).public_key;reject(invalid,rules,3);
 invalid=state;invalid.validators.at(20).exit_epoch=8;reject(invalid,rules,3);
 std::cout<<Json{{"scenario","phase0-slashing-epoch"},{"checks",checks},{"rejected",rejected},{"wideArithmetic",true},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
