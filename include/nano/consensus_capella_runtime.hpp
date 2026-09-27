#pragma once
#include "nano/chain.hpp"
#include "nano/beacon_api.hpp"
#include "nano/consensus_capella_validator.hpp"
class CapellaConsensusService;
class CapellaInventorySessions;
namespace nano {
// Shared lifetime: workers stop before coordinator, journals and witness close.
// Caller owns Blockchain for the lifetime of all returned aliases.
struct CapellaRuntime {
 std::shared_ptr<CapellaValidatorLoop> validator;
 std::shared_ptr<CapellaDurableReactor> reactor;
 std::shared_ptr<CapellaLocalClock> clock;
 BeaconReadHandler beacon_read;
 std::shared_ptr<::CapellaConsensusService> admission;
 std::shared_ptr<::CapellaInventorySessions> catalogs;
 Json network=Json::object();
};
// Locally authenticated profile + initial snapshot, exact restart token, or
// explicitly trusted recovery witness. This does not register P2P/HTTP/CLI.
CapellaRuntime start_capella_runtime(Blockchain&,const Json&,
 const std::function<void()>& checkpoint={},std::function<uint64_t()> local_clock={});
// Caller stops external admission first. Drain duties and clock before receipts.
Json stop_capella_runtime(CapellaRuntime&);
}
