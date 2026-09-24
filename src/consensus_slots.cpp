#include "nano/consensus_slots.hpp"
#include "nano/consensus_transition.hpp"
#include "nano/consensus_beacon_header.hpp"
#include <bit>
#include <stdexcept>
namespace nano {
namespace {
void require(bool v){if(!v)throw std::invalid_argument("invalid Phase0 slot state");}
uint64_t number(const Bytes& b,size_t at,size_t width=8){require(width<=8&&at<=b.size()&&width<=b.size()-at);uint64_t n=0;for(size_t i=0;i<width;++i)n|=uint64_t(b[at+i])<<(8*i);return n;}
void put(Bytes& b,size_t at,uint64_t n,size_t width=8){require(at<=b.size()&&width<=b.size()-at&&width<=8);if(width<8)require(n<(uint64_t(1)<<(width*8)));for(size_t i=0;i<width;++i)b[at+i]=uint8_t(n>>(8*i));}
template<size_t N> std::array<uint8_t,N> array(const Bytes& b,size_t at){require(at<=b.size()&&N<=b.size()-at);std::array<uint8_t,N> a{};std::copy_n(b.begin()+at,N,a.begin());return a;}
template<size_t N> void put(Bytes& b,size_t at,const std::array<uint8_t,N>& a){require(at<=b.size()&&N<=b.size()-at);std::copy(a.begin(),a.end(),b.begin()+at);}
void append(Bytes& a,const Bytes& b){require(b.size()<=UINT32_MAX&&a.size()<=UINT32_MAX-b.size());a.insert(a.end(),b.begin(),b.end());}
struct Layout {
 size_t blocks=176,states,historical,eth1,votes,deposit,validators,balances,randao,slashings,previous,current,bits,checkpoints,fixed;
 explicit Layout(const Phase0StateLimits& l){
  require(l.slots_per_historical_root<=UINT32_MAX/64&&l.epochs_per_historical_vector<=UINT32_MAX/32&&l.epochs_per_slashings_vector<=UINT32_MAX/8);
  states=blocks+32*l.slots_per_historical_root;historical=states+32*l.slots_per_historical_root;
  eth1=historical+4;votes=eth1+72;deposit=votes+4;validators=deposit+8;balances=validators+4;randao=balances+4;
  slashings=randao+32*l.epochs_per_historical_vector;previous=slashings+8*l.epochs_per_slashings_vector;current=previous+4;bits=current+4;checkpoints=bits+1;fixed=checkpoints+120;require(fixed<=UINT32_MAX);
 }
 std::array<size_t,6> offsets()const{return {historical,votes,validators,balances,previous,current};}
};
ConsensusCheckpoint cp(const Bytes& b,size_t at){return {number(b,at),array<32>(b,at+8)};}
void putcp(Bytes& b,size_t at,const ConsensusCheckpoint& c){put(b,at,c.epoch);put(b,at+8,c.root);}
std::vector<Phase0PendingAttestation> pending(const Bytes& b,size_t start,size_t end){
 std::vector<Phase0PendingAttestation> out;if(start==end)return out;
 const auto count=number(b,start,4)/4;
 for(size_t i=0;i<count;++i){
  const auto at=start+number(b,start+4*i,4),stop=i+1==count?end:start+number(b,start+4*(i+1),4);
  require(stop>at+148);Phase0PendingAttestation v;auto& d=v.data;
  d.slot=number(b,at+4);d.index=number(b,at+12);d.beacon_block_root=array<32>(b,at+20);d.source=cp(b,at+52);d.target=cp(b,at+92);
  v.inclusion_delay=number(b,at+132);v.proposer_index=number(b,at+140);
  const auto count_bits=(stop-at-149)*8+std::bit_width(unsigned(b[stop-1]))-1;
  for(size_t j=0;j<count_bits;++j)v.aggregation_bits.push_back(bool((b[at+148+j/8]>>(j%8))&1));out.push_back(std::move(v));
 }return out;
}
Bytes encode_pending(const std::vector<Phase0PendingAttestation>& votes){
 require(votes.size()<=UINT32_MAX/4);Bytes result(4*votes.size());
 for(size_t i=0;i<votes.size();++i){put(result,4*i,result.size(),4);const auto& v=votes[i];const auto& d=v.data;Bytes b(148+v.aggregation_bits.size()/8+1);
  put(b,0,148,4);put(b,4,d.slot);put(b,12,d.index);put(b,20,d.beacon_block_root);putcp(b,52,d.source);putcp(b,92,d.target);put(b,132,v.inclusion_delay);put(b,140,v.proposer_index);
  for(size_t j=0;j<v.aggregation_bits.size();++j)if(v.aggregation_bits[j])b[148+j/8]|=uint8_t(1U<<(j%8));b.back()|=uint8_t(1U<<(v.aggregation_bits.size()%8));append(result,b);
 }return result;
}
Phase0EpochState decode_epoch(const Phase0SlotState& input,const Layout& l,const Phase0EpochRules& rules){
 const auto& b=input.ssz;const auto h=number(b,l.historical,4),votes=number(b,l.votes,4),validators=number(b,l.validators,4),balances=number(b,l.balances,4),previous=number(b,l.previous,4),current=number(b,l.current,4);
 require((balances-validators)/121==(previous-balances)/8);Phase0EpochState out;out.ffg.slot=number(b,40);out.registry.epoch=out.ffg.slot/rules.history.slots_per_epoch;out.randao.epoch=out.registry.epoch;out.randao.last_reveal_slot=number(b,64);out.last_processed_epoch=input.last_processed_epoch;
 for(size_t i=0;i<rules.history.slots_per_historical_root;++i){out.ffg.block_roots.push_back(array<32>(b,l.blocks+32*i));out.state_roots.push_back(array<32>(b,l.states+32*i));}
 for(size_t i=h;i<votes;i+=32)out.historical_roots.push_back(array<32>(b,i));
 for(size_t i=votes;i<validators;i+=72)out.eth1_votes.push_back({array<32>(b,i),number(b,i+32),array<32>(b,i+40)});
 for(size_t i=0;i<(balances-validators)/121;++i){const auto at=validators+i*121;ConsensusSlashingValidator v;v.public_key=array<48>(b,at);v.withdrawal_credentials=array<32>(b,at+48);v.effective_balance=number(b,at+80);v.slashed=b[at+88];v.activation_eligibility_epoch=number(b,at+89);v.activation_epoch=number(b,at+97);v.exit_epoch=number(b,at+105);v.withdrawable_epoch=number(b,at+113);v.balance=number(b,balances+i*8);out.registry.validators.emplace(i,v);}
 for(size_t i=0;i<rules.epochs_per_randao_vector;++i)out.randao.mixes.push_back(array<32>(b,l.randao+32*i));
 for(size_t i=0;i<rules.slashing.epochs_per_slashings_vector;++i)out.registry.slashings.push_back(number(b,l.slashings+8*i));
 out.ffg.previous_attestations=pending(b,previous,current);out.ffg.current_attestations=pending(b,current,b.size());out.ffg.checkpoints={b[l.bits],cp(b,l.checkpoints),cp(b,l.checkpoints+40),cp(b,l.checkpoints+80)};return out;
}
Bytes encode_epoch(const Bytes& original,const Phase0EpochState& state,const Layout& l){
 Bytes b(original.begin(),original.begin()+l.fixed);put(b,40,state.ffg.slot);
 for(size_t i=0;i<state.randao.mixes.size();++i)put(b,l.randao+32*i,state.randao.mixes[i]);for(size_t i=0;i<state.registry.slashings.size();++i)put(b,l.slashings+8*i,state.registry.slashings[i]);
 b[l.bits]=state.ffg.checkpoints.justification_bits;putcp(b,l.checkpoints,state.ffg.checkpoints.previous_justified);putcp(b,l.checkpoints+40,state.ffg.checkpoints.current_justified);putcp(b,l.checkpoints+80,state.ffg.checkpoints.finalized);
 std::array<Bytes,6> lists;
 for(const auto& root:state.historical_roots)append(lists[0],Bytes(root.begin(),root.end()));
 for(const auto& v:state.eth1_votes){Bytes x(72);put(x,0,v.deposit_root);put(x,32,v.deposit_count);put(x,40,v.block_hash);append(lists[1],x);}
 for(const auto& [index,v]:state.registry.validators){require(index==lists[2].size()/121);Bytes x(121);put(x,0,v.public_key);put(x,48,v.withdrawal_credentials);put(x,80,v.effective_balance);x[88]=v.slashed;put(x,89,v.activation_eligibility_epoch);put(x,97,v.activation_epoch);put(x,105,v.exit_epoch);put(x,113,v.withdrawable_epoch);append(lists[2],x);Bytes balance(8);put(balance,0,v.balance);append(lists[3],balance);}
 lists[4]=encode_pending(state.ffg.previous_attestations);lists[5]=encode_pending(state.ffg.current_attestations);
 const auto offsets=l.offsets();for(size_t i=0;i<lists.size();++i){put(b,offsets[i],b.size(),4);append(b,lists[i]);}return b;
}
}
Phase0SlotState prepare_phase0_slots(const Phase0SlotState& input,const Phase0EpochRules& rules,
 const Phase0StateLimits& limits,uint64_t target,uint64_t maximum_slots,const std::function<void()>& checkpoint){
 require(rules.history.slots_per_epoch&&limits.slots_per_historical_root==rules.history.slots_per_historical_root&&limits.epochs_per_historical_vector==rules.epochs_per_randao_vector&&limits.epochs_per_slashings_vector==rules.slashing.epochs_per_slashings_vector&&limits.historical_roots_limit==rules.history.historical_roots_limit&&limits.pending_attestations_limit==rules.maximum_pending_attestations&&rules.epochs_per_eth1_voting_period<=UINT64_MAX/rules.history.slots_per_epoch&&limits.eth1_votes_limit==rules.epochs_per_eth1_voting_period*rules.history.slots_per_epoch);
 const Layout l(limits);auto root=phase0_beacon_state_root(input.ssz,limits,checkpoint);const auto start=number(input.ssz,40),epoch=start/rules.history.slots_per_epoch;
 require(target>start&&number(input.ssz,64)<=start&&(!input.last_processed_epoch||(epoch>0&&*input.last_processed_epoch==epoch-1)));
 if(target-start>maximum_slots)throw std::length_error("Phase0 slot work budget exceeded");
 require((number(input.ssz,l.balances,4)-number(input.ssz,l.validators,4))/121==(number(input.ssz,l.previous,4)-number(input.ssz,l.balances,4))/8);
 auto out=input;
 for(uint64_t slot=start;slot<target;++slot){
  if(checkpoint)checkpoint();
  if(slot!=start)root=phase0_beacon_state_root(out.ssz,limits,checkpoint);
  const auto position=slot%limits.slots_per_historical_root;put(out.ssz,l.states+32*position,root);
  if(array<32>(out.ssz,112)==ConsensusRoot{})put(out.ssz,112,root);
  const ConsensusBeaconHeader header{number(out.ssz,64),number(out.ssz,72),array<32>(out.ssz,80),array<32>(out.ssz,112),array<32>(out.ssz,144)};
  put(out.ssz,l.blocks+32*position,consensus_beacon_header_root(header));
  if((slot+1)%rules.history.slots_per_epoch==0){
   const auto state=decode_epoch(out,l,rules);const auto next=prepare_phase0_epoch(state,rules,slot/rules.history.slots_per_epoch,checkpoint);
   out.ssz=encode_epoch(out.ssz,next,l);out.last_processed_epoch=next.last_processed_epoch;
  }
  put(out.ssz,40,slot+1);
 }
 phase0_beacon_state_root(out.ssz,limits,checkpoint);if(checkpoint)checkpoint();return out;
}
std::optional<Phase0SlotState> prepare_phase0_state_transition(const Phase0SlotState& input,
 std::span<const uint8_t> signed_block,const Phase0TransitionRules& rules,uint64_t maximum_slots,
 const std::function<void()>& checkpoint){
 const auto& duties=rules.epoch.rewards.duties;
 require(rules.maximum_block_bytes>=404&&duties.slots_per_epoch&&
  rules.deposit.validator_registry_limit==rules.state.validator_registry_limit&&
  rules.deposit.maximum_effective_balance==duties.maximum_effective_balance&&
  rules.deposit.effective_balance_increment==duties.effective_balance_increment&&
  rules.body.validators_per_committee==rules.state.validators_per_committee&&
  rules.body.attestations&&duties.slots_per_epoch<=UINT64_MAX/rules.body.attestations&&
  rules.epoch.maximum_pending_attestations==duties.slots_per_epoch*rules.body.attestations);
 if(signed_block.size()<404||signed_block.size()>rules.maximum_block_bytes)return std::nullopt;
 const Bytes block(signed_block.begin(),signed_block.end());
 if(number(block,0,4)!=100||number(block,180,4)!=84)return std::nullopt;
 try{phase0_beacon_body_root(signed_block.subspan(184),rules.body);}catch(const std::invalid_argument&){return std::nullopt;}
 const auto slot=number(block,100);
 if(slot<=number(input.ssz,40))return std::nullopt;
 auto out=prepare_phase0_slots(input,rules.epoch,rules.state,slot,maximum_slots,checkpoint);
 const Layout l(rules.state);auto state=decode_epoch(out,l,rules.epoch);
 const ConsensusBeaconHeader previous{number(out.ssz,64),number(out.ssz,72),array<32>(out.ssz,80),array<32>(out.ssz,112),array<32>(out.ssz,144)};
 const Phase0StateFork fork{array<4>(out.ssz,48),array<4>(out.ssz,52),number(out.ssz,56)};
 const Phase0BlockContext context{slot,consensus_beacon_header_root(previous),state.registry.epoch<fork.epoch?fork.previous_version:fork.current_version,array<32>(out.ssz,8),duties.maximum_effective_balance,duties.shuffle_rounds,rules.maximum_block_bytes};
 const ConsensusRandaoRules randao_rules{duties.slots_per_epoch,duties.min_seed_lookahead};
 const auto auth=authenticate_phase0_block(signed_block,context,rules.body,randao_rules,state.registry,state.randao,checkpoint);
 if(!auth)return std::nullopt;
 // Eth1Data includes all three fields. A strict majority of the complete voting
 // period is required, not a majority of the votes seen so far.
 const Phase0Eth1Data vote{array<32>(block,280),number(block,312),array<32>(block,320)};
 if(state.eth1_votes.size()>=rules.state.eth1_votes_limit)return std::nullopt;
 state.eth1_votes.push_back(vote);uint64_t tally=0;
 for(const auto& v:state.eth1_votes){if(checkpoint)checkpoint();if(v==vote)++tally;}
 Phase0Eth1Data eth1{array<32>(out.ssz,l.eth1),number(out.ssz,l.eth1+32),array<32>(out.ssz,l.eth1+40)};
 if(tally>rules.state.eth1_votes_limit/2)eth1=vote;
 const Phase0DepositState deposits{state.registry,eth1.deposit_root,eth1.deposit_count,number(out.ssz,l.deposit)};
 if(deposits.next_deposit_index>deposits.deposit_count||deposits.deposit_count>(uint64_t(1)<<32))return std::nullopt;
 const Phase0AttestationInclusionRules inclusion{duties,rules.minimum_attestation_inclusion_delay,rules.epoch.maximum_pending_attestations};
 const Phase0AttestationStage stage{state.ffg,inclusion};
 const auto operations=prepare_phase0_registry_operations(signed_block,context,rules.body,randao_rules,rules.epoch.slashing,rules.deposit,rules.exit,fork,deposits,state.randao,checkpoint,&stage);
 if(!std::holds_alternative<Phase0RegistryOperationsCandidate>(operations))return std::nullopt;
 const auto& result=std::get<Phase0RegistryOperationsCandidate>(operations);
 state.registry=result.deposits_after.registry;state.randao=result.authenticated.randao_after;state.ffg=*result.attestations_after;
 put(out.ssz,l.eth1,eth1.deposit_root);put(out.ssz,l.eth1+32,eth1.deposit_count);put(out.ssz,l.eth1+40,eth1.block_hash);put(out.ssz,l.deposit,result.deposits_after.next_deposit_index);
 const auto& header=result.authenticated.header;
 put(out.ssz,64,header.slot);put(out.ssz,72,header.proposer_index);put(out.ssz,80,header.parent_root);put(out.ssz,112,ConsensusRoot{});put(out.ssz,144,header.body_root);
 out.ssz=encode_epoch(out.ssz,state,l);
 if(phase0_beacon_state_root(out.ssz,rules.state,checkpoint)!=header.state_root)return std::nullopt;
 if(checkpoint)checkpoint();return out;
}

}
