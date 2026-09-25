#include "nano/consensus_fork_store.hpp"
namespace nano {
namespace {
uint64_t number(std::span<const uint8_t> b,size_t at,size_t width=8){uint64_t v=0;for(size_t i=0;i<width;++i)v|=uint64_t(b[at+i])<<(8*i);return v;}
ConsensusRoot root(std::span<const uint8_t> b,size_t at){ConsensusRoot r{};std::copy_n(b.begin()+at,32,r.begin());return r;}
ConsensusRoot ancestor(const Phase0ForkChoiceView& v,ConsensusRoot r,uint64_t slot,const std::function<void()>& checkpoint){
 for(;;){if(checkpoint)checkpoint();const auto& n=v.nodes.at(r);if(n.slot<=slot||r==v.anchor)return r;const auto& p=v.nodes.at(n.parent);if(p.slot>=n.slot)throw std::invalid_argument("invalid fork parent slots");r=n.parent;}
}
}
std::optional<Phase0ForkStore> prepare_phase0_fork_block(const Phase0ForkStore& input,
 std::span<const uint8_t> bytes,const Phase0ForkBlockRules& rules,uint64_t maximum_slots,
 const std::function<void()>& checkpoint){
 const auto& t=rules.transition;const auto slots=t.epoch.history.slots_per_epoch;
 const auto& clock=input.timed.clock;const auto& view=input.timed.view;
 if(!slots||!rules.slot_duration_ms||rules.attestation_due_ms>rules.slot_duration_ms||clock.time_ms<clock.genesis_time_ms||view.finalized.epoch>UINT64_MAX/slots)
  throw std::invalid_argument("invalid fork block clock/configuration");
 const auto current_slot=(clock.time_ms-clock.genesis_time_ms)/rules.slot_duration_ms;
 if(view.current_epoch!=current_slot/slots)throw std::invalid_argument("fork clock epoch mismatch");
 if(bytes.size()<404||bytes.size()>t.maximum_block_bytes||number(bytes,0,4)!=100||number(bytes,180,4)!=84)return std::nullopt;
 ConsensusBeaconHeader h{number(bytes,100),number(bytes,108),root(bytes,116),root(bytes,148),{}};
 try{h.body_root=phase0_beacon_body_root(bytes.subspan(184),t.body);}catch(const std::invalid_argument&){return std::nullopt;}
 const auto id=consensus_beacon_header_root(h);
 if(input.blocks.contains(id))return input;
 if(!input.blocks.contains(h.parent_root)||!view.nodes.contains(h.parent_root)||h.slot>current_slot||h.slot<=view.finalized.epoch*slots)return std::nullopt;
 if(ancestor(view,h.parent_root,view.finalized.epoch*slots,checkpoint)!=view.finalized.root)return std::nullopt;
 const auto& parent=input.blocks.at(h.parent_root);
 if(consensus_beacon_header_root(parent.header)!=h.parent_root||phase0_beacon_state_root(parent.state.ssz,t.state,checkpoint)!=parent.header.state_root)
  throw std::invalid_argument("fork parent state binding mismatch");
 auto post=prepare_phase0_state_transition(parent.state,bytes,t,maximum_slots,checkpoint);if(!post)return std::nullopt;
 const auto head=phase0_fork_choice_head(view,{slots,t.epoch.rewards.duties.effective_balance_increment,rules.proposer_score_boost},checkpoint).head;
 const auto ffg=phase0_fork_checkpoints(*post,t.epoch,t.state,checkpoint);
 auto out=input;auto& v=out.timed.view;auto& c=out.timed.clock;
 out.blocks.emplace(id,Phase0ForkBlock{h,std::move(*post)});
 v.nodes.emplace(id,Phase0ForkNode{h.slot,h.parent_root,ffg.realized.current_justified.epoch,ffg.unrealized.current_justified.epoch});
 const auto into=(clock.time_ms-clock.genesis_time_ms)%rules.slot_duration_ms;
 const bool timely=h.slot==current_slot&&into<rules.attestation_due_ms;out.timely[id]=timely;
 const auto lookahead=t.epoch.rewards.duties.min_seed_lookahead;
 const auto dependent_epoch=v.current_epoch>lookahead?v.current_epoch-lookahead:0;
 const auto dependent_start=dependent_epoch*slots,dependent_slot=dependent_start?dependent_start-1:0;
 if(timely&&(!v.proposer_boost||*v.proposer_boost==ConsensusRoot{})&&ancestor(v,head,dependent_slot,checkpoint)==ancestor(v,id,dependent_slot,checkpoint))v.proposer_boost=id;
 auto update=[&](const Phase0FfgCheckpoints& cp){
  if(cp.current_justified.epoch>v.justified.epoch){
   const auto& record=out.blocks.at(cp.current_justified.root);
   const auto ctx=prepare_phase0_fork_state_contexts(record.state,record.header,cp.current_justified,t.epoch,t.state,maximum_slots,checkpoint);
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
