#pragma once
#include <cstddef>
#include <cstdint>
namespace nano {
// Process-local optimization only. No validator/protocol limit, persistence,
// eligibility, membership, voting weight, or proof of possession is introduced.
// Zero in either capacity disables entries. The default is best-effort: failure
// to allocate its optional buffer leaves genuine BLST verification uncached.
struct ConsensusBlsVerificationCacheConfig {
 size_t maximum_bytes=64*1024;
 size_t maximum_entries=256;
};
struct ConsensusBlsVerificationCacheStats {
 uint64_t generation=0,calls=0,hits=0,misses=0,bypasses=0,malformed=0;
 uint64_t raw_calls=0,raw_positive=0,raw_negative=0,raw_exceptions=0;
 uint64_t raw_crypto_nanoseconds=0,evictions=0,allocation_failures=0,stale_completions=0;
 size_t maximum_bytes=0,maximum_entries=0,capacity_entries=0,entries=0;
 size_t fixed_metadata_bytes=0,allocated_entry_bytes=0,resident_bytes=0,in_flight=0;
 bool counters_saturated=false;
};
// The positive-budget bound includes sizeof(cache instance) and the actual
// vector capacity *176. Disabled instances retain only their fixed metadata.
// Allocator bookkeeping and transient caller stacks are not portable RSS
// measures. Returns false on optional allocation failure, preserving the old
// configuration. Invalid/too-small positive budgets are rejected.
bool configure_consensus_bls_verification_cache(ConsensusBlsVerificationCacheConfig);
// Clear/configure start a new generation. Old in-flight crypto may return its
// genuine bool, but cannot repopulate cleared entries or current-gen counters.
// in_flight remains a live count across generations; no false zero on clear.
void clear_consensus_bls_verification_cache();
ConsensusBlsVerificationCacheStats consensus_bls_verification_cache_stats();
}
