#pragma once
#include "nano/consensus_beacon_state.hpp"
#include "nano/consensus_deposit_transition.hpp"
namespace nano {
struct AltairDepositCandidate {Bytes ssz;Phase0DepositOutcome outcome;};
// Altair deposit on an admitted state with authenticated Eth1 deposit root/count.
// New validators additionally receive zero participation flags and inactivity
// score. Top-ups and ignored signatures preserve these lists. Invalid proof or
// consumed index returns nullopt; failures never publish partial state.
// This operation does not authenticate the enclosing block or persist the result.
std::optional<AltairDepositCandidate> prepare_altair_deposit(
 std::span<const uint8_t> state,const Phase0Deposit&,const AltairStateLimits&,
 const Phase0DepositRules&,uint64_t slots_per_epoch,
 const std::function<void()>& checkpoint={});
}
