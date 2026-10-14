#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
namespace rocksdb { struct Options; }
namespace nano {
// Local memory/performance policy, not consensus, validation or durability.
// No implicit defaults: callers supply all values before opening databases.
struct ChainStoreMemoryPolicy {
 size_t write_buffer_bytes;
 uint32_t maximum_write_buffers;
 size_t shared_memtable_bytes;
 size_t shared_block_cache_bytes; // zero disables the block cache
};
// Opens on this thread within one scope share their manager and cache. Nested
// scopes restore the prior group. Construct/destroy in stack order on the same
// thread; this scope is deliberately neither copyable nor movable. RocksDB's
// own shared_ptrs keep the resources alive after the scope/database opens.
// This bounds components, not process RSS or the size of one atomic WriteBatch.
class ScopedChainStoreMemoryPolicy {
 struct Impl;
 std::unique_ptr<Impl> impl_;
public:
 explicit ScopedChainStoreMemoryPolicy(ChainStoreMemoryPolicy policy);
 ~ScopedChainStoreMemoryPolicy();
 ScopedChainStoreMemoryPolicy(const ScopedChainStoreMemoryPolicy&)=delete;
 ScopedChainStoreMemoryPolicy& operator=(const ScopedChainStoreMemoryPolicy&)=delete;
 ScopedChainStoreMemoryPolicy(ScopedChainStoreMemoryPolicy&&)=delete;
 ScopedChainStoreMemoryPolicy& operator=(ScopedChainStoreMemoryPolicy&&)=delete;
};
// No-op outside a scope. Call BEFORE Open/OpenForReadOnly for raw RocksDB
// inspection of physical replicas too. Does not change create_if_missing,
// Env, WAL, WriteOptions, comparator, checksums, compression or atomic batches.
void apply_chain_store_memory_policy(rocksdb::Options& options);
} // namespace nano
