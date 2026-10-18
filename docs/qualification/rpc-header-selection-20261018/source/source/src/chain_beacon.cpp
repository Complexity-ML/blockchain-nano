#include "nano/chain.hpp"
#include <type_traits>
namespace nano {
void Blockchain::require_execution_ready_locked()const{
 if(genesis_.consensus!="beacon")return;
 if(!beacon_ready_||!beacon_finalized_)throw std::runtime_error("Beacon execution unavailable until checkpoint verification");
 // Also detects a poisoned store after an uncertain RocksDB write.
 const auto durable=store_.latest();
 if(!durable||durable->header.hash()!=tip_.header.hash())throw std::runtime_error("Beacon canonical cache differs from durable head");
}
void Blockchain::require_execution_ready()const{std::lock_guard lock(mutex_);require_execution_ready_locked();}

std::optional<Block> Blockchain::finalized_block()const{
 std::lock_guard lock(mutex_);require_execution_ready_locked();
 if(genesis_.consensus!="beacon")return std::nullopt;
 return beacon_finalized_;
}
Blockchain::StateReadContext Blockchain::state_read_context(size_t maximum_bytes,const std::function<void()>& checkpoint)const{
 if(!maximum_bytes||maximum_bytes>256*1024*1024)throw std::invalid_argument("Invalid state-read header policy");
 // Bounded local admission fuse, not a consensus retry/fork-choice rule. Every
 // attempt also shares the caller's absolute deadline. No caller callback runs
 // under ChainStore/Blockchain mutexes, including the durable-read checkpoints.
 for(unsigned attempt=0;attempt<4;++attempt){
  if(checkpoint)checkpoint();std::pair<Header,std::optional<Header>> captured;uint64_t sequence;
  {std::lock_guard lock(mutex_);captured=state_read_headers_locked();sequence=event_sequence_;}
  if(checkpoint)checkpoint();
  if(genesis_.consensus!="beacon")return {std::move(captured.first),std::move(captured.second)};
  const auto durable=store_.uncertified_tip_header(maximum_bytes,checkpoint);
  if(checkpoint)checkpoint();bool changed;
  {
   std::lock_guard lock(mutex_);const auto current=state_read_headers_locked();
   // The existing event generation also detects canonical A->B->A. Comparing
   // only hashes would mislabel a durable B read during that race as corruption.
   // Other events can conservatively cause a bounded retry, never acceptance.
   changed=event_sequence_!=sequence||current.first.hash()!=captured.first.hash()||bool(current.second)!=bool(captured.second)||(current.second&&current.second->hash()!=captured.second->hash());
  }
  if(changed)continue;
  if(!durable||durable->hash()!=captured.first.hash())throw std::runtime_error("Beacon canonical cache differs from durable head");
  if(checkpoint)checkpoint();return {std::move(captured.first),std::move(captured.second)};
 }
 throw ExecutionCancelled("RPC state-read head changed during capture");
}

ForkCheckpointToken Blockchain::commit_beacon_checkpoint(const std::optional<ForkCheckpointToken>& expected,
 const Bytes& expected_tip,const Int& common_height,const Bytes& common_hash,
 const std::vector<ExecutionCheckpointBlock>& suffix,Block finalized,std::string_view snapshot,size_t maximum_bytes,size_t maximum_blocks,const std::vector<CheckpointArchiveEntry>& archive,const std::optional<ExecutionStateRetentionAnchor>& retention){
 return commit_beacon_checkpoint_with_blobs(expected,expected_tip,common_height,common_hash,suffix,std::move(finalized),snapshot,maximum_bytes,maximum_blocks,archive,retention,{});
}
ForkCheckpointToken Blockchain::commit_beacon_checkpoint_with_blobs(const std::optional<ForkCheckpointToken>& expected,
 const Bytes& expected_tip,const Int& common_height,const Bytes& common_hash,
 const std::vector<ExecutionCheckpointBlock>& suffix,Block finalized,std::string_view snapshot,size_t maximum_bytes,size_t maximum_blocks,const std::vector<CheckpointArchiveEntry>& archive,const std::optional<ExecutionStateRetentionAnchor>& retention,const std::vector<CheckpointBlob>& blobs){
 std::lock_guard lock(mutex_);
 if(genesis_.consensus!="beacon"||(genesis_.evm_revision!=EvmRevision::Paris&&genesis_.evm_revision!=EvmRevision::Shanghai)||tip_.header.hash()!=expected_tip)
  throw std::invalid_argument("Beacon canonical publication context mismatch");
 if(common_height<0||common_height>tip_.header.number||tip_.header.number-common_height>Int(std::to_string(maximum_blocks)))throw std::invalid_argument("Beacon canonical replacement bounds");
 Block next;State state;
 if(suffix.empty()){const auto block=store_.block_by_number(common_height);const auto persisted=store_.state_at(common_height);if(!block||!persisted||block->header.hash()!=common_hash)throw std::invalid_argument("Beacon common ancestor missing");next=*block;state=*persisted;}
 else{next=suffix.back().block;state=suffix.back().state;}
 if(finalized.header.number<0||finalized.header.number>next.header.number)throw std::invalid_argument("Finalized execution height outside canonical chain");
 if(beacon_finalized_&&(finalized.header.number<beacon_finalized_->header.number||(finalized.header.number==beacon_finalized_->header.number&&finalized.header.hash()!=beacon_finalized_->header.hash())))throw std::invalid_argument("Finalized execution checkpoint regressed");
 if(finalized.header.number<=common_height){const auto existing=store_.block_by_number(finalized.header.number);if(!existing||existing->header.hash()!=finalized.header.hash())throw std::invalid_argument("Finalized execution ancestor mismatch");}
 else{const auto found=std::find_if(suffix.begin(),suffix.end(),[&](const auto& entry){return entry.block.header.number==finalized.header.number&&entry.block.header.hash()==finalized.header.hash();});if(found==suffix.end())throw std::invalid_argument("Finalized execution suffix mismatch");}
 std::optional<Block> next_finalized(std::move(finalized));
 const bool changed=next.header.hash()!=tip_.header.hash();
 std::function<void()> publish_pool;auto events=events_;auto sequence=event_sequence_;
 if(changed){
  std::vector<SignedTransaction> orphaned,included;
  for(Int at=common_height+1;at<=tip_.header.number;++at){const auto old=store_.block_by_number(at);if(!old)throw std::runtime_error("Beacon orphan missing");orphaned.insert(orphaned.end(),old->transactions.begin(),old->transactions.end());}
  for(const auto& record:suffix)included.insert(included.end(),record.block.transactions.begin(),record.block.transactions.end());
  publish_pool=pool_.prepare_canonical_change(state,included,orphaned);
  auto event=[&](const char* type,Json value){if(sequence==UINT64_MAX)throw std::overflow_error("chain event sequence exhausted");events.push_back({++sequence,type,std::move(value)});if(events.size()>10000)events.pop_front();};
  if(common_height<tip_.header.number)event("reset",nullptr);
  for(const auto& record:suffix)event("block",record.block.header.to_json());
 }
 static_assert(std::is_nothrow_move_assignable_v<State>);
 static_assert(std::is_nothrow_move_assignable_v<Block>);
 static_assert(std::is_nothrow_move_assignable_v<std::optional<Block>>);
 auto token=store_.commit_execution_checkpoint_with_blobs(genesis_.chain_id,genesis_.block.header.hash(),expected,expected_tip,common_height,common_hash,suffix,snapshot,maximum_bytes,maximum_blocks,blobs,maximum_blocks,archive,retention);
 if(changed){state_=std::move(state);tip_=std::move(next);publish_pool();events_.swap(events);event_sequence_=sequence;audited_certified_tip_.reset();}
 beacon_finalized_=std::move(next_finalized);beacon_ready_=true;
 return token;
}
}
