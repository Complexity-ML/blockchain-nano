#pragma once
#include "nano/consensus_capella_fork_store.hpp"
namespace nano {
struct CapellaAttestationDuty {
 uint64_t validator_index;
 Bytes public_key;
 ConsensusRoot genesis_root;
 ConsensusBytes4 fork_version;
 Phase0SignedAttestation attestation; // one membership bit, signature absent
};
// Caller-owned observations for ONE advance call, reset at entry. Slot counts
// cover completed ordinary slot chunks; reused counts are state copies, not
// executed transitions. A failing in-flight chunk has unknown partial work.
// Nothing is stored in an existing continuation's object layout.
struct CapellaDutyPreparationWork {
 uint64_t head_slots=0,target_slots=0,block_slots=0;
 uint64_t target_reused_slots=0,block_reused_slots=0;
 bool incomplete_work_unknown=false;
};
// A local continuation owns its complete immutable profile and the actual
// admitted head/boundary. Advanced states never impersonate signed blocks.
class CapellaDutyPreparation {
 CapellaForkBlockRules rules_;
 uint64_t slot_;
 CapellaSlotPreparation head_;
 std::optional<CapellaTargetPreparation> target_;
 std::optional<CapellaBlockPreparation> block_;
 // One unavailable pure projection, bound to this immutable head/profile.
 // A new authenticated parent requires a new continuation; no state/signature
 // is invented while the selected head remains unavailable.
 bool projection_blocked_=false;
public:
 CapellaDutyPreparation(const CapellaForkStore&,const CapellaForkBlockRules&,uint64_t,
  const std::function<void()>& checkpoint={});
 bool matches(const CapellaForkStore&,const CapellaForkBlockRules&,uint64_t,
  const std::function<void()>& checkpoint={})const;
 uint64_t prepared_slot()const{return head_.slot();}
 bool retarget(const CapellaForkStore&,const CapellaForkBlockRules&,uint64_t,const std::function<void()>& checkpoint={});
 void seed_target(const CapellaTargetPreparation&,const CapellaForkStore&,const std::function<void()>& checkpoint={});
 bool advance(uint64_t maximum_slots,uint64_t trials,const std::function<void()>& checkpoint={});
 bool advance(uint64_t maximum_slots,uint64_t trials,const std::function<void()>& checkpoint,
  CapellaDutyPreparationWork* actual_work);
 bool projection_unavailable()const noexcept{return projection_blocked_;}
 bool ready()const;
 const CapellaSlotState& head_state()const;
 CapellaSlotState sync_state(uint64_t trials,const std::function<void()>& checkpoint={})const;
 // Local sync duty availability only. Strict sync_state remains unchanged;
 // callbacks/resource/context errors still propagate, without any signature.
 std::optional<CapellaSlotState> try_sync_state(uint64_t trials,const std::function<void()>& checkpoint={})const;
 const CapellaBlockPreparation* block()const;
 const CapellaTargetPreparation* target_preparation()const;
 std::map<uint64_t,CapellaAttestationDuty> duties(const CapellaForkStore&,
  const std::function<void()>& checkpoint={})const;
 std::optional<Phase0ForkVoteContext> target_context(const CapellaForkStore&,
  const ConsensusCheckpoint&,const std::function<void()>& checkpoint={})const;
};
// Derive the current committee in one state traversal, independent of local key count.
std::map<uint64_t,CapellaAttestationDuty> prepare_capella_attestation_duties(
 const CapellaForkStore&,const CapellaForkBlockRules&,uint64_t expected_slot,
 uint64_t maximum_slots,uint64_t maximum_sync_trials,const std::function<void()>& checkpoint={});
// Derive a current-slot vote from an authenticated Store. Caller authenticates
// the anchor/registry PoPs and serializes the Store. Beacon and execution records remain paired.
// A missing assignment, slashed/equivocating validator, stale slot or premature
// request returns nullopt; invalid trusted context/resource exhaustion throws.
std::optional<CapellaAttestationDuty> prepare_capella_attestation_duty(
 const CapellaForkStore&,const CapellaForkBlockRules&,uint64_t validator_index,
 uint64_t expected_slot,uint64_t maximum_slots,uint64_t maximum_sync_trials,
 const std::function<void()>& checkpoint={});
}
