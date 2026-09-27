#pragma once
#include "nano/consensus_bellatrix_reactor.hpp"
#include "nano/consensus_capella_reactor.hpp"
#include <string_view>
namespace nano {
struct BeaconReadResponse {unsigned status;Json body;Bytes ssz;std::string version;};
using BeaconReadHandler=std::function<BeaconReadResponse(std::string_view,const std::function<void()>&)>;
// Read-only projection of one authenticated published store. Unsupported routes
// are explicit 404s. This is not a full Beacon API or an admission interface.
BeaconReadResponse bellatrix_beacon_read(const BellatrixForkStore&,
 const BellatrixForkBlockRules&,std::string_view,const std::function<void()>& checkpoint={});
BeaconReadResponse bellatrix_beacon_read(const BellatrixPublishedStore&,
 const BellatrixForkBlockRules&,std::string_view,const std::function<void()>& checkpoint={});
BeaconReadResponse capella_beacon_read(const CapellaForkStore&,
 const CapellaForkBlockRules&,std::string_view,const std::function<void()>& checkpoint={});
}
