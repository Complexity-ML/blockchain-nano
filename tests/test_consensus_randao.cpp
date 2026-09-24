#include "support/process.hpp"
#include "nano/consensus_randao.hpp"
#include "nano/committee.hpp"
#include "blst.h"
using namespace test;
namespace {
Bytes sign(const blst_scalar& sk,uint64_t epoch,const ConsensusBytes4& version,const ConsensusRoot& genesis){
 ConsensusRoot object{};for(size_t i=0;i<8;++i)object[i]=uint8_t(epoch>>(8*i));
 auto root=consensus_signing_root(object,consensus_domain({2,0,0,0},version,genesis));
 static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";
 blst_p2 h,s;blst_hash_to_g2(&h,root.data(),root.size(),reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);
 blst_sign_pk_in_g1(&s,&h,&sk);Bytes out(96);blst_p2_compress(out.data(),&s);return out;
}
std::string encoded(const ConsensusRoot& r){return hex(Bytes(r.begin(),r.end())).substr(2);}
}
int main(){try{
 ConsensusRandaoRules rules{8,1};ConsensusRandaoState initial;initial.epoch=2;initial.mixes.resize(8);
 for(size_t i=0;i<8;++i)initial.mixes[i].fill(uint8_t(i));
 std::array<uint8_t,32> seed{};seed[0]=9;blst_scalar secret;blst_keygen(&secret,seed.data(),seed.size(),nullptr,0);
 blst_p1 pk;blst_sk_to_pk_in_g1(&pk,&secret);Bytes key(48);blst_p1_compress(key.data(),&pk);
 ConsensusBytes4 version{1,0,0,0};ConsensusRoot genesis{};genesis[0]=9;
 auto reveal=sign(secret,2,version,genesis);
 unsigned checked=0,rejected=0;
 auto state=initial;check(process_consensus_randao(state,rules,16,version,genesis,key,reveal),"valid reveal");
 auto digest=sha256(reveal);auto expected=initial;for(size_t i=0;i<32;++i)expected.mixes[2][i]^=digest[i];expected.last_reveal_slot=16;
 check(state==expected,"authenticated XOR mix and watermark");++checked;
 auto refuse=[&](ConsensusRandaoState candidate,uint64_t slot,const auto& v,const auto& g,const auto& k,const auto& sig){auto saved=candidate;check(!process_consensus_randao(candidate,rules,slot,v,g,k,sig)&&candidate==saved,"invalid reveal changed state");++rejected;};
 refuse(state,16,version,genesis,key,reveal);refuse(state,15,version,genesis,key,reveal);refuse(state,24,version,genesis,key,reveal);
 auto altered=reveal;altered[0]^=1;refuse(initial,16,version,genesis,key,altered);
 refuse(initial,16,version,genesis,key,Bytes{});
 auto fork=version;fork[0]^=1;refuse(initial,16,fork,genesis,key,reveal);
 auto network=genesis;network[0]^=1;refuse(initial,16,version,network,key,reveal);
 auto wrongkey=key;wrongkey[0]^=1;refuse(initial,16,version,genesis,wrongkey,reveal);
 refuse(initial,16,version,genesis,key,sign(secret,1,version,genesis));
 check(consensus_randao_seed(state,rules,2,{1,0,0,0})==consensus_randao_seed(initial,rules,2,{1,0,0,0})&&consensus_randao_seed(state,rules,3,{1,0,0,0})==consensus_randao_seed(initial,rules,3,{1,0,0,0}),"lookahead isolates current and next duties");++checked;
 auto repeated=state;check(process_consensus_randao(repeated,rules,17,version,genesis,key,reveal)&&repeated.mixes==initial.mixes,"same epoch signature allowed at another authenticated proposer slot");++checked;
 // Independent SHA-256 reference values from domain || uint64_le(epoch) || mix.
 for(const auto& [epoch,want]:std::vector<std::pair<uint64_t,std::string>>{{0,"6be99d142de57d9d192e212d11cb44025f1e1d02221511f232a25a71672d9dc6"},{2,"6bf352485ca053169f561fc3e81a8e20ecf93808a6ebed6e2640b3a2234adfe8"},{3,"8bdfc229e84420610b2314e48a77b7e8502de704a21b4e0be0a68bcf3fdee75e"},{UINT64_MAX,"dd44ec68ce29e24356143ca7d3968d27bcecee6bf20cdb0bc8503fce4be09c6b"}}){auto sample=initial;sample.epoch=epoch;check(encoded(consensus_randao_seed(sample,rules,epoch,{1,0,0,0}))==want,"reference seed mismatch");++checked;}
 auto next_seed=consensus_randao_seed(initial,rules,3,{1,0,0,0});auto next=initial;next.epoch=3;check(consensus_randao_seed(next,rules,3,{1,0,0,0})==next_seed,"next duty seed consistency");++checked;
 check(consensus_randao_seed(initial,rules,2,{0,0,0,0})!=consensus_randao_seed(initial,rules,2,{1,0,0,0}),"duty domain separation");++checked;
 advance_consensus_randao_epoch(state,rules,2);check(state.epoch==3&&state.mixes[3]==expected.mixes[2]&&state.last_reveal_slot==16,"epoch rotation");++checked;
 refuse(state,24,version,genesis,key,reveal);
 check(process_consensus_randao(state,rules,24,version,genesis,key,sign(secret,3,version,genesis)),"next epoch reveal");++checked;
 advance_consensus_randao_epoch(state,rules,3);
 auto committee_seed=consensus_randao_seed(state,rules,4,{1,0,0,0});
 check(committee_seed!=consensus_randao_seed(ConsensusRandaoState{4,initial.mixes,{}},rules,4,{1,0,0,0}),"old authenticated reveal influences delayed duties");++checked;
 auto order=shuffled_validator_indices(64,committee_seed,10);auto sorted=order;std::sort(sorted.begin(),sorted.end());for(size_t i=0;i<64;++i)check(sorted[i]==i,"seeded committee permutation");++checked;
 auto throws_unchanged=[&](ConsensusRandaoState candidate,const auto& action){auto saved=candidate;bool failed=false;try{action(candidate);}catch(const std::exception&){failed=true;}check(failed&&candidate==saved,"invalid transition mutated RANDAO");++rejected;};
 throws_unchanged(state,[&](auto& s){advance_consensus_randao_epoch(s,rules,3);});
 throws_unchanged(state,[&](auto& s){advance_consensus_randao_epoch(s,rules,5);});
 throws_unchanged(state,[&](auto& s){consensus_randao_seed(s,rules,2,{1,0,0,0});});
 throws_unchanged(state,[&](auto& s){consensus_randao_seed(s,rules,6,{1,0,0,0});});
 auto bad=initial;bad.epoch=UINT64_MAX;throws_unchanged(bad,[&](auto& s){advance_consensus_randao_epoch(s,rules,UINT64_MAX);});
 bad=initial;bad.mixes.clear();throws_unchanged(bad,[&](auto& s){process_consensus_randao(s,rules,16,version,genesis,key,reveal);});
 bad=initial;bad.last_reveal_slot=24;throws_unchanged(bad,[&](auto& s){process_consensus_randao(s,rules,16,version,genesis,key,reveal);});
 throws_unchanged(initial,[&](auto& s){process_consensus_randao(s,{0,1},16,version,genesis,key,reveal);});
 throws_unchanged(initial,[&](auto& s){consensus_randao_seed(s,{8,7},2,{1,0,0,0});});
 std::cout<<Json{{"scenario","authenticated-randao"},{"checks",checked},{"rejected",rejected},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
