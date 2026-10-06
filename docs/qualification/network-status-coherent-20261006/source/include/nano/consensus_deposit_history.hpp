#pragma once
#include "nano/consensus_execution_deposit.hpp"
namespace nano {
// Immutable, branch-local deposit tree. Copying a history shares unchanged
// subtrees; an append copies only the 32-node path. No global mutable cursor.
class ExecutionDepositHistory {
 struct Node;
 std::shared_ptr<const Node> tree_;
 Bytes execution_hash_;
 uint64_t count_=0;
 static std::shared_ptr<const Node> insert(const std::shared_ptr<const Node>&,
  unsigned depth,uint64_t index,const Phase0Deposit&,const std::function<void()>&);
 static std::shared_ptr<const Node> prefix_tree(const std::shared_ptr<const Node>&,
  unsigned depth,uint64_t count,const std::function<void()>&);
public:
 explicit ExecutionDepositHistory(Bytes empty_anchor_hash);
 uint64_t count()const{return count_;}
 const Bytes& execution_hash()const{return execution_hash_;}
 ConsensusRoot root()const;
 // Rebuild an immutable earlier prefix using at most 32 boundary nodes.
 // Caller MUST authenticate the old (execution hash, count, root) tuple from
 // retained canonical ancestry. Root equality alone does not prove ancestry.
 ExecutionDepositHistory authenticated_prefix(uint64_t count,const Bytes& trusted_execution_hash,
  const ConsensusRoot& trusted_root,const std::function<void()>& checkpoint={})const;
 Phase0Deposit proof(uint64_t index,const std::function<void()>& checkpoint={})const;
 void validate_execution(const Header&,const State&,const Bytes& pinned_code_hash,
  const std::function<void()>& checkpoint={})const;
 Json snapshot(const std::function<void()>& checkpoint={})const;
 // Restore against an independently authenticated execution state; a snapshot
 // cannot authenticate its own anchor. Caller also bounds input parsing bytes.
 static ExecutionDepositHistory from_snapshot(const Json&,const Header& trusted_header,
  const State& trusted_state,const Bytes& expected_block_hash,const Bytes& pinned_code_hash,
  size_t maximum_deposits,const std::function<void()>& checkpoint={});
 // For a block already admitted by the full execution/consensus transition.
 // Checks the parent link, state/code and receipts commitment, then binds all
 // extracted deposits to the contract root. Does not authenticate a peer block.
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
