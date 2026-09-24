#pragma once
#include "nano/consensus_signing.hpp"
#include <span>
namespace nano {
// Beacon consensus container, distinct from Nano's execution-layer RLP header.
struct ConsensusBeaconHeader {
 uint64_t slot{},proposer_index{};
 ConsensusRoot parent_root{},state_root{},body_root{};
 bool operator==(const ConsensusBeaconHeader&)const=default;
};
ConsensusRoot consensus_beacon_header_root(const ConsensusBeaconHeader& header);
// Signature only: caller authenticates registry key, fork and genesis, computes
// body_root, and separately validates proposer duty, ancestry and state transition.
bool verify_consensus_beacon_header(const ConsensusBeaconHeader& header,
 const ConsensusBytes4& fork_version,const ConsensusRoot& genesis_root,
 std::span<const uint8_t> public_key,std::span<const uint8_t> signature);
}
