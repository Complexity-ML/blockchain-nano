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
 // Independent slashing event: economic state is deliberately untouched.
 auto slashed_view=initial;slashed_view.justified={2,root(2)};
 Phase0ForkSlashingContext sc{root(2),ctx.registry,ctx.fork,ctx.genesis_root};sc.registry.epoch=2;
 slashed_view.justified_registry=sc.registry;slashed_view.latest_messages[index]={2,root(3)};
 const auto registry_before=sc.registry;
 IndexedConsensusAttestation first{a.data,{index},a.signature},second{conflict.data,{index},conflict.signature};
 check(phase0_fork_choice_head(slashed_view,{8,1000000000,40}).head==root(3),"pre-evidence weight");
 const auto unmarked=slashed_view;
 check(process_phase0_fork_slashing(slashed_view,sc,first,second,2048),"double vote evidence");
 check(slashed_view.equivocating_indices==std::set<uint64_t>{index}&&sc.registry==registry_before,"only equivocator set updated");
 check(phase0_fork_choice_head(slashed_view,{8,1000000000,40}).head==root(4),"evidence removes weight");
 check(process_phase0_fork_slashing(slashed_view,sc,first,second,2048)&&slashed_view.equivocating_indices.size()==1,"repeat evidence idempotent");
 unsigned proof_rejections=0;
 auto reject_proof=[&](const IndexedConsensusAttestation& f,const IndexedConsensusAttestation& s){auto copy=unmarked;check(!process_phase0_fork_slashing(copy,sc,f,s,2048)&&copy.equivocating_indices.empty(),"invalid evidence changed set");++proof_rejections;};
 reject_proof(first,first);auto corrupt=first;corrupt.signature[0]^=1;reject_proof(corrupt,second);corrupt=second;corrupt.signature[0]^=1;reject_proof(first,corrupt);
 corrupt=first;corrupt.indices.clear();reject_proof(corrupt,second);corrupt=first;corrupt.indices.push_back(index);reject_proof(corrupt,second);corrupt=first;corrupt.indices={999};reject_proof(corrupt,second);corrupt=first;corrupt.indices={2,1};reject_proof(corrupt,second);
 auto resign=[&](IndexedConsensusAttestation& vote){const auto version=vote.data.target.epoch<sc.fork.epoch?sc.fork.previous_version:sc.fork.current_version;vote.signature=sign(secrets[vote.indices[0]],consensus_signing_root(consensus_attestation_root(vote.data),consensus_domain({1,0,0,0},version,sc.genesis_root)));};
 auto disjoint=second;disjoint.indices={(index+1)%64};resign(disjoint);auto no_common=unmarked;check(process_phase0_fork_slashing(no_common,sc,first,disjoint,2048)&&no_common.equivocating_indices.empty(),"valid disjoint proof no effect");
 auto aggregate=[&](IndexedConsensusAttestation& vote){blst_p2 sum;bool first_key=true;for(auto i:vote.indices){auto single=vote;single.indices={i};resign(single);blst_p2_affine point;check(blst_p2_uncompress(&point,single.signature.data())==BLST_SUCCESS,"aggregate test signature");if(first_key){blst_p2_from_affine(&sum,&point);first_key=false;}else blst_p2_add_or_double_affine(&sum,&sum,&point);}vote.signature.resize(96);blst_p2_compress(vote.signature.data(),&sum);};
 auto group_first=first,group_second=second;group_first.indices={0,1};group_second.indices={1,2};aggregate(group_first);aggregate(group_second);auto partial=unmarked;check(process_phase0_fork_slashing(partial,sc,group_first,group_second,2048)&&partial.equivocating_indices==std::set<uint64_t>{1},"only aggregate intersection excluded");
 auto capped=unmarked;check(!process_phase0_fork_slashing(capped,sc,group_first,group_second,1)&&capped.equivocating_indices.empty(),"configured indexed list bound");
 auto outer=first,inner=second;outer.data.source.epoch=0;outer.data.target.epoch=3;inner.data.source.epoch=1;inner.data.target.epoch=2;resign(outer);resign(inner);auto surround=unmarked;check(process_phase0_fork_slashing(surround,sc,outer,inner,2048)&&surround.equivocating_indices.contains(index),"surround evidence");reject_proof(inner,outer);
 // Exercise previous/current fork domains across target epochs.
 auto old_first=first,old_second=second;old_first.data.target.epoch=old_second.data.target.epoch=1;resign(old_first);resign(old_second);auto old_proof=unmarked;check(process_phase0_fork_slashing(old_proof,sc,old_first,old_second,2048),"historical fork domain");
 size_t proof_calls=0;auto measured=unmarked;check(process_phase0_fork_slashing(measured,sc,first,second,2048,[&]{++proof_calls;}),"proof cancellation control");
 for(size_t stop=1;stop<=proof_calls;++stop){auto copy=unmarked;size_t calls=0;bool cancelled=false;try{process_phase0_fork_slashing(copy,sc,first,second,2048,[&]{if(++calls==stop)throw std::runtime_error("cancel");});}catch(const std::runtime_error& e){cancelled=std::string(e.what())=="cancel";}check(cancelled&&copy.equivocating_indices.empty(),"proof cancellation atomicity");}
 std::cout<<Json{{"scenario","phase0-fork-vote-local"},{"rejections",rejected},{"cancellations",total},{"proofRejections",proof_rejections},{"proofCancellations",proof_calls},{"officialEventReplay",false},{"activeInConsensus",false}}.dump()<<'\n';
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
