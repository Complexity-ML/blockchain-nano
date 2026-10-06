#pragma once
#include "nano/consensus_bellatrix_clock.hpp"
#include <deque>
namespace nano {
struct BellatrixValidatorKey {
 uint64_t index;
 std::shared_ptr<BeaconSigningJournal> journal;
 BeaconSigningJournal::Signer signer;
 std::string fee_recipient;
};
// One local duty scheduler. Does not supply registry weight or peer time.
// Every signed result re-enters normal reactor admission and its durable archive.
// Signing-provider/journal errors stop this scheduler, not consensus admission.
class BellatrixValidatorLoop {
 BellatrixDurableReactor& reactor_;BellatrixLocalClock& clock_;
 const uint64_t slot_ms_,slots_per_epoch_;const size_t vote_trials_;
 const std::vector<BellatrixValidatorKey> keys_;
 const std::function<uint64_t()> now_;
 std::mutex mutex_,wait_mutex_;std::condition_variable wake_;std::thread thread_;
 uint64_t polls_=0,poll_max_ms_=0,polls_over_slot_=0,proposal_attempts_=0,proposal_max_ms_=0,proposals_unavailable_=0,slot_exits_=0;
 size_t trace_limit_=0;uint64_t trace_failures_=0;std::deque<Json> duty_trace_;std::optional<uint64_t> traced_slot_;
 void record_duty(uint64_t,const Phase0SignedAttestation&);
 bool stopping_=false,failed_=false;uint64_t proposals_=0,votes_=0,stale_retries_=0;std::string failure_;
public:
 BellatrixValidatorLoop(BellatrixDurableReactor&,BellatrixLocalClock&,uint64_t slot_ms,uint64_t slots_per_epoch,
  size_t maximum_vote_trials,std::vector<BellatrixValidatorKey>,std::function<uint64_t()> local_clock);
 ~BellatrixValidatorLoop();
 void poll(const std::function<void()>& checkpoint={});
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
