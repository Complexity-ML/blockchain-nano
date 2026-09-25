#pragma once
#include "nano/consensus_altair_fork_store.hpp"
#include <filesystem>
namespace nano {
// These commitments come from local trusted configuration, independently of
// the files being loaded. They are not a proof of finality or a freshness policy.
struct AltairAnchorCommitment {
 ConsensusRoot block_root,state_root,genesis_validators_root;
 ConsensusBytes4 fork_version;
};
// Bounded local-file import. Verifies state SSZ, block/state binding, fork domain,
// external roots and existing cross-component rules before returning a private
// candidate. Does not touch storage, accept peer snapshots, select checkpoints,
// verify historical PoP, or advance the clock beyond the anchor slot.
AltairForkStore load_altair_anchor(const std::filesystem::path& state_file,
 const std::filesystem::path& unsigned_block_file,const AltairAnchorCommitment&,
 const AltairForkBlockRules&,size_t maximum_state_bytes,
 const std::function<void()>& checkpoint={});
}
