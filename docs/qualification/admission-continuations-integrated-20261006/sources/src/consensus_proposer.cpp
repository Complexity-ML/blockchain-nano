#include "nano/consensus_proposer.hpp"
#include "nano/committee.hpp"
#include "nano/types.hpp"
#include <algorithm>
namespace nano {
uint64_t phase0_proposer_index(std::span<const Phase0ProposerValidator> registry,
 const ConsensusRandaoState& randao,const ConsensusRandaoRules& rules,uint64_t slot,
 uint64_t maximum_effective_balance,uint16_t shuffle_rounds,const std::function<void()>& checkpoint){
 if(!rules.slots_per_epoch||slot/rules.slots_per_epoch!=randao.epoch||!maximum_effective_balance||!shuffle_rounds||shuffle_rounds>256)
  throw std::invalid_argument("invalid proposer epoch or preset");
 auto base=consensus_randao_seed(randao,rules,randao.epoch,{0,0,0,0});
 Bytes input(base.begin(),base.end());for(unsigned b=0;b<8;++b)input.push_back(uint8_t(slot>>(8*b)));
 auto digest=sha256(input);ConsensusRoot seed{};std::copy(digest.begin(),digest.end(),seed.begin());
 std::vector<size_t> active;
 for(size_t i=0;i<registry.size();++i){
  if(checkpoint&&(i%1024==0))checkpoint();
  const auto& v=registry[i];
  if((i&&v.index<=registry[i-1].index)||v.effective_balance>maximum_effective_balance||v.activation_epoch>v.exit_epoch)
   throw std::invalid_argument("invalid proposer registry");
  if(v.activation_epoch<=randao.epoch&&randao.epoch<v.exit_epoch)active.push_back(i);
 }
 if(active.empty())throw std::invalid_argument("no active proposer");
 Bytes random(32);input.assign(seed.begin(),seed.end());input.resize(40);
 for(uint64_t i=0;;++i){
  if(checkpoint)checkpoint();
  auto position=shuffled_validator_index(i%active.size(),active.size(),seed,shuffle_rounds);
  const auto& candidate=registry[active[position]];
  if(i%32==0){for(unsigned b=0;b<8;++b)input[32+b]=uint8_t((i/32)>>(8*b));random=sha256(input);}
  if(static_cast<__uint128_t>(candidate.effective_balance)*255>=static_cast<__uint128_t>(maximum_effective_balance)*random[i%32])return candidate.index;
  if(i==UINT64_MAX)throw std::overflow_error("proposer sampling counter exhausted");
 }
}

bool process_phase0_selected_randao(ConsensusRandaoState& randao,
 const ConsensusSlashingState& registry,const ConsensusRandaoRules& rules,
 uint64_t slot,uint64_t claimed_proposer,uint64_t maximum_effective_balance,uint16_t shuffle_rounds,
 const ConsensusBytes4& fork_version,const ConsensusRoot& genesis_root,
 std::span<const uint8_t> reveal,const std::function<void()>& checkpoint){
 if(registry.epoch!=randao.epoch)throw std::invalid_argument("proposer registry/RANDAO epoch mismatch");
 if(!rules.slots_per_epoch)throw std::invalid_argument("zero slots per epoch");
 if(slot/rules.slots_per_epoch!=randao.epoch||reveal.size()!=96||
    (randao.last_reveal_slot&&slot<=*randao.last_reveal_slot))return false;
 const auto found=registry.validators.find(claimed_proposer);
 if(found==registry.validators.end()||found->second.slashed)return false;
 const auto& v=found->second;
 if(v.activation_epoch>randao.epoch||randao.epoch>=v.exit_epoch)return false;
 std::vector<Phase0ProposerValidator> candidates;candidates.reserve(registry.validators.size());
 size_t visited=0;
 for(const auto& [index,record]:registry.validators){
  if(checkpoint&&visited++%1024==0)checkpoint();
  candidates.push_back({index,record.effective_balance,record.activation_epoch,record.exit_epoch});
 }
 if(phase0_proposer_index(candidates,randao,rules,slot,maximum_effective_balance,shuffle_rounds,checkpoint)!=claimed_proposer)return false;
 if(checkpoint)checkpoint();
 return process_consensus_randao(randao,rules,slot,fork_version,genesis_root,v.public_key,reveal);
}
}
