#include "nano/consensus_capella_reactor.hpp"
#include "nano/chain.hpp"
#include "proposal_vote_selection.hpp"
#include "nano/consensus_message_ssz.hpp"
#include "nano/consensus_capella_deposits.hpp"
namespace nano {
namespace {
uint64_t number(const Bytes& b,size_t at){uint64_t n=0;for(size_t i=0;i<8;++i)n|=uint64_t(b.at(at+i))<<(8*i);return n;}
void offset(Bytes& b,size_t at,size_t n){if(n>UINT32_MAX)throw std::length_error("Capella body offset");for(size_t i=0;i<4;++i)b.at(at+i)=uint8_t(n>>(8*i));}
template<size_t N>void field(std::array<uint8_t,N>& out,const Bytes& in){if(in.size()!=N)throw std::invalid_argument("Capella payload field width");std::copy(in.begin(),in.end(),out.begin());}
CapellaExecutionPayload payload(const Block& b,const std::vector<CapellaWithdrawal>& withdrawals){
 const auto& h=b.header;CapellaExecutionPayload p;p.withdrawals=withdrawals;
 field(p.parent_hash,h.parent_hash);field(p.fee_recipient,unhex(h.coinbase));field(p.state_root,h.state_root);field(p.receipts_root,h.receipts_root);field(p.logs_bloom,h.logs_bloom);field(p.prev_randao,h.mix_hash);field(p.block_hash,h.hash());
 if(!h.base_fee)throw std::invalid_argument("Capella payload missing base fee");auto fee=bytes(*h.base_fee,32);std::reverse(fee.begin(),fee.end());field(p.base_fee_per_gas,fee);
 p.block_number=u64(h.number);p.gas_limit=u64(h.gas_limit);p.gas_used=u64(h.gas_used);p.timestamp=u64(h.timestamp);p.extra_data=h.extra_data;
 for(const auto& tx:b.transactions)p.transactions.push_back(tx.raw());return p;
}
}
std::optional<Bytes> CapellaDurableReactor::prepare_local_body(uint64_t index,uint64_t slot,
 const Bytes& reveal,const std::string& recipient,const ForkCheckpointToken& expected,
 std::span<const SignedTransaction> transactions,size_t maximum_transaction_trials,size_t maximum_vote_trials,const std::function<void()>& check){
 std::lock_guard lock(mutex_);return prepare_local_body_locked(index,slot,reveal,recipient,expected,transactions,maximum_transaction_trials,maximum_vote_trials,check);
}
std::optional<Bytes> CapellaDurableReactor::prepare_local_body_with_bls_changes(uint64_t index,uint64_t slot,const Bytes& reveal,const std::string& recipient,const ForkCheckpointToken& expected,std::span<const SignedTransaction> transactions,size_t maximum_transaction_trials,size_t maximum_vote_trials,std::span<const SignedCapellaBlsChange> changes,size_t maximum_change_trials,const std::function<void()>& check){
 if(!maximum_change_trials)throw std::invalid_argument("Capella change trial budget required");
 std::lock_guard lock(mutex_);return prepare_local_body_locked(index,slot,reveal,recipient,expected,transactions,maximum_transaction_trials,maximum_vote_trials,check,changes,maximum_change_trials);
}
std::optional<Bytes> CapellaDurableReactor::prepare_local_body_from_pool(uint64_t index,uint64_t slot,const Bytes& reveal,const std::string& recipient,const ForkCheckpointToken& expected,size_t maximum_transaction_trials,size_t maximum_vote_trials,const std::function<void()>& check){
 std::lock_guard lock(mutex_);return prepare_local_body_from_pool_locked(index,slot,reveal,recipient,expected,maximum_transaction_trials,maximum_vote_trials,check);
}
std::optional<Bytes> CapellaDurableReactor::prepare_local_body_from_pool_locked(uint64_t index,uint64_t slot,const Bytes& reveal,const std::string& recipient,const ForkCheckpointToken& expected,size_t maximum_transaction_trials,size_t maximum_vote_trials,const std::function<void()>& check){
 require_available();if(!execution_)throw std::invalid_argument("Capella execution owner required");if(expected!=*token_)throw StaleCapellaDuty("Stale pool body preparation");
 std::vector<SignedTransaction> transactions;
 {std::lock_guard chain_lock(execution_->mutex_);execution_->require_execution_ready_locked();transactions=execution_->pool_.pending_for_block(execution_->tip_.header.gas_limit);}
 return prepare_local_body_locked(index,slot,reveal,recipient,expected,transactions,maximum_transaction_trials,maximum_vote_trials,check);
}
std::optional<Bytes> CapellaDurableReactor::prepare_local_body_locked(uint64_t index,uint64_t slot,const Bytes& reveal,const std::string& recipient,const ForkCheckpointToken& expected,std::span<const SignedTransaction> transactions,size_t maximum_transaction_trials,size_t maximum_vote_trials,const std::function<void()>& check,std::span<const SignedCapellaBlsChange> changes,size_t maximum_change_trials){
 require_available();
 if(expected!=*token_)throw StaleCapellaDuty("Stale body preparation");
 if(!maximum_transaction_trials||!maximum_vote_trials||reveal.size()!=96)throw std::invalid_argument("Capella local body context");
 if(check)check();const auto& t=rules_.transition.consensus;const auto& clock=state_->timed.clock;
 if((clock.time_ms-clock.genesis_time_ms)/rules_.slot_duration_ms!=slot)return std::nullopt;
 const auto head=phase0_fork_choice_head(state_->timed.view,{t.epoch.ffg.slots_per_epoch,t.attestation.duties.effective_balance_increment,rules_.proposer_score_boost},check).head;
 const auto& parent=state_->blocks.at(head);
 const CapellaEpochRules epoch{t.epoch,rules_.transition.execution,rules_.transition.maximum_state_bytes};
 const auto identity=prepare_capella_proposer_identity(parent.state,parent.header,slot,epoch,t.state,t.attestation.duties,limits_.replay_slots,limits_.sync_trials,check);
 if(!identity||identity->index!=index)return std::nullopt;
 auto advanced=prepare_capella_slots(parent.state,epoch,t.state,slot,limits_.replay_slots,limits_.sync_trials,check);
 // The complete Capella SSZ layout was validated by prepare_capella_slots.
 const size_t historical=176+64*t.state.slots_per_historical_root,randao=historical+96;
 Bytes deposit_vote(advanced.ssz.begin()+historical+4,advanced.ssz.begin()+historical+76),selected_deposits;
 if(rules_.native_deposit_votes){deposit_vote=capella_native_deposit_vote(*state_,head,slot,rules_,check);selected_deposits=capella_native_deposit_operations(*state_,head,advanced.ssz,deposit_vote,rules_,check);}
 const uint64_t genesis=number(advanced.ssz,0),seconds=rules_.transition.seconds_per_slot;
 if(!seconds||slot>(UINT64_MAX-genesis)/seconds)throw std::overflow_error("Capella proposal timestamp");
 Block block;auto& h=block.header;h.parent_hash=parent.execution_block.header.hash();h.number=parent.execution_block.header.number+1;h.difficulty=0;
 h.coinbase=address(recipient);h.timestamp=Int(std::to_string(genesis+slot*seconds));h.gas_limit=parent.execution_block.header.gas_limit;h.base_fee=next_base_fee(parent.execution_block.header);
 const size_t mix=randao+32*((slot/t.epoch.ffg.slots_per_epoch)%t.state.epochs_per_historical_vector);h.mix_hash=Bytes(advanced.ssz.begin()+mix,advanced.ssz.begin()+mix+32);
 const auto registry_state=read_capella_withdrawal_state(advanced.ssz,capella_beacon_state_root(advanced.ssz,t.state,rules_.transition.execution,rules_.transition.maximum_state_bytes,check),t.state,rules_.transition.execution,rules_.transition.maximum_state_bytes,t.epoch.ffg.slots_per_epoch,check);
 const auto withdrawals=expected_capella_withdrawals(registry_state,rules_.transition.withdrawals,check);for(const auto& w:withdrawals)block.withdrawals.push_back(w.to_json());h.withdrawals_root=withdrawals_root(block.withdrawals);
 std::vector<TxPool::Candidate> pending;
 for(size_t i=0;i<std::min(transactions.size(),maximum_transaction_trials);++i){if(check)check();try{pending.push_back({transactions[i],transactions[i].sender()});}catch(const std::invalid_argument&){}}
 const auto& e=rules_.transition.execution;
 // Reserve the complete configured vote-list capacity before selecting pool
 // transactions; a busy pool must not crowd attestations out of proposals.
 const uint64_t vote_bytes=232+(uint64_t(t.body.validators_per_committee)+8)/8;
 const uint64_t fixed_budget=184+228+(uint64_t(t.state.sync_committee_size)+7)/8+96+512+44*uint64_t(withdrawals.size());
 if(t.body.attestations&&vote_bytes>(UINT64_MAX-fixed_budget)/t.body.attestations)throw std::length_error("Capella vote reserve overflow");
 uint64_t reserved=fixed_budget+uint64_t(t.body.attestations)*vote_bytes;
 const uint64_t proof_bytes=468+16*uint64_t(t.body.validators_per_committee);
 if(t.body.attester_slashings&&proof_bytes>(UINT64_MAX-reserved)/t.body.attester_slashings)throw std::length_error("Capella slashing reserve overflow");
 reserved+=uint64_t(t.body.attester_slashings)*proof_bytes;
 if(uint64_t(t.body.voluntary_exits)>(UINT64_MAX-reserved)/112)throw std::length_error("Capella exit reserve overflow");
 reserved+=uint64_t(t.body.voluntary_exits)*112;
 if(selected_deposits.size()>UINT64_MAX-reserved)throw std::length_error("Capella deposit reserve overflow");
 reserved+=selected_deposits.size();
 const auto change_capacity=std::min({changes.size(),size_t(rules_.transition.maximum_bls_changes),maximum_change_trials});
 if(change_capacity>(UINT64_MAX-reserved)/172)throw std::length_error("Capella BLS change reserve overflow");
 reserved+=uint64_t(change_capacity)*172;
 if(reserved>=t.maximum_block_bytes)throw std::length_error("Capella block capacity cannot hold configured attestation reserve");
 size_t remaining=t.maximum_block_bytes-reserved;
 std::erase_if(pending,[&](const auto& c){const auto n=c.transaction.raw().size();if(n>e.transaction_bytes||remaining<4||n>remaining-4)return true;remaining-=n+4;return false;});
 if(pending.size()>e.transactions)pending.resize(e.transactions);
 auto history=[&](const Int& n){return capella_execution_history(*state_,head,n,check);};
 ExecutionEngine engine(chain_,history,EvmRevision::Shanghai,rules_.transition.replay_policy,ConsensusInputPolicy::EthereumValidated);
 auto built=Blockchain::execute_candidate(block,parent.execution_state,std::move(pending),engine.with_checkpoint(check),check,false);
 const auto encoded=encode_capella_execution_payload(payload(built.block,withdrawals),e,rules_.transition.withdrawals.maximum_per_payload,t.maximum_block_bytes,check);
 const size_t sync_bytes=(uint64_t(t.state.sync_committee_size)+7)/8+96,fixed=228+sync_bytes;
 std::vector<Bytes> selected_slashings;Bytes selected_exits,selected_changes;
 auto assemble=[&](const std::vector<Bytes>& votes){
  Bytes slashings(selected_slashings.size()*4);size_t proof_at=0;
  for(const auto& proof:selected_slashings){offset(slashings,proof_at,slashings.size());proof_at+=4;if(proof.size()>t.maximum_block_bytes-slashings.size())throw std::length_error("Capella slashings budget");slashings.insert(slashings.end(),proof.begin(),proof.end());}
  if(fixed>t.maximum_block_bytes||votes.size()>UINT32_MAX/4)throw std::length_error("Capella proposal body budget");
  Bytes attestations(votes.size()*4);size_t at=0;
  for(const auto& vote:votes){offset(attestations,at,attestations.size());at+=4;if(vote.size()>t.maximum_block_bytes-attestations.size())throw std::length_error("Capella attestations budget");attestations.insert(attestations.end(),vote.begin(),vote.end());}
  Bytes body(fixed);std::copy(reveal.begin(),reveal.end(),body.begin());std::copy(deposit_vote.begin(),deposit_vote.end(),body.begin()+96);
  // Empty sync participation is a valid aggregate (and incurs normal penalties).
  body[220+(uint64_t(t.state.sync_committee_size)+7)/8]=0xc0;
  offset(body,200,fixed);offset(body,204,fixed);offset(body,208,fixed+slashings.size());
  offset(body,212,fixed+slashings.size()+attestations.size());offset(body,216,fixed+slashings.size()+attestations.size()+selected_deposits.size());offset(body,220+sync_bytes,fixed+slashings.size()+attestations.size()+selected_deposits.size()+selected_exits.size());
  offset(body,224+sync_bytes,fixed+slashings.size()+attestations.size()+selected_deposits.size()+selected_exits.size()+encoded.size());
  if(slashings.size()>t.maximum_block_bytes-body.size())throw std::length_error("Capella body slashing budget");body.insert(body.end(),slashings.begin(),slashings.end());
  if(attestations.size()>t.maximum_block_bytes-body.size())throw std::length_error("Capella body vote budget");body.insert(body.end(),attestations.begin(),attestations.end());
  if(selected_deposits.size()>t.maximum_block_bytes-body.size())throw std::length_error("Capella body deposit budget");body.insert(body.end(),selected_deposits.begin(),selected_deposits.end());
  if(selected_exits.size()>t.maximum_block_bytes-body.size())throw std::length_error("Capella body exit budget");body.insert(body.end(),selected_exits.begin(),selected_exits.end());
  if(encoded.size()>t.maximum_block_bytes-body.size())throw std::length_error("Capella body payload budget");body.insert(body.end(),encoded.begin(),encoded.end());
  if(selected_changes.size()>t.maximum_block_bytes-body.size())throw std::length_error("Capella body BLS change budget");body.insert(body.end(),selected_changes.begin(),selected_changes.end());return body;
 };
 auto valid=[&](const Bytes& body){return prepare_capella_unsigned_block(parent.state,slot,index,head,body,rules_.transition,parent.execution_block.header,parent.execution_state,history,limits_.replay_slots,limits_.sync_trials,check).has_value();};
 std::vector<Bytes> selected;auto body=assemble(selected);
 if(!valid(body))throw std::runtime_error("Capella proposal needs valid mandatory operations or execution payload");
 size_t tried=0;
 const auto registry=capella_fork_block_context(parent.state,parent.header,epoch,t.state,check).registry;
 for(const auto& [id,wire]:slashings_){
  if(check)check();if(selected_slashings.size()>=t.body.attester_slashings||tried>=maximum_vote_trials)break;
  const auto proof=decode_consensus_attester_slashing_ssz(wire,t.body.validators_per_committee,check);
  if(!proof)throw std::logic_error("Invalid archived slashing encoding");
  bool useful=false;
  for(auto i:proof->first.indices){if(check)check();if(!std::binary_search(proof->second.indices.begin(),proof->second.indices.end(),i))continue;const auto member=registry.validators.find(i);if(member!=registry.validators.end()&&!member->second.slashed){useful=true;break;}}
  if(!useful)continue;
  ++tried;selected_slashings.push_back(wire);auto candidate=assemble(selected);
  if(valid(candidate))body=std::move(candidate);else selected_slashings.pop_back();
 }

 // Keep rejected operations out of the candidate without changing live state.
 // The transition checks credentials, genesis-domain signatures, duplicates and
 // the required ordering after this block's withdrawal sweep.
 for(size_t i=0;i<std::min(changes.size(),maximum_change_trials)&&selected_changes.size()/172<rules_.transition.maximum_bls_changes;++i){
  if(check)check();const auto prior=selected_changes.size();const auto wire=encode_capella_bls_change(changes[i]);selected_changes.insert(selected_changes.end(),wire.begin(),wire.end());auto candidate=assemble(selected);
  if(valid(candidate))body=std::move(candidate);else selected_changes.resize(prior);
 }

 // An identical attestation already included on this branch adds no new
 // participation. Do not spend full transition trials re-including it. Only
 // walk recent ancestors of the selected parent, never competing branches.
 std::set<Bytes> included;
 auto ancestor=head;const auto oldest=slot>t.epoch.ffg.slots_per_epoch?slot-t.epoch.ffg.slots_per_epoch:0;
 while(state_->blocks.at(ancestor).header.slot>=oldest){
  if(check)check();const auto& previous=state_->blocks.at(ancestor);
  const auto saved=signed_block_locked(ancestor,check);
  if(saved){
   const auto& wire=*saved;
   auto u32=[&](size_t at){uint64_t n=0;for(size_t i=0;i<4;++i)n|=uint64_t(wire.at(at+i))<<(8*i);return size_t(n);};
   // The signed archive contains only admitted canonical SSZ encodings.
   const auto body_at=u32(0)+u32(u32(0)+80);
   const auto begin=body_at+u32(body_at+208),end=body_at+u32(body_at+212);
   if(begin>end||end>wire.size())throw std::logic_error("Invalid archived attestation range");
   if(begin<end){
    const auto first=u32(begin);if(!first||first%4||first>end-begin)throw std::logic_error("Invalid archived attestation offsets");
    const auto count=first/4;
    for(size_t i=0;i<count;++i){
     if(check)check();const auto from=begin+u32(begin+4*i),to=i+1<count?begin+u32(begin+4*(i+1)):end;
     if(from<begin+first||to<from||to>end)throw std::logic_error("Invalid archived attestation bounds");
     included.insert(sha256(Bytes(wire.begin()+from,wire.begin()+to)));
    }
   }
  }
  if(ancestor==state_->timed.view.anchor)break;
  ancestor=previous.header.parent_root;
 }
 std::vector<Bytes> candidates;
 for(const auto& [id,wire]:votes_){
  if(check)check();if(candidates.size()>=maximum_vote_trials-tried)break;
  if(included.contains(Bytes(id.begin(),id.end())))continue;
  const auto vote=decode_consensus_attestation_ssz(wire,t.body.validators_per_committee,check);
  if(!vote||vote->data.slot>=slot||slot-vote->data.slot<t.attestation.minimum_inclusion_delay||slot-vote->data.slot>t.epoch.ffg.slots_per_epoch)continue;
  candidates.push_back(wire);
 }
 selected=detail::select_proposal_votes(candidates,t.body.attestations,maximum_vote_trials-tried,
  [&]{if(check)check();},[&](const std::vector<Bytes>& votes){auto candidate=assemble(votes);if(!valid(candidate))return false;body=std::move(candidate);return true;});
 if(check)check();return body;
}
}
