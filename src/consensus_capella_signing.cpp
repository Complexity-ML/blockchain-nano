#include "nano/consensus_capella_reactor.hpp"
#include "nano/consensus_capella_duty.hpp"
#include "nano/consensus_capella_deposits.hpp"
#include "nano/consensus_proposer.hpp"
namespace nano {
namespace {
template<size_t N>std::array<uint8_t,N> signing_array(std::span<const uint8_t> bytes,size_t at){std::array<uint8_t,N> out{};std::copy_n(bytes.begin()+at,N,out.begin());return out;}
}
std::optional<Phase0SignedAttestation> CapellaDurableReactor::sign_attestation(uint64_t index,
 uint64_t slot,const std::function<uint64_t()>& local_clock,const ForkCheckpointToken& expected,BeaconSigningJournal& journal,
 const BeaconSigningJournal::Signer& signer,const std::function<void()>& checkpoint){
 std::lock_guard lock(mutex_);require_available();
 if(expected!=*token_)throw StaleCapellaDuty("Stale attestation duty request");
 if(!local_clock||!signer)throw std::invalid_argument("Missing duty clock or signing provider");
 const auto& clock=state_->timed.clock;
 // Per-attempt monotonic observations: remaining in the same slot is not
 // sufficient if the independent wall clock moves backwards during signing.
 uint64_t last_time=clock.time_ms;bool expired=false;
 auto timely=[&]{
  const auto time=local_clock();
  expired=rules_.slot_duration_ms&&time>=last_time&&time>=clock.genesis_time_ms&&(time-clock.genesis_time_ms)/rules_.slot_duration_ms>slot;
  if(!rules_.slot_duration_ms||time<last_time||time<clock.genesis_time_ms||(time-clock.genesis_time_ms)/rules_.slot_duration_ms!=slot)return false;
  last_time=time;return true;
 };
 auto expiry=[&](const char* message){if(expired)throw ExpiredCapellaDuty(message);throw std::runtime_error(message);};
 if(!timely())return std::nullopt;
 auto duty=prepare_capella_attestation_duty(*state_,rules_,index,slot,limits_.replay_slots,limits_.sync_trials,checkpoint);if(!duty)return std::nullopt;
 // Non-duty keys must not reread the complete durable archive on every poll.
 // Eligible duties still verify the disk commitment before any signature.
 disk_.fork_checkpoint(chain_,genesis_,limits_.envelope_bytes,token_);
 if(journal.chain_!=chain_||journal.execution_genesis_!=genesis_||journal.validators_genesis_!=duty->genesis_root||journal.public_key_!=duty->public_key||journal.slots_per_epoch_!=rules_.transition.consensus.epoch.ffg.slots_per_epoch||&journal.disk_==&disk_)
  throw std::invalid_argument("Signing journal does not match authenticated duty");
 if(checkpoint)checkpoint();
 if(!timely())return std::nullopt;
 duty->attestation.signature=journal.sign_attestation(duty->attestation.data,duty->fork_version,[&](const ConsensusRoot& root){
  if(!timely())expiry("Attestation duty expired before provider");
  auto signature=signer(root);
  if(!timely())expiry("Attestation duty expired during provider");
  return signature;
 });
 // The journal verifies BLS after the provider returns. Verification and
 // cancellation handling can outlive the duty too; keep its reservation but
 // never return an already expired or cancelled result.
 if(checkpoint)checkpoint();
 if(!timely())expiry("Attestation duty expired after verification");
 return std::move(duty->attestation);
}
std::optional<Bytes> CapellaDurableReactor::sign_proposal(uint64_t index,uint64_t slot,
 std::span<const uint8_t> body,const std::function<uint64_t()>& local_clock,
 const ForkCheckpointToken& expected,BeaconSigningJournal& journal,
 const BeaconSigningJournal::Signer& signer,const std::function<void()>& checkpoint){
 std::lock_guard lock(mutex_);require_available();
 if(expected!=*token_)throw StaleCapellaDuty("Stale proposal duty request");
 disk_.fork_checkpoint(chain_,genesis_,limits_.envelope_bytes,token_);
 if(!local_clock||!signer)throw std::invalid_argument("Missing proposal clock or signing provider");
 const auto& clock=state_->timed.clock;const auto& t=rules_.transition.consensus;
 validate_altair_transition_rules(t);
 if(!rules_.slot_duration_ms||clock.time_ms<clock.genesis_time_ms)throw std::invalid_argument("Invalid proposal clock");
 if((clock.time_ms-clock.genesis_time_ms)/rules_.slot_duration_ms!=slot||state_->timed.view.current_epoch!=slot/t.epoch.ffg.slots_per_epoch||state_->timed.view.equivocating_indices.contains(index))return std::nullopt;
 uint64_t last_time=clock.time_ms;bool expired=false;
 auto timely=[&]{const auto time=local_clock();expired=time>=last_time&&time>=clock.genesis_time_ms&&(time-clock.genesis_time_ms)/rules_.slot_duration_ms>slot;if(time<last_time||time<clock.genesis_time_ms||(time-clock.genesis_time_ms)/rules_.slot_duration_ms!=slot)return false;last_time=time;return true;};
 auto expiry=[&](const char* message){if(expired)throw ExpiredCapellaDuty(message);throw std::runtime_error(message);};
 if(!timely())return std::nullopt;
 const auto head=phase0_fork_choice_head(state_->timed.view,{t.epoch.ffg.slots_per_epoch,t.attestation.duties.effective_balance_increment,rules_.proposer_score_boost},checkpoint).head;
 const auto& parent=state_->blocks.at(head);
 if(parent.header.slot>=slot)return std::nullopt;
 if(rules_.native_deposit_votes&&(body.size()<168||!capella_native_deposit_vote_known(*state_,head,body.subspan(96,72),checkpoint)))return std::nullopt;
 const auto parent_context=capella_fork_block_context(parent.state,parent.header,CapellaEpochRules{t.epoch,rules_.transition.execution,rules_.transition.maximum_state_bytes},t.state,checkpoint);
 validate_capella_execution_record(parent,rules_,checkpoint);
 if(parent_context.block_root!=head)throw std::invalid_argument("Proposal parent binding mismatch");
 auto candidate=prepare_capella_unsigned_block(parent.state,slot,index,head,body,rules_.transition,parent.execution_block.header,parent.execution_state,[&](const Int& height){return capella_execution_history(*state_,head,height,checkpoint);},limits_.replay_slots,limits_.sync_trials,checkpoint);
 if(!candidate)return std::nullopt;
 const auto& message=candidate->message_ssz;
 const ConsensusBeaconHeader header{slot,index,head,signing_array<32>(message,48),capella_beacon_body_root(std::span<const uint8_t>(message).subspan(84),t.body,t.state.sync_committee_size,rules_.transition.execution,rules_.transition.withdrawals.maximum_per_payload,rules_.transition.maximum_bls_changes,t.maximum_block_bytes,checkpoint)};
 // The post-state is derived locally by the complete operation transition;
 // this checks its header binding and resolves the key/domain from that state.
 const auto context=capella_fork_block_context(candidate->post.consensus,header,CapellaEpochRules{t.epoch,rules_.transition.execution,rules_.transition.maximum_state_bytes},t.state,checkpoint);
 const auto member=context.registry.validators.find(index);
 if(member==context.registry.validators.end())throw std::invalid_argument("Missing derived proposer identity");
 const Bytes key(member->second.public_key.begin(),member->second.public_key.end());
 if(journal.chain_!=chain_||journal.execution_genesis_!=genesis_||journal.validators_genesis_!=context.genesis_root||journal.public_key_!=key||journal.slots_per_epoch_!=t.epoch.ffg.slots_per_epoch||&journal.disk_==&disk_)
  throw std::invalid_argument("Signing journal does not match authenticated proposal");
 const auto epoch=slot/t.epoch.ffg.slots_per_epoch;const auto version=epoch<context.fork.epoch?context.fork.previous_version:context.fork.current_version;
 // Allocate the result before reserving/signing, with only the signature empty.
 Bytes wire(100,0);wire[0]=100;wire.insert(wire.end(),message.begin(),message.end());
 if(checkpoint)checkpoint();if(!timely())return std::nullopt;
 const auto signature=journal.sign_proposal(header,version,[&](const ConsensusRoot& root){
  if(!timely())expiry("Proposal expired before provider");
  auto result=signer(root);
  if(!timely())expiry("Proposal expired during provider");
  return result;
 });
 std::copy(signature.begin(),signature.end(),wire.begin()+4);
 if(checkpoint)checkpoint();if(!timely())expiry("Proposal expired after verification");
 return wire;
}
std::optional<Bytes> CapellaDurableReactor::sign_randao(uint64_t index,uint64_t slot,
 const std::function<uint64_t()>& local_clock,const ForkCheckpointToken& expected,
 BeaconSigningJournal& journal,const BeaconSigningJournal::Signer& signer,const std::function<void()>& checkpoint){
 std::lock_guard lock(mutex_);require_available();
 if(expected!=*token_)throw StaleCapellaDuty("Stale RANDAO duty request");
 if(!local_clock||!signer)throw std::invalid_argument("Missing RANDAO clock or signing provider");
 const auto& clock=state_->timed.clock;const auto& t=rules_.transition.consensus;validate_altair_transition_rules(t);
 if(!rules_.slot_duration_ms||clock.time_ms<clock.genesis_time_ms)throw std::invalid_argument("Invalid RANDAO clock");
 if((clock.time_ms-clock.genesis_time_ms)/rules_.slot_duration_ms!=slot||state_->timed.view.current_epoch!=slot/t.epoch.ffg.slots_per_epoch||state_->timed.view.equivocating_indices.contains(index))return std::nullopt;
 uint64_t last_time=clock.time_ms;bool expired=false;
 auto timely=[&]{const auto time=local_clock();expired=time>=last_time&&time>=clock.genesis_time_ms&&(time-clock.genesis_time_ms)/rules_.slot_duration_ms>slot;if(time<last_time||time<clock.genesis_time_ms||(time-clock.genesis_time_ms)/rules_.slot_duration_ms!=slot)return false;last_time=time;return true;};
 auto expiry=[&](const char* message){if(expired)throw ExpiredCapellaDuty(message);throw std::runtime_error(message);};
 if(!timely())return std::nullopt;
 const auto head=phase0_fork_choice_head(state_->timed.view,{t.epoch.ffg.slots_per_epoch,t.attestation.duties.effective_balance_increment,rules_.proposer_score_boost},checkpoint).head;
 const auto& parent=state_->blocks.at(head);
 if(consensus_beacon_header_root(parent.header)!=head)throw std::invalid_argument("RANDAO parent binding mismatch");
 const auto identity=prepare_capella_proposer_identity(parent.state,parent.header,slot,CapellaEpochRules{t.epoch,rules_.transition.execution,rules_.transition.maximum_state_bytes},t.state,t.attestation.duties,limits_.replay_slots,limits_.sync_trials,checkpoint);
 if(!identity||identity->index!=index)return std::nullopt;
 // Non-duty keys must not reread the complete durable archive on every poll.
 // Eligible duties still verify the disk commitment before any signature.
 disk_.fork_checkpoint(chain_,genesis_,limits_.envelope_bytes,token_);
 if(journal.chain_!=chain_||journal.execution_genesis_!=genesis_||journal.validators_genesis_!=identity->genesis_root||journal.public_key_!=identity->public_key||journal.slots_per_epoch_!=t.epoch.ffg.slots_per_epoch||&journal.disk_==&disk_)
  throw std::invalid_argument("Signing journal does not match authenticated RANDAO duty");
 ConsensusRoot epoch_root{};const auto epoch=slot/t.epoch.ffg.slots_per_epoch;
 for(unsigned i=0;i<8;++i)epoch_root[i]=uint8_t(epoch>>(8*i));
 const auto root=consensus_signing_root(epoch_root,consensus_domain({2,0,0,0},identity->fork_version,identity->genesis_root));
 if(checkpoint)checkpoint();if(!timely())return std::nullopt;
 // Serialize with retirement/import/signing on this key, without fabricating
 // a proposal reservation for an epoch-only RANDAO signature.
 std::lock_guard signing_lock(journal.mutex_);journal.ready();
 if(journal.retired_)throw std::runtime_error("Beacon signing journal permanently retired");
 if(!timely())return std::nullopt;
 auto signature=journal.sign(root,[&](const ConsensusRoot& signing_root){
  auto result=signer(signing_root);
  if(!timely())expiry("RANDAO expired during provider");
  return result;
 });
 if(checkpoint)checkpoint();if(!timely())expiry("RANDAO expired after verification");
 return signature;
}

}
