#pragma once
#include "nano/consensus_capella_fork_store.hpp"
#include "nano/consensus_altair_snapshot.hpp"
namespace nano {
// Canonical local consensus/execution configuration commitment.
Json capella_fork_profile(const CapellaForkBlockRules&);
// Explicit canonical profile, authenticated before parsing; no inferred defaults.
CapellaForkBlockRules load_capella_fork_profile(std::string_view,const Bytes& expected_digest,size_t maximum_bytes);
struct CapellaStoreSnapshotLimits {
 size_t envelope_bytes,blocks,messages,equivocations,state_bytes,total_state_bytes;
 uint64_t replay_slots,sync_trials;
};
// Process-local validation memo, never serialized or accepted from peers. Entries
// are reused only for byte-equivalent complete records under identical rules.
class CapellaReferenceSnapshotCache;
class CapellaStoreSnapshotCache {
 friend std::string capella_store_snapshot_reference_memo_impl(const CapellaForkStore&,const CapellaForkBlockRules&,
  const CapellaStoreSnapshotLimits&,const std::function<void()>&,CapellaStoreSnapshotCache*,
  const std::function<void(const CapellaColdReference&,std::string_view)>*,CapellaReferenceSnapshotCache*);
 Json profile_;
 std::map<ConsensusRoot,Json> records_;
 friend std::string capella_store_snapshot_impl(const CapellaForkStore&,const CapellaForkBlockRules&,
  const CapellaStoreSnapshotLimits&,const std::function<void()>&,CapellaStoreSnapshotCache*,
  const std::function<void(const CapellaColdReference&,std::string_view)>*);
 friend std::string capella_store_snapshot(const CapellaForkStore&,const CapellaForkBlockRules&,
  const CapellaStoreSnapshotLimits&,const std::function<void()>&,CapellaStoreSnapshotCache*);
};
// Process-local row-validation certificates. Owner-serialized, never persisted
// or supplied by peers. No heavy SSZ/accounts retained; encoded metadata budget
// is distinct from physical RSS. Cached API commits memo changes only on success.
struct CapellaReferenceSnapshotCacheUsage {size_t records,encoded_bytes;uint64_t reuses,validated_cold_rows;};
class CapellaReferenceSnapshotCache {
 struct Entry {CapellaColdReference reference;Json metadata;size_t state_bytes;uint64_t genesis_seconds;size_t encoded_bytes;};
 Json profile_;std::map<ConsensusRoot,Entry> records_;size_t encoded_bytes_=0;uint64_t reuses_=0,validated_cold_rows_=0;
 friend std::string capella_store_snapshot_reference_memo_impl(const CapellaForkStore&,const CapellaForkBlockRules&,
  const CapellaStoreSnapshotLimits&,const std::function<void()>&,CapellaStoreSnapshotCache*,
  const std::function<void(const CapellaColdReference&,std::string_view)>*,CapellaReferenceSnapshotCache*);

 friend CapellaForkStore capella_store_from_reference_snapshot_reusing(std::string_view,const Bytes&,const CapellaForkBlockRules&,
  const CapellaStoreSnapshotLimits&,size_t,size_t,const std::function<std::optional<std::string>(const Bytes&,size_t)>&,
  const CapellaForkStore&,CapellaReferenceSnapshotCache&,const std::function<void()>&,size_t*);
public:
 CapellaReferenceSnapshotCacheUsage usage()const{return {records_.size(),encoded_bytes_,reuses_,validated_cold_rows_};}
};
// Canonical LOCAL checkpoint of an already authenticated Store, including votes,
// branches, clock and all checkpoints. Not peer admission. Caller atomically
// persists bytes and an independently trusted digest/sequence; this API does not
// prevent rollback of that external commitment and does not perform disk I/O.
std::string capella_store_snapshot(const CapellaForkStore&,const CapellaForkBlockRules&,
 const CapellaStoreSnapshotLimits&,const std::function<void()>& checkpoint={},CapellaStoreSnapshotCache* cache=nullptr);
// LOCAL reference-only manifest. Validate one complete row at a time, emit new
// hot rows synchronously, reuse authenticated cold rows without emitting them.
// The sink must stage only: callbacks can precede a later validation failure.
// Per-row/manifest bounds apply; no total flat-state budget or JSON memo is used.
// Caller must commit every referenced blob and manifest atomically. Not runtime
// activation; legacy restoration intentionally does not accept this format.
std::string capella_store_reference_snapshot(const CapellaForkStore&,const CapellaForkBlockRules&,
 const CapellaStoreSnapshotLimits&,
 const std::function<void(const CapellaColdReference&,std::string_view)>& stage,
 const std::function<void()>& checkpoint={});
// Optional LOCAL writer memo: cold digest/length/root, complete retained headers,
// node epochs, justification, anchor role and profile must match a row previously
// validated by this writer. Hot rows always undergo full validation. This does
// not verify current disk availability or replace durable recovery preflight.
std::string capella_store_reference_snapshot_cached(const CapellaForkStore&,const CapellaForkBlockRules&,
 const CapellaStoreSnapshotLimits&,const std::function<void(const CapellaColdReference&,std::string_view)>&,
 CapellaReferenceSnapshotCache&,const std::function<void()>& checkpoint={});
// Authenticate the reference manifest before fetch, validate rows through a
// bounded cache and retain metadata/references only. Fetch is owned by returned
// records: its backing store must outlive them. This is LOCAL trusted recovery,
// not gossip admission. Cache byte budget is distinct from per-row/manifest size.
CapellaForkStore capella_store_from_reference_snapshot(std::string_view,const Bytes& expected_digest,
 const CapellaForkBlockRules&,const CapellaStoreSnapshotLimits&,size_t cache_records,size_t cache_bytes,
 const std::function<std::optional<std::string>(const Bytes&,size_t)>& fetch,
 const std::function<void()>& checkpoint={});
// LOCAL publication preflight only: reuse certificates for unchanged cold rows
// from an already validated source, while fetching and authenticating every blob.
// Misses use the complete decoder; canonical graph validation still runs. Normal
// startup recovery uses the uncached API above. Memo/stats update on success only.
CapellaForkStore capella_store_from_reference_snapshot_reusing(std::string_view,const Bytes&,
 const CapellaForkBlockRules&,const CapellaStoreSnapshotLimits&,size_t cache_records,size_t cache_bytes,
 const std::function<std::optional<std::string>(const Bytes&,size_t)>& fetch,
 const CapellaForkStore& validated_source,CapellaReferenceSnapshotCache&,
 const std::function<void()>& checkpoint={},size_t* reused_records=nullptr);
// Verify external digest before JSON parsing. Rebuild the justified registry
// from authenticated SSZ rather than accepting duplicate serialized weights.
CapellaForkStore capella_store_from_snapshot(std::string_view,const Bytes& expected_digest,
 const CapellaForkBlockRules&,const CapellaStoreSnapshotLimits&,
 const std::function<void()>& checkpoint={});
}
