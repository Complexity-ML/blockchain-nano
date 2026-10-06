#pragma once
#include "nano/consensus_altair_epoch.hpp"
namespace nano {
struct AltairSyncAggregateRules {
 uint64_t slots_per_epoch,effective_balance_increment,base_reward_factor;
 uint64_t sync_reward_weight,proposer_weight,weight_denominator;
 uint64_t min_seed_lookahead,maximum_effective_balance;
 uint16_t shuffle_rounds;
};
// Signature/committee validation only; no reward or balance transition.
// Caller must authenticate the surrounding state and its chain/branch identity.
bool verify_altair_sync_aggregate(std::span<const uint8_t> state,
 std::span<const uint8_t> aggregate,const AltairStateLimits&,const AltairSyncAggregateRules&,
 const std::function<void()>& checkpoint={});
// Isolated process_sync_aggregate on a validated state at the block slot.
// Derives proposer, membership and signing domain from state. Registry keys
// require PoP and the stored committee must be authenticated at admission.
// Returns private SSZ or nullopt for invalid aggregate encoding/signature.
// No enclosing block authentication, durable commit or replay guard.
std::optional<Bytes> prepare_altair_sync_aggregate(std::span<const uint8_t> state,
 std::span<const uint8_t> aggregate,const AltairStateLimits&,const AltairSyncAggregateRules&,
 const std::function<void()>& checkpoint={});
}
