#include "nano/consensus_fork_clock.hpp"
#include <iostream>
#include <random>
using namespace nano;
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
ConsensusRoot root(uint8_t n){ConsensusRoot r{};r[0]=n;return r;}
int main(){try{
 Phase0ForkTimedView input{};input.clock={1000,1000+23*12000,{2,root(2)},{1,root(1)}};
 input.view.anchor=root(1);input.view.current_epoch=2;input.view.justified={0,root(1)};input.view.finalized=input.view.justified;input.view.justified_registry.epoch=0;input.view.proposer_boost=root(2);input.view.nodes[root(1)]={0,{},0,0};input.view.nodes[root(2)]={16,root(1),2,2};
 Phase0ForkVoteContext ctx{};ctx.checkpoint=input.clock.unrealized_justified;ctx.registry.epoch=ctx.randao.epoch=2;ConsensusSlashingValidator x;x.public_key[0]=1;x.activation_epoch=0;x.effective_balance=32;ctx.registry.validators[0]=x;
 auto same=prepare_phase0_fork_tick(input,input.clock.time_ms+11999,12000,8);check(same.view.proposer_boost==input.view.proposer_boost&&same.view.justified==input.view.justified,"within slot preserves boost and checkpoints");
 auto next=prepare_phase0_fork_tick(input,input.clock.time_ms+12000,12000,8,&ctx);check(!next.view.proposer_boost&&next.view.current_epoch==3&&next.view.justified==ctx.checkpoint&&next.view.justified_registry==ctx.registry&&next.view.finalized==input.clock.unrealized_finalized,"boundary promotion and weight snapshot");
 auto repeat=prepare_phase0_fork_tick(next,next.clock.time_ms,12000,8);check(repeat.view.justified==next.view.justified&&repeat.view.justified_registry==next.view.justified_registry,"same time idempotent");
 auto fast=input;fast.clock.time_ms=1023;auto huge=prepare_phase0_fork_tick(fast,UINT64_MAX,1,8,&ctx);check(huge.clock.time_ms==UINT64_MAX&&!huge.view.proposer_boost,"huge catchup without slot loop");
 unsigned rejected=0;auto refuse=[&](auto in,uint64_t time,uint64_t duration,uint64_t slots,const Phase0ForkVoteContext* context){bool threw=false;try{prepare_phase0_fork_tick(in,time,duration,slots,context);}catch(const std::invalid_argument&){threw=true;}check(threw,"bad clock accepted");++rejected;};
 const auto time=input.clock.time_ms;
 refuse(input,time-1,12000,8,&ctx);refuse(input,time,0,8,&ctx);refuse(input,time,12000,0,&ctx);refuse(input,time+12000,12000,8,nullptr);auto wrong=ctx;wrong.checkpoint.root=root(9);refuse(input,time+12000,12000,8,&wrong);wrong=ctx;wrong.registry.epoch=1;refuse(input,time+12000,12000,8,&wrong);
 auto bad=input;bad.view.current_epoch=3;refuse(bad,time,12000,8,&ctx);bad=input;bad.clock.genesis_time_ms=time+1;refuse(bad,time,12000,8,&ctx);
 for(unsigned stop=1;stop<=2;++stop){unsigned calls=0;bool caught=false;try{prepare_phase0_fork_tick(input,time+12000,12000,8,&ctx,[&]{if(++calls==stop)throw std::runtime_error("cancel");});}catch(const std::runtime_error&){caught=true;}check(caught&&input.clock.time_ms==time&&input.view.justified.epoch==0&&input.view.proposer_boost.has_value(),"cancel leaves original unchanged");}
 // Direct slot-by-slot reference over bounded random catch-up intervals.
 std::mt19937_64 rng(20260925);
 for(unsigned trial=0;trial<1000;++trial){auto in=input;const uint64_t old=16+rng()%8;in.clock.time_ms=1000+old*12000+rng()%12000;const uint64_t arrival=in.clock.time_ms+rng()%1000000;
  auto expected=in;const auto end=(arrival-1000)/12000;
  for(uint64_t s=old+1;s<=end;++s){expected.view.proposer_boost.reset();if(s%8==0){if(ctx.checkpoint.epoch>expected.view.justified.epoch){expected.view.justified=ctx.checkpoint;expected.view.justified_registry=ctx.registry;}if(in.clock.unrealized_finalized.epoch>expected.view.finalized.epoch)expected.view.finalized=in.clock.unrealized_finalized;}}
  auto result=prepare_phase0_fork_tick(in,arrival,12000,8,&ctx);
  check(result.view.justified==expected.view.justified&&result.view.finalized==expected.view.finalized&&result.view.proposer_boost==expected.view.proposer_boost&&result.view.justified_registry==expected.view.justified_registry&&result.view.current_epoch==end/8,"slot-by-slot oracle");
 }
 std::cout<<"Phase0 clock: 1000 catch-up comparisons, "<<rejected<<" rejections; network inactive\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
