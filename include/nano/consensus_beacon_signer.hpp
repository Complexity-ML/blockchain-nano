#pragma once
#include "nano/consensus_beacon_header.hpp"
#include "nano/consensus_attestation.hpp"
#include "nano/storage.hpp"
#include <functional>
namespace nano {
// Local anti-slashing guard for ONE BLS key. The provider holds the secret key;
// this class never stores it. No validator-duty or execution authorization is
// implied: callers must authenticate those before requesting a signature.
// Minimal strategy: old slots/targets and decreasing sources are refused, even
// when some old messages could be safe. Fork changes cannot reset the guard.
class BeaconSigningJournal {
public:
 using Signer=std::function<Bytes(const ConsensusRoot&)>;
private:
 // Qualification harness supplies upstream signing roots without a private key.
 friend struct BeaconSigningJournalFixtureAccess;
 friend class AltairDurableReactor;
 friend class BellatrixDurableReactor;
 void reserve_proposal(uint64_t,const ConsensusRoot&); // mutex held
 void reserve_attestation(uint64_t,uint64_t,const ConsensusRoot&); // mutex held
 struct Proposal {uint64_t slot;std::optional<ConsensusRoot> root;bool operator==(const Proposal&)const=default;};
 struct Vote {uint64_t source,target;std::optional<ConsensusRoot> root;bool operator==(const Vote&)const=default;};
 ChainStore& disk_;const Int chain_;const Bytes execution_genesis_,public_key_;
 const ConsensusRoot validators_genesis_;const uint64_t slots_per_epoch_;
 std::mutex& mutex_;std::optional<Proposal> proposal_;std::optional<Vote> vote_;
 std::optional<ForkCheckpointToken> token_;bool unavailable_=false,retired_=false;
 std::string encode(const std::optional<Proposal>&,const std::optional<Vote>&,bool retired)const;
 void commit(const std::optional<Proposal>&,const std::optional<Vote>&,bool retired=false);
 void ready()const;
 Bytes sign(const ConsensusRoot&,const Signer&)const;
public:
 // Empty DB only when expected is absent. Existing DB requires an independently
 // retained exact token. Database must be dedicated to this key and purpose.
 BeaconSigningJournal(ChainStore&,Int chain_id,Bytes execution_genesis,
  ConsensusRoot genesis_validators_root,Bytes public_key,uint64_t slots_per_epoch,
  const std::optional<ForkCheckpointToken>& expected=std::nullopt);
 BeaconSigningJournal(const BeaconSigningJournal&)=delete;
 BeaconSigningJournal& operator=(const BeaconSigningJournal&)=delete;
 ForkCheckpointToken commitment()const;
 bool is_retired()const;
 // Caller must first stop all OTHER users of this key (including copies using
 // independent databases). All instances sharing this ChainStore serialize
 // export/import and callbacks.
 // Permanently retire this database before returning EIP-3076 v5 JSON.
 // Repeated exports are allowed; signing cannot resume from this database.
 // Caller persists the returned JSON and independently retains commitment().
 std::string retire_and_export_interchange();
 // Offline per-key import. Stop other users of this key first. Validate the
 // entire bounded document, merge this key only, then commit atomically.
 // Returns matching key entries (duplicates merged); zero means no match.
 // Never reactivates a retired journal. Caller retains the new commitment.
 size_t import_interchange(std::string_view json,size_t max_bytes);
 Bytes sign_proposal(const ConsensusBeaconHeader&,const ConsensusBytes4& fork_version,const Signer&);
 Bytes sign_attestation(const ConsensusAttestationData&,const ConsensusBytes4& fork_version,const Signer&);
};
}
