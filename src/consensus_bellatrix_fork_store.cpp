#include "nano/consensus_bellatrix_fork_store.hpp"
#include "consensus_bellatrix_projection.hpp"
#include <exception>
namespace nano {
namespace {
uint64_t number(std::span<const uint8_t> b,size_t at,size_t width=8){uint64_t v=0;for(size_t i=0;i<width;++i)v|=uint64_t(b[at+i])<<(8*i);return v;}
ConsensusRoot root(std::span<const uint8_t> b,size_t at){ConsensusRoot r{};std::copy_n(b.begin()+at,32,r.begin());return r;}
BellatrixEpochRules epoch_rules(const BellatrixForkBlockRules& r){return {r.transition.consensus.epoch,r.transition.execution,r.transition.maximum_state_bytes};}
void validate_rules(const BellatrixForkBlockRules& r){
 validate_altair_transition_rules(r.transition.consensus);
 if(!r.transition.seconds_per_slot||r.transition.seconds_per_slot>UINT64_MAX/1000||r.slot_duration_ms!=r.transition.seconds_per_slot*1000||r.attestation_due_ms>r.slot_duration_ms)
  throw std::invalid_argument("inconsistent Bellatrix fork clock rules");
}
template<class A> Bytes as_bytes(const A& a){return {a.begin(),a.end()};}
void validate_execution(const BellatrixForkBlock& record,const BellatrixForkBlockRules& rules,const std::function<void()>& check){
 const auto& l=rules.transition.consensus.state;const auto& e=rules.transition.execution;const auto& b=record.state.ssz;
 bellatrix_beacon_state_root(b,l,e,rules.transition.maximum_state_bytes,check);
 const size_t start=bellatrix_detail::offset(b,bellatrix_detail::execution_field(l));
 const auto latest=decode_bellatrix_execution_header(std::span<const uint8_t>(b).subspan(start),e.extra_data_bytes,rules.transition.maximum_state_bytes,check);
 const auto& block=record.execution_block;const auto& h=block.header;const auto hash=h.hash();
 if(!latest||hash!=as_bytes(latest->block_hash)||h.state_root!=record.execution_state.root()||h.state_root!=as_bytes(latest->state_root)||
 h.parent_hash!=as_bytes(latest->parent_hash)||unhex(address(h.coinbase))!=as_bytes(latest->fee_recipient)||h.receipts_root!=as_bytes(latest->receipts_root)||h.logs_bloom!=as_bytes(latest->logs_bloom)||h.mix_hash!=as_bytes(latest->prev_randao)||
 h.number!=Int(std::to_string(latest->block_number))||h.timestamp!=Int(std::to_string(latest->timestamp))||h.gas_limit!=Int(std::to_string(latest->gas_limit))||h.gas_used!=Int(std::to_string(latest->gas_used))||h.extra_data!=latest->extra_data||h.base_fee!=std::optional<Int>(integer(Bytes(latest->base_fee_per_gas.rbegin(),latest->base_fee_per_gas.rend())))||
 h.difficulty!=0||h.nonce!=Bytes(8)||h.uncle_hash!=keccak(Rlp(std::vector<Rlp>{}).encode())||h.withdrawals_root||h.blob_gas_used||h.excess_blob_gas||h.parent_beacon_block_root||h.requests_hash||!block.uncles.empty()||!block.withdrawals.empty()||!block.producer_signature.empty()||
 h.transactions_root!=transactions_root(block.transactions)||block.transactions.size()!=record.receipts.size()||h.receipts_root!=receipts_root(record.receipts))throw std::invalid_argument("Bellatrix fork execution binding mismatch");
 BellatrixExecutionPayload p;static_cast<BellatrixExecutionFields&>(p)=*latest;
 Int cumulative=0;std::vector<Log> logs;
 for(size_t i=0;i<block.transactions.size();++i){if(check)check();p.transactions.push_back(block.transactions[i].raw());const auto& receipt=record.receipts[i];cumulative+=receipt.gas_used;logs.insert(logs.end(),receipt.logs.begin(),receipt.logs.end());if(receipt.gas_used<0||receipt.cumulative_gas_used!=cumulative||receipt.transaction_hash!=block.transactions[i].hash()||receipt.block_hash!=hash||receipt.block_number!=h.number||receipt.index!=Int(i))throw std::invalid_argument("Bellatrix fork receipt binding mismatch");}
 if(cumulative!=h.gas_used||logs_bloom(logs)!=h.logs_bloom||bellatrix_execution_payload_header(p,e,check)!=*latest)throw std::invalid_argument("Bellatrix fork payload binding mismatch");
}
Bytes branch_hash(const BellatrixForkStore& store,ConsensusRoot id,const Int& height,const std::function<void()>& check){
 if(height<0)throw std::invalid_argument("negative execution history height");
 for(;;){if(check)check();const auto& block=store.blocks.at(id);const auto& h=block.execution_block.header;
  if(height==h.number)return h.hash();if(height>h.number)throw std::invalid_argument("future execution history height");
  if(id==store.timed.view.anchor){for(const auto& ancestor:store.anchor_execution_ancestors){if(check)check();if(ancestor.number==height)return ancestor.hash();}throw std::invalid_argument("execution history unavailable before anchor");}
  const auto& parent=store.blocks.at(block.header.parent_root);if(parent.header.slot>=block.header.slot||parent.execution_block.header.number+1!=h.number||parent.execution_block.header.hash()!=h.parent_hash)throw std::invalid_argument("incoherent branch execution ancestry");id=block.header.parent_root;
 }
}
ConsensusRoot ancestor(const Phase0ForkChoiceView& v,ConsensusRoot r,uint64_t slot,const std::function<void()>& checkpoint){
 for(;;){if(checkpoint)checkpoint();const auto& n=v.nodes.at(r);if(n.slot<=slot||r==v.anchor)return r;const auto& p=v.nodes.at(n.parent);if(p.slot>=n.slot)throw std::invalid_argument("invalid fork parent slots");r=n.parent;}
}
}
std::optional<BellatrixForkStore> prepare_bellatrix_fork_block(const BellatrixForkStore& input,
 std::span<const uint8_t> bytes,const BellatrixForkBlockRules& rules,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint){
 validate_rules(rules);const auto& t=rules.transition.consensus;const auto slots=t.epoch.history.slots_per_epoch;
 const auto& clock=input.timed.clock;const auto& view=input.timed.view;
 if(!slots||!rules.slot_duration_ms||rules.attestation_due_ms>rules.slot_duration_ms||clock.time_ms<clock.genesis_time_ms||view.finalized.epoch>UINT64_MAX/slots)
  throw std::invalid_argument("invalid fork block clock/configuration");
 const auto current_slot=(clock.time_ms-clock.genesis_time_ms)/rules.slot_duration_ms;
 if(view.current_epoch!=current_slot/slots)throw std::invalid_argument("fork clock epoch mismatch");
 if(bytes.size()<404||bytes.size()>t.maximum_block_bytes||number(bytes,0,4)!=100||number(bytes,180,4)!=84)return std::nullopt;
 ConsensusBeaconHeader h{number(bytes,100),number(bytes,108),root(bytes,116),root(bytes,148),{}};
 std::exception_ptr interrupted;auto body_checkpoint=[&]{try{if(checkpoint)checkpoint();}catch(...){interrupted=std::current_exception();throw;}};
 try{h.body_root=bellatrix_beacon_body_root(bytes.subspan(184),t.body,t.state.sync_committee_size,rules.transition.execution,t.maximum_block_bytes,body_checkpoint);}catch(const std::invalid_argument&){if(interrupted)std::rethrow_exception(interrupted);return std::nullopt;}
 const auto id=consensus_beacon_header_root(h);
 if(input.blocks.contains(id))return input;
 if(!input.blocks.contains(h.parent_root)||!view.nodes.contains(h.parent_root)||h.slot>current_slot||h.slot<=view.finalized.epoch*slots)return std::nullopt;
 if(ancestor(view,h.parent_root,view.finalized.epoch*slots,checkpoint)!=view.finalized.root)return std::nullopt;
 const auto& parent=input.blocks.at(h.parent_root);
 if(consensus_beacon_header_root(parent.header)!=h.parent_root||bellatrix_beacon_state_root(parent.state.ssz,t.state,rules.transition.execution,rules.transition.maximum_state_bytes,checkpoint)!=parent.header.state_root)
  throw std::invalid_argument("fork parent state binding mismatch");
 validate_execution(parent,rules,checkpoint);
 auto history=[&](const Int& height){return branch_hash(input,h.parent_root,height,checkpoint);};
 auto post=prepare_bellatrix_state_transition(parent.state,bytes,rules.transition,parent.execution_block.header,parent.execution_state,history,maximum_slots,maximum_sync_trials,checkpoint);if(!post)return std::nullopt;
 const auto head=phase0_fork_choice_head(view,{slots,t.attestation.duties.effective_balance_increment,rules.proposer_score_boost},checkpoint).head;
 const auto ffg=bellatrix_fork_checkpoints(post->consensus,epoch_rules(rules),t.state,checkpoint);
 auto out=input;auto& v=out.timed.view;auto& c=out.timed.clock;
 out.blocks.emplace(id,BellatrixForkBlock{h,std::move(post->consensus),std::move(post->execution.block),std::move(post->execution.state),std::move(post->execution.receipts)});
 v.nodes.emplace(id,Phase0ForkNode{h.slot,h.parent_root,ffg.realized.current_justified.epoch,ffg.unrealized.current_justified.epoch});
 const auto into=(clock.time_ms-clock.genesis_time_ms)%rules.slot_duration_ms;
 const bool timely=h.slot==current_slot&&into<rules.attestation_due_ms;out.timely[id]=timely;
 const auto lookahead=t.attestation.duties.min_seed_lookahead;
 const auto dependent_epoch=v.current_epoch>lookahead?v.current_epoch-lookahead:0;
 const auto dependent_start=dependent_epoch*slots,dependent_slot=dependent_start?dependent_start-1:0;
 const bool same_dependency=ancestor(v,head,dependent_slot,checkpoint)==ancestor(v,id,dependent_slot,checkpoint);
 if(rules.boost_profile!=BellatrixProposerBoostProfile::Release160Beta0&&rules.boost_profile!=BellatrixProposerBoostProfile::ShufflingDependent)throw std::invalid_argument("unknown Bellatrix boost profile");
 if(timely&&(!v.proposer_boost||*v.proposer_boost==ConsensusRoot{})&&(rules.boost_profile==BellatrixProposerBoostProfile::Release160Beta0||same_dependency))v.proposer_boost=id;
 auto update=[&](const Phase0FfgCheckpoints& cp){
  if(cp.current_justified.epoch>v.justified.epoch){
   const auto& record=out.blocks.at(cp.current_justified.root);
   const auto ctx=prepare_bellatrix_fork_state_contexts(record.state,record.header,cp.current_justified,epoch_rules(rules),t.state,maximum_slots,maximum_sync_trials,checkpoint);
   v.justified=cp.current_justified;v.justified_registry=ctx.target.registry;
  }
  if(cp.finalized.epoch>v.finalized.epoch)v.finalized=cp.finalized;
 };
 update(ffg.realized);
 out.unrealized_justifications[id]=ffg.unrealized.current_justified;
 if(ffg.unrealized.current_justified.epoch>c.unrealized_justified.epoch)c.unrealized_justified=ffg.unrealized.current_justified;
 if(ffg.unrealized.finalized.epoch>c.unrealized_finalized.epoch)c.unrealized_finalized=ffg.unrealized.finalized;
 if(h.slot/slots<v.current_epoch)update(ffg.unrealized);
 if(checkpoint)checkpoint();return out;
}
}

namespace nano {
std::optional<BellatrixForkStore> prepare_bellatrix_store_vote(const BellatrixForkStore& input,
 const Phase0SignedAttestation& vote,const BellatrixForkBlockRules& rules,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint){
 validate_rules(rules);const auto& c=input.timed.clock;const auto slots=rules.transition.consensus.epoch.history.slots_per_epoch;
 if(!slots||!rules.slot_duration_ms||c.time_ms<c.genesis_time_ms)throw std::invalid_argument("invalid store vote clock");
 const auto slot=(c.time_ms-c.genesis_time_ms)/rules.slot_duration_ms,epoch=slot/slots;
 const auto& d=vote.data;
 if(d.slot>=slot||d.target.epoch!=d.slot/slots||(d.target.epoch!=epoch&&d.target.epoch!=(epoch?epoch-1:0))||
    !input.blocks.contains(d.target.root)||!input.timed.view.nodes.contains(d.beacon_block_root))return std::nullopt;
 const auto& block=input.blocks.at(d.target.root);
 if(block.header.slot>d.target.epoch*slots)return std::nullopt;
 const auto ctx=prepare_bellatrix_fork_state_contexts(block.state,block.header,d.target,epoch_rules(rules),rules.transition.consensus.state,maximum_slots,maximum_sync_trials,checkpoint);
 auto out=input;
 if(!process_phase0_fork_vote(out.timed.view,slot,ctx.target,rules.transition.consensus.attestation.duties,vote,false,checkpoint))return std::nullopt;
 return out;
}
std::optional<BellatrixForkStore> prepare_bellatrix_store_slashing(const BellatrixForkStore& input,
 const IndexedConsensusAttestation& first,const IndexedConsensusAttestation& second,
 const BellatrixForkBlockRules& rules,const std::function<void()>& checkpoint){
 validate_rules(rules);const auto& block=input.blocks.at(input.timed.view.justified.root);
 const auto ctx=bellatrix_fork_block_context(block.state,block.header,epoch_rules(rules),rules.transition.consensus.state,checkpoint);
 auto out=input;
 if(!process_phase0_fork_slashing(out.timed.view,ctx,first,second,rules.transition.consensus.body.validators_per_committee,checkpoint))return std::nullopt;
 return out;
}
BellatrixForkStore prepare_bellatrix_store_tick(const BellatrixForkStore& input,uint64_t time_ms,
 const BellatrixForkBlockRules& rules,uint64_t maximum_slots,uint64_t maximum_sync_trials,const std::function<void()>& checkpoint){
 validate_rules(rules);const auto& c=input.timed.clock;const auto slots=rules.transition.consensus.epoch.history.slots_per_epoch;
 if(!slots||!rules.slot_duration_ms||time_ms<c.time_ms||c.time_ms<c.genesis_time_ms)throw std::invalid_argument("invalid store tick clock");
 const auto epoch=((time_ms-c.genesis_time_ms)/rules.slot_duration_ms)/slots;
 std::optional<BellatrixForkStateContexts> ctx;
 if(epoch>input.timed.view.current_epoch&&c.unrealized_justified.epoch>input.timed.view.justified.epoch){
  const auto& block=input.blocks.at(c.unrealized_justified.root);
  ctx=prepare_bellatrix_fork_state_contexts(block.state,block.header,c.unrealized_justified,epoch_rules(rules),rules.transition.consensus.state,maximum_slots,maximum_sync_trials,checkpoint);
 }
 auto timed=prepare_phase0_fork_tick(input.timed,time_ms,rules.slot_duration_ms,slots,ctx?&ctx->target:nullptr,checkpoint);
 auto out=input;out.timed=std::move(timed);if(checkpoint)checkpoint();return out;
}
}

namespace nano {
BellatrixForkStore initialize_bellatrix_fork_store(const BellatrixSlotState& state,
 std::span<const uint8_t> block,const Block& execution_block,const State& execution_state,const std::vector<Receipt>& execution_receipts,
 const std::vector<Header>& execution_ancestors,const BellatrixForkBlockRules& r,const std::function<void()>& checkpoint){
 validate_rules(r);const auto slots=r.transition.consensus.epoch.ffg.slots_per_epoch;
 if(!slots||!r.slot_duration_ms||block.size()<304||block.size()>r.transition.consensus.maximum_block_bytes||number(block,80,4)!=84)
  throw std::invalid_argument("invalid Bellatrix anchor block");
 const ConsensusBeaconHeader h{number(block,0),number(block,8),root(block,16),root(block,48),bellatrix_beacon_body_root(block.subspan(84),r.transition.consensus.body,r.transition.consensus.state.sync_committee_size,r.transition.execution,r.transition.consensus.maximum_block_bytes,checkpoint)};
 const auto context=bellatrix_fork_block_context(state,h,epoch_rules(r),r.transition.consensus.state,checkpoint);const auto id=consensus_beacon_header_root(h);const auto epoch=h.slot/slots;
 const auto seconds=number(state.ssz,0);if(seconds>UINT64_MAX/1000||h.slot>(UINT64_MAX-seconds*1000)/r.slot_duration_ms)throw std::overflow_error("Bellatrix anchor time overflow");
 BellatrixForkStore out;auto& v=out.timed.view;v.anchor=id;v.justified={epoch,id};v.finalized=v.justified;v.current_epoch=epoch;v.justified_registry=context.registry;
 v.nodes.emplace(id,Phase0ForkNode{h.slot,h.parent_root,epoch,epoch});out.blocks.emplace(id,BellatrixForkBlock{h,state,execution_block,execution_state,execution_receipts});validate_execution(out.blocks.at(id),r,checkpoint);
 const auto payload_start=number(block,84+220+(r.transition.consensus.state.sync_committee_size+7)/8+96,4);
 const auto payload=decode_bellatrix_execution_payload(block.subspan(84+payload_start),r.transition.execution,r.transition.consensus.maximum_block_bytes,checkpoint);
 const auto& state_bytes=state.ssz;const size_t header_at=bellatrix_detail::offset(state_bytes,bellatrix_detail::execution_field(r.transition.consensus.state));
 const auto latest=decode_bellatrix_execution_header(std::span<const uint8_t>(state_bytes).subspan(header_at),r.transition.execution.extra_data_bytes,r.transition.maximum_state_bytes,checkpoint);
 if(!payload||!latest||bellatrix_execution_payload_header(*payload,r.transition.execution,checkpoint)!=*latest)throw std::invalid_argument("Bellatrix anchor payload/state mismatch");
 const auto anchor_number=execution_block.header.number;if(anchor_number<0||anchor_number>Int("18446744073709551615"))throw std::invalid_argument("Bellatrix anchor execution height");
 if(execution_ancestors.size()!=std::min<uint64_t>(u64(anchor_number),256))throw std::invalid_argument("Bellatrix anchor execution history incomplete");
 Header child=execution_block.header;for(const auto& ancestor:execution_ancestors){if(checkpoint)checkpoint();if(ancestor.number+1!=child.number||ancestor.hash()!=child.parent_hash)throw std::invalid_argument("Bellatrix anchor execution ancestry mismatch");child=ancestor;}
 out.anchor_execution_ancestors=execution_ancestors;out.unrealized_justifications[id]=v.justified;
 out.timed.clock={seconds*1000,seconds*1000+h.slot*r.slot_duration_ms,v.justified,v.finalized};if(checkpoint)checkpoint();return out;
}
}

namespace nano {
namespace {
using View=std::span<const uint8_t>;
// These decoders only consume a body already checked by on_block's SSZ hasher.
ConsensusAttestationData embedded_data(View b){return {number(b,4),number(b,12),root(b,20),{number(b,52),root(b,60)},{number(b,92),root(b,100)}};}
IndexedConsensusAttestation embedded_indexed(View b){IndexedConsensusAttestation a{embedded_data(b),{},Bytes(b.begin()+132,b.begin()+228)};for(size_t i=228;i<b.size();i+=8)a.indices.push_back(number(b,i));return a;}
std::vector<View> embedded_list(View b){std::vector<View> list;if(b.empty())return list;const auto count=number(b,0,4)/4;for(size_t i=0;i<count;++i){const auto start=number(b,4*i,4),end=i+1==count?b.size():number(b,4*(i+1),4);list.push_back(b.subspan(start,end-start));}return list;}
}
std::optional<BellatrixForkStore> prepare_bellatrix_received_block(const BellatrixForkStore& input,
 View bytes,const BellatrixForkBlockRules& rules,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint){
 auto out=prepare_bellatrix_fork_block(input,bytes,rules,maximum_slots,maximum_sync_trials,checkpoint);if(!out)return std::nullopt;
 const auto body=bytes.subspan(184);const auto attesters=number(body,204,4),attestations=number(body,208,4),deposits=number(body,212,4);
 const auto slots=rules.transition.consensus.epoch.ffg.slots_per_epoch;
 const auto current_slot=(out->timed.clock.time_ms-out->timed.clock.genesis_time_ms)/rules.slot_duration_ms;
 for(auto b:embedded_list(body.subspan(attestations,deposits-attestations))){
  if(checkpoint)checkpoint();Phase0SignedAttestation vote{embedded_data(b),{},Bytes(b.begin()+132,b.begin()+228)};
  unsigned last=0;for(uint8_t n=b.back();n>1;n>>=1)++last;const auto bit_count=(b.size()-229)*8+last;
  for(size_t i=0;i<bit_count;++i)vote.aggregation_bits.push_back((b[228+i/8]>>(i%8))&1);
  const auto& d=vote.data;
  if(d.slot>=current_slot||d.target.epoch!=d.slot/slots||!out->blocks.contains(d.target.root)||!out->timed.view.nodes.contains(d.beacon_block_root))continue;
  const auto& target=out->blocks.at(d.target.root);if(target.header.slot>d.target.epoch*slots)continue;
  const auto ctx=prepare_bellatrix_fork_state_contexts(target.state,target.header,d.target,epoch_rules(rules),rules.transition.consensus.state,maximum_slots,maximum_sync_trials,checkpoint);
  process_phase0_fork_vote(out->timed.view,current_slot,ctx.target,rules.transition.consensus.attestation.duties,vote,true,checkpoint);
 }
 for(auto b:embedded_list(body.subspan(attesters,attestations-attesters))){
  if(checkpoint)checkpoint();const auto second=number(b,4,4);const auto first=embedded_indexed(b.subspan(8,second-8)),other=embedded_indexed(b.subspan(second));
  const auto& target=out->blocks.at(out->timed.view.justified.root);const auto ctx=bellatrix_fork_block_context(target.state,target.header,epoch_rules(rules),rules.transition.consensus.state,checkpoint);
  process_phase0_fork_slashing(out->timed.view,ctx,first,other,rules.transition.consensus.body.validators_per_committee,checkpoint);
 }
 if(checkpoint)checkpoint();return out;
}
}

namespace nano {
const BellatrixForkBlock& bellatrix_fork_head(const BellatrixForkStore& store,const BellatrixForkBlockRules& r,const std::function<void()>& check){
 validate_rules(r);const auto& t=r.transition.consensus;const auto id=phase0_fork_choice_head(store.timed.view,{t.epoch.ffg.slots_per_epoch,t.attestation.duties.effective_balance_increment,r.proposer_score_boost},check).head;
 const auto& record=store.blocks.at(id);if(consensus_beacon_header_root(record.header)!=id)throw std::invalid_argument("Bellatrix fork head root mismatch");bellatrix_fork_block_context(record.state,record.header,epoch_rules(r),t.state,check);validate_execution(record,r,check);return record;
}
}

namespace nano {
void validate_bellatrix_execution_record(const BellatrixForkBlock& b,const BellatrixForkBlockRules& r,const std::function<void()>& c){validate_rules(r);bellatrix_fork_block_context(b.state,b.header,epoch_rules(r),r.transition.consensus.state,c);validate_execution(b,r,c);}
}

namespace nano {
Bytes bellatrix_execution_history(const BellatrixForkStore& s,const ConsensusRoot& p,const Int& h,const std::function<void()>& c){return branch_hash(s,p,h,c);}
}
