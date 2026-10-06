#pragma once
#include "nano/core.hpp"
#include "nano/consensus_signing.hpp"
namespace nano {
struct CapellaSyncMessage {uint64_t slot=0,validator_index=0;ConsensusRoot beacon_block_root{};std::array<uint8_t,96> signature{};};
// Fixed SSZ SyncCommitteeMessage: slot, block root, validator index, signature.
// Encoding only; admission checks time, branch, membership and signature.
Bytes encode_capella_sync_message(const CapellaSyncMessage&);
std::optional<CapellaSyncMessage> decode_capella_sync_message(std::span<const uint8_t>);
}
