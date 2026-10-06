#include "nano/consensus_ffg.hpp"
#include "nano/committee.hpp"
#include "nano/types.hpp"
#include <algorithm>
#include <set>
namespace nano {
namespace {
Int amount(uint64_t v){return Int(std::to_string(v));}
ConsensusRoot target_root(const Phase0FfgState& state,uint64_t epoch,uint64_t slots){
 if(epoch>UINT64_MAX/slots)throw std::invalid_argument("FFG epoch slot overflow");
 const auto slot=epoch*slots;
 if(slot>=state.slot||state.slot-slot>state.block_roots.size())throw std::invalid_argument("FFG target outside block history");
 return state.block_roots[slot%state.block_roots.size()];
}
Int target_balance(const Phase0FfgState& state,const ConsensusSlashingState& registry,
 const ConsensusRandaoState& randao,const Phase0FfgRules& rules,uint64_t epoch,
 const std::vector<Phase0PendingAttestation>& votes,const std::function<void()>& checkpoint){
 const auto duties=phase0_attestation_duties(registry,randao,rules,epoch,checkpoint);
 const auto per_slot=duties.committees_per_slot,count=per_slot*rules.slots_per_epoch;
 const auto& shuffled=duties.shuffled;
 std::set<uint64_t> participants;
 for(const auto& vote:votes){
  if(checkpoint)checkpoint();
  if(vote.data.slot/rules.slots_per_epoch!=epoch||vote.data.target.epoch!=epoch||vote.data.index>=per_slot)
   throw std::invalid_argument("FFG pending duty outside epoch");
  const auto committee=partition_committee(shuffled,(vote.data.slot%rules.slots_per_epoch)*per_slot+vote.data.index,count);
  if(vote.aggregation_bits.size()!=committee.size())throw std::invalid_argument("FFG pending bitlist length");
  if(vote.data.target.root!=target_root(state,epoch,rules.slots_per_epoch))continue;
  for(size_t i=0;i<committee.size();++i)if(vote.aggregation_bits[i]&&!registry.validators.at(committee[i]).slashed)participants.insert(committee[i]);
 }
 Int balance=0;for(auto index:participants)balance+=amount(registry.validators.at(index).effective_balance);
 return std::max(amount(rules.effective_balance_increment),balance);
}
}
Phase0AttestationDuties phase0_attestation_duties(
 const ConsensusSlashingState& registry,const ConsensusRandaoState& randao,
 const Phase0FfgRules& rules,uint64_t epoch,const std::function<void()>& checkpoint){
 if(!rules.slots_per_epoch||!rules.target_committee_size||!rules.max_committees_per_slot||
    rules.max_committees_per_slot>UINT64_MAX/rules.slots_per_epoch||!rules.shuffle_rounds||rules.shuffle_rounds>256||
    randao.mixes.size()<3||rules.min_seed_lookahead>=randao.mixes.size()-2||randao.epoch!=registry.epoch||
    (epoch!=registry.epoch&&(registry.epoch==0||epoch!=registry.epoch-1)))
  throw std::invalid_argument("invalid attestation duty context");
 std::vector<uint64_t> active;
 uint64_t expected=0;std::set<std::array<uint8_t,48>> keys;
 for(const auto& [index,v]:registry.validators){if(checkpoint)checkpoint();
  if(index!=expected++||!keys.insert(v.public_key).second||v.activation_epoch>v.exit_epoch||v.exit_epoch>v.withdrawable_epoch)
   throw std::invalid_argument("invalid attestation duty registry");
  if(v.activation_epoch<=epoch&&epoch<v.exit_epoch)active.push_back(index);
 }
 const auto per_slot=std::max(uint64_t(1),std::min(rules.max_committees_per_slot,uint64_t(active.size())/rules.slots_per_epoch/rules.target_committee_size));
 const uint64_t n=randao.mixes.size(),position=epoch%n,back=rules.min_seed_lookahead+1;
 const auto mix=position>=back?position-back:n-(back-position);
 Bytes seed_bytes{1,0,0,0};for(unsigned i=0;i<8;++i)seed_bytes.push_back(uint8_t(epoch>>(i*8)));
 seed_bytes.insert(seed_bytes.end(),randao.mixes[mix].begin(),randao.mixes[mix].end());const auto hash=sha256(seed_bytes);ConsensusRoot seed{};std::copy(hash.begin(),hash.end(),seed.begin());
 auto shuffled=shuffled_validator_indices(active.size(),seed,rules.shuffle_rounds);for(auto& i:shuffled)i=active[i];
 if(checkpoint)checkpoint();
 return {per_slot,std::move(shuffled)};
}
Phase0FfgCandidate prepare_phase0_justification_finalization(
 const Phase0FfgState& state,const ConsensusSlashingState& registry,
 const ConsensusRandaoState& randao,const Phase0FfgRules& rules,const std::function<void()>& checkpoint){
 if(!rules.slots_per_epoch||!rules.target_committee_size||!rules.max_committees_per_slot||
    rules.max_committees_per_slot>UINT64_MAX/rules.slots_per_epoch||!rules.effective_balance_increment||
    !rules.maximum_effective_balance||rules.maximum_effective_balance<rules.effective_balance_increment||
    !rules.shuffle_rounds||rules.shuffle_rounds>256||state.block_roots.size()/rules.slots_per_epoch<2||
    randao.mixes.size()<3||rules.min_seed_lookahead>=randao.mixes.size()-2||
    state.slot/rules.slots_per_epoch!=registry.epoch||randao.epoch!=registry.epoch||state.checkpoints.justification_bits>15||
    state.checkpoints.previous_justified.epoch>registry.epoch||state.checkpoints.current_justified.epoch>registry.epoch||state.checkpoints.finalized.epoch>registry.epoch)
  throw std::invalid_argument("invalid Phase0 FFG context");
 if(checkpoint)checkpoint();
 Phase0FfgCandidate out{state.checkpoints,0,0,0};
 if(registry.epoch<=1)return out;
 uint64_t index=0;
 for(const auto& [i,v]:registry.validators){
  if(checkpoint)checkpoint();
  if(i!=index++||v.activation_epoch>v.exit_epoch||v.exit_epoch>v.withdrawable_epoch||v.effective_balance>rules.maximum_effective_balance)
   throw std::invalid_argument("invalid FFG validator registry");
  if(v.activation_epoch<=registry.epoch&&registry.epoch<v.exit_epoch)out.total_active_balance+=amount(v.effective_balance);
 }
 out.total_active_balance=std::max(amount(rules.effective_balance_increment),out.total_active_balance);
 out.previous_target_balance=target_balance(state,registry,randao,rules,registry.epoch-1,state.previous_attestations,checkpoint);
 out.current_target_balance=target_balance(state,registry,randao,rules,registry.epoch,state.current_attestations,checkpoint);
 auto& cp=out.checkpoints;const auto old_previous=cp.previous_justified,old_current=cp.current_justified;
 cp.previous_justified=old_current;cp.justification_bits=uint8_t((cp.justification_bits<<1)&15);
 if(out.previous_target_balance*3>=out.total_active_balance*2){cp.current_justified={registry.epoch-1,target_root(state,registry.epoch-1,rules.slots_per_epoch)};cp.justification_bits|=2;}
 if(out.current_target_balance*3>=out.total_active_balance*2){cp.current_justified={registry.epoch,target_root(state,registry.epoch,rules.slots_per_epoch)};cp.justification_bits|=1;}
 // Compare by subtraction to avoid overflowing hostile checkpoint epochs.
 const auto bits=cp.justification_bits;const auto epoch=registry.epoch;
 if((bits&14)==14&&epoch>=3&&old_previous.epoch==epoch-3)cp.finalized=old_previous;
 if((bits&6)==6&&old_previous.epoch==epoch-2)cp.finalized=old_previous;
 if((bits&7)==7&&old_current.epoch==epoch-2)cp.finalized=old_current;
 if((bits&3)==3&&old_current.epoch==epoch-1)cp.finalized=old_current;
 if(checkpoint)checkpoint();
 return out;
}
}
