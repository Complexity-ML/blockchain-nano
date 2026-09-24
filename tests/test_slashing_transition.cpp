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
 unsigned checked=0;
 auto apply=[&](auto& state){return process_phase0_attester_slashing(state,rules,first,committee,second,committee,10,30);};
 auto state=initial;auto result=apply(state);
 check(result&&*result==std::vector<uint64_t>{20},"eligible signer");
 check(state.validators.at(20).slashed&&state.validators.at(20).balance==31000,"penalty");
 check(state.validators.at(20).exit_epoch==5&&state.validators.at(20).withdrawable_epoch==11,"exit and withdrawal delay");
 check(state.validators.at(10).balance==32007&&state.validators.at(30).balance==32055,"reward split");
 check(state.slashings[3]==32000,"slashing bucket");++checked;
 auto already=state;check(!apply(state)&&state==already,"replay changed state");++checked;
 auto rejected=[&](ConsensusSlashingState candidate){auto before=candidate;check(!apply(candidate)&&candidate==before,"ineligible proof changed state");++checked;};
 state=initial;state.validators.at(20).activation_epoch=4;rejected(state);
 state=initial;state.validators.at(20).exit_epoch=2;state.validators.at(20).withdrawable_epoch=3;rejected(state);
 state=initial;state.validators.at(20).slashed=true;rejected(state);
 state=initial;state.validators.erase(20);rejected(state);
 state=initial;state.validators.at(20).public_key[0]^=1;rejected(state);
 state=initial;state.validators.at(20).activation_epoch=3;check(apply(state).has_value(),"activation boundary");++checked;
 state=initial;state.validators.at(20).exit_epoch=2;state.validators.at(20).withdrawable_epoch=4;check(apply(state).has_value()&&state.validators.at(20).exit_epoch==2&&state.validators.at(20).withdrawable_epoch==11,"exited validator still slashable until withdrawal");++checked;
 state=initial;state.validators.at(20).exit_epoch=2;state.validators.at(20).withdrawable_epoch=20;check(apply(state).has_value()&&state.validators.at(20).withdrawable_epoch==20,"never shorten withdrawal lock");++checked;
 state=initial;state.validators.at(20).balance=1;check(apply(state).has_value()&&state.validators.at(20).balance==0,"saturating penalty");++checked;
 state=initial;auto invalid=second;invalid.signature[0]^=1;
 check(!process_phase0_attester_slashing(state,rules,first,committee,invalid,committee,10,30)&&state==initial,"invalid signature mutated registry");++checked;
 auto all1=signed_vote(vote,{10,20,30},domain),all2=signed_vote(other,{10,20,30},domain);
 auto all=[&](auto& st){return process_phase0_attester_slashing(st,rules,all1,committee,all2,committee,10,10);};
 state=initial;result=all(state);check(result&&result->size()==3,"all slashable");
 check(state.validators.at(10).exit_epoch==5&&state.validators.at(20).exit_epoch==6&&state.validators.at(30).exit_epoch==7,"churn queue");
 check(state.validators.at(10).balance==31186&&state.validators.at(20).balance==31000&&state.validators.at(30).balance==31000&&state.slashings[3]==96000,"self rewards and accounting");++checked;
 state=initial;state.validators.at(20).slashed=true;result=all(state);check(result&&*result==std::vector<uint64_t>({10,30})&&state.validators.at(20).balance==32000&&state.validators.at(20).exit_epoch==UINT64_MAX,"mixed eligible set");++checked;
 state=initial;state.validators.at(30).exit_epoch=9;state.validators.at(30).withdrawable_epoch=12;
 check(apply(state).has_value()&&state.validators.at(20).exit_epoch==10&&state.validators.at(20).withdrawable_epoch==12,"existing full exit queue");++checked;
 state=initial;state.epoch=11;check(apply(state).has_value()&&state.slashings[3]==32000&&state.validators.at(20).withdrawable_epoch==19,"ring bucket and delay across epochs");++checked;
 auto wider=rules;wider.churn_limit_quotient=1;state=initial;
 result=process_phase0_attester_slashing(state,wider,all1,committee,all2,committee,10,30);
 check(result&&result->size()==3&&state.validators.at(10).exit_epoch==5&&state.validators.at(20).exit_epoch==5&&state.validators.at(30).exit_epoch==5,"active count changes churn");++checked;
 auto economics=rules;economics.minimum_penalty_quotient=64;economics.whistleblower_reward_quotient=100;economics.proposer_reward_quotient=4;state=initial;
 result=process_phase0_attester_slashing(state,economics,first,committee,second,committee,10,30);
 check(result&&state.validators.at(20).balance==31500&&state.validators.at(10).balance==32080&&state.validators.at(30).balance==32240,"configured economics");++checked;
 auto fails_atomically=[&](ConsensusSlashingState candidate,const auto& call){auto before=candidate;bool threw=false;try{call(candidate);}catch(const std::exception&){threw=true;}check(threw&&candidate==before,"failure was not atomic");++checked;};
 state=initial;state.slashings[3]=UINT64_MAX-32000;fails_atomically(state,all);
 state=initial;state.validators.at(30).balance=UINT64_MAX;fails_atomically(state,apply);
 state=initial;state.epoch=UINT64_MAX-1;fails_atomically(state,apply);
 state=initial;state.slashings.pop_back();fails_atomically(state,apply);
 state=initial;state.validators.at(30).public_key=state.validators.at(10).public_key;fails_atomically(state,apply);
 state=initial;state.validators.at(20).exit_epoch=1;state.validators.at(20).withdrawable_epoch=0;fails_atomically(state,apply);
 state=initial;state.validators.erase(30);fails_atomically(state,apply);
 auto badrules=rules;badrules.minimum_penalty_quotient=0;
 fails_atomically(initial,[&](auto& st){return process_phase0_attester_slashing(st,badrules,first,committee,second,committee,10,30);});
 std::cout<<Json{{"scenario","phase0-attester-slashing-transition"},{"cases",checked},{"inMemoryTransition",true},{"persistent",false},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
