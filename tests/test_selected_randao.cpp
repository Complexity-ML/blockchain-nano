#include "support/process.hpp"
#include "nano/consensus_committee.hpp"
#include "nano/consensus_attestation.hpp"
#include "nano/consensus_slashing.hpp"
#include "blst.h"
#include "nano/consensus_proposer.hpp"
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

 ConsensusRandaoRules randao_rules{8,1};ConsensusRandaoState randao;randao.epoch=initial.epoch;randao.mixes.resize(8);
 std::vector<Phase0ProposerValidator> candidates;for(const auto& [index,v]:initial.validators)candidates.push_back({index,v.effective_balance,v.activation_epoch,v.exit_epoch});
 const auto selected=phase0_proposer_index(candidates,randao,randao_rules,24,32000,10);
 const auto outsider=selected==10?20:10;
 ConsensusRoot epoch{};epoch[0]=3;
 auto root=consensus_signing_root(epoch,consensus_domain({2,0,0,0},version,genesis));
 auto reveal=sign(secrets.at((selected-10)/10),root),other_reveal=sign(secrets.at((outsider-10)/10),root);
 auto apply=[&](auto& state,const auto& registry,uint64_t claim,const auto& sig){return process_phase0_selected_randao(state,registry,randao_rules,24,claim,32000,10,version,genesis,sig);};
 unsigned checks=0,rejected=0;
 auto positive_control=randao;
 check(process_consensus_randao(positive_control,randao_rules,24,version,genesis,initial.validators.at(outsider).public_key,other_reveal),"other validator has a genuinely valid epoch signature");++checks;
 auto refuse=[&](const auto& registry,uint64_t claim,const auto& sig){auto copy=randao;check(!apply(copy,registry,claim,sig)&&copy==randao,"unauthorized reveal mutated state");++rejected;};
 refuse(initial,outsider,other_reveal); // Valid signature, wrong duty.
 refuse(initial,selected,other_reveal);refuse(initial,999,reveal);refuse(initial,selected,Bytes{});
 auto bad=initial;bad.validators.at(selected).slashed=true;refuse(bad,selected,reveal);
 bad=initial;bad.validators.at(selected).activation_epoch=4;refuse(bad,selected,reveal);
 bad=initial;bad.validators.at(selected).exit_epoch=3;refuse(bad,selected,reveal);
 bad=initial;bad.validators.at(selected).public_key=initial.validators.at(outsider).public_key;refuse(bad,selected,reveal);
 auto copy=randao;auto pure=randao;check(process_consensus_randao(pure,randao_rules,24,version,genesis,initial.validators.at(selected).public_key,reveal),"low-level selected reveal");
 check(apply(copy,initial,selected,reveal)&&copy==pure,"joint gate differs from valid mix");++checks;
 auto saved=copy;check(!apply(copy,initial,selected,reveal)&&copy==saved,"joint gate slot replay");++rejected;
 auto fail_atomic=[&](const auto& action){auto candidate=randao;bool failed=false;try{action(candidate);}catch(const std::exception&){failed=true;}check(failed&&candidate==randao,"exception changed mix");++rejected;};
 bad=initial;bad.epoch=2;fail_atomic([&](auto& candidate){apply(candidate,bad,selected,reveal);});
 fail_atomic([&](auto& candidate){process_phase0_selected_randao(candidate,initial,randao_rules,24,selected,32000,10,version,genesis,reveal,[]{throw std::runtime_error("cancelled");});});
 // Slashing one selected proposer must reject that block, not silently reroll.
 bad=initial;bad.validators.at(selected).slashed=true;refuse(bad,outsider,other_reveal);
 check(initial.validators.at(selected).slashed==false&&randao.last_reveal_slot==std::nullopt,"input registry and baseline unchanged");++checks;
 std::cout<<Json{{"scenario","selected-proposer-randao"},{"checks",checks},{"rejected",rejected},{"validNonProposerSignatureRejected",true},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
