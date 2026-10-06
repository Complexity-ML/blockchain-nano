#pragma once
#include "nano/consensus_altair_reactor.hpp"
#include "nano/consensus_altair_anchor.hpp"
class AltairRpcService;
namespace nano {
// Semantic compatibility commitment; excludes local paths, worker/storage budgets,
// checkpoint sequence and wall clock. Not a proof of peer honesty or finality.
Bytes altair_consensus_identity(const AltairForkBlockRules&,const AltairAnchorCommitment&,
 const Int& chain_id,const Bytes& execution_genesis);
// Both aliases retain the owner of the service, reactor and database. Service
// destruction drains the worker before destroying the reactor/database.
struct AltairRuntime {
 std::shared_ptr<::AltairRpcService> rpc;
 std::shared_ptr<AltairDurableReactor> reactor;
};
// Explicit local configuration. A null restart token means first initialization
// ONLY; a non-null token must match the existing checkpoint exactly. No fallback
// to a fresh Store, automatic checkpoint trust, signing or execution binding.
AltairRuntime start_altair_runtime(const Json&,const Int& chain_id,const Bytes& execution_genesis,
 const std::function<void()>& checkpoint={},std::function<uint64_t()> local_clock={});
}
