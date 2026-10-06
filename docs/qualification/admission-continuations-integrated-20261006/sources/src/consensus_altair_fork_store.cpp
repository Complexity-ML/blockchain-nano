#include "nano/consensus_altair_fork_store.hpp"
namespace nano {
namespace {
uint64_t number(std::span<const uint8_t> b,size_t at,size_t width=8){uint64_t v=0;for(size_t i=0;i<width;++i)v|=uint64_t(b[at+i])<<(8*i);return v;}
ConsensusRoot root(std::span<const uint8_t> b,size_t at){ConsensusRoot r{};std::copy_n(b.begin()+at,32,r.begin());return r;}
ConsensusRoot ancestor(const Phase0ForkChoiceView& v,ConsensusRoot r,uint64_t slot,const std::function<void()>& checkpoint){
 for(;;){if(checkpoint)checkpoint();const auto& n=v.nodes.at(r);if(n.slot<=slot||r==v.anchor)return r;const auto& p=v.nodes.at(n.parent);if(p.slot>=n.slot)throw std::invalid_argument("invalid fork parent slots");r=n.parent;}
}
}
std::optional<AltairForkStore> prepare_altair_fork_block(const AltairForkStore& input,
 std::span<const uint8_t> bytes,const AltairForkBlockRules& rules,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint){
 const auto& t=rules.transition;const auto slots=t.epoch.history.slots_per_epoch;
 const auto& clock=input.timed.clock;const auto& view=input.timed.view;
 if(!slots||!rules.slot_duration_ms||rules.attestation_due_ms>rules.slot_duration_ms||clock.time_ms<clock.genesis_time_ms||view.finalized.epoch>UINT64_MAX/slots)
  throw std::invalid_argument("invalid fork block clock/configuration");
 const auto current_slot=(clock.time_ms-clock.genesis_time_ms)/rules.slot_duration_ms;
 if(view.current_epoch!=current_slot/slots)throw std::invalid_argument("fork clock epoch mismatch");
 if(bytes.size()<404||bytes.size()>t.maximum_block_bytes||number(bytes,0,4)!=100||number(bytes,180,4)!=84)return std::nullopt;
 ConsensusBeaconHeader h{number(bytes,100),number(bytes,108),root(bytes,116),root(bytes,148),{}};
 try{h.body_root=altair_beacon_body_root(bytes.subspan(184),t.body,t.state.sync_committee_size);}catch(const std::invalid_argument&){return std::nullopt;}
 const auto id=consensus_beacon_header_root(h);
 if(input.blocks.contains(id))return input;
 if(!input.blocks.contains(h.parent_root)||!view.nodes.contains(h.parent_root)||h.slot>current_slot||h.slot<=view.finalized.epoch*slots)return std::nullopt;
 if(ancestor(view,h.parent_root,view.finalized.epoch*slots,checkpoint)!=view.finalized.root)return std::nullopt;
 const auto& parent=input.blocks.at(h.parent_root);
 if(consensus_beacon_header_root(parent.header)!=h.parent_root||altair_beacon_state_root(parent.state.ssz,t.state,checkpoint)!=parent.header.state_root)
  throw std::invalid_argument("fork parent state binding mismatch");
 auto post=prepare_altair_state_transition(parent.state,bytes,t,maximum_slots,maximum_sync_trials,checkpoint);if(!post)return std::nullopt;
 const auto head=phase0_fork_choice_head(view,{slots,t.attestation.duties.effective_balance_increment,rules.proposer_score_boost},checkpoint).head;
 const auto ffg=altair_fork_checkpoints(*post,t.epoch,t.state,checkpoint);
 auto out=input;auto& v=out.timed.view;auto& c=out.timed.clock;
 out.blocks.emplace(id,AltairForkBlock{h,std::move(*post)});
 v.nodes.emplace(id,Phase0ForkNode{h.slot,h.parent_root,ffg.realized.current_justified.epoch,ffg.unrealized.current_justified.epoch});
 const auto into=(clock.time_ms-clock.genesis_time_ms)%rules.slot_duration_ms;
 const bool timely=h.slot==current_slot&&into<rules.attestation_due_ms;out.timely[id]=timely;
 const auto lookahead=t.attestation.duties.min_seed_lookahead;
 const auto dependent_epoch=v.current_epoch>lookahead?v.current_epoch-lookahead:0;
 const auto dependent_start=dependent_epoch*slots,dependent_slot=dependent_start?dependent_start-1:0;
 const bool same_dependency=ancestor(v,head,dependent_slot,checkpoint)==ancestor(v,id,dependent_slot,checkpoint);
 if(rules.boost_profile!=AltairProposerBoostProfile::Release160Beta0&&rules.boost_profile!=AltairProposerBoostProfile::ShufflingDependent)throw std::invalid_argument("unknown Altair boost profile");
 if(timely&&(!v.proposer_boost||*v.proposer_boost==ConsensusRoot{})&&(rules.boost_profile==AltairProposerBoostProfile::Release160Beta0||same_dependency))v.proposer_boost=id;
 auto update=[&](const Phase0FfgCheckpoints& cp){
  if(cp.current_justified.epoch>v.justified.epoch){
   const auto& record=out.blocks.at(cp.current_justified.root);
   const auto ctx=prepare_altair_fork_state_contexts(record.state,record.header,cp.current_justified,t.epoch,t.state,maximum_slots,maximum_sync_trials,checkpoint);
   v.justified=cp.current_justified;v.justified_registry=ctx.target.registry;
  }
  if(cp.finalized.epoch>v.finalized.epoch)v.finalized=cp.finalized;
 };
 update(ffg.realized);
 out.unrealized_justifications[id]=ffg.unrealized.current_justified;
 if(ffg.unrealized.current_justified.epoch>c.unrealized_justified.epoch)c.unrealized_justified=ffg.unrealized.current_justified;
 if(ffg.unrealized.finalized.epoch>c.unrealized_finalized.epoch)c.unrealized_finalized=ffg.unrealized.finalized;
 if(h.slot/slots<v.current_epoch)update(ffg.unrealized);
 if(checkpoint)checkpoint();return out;
}
}

namespace nano {
std::optional<AltairForkStore> prepare_altair_store_vote(const AltairForkStore& input,
 const Phase0SignedAttestation& vote,const AltairForkBlockRules& rules,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint){
 const auto& c=input.timed.clock;const auto slots=rules.transition.epoch.history.slots_per_epoch;
 if(!slots||!rules.slot_duration_ms||c.time_ms<c.genesis_time_ms)throw std::invalid_argument("invalid store vote clock");
 const auto slot=(c.time_ms-c.genesis_time_ms)/rules.slot_duration_ms,epoch=slot/slots;
 const auto& d=vote.data;
 if(d.slot>=slot||d.target.epoch!=d.slot/slots||(d.target.epoch!=epoch&&d.target.epoch!=(epoch?epoch-1:0))||
    !input.blocks.contains(d.target.root)||!input.timed.view.nodes.contains(d.beacon_block_root))return std::nullopt;
 const auto& block=input.blocks.at(d.target.root);
 if(block.header.slot>d.target.epoch*slots)return std::nullopt;
 const auto ctx=prepare_altair_fork_state_contexts(block.state,block.header,d.target,rules.transition.epoch,rules.transition.state,maximum_slots,maximum_sync_trials,checkpoint);
 auto out=input;
 if(!process_phase0_fork_vote(out.timed.view,slot,ctx.target,rules.transition.attestation.duties,vote,false,checkpoint))return std::nullopt;
 return out;
}
std::optional<AltairForkStore> prepare_altair_store_slashing(const AltairForkStore& input,
 const IndexedConsensusAttestation& first,const IndexedConsensusAttestation& second,
 const AltairForkBlockRules& rules,const std::function<void()>& checkpoint){
 const auto& block=input.blocks.at(input.timed.view.justified.root);
 const auto ctx=altair_fork_block_context(block.state,block.header,rules.transition.epoch,rules.transition.state,checkpoint);
 auto out=input;
 if(!process_phase0_fork_slashing(out.timed.view,ctx,first,second,rules.transition.body.validators_per_committee,checkpoint))return std::nullopt;
 return out;
}
AltairForkStore prepare_altair_store_tick(const AltairForkStore& input,uint64_t time_ms,
 const AltairForkBlockRules& rules,uint64_t maximum_slots,uint64_t maximum_sync_trials,const std::function<void()>& checkpoint){
 const auto& c=input.timed.clock;const auto slots=rules.transition.epoch.history.slots_per_epoch;
 if(!slots||!rules.slot_duration_ms||time_ms<c.time_ms||c.time_ms<c.genesis_time_ms)throw std::invalid_argument("invalid store tick clock");
 const auto epoch=((time_ms-c.genesis_time_ms)/rules.slot_duration_ms)/slots;
 std::optional<AltairForkStateContexts> ctx;
 if(epoch>input.timed.view.current_epoch&&c.unrealized_justified.epoch>input.timed.view.justified.epoch){
  const auto& block=input.blocks.at(c.unrealized_justified.root);
  ctx=prepare_altair_fork_state_contexts(block.state,block.header,c.unrealized_justified,rules.transition.epoch,rules.transition.state,maximum_slots,maximum_sync_trials,checkpoint);
 }
 auto timed=prepare_phase0_fork_tick(input.timed,time_ms,rules.slot_duration_ms,slots,ctx?&ctx->target:nullptr,checkpoint);
 auto out=input;out.timed=std::move(timed);if(checkpoint)checkpoint();return out;
}
}

namespace nano {
AltairForkStore initialize_altair_fork_store(const AltairSlotState& state,
 std::span<const uint8_t> block,const AltairForkBlockRules& r,const std::function<void()>& checkpoint){
 const auto slots=r.transition.epoch.ffg.slots_per_epoch;
 if(!slots||!r.slot_duration_ms||block.size()<304||block.size()>r.transition.maximum_block_bytes||number(block,80,4)!=84)
  throw std::invalid_argument("invalid Altair anchor block");
 const ConsensusBeaconHeader h{number(block,0),number(block,8),root(block,16),root(block,48),altair_beacon_body_root(block.subspan(84),r.transition.body,r.transition.state.sync_committee_size)};
 const auto context=altair_fork_block_context(state,h,r.transition.epoch,r.transition.state,checkpoint);const auto id=consensus_beacon_header_root(h);const auto epoch=h.slot/slots;
 const auto seconds=number(state.ssz,0);if(seconds>UINT64_MAX/1000||h.slot>(UINT64_MAX-seconds*1000)/r.slot_duration_ms)throw std::overflow_error("Altair anchor time overflow");
 AltairForkStore out;auto& v=out.timed.view;v.anchor=id;v.justified={epoch,id};v.finalized=v.justified;v.current_epoch=epoch;v.justified_registry=context.registry;
 v.nodes.emplace(id,Phase0ForkNode{h.slot,h.parent_root,epoch,epoch});out.blocks.emplace(id,AltairForkBlock{h,state});out.unrealized_justifications[id]=v.justified;
 out.timed.clock={seconds*1000,seconds*1000+h.slot*r.slot_duration_ms,v.justified,v.finalized};if(checkpoint)checkpoint();return out;
}
}

namespace nano {
namespace {
using View=std::span<const uint8_t>;
// These decoders only consume a body already checked by on_block's SSZ hasher.
ConsensusAttestationData embedded_data(View b){return {number(b,4),number(b,12),root(b,20),{number(b,52),root(b,60)},{number(b,92),root(b,100)}};}
IndexedConsensusAttestation embedded_indexed(View b){IndexedConsensusAttestation a{embedded_data(b),{},Bytes(b.begin()+132,b.begin()+228)};for(size_t i=228;i<b.size();i+=8)a.indices.push_back(number(b,i));return a;}
std::vector<View> embedded_list(View b){std::vector<View> list;if(b.empty())return list;const auto count=number(b,0,4)/4;for(size_t i=0;i<count;++i){const auto start=number(b,4*i,4),end=i+1==count?b.size():number(b,4*(i+1),4);list.push_back(b.subspan(start,end-start));}return list;}
}
std::optional<AltairForkStore> prepare_altair_received_block(const AltairForkStore& input,
 View bytes,const AltairForkBlockRules& rules,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint){
 auto out=prepare_altair_fork_block(input,bytes,rules,maximum_slots,maximum_sync_trials,checkpoint);if(!out)return std::nullopt;
 const auto body=bytes.subspan(184);const auto attesters=number(body,204,4),attestations=number(body,208,4),deposits=number(body,212,4);
 const auto slots=rules.transition.epoch.ffg.slots_per_epoch;
 const auto current_slot=(out->timed.clock.time_ms-out->timed.clock.genesis_time_ms)/rules.slot_duration_ms;
 for(auto b:embedded_list(body.subspan(attestations,deposits-attestations))){
  if(checkpoint)checkpoint();Phase0SignedAttestation vote{embedded_data(b),{},Bytes(b.begin()+132,b.begin()+228)};
  unsigned last=0;for(uint8_t n=b.back();n>1;n>>=1)++last;const auto bit_count=(b.size()-229)*8+last;
  for(size_t i=0;i<bit_count;++i)vote.aggregation_bits.push_back((b[228+i/8]>>(i%8))&1);
  const auto& d=vote.data;
  if(d.slot>=current_slot||d.target.epoch!=d.slot/slots||!out->blocks.contains(d.target.root)||!out->timed.view.nodes.contains(d.beacon_block_root))continue;
  const auto& target=out->blocks.at(d.target.root);if(target.header.slot>d.target.epoch*slots)continue;
  const auto ctx=prepare_altair_fork_state_contexts(target.state,target.header,d.target,rules.transition.epoch,rules.transition.state,maximum_slots,maximum_sync_trials,checkpoint);
  process_phase0_fork_vote(out->timed.view,current_slot,ctx.target,rules.transition.attestation.duties,vote,true,checkpoint);
 }
 for(auto b:embedded_list(body.subspan(attesters,attestations-attesters))){
  if(checkpoint)checkpoint();const auto second=number(b,4,4);const auto first=embedded_indexed(b.subspan(8,second-8)),other=embedded_indexed(b.subspan(second));
  const auto& target=out->blocks.at(out->timed.view.justified.root);const auto ctx=altair_fork_block_context(target.state,target.header,rules.transition.epoch,rules.transition.state,checkpoint);
  process_phase0_fork_slashing(out->timed.view,ctx,first,other,rules.transition.body.validators_per_committee,checkpoint);
 }
 if(checkpoint)checkpoint();return out;
}
}
