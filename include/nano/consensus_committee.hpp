#pragma once
#include "nano/consensus_deposit.hpp"
#include "nano/core.hpp"
#include <map>
#include <optional>
namespace nano {
struct ConsensusCommitteeMember {
 uint64_t validator_index=0;
 ConsensusDepositMessage possession_message;
 Bytes possession_signature;
 uint64_t effective_weight=0;
};
struct ConsensusParticipation {
 Int signed_weight=0;
 Int total_weight=0;
 size_t signers=0;
};
// Immutable eligibility snapshot. The caller MUST derive members and effective
// weights from authenticated consensus state for the duty, not a network packet.
// Deposit signatures prove key possession only, not funding or eligibility.
// Domain must be selected from the authenticated fork/genesis and duty epoch.
// This boundary verifies indexed participants; it does not finalize blocks.
class ConsensusCommittee {
 struct Member {Bytes public_key;uint64_t weight;};
 std::map<uint64_t,Member> members_;
 Int total_weight_=0;
 ConsensusRoot domain_{};
public:
 ConsensusCommittee(std::span<const ConsensusCommitteeMember> members,
                    const ConsensusBytes4& deposit_version,const ConsensusRoot& duty_domain);
 // Historical snapshots must map a validator index to the same public key.
 bool same_validator_key(uint64_t index,const ConsensusCommittee& other)const;
 std::optional<ConsensusParticipation> verify(std::span<const uint64_t> indices,
                    const ConsensusRoot& object_root,std::span<const uint8_t> signature)const;
};
}
