#pragma once
#include "nano/consensus_capella_slots.hpp"
#include "nano/consensus_fork_state.hpp"
namespace nano {
struct CapellaForkStateContexts {Phase0ForkSlashingContext block;Phase0ForkVoteContext target;CapellaSlotState target_state;};
Phase0ForkSlashingContext capella_fork_block_context(const CapellaSlotState&,const ConsensusBeaconHeader&,
 const CapellaEpochRules&,const AltairStateLimits&,const std::function<void()>& checkpoint={});
Phase0ForkCheckpoints capella_fork_checkpoints(const CapellaSlotState&,const CapellaEpochRules&,
 const AltairStateLimits&,const std::function<void()>& checkpoint={});
// Same authenticated-block precondition as the Phase0 helper; target context
// advances with Capella slots and epochs. No snapshot admission or durable store.
CapellaForkStateContexts prepare_capella_fork_state_contexts(const CapellaSlotState&,
 const ConsensusBeaconHeader&,const ConsensusCheckpoint&,const CapellaEpochRules&,
 const AltairStateLimits&,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});
struct CapellaProposerIdentity {
 uint64_t index;
 ConsensusBytes4 fork_version;
 ConsensusRoot genesis_root;
 Bytes public_key;
 bool operator==(const CapellaProposerIdentity&)const=default;
};
// Ephemeral empty-slot continuation authenticated once against a real block.
// Never substitute state() for that block's signed post-state: its slot/root
// changes. One complete bounded chunk commits locally; cancellation discards
// only the unfinished chunk. Caller serializes access and rebinds on head change.
class CapellaSlotPreparation {
 ConsensusBeaconHeader parent_;
 CapellaEpochRules rules_;
 AltairStateLimits limits_;
 CapellaSlotState state_;
public:
 CapellaSlotPreparation(const CapellaSlotState&,const ConsensusBeaconHeader&,
  const CapellaEpochRules&,const AltairStateLimits&,
  const std::function<void()>& checkpoint={});
 const ConsensusBeaconHeader& parent()const{return parent_;}
 const CapellaSlotState& state()const{return state_;}
 uint64_t slot()const;
 // Derive duties ONLY at the prepared slot. Neither call advances or signs.
 // Caller must match slot()/parent() against its current immutable publication.
 std::optional<CapellaProposerIdentity> proposer_identity(const Phase0FfgRules&,
  const std::function<void()>& checkpoint={})const;
 // Target root is the authenticated parent; prepared slot must be exactly the
 // requested epoch boundary. No fabricated target or implicit rewind/replay.
 Phase0ForkVoteContext vote_context(const ConsensusCheckpoint&,
  const std::function<void()>& checkpoint={})const;
 // Returns true only when target is reached. Zero budget and backwards targets
 // fail, including at completion. No signatures, journal or durable writes.
 bool advance(uint64_t target,uint64_t maximum_slots,uint64_t maximum_sync_trials,
  const std::function<void()>& checkpoint={});
};
// Authenticate the parent, advance to the proposed slot and derive the selected
// proposer from that state. Slashed selection is refused, never rerolled.
std::optional<CapellaProposerIdentity> prepare_capella_proposer_identity(
 const CapellaSlotState&,const ConsensusBeaconHeader&,uint64_t slot,
 const CapellaEpochRules&,const AltairStateLimits&,const Phase0FfgRules&,
 uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});

}
