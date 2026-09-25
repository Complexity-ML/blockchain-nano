#include "nano/consensus_altair_transition.hpp"
#include "nano/consensus_proposer.hpp"
#include <algorithm>
#include <bit>
namespace nano {
namespace {
using View=std::span<const uint8_t>;
uint64_t number(View b,size_t at,size_t width=8){if(at>b.size()||width>b.size()-at)throw std::invalid_argument("short Altair input");uint64_t n=0;for(size_t j=0;j<width;++j)n|=uint64_t(b[at+j])<<(8*j);return n;}
template<size_t N> std::array<uint8_t,N> array(View b,size_t at){if(at>b.size()||N>b.size()-at)throw std::invalid_argument("short Altair input");std::array<uint8_t,N> a{};std::copy_n(b.begin()+at,N,a.begin());return a;}
void put(Bytes& b,size_t at,uint64_t n,size_t width=8){for(size_t j=0;j<width;++j)b.at(at+j)=uint8_t(n>>(8*j));}
template<size_t N> void put(Bytes& b,size_t at,const std::array<uint8_t,N>& value){if(at>b.size()||N>b.size()-at)throw std::invalid_argument("short Altair output");std::copy(value.begin(),value.end(),b.begin()+at);}
struct Layout {
 size_t historical,randao,slashings,previous;
 std::array<size_t,7> fields;
 explicit Layout(const AltairStateLimits& l):historical(176+64*l.slots_per_historical_root),randao(historical+96),slashings(randao+32*l.epochs_per_historical_vector),previous(slashings+8*l.epochs_per_slashings_vector),fields{historical,historical+76,historical+88,historical+92,previous,previous+4,previous+129}{}
};
ConsensusSlashingState registry(View b,const Layout& l,uint64_t slots,const std::function<void()>& checkpoint){
 ConsensusSlashingState s;s.epoch=number(b,40)/slots;const auto start=number(b,l.fields[2],4),balance=number(b,l.fields[3],4),count=(balance-start)/121;
 if((number(b,l.fields[4],4)-balance)/8!=count||number(b,l.fields[5],4)-number(b,l.fields[4],4)!=count||number(b,l.fields[6],4)-number(b,l.fields[5],4)!=count||(b.size()-number(b,l.fields[6],4))/8!=count)throw std::invalid_argument("Altair state registry cardinality");
 for(size_t at=l.slashings;at<l.previous;at+=8)s.slashings.push_back(number(b,at));
 for(size_t i=0;i<count;++i){if(checkpoint)checkpoint();const auto at=start+121*i;ConsensusSlashingValidator v{};v.public_key=array<48>(b,at);v.withdrawal_credentials=array<32>(b,at+48);v.effective_balance=number(b,at+80);v.slashed=b[at+88];v.activation_eligibility_epoch=number(b,at+89);v.activation_epoch=number(b,at+97);v.exit_epoch=number(b,at+105);v.withdrawable_epoch=number(b,at+113);v.balance=number(b,balance+8*i);s.validators.emplace(i,v);}return s;
}
ConsensusAttestationData data(View b){return {number(b,4),number(b,12),array<32>(b,20),{number(b,52),array<32>(b,60)},{number(b,92),array<32>(b,100)}};}
IndexedConsensusAttestation indexed(View b){IndexedConsensusAttestation a{data(b),{},Bytes(b.begin()+132,b.begin()+228)};for(size_t i=228;i<b.size();i+=8)a.indices.push_back(number(b,i));return a;}
Phase0SignedAttestation attestation(View b){Phase0SignedAttestation a{data(b),{},Bytes(b.begin()+132,b.begin()+228)};const auto bits=(b.size()-229)*8+std::bit_width(unsigned(b.back()))-1;for(size_t i=0;i<bits;++i)a.aggregation_bits.push_back((b[228+i/8]>>(i%8))&1);return a;}
SignedConsensusBeaconHeader header(View b){return {{number(b,0),number(b,8),array<32>(b,16),array<32>(b,48),array<32>(b,80)},Bytes(b.begin()+112,b.begin()+208)};}
Phase0Deposit deposit(View b){Phase0Deposit d;for(size_t i=0;i<33;++i)d.proof[i]=array<32>(b,32*i);d.message={array<48>(b,1056),array<32>(b,1104),number(b,1136)};d.signature=array<96>(b,1144);return d;}
// Called only after the entire body has passed canonical SSZ validation.
std::vector<View> variable_list(View b){std::vector<View> out;if(b.empty())return out;const auto count=number(b,0,4)/4;for(size_t i=0;i<count;++i){const auto start=number(b,4*i,4),end=i+1==count?b.size():number(b,4*(i+1),4);out.push_back(b.subspan(start,end-start));}return out;}
void check_rules(const AltairTransitionRules& r){
 const auto& d=r.attestation.duties;const auto& e=r.epoch;const auto& s=r.sync;
 if(!d.slots_per_epoch||r.maximum_block_bytes<404||!r.state.sync_committee_size||r.state.sync_committee_size>UINT32_MAX||
 r.deposit.validator_registry_limit!=r.state.validator_registry_limit||r.deposit.maximum_effective_balance!=d.maximum_effective_balance||r.deposit.effective_balance_increment!=d.effective_balance_increment||
 e.ffg.slots_per_epoch!=d.slots_per_epoch||e.ffg.effective_balance_increment!=d.effective_balance_increment||e.rewards.base_reward_factor!=r.attestation.base_reward_factor||e.rewards.participation_weights!=r.attestation.participation_weights||e.rewards.weight_denominator!=r.attestation.weight_denominator||
 e.sync.min_seed_lookahead!=d.min_seed_lookahead||e.sync.shuffle_rounds!=d.shuffle_rounds||e.sync.maximum_effective_balance!=d.maximum_effective_balance||
 s.slots_per_epoch!=d.slots_per_epoch||s.effective_balance_increment!=d.effective_balance_increment||s.base_reward_factor!=r.attestation.base_reward_factor||s.proposer_weight!=r.attestation.proposer_weight||s.weight_denominator!=r.attestation.weight_denominator||s.min_seed_lookahead!=d.min_seed_lookahead||s.maximum_effective_balance!=d.maximum_effective_balance||s.shuffle_rounds!=d.shuffle_rounds||
 !s.proposer_weight||s.proposer_weight>=s.weight_denominator||s.weight_denominator%s.proposer_weight||e.slashing.proposer_reward_quotient!=s.weight_denominator/s.proposer_weight||
 e.registry.minimum_churn_limit!=e.slashing.minimum_churn_limit||e.registry.churn_limit_quotient!=e.slashing.churn_limit_quotient||e.registry.max_seed_lookahead!=e.slashing.max_seed_lookahead||e.registry.minimum_withdrawability_delay!=e.slashing.minimum_withdrawability_delay||
 r.exit.minimum_churn_limit!=e.slashing.minimum_churn_limit||r.exit.churn_limit_quotient!=e.slashing.churn_limit_quotient||r.exit.max_seed_lookahead!=e.slashing.max_seed_lookahead||r.exit.minimum_withdrawability_delay!=e.slashing.minimum_withdrawability_delay)
  throw std::invalid_argument("inconsistent Altair block rules");
}
}
void validate_altair_transition_rules(const AltairTransitionRules& r){check_rules(r);validate_altair_epoch_rules(r.epoch,r.state);}
std::optional<AltairSlotState> prepare_altair_state_transition(const AltairSlotState& input,
 View block,const AltairTransitionRules& r,uint64_t maximum_slots,uint64_t maximum_sync_trials,const std::function<void()>& checkpoint){
 check_rules(r);const auto& d=r.attestation.duties;const auto sync_bytes=(r.state.sync_committee_size+7)/8+96;
 if(block.size()<404+sync_bytes||block.size()>r.maximum_block_bytes)return std::nullopt;
 if(number(block,0,4)!=100||number(block,180,4)!=84)return std::nullopt;
 ConsensusRoot body_root;const auto body=block.subspan(184);
 try{body_root=altair_beacon_body_root(body,r.body,r.state.sync_committee_size);}catch(const std::invalid_argument&){return std::nullopt;}
 altair_beacon_state_root(input.ssz,r.state,checkpoint);
 const auto slot=number(block,100);if(slot<=number(input.ssz,40))return std::nullopt;
 auto out=prepare_altair_slots(input,r.epoch,r.state,slot,maximum_slots,maximum_sync_trials,checkpoint);const Layout l(r.state);
 auto reg=registry(out.ssz,l,d.slots_per_epoch,checkpoint);ConsensusRandaoState randao;randao.epoch=reg.epoch;randao.last_reveal_slot=number(out.ssz,64);for(size_t at=l.randao;at<l.slashings;at+=32)randao.mixes.push_back(array<32>(out.ssz,at));
 const ConsensusBeaconHeader previous{number(out.ssz,64),number(out.ssz,72),array<32>(out.ssz,80),array<32>(out.ssz,112),array<32>(out.ssz,144)};
 const ConsensusBeaconHeader h{slot,number(block,108),array<32>(block,116),array<32>(block,148),body_root};
 if(h.parent_root!=consensus_beacon_header_root(previous)||slot<=previous.slot)return std::nullopt;
 const auto proposer=reg.validators.find(h.proposer_index);if(proposer==reg.validators.end()||proposer->second.slashed)return std::nullopt;
 const auto fork_epoch=number(out.ssz,56);auto version=[&](uint64_t epoch){return array<4>(out.ssz,epoch<fork_epoch?48:52);};const auto genesis=array<32>(out.ssz,8);
 if(!verify_consensus_beacon_header(h,version(reg.epoch),genesis,proposer->second.public_key,block.subspan(4,96)))return std::nullopt;
 if(!process_phase0_selected_randao(randao,reg,{d.slots_per_epoch,d.min_seed_lookahead},slot,h.proposer_index,d.maximum_effective_balance,d.shuffle_rounds,version(reg.epoch),genesis,body.first(96),checkpoint))return std::nullopt;
 for(size_t i=0;i<randao.mixes.size();++i)put(out.ssz,l.randao+32*i,randao.mixes[i]);
 put(out.ssz,64,h.slot);put(out.ssz,72,h.proposer_index);put(out.ssz,80,h.parent_root);put(out.ssz,112,ConsensusRoot{});put(out.ssz,144,h.body_root);
 // Append Eth1 vote, then update the canonical root only on a full-period
 // strict majority. Rebuild offsets before passing state to any operation.
 std::array<Bytes,7> lists;for(size_t i=0;i<7;++i){const auto start=number(out.ssz,l.fields[i],4),end=i==6?out.ssz.size():number(out.ssz,l.fields[i+1],4);lists[i]=Bytes(out.ssz.begin()+start,out.ssz.begin()+end);}
 if(lists[1].size()/72>=r.state.eth1_votes_limit)return std::nullopt;const auto vote=body.subspan(96,72);lists[1].insert(lists[1].end(),vote.begin(),vote.end());size_t tally=0;for(size_t at=0;at<lists[1].size();at+=72){if(checkpoint)checkpoint();if(std::equal(vote.begin(),vote.end(),lists[1].begin()+at))++tally;}
 if(tally>r.state.eth1_votes_limit/2)std::copy(vote.begin(),vote.end(),out.ssz.begin()+l.historical+4);
 out.ssz.resize(number(out.ssz,l.fields[0],4));for(size_t i=0;i<7;++i){if(out.ssz.size()>UINT32_MAX||lists[i].size()>UINT32_MAX-out.ssz.size())throw std::overflow_error("Altair block SSZ offset overflow");put(out.ssz,l.fields[i],out.ssz.size(),4);out.ssz.insert(out.ssz.end(),lists[i].begin(),lists[i].end());}
 std::array<View,5> ops;for(size_t i=0;i<5;++i){const auto start=number(body,200+4*i,4),end=i==4?body.size():number(body,204+4*i,4);ops[i]=body.subspan(start,end-start);}
 const auto count=number(out.ssz,l.historical+36),next=number(out.ssz,l.historical+80);
 if(count>(uint64_t(1)<<32)||next>count||ops[3].size()/1240!=std::min<uint64_t>(r.body.deposits,count-next))return std::nullopt;
 const AltairSlashingRules slashing{d,r.epoch.slashing,r.body.validators_per_committee};
 for(size_t i=0;i<ops[0].size();i+=416){if(checkpoint)checkpoint();auto result=prepare_altair_proposer_slashing(out.ssz,header(ops[0].subspan(i,208)),header(ops[0].subspan(i+208,208)),r.state,slashing,checkpoint);if(!result)return std::nullopt;out.ssz=std::move(result->ssz);}
 for(auto op:variable_list(ops[1])){const auto second=number(op,4,4);auto result=prepare_altair_attester_slashing(out.ssz,indexed(op.subspan(8,second-8)),indexed(op.subspan(second)),r.state,slashing,checkpoint);if(!result)return std::nullopt;out.ssz=std::move(result->ssz);}
 for(auto op:variable_list(ops[2])){auto result=prepare_altair_attestation(out.ssz,attestation(op),r.state,r.attestation,checkpoint);if(!result)return std::nullopt;out.ssz=std::move(*result);}
 for(size_t i=0;i<ops[3].size();i+=1240){auto result=prepare_altair_deposit(out.ssz,deposit(ops[3].subspan(i,1240)),r.state,r.deposit,d.slots_per_epoch,checkpoint);if(!result)return std::nullopt;out.ssz=std::move(result->ssz);}
 if(!ops[4].empty()){
  auto exits=registry(out.ssz,l,d.slots_per_epoch,checkpoint);
  for(size_t i=0;i<ops[4].size();i+=112){if(checkpoint)checkpoint();const auto op=ops[4].subspan(i,112);const SignedConsensusVoluntaryExit exit{number(op,0),number(op,8),Bytes(op.begin()+16,op.end())};if(!process_phase0_voluntary_exit(exits,r.exit,exit,version(exit.epoch),genesis))return std::nullopt;}
  const auto start=number(out.ssz,l.fields[2],4);for(const auto& [i,v]:exits.validators){put(out.ssz,start+121*i+105,v.exit_epoch);put(out.ssz,start+121*i+113,v.withdrawable_epoch);}
 }
 auto sync=prepare_altair_sync_aggregate(out.ssz,body.subspan(220,sync_bytes),r.state,r.sync,checkpoint);if(!sync)return std::nullopt;out.ssz=std::move(*sync);
 if(altair_beacon_state_root(out.ssz,r.state,checkpoint)!=h.state_root)return std::nullopt;if(checkpoint)checkpoint();return out;
}
}
