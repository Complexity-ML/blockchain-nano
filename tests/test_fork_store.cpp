#include "support/phase0_transition_rules.hpp"
#include "nano/consensus_fork_store.hpp"
#include "nano/committee.hpp"
#include "blst.h"
using namespace test;
using namespace test::phase0_minimal;
Bytes sign(const blst_scalar& key,const ConsensusRoot& r){static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";blst_p2 h,s;blst_hash_to_g2(&h,r.data(),r.size(),reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);blst_sign_pk_in_g1(&s,&h,&key);Bytes b(96);blst_p2_compress(b.data(),&s);return b;}
Phase0ForkStore anchor(const Phase0SlotState& s,const Phase0TransitionRules& r){
 check(little(s.ssz,40,8)==0&&little(s.ssz,64,8)==0,"genesis fixture anchor");
 ConsensusBeaconHeader h{0,little(s.ssz,72,8),root_at(s.ssz,80),phase0_beacon_state_root(s.ssz,r.state),root_at(s.ssz,144)};const auto id=consensus_beacon_header_root(h);
 Phase0ForkStore out{};auto& v=out.timed.view;v.anchor=id;v.justified=v.finalized={0,id};v.current_epoch=0;v.justified_registry=decode(s.ssz);v.nodes[id]={0,h.parent_root,0,0};out.blocks[id]={h,s};out.timed.clock={0,0,{0,id},{0,id}};return out;
}
int main(){try{
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures",corpus=fixtures/"consensus-full-blocks-official";const auto transition=transition_rules(fixtures/"consensus-slashing-official");const Phase0ForkBlockRules rules{transition,12000,4000,40};const auto m=load(corpus/"manifest.json");unsigned valid=0,invalid=0,excluded=0,accepted=0;
 for(const auto& row:m.at("cases")){
  const auto dir=corpus/row.at("name").get<std::string>();const Phase0SlotState pre{binary(dir/"pre.ssz"),std::nullopt};if(little(pre.ssz,40,8)!=0){++excluded;continue;}
  for(const auto& [path,info]:row.at("files").items())check(hex(sha256(binary(corpus/path))).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");
  auto store=anchor(pre,transition);bool rejected=false;ConsensusRoot last=store.timed.view.anchor;
  for(size_t i=0;i<row.at("blockCount").get<size_t>();++i){auto bytes=binary(dir/("blocks_"+std::to_string(i)+".ssz"));const uint64_t arrival=std::max(store.timed.clock.time_ms,little(bytes,100,8)*12000);
   store=prepare_phase0_store_tick(store,arrival,rules,4096);
   const auto size=store.blocks.size();auto next=prepare_phase0_fork_block(store,bytes,rules,4096);check(store.blocks.size()==size,"source store mutated");if(!next){rejected=true;break;}
   const ConsensusBeaconHeader h{little(bytes,100,8),little(bytes,108,8),root_at(bytes,116),root_at(bytes,148),phase0_beacon_body_root(std::span<const uint8_t>(bytes).subspan(184),transition.body)};last=consensus_beacon_header_root(h);
   store=std::move(*next);check(store.blocks.size()==size+1&&store.timely.at(last),"block and timeliness published");auto duplicate=prepare_phase0_fork_block(store,bytes,rules,4096);check(duplicate&&duplicate->blocks.size()==store.blocks.size(),"duplicate block idempotent");++accepted;
  }
  if(row.at("valid").get<bool>()){check(!rejected,"valid genesis sequence rejected: "+row.at("name").get<std::string>());check(store.blocks.at(last).state.ssz==binary(dir/"post.ssz"),"official post-state mismatch");++valid;}else{check(rejected,"invalid sequence admitted");++invalid;}
 }
 check(m.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908"&&valid==15&&invalid==12&&excluded==18&&accepted==16,"genesis subset coverage");
 const Phase0SlotState pre{binary(corpus/"empty_block_transition/pre.ssz"),std::nullopt};const auto bytes=binary(corpus/"empty_block_transition/blocks_0.ssz");auto store=anchor(pre,transition);
 check(!prepare_phase0_fork_block(store,bytes,rules,32),"future block deferred");store.timed=prepare_phase0_fork_tick(store.timed,12000,12000,8);const auto original=store;
 auto timely=prepare_phase0_fork_block(store,bytes,rules,32);check(timely&&timely->timed.view.proposer_boost.has_value(),"timely first boost");auto late=store;late.timed.clock.time_ms+=4000;auto unboosted=prepare_phase0_fork_block(late,bytes,rules,32);check(unboosted&&!unboosted->timed.view.proposer_boost&& !unboosted->timely.rbegin()->second,"attestation threshold is exclusive");
 auto corrupt=bytes;corrupt[4]^=1;check(!prepare_phase0_fork_block(store,corrupt,rules,32)&&store.blocks.size()==1,"invalid signature atomic");corrupt=bytes;corrupt[116]^=1;check(!prepare_phase0_fork_block(store,corrupt,rules,32),"unknown parent rejected");
 size_t calls=0;check(bool(prepare_phase0_fork_block(store,bytes,rules,32,[&]{++calls;})),"cancel control");std::set<size_t> stops{1,2,calls/4,calls/2,3*calls/4,calls-1,calls};for(auto stop:stops){size_t n=0;bool cancelled=false;try{prepare_phase0_fork_block(store,bytes,rules,32,[&]{if(++n==stop)throw std::runtime_error("cancel");});}catch(const std::runtime_error& e){cancelled=std::string(e.what())=="cancel";}check(cancelled&&store.blocks.size()==1&&store.timed.view.nodes.size()==1&&!store.timed.view.proposer_boost,"cancel published candidate");}
 // Local trusted genesis with generated test keys, distinct from official vectors.
 auto local=pre;const auto n=decode(local.ssz).validators.size();std::vector<blst_scalar> secrets(n);const auto at=little(local.ssz,4360,4);
 for(size_t i=0;i<n;++i){std::array<uint8_t,32> seed{};seed[0]=uint8_t(i+1);blst_keygen(&secrets[i],seed.data(),seed.size(),nullptr,0);blst_p1 pub;blst_sk_to_pk_in_g1(&pub,&secrets[i]);blst_p1_compress(local.ssz.data()+at+121*i,&pub);}
 auto local_store=anchor(local,transition);local_store=prepare_phase0_store_tick(local_store,12000,rules,8);const auto id=local_store.timed.view.anchor;
 const auto& record=local_store.blocks.at(id);const auto context=prepare_phase0_fork_state_contexts(record.state,record.header,{0,id},transition.epoch,transition.state,0);
 const auto duties=phase0_attestation_duties(context.target.registry,context.target.randao,transition.epoch.rewards.duties,0);const auto committee=partition_committee(duties.shuffled,0,duties.committees_per_slot*8);check(!committee.empty(),"local committee");
 Phase0SignedAttestation vote;vote.data={0,0,id,{0,id},{0,id}};vote.aggregation_bits.resize(committee.size());vote.aggregation_bits[0]=true;
 auto signing_root=[&](const ConsensusAttestationData& d){return consensus_signing_root(consensus_attestation_root(d),consensus_domain({1,0,0,0},context.target.fork.current_version,context.target.genesis_root));};
 vote.signature=sign(secrets[committee[0]],signing_root(vote.data));
 auto voted=prepare_phase0_store_vote(local_store,vote,rules,8);check(voted&&voted->timed.view.latest_messages.contains(committee[0])&&local_store.timed.view.latest_messages.empty(),"store-bound BLS vote");
 auto forged=vote;forged.signature[0]^=1;check(!prepare_phase0_store_vote(local_store,forged,rules,8),"bad store vote signature");
 auto future=vote;future.data.slot=1;check(!prepare_phase0_store_vote(local_store,future,rules,8),"store vote cannot bypass slot window");
 IndexedConsensusAttestation first{vote.data,{committee[0]},vote.signature},second=first;second.data.source.root[0]^=1;second.signature=sign(secrets[committee[0]],signing_root(second.data));
 auto marked=prepare_phase0_store_slashing(*voted,first,second,rules);check(marked&&marked->timed.view.equivocating_indices.contains(committee[0])&&voted->timed.view.equivocating_indices.empty(),"store-bound proof");
 auto again=prepare_phase0_store_vote(*marked,vote,rules,8);check(again&&again->timed.view.latest_messages==marked->timed.view.latest_messages,"equivocation survives repeated vote");
 auto ticked=prepare_phase0_store_tick(*marked,24000,rules,8);check(ticked.timed.view.equivocating_indices==marked->timed.view.equivocating_indices,"tick preserves evidence");
 // Synthetic authenticated checkpoint metadata tests context retrieval across
 // skipped slots; it is not a claim that these empty slots justify themselves.
 auto promotion=prepare_phase0_store_tick(local_store,8*12000,rules,8);promotion.timed.clock.unrealized_justified={1,id};
 const auto promoted=prepare_phase0_store_tick(promotion,16*12000,rules,8);
 check(promoted.timed.view.justified==ConsensusCheckpoint{1,id}&&promoted.timed.view.justified_registry.epoch==1&&promoted.blocks.at(id).state==local,"automatic checkpoint context retrieval");
 std::cout<<Json{{"scenario","phase0-fork-store-blocks"},{"validSequences",valid},{"invalidSequences",invalid},{"nonGenesisFixturesExcluded",excluded},{"acceptedBlocks",accepted},{"cancellations",stops.size()},{"storeEventAdapters",true},{"durable",false},{"networkActive",false}}.dump()<<'\n';
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
