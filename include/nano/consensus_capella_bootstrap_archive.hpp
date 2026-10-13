#pragma once
#include "nano/chain.hpp"
#include "nano/consensus_capella_fork_store.hpp"
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
namespace nano {
class CapellaDurableReactor;
// Process-local proof of an ALREADY signed first block. No public/default
// constructor, deserializer, signer or Boolean-certificate import exists.
class CapellaSignedBootstrapArchive {
 friend class CapellaDurableReactor;
 friend std::optional<CapellaSignedBootstrapArchive> prepare_capella_signed_bootstrap_archive(
  Genesis,CapellaSlotState,Bytes,CapellaForkBlockRules,uint64_t,uint64_t,const std::function<void()>&);
 Int chain_;Bytes genesis_;Json profile_;
 ConsensusBeaconHeader header_;Bytes wire_;CapellaBlockCandidate post_;
 CapellaSignedBootstrapArchive(Int,Bytes,Json,ConsensusBeaconHeader,Bytes,CapellaBlockCandidate);
 bool matches(const CapellaForkStore&,const CapellaForkBlockRules&,const std::function<void()>&)const;
 CheckpointArchiveEntry entry()const;
public:
 CapellaSignedBootstrapArchive(const CapellaSignedBootstrapArchive&)=default;
 CapellaSignedBootstrapArchive(CapellaSignedBootstrapArchive&&)=default;
 ConsensusBeaconHeader header()const{return header_;}
 Bytes signed_wire()const{return wire_;}
};
// All input values are copied before callbacks. This authenticates the paired
// Genesis execution state and the FULL signed slot0->slot1 transition, with
// the supplied complete profile/limits; it imposes no validator-count constant.
// Production transition rejections return nullopt. Malformed authenticated
// parents, invalid execution/configuration, resource failures and caller
// cancellation propagate unchanged. No state is published or written.
std::optional<CapellaSignedBootstrapArchive> prepare_capella_signed_bootstrap_archive(
 Genesis,CapellaSlotState,Bytes,CapellaForkBlockRules,uint64_t maximum_slots,
 uint64_t maximum_sync_trials,const std::function<void()>& checkpoint={});
// A plan can originate only from the reactor's authenticated, current source.
// Retain BOTH predicted next commitments independently before applying it, so
// an interrupted witness-first write has an exact external recovery authority.
class CapellaBootstrapArchivePlan {
 friend class CapellaDurableReactor;
 CapellaSignedBootstrapArchive proof_;
 std::string snapshot_;
 ForkCheckpointToken primary_before_,witness_before_,primary_after_,witness_after_;
 bool work_;
 CapellaBootstrapArchivePlan(CapellaSignedBootstrapArchive,std::string,
  ForkCheckpointToken,ForkCheckpointToken,ForkCheckpointToken,ForkCheckpointToken,bool);
public:
 CapellaBootstrapArchivePlan(const CapellaBootstrapArchivePlan&)=default;
 CapellaBootstrapArchivePlan(CapellaBootstrapArchivePlan&&)=default;
 bool has_work()const{return work_;}
 ForkCheckpointToken next_primary()const{return primary_after_;}
 ForkCheckpointToken next_witness()const{return witness_after_;}
};
}
