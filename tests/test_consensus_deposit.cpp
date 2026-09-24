#include "support/process.hpp"
#include "nano/consensus_deposit.hpp"
#include "blst.h"
using namespace test;
template<size_t N> std::array<uint8_t,N> fixed(const Json& value){auto bytes=unhex(value.get<std::string>());check(bytes.size()==N,"fixture width");std::array<uint8_t,N> out{};std::copy(bytes.begin(),bytes.end(),out.begin());return out;}
int main(){try{
 auto f=load(std::string(NANO_TEST_SOURCE)+"/tests/fixtures/consensus-deposit.json");unsigned official=0,valid=0,rejected=0;
 for(const auto& c:f.at("cases")){ConsensusDepositMessage d{fixed<48>(c.at("pubkey")),fixed<32>(c.at("withdrawal_credentials")),std::stoull(c.at("amount").get<std::string>())};check(consensus_deposit_root(d)==fixed<32>(c.at("root")),"official deposit SSZ root");++official;}
 std::array<uint8_t,32> seed{};seed[0]=1;blst_scalar sk;blst_keygen(&sk,seed.data(),seed.size(),nullptr,0);blst_p1 pk;blst_sk_to_pk_in_g1(&pk,&sk);
 ConsensusDepositMessage d;blst_p1_compress(d.public_key.data(),&pk);d.withdrawal_credentials[0]=1;ConsensusBytes4 version{0,0,0,1};
 auto sign=[&](const ConsensusRoot& root){static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";blst_p2 h,s;blst_hash_to_g2(&h,root.data(),root.size(),reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);blst_sign_pk_in_g1(&s,&h,&sk);Bytes out(96);blst_p2_compress(out.data(),&s);return out;};
 for(uint64_t amount:{uint64_t(0),uint64_t(32000000000),UINT64_MAX}){
  d.amount_gwei=amount;auto root=consensus_deposit_root(d);auto domain=consensus_domain({3,0,0,0},version,{});auto sig=sign(consensus_signing_root(root,domain));
  check(verify_consensus_deposit_signature(d,sig,version),"valid deposit signature");++valid;
  auto refuse=[&](const ConsensusDepositMessage& changed,const Bytes& signature,const ConsensusBytes4& fork){check(!verify_consensus_deposit_signature(changed,signature,fork),"altered deposit accepted");++rejected;};
  auto changed=d;changed.amount_gwei^=1;refuse(changed,sig,version);changed=d;changed.withdrawal_credentials[31]^=1;refuse(changed,sig,version);changed=d;changed.public_key[0]^=0x20;refuse(changed,sig,version);
  auto fork=version;fork[3]^=1;refuse(d,sig,fork);
  refuse(d,sign(consensus_signing_root(root,consensus_domain({1,0,0,0},version,{}))),version);
  for(size_t n:{0u,95u,97u}){auto bad=sig;bad.resize(n);refuse(d,bad,version);}
 }
 check(official==5,"complete selected official corpus");std::cout<<Json{{"scenario","consensus-deposit-signature"},{"officialRoots",official},{"signedCases",valid},{"mutationsRejected",rejected},{"fundingAuthenticated",false},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
