#pragma once
#include "nano/consensus_capella_fork_store.hpp"
namespace nano {
struct CapellaSyncBlock {ConsensusRoot root;Bytes ssz;};
// Structural scheduling only, never consensus admission. Caller supplies trusted
// rules and known local roots. Returns indexes into the original batch without
// copying payloads. Every scheduled block still needs full reactor admission.
std::vector<size_t> order_capella_sync_blocks(std::span<const CapellaSyncBlock>,
 const std::set<ConsensusRoot>& known,const CapellaForkBlockRules&,size_t maximum_blocks,
 size_t maximum_payload_bytes,const std::function<void()>& checkpoint={});
}
