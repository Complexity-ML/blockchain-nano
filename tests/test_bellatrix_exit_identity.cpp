#include "nano/consensus_bellatrix_reactor.hpp"
#include "nano/consensus_message_ssz.hpp"
#include "nano/consensus_bls.hpp"
#include "blst.h"
#include <iostream>
using namespace nano;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
BellatrixSignedExitRecord record(uint64_t secret_word,uint64_t epoch){
 uint64_t words[4]{secret_word,0,0,0};blst_scalar secret;blst_scalar_from_uint64(&secret,words);blst_p1 pk;blst_sk_to_pk_in_g1(&pk,&secret);std::array<uint8_t,48> key{};blst_p1_compress(key.data(),&pk);
 const auto root=consensus_signing_root(consensus_voluntary_exit_root(epoch,64),consensus_domain({4,0,0,0},{},{}));static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";blst_p2 hash,sig;blst_hash_to_g2(&hash,root.data(),root.size(),reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);blst_sign_pk_in_g1(&sig,&hash,&secret);Bytes signature(96);blst_p2_compress(signature.data(),&sig);check(verify_consensus_bls(key,root,signature),"test exit signature invalid");return {{},encode_consensus_voluntary_exit_ssz({epoch,64,signature}),key};
}
}
int main(){try{
 // Counterexample to index-only identity: both signatures are valid for their
 // respective keys, same index and epoch; they must coexist without dedup.
 const auto a=record(65,8),b=record(66,8),later_a=record(65,9);
 check(a.identity()!=b.identity()&&a.identity()==later_a.identity(),"exit identity aliases another BLS key");
 std::set<BellatrixExitIdentity> identities;check(identities.insert(a.identity()).second&&identities.insert(b.identity()).second&&!identities.insert(later_a.identity()).second,"exit restore identity uniqueness");
 ConsensusSlashingState registry{};check(bellatrix_exit_membership(a,registry)==BellatrixExitMembership::Unknown,"absent index lost pending request");
 ConsensusSlashingValidator v;v.public_key=b.public_key;registry.validators.emplace(64,v);
 check(bellatrix_exit_membership(a,registry)==BellatrixExitMembership::DifferentKey&&bellatrix_exit_membership(b,registry)==BellatrixExitMembership::Matching,"wrong-key request selected on new registry");
 registry.validators.at(64).exit_epoch=10;
 check(bellatrix_exit_membership(a,registry)==BellatrixExitMembership::DifferentKey&&bellatrix_exit_membership(b,registry)==BellatrixExitMembership::Scheduled,"other key exit mistaken for served request");
 registry.validators.at(64).public_key=a.public_key;registry.validators.at(64).exit_epoch=UINT64_MAX;
 check(bellatrix_exit_membership(a,registry)==BellatrixExitMembership::Matching&&bellatrix_exit_membership(b,registry)==BellatrixExitMembership::DifferentKey,"branch switch reused stale identity");
 auto short_record=a;short_record.ssz.pop_back();bool refused=false;try{short_record.identity();}catch(const std::invalid_argument&){refused=true;}check(refused,"short archive entry accepted");
 std::cout<<"BLS exit identities, index reuse and registry membership passed; synthetic contexts, not conflicting finalized histories\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
