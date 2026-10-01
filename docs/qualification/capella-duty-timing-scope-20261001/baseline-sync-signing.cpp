#include "nano/consensus_capella_reactor.hpp"
#include "consensus_capella_projection.hpp"
namespace nano {
namespace {
uint64_t sync_number(const Bytes& b,size_t at){uint64_t n=0;for(size_t i=0;i<8;++i)n|=uint64_t(b.at(at+i))<<(8*i);return n;}
template<size_t N>std::array<uint8_t,N> sync_field(const Bytes& b,size_t at){std::array<uint8_t,N> out{};std::copy_n(b.begin()+at,N,out.begin());return out;}
}
const CapellaDurableReactor::SyncSigningContext& CapellaDurableReactor::sync_signing_context_locked(uint64_t slot,const std::function<void()>& check){
 const auto& t=rules_.transition;const auto& c=t.consensus;const auto& l=c.state;
 const auto head=phase0_fork_choice_head(state_->timed.view,{c.epoch.ffg.slots_per_epoch,c.attestation.duties.effective_balance_increment,rules_.proposer_score_boost},check).head;
 if(sync_signing_context_&&sync_signing_context_->slot==slot&&sync_signing_context_->parent==head)return *sync_signing_context_;
 if(slot==UINT64_MAX)throw std::overflow_error("Sync duty slot overflow");
 const auto& parent=state_->blocks.at(head);
 const auto advanced=prepare_capella_slots(parent.state,{c.epoch,t.execution,t.maximum_state_bytes},l,slot+1,limits_.replay_slots,limits_.sync_trials,check);
 const auto& bytes=advanced.ssz;const size_t historical=176+64*l.slots_per_historical_root;
 SyncSigningContext candidate{slot,head,sync_field<32>(bytes,176+32*(slot%l.slots_per_historical_root)),sync_field<32>(bytes,8),sync_field<4>(bytes,slot/c.sync.slots_per_epoch<sync_number(bytes,56)?48:52),{}};
 const size_t committee=historical+96+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector+133;
 std::set<Bytes> members;for(size_t i=0;i<l.sync_committee_size;++i){if(check)check();members.emplace(bytes.begin()+committee+48*i,bytes.begin()+committee+48*(i+1));}
 const auto validators=capella_detail::offset(bytes,historical+88),balances=capella_detail::offset(bytes,historical+92);
 for(size_t i=0;i<(balances-validators)/121;++i){if(check)check();Bytes key(bytes.begin()+validators+121*i,bytes.begin()+validators+121*i+48);if(members.contains(key))candidate.members.emplace(i,std::move(key));}
 if(check)check();sync_signing_context_=std::move(candidate);return *sync_signing_context_;
}
std::set<uint64_t> CapellaDurableReactor::current_sync_members(uint64_t slot,const std::function<void()>& check){
 std::lock_guard lock(mutex_);require_available();const auto& clock=state_->timed.clock;
 if(slot==UINT64_MAX||(clock.time_ms-clock.genesis_time_ms)/rules_.slot_duration_ms!=slot)return {};
 const auto& context=sync_signing_context_locked(slot,check);std::set<uint64_t> result;
 const auto saved=pending_sync_messages_.find(slot+1);
 for(const auto& [index,key]:context.members){if(check)check();if(saved!=pending_sync_messages_.end()&&saved->second.parent==context.parent&&saved->second.wires.contains(index))continue;result.insert(index);}return result;
}
std::optional<CapellaSyncMessage> CapellaDurableReactor::sync_current(uint64_t index,uint64_t slot,const std::function<uint64_t()>& local_clock,BeaconSigningJournal& journal,const BeaconSigningJournal::Signer& signer,const std::function<void()>& check,const Bytes& registration_public_key){
 std::lock_guard lock(mutex_);require_available();check_registered_index_locked(index,registration_public_key,journal,check);
 if(!local_clock||!signer)throw std::invalid_argument("Missing sync duty clock or signer");
 const auto& clock=state_->timed.clock;if(slot==UINT64_MAX||(clock.time_ms-clock.genesis_time_ms)/rules_.slot_duration_ms!=slot)return std::nullopt;
 uint64_t last=clock.time_ms;size_t reads=0;const char* phase="entry";
 auto timely=[&]{
  const auto now=local_clock();++reads;
  const bool before_last=now<last,before_genesis=now<clock.genesis_time_ms;
  if(before_last||before_genesis)throw std::runtime_error("Sync duty clock regressed: nowMs="+std::to_string(now)+
   ", lastMs="+std::to_string(last)+", durableMs="+std::to_string(clock.time_ms)+
   ", genesisMs="+std::to_string(clock.genesis_time_ms)+", slot="+std::to_string(slot)+
   ", phase="+phase+", read="+std::to_string(reads)+
   ", beforeLast="+std::to_string(before_last)+", beforeGenesis="+std::to_string(before_genesis));
  if((now-clock.genesis_time_ms)/rules_.slot_duration_ms!=slot)throw ExpiredCapellaDuty("Sync duty slot expired");last=now;
 };
 timely();const auto& context=sync_signing_context_locked(slot,check);
 const auto member=context.members.find(index);if(member==context.members.end())return std::nullopt;
 const auto& parent=state_->blocks.at(context.parent);
 // A received current-slot block or one third of a skipped slot triggers sync.
 if(parent.header.slot<slot&&(last-clock.genesis_time_ms)%rules_.slot_duration_ms<(rules_.slot_duration_ms/3+uint64_t(rules_.slot_duration_ms%3!=0)))return std::nullopt;
 const auto saved=pending_sync_messages_.find(slot+1);if(saved!=pending_sync_messages_.end()&&saved->second.parent==context.parent&&saved->second.wires.contains(index))return std::nullopt;
 disk_.fork_checkpoint(chain_,genesis_,limits_.envelope_bytes,token_);
 if(journal.chain_!=chain_||journal.execution_genesis_!=genesis_||journal.validators_genesis_!=context.genesis_root||journal.public_key_!=member->second||journal.slots_per_epoch_!=rules_.transition.consensus.epoch.ffg.slots_per_epoch||&journal.disk_==&disk_)throw std::invalid_argument("Signing journal does not match authenticated sync duty");
 CapellaSyncMessage message{};message.slot=slot;message.validator_index=index;message.beacon_block_root=context.block_root;
 const auto root=consensus_signing_root(message.beacon_block_root,consensus_domain({7,0,0,0},context.version,context.genesis_root));
 auto guarded=[&]{if(check)check();timely();};phase="before-provider";guarded();
 {
  // Sync signatures have no proposal/attestation slashing reservation. Still
  // serialize key retirement/import and authenticate the provider's signature.
  std::lock_guard signing_lock(journal.mutex_);journal.ready();if(journal.retired_)throw std::runtime_error("Beacon signing journal permanently retired");guarded();
  const auto signature=journal.sign(root,[&](const ConsensusRoot& r){auto result=signer(r);phase="after-provider";guarded();return result;});std::copy(signature.begin(),signature.end(),message.signature.begin());
 }
 phase="after-verification";guarded();phase="admission";if(!submit_sync_message_locked(message,*token_,guarded))throw std::runtime_error("Locally signed sync message refused by admission");return message;
}
}
