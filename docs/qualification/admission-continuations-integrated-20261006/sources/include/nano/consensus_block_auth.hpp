#pragma once
#include "nano/consensus_beacon_body.hpp"
#include "nano/consensus_beacon_header.hpp"
#include "nano/consensus_proposer.hpp"
namespace nano {
struct Phase0BlockContext {
 uint64_t slot;
 ConsensusRoot parent_root;
 ConsensusBytes4 fork_version;
 ConsensusRoot genesis_root;
 uint64_t maximum_effective_balance;
 uint16_t shuffle_rounds;
 size_t maximum_serialized_bytes;
};
// A provisional authentication result, NOT an accepted block or post-state.
// Operation semantics and the advertised state_root remain unvalidated.
struct AuthenticatedPhase0Block {
 ConsensusBeaconHeader header;
 ConsensusRandaoState randao_after;
};
// Requires an authenticated context and funded registry/RANDAO advanced to the
// expected slot's epoch. Does not advance epochs or mutate either input.
// Rejects malformed SSZ, wrong parent/slot, signature, duty or RANDAO with nullopt.
// Trusted configuration errors and cancellation propagate as exceptions.
// Caller must validate all operations and state_root before one durable commit;
// never publish randao_after as canonical merely because this gate succeeds.
std::optional<AuthenticatedPhase0Block> authenticate_phase0_block(
 std::span<const uint8_t> signed_block,const Phase0BlockContext& context,
 const Phase0BodyLimits& body_limits,const ConsensusRandaoRules& randao_rules,
 const ConsensusSlashingState& registry,const ConsensusRandaoState& randao,
 const std::function<void()>& checkpoint={});
}
