#include "support/process.hpp"
#include "nano/consensus_committee.hpp"
#include "nano/consensus_attestation.hpp"
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
 ConsensusCommittee committee(members,version,domain);std::vector<uint64_t> all{10,20,30},pair{10,30};
 std::vector<Bytes> signatures;for(const auto& key:secrets)signatures.push_back(sign(key,consensus_signing_root(object,domain)));
 auto full=aggregate(signatures);auto result=committee.verify(all,object,full);check(result.has_value(),"valid aggregate");
 const Int total=Int(std::to_string(UINT64_MAX))+20;check(result->signed_weight==total&&result->total_weight==total&&result->signers==3,"weight does not overflow uint64");
 auto partial=aggregate({signatures[0],signatures[2]});result=committee.verify(pair,object,partial);check(result&&result->signed_weight==total-10&&result->total_weight==total,"exact partial weight");
 unsigned rejected=0;auto refuse=[&](const std::vector<uint64_t>& indices,const ConsensusRoot& root,const Bytes& sig){check(!committee.verify(indices,root,sig),"bad vote accepted");++rejected;};
 refuse({},object,full);refuse({10,10,30},object,full);refuse({30,20,10},object,full);refuse({10,20,99},object,full);refuse(pair,object,full);
 auto changed=object;changed[0]^=1;refuse(all,changed,full);auto bad=full;bad[0]^=1;refuse(all,object,bad);
 for(size_t n:{0u,95u,97u}){bad=full;bad.resize(n);refuse(all,object,bad);}
 for(unsigned field=0;field<7;++field){auto altered=vote;switch(field){case 0:altered.slot^=1;break;case 1:altered.index^=1;break;case 2:altered.beacon_block_root[0]^=1;break;case 3:altered.source.epoch^=1;break;case 4:altered.source.root[0]^=1;break;case 5:altered.target.epoch^=1;break;case 6:altered.target.root[0]^=1;break;}refuse(all,consensus_attestation_root(altered),full);}
 auto invalidRoster=[&](std::vector<ConsensusCommitteeMember> roster,ConsensusBytes4 fork){bool failed=false;try{ConsensusCommittee c(roster,fork,domain);}catch(const std::invalid_argument&){failed=true;}check(failed,"invalid roster accepted");++rejected;};
 invalidRoster({},version);auto roster=members;roster[1].validator_index=10;invalidRoster(roster,version);
 roster=members;roster[1].possession_message=roster[0].possession_message;roster[1].possession_signature=roster[0].possession_signature;invalidRoster(roster,version);
 roster=members;roster[0].effective_weight=0;invalidRoster(roster,version);
 roster=members;roster[0].possession_signature.clear();invalidRoster(roster,version);
 roster=members;roster[0].possession_message.amount_gwei++;invalidRoster(roster,version);
 auto fork=version;fork[0]^=1;invalidRoster(members,fork);
 for(unsigned kind=0;kind<3;++kind){auto altered=domain;altered[kind*8]^=1;ConsensusCommittee other(members,version,altered);check(!other.verify(all,object,full),"wrong domain accepted");++rejected;}
 // The snapshot must own its keys and weights; later source mutation cannot alter authority.
 members[0].effective_weight=999;members[0].possession_message.public_key.fill(0);
 result=committee.verify(all,object,full);check(result&&result->signed_weight==total,"snapshot aliases caller memory");
 std::cout<<Json{{"scenario","consensus-committee-participation"},{"validAggregates",3},{"rejected",rejected},{"wideWeight",true},{"snapshotOwned",true},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
