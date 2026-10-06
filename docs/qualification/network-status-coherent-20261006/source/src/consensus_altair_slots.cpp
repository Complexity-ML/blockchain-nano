#include "nano/consensus_altair_slots.hpp"
#include "consensus_epoch_common.hpp"
#include <algorithm>
namespace nano {
namespace {
void require(bool ok){if(!ok)throw std::invalid_argument("incoherent Altair epoch/slot state");}
uint64_t number(const Bytes& b,size_t at,size_t width=8){require(at<=b.size()&&width<=b.size()-at);uint64_t n=0;for(size_t i=0;i<width;++i)n|=uint64_t(b[at+i])<<(8*i);return n;}
void put(Bytes& b,size_t at,uint64_t n,size_t width=8){require(at<=b.size()&&width<=b.size()-at&&(width!=4||n<=UINT32_MAX));for(size_t i=0;i<width;++i)b[at+i]=uint8_t(n>>(8*i));}
template<size_t N>std::array<uint8_t,N> array(const Bytes& b,size_t at){require(at<=b.size()&&N<=b.size()-at);std::array<uint8_t,N> a{};std::copy_n(b.begin()+at,N,a.begin());return a;}
void putroot(Bytes& b,size_t at,const ConsensusRoot& root){require(at<=b.size()&&32<=b.size()-at);std::copy(root.begin(),root.end(),b.begin()+at);}
struct Layout {
 size_t historical,randao,previous;std::array<size_t,7> fields;
 explicit Layout(const AltairStateLimits& l){historical=176+64*l.slots_per_historical_root;randao=historical+96;previous=randao+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector;fields={historical,historical+76,historical+88,historical+92,previous,previous+4,previous+129};}
};
void config(const AltairEpochRules& r,const AltairStateLimits& l){
 const auto slots=r.ffg.slots_per_epoch,inc=r.ffg.effective_balance_increment,max=r.effective.maximum;
 require(slots&&inc&&r.inactivity.slots_per_epoch==slots&&r.rewards.slots_per_epoch==slots&&r.sync.slots_per_epoch==slots&&r.history.slots_per_epoch==slots&&r.rewards.effective_balance_increment==inc&&r.effective.increment==inc&&r.slashing.effective_balance_increment==inc&&r.registry.maximum_effective_balance==max&&r.sync.maximum_effective_balance==max&&r.inactivity.score_bias==r.rewards.score_bias&&r.inactivity.minimum_epochs_to_inactivity_penalty==r.rewards.minimum_epochs_to_inactivity_penalty&&r.history.slots_per_historical_root==l.slots_per_historical_root&&r.history.historical_roots_limit==l.historical_roots_limit&&r.slashing.epochs_per_slashings_vector==l.epochs_per_slashings_vector&&r.epochs_per_eth1_voting_period&&r.epochs_per_eth1_voting_period<=UINT64_MAX/slots&&r.epochs_per_eth1_voting_period*slots==l.eth1_votes_limit);
}
void coherent(const Bytes& b,const Layout& l){const auto v=number(b,l.fields[2],4),bal=number(b,l.fields[3],4),prev=number(b,l.fields[4],4),curr=number(b,l.fields[5],4),scores=number(b,l.fields[6],4),n=(bal-v)/121;require((prev-bal)/8==n&&curr-prev==n&&scores-curr==n&&(b.size()-scores)/8==n);}
}
void validate_altair_epoch_rules(const AltairEpochRules& r,const AltairStateLimits& l){config(r,l);}
AltairSlotState detail::prepare_common_epoch(const AltairSlotState& input,const AltairEpochRules& r,
 const AltairStateLimits& l,uint64_t expected,uint64_t trials,const std::function<void()>& checkpoint,bool update_history){
 config(r,l);altair_beacon_state_root(input.ssz,l,checkpoint);const Layout layout(l);coherent(input.ssz,layout);
 const auto slot=number(input.ssz,40),epoch=slot/r.ffg.slots_per_epoch;
 require(slot!=UINT64_MAX&&number(input.ssz,64)<=slot&&(slot+1)%r.ffg.slots_per_epoch==0&&epoch==expected&&(!input.last_processed_epoch||(epoch&&*input.last_processed_epoch==epoch-1)));
 auto out=input;
 out.ssz=prepare_altair_justification_finalization(out.ssz,l,r.ffg,checkpoint);
 out.ssz=prepare_altair_inactivity_updates(out.ssz,l,r.inactivity,checkpoint);
 out.ssz=prepare_altair_rewards(out.ssz,l,r.rewards,checkpoint);
 auto& b=out.ssz;std::array<Bytes,7> lists;
 for(size_t i=0;i<7;++i){const auto start=number(b,layout.fields[i],4),end=i==6?b.size():number(b,layout.fields[i+1],4);lists[i]=Bytes(b.begin()+start,b.begin()+end);}
 ConsensusSlashingState registry;registry.epoch=epoch;
 for(size_t i=0;i<lists[2].size()/121;++i){if(checkpoint)checkpoint();const auto at=121*i;ConsensusSlashingValidator v;v.public_key=array<48>(lists[2],at);v.withdrawal_credentials=array<32>(lists[2],at+48);v.effective_balance=number(lists[2],at+80);v.slashed=lists[2][at+88];v.activation_eligibility_epoch=number(lists[2],at+89);v.activation_epoch=number(lists[2],at+97);v.exit_epoch=number(lists[2],at+105);v.withdrawable_epoch=number(lists[2],at+113);v.balance=number(lists[3],8*i);registry.validators[i]=v;}
 const auto slash_at=layout.randao+32*l.epochs_per_historical_vector;
 for(size_t i=0;i<l.epochs_per_slashings_vector;++i)registry.slashings.push_back(number(b,slash_at+8*i));
 registry=prepare_phase0_registry_updates(registry,r.registry,number(b,layout.previous+89),checkpoint).registry_after;
 const auto debits=phase0_slashing_penalties(registry,r.slashing);for(const auto& [i,debit]:debits){if(checkpoint)checkpoint();registry.validators.at(i).balance-=debit;}
 if((epoch+1)%r.epochs_per_eth1_voting_period==0)lists[1].clear();
 registry=prepare_phase0_effective_balances(registry,r.effective,checkpoint);
 registry.slashings=phase0_slashings_after_reset(epoch,registry.slashings,l.epochs_per_slashings_vector);
 ConsensusRandaoState randao;randao.epoch=epoch;randao.last_reveal_slot=number(b,64);for(size_t i=0;i<l.epochs_per_historical_vector;++i)randao.mixes.push_back(array<32>(b,layout.randao+32*i));randao=phase0_randao_after_reset(randao,{r.ffg.slots_per_epoch,r.sync.min_seed_lookahead});
 if(update_history){std::vector<ConsensusRoot> blocks,states,history;for(size_t i=0;i<l.slots_per_historical_root;++i){blocks.push_back(array<32>(b,176+32*i));states.push_back(array<32>(b,176+32*l.slots_per_historical_root+32*i));}for(size_t at=0;at<lists[0].size();at+=32)history.push_back(array<32>(lists[0],at));history=phase0_history_after_update(epoch,blocks,states,history,r.history,checkpoint);lists[0].clear();for(const auto& root:history)lists[0].insert(lists[0].end(),root.begin(),root.end());}
 for(const auto& [i,v]:registry.validators){if(checkpoint)checkpoint();const size_t at=121*i;put(lists[2],at+80,v.effective_balance);lists[2][at+88]=v.slashed;put(lists[2],at+89,v.activation_eligibility_epoch);put(lists[2],at+97,v.activation_epoch);put(lists[2],at+105,v.exit_epoch);put(lists[2],at+113,v.withdrawable_epoch);put(lists[3],8*i,v.balance);}
 for(size_t i=0;i<randao.mixes.size();++i)putroot(b,layout.randao+32*i,randao.mixes[i]);for(size_t i=0;i<registry.slashings.size();++i)put(b,slash_at+8*i,registry.slashings[i]);
 b.resize(number(b,layout.fields[0],4));for(size_t i=0;i<7;++i){require(lists[i].size()<=UINT32_MAX&&b.size()<=UINT32_MAX-lists[i].size());put(b,layout.fields[i],b.size(),4);b.insert(b.end(),lists[i].begin(),lists[i].end());}
 b=prepare_altair_participation_updates(b,l,checkpoint);b=prepare_altair_sync_updates(b,l,r.sync,trials,checkpoint);out.last_processed_epoch=epoch;
 altair_beacon_state_root(b,l,checkpoint);if(checkpoint)checkpoint();return out;
}
AltairSlotState prepare_altair_epoch(const AltairSlotState& input,const AltairEpochRules& r,
 const AltairStateLimits& l,uint64_t expected,uint64_t trials,const std::function<void()>& check){
 return detail::prepare_common_epoch(input,r,l,expected,trials,check,true);
}
AltairSlotState prepare_altair_slots(const AltairSlotState& input,const AltairEpochRules& r,
 const AltairStateLimits& l,uint64_t target,uint64_t maximum_slots,uint64_t trials,const std::function<void()>& checkpoint){
 config(r,l);auto state_root=altair_beacon_state_root(input.ssz,l,checkpoint);const Layout layout(l);coherent(input.ssz,layout);
 const auto start=number(input.ssz,40),epoch=start/r.ffg.slots_per_epoch;require(target>start&&number(input.ssz,64)<=start&&(!input.last_processed_epoch||(epoch&&*input.last_processed_epoch==epoch-1)));
 if(target-start>maximum_slots)throw std::length_error("Altair slot work budget exceeded");auto out=input;
 for(uint64_t slot=start;slot<target;++slot){if(checkpoint)checkpoint();if(slot!=start)state_root=altair_beacon_state_root(out.ssz,l,checkpoint);const auto pos=slot%l.slots_per_historical_root;putroot(out.ssz,176+32*l.slots_per_historical_root+32*pos,state_root);if(array<32>(out.ssz,112)==ConsensusRoot{})putroot(out.ssz,112,state_root);
  const ConsensusBeaconHeader h{number(out.ssz,64),number(out.ssz,72),array<32>(out.ssz,80),array<32>(out.ssz,112),array<32>(out.ssz,144)};putroot(out.ssz,176+32*pos,consensus_beacon_header_root(h));
  if((slot+1)%r.ffg.slots_per_epoch==0)out=prepare_altair_epoch(out,r,l,slot/r.ffg.slots_per_epoch,trials,checkpoint);put(out.ssz,40,slot+1);
 }
 altair_beacon_state_root(out.ssz,l,checkpoint);if(checkpoint)checkpoint();return out;
}
}
