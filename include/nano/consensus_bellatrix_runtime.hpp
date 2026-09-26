#pragma once
#include "nano/chain.hpp"
#include "nano/beacon_api.hpp"
#include "nano/beacon_sync.hpp"
#include "nano/consensus_bellatrix_clock.hpp"
#include "nano/consensus_bellatrix_validator.hpp"
class BellatrixRpcService;
namespace nano {
// Owns the coordinator and clock; the caller owns Blockchain for their lifetime.
// Aliases share an owner whose clock stops before destroying the coordinator.
struct BellatrixRuntime {
 std::shared_ptr<::BellatrixRpcService> rpc;
 std::shared_ptr<BellatrixValidatorLoop> validator;
 std::shared_ptr<BellatrixDurableReactor> reactor;
 std::shared_ptr<BellatrixLocalClock> clock;
 BeaconReadHandler beacon_read;
 std::shared_ptr<BeaconSyncTracker> sync_tracker;
 BeaconEventsHandler beacon_events;
};
// Local authenticated initial snapshot OR explicit durable restart token.
// No remote snapshots or automatic trust of disk contents. Optional validator
// configuration loads local BLS keys and requires per-key durable journals.
// This starts execution-bound consensus; network registration is separate.
BellatrixRuntime start_bellatrix_runtime(Blockchain&,const Json&,
 const std::function<void()>& checkpoint={},std::function<uint64_t()> local_clock={});
// Caller first stops external admission. Drain workers and stop local duties,
// then capture exact commitments for independently retained restart metadata.
// Throws if the coordinator/journals are unavailable; no disk trust fallback.
Json stop_bellatrix_runtime(BellatrixRuntime&);
}
