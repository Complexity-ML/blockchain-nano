#include "nano/consensus_capella_fork_store.hpp"
#include "nano/consensus_capella_store_snapshot.hpp"
#include "consensus_capella_projection.hpp"
#include "nano/consensus_capella_deposits.hpp"
#include <exception>
namespace nano {
namespace {
uint64_t number(std::span<const uint8_t> b,size_t at,size_t width=8){uint64_t v=0;for(size_t i=0;i<width;++i)v|=uint64_t(b[at+i])<<(8*i);return v;}
ConsensusRoot root(std::span<const uint8_t> b,size_t at){ConsensusRoot r{};std::copy_n(b.begin()+at,32,r.begin());return r;}
CapellaEpochRules epoch_rules(const CapellaForkBlockRules& r){return {r.transition.consensus.epoch,r.transition.execution,r.transition.maximum_state_bytes};}
void validate_rules(const CapellaForkBlockRules& r){
 validate_altair_transition_rules(r.transition.consensus);
 if(r.native_deposit_votes&&r.deposit_contract_code_hash.empty())throw std::invalid_argument("native deposit voting requires contract identity");
 if(!r.deposit_contract_code_hash.empty()&&r.deposit_contract_code_hash.size()!=32)throw std::invalid_argument("Capella deposit code hash width");
 if(!r.transition.seconds_per_slot||r.transition.seconds_per_slot>UINT64_MAX/1000||r.slot_duration_ms!=r.transition.seconds_per_slot*1000||r.attestation_due_ms>r.slot_duration_ms)
  throw std::invalid_argument("inconsistent Capella fork clock rules");
}
template<class A> Bytes as_bytes(const A& a){return {a.begin(),a.end()};}
void validate_execution(const CapellaForkBlock& record,const CapellaForkBlockRules& rules,const std::function<void()>& check){
 if(rules.deposit_contract_code_hash.empty()){if(record.deposit_history)throw std::invalid_argument("unexpected Capella deposit history");}
 else {if(!record.deposit_history)throw std::invalid_argument("missing Capella deposit history");record.deposit_history->validate_execution(record.execution_block.header,record.execution_state,rules.deposit_contract_code_hash,check);}
 const auto& l=rules.transition.consensus.state;const auto& e=rules.transition.execution;const auto& b=record.state.ssz;
 capella_beacon_state_root(b,l,e,rules.transition.maximum_state_bytes,check);
 const size_t start=capella_detail::offset(b,capella_detail::execution_field(l));
 const auto latest=decode_capella_execution_header(std::span<const uint8_t>(b).subspan(start,capella_detail::offset(b,capella_detail::execution_field(l)+20)-start),e.extra_data_bytes,rules.transition.maximum_state_bytes,check);
 const auto& block=record.execution_block;const auto& h=block.header;const auto hash=h.hash();
 if(!latest||hash!=as_bytes(latest->block_hash)||h.state_root!=record.execution_state.root()||h.state_root!=as_bytes(latest->state_root)||
 h.parent_hash!=as_bytes(latest->parent_hash)||unhex(address(h.coinbase))!=as_bytes(latest->fee_recipient)||h.receipts_root!=as_bytes(latest->receipts_root)||h.logs_bloom!=as_bytes(latest->logs_bloom)||h.mix_hash!=as_bytes(latest->prev_randao)||
 h.number!=Int(std::to_string(latest->block_number))||h.timestamp!=Int(std::to_string(latest->timestamp))||h.gas_limit!=Int(std::to_string(latest->gas_limit))||h.gas_used!=Int(std::to_string(latest->gas_used))||h.extra_data!=latest->extra_data||h.base_fee!=std::optional<Int>(integer(Bytes(latest->base_fee_per_gas.rbegin(),latest->base_fee_per_gas.rend())))||
 h.difficulty!=0||h.nonce!=Bytes(8)||h.uncle_hash!=keccak(Rlp(std::vector<Rlp>{}).encode())||!h.withdrawals_root||*h.withdrawals_root!=withdrawals_root(block.withdrawals)||h.blob_gas_used||h.excess_blob_gas||h.parent_beacon_block_root||h.requests_hash||!block.uncles.empty()||!block.producer_signature.empty()||
 h.transactions_root!=transactions_root(block.transactions)||block.transactions.size()!=record.receipts.size()||h.receipts_root!=receipts_root(record.receipts))throw std::invalid_argument("Capella fork execution binding mismatch");
 CapellaExecutionPayload p;static_cast<BellatrixExecutionFields&>(p)=*latest;
 for(const auto& value:block.withdrawals){CapellaWithdrawal w;w.index=u64(quantity(value.at("index")));w.validator_index=u64(quantity(value.at("validatorIndex")));w.amount_gwei=u64(quantity(value.at("amount")));const auto address=unhex(value.at("address"));if(address.size()!=20)throw std::invalid_argument("Capella withdrawal address width");std::copy(address.begin(),address.end(),w.address.begin());p.withdrawals.push_back(w);}
 Int cumulative=0;std::vector<Log> logs;
 for(size_t i=0;i<block.transactions.size();++i){if(check)check();p.transactions.push_back(block.transactions[i].raw());const auto& receipt=record.receipts[i];cumulative+=receipt.gas_used;logs.insert(logs.end(),receipt.logs.begin(),receipt.logs.end());if(receipt.gas_used<0||receipt.cumulative_gas_used!=cumulative||receipt.transaction_hash!=block.transactions[i].hash()||receipt.block_hash!=hash||receipt.block_number!=h.number||receipt.index!=Int(i))throw std::invalid_argument("Capella fork receipt binding mismatch");}
 if(cumulative!=h.gas_used||logs_bloom(logs)!=h.logs_bloom||capella_execution_payload_header(p,e,rules.transition.withdrawals.maximum_per_payload,check)!=*latest)throw std::invalid_argument("Capella fork payload binding mismatch");
}
Bytes branch_hash(const CapellaForkStore& store,ConsensusRoot id,const Int& height,const std::function<void()>& check){
 if(height<0)throw std::invalid_argument("negative execution history height");
 for(;;){if(check)check();const auto block_pin=store.blocks.at(id).lease(check);const auto& block=*block_pin;const auto& h=block.execution_block.header;
  if(height==h.number)return h.hash();if(height>h.number)throw std::invalid_argument("future execution history height");
  if(id==store.timed.view.anchor){for(const auto& ancestor:store.anchor_execution_ancestors){if(check)check();if(ancestor.number==height)return ancestor.hash();}throw std::invalid_argument("execution history unavailable before anchor");}
  const auto parent_pin=store.blocks.at(block.header.parent_root).lease(check);const auto& parent=*parent_pin;if(parent.header.slot>=block.header.slot||parent.execution_block.header.number+1!=h.number||parent.execution_block.header.hash()!=h.parent_hash)throw std::invalid_argument("incoherent branch execution ancestry");id=block.header.parent_root;
 }
}
ConsensusRoot ancestor(const Phase0ForkChoiceView& v,ConsensusRoot r,uint64_t slot,const std::function<void()>& checkpoint){
 for(;;){if(checkpoint)checkpoint();const auto& n=v.nodes.at(r);if(n.slot<=slot||r==v.anchor)return r;const auto& p=v.nodes.at(n.parent);if(p.slot>=n.slot)throw std::invalid_argument("invalid fork parent slots");r=n.parent;}
}
Phase0ForkVoteContext target_context(const CapellaForkStore& store,const CapellaForkBlockRules& rules,
 const ConsensusCheckpoint& target,std::span<const CapellaTargetPreparation* const> frames,
 uint64_t maximum_slots,uint64_t trials,const std::function<void()>& check,bool require_prepared=false){
 for(const auto* frame:frames){
  if(check)check();
  if(!frame)throw std::invalid_argument("Null Capella target frame");
  if(frame->target()==target)return frame->context(store,rules,check);
 }
 const auto record_pin=store.blocks.at(target.root).lease(check);const auto& record=*record_pin;
 const auto slots=rules.transition.consensus.epoch.ffg.slots_per_epoch;
 if(require_prepared&&slots&&target.epoch<=UINT64_MAX/slots&&record.header.slot<=target.epoch*slots&&target.epoch*slots-record.header.slot>maximum_slots)
  throw CapellaTargetContextRequired(target);
 return prepare_capella_fork_state_contexts(record.state,record.header,target,epoch_rules(rules),
  rules.transition.consensus.state,maximum_slots,trials,check).target;
}
}
static std::optional<CapellaForkStore> prepare_fork_block_impl(const CapellaForkStore& input,
 std::span<const uint8_t> bytes,const CapellaForkBlockRules& rules,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint,const CapellaBlockPreparation* prepared,
 std::span<const CapellaTargetPreparation* const> targets={},bool require_prepared=false){
 validate_rules(rules);const auto& t=rules.transition.consensus;const auto slots=t.epoch.history.slots_per_epoch;
 const auto& clock=input.timed.clock;const auto& view=input.timed.view;
 if(!slots||!rules.slot_duration_ms||rules.attestation_due_ms>rules.slot_duration_ms||clock.time_ms<clock.genesis_time_ms||view.finalized.epoch>UINT64_MAX/slots)
  throw std::invalid_argument("invalid fork block clock/configuration");
 const auto current_slot=(clock.time_ms-clock.genesis_time_ms)/rules.slot_duration_ms;
 if(view.current_epoch!=current_slot/slots)throw std::invalid_argument("fork clock epoch mismatch");
 if(bytes.size()<404||bytes.size()>t.maximum_block_bytes||number(bytes,0,4)!=100||number(bytes,180,4)!=84)return std::nullopt;
 ConsensusBeaconHeader h{number(bytes,100),number(bytes,108),root(bytes,116),root(bytes,148),{}};
 std::exception_ptr interrupted;auto body_checkpoint=[&]{try{if(checkpoint)checkpoint();}catch(...){interrupted=std::current_exception();throw;}};
 try{h.body_root=capella_beacon_body_root(bytes.subspan(184),t.body,t.state.sync_committee_size,rules.transition.execution,rules.transition.withdrawals.maximum_per_payload,rules.transition.maximum_bls_changes,t.maximum_block_bytes,body_checkpoint);}catch(const std::invalid_argument&){if(interrupted)std::rethrow_exception(interrupted);return std::nullopt;}
 const auto id=consensus_beacon_header_root(h);
 if(input.blocks.contains(id))return input;
 if(!input.blocks.contains(h.parent_root)||!view.nodes.contains(h.parent_root)||h.slot>current_slot||h.slot<=view.finalized.epoch*slots)return std::nullopt;
 if(ancestor(view,h.parent_root,view.finalized.epoch*slots,checkpoint)!=view.finalized.root)return std::nullopt;
 if(rules.native_deposit_votes&&!capella_native_deposit_vote_known(input,h.parent_root,bytes.subspan(184+96,72),checkpoint))return std::nullopt;
 const auto parent_pin=input.blocks.at(h.parent_root).lease(checkpoint);const auto& parent=*parent_pin;
 if(consensus_beacon_header_root(parent.header)!=h.parent_root||capella_beacon_state_root(parent.state.ssz,t.state,rules.transition.execution,rules.transition.maximum_state_bytes,checkpoint)!=parent.header.state_root)
  throw std::invalid_argument("fork parent state binding mismatch");
 validate_execution(parent,rules,checkpoint);
 auto history=[&](const Int& height){return branch_hash(input,h.parent_root,height,checkpoint);};
 if(prepared){
  auto profile=rules;profile.transition=prepared->transition_rules();
  if(prepared->parent()!=parent.header||capella_fork_profile(profile)!=capella_fork_profile(rules))
   throw std::invalid_argument("Capella prepared block parent/profile mismatch");
 }
 auto post=prepared?prepared->receive(bytes,parent.execution_block.header,parent.execution_state,history,maximum_sync_trials,checkpoint):prepare_capella_state_transition(parent.state,bytes,rules.transition,parent.execution_block.header,parent.execution_state,history,maximum_slots,maximum_sync_trials,checkpoint);if(!post)return std::nullopt;
 const auto head=phase0_fork_choice_head(view,{slots,t.attestation.duties.effective_balance_increment,rules.proposer_score_boost},checkpoint).head;
 const auto ffg=capella_fork_checkpoints(post->consensus,epoch_rules(rules),t.state,checkpoint);
 auto out=input;auto& v=out.timed.view;auto& c=out.timed.clock;
 std::shared_ptr<const ExecutionDepositHistory> deposits;
 if(!rules.deposit_contract_code_hash.empty()){
  if(!parent.deposit_history)throw std::invalid_argument("missing Capella parent deposits");
  deposits=std::make_shared<const ExecutionDepositHistory>(parent.deposit_history->append_verified_execution(post->execution.block.header,post->execution.state,post->execution.receipts,rules.deposit_contract_code_hash,UINT32_MAX,checkpoint));
 }
 out.blocks.emplace(id,CapellaForkBlock{h,std::move(post->consensus),std::move(post->execution.block),std::move(post->execution.state),std::move(post->execution.receipts),std::move(deposits)});
 v.nodes.emplace(id,Phase0ForkNode{h.slot,h.parent_root,ffg.realized.current_justified.epoch,ffg.unrealized.current_justified.epoch});
 const auto into=(clock.time_ms-clock.genesis_time_ms)%rules.slot_duration_ms;
 const bool timely=h.slot==current_slot&&into<rules.attestation_due_ms;out.timely[id]=timely;
 const auto lookahead=t.attestation.duties.min_seed_lookahead;
 const auto dependent_epoch=v.current_epoch>lookahead?v.current_epoch-lookahead:0;
 const auto dependent_start=dependent_epoch*slots,dependent_slot=dependent_start?dependent_start-1:0;
 const bool same_dependency=ancestor(v,head,dependent_slot,checkpoint)==ancestor(v,id,dependent_slot,checkpoint);
 if(rules.boost_profile!=CapellaProposerBoostProfile::Release160Beta0&&rules.boost_profile!=CapellaProposerBoostProfile::ShufflingDependent)throw std::invalid_argument("unknown Capella boost profile");
 if(timely&&(!v.proposer_boost||*v.proposer_boost==ConsensusRoot{})&&(rules.boost_profile==CapellaProposerBoostProfile::Release160Beta0||same_dependency))v.proposer_boost=id;
 auto update=[&](const Phase0FfgCheckpoints& cp){
  if(cp.current_justified.epoch>v.justified.epoch){
   const auto ctx=target_context(out,rules,cp.current_justified,targets,maximum_slots,maximum_sync_trials,checkpoint,require_prepared);
   v.justified=cp.current_justified;v.justified_registry=ctx.registry;
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
std::optional<CapellaForkStore> prepare_capella_fork_block(const CapellaForkStore& input,
 std::span<const uint8_t> bytes,const CapellaForkBlockRules& rules,uint64_t maximum_slots,
 uint64_t trials,const std::function<void()>& checkpoint){
 return prepare_fork_block_impl(input,bytes,rules,maximum_slots,trials,checkpoint,nullptr);
}
std::optional<CapellaForkStore> prepare_capella_resumed_fork_block(const CapellaForkStore& input,
 std::span<const uint8_t> bytes,const CapellaForkBlockRules& rules,uint64_t maximum_slots,
 uint64_t trials,const std::function<void()>& check){
 validate_rules(rules);const auto& t=rules.transition.consensus;
 const auto slots=t.epoch.ffg.slots_per_epoch,chunk=std::min(maximum_slots,slots);
 if(!chunk||!trials)throw std::invalid_argument("Invalid resumed Capella archive budget");
 if(check)check();
 if(bytes.size()<404||bytes.size()>t.maximum_block_bytes||number(bytes,0,4)!=100||number(bytes,180,4)!=84)return std::nullopt;
 ConsensusBeaconHeader header{number(bytes,100),number(bytes,108),root(bytes,116),root(bytes,148),{}};
 std::exception_ptr interrupted;auto guard=[&]{try{if(check)check();}catch(...){interrupted=std::current_exception();throw;}};
 try{header.body_root=capella_beacon_body_root(bytes.subspan(184),t.body,t.state.sync_committee_size,rules.transition.execution,rules.transition.withdrawals.maximum_per_payload,rules.transition.maximum_bls_changes,t.maximum_block_bytes,guard);}catch(const std::invalid_argument&){if(interrupted)std::rethrow_exception(interrupted);return std::nullopt;}
 if(input.blocks.contains(consensus_beacon_header_root(header)))return input;
 const auto& clock=input.timed.clock;
 if(clock.time_ms<clock.genesis_time_ms)throw std::invalid_argument("Invalid resumed Capella archive clock");
 const auto parent=input.blocks.find(header.parent_root);
 if(parent==input.blocks.end()||header.slot<=parent->second.header().slot||header.slot>(clock.time_ms-clock.genesis_time_ms)/rules.slot_duration_ms)return std::nullopt;
 const auto parent_pin=parent->second.lease(check);CapellaBlockPreparation block(parent_pin->state,parent_pin->header,rules.transition,check);
 while(!block.advance_for_block(header.slot,chunk,trials,check)){}
 std::vector<CapellaTargetPreparation> frames;
 for(;;){
  std::vector<const CapellaTargetPreparation*> targets;for(const auto& frame:frames)targets.push_back(&frame);
  try{return prepare_fork_block_impl(input,bytes,rules,maximum_slots,trials,check,&block,targets,true);}
  catch(const CapellaTargetContextRequired& required){
   // on_block can promote its realized and unrealized checkpoints only.
   if(frames.size()>=2)throw std::length_error("Resumed Capella archive target capacity");
   frames.emplace_back(input,rules,required.target(),check);
   while(!frames.back().advance(chunk,trials,check)){}
  }
 }
}

}

namespace nano {
CapellaTargetPreparation::CapellaTargetPreparation(const CapellaForkStore& store,
 const CapellaForkBlockRules& rules,const ConsensusCheckpoint& target,const std::function<void()>& check)
 :rules_(rules),target_(target),slots_(store.blocks.at(target.root).lease(check)->state,store.blocks.at(target.root).header(),
 epoch_rules(rules),rules.transition.consensus.state,check){
 validate_rules(rules);const auto count=rules.transition.consensus.epoch.ffg.slots_per_epoch;
 if(!count||target.epoch>store.timed.view.current_epoch||target.epoch>UINT64_MAX/count||slots_.parent().slot>target.epoch*count||consensus_beacon_header_root(slots_.parent())!=target.root)throw std::invalid_argument("Invalid prepared Capella target");
}
bool CapellaTargetPreparation::matches(const CapellaForkStore& store,const CapellaForkBlockRules& rules,
 const ConsensusCheckpoint& target,const std::function<void()>& check)const{
 if(target!=target_||capella_fork_profile(rules)!=capella_fork_profile(rules_))return false;
 const auto found=store.blocks.find(target.root);if(found==store.blocks.end()||found->second.header()!=slots_.parent())return false;
 // Recheck the source state binding. A reused root/header must not conceal a
 // mutated record or changed processing metadata in local checkpoint validation.
 const auto found_pin=found->second.lease(check);capella_fork_block_context(found_pin->state,found_pin->header,epoch_rules(rules),rules.transition.consensus.state,check);
 return true;
}
bool CapellaTargetPreparation::retarget(const CapellaForkStore& store,const CapellaForkBlockRules& rules,
 const ConsensusCheckpoint& target,const std::function<void()>& check){
 const auto count=rules_.transition.consensus.epoch.ffg.slots_per_epoch;
 if(target.root!=target_.root||target.epoch<target_.epoch||target.epoch>store.timed.view.current_epoch||target.epoch>UINT64_MAX/count||!matches(store,rules,target_,check))return false;
 if(check)check();target_=target;return true;
}
bool CapellaTargetPreparation::advance(uint64_t maximum_slots,uint64_t trials,const std::function<void()>& check){
 return slots_.advance(target_.epoch*rules_.transition.consensus.epoch.ffg.slots_per_epoch,maximum_slots,trials,check);
}
bool CapellaTargetPreparation::ready()const{return slots_.slot()==target_.epoch*rules_.transition.consensus.epoch.ffg.slots_per_epoch;}
Phase0ForkVoteContext CapellaTargetPreparation::context(const CapellaForkStore& store,
 const CapellaForkBlockRules& rules,const std::function<void()>& check)const{
 if(!ready()||!matches(store,rules,target_,check))throw std::invalid_argument("Incomplete or stale Capella target frame");
 return slots_.vote_context(target_,check);
}
Phase0ForkVoteContext prepare_capella_resumed_target_context(const CapellaForkStore& store,
 const CapellaForkBlockRules& rules,const ConsensusCheckpoint& target,uint64_t maximum_slots,
 uint64_t trials,const std::function<void()>& check){
 CapellaTargetPreparation frame(store,rules,target,check);
 const auto chunk=std::min(maximum_slots,rules.transition.consensus.epoch.ffg.slots_per_epoch);
 while(!frame.advance(chunk,trials,check)){};
 return frame.context(store,rules,check);
}
static std::optional<CapellaForkStore> prepare_store_vote_impl(const CapellaForkStore& input,
 const Phase0SignedAttestation& vote,const CapellaForkBlockRules& rules,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint,const CapellaTargetPreparation* prepared){
 validate_rules(rules);const auto& c=input.timed.clock;const auto slots=rules.transition.consensus.epoch.history.slots_per_epoch;
 if(!slots||!rules.slot_duration_ms||c.time_ms<c.genesis_time_ms)throw std::invalid_argument("invalid store vote clock");
 const auto slot=(c.time_ms-c.genesis_time_ms)/rules.slot_duration_ms,epoch=slot/slots;
 const auto& d=vote.data;
 if(d.slot>=slot||d.target.epoch!=d.slot/slots||(d.target.epoch!=epoch&&d.target.epoch!=(epoch?epoch-1:0))||
    !input.blocks.contains(d.target.root)||!input.timed.view.nodes.contains(d.beacon_block_root))return std::nullopt;
 const auto block_pin=input.blocks.at(d.target.root).lease(checkpoint);const auto& block=*block_pin;
 if(block.header.slot>d.target.epoch*slots)return std::nullopt;
 const auto ctx=prepared?prepared->context(input,rules,checkpoint):prepare_capella_fork_state_contexts(block.state,block.header,d.target,epoch_rules(rules),rules.transition.consensus.state,maximum_slots,maximum_sync_trials,checkpoint).target;
 if(ctx.checkpoint!=d.target)throw std::invalid_argument("Prepared vote target mismatch");
 auto out=input;
 if(!process_phase0_fork_vote(out.timed.view,slot,ctx,rules.transition.consensus.attestation.duties,vote,false,checkpoint))return std::nullopt;
 return out;
}
std::optional<CapellaForkStore> prepare_capella_store_vote(const CapellaForkStore& store,
 const Phase0SignedAttestation& vote,const CapellaForkBlockRules& rules,uint64_t maximum_slots,
 uint64_t trials,const std::function<void()>& check){return prepare_store_vote_impl(store,vote,rules,maximum_slots,trials,check,nullptr);}
std::optional<CapellaForkStore> prepare_capella_prepared_store_vote(const CapellaForkStore& store,
 const Phase0SignedAttestation& vote,const CapellaForkBlockRules& rules,const CapellaTargetPreparation& prepared,
 const std::function<void()>& check){return prepare_store_vote_impl(store,vote,rules,1,1,check,&prepared);}
std::optional<CapellaForkStore> prepare_capella_store_slashing(const CapellaForkStore& input,
 const IndexedConsensusAttestation& first,const IndexedConsensusAttestation& second,
 const CapellaForkBlockRules& rules,const std::function<void()>& checkpoint){
 validate_rules(rules);const auto block_pin=input.blocks.at(input.timed.view.justified.root).lease(checkpoint);const auto& block=*block_pin;
 const auto ctx=capella_fork_block_context(block.state,block.header,epoch_rules(rules),rules.transition.consensus.state,checkpoint);
 auto out=input;
 if(!process_phase0_fork_slashing(out.timed.view,ctx,first,second,rules.transition.consensus.body.validators_per_committee,checkpoint))return std::nullopt;
 return out;
}
CapellaForkStore prepare_capella_contextual_store_tick(const CapellaForkStore& input,uint64_t time_ms,
 const CapellaForkBlockRules& rules,std::span<const CapellaTargetPreparation* const> targets,
 uint64_t maximum_slots,uint64_t maximum_sync_trials,const std::function<void()>& checkpoint){
 validate_rules(rules);const auto& c=input.timed.clock;const auto slots=rules.transition.consensus.epoch.history.slots_per_epoch;
 if(!slots||!rules.slot_duration_ms||time_ms<c.time_ms||c.time_ms<c.genesis_time_ms)throw std::invalid_argument("invalid store tick clock");
 const auto epoch=((time_ms-c.genesis_time_ms)/rules.slot_duration_ms)/slots;
 std::optional<Phase0ForkVoteContext> ctx;
 if(epoch>input.timed.view.current_epoch&&c.unrealized_justified.epoch>input.timed.view.justified.epoch){
  ctx=target_context(input,rules,c.unrealized_justified,targets,maximum_slots,maximum_sync_trials,checkpoint);
 }
 auto timed=prepare_phase0_fork_tick(input.timed,time_ms,rules.slot_duration_ms,slots,ctx?&*ctx:nullptr,checkpoint);
 auto out=input;out.timed=std::move(timed);if(checkpoint)checkpoint();return out;
}
CapellaForkStore prepare_capella_store_tick(const CapellaForkStore& input,uint64_t time_ms,
 const CapellaForkBlockRules& rules,uint64_t maximum_slots,uint64_t trials,const std::function<void()>& check){
 return prepare_capella_contextual_store_tick(input,time_ms,rules,{},maximum_slots,trials,check);
}
CapellaForkStore prepare_capella_resumed_store_tick(const CapellaForkStore& input,uint64_t time_ms,
 const CapellaForkBlockRules& rules,uint64_t maximum_slots,uint64_t trials,const std::function<void()>& check){
 validate_rules(rules);const auto& c=input.timed.clock;const auto slots=rules.transition.consensus.epoch.ffg.slots_per_epoch;
 const auto chunk=std::min(maximum_slots,slots);
 if(!chunk||!trials||time_ms<c.time_ms||c.time_ms<c.genesis_time_ms)throw std::invalid_argument("Invalid resumed Capella tick budget/clock");
 std::optional<CapellaTargetPreparation> frame;std::vector<const CapellaTargetPreparation*> targets;
 if((time_ms-c.genesis_time_ms)/rules.slot_duration_ms/slots>input.timed.view.current_epoch&&c.unrealized_justified.epoch>input.timed.view.justified.epoch){
  frame.emplace(input,rules,c.unrealized_justified,check);
  while(!frame->advance(chunk,trials,check)){}
  targets.push_back(&*frame);
 }
 return prepare_capella_contextual_store_tick(input,time_ms,rules,targets,maximum_slots,trials,check);
}
}

namespace nano {
CapellaForkStore initialize_capella_fork_store(const CapellaSlotState& state,
 std::span<const uint8_t> block,const Block& execution_block,const State& execution_state,const std::vector<Receipt>& execution_receipts,
 const std::vector<Header>& execution_ancestors,const CapellaForkBlockRules& r,const std::function<void()>& checkpoint,std::shared_ptr<const ExecutionDepositHistory> anchor_deposits){
 validate_rules(r);const auto slots=r.transition.consensus.epoch.ffg.slots_per_epoch;
 if(!slots||!r.slot_duration_ms||block.size()<304||block.size()>r.transition.consensus.maximum_block_bytes||number(block,80,4)!=84)
  throw std::invalid_argument("invalid Capella anchor block");
 const ConsensusBeaconHeader h{number(block,0),number(block,8),root(block,16),root(block,48),capella_beacon_body_root(block.subspan(84),r.transition.consensus.body,r.transition.consensus.state.sync_committee_size,r.transition.execution,r.transition.withdrawals.maximum_per_payload,r.transition.maximum_bls_changes,r.transition.consensus.maximum_block_bytes,checkpoint)};
 const auto context=capella_fork_block_context(state,h,epoch_rules(r),r.transition.consensus.state,checkpoint);const auto id=consensus_beacon_header_root(h);const auto epoch=h.slot/slots;
 const auto seconds=number(state.ssz,0);if(seconds>UINT64_MAX/1000||h.slot>(UINT64_MAX-seconds*1000)/r.slot_duration_ms)throw std::overflow_error("Capella anchor time overflow");
 CapellaForkStore out;auto& v=out.timed.view;v.anchor=id;v.justified={epoch,id};v.finalized=v.justified;v.current_epoch=epoch;v.justified_registry=context.registry;
 if(!r.deposit_contract_code_hash.empty()&&!anchor_deposits)anchor_deposits=std::make_shared<const ExecutionDepositHistory>(execution_block.header.hash());
 v.nodes.emplace(id,Phase0ForkNode{h.slot,h.parent_root,epoch,epoch});out.blocks.emplace(id,CapellaForkBlock{h,state,execution_block,execution_state,execution_receipts,std::move(anchor_deposits)});validate_execution((*out.blocks.at(id).lease(checkpoint)),r,checkpoint);
 const auto payload_start=number(block,84+220+(r.transition.consensus.state.sync_committee_size+7)/8+96,4);
 const auto payload_end=number(block,84+224+(r.transition.consensus.state.sync_committee_size+7)/8+96,4);
 const auto payload=decode_capella_execution_payload(block.subspan(84+payload_start,payload_end-payload_start),r.transition.execution,r.transition.withdrawals.maximum_per_payload,r.transition.consensus.maximum_block_bytes,checkpoint);
 const auto& state_bytes=state.ssz;const size_t header_at=capella_detail::offset(state_bytes,capella_detail::execution_field(r.transition.consensus.state));
 const auto latest=decode_capella_execution_header(std::span<const uint8_t>(state_bytes).subspan(header_at,capella_detail::offset(state_bytes,capella_detail::execution_field(r.transition.consensus.state)+20)-header_at),r.transition.execution.extra_data_bytes,r.transition.maximum_state_bytes,checkpoint);
 if(!payload||!latest||capella_execution_payload_header(*payload,r.transition.execution,r.transition.withdrawals.maximum_per_payload,checkpoint)!=*latest)throw std::invalid_argument("Capella anchor payload/state mismatch");
 const auto anchor_number=execution_block.header.number;if(anchor_number<0||anchor_number>Int("18446744073709551615"))throw std::invalid_argument("Capella anchor execution height");
 if(execution_ancestors.size()!=std::min<uint64_t>(u64(anchor_number),256))throw std::invalid_argument("Capella anchor execution history incomplete");
 Header child=execution_block.header;for(const auto& ancestor:execution_ancestors){if(checkpoint)checkpoint();if(ancestor.number+1!=child.number||ancestor.hash()!=child.parent_hash)throw std::invalid_argument("Capella anchor execution ancestry mismatch");child=ancestor;}
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
static std::optional<CapellaForkStore> prepare_received_block_impl(const CapellaForkStore& input,
 View bytes,const CapellaForkBlockRules& rules,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint,const CapellaBlockPreparation* prepared,
 std::span<const CapellaTargetPreparation* const> targets={},bool require_prepared=false){
 auto out=prepare_fork_block_impl(input,bytes,rules,maximum_slots,maximum_sync_trials,checkpoint,prepared,targets,require_prepared);if(!out)return std::nullopt;
 const auto body=bytes.subspan(184);const auto attesters=number(body,204,4),attestations=number(body,208,4),deposits=number(body,212,4);
 const auto slots=rules.transition.consensus.epoch.ffg.slots_per_epoch;
 const auto current_slot=(out->timed.clock.time_ms-out->timed.clock.genesis_time_ms)/rules.slot_duration_ms;
 for(auto b:embedded_list(body.subspan(attestations,deposits-attestations))){
  if(checkpoint)checkpoint();Phase0SignedAttestation vote{embedded_data(b),{},Bytes(b.begin()+132,b.begin()+228)};
  unsigned last=0;for(uint8_t n=b.back();n>1;n>>=1)++last;const auto bit_count=(b.size()-229)*8+last;
  for(size_t i=0;i<bit_count;++i)vote.aggregation_bits.push_back((b[228+i/8]>>(i%8))&1);
  const auto& d=vote.data;
  if(d.slot>=current_slot||d.target.epoch!=d.slot/slots||!out->blocks.contains(d.target.root)||!out->timed.view.nodes.contains(d.beacon_block_root))continue;
  const auto target_pin=out->blocks.at(d.target.root).lease(checkpoint);const auto& target=*target_pin;if(target.header.slot>d.target.epoch*slots)continue;
  const auto ctx=target_context(*out,rules,d.target,targets,maximum_slots,maximum_sync_trials,checkpoint,require_prepared);
  process_phase0_fork_vote(out->timed.view,current_slot,ctx,rules.transition.consensus.attestation.duties,vote,true,checkpoint);
 }
 for(auto b:embedded_list(body.subspan(attesters,attestations-attesters))){
  if(checkpoint)checkpoint();const auto second=number(b,4,4);const auto first=embedded_indexed(b.subspan(8,second-8)),other=embedded_indexed(b.subspan(second));
  const auto target_pin=out->blocks.at(out->timed.view.justified.root).lease(checkpoint);const auto& target=*target_pin;const auto ctx=capella_fork_block_context(target.state,target.header,epoch_rules(rules),rules.transition.consensus.state,checkpoint);
  process_phase0_fork_slashing(out->timed.view,ctx,first,other,rules.transition.consensus.body.validators_per_committee,checkpoint);
 }
 if(checkpoint)checkpoint();return out;
}
std::optional<CapellaForkStore> prepare_capella_received_block(const CapellaForkStore& input,
 View bytes,const CapellaForkBlockRules& rules,uint64_t maximum_slots,uint64_t trials,
 const std::function<void()>& checkpoint){
 return prepare_received_block_impl(input,bytes,rules,maximum_slots,trials,checkpoint,nullptr);
}
std::optional<CapellaForkStore> prepare_capella_prepared_received_block(const CapellaForkStore& input,
 View bytes,const CapellaForkBlockRules& rules,const CapellaBlockPreparation& prepared,
 uint64_t maximum_slots,uint64_t trials,const std::function<void()>& checkpoint){
 return prepare_received_block_impl(input,bytes,rules,maximum_slots,trials,checkpoint,&prepared);
}
std::optional<CapellaForkStore> prepare_capella_contextual_received_block(const CapellaForkStore& input,
 View bytes,const CapellaForkBlockRules& rules,const CapellaBlockPreparation& prepared,
 std::span<const CapellaTargetPreparation* const> targets,uint64_t maximum_slots,uint64_t trials,
 const std::function<void()>& checkpoint){
 return prepare_received_block_impl(input,bytes,rules,maximum_slots,trials,checkpoint,&prepared,targets,true);
}

}

namespace nano {
std::shared_ptr<const CapellaForkBlock> capella_fork_head(const CapellaForkStore& store,const CapellaForkBlockRules& r,const std::function<void()>& check){
 validate_rules(r);const auto& t=r.transition.consensus;const auto id=phase0_fork_choice_head(store.timed.view,{t.epoch.ffg.slots_per_epoch,t.attestation.duties.effective_balance_increment,r.proposer_score_boost},check).head;
 const auto record_pin=store.blocks.at(id).lease(check);const auto& record=*record_pin;if(consensus_beacon_header_root(record.header)!=id)throw std::invalid_argument("Capella fork head root mismatch");capella_fork_block_context(record.state,record.header,epoch_rules(r),t.state,check);validate_execution(record,r,check);return record_pin;
}
}

namespace nano {
void validate_capella_execution_record(const CapellaForkBlock& b,const CapellaForkBlockRules& r,const std::function<void()>& c){validate_rules(r);capella_fork_block_context(b.state,b.header,epoch_rules(r),r.transition.consensus.state,c);validate_execution(b,r,c);}
}

namespace nano {
Bytes capella_execution_history(const CapellaForkStore& s,const ConsensusRoot& p,const Int& h,const std::function<void()>& c){return branch_hash(s,p,h,c);}
}

namespace nano {
ConsensusRoot capella_network_anchor(const CapellaForkStore& store){return store.network_anchor.value_or(store.timed.view.anchor);}
CapellaForkStore prepare_capella_pruned_store(const CapellaForkStore& input,const ConsensusRoot& anchor,const CapellaForkBlockRules& rules,const std::function<void()>& check){
 if(check)check();validate_rules(rules);const auto& view=input.timed.view;if(anchor==view.anchor)return input;
 const auto slots=rules.transition.consensus.epoch.ffg.slots_per_epoch,lookahead=rules.transition.consensus.attestation.duties.min_seed_lookahead;
 if(!slots||lookahead>=view.current_epoch||!input.blocks.contains(anchor))throw std::invalid_argument("Capella pruning anchor/context unavailable");
 const auto retained_epoch=view.current_epoch-lookahead-1;
 if(retained_epoch>UINT64_MAX/slots||input.blocks.at(anchor).header().slot>retained_epoch*slots)throw std::invalid_argument("Capella pruning would remove recent fork context");
 auto at=view.finalized.root;while(at!=anchor&&at!=view.anchor){if(check)check();at=input.blocks.at(at).header().parent_root;}
 if(at!=anchor)throw std::invalid_argument("Capella pruning anchor is not finalized ancestry");
 const auto expected=consensus_beacon_header_root(capella_fork_head(input,rules,check)->header);
 auto output=input;output.network_anchor=capella_network_anchor(input);output.timed.view.anchor=anchor;
 if(rules.native_deposit_votes){output.deposit_ancestors=capella_deposit_ancestors_before(input,anchor,check);validate_capella_deposit_ancestors(output,check);}
 std::vector<std::pair<uint64_t,ConsensusRoot>> ordered;for(const auto& [id,record]:input.blocks)ordered.emplace_back(record.header().slot,id);std::sort(ordered.begin(),ordered.end());std::set<ConsensusRoot> retained{anchor};
 for(const auto& [slot,id]:ordered){if(check)check();if(id!=anchor&&retained.contains(input.blocks.at(id).header().parent_root))retained.insert(id);}
 for(const auto& required:{view.justified.root,view.finalized.root,input.timed.clock.unrealized_justified.root,input.timed.clock.unrealized_finalized.root})if(!retained.contains(required))throw std::invalid_argument("Capella pruning would remove checkpoint context");
 output.anchor_execution_ancestors.clear();at=anchor;
 while(at!=view.anchor&&output.anchor_execution_ancestors.size()<256){if(check)check();at=input.blocks.at(at).header().parent_root;output.anchor_execution_ancestors.push_back(input.blocks.at(at).execution_header());}
 for(const auto& header:input.anchor_execution_ancestors){if(output.anchor_execution_ancestors.size()==256)break;output.anchor_execution_ancestors.push_back(header);}
 std::erase_if(output.blocks,[&](const auto& item){if(check)check();return !retained.contains(item.first);});std::erase_if(output.timed.view.nodes,[&](const auto& item){return !retained.contains(item.first);});std::erase_if(output.timely,[&](const auto& item){return item.first==anchor||!retained.contains(item.first);});std::erase_if(output.unrealized_justifications,[&](const auto& item){return !retained.contains(item.first);});
 // Preserve epoch watermarks: erasing messages could admit older replayed votes.
 for(auto& [index,message]:output.timed.view.latest_messages)if(!retained.contains(message.root))message.root=anchor;
 if(output.timed.view.proposer_boost&&!retained.contains(*output.timed.view.proposer_boost))output.timed.view.proposer_boost.reset();
 if(consensus_beacon_header_root(capella_fork_head(output,rules,check)->header)!=expected)throw std::invalid_argument("Capella pruning changed fork choice");if(check)check();return output;
}
}
