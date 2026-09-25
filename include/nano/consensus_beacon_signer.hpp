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
 struct Proposal {uint64_t slot;ConsensusRoot root;};
 struct Vote {uint64_t source,target;ConsensusRoot root;};
 ChainStore& disk_;const Int chain_;const Bytes execution_genesis_,public_key_;
 const ConsensusRoot validators_genesis_;const uint64_t slots_per_epoch_;
 mutable std::mutex mutex_;std::optional<Proposal> proposal_;std::optional<Vote> vote_;
 std::optional<ForkCheckpointToken> token_;bool unavailable_=false;
 std::string encode(const std::optional<Proposal>&,const std::optional<Vote>&)const;
 void commit(const std::optional<Proposal>&,const std::optional<Vote>&);
 void ready()const;
 Bytes sign(const ConsensusRoot&,const Signer&)const;
public:
 // Empty DB only when expected is absent. Existing DB requires an independently
 // retained exact token. Database must be dedicated to this key and purpose.
 BeaconSigningJournal(ChainStore&,Int chain_id,Bytes execution_genesis,
  ConsensusRoot genesis_validators_root,Bytes public_key,uint64_t slots_per_epoch,
  const std::optional<ForkCheckpointToken>& expected=std::nullopt);
 ForkCheckpointToken commitment()const;
 Bytes sign_proposal(const ConsensusBeaconHeader&,const ConsensusBytes4& fork_version,const Signer&);
 Bytes sign_attestation(const ConsensusAttestationData&,const ConsensusBytes4& fork_version,const Signer&);
};
}
