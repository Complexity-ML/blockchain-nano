#pragma once
#include "nano/beacon_api.hpp"
namespace nano {
// Input must already pass validate_bellatrix_archived_block. All offsets still
// receive bounds checks; this adapter never authenticates or admits a block.
Json beacon_block_json(const Bytes&,const BellatrixForkBlockRules&,const std::function<void()>&);
}
