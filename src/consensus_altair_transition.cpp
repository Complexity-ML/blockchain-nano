#include "nano/consensus_altair_transition.hpp"
#include "nano/consensus_bellatrix_transition.hpp"
#include "consensus_bellatrix_projection.hpp"
#include "nano/consensus_capella_transition.hpp"
#include "nano/consensus_capella_bls_change.hpp"
#include "consensus_capella_projection.hpp"
#include <exception>
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
namespace {
// Private construction mode cannot be selected by network admission callers.
enum class BlockPurpose { Receive, Construct };
struct BellatrixContext {
 const BellatrixTransitionRules& rules;
 const Header& parent;
 const State& state;
 const std::function<Bytes(const Int&)>& history;
 const CapellaTransitionRules* capella=nullptr;
};
std::optional<AltairSlotState> transition(const AltairSlotState& input,
 View block,const AltairTransitionRules& r,uint64_t maximum_slots,uint64_t maximum_sync_trials,const std::function<void()>& checkpoint,BlockPurpose purpose,ConsensusRoot& computed_root,const BellatrixContext* bellatrix=nullptr,BellatrixExecutionCandidate* execution_result=nullptr){
 const auto* cr=bellatrix?bellatrix->capella:nullptr;
 check_rules(r);const auto& d=r.attestation.duties;const auto sync_bytes=(r.state.sync_committee_size+7)/8+96;
 if(bellatrix){const auto& br=bellatrix->rules;if(!br.seconds_per_slot||br.chain_id<0||!br.execution.extra_data_bytes||!br.execution.transaction_bytes||!br.execution.transactions||br.execution.transactions>UINT32_MAX/4)throw std::invalid_argument("invalid Bellatrix execution rules");}
 if(cr&&(!cr->maximum_bls_changes||cr->withdrawals.maximum_per_payload>UINT32_MAX/44||cr->genesis_version!=r.deposit.genesis_fork_version||cr->withdrawals.maximum_effective_balance!=d.maximum_effective_balance))throw std::invalid_argument("invalid Capella block rules");
 if(block.size()<404+sync_bytes+(bellatrix?512:0)||block.size()>r.maximum_block_bytes)return std::nullopt;
 if(number(block,0,4)!=100||number(block,180,4)!=84)return std::nullopt;
 ConsensusRoot body_root;const auto body=block.subspan(184);
 std::exception_ptr body_interrupted;
 auto body_checkpoint=[&]{try{if(checkpoint)checkpoint();}catch(...){body_interrupted=std::current_exception();throw;}};
 try{body_root=cr?capella_beacon_body_root(body,r.body,r.state.sync_committee_size,cr->execution,cr->withdrawals.maximum_per_payload,cr->maximum_bls_changes,r.maximum_block_bytes,body_checkpoint):bellatrix?bellatrix_beacon_body_root(body,r.body,r.state.sync_committee_size,bellatrix->rules.execution,r.maximum_block_bytes,body_checkpoint):altair_beacon_body_root(body,r.body,r.state.sync_committee_size);}
 catch(const std::invalid_argument&){if(body_interrupted)std::rethrow_exception(body_interrupted);return std::nullopt;}
 AltairSlotState out;Bytes execution_header,withdrawal_cursors,historical_summaries;std::optional<BellatrixExecutionCandidate> execution;
 const auto slot=number(block,100);const Layout l(r.state);
 if(cr){
  capella_beacon_state_root(input.ssz,r.state,cr->execution,cr->maximum_state_bytes,checkpoint);
  if(slot<=number(input.ssz,40))return std::nullopt;
  auto advanced=prepare_capella_slots({input.ssz,input.last_processed_epoch},{r.epoch,cr->execution,cr->maximum_state_bytes},r.state,slot,maximum_slots,maximum_sync_trials,checkpoint);
  auto projection=capella_detail::project(advanced.ssz,r.state,cr->execution,cr->maximum_state_bytes,checkpoint);
  out={std::move(projection.common),advanced.last_processed_epoch};execution_header=std::move(projection.execution_header);withdrawal_cursors=std::move(projection.cursors);historical_summaries=std::move(projection.summaries);
 }else if(bellatrix){
  const auto& br=bellatrix->rules;
  bellatrix_beacon_state_root(input.ssz,r.state,br.execution,br.maximum_state_bytes,checkpoint);
  if(slot<=number(input.ssz,40))return std::nullopt;
  auto advanced=prepare_bellatrix_slots({input.ssz,input.last_processed_epoch},{r.epoch,br.execution,br.maximum_state_bytes},r.state,slot,maximum_slots,maximum_sync_trials,checkpoint);
  auto projection=bellatrix_detail::project(advanced.ssz,r.state,br.execution,br.maximum_state_bytes,checkpoint);
  out={std::move(projection.common),advanced.last_processed_epoch};execution_header=std::move(projection.execution_header);
 }else{
  altair_beacon_state_root(input.ssz,r.state,checkpoint);
  if(slot<=number(input.ssz,40))return std::nullopt;
  out=prepare_altair_slots(input,r.epoch,r.state,slot,maximum_slots,maximum_sync_trials,checkpoint);
 }
 auto reg=registry(out.ssz,l,d.slots_per_epoch,checkpoint);ConsensusRandaoState randao;randao.epoch=reg.epoch;randao.last_reveal_slot=number(out.ssz,64);for(size_t at=l.randao;at<l.slashings;at+=32)randao.mixes.push_back(array<32>(out.ssz,at));
 const ConsensusBeaconHeader previous{number(out.ssz,64),number(out.ssz,72),array<32>(out.ssz,80),array<32>(out.ssz,112),array<32>(out.ssz,144)};
 const ConsensusBeaconHeader h{slot,number(block,108),array<32>(block,116),array<32>(block,148),body_root};
 if(h.parent_root!=consensus_beacon_header_root(previous)||slot<=previous.slot)return std::nullopt;
 const auto proposer=reg.validators.find(h.proposer_index);if(proposer==reg.validators.end()||proposer->second.slashed)return std::nullopt;
 const auto fork_epoch=number(out.ssz,56);auto version=[&](uint64_t epoch){return array<4>(out.ssz,epoch<fork_epoch?48:52);};const auto genesis=array<32>(out.ssz,8);
 if(purpose==BlockPurpose::Receive&&!verify_consensus_beacon_header(h,version(reg.epoch),genesis,proposer->second.public_key,block.subspan(4,96)))return std::nullopt;
 if(!process_phase0_selected_randao(randao,reg,{d.slots_per_epoch,d.min_seed_lookahead},slot,h.proposer_index,d.maximum_effective_balance,d.shuffle_rounds,version(reg.epoch),genesis,body.first(96),checkpoint))return std::nullopt;
 put(out.ssz,64,h.slot);put(out.ssz,72,h.proposer_index);put(out.ssz,80,h.parent_root);put(out.ssz,112,ConsensusRoot{});put(out.ssz,144,h.body_root);
 // RANDAO was verified on a private candidate above, but its update is applied
 // only after execution has used the previous mix from the authenticated state.
 if(cr){
  auto before_execution=capella_detail::restore(out.ssz,execution_header,withdrawal_cursors,historical_summaries,r.state,cr->execution,cr->maximum_state_bytes,checkpoint);
  const auto authenticated=capella_beacon_state_root(before_execution,r.state,cr->execution,cr->maximum_state_bytes,checkpoint);
  const auto start=number(body,220+sync_bytes,4),end=number(body,224+sync_bytes,4);
  auto paired=prepare_capella_payload_state(before_execution,authenticated,r.state,cr->execution,cr->maximum_state_bytes,d.slots_per_epoch,cr->seconds_per_slot,cr->withdrawals,body.subspan(start,end-start),r.maximum_block_bytes,bellatrix->parent,bellatrix->state,cr->chain_id,cr->replay_policy,bellatrix->history,checkpoint);
  auto projection=capella_detail::project(paired.consensus_state,r.state,cr->execution,cr->maximum_state_bytes,checkpoint);
  out.ssz=std::move(projection.common);execution_header=std::move(projection.execution_header);withdrawal_cursors=std::move(projection.cursors);historical_summaries=std::move(projection.summaries);execution=std::move(paired.execution);
 }else if(bellatrix){
  const auto& br=bellatrix->rules;
  auto before_execution=bellatrix_detail::restore(out.ssz,execution_header,r.state,br.execution,br.maximum_state_bytes,checkpoint);
  const auto authenticated=bellatrix_beacon_state_root(before_execution,r.state,br.execution,br.maximum_state_bytes,checkpoint);
  auto paired=prepare_bellatrix_payload_state(before_execution,authenticated,r.state,br.execution,br.maximum_state_bytes,d.slots_per_epoch,br.seconds_per_slot,body.subspan(number(body,220+sync_bytes,4)),r.maximum_block_bytes,bellatrix->parent,bellatrix->state,br.chain_id,br.replay_policy,bellatrix->history,checkpoint);
  execution_header=bellatrix_detail::project(paired.consensus_state,r.state,br.execution,br.maximum_state_bytes,checkpoint).execution_header;
  execution=std::move(paired.execution);
 }
 for(size_t i=0;i<randao.mixes.size();++i)put(out.ssz,l.randao+32*i,randao.mixes[i]);
 // Append Eth1 vote, then update the canonical root only on a full-period
 // strict majority. Rebuild offsets before passing state to any operation.
 std::array<Bytes,7> lists;for(size_t i=0;i<7;++i){const auto start=number(out.ssz,l.fields[i],4),end=i==6?out.ssz.size():number(out.ssz,l.fields[i+1],4);lists[i]=Bytes(out.ssz.begin()+start,out.ssz.begin()+end);}
 if(lists[1].size()/72>=r.state.eth1_votes_limit)return std::nullopt;const auto vote=body.subspan(96,72);lists[1].insert(lists[1].end(),vote.begin(),vote.end());size_t tally=0;for(size_t at=0;at<lists[1].size();at+=72){if(checkpoint)checkpoint();if(std::equal(vote.begin(),vote.end(),lists[1].begin()+at))++tally;}
 if(tally>r.state.eth1_votes_limit/2)std::copy(vote.begin(),vote.end(),out.ssz.begin()+l.historical+4);
 out.ssz.resize(number(out.ssz,l.fields[0],4));for(size_t i=0;i<7;++i){if(out.ssz.size()>UINT32_MAX||lists[i].size()>UINT32_MAX-out.ssz.size())throw std::overflow_error("Altair block SSZ offset overflow");put(out.ssz,l.fields[i],out.ssz.size(),4);out.ssz.insert(out.ssz.end(),lists[i].begin(),lists[i].end());}
 std::array<View,5> ops;for(size_t i=0;i<5;++i){const auto start=number(body,200+4*i,4),end=i==4?(bellatrix?number(body,220+sync_bytes,4):body.size()):number(body,204+4*i,4);ops[i]=body.subspan(start,end-start);}
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
 if(cr){
  auto full=capella_detail::restore(out.ssz,execution_header,withdrawal_cursors,historical_summaries,r.state,cr->execution,cr->maximum_state_bytes,checkpoint);
  const auto root=capella_beacon_state_root(full,r.state,cr->execution,cr->maximum_state_bytes,checkpoint);
  std::vector<SignedCapellaBlsChange> changes;const auto list=body.subspan(number(body,224+sync_bytes,4));
  for(size_t i=0;i<list.size();i+=172){if(checkpoint)checkpoint();changes.push_back(*decode_capella_bls_change(list.subspan(i,172)));}
  auto changed=prepare_capella_bls_changes(full,root,r.state,cr->execution,cr->maximum_state_bytes,cr->genesis_version,cr->maximum_bls_changes,changes,checkpoint);if(!changed)return std::nullopt;
  out.ssz=capella_detail::project(changed->state,r.state,cr->execution,cr->maximum_state_bytes,checkpoint).common;
 }
 auto sync=prepare_altair_sync_aggregate(out.ssz,body.subspan(220,sync_bytes),r.state,r.sync,checkpoint);if(!sync)return std::nullopt;out.ssz=std::move(*sync);
 if(cr){out.ssz=capella_detail::restore(out.ssz,execution_header,withdrawal_cursors,historical_summaries,r.state,cr->execution,cr->maximum_state_bytes,checkpoint);computed_root=capella_beacon_state_root(out.ssz,r.state,cr->execution,cr->maximum_state_bytes,checkpoint);}
 else if(bellatrix){const auto& br=bellatrix->rules;out.ssz=bellatrix_detail::restore(out.ssz,execution_header,r.state,br.execution,br.maximum_state_bytes,checkpoint);computed_root=bellatrix_beacon_state_root(out.ssz,r.state,br.execution,br.maximum_state_bytes,checkpoint);}
 else computed_root=altair_beacon_state_root(out.ssz,r.state,checkpoint);
 if(purpose==BlockPurpose::Receive&&computed_root!=h.state_root)return std::nullopt;
 if(checkpoint)checkpoint();if(bellatrix)*execution_result=std::move(*execution);return out;
}
} // anonymous namespace
std::optional<AltairSlotState> prepare_altair_state_transition(const AltairSlotState& input,
 View block,const AltairTransitionRules& r,uint64_t maximum_slots,uint64_t maximum_sync_trials,const std::function<void()>& checkpoint){
 ConsensusRoot computed{};
 return transition(input,block,r,maximum_slots,maximum_sync_trials,checkpoint,BlockPurpose::Receive,computed);
}
std::optional<AltairUnsignedBlockCandidate> prepare_altair_unsigned_block(const AltairSlotState& input,
 uint64_t slot,uint64_t proposer,const ConsensusRoot& parent,View body,
 const AltairTransitionRules& r,uint64_t maximum_slots,uint64_t maximum_sync_trials,const std::function<void()>& checkpoint){
 check_rules(r);
 // Bound before allocating/copying a caller's candidate body.
 if(r.maximum_block_bytes<184||body.size()>r.maximum_block_bytes-184)return std::nullopt;
 if(checkpoint)checkpoint();
 Bytes envelope(184,0);put(envelope,0,100,4);put(envelope,100,slot);put(envelope,108,proposer);put(envelope,116,parent);put(envelope,180,84,4);
 envelope.insert(envelope.end(),body.begin(),body.end());ConsensusRoot computed{};
 auto state=transition(input,envelope,r,maximum_slots,maximum_sync_trials,checkpoint,BlockPurpose::Construct,computed);
 if(!state)return std::nullopt;
 put(envelope,148,computed);
 Bytes message(envelope.begin()+100,envelope.end());
 if(checkpoint)checkpoint();
 return AltairUnsignedBlockCandidate{std::move(message),std::move(*state)};
}
std::optional<BellatrixBlockCandidate> prepare_bellatrix_state_transition(
 const BellatrixSlotState& input,View block,const BellatrixTransitionRules& r,
 const Header& parent,const State& state,const std::function<Bytes(const Int&)>& history,
 uint64_t maximum_slots,uint64_t trials,const std::function<void()>& checkpoint){
 if(block.size()<184||block.size()>r.consensus.maximum_block_bytes)return std::nullopt;
 if(input.ssz.size()>r.maximum_state_bytes)throw std::length_error("Bellatrix state budget exceeded");
 const BellatrixContext context{r,parent,state,history};ConsensusRoot computed{};BellatrixExecutionCandidate execution;
 auto out=transition({input.ssz,input.last_processed_epoch},block,r.consensus,maximum_slots,trials,checkpoint,BlockPurpose::Receive,computed,&context,&execution);
 if(!out)return std::nullopt;
 return BellatrixBlockCandidate{{std::move(out->ssz),out->last_processed_epoch},std::move(execution)};
}
std::optional<BellatrixUnsignedBlockCandidate> prepare_bellatrix_unsigned_block(
 const BellatrixSlotState& input,uint64_t slot,uint64_t proposer,const ConsensusRoot& parent_root,View body,
 const BellatrixTransitionRules& r,const Header& parent,const State& state,
 const std::function<Bytes(const Int&)>& history,uint64_t maximum_slots,uint64_t trials,const std::function<void()>& checkpoint){
 check_rules(r.consensus);
 if(input.ssz.size()>r.maximum_state_bytes)throw std::length_error("Bellatrix state budget exceeded");
 if(r.consensus.maximum_block_bytes<184||body.size()>r.consensus.maximum_block_bytes-184)return std::nullopt;
 if(checkpoint)checkpoint();Bytes envelope(184,0);put(envelope,0,100,4);put(envelope,100,slot);put(envelope,108,proposer);put(envelope,116,parent_root);put(envelope,180,84,4);envelope.insert(envelope.end(),body.begin(),body.end());
 const BellatrixContext context{r,parent,state,history};ConsensusRoot computed{};BellatrixExecutionCandidate execution;
 auto out=transition({input.ssz,input.last_processed_epoch},envelope,r.consensus,maximum_slots,trials,checkpoint,BlockPurpose::Construct,computed,&context,&execution);
 if(!out)return std::nullopt;put(envelope,148,computed);
 return BellatrixUnsignedBlockCandidate{Bytes(envelope.begin()+100,envelope.end()),{{std::move(out->ssz),out->last_processed_epoch},std::move(execution)}};
}
std::optional<CapellaBlockCandidate> prepare_capella_state_transition(
 const CapellaSlotState& input,View block,const CapellaTransitionRules& r,
 const Header& parent,const State& state,const std::function<Bytes(const Int&)>& history,
 uint64_t maximum_slots,uint64_t trials,const std::function<void()>& checkpoint){
 if(block.size()<184||block.size()>r.consensus.maximum_block_bytes)return std::nullopt;
 if(input.ssz.size()>r.maximum_state_bytes)throw std::length_error("Bellatrix state budget exceeded");
 const BellatrixTransitionRules common{r.consensus,r.execution,r.maximum_state_bytes,r.seconds_per_slot,r.chain_id,r.replay_policy};
 const BellatrixContext context{common,parent,state,history,&r};ConsensusRoot computed{};BellatrixExecutionCandidate execution;
 auto out=transition({input.ssz,input.last_processed_epoch},block,r.consensus,maximum_slots,trials,checkpoint,BlockPurpose::Receive,computed,&context,&execution);
 if(!out)return std::nullopt;
 return CapellaBlockCandidate{{std::move(out->ssz),out->last_processed_epoch},std::move(execution)};
}
std::optional<CapellaUnsignedBlockCandidate> prepare_capella_unsigned_block(
 const CapellaSlotState& input,uint64_t slot,uint64_t proposer,const ConsensusRoot& parent_root,View body,
 const CapellaTransitionRules& r,const Header& parent,const State& state,
 const std::function<Bytes(const Int&)>& history,uint64_t maximum_slots,uint64_t trials,const std::function<void()>& checkpoint){
 check_rules(r.consensus);
 if(input.ssz.size()>r.maximum_state_bytes)throw std::length_error("Bellatrix state budget exceeded");
 if(r.consensus.maximum_block_bytes<184||body.size()>r.consensus.maximum_block_bytes-184)return std::nullopt;
 if(checkpoint)checkpoint();Bytes envelope(184,0);put(envelope,0,100,4);put(envelope,100,slot);put(envelope,108,proposer);put(envelope,116,parent_root);put(envelope,180,84,4);envelope.insert(envelope.end(),body.begin(),body.end());
 const BellatrixTransitionRules common{r.consensus,r.execution,r.maximum_state_bytes,r.seconds_per_slot,r.chain_id,r.replay_policy};
 const BellatrixContext context{common,parent,state,history,&r};ConsensusRoot computed{};BellatrixExecutionCandidate execution;
 auto out=transition({input.ssz,input.last_processed_epoch},envelope,r.consensus,maximum_slots,trials,checkpoint,BlockPurpose::Construct,computed,&context,&execution);
 if(!out)return std::nullopt;put(envelope,148,computed);
 return CapellaUnsignedBlockCandidate{Bytes(envelope.begin()+100,envelope.end()),{{std::move(out->ssz),out->last_processed_epoch},std::move(execution)}};
}

}
