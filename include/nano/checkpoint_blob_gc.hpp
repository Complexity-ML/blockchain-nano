#pragma once
#include "nano/storage.hpp"
#include "nano/consensus_capella_store_snapshot.hpp"
#include "nano/consensus_fork_vote.hpp"
#include <memory>
#include <map>
namespace nano {
// Administrative operation budgets, never consensus/validator limits.
struct CheckpointBlobGcBudget {
 uint64_t inventory_files,inventory_bytes,scan_records,scan_bytes;
 uint64_t delete_records,delete_bytes,batch_bytes,duration_ms;
};
CheckpointBlobGcBudget checkpoint_blob_gc_budget(const Json&);
CapellaStoreSnapshotLimits checkpoint_blob_gc_snapshot_limits(const Json&);
// Deterministic closed-file inventory; refuses symlinks, special files and
// shared hardlinks. No database open, signing, repair or deletion.
std::string checkpoint_blob_gc_copy_inventory(const std::filesystem::path&,uint64_t maximum_files,uint64_t maximum_bytes,const std::function<void()>& checkpoint={});
class CheckpointBlobGcCopies;
class AuthenticatedCheckpointBlobClosure {
 friend class CheckpointBlobGcCopies;
 friend AuthenticatedCheckpointBlobClosure authenticate_capella_checkpoint_blob_closure(
  std::string_view,const Bytes&,const CapellaForkBlockRules&,const CapellaStoreSnapshotLimits&,
  const std::function<std::optional<std::string>(const Bytes&,size_t)>&,
  const std::vector<CheckpointArchiveEntry>&,const std::function<void()>&);
 std::map<Bytes,size_t> references_;
 Json identity_,execution_head_;
 Phase0ForkSlashingContext head_context_{};
 AuthenticatedCheckpointBlobClosure()=default;
public:
 Json description()const;
 // Authenticated HEAD block context, not justified/target/UTC duty context.
 // Owns the full registry and returns a copy; no mutable Store/lease escapes.
 Phase0ForkSlashingContext head_context()const{return head_context_;}
};
// Uses the actual complete durable decoder, with fresh caches and no reactor.
// It returns only a sealed closure, never a mutable Store or signing authority.
AuthenticatedCheckpointBlobClosure authenticate_capella_checkpoint_blob_closure(
 std::string_view,const Bytes& expected_digest,const CapellaForkBlockRules&,const CapellaStoreSnapshotLimits&,
 const std::function<std::optional<std::string>(const Bytes&,size_t)>&,
 const std::vector<CheckpointArchiveEntry>& archive={},const std::function<void()>& checkpoint={});
class CheckpointBlobGcPlan {
 friend class CheckpointBlobGcCopies;
 struct Candidate {std::string key;Bytes digest;uint64_t bytes;bool operator==(const Candidate&)const=default;};
 const void* owner_=nullptr;
 std::vector<Candidate> primary_,witness_;
 std::map<Bytes,size_t> protected_;
 ForkCheckpointToken primary_token_,witness_token_,next_token_;
 std::string primary_snapshot_,witness_snapshot_,non_blob_primary_,non_blob_witness_,id_;
 Json identity_;
 CapellaForkBlockRules rules_;
 CapellaStoreSnapshotLimits limits_{};
 CheckpointBlobGcBudget budget_{};
 CheckpointBlobGcPlan()=default;
public:
 Json description()const;
};
// Owns only exclusive handles on stopped, privately owned physical copies.
// Externally pinned request/stop receipt are operator authority, not proof that
// a privileged process cannot fabricate receipts or access the same filesystem.
// No borrowed live ChainStore/provider/cache/lease can be passed to this API.
class CheckpointBlobGcCopies {
 struct Impl;
 std::unique_ptr<Impl> impl_;
 explicit CheckpointBlobGcCopies(std::unique_ptr<Impl>);
 CheckpointBlobGcPlan plan_locked(const CapellaForkBlockRules&,const CapellaStoreSnapshotLimits&,
  const CheckpointBlobGcBudget&,const std::function<void()>&);
public:
 static std::unique_ptr<CheckpointBlobGcCopies> open(std::string_view pinned_request,const Bytes& request_sha256,
  std::string_view pinned_stop_receipt,const Bytes& stop_sha256,const CheckpointBlobGcBudget&,
  const std::function<void()>& checkpoint={});
 ~CheckpointBlobGcCopies();
 CheckpointBlobGcCopies(const CheckpointBlobGcCopies&)=delete;
 CheckpointBlobGcCopies& operator=(const CheckpointBlobGcCopies&)=delete;
 CheckpointBlobGcPlan plan(const CapellaForkBlockRules&,const CapellaStoreSnapshotLimits&,
  const CheckpointBlobGcBudget&,const std::function<void()>& checkpoint={});
 // One-shot collection, flush/compaction, close and RO reopen. Any incomplete
 // operation requires destruction/reopen and a NEW plan. No old-plan replay.
 Json collect(const CheckpointBlobGcPlan&,const std::function<void()>& checkpoint={});
};
}
