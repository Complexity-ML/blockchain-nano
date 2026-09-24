#pragma once
#include "nano/consensus_signing.hpp"
#include <optional>
#include <span>
#include <vector>
namespace nano {
struct ConsensusRandaoRules {uint64_t slots_per_epoch,min_seed_lookahead;};
struct ConsensusRandaoState {
 uint64_t epoch=0;
 std::vector<ConsensusRoot> mixes;
 std::optional<uint64_t> last_reveal_slot;
 bool operator==(const ConsensusRandaoState&)const=default;
};
// Caller MUST authenticate proposer selection/eligibility, fork version and
// genesis root from consensus state and validate the block before committing.
// A BLS epoch signature alone does not establish proposer membership.
// Rejects duplicate/out-of-order slots or wrong epochs without mutation.
bool process_consensus_randao(ConsensusRandaoState& state,const ConsensusRandaoRules& rules,
 uint64_t slot,const ConsensusBytes4& fork_version,const ConsensusRoot& genesis_root,
 std::span<const uint8_t> proposer_key,std::span<const uint8_t> reveal);
// One-epoch rotation; expected epoch guards replay/skips. This is only the
// RANDAO sub-transition, to be committed with other epoch/block state changes.
void advance_consensus_randao_epoch(ConsensusRandaoState& state,const ConsensusRandaoRules& rules,uint64_t expected_epoch);
// Duties for current/next epoch only. Historical duties require their historical
// snapshot; no silent reuse of overwritten ring entries. Domain is the duty type.
ConsensusRoot consensus_randao_seed(const ConsensusRandaoState& state,const ConsensusRandaoRules& rules,
 uint64_t duty_epoch,const ConsensusBytes4& duty_domain);
}
