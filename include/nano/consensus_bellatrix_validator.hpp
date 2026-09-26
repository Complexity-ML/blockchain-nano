#pragma once
#include "nano/consensus_bellatrix_clock.hpp"
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
 bool stopping_=false,failed_=false;uint64_t proposals_=0,votes_=0,stale_retries_=0;std::string failure_;
public:
 BellatrixValidatorLoop(BellatrixDurableReactor&,BellatrixLocalClock&,uint64_t slot_ms,uint64_t slots_per_epoch,
  size_t maximum_vote_trials,std::vector<BellatrixValidatorKey>,std::function<uint64_t()> local_clock);
 ~BellatrixValidatorLoop();
 void poll(const std::function<void()>& checkpoint={});
 Json status();
 // Capture after stopping duties; retain externally, never infer trust from disk.
 Json restart_commitments();
 void start(std::chrono::milliseconds interval);
 void stop();
};
}
