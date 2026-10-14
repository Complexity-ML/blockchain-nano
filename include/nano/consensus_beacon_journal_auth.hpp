#pragma once
#include "nano/consensus_beacon_header.hpp"
#include "nano/consensus_slashing.hpp"
#include "nano/storage.hpp"
#include <functional>
#include <optional>
#include <cstdint>
namespace nano {
struct BeaconJournalIdentity {
 Bytes public_key;ConsensusRoot genesis_validators_root{};uint64_t slots_per_epoch=0;
};
struct BeaconJournalProposal {uint64_t slot;std::optional<ConsensusRoot> root;};
struct BeaconJournalAttestation {uint64_t source,target;std::optional<ConsensusRoot> root;};
struct BeaconJournalReservations {
 std::optional<BeaconJournalProposal> proposal;
 std::optional<BeaconJournalAttestation> attestation;
 bool retired=false;
};
struct BeaconJournalPairInput {
 ForkCheckpointRecord primary,witness;
 ForkCheckpointToken current_primary,current_witness;
 std::optional<ForkCheckpointToken> primary_floor,witness_floor;
};
enum class BeaconJournalPairDisposition {Completed,Pending};
struct BeaconJournalFloorObservation {bool supplied=false,current_equals_floor=false;};
// Pure observation, never signing/recovery authority. Namespace/chain identity
// and current-pin provenance MUST be bound externally on sealed RO handles.
// Journals contain reservations, not delivered signatures or EVM nonces.
class AuthenticatedBeaconJournalPair {
 const BeaconJournalIdentity identity_;
 const BeaconJournalReservations primary_,candidate_;
 const ForkCheckpointToken primary_token_,witness_token_,next_;
 const std::optional<ForkCheckpointToken> previous_;
 const BeaconJournalPairDisposition disposition_;
 const BeaconJournalFloorObservation primary_floor_,witness_floor_;
 AuthenticatedBeaconJournalPair(BeaconJournalIdentity,BeaconJournalReservations,
  BeaconJournalReservations,ForkCheckpointToken,ForkCheckpointToken,
  ForkCheckpointToken,std::optional<ForkCheckpointToken>,BeaconJournalPairDisposition,
  BeaconJournalFloorObservation,BeaconJournalFloorObservation);
 friend AuthenticatedBeaconJournalPair authenticate_beacon_signing_journal_pair(
  BeaconJournalPairInput,BeaconJournalIdentity,std::function<void()>);
public:
 BeaconJournalIdentity identity()const{return identity_;}
 BeaconJournalReservations primary()const{return primary_;}
 BeaconJournalReservations candidate()const{return candidate_;}
 ForkCheckpointToken primary_token()const{return primary_token_;}
 ForkCheckpointToken witness_token()const{return witness_token_;}
 ForkCheckpointToken next_token()const{return next_;}
 std::optional<ForkCheckpointToken> previous_token()const{return previous_;}
 BeaconJournalPairDisposition disposition()const{return disposition_;}
 BeaconJournalFloorObservation primary_floor()const{return primary_floor_;}
 BeaconJournalFloorObservation witness_floor()const{return witness_floor_;}
 // A numeric lower bound never establishes historical digest ancestry.
 bool historical_digest_ancestry_verified()const{return false;}
 bool delivered_signature_history_verified()const{return false;}
};
// Owned copies precede callbacks. Fixed existing bounds: 4096B journal and
// 16384B witness. Malformed inputs throw; cancellation exceptions propagate.
AuthenticatedBeaconJournalPair authenticate_beacon_signing_journal_pair(
 BeaconJournalPairInput,BeaconJournalIdentity,std::function<void()> checkpoint={});
struct BeaconJournalRegistrationObservation {std::optional<uint64_t> index;};
// Caller supplies the FULL registry from an authenticated current head context.
// Fixed index requires exact key match; automatic lookup reports absent keys.
// Registered is not active/duty/signing eligibility. No count/index defaults.
BeaconJournalRegistrationObservation observe_beacon_journal_registration(
 AuthenticatedBeaconJournalPair,ConsensusSlashingState current_registry,
 std::optional<uint64_t> configured_index,size_t maximum_registry_entries,
 std::function<void()> checkpoint={});
}
