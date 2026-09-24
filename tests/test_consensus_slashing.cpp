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
 auto proof=verify_attester_slashing(first,committee,second,committee);
 check(proof&&*proof==std::vector<uint64_t>{20},"exact double-voter intersection");unsigned valid=1,rejected=0;
 auto refuse=[&](const auto& a,const auto& ca,const auto& b,const auto& cb){check(!verify_attester_slashing(a,ca,b,cb),"invalid slashing proof accepted");++rejected;};
 refuse(first,committee,first,committee);
 auto bad=second;bad.signature[0]^=1;refuse(first,committee,bad,committee);
 bad=first;bad.signature.clear();refuse(bad,committee,second,committee);
 bad=second;bad.indices={20,20};refuse(first,committee,bad,committee);
 bad=second;bad.indices={30,20};refuse(first,committee,bad,committee);
 bad=second;bad.indices={20,99};refuse(first,committee,bad,committee);
 auto disjoint=signed_vote(other,{30},domain);refuse(first,committee,disjoint,committee);
 auto altered=domain;altered[0]^=1;ConsensusCommittee wrong(members,version,altered);refuse(first,committee,second,wrong);
 auto mismatched=members;std::swap(mismatched[0].possession_message,mismatched[1].possession_message);std::swap(mismatched[0].possession_signature,mismatched[1].possession_signature);
 ConsensusCommittee remapped(mismatched,version,domain);
 auto remapped_vote=second;remapped_vote.signature=aggregate({sign(secrets[0],consensus_signing_root(consensus_attestation_root(other),domain)),sign(secrets[2],consensus_signing_root(consensus_attestation_root(other),domain))});
 check(remapped.verify(remapped_vote.indices,consensus_attestation_root(other),remapped_vote.signature).has_value(),"positive control for remapped keys");refuse(first,committee,remapped_vote,remapped);
 auto outer=vote;outer.source.epoch=0;outer.target.epoch=4;auto inner=vote;inner.source.epoch=1;inner.target.epoch=3;
 auto a=signed_vote(outer,{10,20},domain),b=signed_vote(inner,{20,30},domain);proof=verify_attester_slashing(a,committee,b,committee);check(proof&&*proof==std::vector<uint64_t>{20},"surround signer");++valid;refuse(b,committee,a,committee);
 auto historical=signed_vote(inner,{20,30},altered);proof=verify_attester_slashing(a,committee,historical,wrong);check(proof&&*proof==std::vector<uint64_t>{20},"distinct historical domains");++valid;
 std::cout<<Json{{"scenario","authenticated-attester-slashing"},{"validProofs",valid},{"rejected",rejected},{"penaltiesApplied",false},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
