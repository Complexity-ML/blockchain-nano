#include "nano/consensus_fork_clock.hpp"
namespace nano {
Phase0ForkTimedView prepare_phase0_fork_tick(const Phase0ForkTimedView& input,
 uint64_t time_ms,uint64_t slot_duration_ms,uint64_t slots_per_epoch,
 const Phase0ForkVoteContext* promoted_context,const std::function<void()>& checkpoint){
 const auto& c=input.clock;const auto& v=input.view;
 auto require=[](bool ok){if(!ok)throw std::invalid_argument("invalid Phase0 fork clock");};
 require(slot_duration_ms&&slots_per_epoch&&c.time_ms>=c.genesis_time_ms&&time_ms>=c.time_ms);
 const auto old_slot=(c.time_ms-c.genesis_time_ms)/slot_duration_ms;
 const auto new_slot=(time_ms-c.genesis_time_ms)/slot_duration_ms;
 const auto old_epoch=old_slot/slots_per_epoch,new_epoch=new_slot/slots_per_epoch;
 require(v.current_epoch==old_epoch&&v.finalized.epoch<=v.justified.epoch&&v.justified.epoch<=old_epoch&&
  v.justified_registry.epoch==v.justified.epoch&&c.unrealized_finalized.epoch<=c.unrealized_justified.epoch&&c.unrealized_justified.epoch<=old_epoch);
 if(checkpoint)checkpoint();
 const bool boundary=new_epoch>old_epoch;
 const bool promote=boundary&&c.unrealized_justified.epoch>v.justified.epoch;
 if(promote){
  require(promoted_context&&promoted_context->checkpoint==c.unrealized_justified&&
   promoted_context->registry.epoch==c.unrealized_justified.epoch&&
   promoted_context->randao.epoch==c.unrealized_justified.epoch&&v.nodes.contains(c.unrealized_justified.root));
 }
 if(boundary&&c.unrealized_finalized.epoch>v.finalized.epoch)require(v.nodes.contains(c.unrealized_finalized.root));
 auto out=input;
 out.clock.time_ms=time_ms;out.view.current_epoch=new_epoch;
 if(new_slot>old_slot)out.view.proposer_boost.reset();
 if(promote){out.view.justified=c.unrealized_justified;out.view.justified_registry=promoted_context->registry;}
 if(boundary&&c.unrealized_finalized.epoch>v.finalized.epoch)out.view.finalized=c.unrealized_finalized;
 require(out.view.finalized.epoch<=out.view.justified.epoch);
 if(checkpoint)checkpoint();return out;
}
}
