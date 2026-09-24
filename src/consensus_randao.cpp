#include "nano/consensus_randao.hpp"
#include "nano/consensus_bls.hpp"
#include "nano/types.hpp"
#include <algorithm>
#include <stdexcept>
namespace nano {
namespace {
void validate(const ConsensusRandaoState& state,const ConsensusRandaoRules& rules){
 if(!rules.slots_per_epoch||state.mixes.size()<2||rules.min_seed_lookahead>=state.mixes.size()-1)
  throw std::invalid_argument("invalid RANDAO rules/history length");
 if(state.last_reveal_slot&&*state.last_reveal_slot/rules.slots_per_epoch>state.epoch)
  throw std::invalid_argument("RANDAO reveal watermark from future epoch");
}
ConsensusRoot epoch_root(uint64_t epoch){ConsensusRoot root{};for(size_t i=0;i<8;++i)root[i]=uint8_t(epoch>>(8*i));return root;}
}
bool process_consensus_randao(ConsensusRandaoState& state,const ConsensusRandaoRules& rules,
 uint64_t slot,const ConsensusBytes4& fork_version,const ConsensusRoot& genesis_root,
 std::span<const uint8_t> proposer_key,std::span<const uint8_t> reveal){
 validate(state,rules);
 if(slot/rules.slots_per_epoch!=state.epoch||(state.last_reveal_slot&&slot<=*state.last_reveal_slot))return false;
 const auto signing_root=consensus_signing_root(epoch_root(state.epoch),consensus_domain({2,0,0,0},fork_version,genesis_root));
 if(!verify_consensus_bls(proposer_key,signing_root,reveal))return false;
 const auto digest=sha256(Bytes(reveal.begin(),reveal.end()));
 auto next=state.mixes[state.epoch%state.mixes.size()];
 for(size_t i=0;i<next.size();++i)next[i]^=digest[i];
 state.mixes[state.epoch%state.mixes.size()]=next;
 state.last_reveal_slot=slot;
 return true;
}
void advance_consensus_randao_epoch(ConsensusRandaoState& state,const ConsensusRandaoRules& rules,uint64_t expected_epoch){
 validate(state,rules);
 if(state.epoch!=expected_epoch||state.epoch==UINT64_MAX)throw std::invalid_argument("invalid RANDAO epoch transition");
 const auto next=state.epoch+1;
 state.mixes[next%state.mixes.size()]=state.mixes[state.epoch%state.mixes.size()];
 state.epoch=next;
}
ConsensusRoot consensus_randao_seed(const ConsensusRandaoState& state,const ConsensusRandaoRules& rules,
 uint64_t duty_epoch,const ConsensusBytes4& duty_domain){
 validate(state,rules);
 if(duty_epoch!=state.epoch&&(state.epoch==UINT64_MAX||duty_epoch!=state.epoch+1))
  throw std::invalid_argument("RANDAO duty requires current/next epoch snapshot");
 const uint64_t n=state.mixes.size(),position=duty_epoch%n,back=rules.min_seed_lookahead+1;
 const auto index=position>=back?position-back:n-(back-position);
 Bytes input(duty_domain.begin(),duty_domain.end());
 auto epoch=epoch_root(duty_epoch);input.insert(input.end(),epoch.begin(),epoch.begin()+8);
 input.insert(input.end(),state.mixes[index].begin(),state.mixes[index].end());
 auto digest=sha256(input);ConsensusRoot result{};std::copy(digest.begin(),digest.end(),result.begin());return result;
}
}
