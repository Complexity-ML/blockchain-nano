#pragma once
#include "nano/consensus_execution_deposit.hpp"
namespace nano {
class CanonicalDepositContext;
// Immutable, branch-local deposit tree. Memory histories share subtrees. Cold
// histories retain typed references plus at most a depth-32 prefix frontier;
// their shared context owns bounded blob leases, never a global tree registry.
class ExecutionDepositHistory {
 struct Node;
 std::shared_ptr<const Node> tree_;
 std::shared_ptr<CanonicalDepositContext> cold_storage_;
 Bytes execution_hash_;
 uint64_t count_=0;
 static std::shared_ptr<const Node> insert(const std::shared_ptr<const Node>&,
  unsigned depth,uint64_t index,const Phase0Deposit&,const std::function<void()>&,
  const std::shared_ptr<CanonicalDepositContext>& context={});
 static std::shared_ptr<const Node> prefix_tree(const std::shared_ptr<const Node>&,
  unsigned depth,uint64_t count,const std::function<void()>&,
  const std::shared_ptr<CanonicalDepositContext>& context={});
 void check_storage(const std::function<void()>& checkpoint)const;
 static ExecutionDepositHistory read_snapshot(const Json&,const Header&,const State&,
  const Bytes&,const Bytes&,size_t,std::shared_ptr<CanonicalDepositContext>,
  const std::function<void()>&);
public:
 explicit ExecutionDepositHistory(Bytes empty_anchor_hash);
 uint64_t count()const{return count_;}
 const Bytes& execution_hash()const{return execution_hash_;}
 ConsensusRoot root()const;
 // Return a new history whose complete nonempty node closure is persisted in
 // this context. Does not mutate the input. Failed/cancelled persistence may
 // leave counted, unreachable scratch blobs but publishes no new history.
 ExecutionDepositHistory with_cold_storage(std::shared_ptr<CanonicalDepositContext>,
  const std::function<void()>& checkpoint={})const;
 const std::shared_ptr<CanonicalDepositContext>& cold_storage()const{return cold_storage_;}
 bool uses_cold_storage()const{return bool(cold_storage_);}
 // Caller MUST authenticate the old (execution hash, count, root) tuple from
 // canonical ancestry. Root equality alone does not prove ancestry.
 // Prefix reconstruction retains at most 32 hot boundary nodes and performs
 // no scratch writes; cold children remain lazily authenticated references.
 ExecutionDepositHistory authenticated_prefix(uint64_t count,const Bytes& trusted_execution_hash,
  const ConsensusRoot& trusted_root,const std::function<void()>& checkpoint={})const;
 Phase0Deposit proof(uint64_t index,const std::function<void()>& checkpoint={})const;
 void validate_execution(const Header&,const State&,const Bytes& pinned_code_hash,
  const std::function<void()>& checkpoint={})const;
 // Compatibility export: exact legacy v1 bytes, still materializes the entire
 // JSON list. Runtime store serialization must reject cold histories before
 // invoking this method; it is not a new bounded persisted representation.
 Json snapshot(const std::function<void()>& checkpoint={})const;
 // Restore against an independently authenticated execution state; a snapshot
 // cannot authenticate its own anchor. Caller also bounds input parsing bytes.
 static ExecutionDepositHistory from_snapshot(const Json&,const Header& trusted_header,
  const State& trusted_state,const Bytes& expected_block_hash,const Bytes& pinned_code_hash,
  size_t maximum_deposits,const std::function<void()>& checkpoint={});
 // Legacy v1 input with incremental cold construction. The caller separately
 // admits/bounds JSON parsing; this method never builds a complete hot tree.
 static ExecutionDepositHistory from_snapshot_cold(const Json&,const Header& trusted_header,
  const State& trusted_state,const Bytes& expected_block_hash,const Bytes& pinned_code_hash,
  size_t maximum_deposits,std::shared_ptr<CanonicalDepositContext>,
  const std::function<void()>& checkpoint={});
 // For a block already admitted by the full execution/consensus transition.
 // Checks parent, state/code and receipts, then binds deposits to the contract
 // root. Does not itself authenticate a peer block.
 ExecutionDepositHistory append_verified_execution(const Header&,const State&,
  const std::vector<Receipt>&,const Bytes& pinned_code_hash,size_t maximum_deposits,
  const std::function<void()>& checkpoint={})const;
 // Extends only this execution parent. Independently trusted block/code hashes
 // have the same requirements as replay_execution_deposits. Both contract views
 // must agree with the parent and resulting tree. The original history and
 // parent state stay unchanged on success, rejection and cancellation.
 std::pair<ExecutionDepositHistory,ExecutionDepositBatch> extend(
  const ExecutionEngine&,const State& parent_state,const Header& parent,const Block&,
  const Bytes& expected_block_hash,const Bytes& pinned_code_hash,size_t maximum_deposits,
  const std::function<void()>& checkpoint={})const;
};
}
