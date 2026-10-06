#pragma once
#include <array>
#include <cstdint>
namespace nano {
using ConsensusBytes4 = std::array<uint8_t,4>;
using ConsensusRoot = std::array<uint8_t,32>;
// SSZ primitives only. Callers must authenticate the genesis registry root,
// select the protocol version for the signing epoch, and choose the duty domain.
ConsensusRoot consensus_fork_data_root(const ConsensusBytes4& version,const ConsensusRoot& genesis);
ConsensusRoot consensus_domain(const ConsensusBytes4& type,const ConsensusBytes4& version,const ConsensusRoot& genesis);
ConsensusRoot consensus_signing_root(const ConsensusRoot& object,const ConsensusRoot& domain);
}
