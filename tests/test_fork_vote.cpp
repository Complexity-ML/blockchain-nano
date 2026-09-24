#include "support/process.hpp"
#include "nano/consensus_fork_vote.hpp"
#include "nano/committee.hpp"
#include "blst.h"
using namespace test;
ConsensusRoot root(uint8_t n){ConsensusRoot r{};r[0]=n;return r;}
Bytes sign(const blst_scalar& key,const ConsensusRoot& r){static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";blst_p2 h,s;blst_hash_to_g2(&h,r.data(),r.size(),reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);blst_sign_pk_in_g1(&s,&h,&key);Bytes b(96);blst_p2_compress(b.data(),&s);return b;}
int main(){try{
 const Phase0FfgRules rules{8,1,4,4,1000000000,32000000000ULL,10};
 Phase0ForkVoteContext ctx{};ctx.checkpoint={2,root(2)};ctx.registry.epoch=2;ctx.randao.epoch=2;ctx.randao.mixes.resize(64);ctx.fork={{1,0,0,0},{2,0,0,0},2};ctx.genesis_root=root(99);
 std::array<blst_scalar,64> secrets;
 for(size_t i=0;i<64;++i){std::array<uint8_t,32> seed{};seed[0]=uint8_t(i+1);blst_keygen(&secrets[i],seed.data(),seed.size(),nullptr,0);blst_p1 pub;blst_sk_to_pk_in_g1(&pub,&secrets[i]);ConsensusSlashingValidator x;blst_p1_compress(x.public_key.data(),&pub);x.activation_epoch=0;x.effective_balance=x.balance=32000000000ULL;ctx.registry.validators[i]=x;}
 Phase0ForkChoiceView view{};view.anchor=root(1);view.current_epoch=2;view.justified={0,root(1)};view.finalized=view.justified;view.nodes[root(1)]={0,{},0,0};view.nodes[root(2)]={16,root(1),0,0};view.nodes[root(3)]={17,root(2),0,0};view.nodes[root(4)]={17,root(2),0,0};
 auto make=[&](uint64_t slot,ConsensusRoot head){Phase0SignedAttestation a;a.data={slot,0,head,{0,root(1)},ctx.checkpoint};const auto duties=phase0_attestation_duties(ctx.registry,ctx.randao,rules,ctx.checkpoint.epoch);const auto c=partition_committee(duties.shuffled,(slot%8)*duties.committees_per_slot,duties.committees_per_slot*8);check(!c.empty(),"test committee");a.aggregation_bits.resize(c.size());a.aggregation_bits[0]=true;const auto version=ctx.checkpoint.epoch<ctx.fork.epoch?ctx.fork.previous_version:ctx.fork.current_version;a.signature=sign(secrets[c[0]],consensus_signing_root(consensus_attestation_root(a.data),consensus_domain({1,0,0,0},version,ctx.genesis_root)));return std::pair{a,c[0]};};
 view.justified_registry=ctx.registry;view.justified_registry.epoch=0;
 check(phase0_fork_choice_head(view,{8,1000000000,40}).head==root(4),"head before vote");
 auto [a,index]=make(17,root(3));const auto initial=view;
 check(process_phase0_fork_vote(view,18,ctx,rules,a),"signed admission");check(view.latest_messages.size()==1&&view.latest_messages.at(index).root==root(3),"committee signer derived");const auto saved=view.latest_messages;
 check(phase0_fork_choice_head(view,{8,1000000000,40}).head==root(3),"authenticated vote changes head");
 check(process_phase0_fork_vote(view,18,ctx,rules,a)&&view.latest_messages==saved,"duplicate idempotent");auto [conflict,other]=make(17,root(4));check(other==index&&process_phase0_fork_vote(view,18,ctx,rules,conflict)&&view.latest_messages==saved,"same epoch first message retained");
 auto newer=view;newer.latest_messages[index]={3,root(4)};const auto newest=newer.latest_messages;
 check(process_phase0_fork_vote(newer,18,ctx,rules,a)&&newer.latest_messages==newest,"older vote cannot overwrite newer message");
 unsigned rejected=0;auto refuse=[&](const Phase0SignedAttestation& bad,uint64_t slot){const auto before=view.latest_messages;check(!process_phase0_fork_vote(view,slot,ctx,rules,bad)&&view.latest_messages==before,"bad vote changed messages");++rejected;};
 refuse(a,17);auto bad=a;bad.signature[0]^=1;refuse(bad,18);bad=a;bad.data.target.root=root(1);refuse(bad,18);bad=a;bad.data.beacon_block_root=root(50);refuse(bad,18);bad=a;bad.data.slot=16;refuse(bad,18);bad=a;bad.aggregation_bits.push_back(false);refuse(bad,18);bad=a;bad.aggregation_bits.assign(a.aggregation_bits.size(),false);refuse(bad,18);bad=a;bad.data.source.root=root(7);refuse(bad,18);
 const auto g=ctx.genesis_root;ctx.genesis_root=root(98);refuse(a,18);ctx.genesis_root=g;
 // A known root on another branch cannot be an FFG target of this LMD head.
 const auto before_nodes=view.nodes;view.nodes[root(3)].parent=root(1);refuse(a,18);view.nodes=before_nodes;
 auto eq=initial;eq.equivocating_indices.insert(index);check(process_phase0_fork_vote(eq,18,ctx,rules,a)&&eq.latest_messages.empty(),"equivocator not updated");
 view.current_epoch=5;refuse(a,40);check(process_phase0_fork_vote(view,40,ctx,rules,a,true)&&view.latest_messages==saved,"verified-block older epoch allowed");view.current_epoch=2;
 size_t total=0;auto control=initial;check(process_phase0_fork_vote(control,18,ctx,rules,a,false,[&]{++total;}),"cancel control");
 for(size_t stop=1;stop<=total;++stop){auto copy=initial;size_t calls=0;bool cancelled=false;try{process_phase0_fork_vote(copy,18,ctx,rules,a,false,[&]{if(++calls==stop)throw std::runtime_error("cancel");});}catch(const std::runtime_error& e){cancelled=std::string(e.what())=="cancel";}check(cancelled&&copy.latest_messages.empty(),"cancellation atomicity");}
 ctx.checkpoint={3,root(5)};ctx.registry.epoch=ctx.randao.epoch=3;view.current_epoch=3;view.nodes[root(5)]={24,root(3),0,0};auto [new_vote,new_index]=make(24,root(5));view.latest_messages[new_index]={2,root(3)};check(process_phase0_fork_vote(view,25,ctx,rules,new_vote)&&view.latest_messages.at(new_index)==Phase0LatestMessage{3,root(5)},"new epoch replaces older message");
 std::cout<<Json{{"scenario","phase0-fork-vote-local"},{"rejections",rejected},{"cancellations",total},{"officialEventReplay",false},{"activeInConsensus",false}}.dump()<<'\n';
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
