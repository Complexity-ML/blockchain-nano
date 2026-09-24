#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <vector>
namespace nano {
// Consensus primitive only; no activation, seed generation or voting authority.
// rounds belongs to the versioned consensus preset (Ethereum mainnet: 90).
// Scalar lookup avoids allocating a full permutation for each proposer trial.
uint64_t shuffled_validator_index(uint64_t index,uint64_t count,const std::array<uint8_t,32>& seed,uint16_t rounds);
std::vector<uint64_t> shuffled_validator_indices(uint64_t count,const std::array<uint8_t,32>& seed,uint16_t rounds);
std::vector<uint64_t> partition_committee(std::span<const uint64_t> shuffled,uint64_t index,uint64_t count);
}
