#include "nano/consensus_beacon_journal_auth.hpp"
#include "nano/consensus_bls.hpp"
#include "consensus_beacon_journal_codec.hpp"
#include <utility>
namespace nano {
AuthenticatedBeaconJournalPair::AuthenticatedBeaconJournalPair(BeaconJournalIdentity id,BeaconJournalReservations p,BeaconJournalReservations c,ForkCheckpointToken pt,ForkCheckpointToken wt,ForkCheckpointToken next,std::optional<ForkCheckpointToken> previous,BeaconJournalPairDisposition state,BeaconJournalFloorObservation pf,BeaconJournalFloorObservation wf)
 :identity_(std::move(id)),primary_(std::move(p)),candidate_(std::move(c)),primary_token_(std::move(pt)),witness_token_(std::move(wt)),next_(std::move(next)),previous_(std::move(previous)),disposition_(state),primary_floor_(pf),witness_floor_(wf){}
AuthenticatedBeaconJournalPair authenticate_beacon_signing_journal_pair(BeaconJournalPairInput input,BeaconJournalIdentity identity,std::function<void()> check){
 using namespace beacon_journal_detail;
 require(identity.slots_per_epoch&&identity.public_key.size()==48);
 require(input.primary.snapshot.size()<=journal_bytes&&input.witness.snapshot.size()<=witness_bytes);if(check)check();
 require(aggregate_consensus_bls_keys(std::vector<Bytes>{identity.public_key}).has_value());
 const auto record=[&](const ForkCheckpointRecord& r,const ForkCheckpointToken& expected,size_t maximum){token_shape(r.token);token_shape(expected);require(r.token==expected&&!r.snapshot.empty()&&r.snapshot.size()<=maximum&&sha256(Bytes(r.snapshot.begin(),r.snapshot.end()))==r.token.digest);};
 record(input.primary,input.current_primary,journal_bytes);record(input.witness,input.current_witness,witness_bytes);
 const auto floor=[&](const ForkCheckpointToken& current,const std::optional<ForkCheckpointToken>& minimum){if(!minimum)return BeaconJournalFloorObservation{};token_shape(*minimum);require(current.sequence>=minimum->sequence);if(current.sequence==minimum->sequence)require(current.digest==minimum->digest);return BeaconJournalFloorObservation{true,current==*minimum};};
 const auto pf=floor(input.current_primary,input.primary_floor),wf=floor(input.current_witness,input.witness_floor);
 const auto primary=decode(input.primary.snapshot,identity);if(check)check();const auto intent=parse(input.witness.snapshot,witness_bytes);
 require(intent.is_object()&&intent.size()==4&&intent.at("format")=="nano-beacon-signing-witness-v1"&&intent.dump()==input.witness.snapshot&&intent.at("snapshot").is_string());
 const auto previous=token(intent.at("previous")),next=token(intent.at("next"));require(next&&(!previous||previous->sequence<UINT64_MAX)&&next->sequence==(previous?previous->sequence+1:1));
 const auto snapshot=intent.at("snapshot").get<std::string>();require(!snapshot.empty()&&snapshot.size()<=journal_bytes&&sha256(Bytes(snapshot.begin(),snapshot.end()))==next->digest);
 const auto candidate=decode(snapshot,identity);if(check)check();
 const bool completed=input.current_primary==*next;
 if(completed)require(input.primary.snapshot==snapshot);else require(previous&&*previous==input.current_primary&&input.current_primary.sequence<UINT64_MAX&&next->sequence==input.current_primary.sequence+1);
 if(!completed){
  // A trusted NEXT still cannot weaken the CURRENT anti-slashing watermark.
  // Interchange merge may conservatively replace a known root with unknown.
  require(!primary.retired||candidate.retired);
  if(primary.proposal){require(candidate.proposal&&candidate.proposal->slot>=primary.proposal->slot);if(candidate.proposal->slot==primary.proposal->slot)require(!candidate.proposal->root||(primary.proposal->root&&candidate.proposal->root==primary.proposal->root));}
  if(primary.attestation){require(candidate.attestation&&candidate.attestation->source>=primary.attestation->source&&candidate.attestation->target>=primary.attestation->target);if(candidate.attestation->target==primary.attestation->target)require(!candidate.attestation->root||(candidate.attestation->source==primary.attestation->source&&primary.attestation->root&&candidate.attestation->root==primary.attestation->root));}
 }
 if(check)check();return {std::move(identity),primary,candidate,input.current_primary,input.current_witness,*next,previous,completed?BeaconJournalPairDisposition::Completed:BeaconJournalPairDisposition::Pending,pf,wf};
}
BeaconJournalRegistrationObservation observe_beacon_journal_registration(AuthenticatedBeaconJournalPair pair,ConsensusSlashingState registry,std::optional<uint64_t> configured,size_t maximum,std::function<void()> check){
 using namespace beacon_journal_detail;require(maximum&&registry.validators.size()<=maximum);const auto identity=pair.identity();std::optional<uint64_t> actual;
 for(const auto& [index,v]:registry.validators){if(check)check();if(Bytes(v.public_key.begin(),v.public_key.end())==identity.public_key){require(!actual);actual=index;}}
 if(configured)require(actual&&*actual==*configured);if(check)check();return {actual};
}
}
