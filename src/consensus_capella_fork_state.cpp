#include "nano/consensus_capella_fork_state.hpp"
#include <algorithm>
#include "consensus_capella_projection.hpp"
#include "nano/consensus_proposer.hpp"
namespace nano {
namespace {
void require(bool ok){if(!ok)throw std::invalid_argument("invalid Capella fork state binding");}
uint64_t number(const Bytes& b,size_t at,size_t width=8){require(at<=b.size()&&width<=b.size()-at);uint64_t n=0;for(size_t j=0;j<width;++j)n|=uint64_t(b[at+j])<<(8*j);return n;}
template<size_t N>std::array<uint8_t,N> array(const Bytes& b,size_t at){require(at<=b.size()&&N<=b.size()-at);std::array<uint8_t,N> a{};std::copy_n(b.begin()+at,N,a.begin());return a;}
struct Projection {ConsensusSlashingState registry;ConsensusRandaoState randao;Phase0FfgCheckpoints checkpoints;};
Projection project(const Bytes& b,const CapellaEpochRules& r,const AltairStateLimits& l,const std::function<void()>& checkpoint){
 require(r.operations.ffg.slots_per_epoch);capella_beacon_state_root(b,l,r.execution,r.maximum_state_bytes,checkpoint);
 const size_t historical=176+64*l.slots_per_historical_root,randao_at=historical+96,slashings_at=randao_at+32*l.epochs_per_historical_vector,prev=slashings_at+8*l.epochs_per_slashings_vector;
 const auto validators=number(b,historical+88,4),balances=number(b,historical+92,4),previous=number(b,prev,4),current=number(b,prev+4,4),scores=number(b,prev+129,4),count=(balances-validators)/121;
 require((previous-balances)/8==count&&current-previous==count&&scores-current==count&&(number(b,prev+229+96*l.sync_committee_size,4)-scores)/8==count);
 Projection out;out.registry.epoch=number(b,40)/r.operations.ffg.slots_per_epoch;out.randao.epoch=out.registry.epoch;out.randao.last_reveal_slot=number(b,64);
 for(size_t i=0;i<l.epochs_per_historical_vector;++i)out.randao.mixes.push_back(array<32>(b,randao_at+32*i));for(size_t i=0;i<l.epochs_per_slashings_vector;++i)out.registry.slashings.push_back(number(b,slashings_at+8*i));
 for(size_t i=0;i<count;++i){if(checkpoint)checkpoint();const auto at=validators+121*i;ConsensusSlashingValidator v{};v.public_key=array<48>(b,at);v.withdrawal_credentials=array<32>(b,at+48);v.effective_balance=number(b,at+80);v.slashed=b[at+88];v.activation_eligibility_epoch=number(b,at+89);v.activation_epoch=number(b,at+97);v.exit_epoch=number(b,at+105);v.withdrawable_epoch=number(b,at+113);v.balance=number(b,balances+8*i);out.registry.validators.emplace(i,v);}
 auto cp=[&](size_t at){return ConsensusCheckpoint{number(b,at),array<32>(b,at+8)};};out.checkpoints={b[prev+8],cp(prev+9),cp(prev+49),cp(prev+89)};return out;
}
}
Phase0ForkSlashingContext capella_fork_block_context(const CapellaSlotState& input,const ConsensusBeaconHeader& h,
 const CapellaEpochRules& r,const AltairStateLimits& l,const std::function<void()>& checkpoint){
 require(r.operations.ffg.slots_per_epoch&&capella_beacon_state_root(input.ssz,l,r.execution,r.maximum_state_bytes,checkpoint)==h.state_root&&number(input.ssz,40)==h.slot);const auto epoch=h.slot/r.operations.ffg.slots_per_epoch;
 require(epoch?input.last_processed_epoch&&*input.last_processed_epoch==epoch-1:!input.last_processed_epoch);
 const auto& b=input.ssz;const ConsensusBeaconHeader stored{number(b,64),number(b,72),array<32>(b,80),h.state_root,array<32>(b,144)};require(array<32>(b,112)==ConsensusRoot{}&&stored==h);
 const Phase0StateFork fork{array<4>(b,48),array<4>(b,52),number(b,56)};require(fork.epoch<=epoch);auto p=project(b,r,l,checkpoint);return {consensus_beacon_header_root(h),std::move(p.registry),fork,array<32>(b,8)};
}
Phase0ForkCheckpoints capella_fork_checkpoints(const CapellaSlotState& input,const CapellaEpochRules& r,
 const AltairStateLimits& l,const std::function<void()>& checkpoint){
 const auto realized=project(input.ssz,r,l,checkpoint).checkpoints;const auto projection=capella_detail::project(input.ssz,l,r.execution,r.maximum_state_bytes,checkpoint);const auto common=prepare_altair_justification_finalization(projection.common,l,r.operations.ffg,checkpoint);const auto pulled=capella_detail::restore(common,projection.execution_header,projection.cursors,projection.summaries,l,r.execution,r.maximum_state_bytes,checkpoint);return {realized,project(pulled,r,l,checkpoint).checkpoints};
}
CapellaForkStateContexts prepare_capella_fork_state_contexts(const CapellaSlotState& input,const ConsensusBeaconHeader& h,
 const ConsensusCheckpoint& target,const CapellaEpochRules& r,const AltairStateLimits& l,
 uint64_t maximum_slots,uint64_t maximum_sync_trials,const std::function<void()>& checkpoint){
 require(r.operations.ffg.slots_per_epoch&&target.epoch<=UINT64_MAX/r.operations.ffg.slots_per_epoch);const auto slot=target.epoch*r.operations.ffg.slots_per_epoch;require(h.slot<=slot&&consensus_beacon_header_root(h)==target.root);
 auto block=capella_fork_block_context(input,h,r,l,checkpoint);auto state=h.slot==slot?input:prepare_capella_slots(input,r,l,slot,maximum_slots,maximum_sync_trials,checkpoint);auto p=project(state.ssz,r,l,checkpoint);require(p.registry.epoch==target.epoch);
 Phase0ForkVoteContext vote{target,std::move(p.registry),std::move(p.randao),block.fork,block.genesis_root};if(checkpoint)checkpoint();return {std::move(block),std::move(vote),std::move(state)};
}
CapellaSlotPreparation::CapellaSlotPreparation(const CapellaSlotState& input,
 const ConsensusBeaconHeader& parent,const CapellaEpochRules& rules,
 const AltairStateLimits& limits,const std::function<void()>& checkpoint)
 :parent_(parent),rules_(rules),limits_(limits),state_(input){
 capella_fork_block_context(input,parent,rules,limits,checkpoint);
}
uint64_t CapellaSlotPreparation::slot()const{return number(state_.ssz,40);}
bool CapellaSlotPreparation::advance(uint64_t target,uint64_t maximum_slots,
 uint64_t trials,const std::function<void()>& checkpoint){
 const auto start=slot();
 if(!maximum_slots||!trials||target<start)
  throw std::invalid_argument("invalid Capella slot continuation bounds");
 if(checkpoint)checkpoint();
 if(target==start)return true;
 // Subtraction first keeps the addition safe even for UINT64_MAX targets.
 const auto next=start+std::min(maximum_slots,target-start);
 auto prepared=prepare_capella_slots(state_,rules_,limits_,next,maximum_slots,trials,checkpoint);
 if(checkpoint)checkpoint();
 state_=std::move(prepared);
 return next==target;
}
namespace {
std::optional<CapellaProposerIdentity> derive_proposer_identity(const CapellaSlotState& advanced,
 const CapellaEpochRules& r,const AltairStateLimits& l,const Phase0FfgRules& d,
 const ConsensusRoot& genesis_root,const std::function<void()>& checkpoint){
 require(d.slots_per_epoch==r.operations.ffg.slots_per_epoch);
 const auto slot=number(advanced.ssz,40);
 const auto p=project(advanced.ssz,r,l,checkpoint);
 std::vector<Phase0ProposerValidator> candidates;candidates.reserve(p.registry.validators.size());
 for(const auto& [index,v]:p.registry.validators){if(checkpoint)checkpoint();candidates.push_back({index,v.effective_balance,v.activation_epoch,v.exit_epoch});}
 const auto index=phase0_proposer_index(candidates,p.randao,{d.slots_per_epoch,d.min_seed_lookahead},slot,d.maximum_effective_balance,d.shuffle_rounds,checkpoint);
 const auto& selected=p.registry.validators.at(index);if(selected.slashed)return std::nullopt;
 const auto epoch=slot/d.slots_per_epoch;const auto version=array<4>(advanced.ssz,epoch<number(advanced.ssz,56)?48:52);
 if(checkpoint)checkpoint();
 return CapellaProposerIdentity{index,version,genesis_root,Bytes(selected.public_key.begin(),selected.public_key.end())};
}
}
std::optional<CapellaProposerIdentity> CapellaSlotPreparation::proposer_identity(
 const Phase0FfgRules& duties,const std::function<void()>& checkpoint)const{
 require(duties.slots_per_epoch==rules_.operations.ffg.slots_per_epoch);
 if(slot()<=parent_.slot)return std::nullopt;
 return derive_proposer_identity(state_,rules_,limits_,duties,array<32>(state_.ssz,8),checkpoint);
}
Phase0ForkVoteContext CapellaSlotPreparation::vote_context(const ConsensusCheckpoint& target,
 const std::function<void()>& checkpoint)const{
 const auto slots=rules_.operations.ffg.slots_per_epoch;
 require(slots&&target.epoch<=UINT64_MAX/slots&&target.epoch*slots==slot()&&
  parent_.slot<=slot()&&target.root==consensus_beacon_header_root(parent_));
 auto p=project(state_.ssz,rules_,limits_,checkpoint);require(p.registry.epoch==target.epoch);
 const Phase0StateFork fork{array<4>(state_.ssz,48),array<4>(state_.ssz,52),number(state_.ssz,56)};
 require(fork.epoch<=target.epoch);
 if(checkpoint)checkpoint();
 return {target,std::move(p.registry),std::move(p.randao),fork,array<32>(state_.ssz,8)};
}
std::optional<CapellaProposerIdentity> prepare_capella_proposer_identity(
 const CapellaSlotState& input,const ConsensusBeaconHeader& h,uint64_t slot,
 const CapellaEpochRules& r,const AltairStateLimits& l,const Phase0FfgRules& d,
 uint64_t maximum_slots,uint64_t maximum_sync_trials,const std::function<void()>& checkpoint){
 require(d.slots_per_epoch==r.operations.ffg.slots_per_epoch);
 const auto parent=capella_fork_block_context(input,h,r,l,checkpoint);
 if(slot<=h.slot)return std::nullopt;
 const auto advanced=prepare_capella_slots(input,r,l,slot,maximum_slots,maximum_sync_trials,checkpoint);
 return derive_proposer_identity(advanced,r,l,d,parent.genesis_root,checkpoint);
}

}
