#include "nano/consensus_epoch_maintenance.hpp"
#include <bit>
#include <stdexcept>
namespace nano {
namespace {
uint64_t next_epoch(uint64_t epoch){
 if(epoch==UINT64_MAX)throw std::overflow_error("epoch maintenance overflow");
 return epoch+1;
}
ConsensusRoot vector_root(std::span<const ConsensusRoot> roots,const std::function<void()>& checkpoint){
 std::vector<ConsensusRoot> level(roots.begin(),roots.end());
 while(level.size()>1){
  for(size_t i=0;i<level.size()/2;++i){
   if(checkpoint)checkpoint();
   level[i]=consensus_signing_root(level[2*i],level[2*i+1]);
  }
  level.resize(level.size()/2);
 }
 return level.front();
}
}
std::vector<Phase0Eth1Data> phase0_eth1_votes_after_reset(uint64_t epoch,
 std::span<const Phase0Eth1Data> votes,uint64_t period){
 if(!period)throw std::invalid_argument("zero Eth1 voting period");
 if(next_epoch(epoch)%period==0)return {};
 return {votes.begin(),votes.end()};
}
std::vector<uint64_t> phase0_slashings_after_reset(uint64_t epoch,
 std::span<const uint64_t> slashings,uint64_t length){
 if(!length||slashings.size()!=length)throw std::invalid_argument("invalid slashings vector");
 const auto next=next_epoch(epoch);
 std::vector<uint64_t> result(slashings.begin(),slashings.end());result[next%length]=0;return result;
}
ConsensusRandaoState phase0_randao_after_reset(const ConsensusRandaoState& state,const ConsensusRandaoRules& rules){
 auto result=state;
 // Reuse validated rotation and watermark preservation, but keep the component
 // epoch unchanged: process_slots advances it after ALL epoch operations.
 advance_consensus_randao_epoch(result,rules,state.epoch);result.epoch=state.epoch;return result;
}
std::vector<ConsensusRoot> phase0_history_after_update(uint64_t epoch,
 std::span<const ConsensusRoot> block_roots,std::span<const ConsensusRoot> state_roots,
 std::span<const ConsensusRoot> history,const Phase0HistoryRules& rules,
 const std::function<void()>& checkpoint){
 if(!rules.slots_per_epoch||!std::has_single_bit(rules.slots_per_historical_root)||
    rules.slots_per_historical_root%rules.slots_per_epoch||!rules.historical_roots_limit||
    block_roots.size()!=rules.slots_per_historical_root||state_roots.size()!=rules.slots_per_historical_root||
    history.size()>rules.historical_roots_limit)
  throw std::invalid_argument("invalid Phase0 history rules/state");
 const auto next=next_epoch(epoch);
 if(checkpoint)checkpoint();
 std::vector<ConsensusRoot> result(history.begin(),history.end());
 if(next%(rules.slots_per_historical_root/rules.slots_per_epoch)==0){
  if(result.size()==rules.historical_roots_limit)throw std::overflow_error("historical roots list full");
  const auto blocks=vector_root(block_roots,checkpoint),states=vector_root(state_roots,checkpoint);
  result.push_back(consensus_signing_root(blocks,states));
 }
 if(checkpoint)checkpoint();
 return result;
}
Phase0FfgState phase0_participation_after_rotation(const Phase0FfgState& state){
 auto result=state;result.previous_attestations=std::move(result.current_attestations);
 result.current_attestations.clear();return result;
}
}
