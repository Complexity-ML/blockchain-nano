#include "nano/consensus_capella_reactor.hpp"
#include "nano/consensus_capella_duty.hpp"
namespace nano {
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
}
