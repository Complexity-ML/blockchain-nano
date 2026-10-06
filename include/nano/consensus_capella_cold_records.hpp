#pragma once
#include "nano/consensus_capella_store_snapshot.hpp"
namespace nano {
// LOCAL only: references must come from an externally authenticated checkpoint,
// not gossip. Cache is serialized by its owner and retains all outstanding pins.
struct CapellaColdReference {
 ConsensusRoot root;Bytes digest;size_t bytes;bool anchor;
 bool operator==(const CapellaColdReference&)const=default;
};
struct CapellaColdRecord {CapellaColdReference reference;CapellaForkBlock block;Phase0ForkNode node;};
struct CapellaColdCacheUsage {size_t records,encoded_bytes;};
class CapellaColdRecordCache {
 struct Entry {std::shared_ptr<const CapellaColdRecord> record;uint64_t used;};
 CapellaForkBlockRules rules_;CapellaStoreSnapshotLimits limits_;
 size_t maximum_records_,maximum_encoded_bytes_,encoded_bytes_=0;uint64_t clock_=0;
 std::map<std::string,Entry> entries_;
public:
 CapellaColdRecordCache(CapellaForkBlockRules,CapellaStoreSnapshotLimits,size_t maximum_records,size_t maximum_encoded_bytes);
 // Authenticate hash/length/root and fully validate Beacon/EVM/receipt binding.
 // Resource pressure throws before eviction/publication. Pinned records cannot
 // be evicted. Encoded-byte accounting is NOT a physical RSS bound.
 std::shared_ptr<const CapellaColdRecord> load(const CapellaColdReference&,
  const std::function<std::optional<std::string>(const Bytes&,size_t)>& fetch,
  const std::function<void()>& checkpoint={});
 CapellaColdCacheUsage usage()const{return {entries_.size(),encoded_bytes_};}
};
}
