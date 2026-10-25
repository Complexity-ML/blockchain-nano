#pragma once
#include "nano/consensus_capella_fork_store.hpp"
#include "nano/storage_memory_policy.hpp"
#include <filesystem>
#include <functional>
#include <memory>

namespace nano {
struct CanonicalDepositContextState;

// A cursor is created only by its live context. Its ordinal is a locator, not
// ancestry authority; context identity and the committed row digest are checked.
class CanonicalDepositCursor {
 friend class CanonicalDepositContext;
 friend struct CanonicalDepositContextState;
 std::weak_ptr<const CanonicalDepositContextState> owner_;
 uint64_t ordinal_=0;
 ConsensusRoot digest_{};
 CanonicalDepositCursor(std::shared_ptr<const CanonicalDepositContextState>,uint64_t,ConsensusRoot);
public:
 ~CanonicalDepositCursor()=default;
 CanonicalDepositCursor(const CanonicalDepositCursor&)=default;
 uint64_t ordinal()const{return ordinal_;}
};

struct CanonicalDepositBlobRef {
 ConsensusRoot digest; // SHA256 of encoded payload, never its SSZ Merkle root.
 uint64_t bytes;
 bool operator==(const CanonicalDepositBlobRef&)const=default;
};
struct CanonicalDepositContextLimits {
 uint64_t maximum_tuples,maximum_tuple_bytes;
 // One encoded row plus its tuple key and two fixed-width secondary indexes.
 // 8192 is a useful local choice; oversized values refuse, never truncate.
 size_t maximum_tuple_record_bytes;
 uint64_t maximum_blob_records,maximum_blob_bytes;
 size_t maximum_cached_blob_records,maximum_cached_blob_bytes;
 ChainStoreMemoryPolicy database_memory;
};
struct CanonicalDepositContextUsage {
 uint64_t tuples,tuple_bytes,blob_records,blob_bytes;
 // Counts live allocations, including a lease concurrently retired from the
 // lookup cache. Its charge survives until the last shared byte owner dies.
 size_t cached_blob_records,cached_blob_bytes,pinned_blob_records;
};

// Audit-local canonical prefix only. Root must create new_directory outside its
// audited DB, and supply the externally authenticated anchor/profile binding.
// Root calls append_admitted ONLY after full signed admission AND execution
// validation. This component validates links; it is not a block-admission API.
// There is one new owner0700 RocksDB scratch, not one file per node. Physical
// SST/WAL/RSS is not equal to the logical value/cache budgets below.
class CanonicalDepositContext {
 std::shared_ptr<CanonicalDepositContextState> state_;
 explicit CanonicalDepositContext(std::shared_ptr<CanonicalDepositContextState>);
 std::shared_ptr<const CanonicalDepositCursor> make_cursor(uint64_t,const ConsensusRoot&)const;
public:
 using Cursor=std::shared_ptr<const CanonicalDepositCursor>;
 static std::shared_ptr<CanonicalDepositContext> create(
  const std::filesystem::path& new_directory,const CapellaDepositAncestor& anchor,
  const Bytes& profile_digest,const CanonicalDepositContextLimits&,
  const std::function<void()>& checkpoint);
 ~CanonicalDepositContext();
 CanonicalDepositContext(const CanonicalDepositContext&)=delete;
 CanonicalDepositContext& operator=(const CanonicalDepositContext&)=delete;

 Cursor anchor_cursor(const std::function<void()>& checkpoint)const;
 // Exact duplicate is idempotent. A different sibling or non-tip parent refuses;
 // this canonical-audit context never becomes a shared mutable fork-choice head.
 Cursor append_admitted(const CapellaDepositAncestor&,const std::function<void()>& checkpoint);
 Cursor cursor_for_root(const ConsensusRoot&,const std::function<void()>& checkpoint)const;
 CapellaDepositAncestor at(const Cursor&,const std::function<void()>& checkpoint)const;
 void validate_cursor(const Cursor&,const ConsensusBeaconHeader&,const Header&,
  const std::function<void()>& checkpoint)const;
 // The supplied cursor is the hot anchor. Only its STRICT ancestors may match;
 // later rows are never admitted by consulting the context's newest ordinal.
 std::optional<CapellaDepositAncestor> find_before(const Cursor& exclusive_anchor,
  const Bytes& execution_hash,uint64_t count,const ConsensusRoot& deposit_root,
  const std::function<void()>& checkpoint)const;
 CapellaDepositAncestor last_before(const Cursor& exclusive_anchor,uint64_t start_slot,
  const std::function<void()>& checkpoint)const;

 CanonicalDepositBlobRef put_node_blob(const Bytes&,const std::function<void()>& checkpoint);
 std::shared_ptr<const Bytes> lease_node_blob(const CanonicalDepositBlobRef&,
  const std::function<void()>& checkpoint);
 void ensure_open(const std::function<void()>& checkpoint)const;
 CanonicalDepositContextUsage usage()const;
 // Serializes with in-flight operations. Existing immutable byte leases survive;
 // every later operation, including a cache hit, fails closed.
 void close();
};
}
