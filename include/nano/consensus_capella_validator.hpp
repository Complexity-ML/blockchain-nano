#pragma once
#include "nano/consensus_capella_clock.hpp"
#include <deque>
#include <atomic>
namespace nano {
struct CapellaValidatorKey {
 uint64_t index;
 std::shared_ptr<BeaconSigningJournal> journal;
 BeaconSigningJournal::Signer signer;
 std::string fee_recipient;
 // Nonempty: resolve the index from the authenticated head at each poll.
 Bytes registration_public_key;
};
// One local duty scheduler. Does not supply registry weight or peer time.
// Signed results re-enter normal admission; sync messages remain ephemeral.
// Signing-provider/journal errors stop this scheduler, not consensus admission.
class CapellaValidatorLoop {
 CapellaDurableReactor& reactor_;CapellaLocalClock& clock_;
 const uint64_t slot_ms_,slots_per_epoch_;const size_t transaction_trials_,vote_trials_;
 const std::vector<CapellaValidatorKey> keys_;
 const std::function<uint64_t()> now_;
 std::mutex mutex_,wait_mutex_;std::condition_variable wake_;std::thread thread_;
 uint64_t polls_=0,poll_max_ms_=0,polls_over_slot_=0,proposal_attempts_=0,proposal_max_ms_=0,proposals_unavailable_=0,slot_exits_=0,duty_waits_=0;
 size_t awaiting_registration_=0;std::set<uint64_t> resolved_indices_;
 size_t trace_limit_=0;uint64_t trace_failures_=0;std::deque<Json> duty_trace_;std::optional<uint64_t> traced_slot_;
 void record_duty(uint64_t,const Phase0SignedAttestation&);
 bool stopping_=false;std::atomic<bool> failed_{false};uint64_t proposals_=0,votes_=0,sync_messages_=0,stale_retries_=0;std::string failure_;
public:
 CapellaValidatorLoop(CapellaDurableReactor&,CapellaLocalClock&,uint64_t slot_ms,uint64_t slots_per_epoch,
  size_t maximum_transaction_trials,size_t maximum_vote_trials,std::vector<CapellaValidatorKey>,std::function<uint64_t()> local_clock);
 ~CapellaValidatorLoop();
 void poll(const std::function<void()>& checkpoint={});
 // Nonblocking health observation; private failure text stays under mutex_.
 bool healthy()const noexcept{return !failed_.load(std::memory_order_acquire);}
 Json status(bool include_trace=false);
 // Opt-in diagnostics: one successful local attestation per slot, bounded history.
 // This records a post-signing observation, not an atomic fork-choice decision.
 void set_duty_trace_limit(size_t);
 // Capture after stopping duties; retain externally, never infer trust from disk.
 Json restart_commitments();
 void start(std::chrono::milliseconds interval);
 void stop();
};
}
