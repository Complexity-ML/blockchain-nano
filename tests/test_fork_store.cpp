#include "support/phase0_transition_rules.hpp"
#include "nano/consensus_fork_store.hpp"
using namespace test;
using namespace test::phase0_minimal;
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
  for(size_t i=0;i<row.at("blockCount").get<size_t>();++i){auto bytes=binary(dir/("blocks_"+std::to_string(i)+".ssz"));const uint64_t arrival=std::max(store.timed.clock.time_ms,little(bytes,100,8)*12000);std::optional<Phase0ForkStateContexts> ctx;
   const auto cp=store.timed.clock.unrealized_justified;if(cp.epoch>store.timed.view.justified.epoch){const auto& block=store.blocks.at(cp.root);ctx=prepare_phase0_fork_state_contexts(block.state,block.header,cp,transition.epoch,transition.state,4096);}
   store.timed=prepare_phase0_fork_tick(store.timed,arrival,12000,8,ctx?&ctx->target:nullptr);
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
 std::cout<<Json{{"scenario","phase0-fork-store-blocks"},{"validSequences",valid},{"invalidSequences",invalid},{"nonGenesisFixturesExcluded",excluded},{"acceptedBlocks",accepted},{"cancellations",stops.size()},{"durable",false},{"networkActive",false}}.dump()<<'\n';
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
