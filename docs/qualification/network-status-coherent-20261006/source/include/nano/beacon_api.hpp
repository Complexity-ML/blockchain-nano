#pragma once
#include "nano/consensus_bellatrix_reactor.hpp"
#include "nano/consensus_capella_reactor.hpp"
#include <string_view>
namespace nano {
enum class BeaconReadPhase {publication,selection,fork_choice,state_commitment,projection,archive,ancestry};
inline const char* beacon_read_phase_name(BeaconReadPhase phase){
 switch(phase){case BeaconReadPhase::publication:return "publication-pin";case BeaconReadPhase::selection:return "state-selection";case BeaconReadPhase::fork_choice:return "fork-choice";case BeaconReadPhase::state_commitment:return "state-commitment";case BeaconReadPhase::projection:return "response-projection";case BeaconReadPhase::archive:return "signed-archive";case BeaconReadPhase::ancestry:return "ancestry";}return "unknown";
}
struct BeaconReadCancelled : ExecutionCancelled {
 BeaconReadPhase phase;uint64_t elapsed_ms;
 BeaconReadCancelled(BeaconReadPhase p,uint64_t elapsed):ExecutionCancelled("Beacon read cancelled"),phase(p),elapsed_ms(elapsed){}
};
struct BeaconReadResponse {unsigned status;Json body;Bytes ssz;std::string version;};
using BeaconReadHandler=std::function<BeaconReadResponse(std::string_view,const std::function<void()>&)>;
// Read-only projection of one authenticated published store. Unsupported routes
// are explicit 404s. This is not a full Beacon API or an admission interface.
BeaconReadResponse bellatrix_beacon_read(const BellatrixForkStore&,
 const BellatrixForkBlockRules&,std::string_view,const std::function<void()>& checkpoint={});
BeaconReadResponse bellatrix_beacon_read(const BellatrixPublishedStore&,
 const BellatrixForkBlockRules&,std::string_view,const std::function<void()>& checkpoint={});
BeaconReadResponse capella_beacon_read(const CapellaForkStore&,
 const CapellaForkBlockRules&,std::string_view,const std::function<void()>& checkpoint={},
 const std::function<void(BeaconReadPhase)>& phase={});
}
