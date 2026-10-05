#pragma once
#include "nano/consensus_altair_transition.hpp"
#include "nano/consensus_capella_slots.hpp"
#include "nano/consensus_capella_fork_state.hpp"
#include "nano/consensus_capella_fork.hpp"
#include "nano/consensus_capella_execution.hpp"
namespace nano {
struct CapellaTransitionRules {
 AltairTransitionRules consensus; // common operations, Capella penalty values
 ExecutionPayloadLimits execution;
 size_t maximum_state_bytes;
 uint64_t seconds_per_slot;
 CapellaWithdrawalRules withdrawals;
 ConsensusBytes4 genesis_version;
 uint32_t maximum_bls_changes;
 Int chain_id;
 ReplayPolicy replay_policy;
};
struct CapellaBlockCandidate {
 CapellaSlotState consensus;
 BellatrixExecutionCandidate execution;
};
// Full signed post-merge Capella block, starting from authenticated parents.
// Includes slots/epochs, BLS/proposer/parent, EVM, RANDAO, all body operations,
// sync aggregate and advertised post-state root. Never publishes either state.
// Invalid envelope/consensus operations -> nullopt. Invalid execution, invalid
// configuration, budget exhaustion, unavailable history or cancellation throw.
// Terminal PoW and fork activation are outside this post-merge API.
std::optional<CapellaBlockCandidate> prepare_capella_state_transition(
 const CapellaSlotState&,std::span<const uint8_t> signed_block,
 const CapellaTransitionRules&,const Header& execution_parent,const State& execution_state,
 const std::function<Bytes(const Int&)>& authenticated_block_hash,
 uint64_t maximum_slots,uint64_t maximum_sync_trials,const std::function<void()>& checkpoint={});
struct CapellaUnsignedBlockCandidate {Bytes message_ssz;CapellaBlockCandidate post;};
// Construction only; NOT admission or authorization to sign. The signing
// reactor must bind duty/head/time and reserve the anti-slashing journal.
std::optional<CapellaUnsignedBlockCandidate> prepare_capella_unsigned_block(
 const CapellaSlotState&,uint64_t slot,uint64_t proposer_index,
 const ConsensusRoot& parent_root,std::span<const uint8_t> body,
 const CapellaTransitionRules&,const Header& execution_parent,const State& execution_state,
 const std::function<Bytes(const Int&)>& authenticated_block_hash,
 uint64_t maximum_slots,uint64_t maximum_sync_trials,const std::function<void()>& checkpoint={});
// Typed, ephemeral preparation for a block after skipped slots. Owns the exact
// transition profile and an authenticated parent. No signing/admission or writes.
// Caller must bind parent() to the current admitted record and authenticate the
// paired execution parent/state/history. Recreate on branch/profile changes.
class CapellaBlockPreparation {
 CapellaTransitionRules rules_;
 CapellaSlotPreparation slots_;
 void require_next_slot(uint64_t)const;
public:
 CapellaBlockPreparation(const CapellaSlotState&,const ConsensusBeaconHeader&,
  const CapellaTransitionRules&,const std::function<void()>& checkpoint={});
 const ConsensusBeaconHeader& parent()const{return slots_.parent();}
 const CapellaSlotState& prepared_state()const{return slots_.state();}
 uint64_t prepared_slot()const{return slots_.slot();}
 // Prepare only through block_slot-1. The existing full block transition always
 // processes the final slot normally, including an epoch boundary if applicable.
 bool advance_for_block(uint64_t block_slot,uint64_t maximum_slots,
  uint64_t maximum_sync_trials,const std::function<void()>& checkpoint={});
 std::optional<CapellaProposerIdentity> proposer_for_block(uint64_t block_slot,
  uint64_t maximum_sync_trials,const std::function<void()>& checkpoint={})const;
 std::optional<CapellaUnsignedBlockCandidate> construct(uint64_t block_slot,
  uint64_t proposer_index,std::span<const uint8_t> body,const Header&,const State&,
  const std::function<Bytes(const Int&)>&,uint64_t maximum_sync_trials,
  const std::function<void()>& checkpoint={})const;
 std::optional<CapellaBlockCandidate> receive(std::span<const uint8_t> signed_block,
  const Header&,const State&,const std::function<Bytes(const Int&)>&,
  uint64_t maximum_sync_trials,const std::function<void()>& checkpoint={})const;
};
// First Capella block from an authenticated Bellatrix state. These compose the
// scheduled slot migration and the same signed admission/construction paths.
std::optional<CapellaBlockCandidate> prepare_capella_fork_state_transition(
 const BellatrixSlotState&,const ConsensusRoot&,const CapellaForkSchedule&,
 std::span<const uint8_t>,const CapellaTransitionRules&,const Header&,const State&,
 const std::function<Bytes(const Int&)>&,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});
std::optional<CapellaUnsignedBlockCandidate> prepare_capella_fork_unsigned_block(
 const BellatrixSlotState&,const ConsensusRoot&,const CapellaForkSchedule&,
 uint64_t slot,uint64_t proposer_index,const ConsensusRoot& parent_root,std::span<const uint8_t> body,
 const CapellaTransitionRules&,const Header&,const State&,
 const std::function<Bytes(const Int&)>&,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});

}
