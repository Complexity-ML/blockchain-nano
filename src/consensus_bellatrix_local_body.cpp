#include "nano/consensus_bellatrix_reactor.hpp"
#include "nano/chain.hpp"
#include "nano/consensus_message_ssz.hpp"
namespace nano {
namespace {
uint64_t number(const Bytes& b,size_t at){uint64_t n=0;for(size_t i=0;i<8;++i)n|=uint64_t(b.at(at+i))<<(8*i);return n;}
void offset(Bytes& b,size_t at,size_t n){if(n>UINT32_MAX)throw std::length_error("Bellatrix body offset");for(size_t i=0;i<4;++i)b.at(at+i)=uint8_t(n>>(8*i));}
template<size_t N>void field(std::array<uint8_t,N>& out,const Bytes& in){if(in.size()!=N)throw std::invalid_argument("Bellatrix payload field width");std::copy(in.begin(),in.end(),out.begin());}
BellatrixExecutionPayload payload(const Block& b){
 const auto& h=b.header;BellatrixExecutionPayload p;
 field(p.parent_hash,h.parent_hash);field(p.fee_recipient,unhex(h.coinbase));field(p.state_root,h.state_root);field(p.receipts_root,h.receipts_root);field(p.logs_bloom,h.logs_bloom);field(p.prev_randao,h.mix_hash);field(p.block_hash,h.hash());
 if(!h.base_fee)throw std::invalid_argument("Bellatrix payload missing base fee");auto fee=bytes(*h.base_fee,32);std::reverse(fee.begin(),fee.end());field(p.base_fee_per_gas,fee);
 p.block_number=u64(h.number);p.gas_limit=u64(h.gas_limit);p.gas_used=u64(h.gas_used);p.timestamp=u64(h.timestamp);p.extra_data=h.extra_data;
 for(const auto& tx:b.transactions)p.transactions.push_back(tx.raw());return p;
}
}
std::optional<Bytes> BellatrixDurableReactor::prepare_local_body(uint64_t index,uint64_t slot,
 const Bytes& reveal,const std::string& recipient,const ForkCheckpointToken& expected,size_t maximum_vote_trials,const std::function<void()>& check){
 std::lock_guard lock(mutex_);return prepare_local_body_locked(index,slot,reveal,recipient,expected,maximum_vote_trials,check);
}
std::optional<Bytes> BellatrixDurableReactor::prepare_local_body_locked(uint64_t index,uint64_t slot,
 const Bytes& reveal,const std::string& recipient,const ForkCheckpointToken& expected,size_t maximum_vote_trials,const std::function<void()>& check){
 require_available();
 if(expected!=*token_)throw StaleBellatrixDuty("Stale body preparation");
 if(!execution_||!maximum_vote_trials||reveal.size()!=96)throw std::invalid_argument("Bellatrix local body context");
 if(check)check();const auto& t=rules_.transition.consensus;const auto& clock=state_->timed.clock;
 if((clock.time_ms-clock.genesis_time_ms)/rules_.slot_duration_ms!=slot)return std::nullopt;
 const auto head=phase0_fork_choice_head(state_->timed.view,{t.epoch.ffg.slots_per_epoch,t.attestation.duties.effective_balance_increment,rules_.proposer_score_boost},check).head;
 const auto& parent=state_->blocks.at(head);
 const BellatrixEpochRules epoch{t.epoch,rules_.transition.execution,rules_.transition.maximum_state_bytes};
 const auto identity=prepare_bellatrix_proposer_identity(parent.state,parent.header,slot,epoch,t.state,t.attestation.duties,limits_.replay_slots,limits_.sync_trials,check);
 if(!identity||identity->index!=index)return std::nullopt;
 auto advanced=prepare_bellatrix_slots(parent.state,epoch,t.state,slot,limits_.replay_slots,limits_.sync_trials,check);
 // The complete Bellatrix SSZ layout was validated by prepare_bellatrix_slots.
 const size_t historical=176+64*t.state.slots_per_historical_root,randao=historical+96;
 const uint64_t genesis=number(advanced.ssz,0),seconds=rules_.transition.seconds_per_slot;
 if(!seconds||slot>(UINT64_MAX-genesis)/seconds)throw std::overflow_error("Bellatrix proposal timestamp");
 Block block;auto& h=block.header;h.parent_hash=parent.execution_block.header.hash();h.number=parent.execution_block.header.number+1;h.difficulty=0;
 h.coinbase=address(recipient);h.timestamp=Int(std::to_string(genesis+slot*seconds));h.gas_limit=parent.execution_block.header.gas_limit;h.base_fee=next_base_fee(parent.execution_block.header);
 const size_t mix=randao+32*((slot/t.epoch.ffg.slots_per_epoch)%t.state.epochs_per_historical_vector);h.mix_hash=Bytes(advanced.ssz.begin()+mix,advanced.ssz.begin()+mix+32);
 std::vector<TxPool::Candidate> pending;
 {std::lock_guard chain_lock(execution_->mutex_);execution_->require_execution_ready_locked();pending=execution_->pool_.candidates_for_block(Int(1)<<256);}
 const auto& e=rules_.transition.execution;
 // Reserve the complete configured vote-list capacity before selecting pool
 // transactions; a busy pool must not crowd attestations out of proposals.
 const uint64_t vote_bytes=232+(uint64_t(t.body.validators_per_committee)+8)/8;
 const uint64_t fixed_budget=184+224+(uint64_t(t.state.sync_committee_size)+7)/8+96+508;
 if(t.body.attestations&&vote_bytes>(UINT64_MAX-fixed_budget)/t.body.attestations)throw std::length_error("Bellatrix vote reserve overflow");
 const uint64_t reserved=fixed_budget+uint64_t(t.body.attestations)*vote_bytes;
 if(reserved>=t.maximum_block_bytes)throw std::length_error("Bellatrix block capacity cannot hold configured attestation reserve");
 size_t remaining=t.maximum_block_bytes-reserved;
 std::erase_if(pending,[&](const auto& c){const auto n=c.transaction.raw().size();if(n>e.transaction_bytes||remaining<4||n>remaining-4)return true;remaining-=n+4;return false;});
 if(pending.size()>e.transactions)pending.resize(e.transactions);
 auto history=[&](const Int& n){return bellatrix_execution_history(*state_,head,n,check);};
 ExecutionEngine engine(chain_id_,history,EvmRevision::Paris,rules_.transition.replay_policy);
 auto built=Blockchain::execute_candidate(block,parent.execution_state,std::move(pending),engine.with_checkpoint(check),check,false);
 const auto encoded=encode_bellatrix_execution_payload(payload(built.block),e,t.maximum_block_bytes,check);
 const size_t sync_bytes=(uint64_t(t.state.sync_committee_size)+7)/8+96,fixed=224+sync_bytes;
 auto assemble=[&](const std::vector<Bytes>& votes){
  if(fixed>t.maximum_block_bytes||votes.size()>UINT32_MAX/4)throw std::length_error("Bellatrix proposal body budget");
  Bytes attestations(votes.size()*4);size_t at=0;
  for(const auto& vote:votes){offset(attestations,at,attestations.size());at+=4;if(vote.size()>t.maximum_block_bytes-attestations.size())throw std::length_error("Bellatrix attestations budget");attestations.insert(attestations.end(),vote.begin(),vote.end());}
  Bytes body(fixed);std::copy(reveal.begin(),reveal.end(),body.begin());std::copy_n(advanced.ssz.begin()+historical+4,72,body.begin()+96);
  // Empty sync participation is a valid aggregate (and incurs normal penalties).
  body[220+(uint64_t(t.state.sync_committee_size)+7)/8]=0xc0;
  offset(body,200,fixed);offset(body,204,fixed);offset(body,208,fixed);
  offset(body,212,fixed+attestations.size());offset(body,216,fixed+attestations.size());offset(body,220+sync_bytes,fixed+attestations.size());
  if(attestations.size()>t.maximum_block_bytes-body.size())throw std::length_error("Bellatrix body vote budget");body.insert(body.end(),attestations.begin(),attestations.end());
  if(encoded.size()>t.maximum_block_bytes-body.size())throw std::length_error("Bellatrix body payload budget");body.insert(body.end(),encoded.begin(),encoded.end());return body;
 };
 auto valid=[&](const Bytes& body){return prepare_bellatrix_unsigned_block(parent.state,slot,index,head,body,rules_.transition,parent.execution_block.header,parent.execution_state,history,limits_.replay_slots,limits_.sync_trials,check).has_value();};
 std::vector<Bytes> selected;auto body=assemble(selected);
 if(!valid(body))throw std::runtime_error("Bellatrix proposal needs valid mandatory operations or execution payload");
 // An identical attestation already included on this branch adds no new
 // participation. Do not spend full transition trials re-including it. Only
 // walk recent ancestors of the selected parent, never competing branches.
 std::set<Bytes> included;
 auto ancestor=head;const auto oldest=slot>t.epoch.ffg.slots_per_epoch?slot-t.epoch.ffg.slots_per_epoch:0;
 while(state_->blocks.at(ancestor).header.slot>=oldest){
  if(check)check();const auto& previous=state_->blocks.at(ancestor);
  const auto found=signed_blocks_->find(ancestor);
  if(found!=signed_blocks_->end()){
   const auto& wire=*found->second;
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
 size_t tried=0;
 for(const auto& [id,wire]:*signed_votes_){
  if(check)check();if(selected.size()>=t.body.attestations||tried>=maximum_vote_trials)break;
  if(included.contains(Bytes(id.begin(),id.end())))continue;
  const auto vote=decode_consensus_attestation_ssz(*wire,t.body.validators_per_committee,check);
  if(!vote||vote->data.slot>=slot||slot-vote->data.slot<t.attestation.minimum_inclusion_delay||slot-vote->data.slot>t.epoch.ffg.slots_per_epoch)continue;
  ++tried;selected.push_back(*wire);auto candidate=assemble(selected);
  if(valid(candidate))body=std::move(candidate);else selected.pop_back();
 }
 if(check)check();return body;
}
}
