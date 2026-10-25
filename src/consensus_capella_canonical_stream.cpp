#include "nano/consensus_capella_canonical_stream.hpp"
#include "consensus_capella_stream_index.hpp"

namespace nano {
namespace canonical_stream_detail {
ConsensusBeaconHeader header(const CheckpointArchiveEntry& e,const ConsensusRoot& id,const CapellaForkBlockRules& r,size_t maximum,const std::function<void()>& check){
 const auto& t=r.transition;require(e.root==bytes(id)&&e.payload.size()>=404&&e.payload.size()<=maximum&&little(e.payload,0,4)==100&&little(e.payload,180,4)==84&&little(e.payload,100)==e.slot,"stream archive root/slot/envelope differs");
 ConsensusBeaconHeader h{e.slot,little(e.payload,108),root(e.payload,116),root(e.payload,148),capella_beacon_body_root(std::span<const uint8_t>(e.payload).subspan(184),t.consensus.body,t.consensus.state.sync_committee_size,t.execution,t.withdrawals.maximum_per_payload,t.maximum_bls_changes,t.consensus.maximum_block_bytes,check)};
 require(consensus_beacon_header_root(h)==id,"stream archive signed header root differs");return h;
}
size_t live_bytes(const CapellaForkStore& s,size_t maximum,const std::function<void()>& check,CanonicalStreamResult& result){
 result.peak_deposit_ancestor_records=std::max(result.peak_deposit_ancestor_records,s.deposit_ancestors.size());
 size_t total=0;for(const auto& [_,entry]:s.blocks){
  check();const auto p=entry.lease(check);require(p->state.ssz.size()<=maximum-total,"stream live state byte bound");total+=p->state.ssz.size();
  if(p->deposit_history)result.largest_deposit_history_count=std::max(result.largest_deposit_history_count,p->deposit_history->count());
 }return total;
}
}

CanonicalStreamResult stream_capella_canonical_history(const CapellaForkStore& trusted,const CapellaForkBlockRules& rules,const CapellaStoreSnapshotLimits& snapshot,const CanonicalStreamLimits& bound,const CanonicalStreamBoundary& boundary,const CanonicalStreamSource& source,const std::filesystem::path& index_path,const CanonicalStreamVisitor& visit,const std::function<void()>& check){
 using namespace canonical_stream_detail;
 require(bool(check)&&bool(source.archive)&&bool(source.verify_execution)&&bool(visit),"stream explicit source/visitor/cancellation required");check();
 const auto& t=rules.transition;const auto slots=t.consensus.epoch.ffg.slots_per_epoch;
 require(slots&&rules.slot_duration_ms&&snapshot.blocks&&snapshot.total_state_bytes&&bound.maximum_live_blocks>=2&&bound.maximum_live_blocks<=snapshot.blocks&&bound.maximum_live_state_bytes>0&&bound.maximum_live_state_bytes<=snapshot.total_state_bytes&&t.maximum_state_bytes<=snapshot.state_bytes&&t.maximum_state_bytes<=bound.maximum_live_state_bytes&&bound.maximum_total_blocks>0&&bound.maximum_total_archive_bytes>0&&bound.maximum_private_index_bytes>=record_bytes,"stream configured bounds refused");
 require(bound.maximum_total_blocks<=bound.maximum_private_index_bytes/record_bytes&&bound.maximum_private_index_bytes<=uint64_t(std::numeric_limits<off_t>::max()),"stream total/index bounds differ");
 require(trusted.blocks.size()==1&&trusted.blocks.contains(trusted.timed.view.anchor)&&capella_network_anchor(trusted)==trusted.timed.view.anchor,"stream single original trusted anchor required");
 const auto input_state_bytes=trusted.blocks.at(trusted.timed.view.anchor).lease(check)->state.ssz.size();require(input_state_bytes<=bound.maximum_live_state_bytes&&input_state_bytes<=bound.maximum_live_state_bytes-input_state_bytes,"stream input plus validated anchor byte bound");
 auto replay=[&]{const auto encoded=capella_store_snapshot(trusted,rules,snapshot,check);return capella_store_from_snapshot(encoded,sha256(Bytes(encoded.begin(),encoded.end())),rules,snapshot,check);}();
 const auto anchor=replay.timed.view.anchor,head=consensus_beacon_header_root(boundary.stopped_head);const auto anchor_header=replay.blocks.at(anchor).header();
 require(boundary.stopped_head.slot>=anchor_header.slot&&boundary.required_finalized.epoch<=boundary.stopped_finalized.epoch&&boundary.stopped_finalized.epoch<=boundary.stopped_head.slot/slots,"stream checkpoint/head bounds refused");
 std::array<ConsensusCheckpoint,2> checkpoints{boundary.required_finalized,boundary.stopped_finalized};std::array<bool,2> seen{};for(const auto& cp:checkpoints)require(cp.epoch<=UINT64_MAX/slots,"stream checkpoint slot overflow");
 auto bind_checkpoints=[&](const ConsensusBeaconHeader& h,const ConsensusRoot& id){for(size_t i=0;i<2;++i)if(!seen[i]&&h.slot<=checkpoints[i].epoch*slots){require(id==checkpoints[i].root,"stream external finalized boundary differs");seen[i]=true;}};
 Index index(index_path,bound.maximum_private_index_bytes);CanonicalStreamResult result;
 result.peak_deposit_ancestor_records=trusted.deposit_ancestors.size();
 {const auto input=trusted.blocks.at(anchor).lease(check);if(input->deposit_history)result.largest_deposit_history_count=input->deposit_history->count();}auto tip=head;uint64_t child=UINT64_MAX;
 while(tip!=anchor){
  check();require(index.count()<bound.maximum_total_blocks,"stream total block bound");const auto e=source.archive(tip,t.consensus.maximum_block_bytes,check);require(bool(e),"stream canonical signed archive missing");const auto h=header(*e,tip,rules,t.consensus.maximum_block_bytes,check);
  require(h.slot<child&&h.slot>anchor_header.slot&&h.slot<=boundary.stopped_head.slot,"stream ancestry does not descend");require(e->payload.size()<=bound.maximum_total_archive_bytes-result.archive_bytes,"stream total archive byte bound");result.archive_bytes+=e->payload.size();bind_checkpoints(h,tip);index.append(record(tip,*e),check);child=h.slot;tip=h.parent_root;
 }
 bind_checkpoints(anchor_header,anchor);for(size_t i=0;i<2;++i)if(!seen[i]&&checkpoints[i].epoch==trusted.timed.view.finalized.epoch&&checkpoints[i].root==anchor)seen[i]=true;
 require(seen[0]&&seen[1],"stream external finalized ancestor outside anchor");index.seal();index.verify(check);result.index_bytes=index.count()*record_bytes;
 auto measure=[&]{
  require(replay.blocks.size()<bound.maximum_live_blocks,"stream live record bound");
  const auto n=live_bytes(replay,bound.maximum_live_state_bytes-input_state_bytes,check,result)+input_state_bytes;
  result.peak_live_blocks=std::max(result.peak_live_blocks,replay.blocks.size()+1);
  result.peak_live_state_ssz_bytes=std::max(result.peak_live_state_ssz_bytes,n);return n;
 };
 auto prune=[&]{
  const auto& v=replay.timed.view;const auto lookahead=t.consensus.attestation.duties.min_seed_lookahead;
  if(v.finalized.root==v.anchor||lookahead>=v.current_epoch)return;
  const auto retained_epoch=v.current_epoch-lookahead-1;require(retained_epoch<=UINT64_MAX/slots,"stream pruning epoch overflow");
  const auto retained_slot=retained_epoch*slots;auto at=v.finalized.root;uint64_t child_slot=UINT64_MAX;
  // The newest finalized root can be too recent for the shuffling lookahead.
  // Walk only its admitted ancestry; never choose a root by a slot lookup alone.
  for(size_t traversed=0;;++traversed){
   check();require(traversed<replay.blocks.size(),"stream finalized pruning ancestry bound");
   const auto found=replay.blocks.find(at);require(found!=replay.blocks.end(),"stream finalized pruning ancestor unavailable");
   const auto& h=found->second.header();require(h.slot<child_slot,"stream finalized pruning ancestry does not descend");
   if(h.slot<=retained_slot)break;
   if(at==v.anchor)return;
   child_slot=h.slot;at=h.parent_root;
  }
  if(at==v.anchor)return;
  auto next=prepare_capella_pruned_store(replay,at,rules,check);
  require(next.blocks.size()<=replay.blocks.size()&&capella_network_anchor(next)==anchor,"stream pruning identity differs");
  replay=std::move(next);++result.prunes;
 };
 {const auto first=replay.blocks.at(anchor).lease(check);source.verify_execution(*first,check);visit({nullptr,*first,{},anchor,0},check);}measure();
 for(uint64_t ordinal=0;ordinal<index.count();++ordinal){
  check();prune();const auto n=measure();require(replay.blocks.size()+1<bound.maximum_live_blocks&&t.maximum_state_bytes<=bound.maximum_live_state_bytes-n,"stream finalized window unavailable before next state");result.peak_live_state_ssz_plus_next_reserve_bytes=std::max(result.peak_live_state_ssz_plus_next_reserve_bytes,n+t.maximum_state_bytes);
  const auto row=index.read(index.count()-ordinal-1,check);const auto id=root(row,0);const auto e=source.archive(id,t.consensus.maximum_block_bytes,check);require(e&&e->payload.size()>=404&&e->payload.size()<=t.consensus.maximum_block_bytes&&record(id,*e)==row,"stream archive changed since ancestry indexing");const auto h=header(*e,id,rules,t.consensus.maximum_block_bytes,check);require(replay.blocks.contains(h.parent_root),"stream replay parent unavailable");
  require(h.slot<=(UINT64_MAX-replay.timed.clock.genesis_time_ms)/rules.slot_duration_ms,"stream replay clock overflow");replay=prepare_capella_resumed_store_tick(replay,replay.timed.clock.genesis_time_ms+h.slot*rules.slot_duration_ms,rules,snapshot.replay_slots,snapshot.sync_trials,check);
  auto next=prepare_capella_resumed_fork_block(replay,e->payload,rules,snapshot.replay_slots,snapshot.sync_trials,check);require(next&&next->blocks.contains(id)&&next->blocks.size()==replay.blocks.size()+1,"stream canonical signed transition rejected");
  {const auto parent=replay.blocks.at(h.parent_root).lease(check),post=next->blocks.at(id).lease(check);require(post->header==h,"stream admitted signed header differs");source.verify_execution(*post,check);visit({parent.get(),*post,e->payload,id,ordinal+1},check);}
  replay=std::move(*next);++result.verified_signed_blocks;measure();
 }
 index.verify(check);require(replay.blocks.contains(head),"stream stopped head unavailable");result.head=replay.blocks.at(head).lease(check);require(result.head->header==boundary.stopped_head,"stream stopped head differs from authenticated replay");result.replay_anchor=replay.timed.view.anchor;result.replay_anchor_slot=replay.blocks.at(result.replay_anchor).header().slot;
 result.required_finalized_ancestor_verified=true;check();return result;
}
}
