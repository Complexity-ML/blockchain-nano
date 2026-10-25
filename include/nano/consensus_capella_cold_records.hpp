#pragma once
#include "nano/consensus_capella_store_snapshot.hpp"
#include "nano/consensus_canonical_deposit_context.hpp"
#include <mutex>
namespace nano {
// LOCAL only: references must come from an externally authenticated checkpoint,
// not gossip. Concurrent readers are serialized; outstanding pins stay immutable.
// Lifetime gate for LOCAL checkpoint fetches. Close before destroying backing
// storage. Existing decoded leases remain valid; later fetches fail closed.
// Fetch callbacks must not re-enter this provider or call close().
// A nested operation on another provider refuses rather than waiting when its
// gate is busy; this prevents cross-thread, cross-provider lock cycles.
class CapellaCheckpointProvider {
 struct Shared;
 std::shared_ptr<Shared> shared_;
public:
 using Fetch=std::function<std::optional<std::string>(const Bytes&,size_t)>;
 explicit CapellaCheckpointProvider(Fetch);
 CapellaCheckpointProvider(Fetch,CanonicalDepositContext::FetchLocator);
 CapellaCheckpointProvider(const CapellaCheckpointProvider&)=delete;
 CapellaCheckpointProvider& operator=(const CapellaCheckpointProvider&)=delete;
 ~CapellaCheckpointProvider();
 Fetch reader()const;
 // Only the paired constructor has this capability. Both readers share the
 // same close/reentry gate; a locator is still only an authenticated hint.
 CanonicalDepositContext::FetchLocator locator_reader()const;
 // Wait for either in-flight operation, then retire both backing closures
 // outside the mutex. Concurrent close callers join the same retirement.
 void close();
};
struct CapellaColdRecord {CapellaColdReference reference;CapellaForkBlock block;Phase0ForkNode node;};
struct CapellaColdCacheUsage {size_t records,encoded_bytes;};
class CapellaColdRecordCache {
 struct Entry {std::shared_ptr<const CapellaColdRecord> record;uint64_t used;};
 CapellaForkBlockRules rules_;CapellaStoreSnapshotLimits limits_;
 std::shared_ptr<CanonicalDepositContext> deposit_context_;
 DepositCheckpointTreeBudget deposit_tree_budget_{};
 size_t maximum_records_,maximum_encoded_bytes_,encoded_bytes_=0;uint64_t clock_=0;
 std::map<std::string,Entry> entries_;
 mutable std::recursive_timed_mutex mutex_;bool loading_=false;
public:
 CapellaColdRecordCache(CapellaForkBlockRules,CapellaStoreSnapshotLimits,size_t maximum_records,size_t maximum_encoded_bytes);
 // Explicit v4 decoder. The context must come from the same authenticated
 // descriptor/provider as the store; the old constructor remains v1-v3 only.
 CapellaColdRecordCache(CapellaForkBlockRules,CapellaStoreSnapshotLimits,size_t maximum_records,size_t maximum_encoded_bytes,
  std::shared_ptr<CanonicalDepositContext>,const DepositCheckpointTreeBudget&);
 // Authenticate hash/length/root and fully validate Beacon/EVM/receipt binding.
 // Total cache bytes and per-row limits are independent explicit budgets.
 // Resource pressure throws before eviction/publication. Pinned records cannot
 // be evicted. Encoded-byte accounting is NOT a physical RSS bound.
 std::shared_ptr<const CapellaColdRecord> load(const CapellaColdReference&,
  const std::function<std::optional<std::string>(const Bytes&,size_t)>& fetch,
  const std::function<void()>& checkpoint={});
 // Callbacks may inspect usage; nested load is rejected before mutation.
 CapellaColdCacheUsage usage()const{std::lock_guard lock(mutex_);return {entries_.size(),encoded_bytes_};}
};
}
