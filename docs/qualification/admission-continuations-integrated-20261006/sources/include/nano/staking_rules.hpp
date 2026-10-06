#pragma once
#include <cstdint>
#include <stdexcept>
namespace nano {
struct StakingQueryLimits {
 uint64_t per_call_gas=5000000;
 uint64_t sync_gas=50000000;
 uint64_t page_size=0; // Zero selects the historical full-list interface.
 void validate() const {if(!per_call_gas||sync_gas<per_call_gas)throw std::invalid_argument("invalid staking query gas limits");}
};
}
