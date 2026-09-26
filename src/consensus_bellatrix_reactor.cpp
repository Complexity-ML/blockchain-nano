#include "nano/consensus_bellatrix_reactor.hpp"
#include <type_traits>
#include "nano/chain.hpp"
#include "nano/consensus_message_ssz.hpp"
#include "nano/consensus_bls.hpp"
#include "nano/consensus_bellatrix_duty.hpp"
#include "nano/consensus_bellatrix_deposits.hpp"
namespace nano {
namespace {
void archive_require(bool ok){if(!ok)throw std::invalid_argument("Invalid Bellatrix durable archive");}
uint64_t archive_number(std::span<const uint8_t> b,size_t at,size_t width=8){uint64_t n=0;for(size_t i=0;i<width;++i)n|=uint64_t(b[at+i])<<(8*i);return n;}
template<size_t N>std::array<uint8_t,N> archive_array(std::span<const uint8_t> b,size_t at){std::array<uint8_t,N> r{};std::copy_n(b.begin()+at,N,r.begin());return r;}
void verify_archived_block(const ConsensusRoot& id,const Bytes& bytes,const BellatrixForkStore& store,const BellatrixForkBlockRules& rules,const std::function<void()>& cancel){
 archive_require(id!=store.timed.view.anchor&&store.blocks.contains(id)&&bytes.size()>=404&&bytes.size()<=rules.transition.consensus.maximum_block_bytes);
 archive_require(archive_number(bytes,0,4)==100&&archive_number(bytes,180,4)==84);
 const ConsensusBeaconHeader header{archive_number(bytes,100),archive_number(bytes,108),archive_array<32>(bytes,116),archive_array<32>(bytes,148),bellatrix_beacon_body_root(std::span(bytes).subspan(184),rules.transition.consensus.body,rules.transition.consensus.state.sync_committee_size,rules.transition.execution,rules.transition.consensus.maximum_block_bytes,cancel)};
 const auto& block=store.blocks.at(id);archive_require(header==block.header&&consensus_beacon_header_root(header)==id);
 const auto context=bellatrix_fork_block_context(block.state,header,BellatrixEpochRules{rules.transition.consensus.epoch,rules.transition.execution,rules.transition.maximum_state_bytes},rules.transition.consensus.state,cancel);
 const auto proposer=context.registry.validators.find(header.proposer_index);archive_require(proposer!=context.registry.validators.end());
 const auto& state=block.state.ssz;const auto version=archive_array<4>(state,context.registry.epoch<archive_number(state,56)?48:52);
 archive_require(verify_consensus_beacon_header(header,version,archive_array<32>(state,8),proposer->second.public_key,std::span(bytes).subspan(4,96)));
 if(cancel)cancel();
}
// Current-slot votes are authenticated and durably queued, but cannot affect
// fork choice until the next slot (on_attestation's slot + 1 condition).
std::optional<BellatrixForkStore> checked_vote(const BellatrixForkStore& store,const Phase0SignedAttestation& vote,
 const BellatrixForkBlockRules& rules,const BellatrixStoreSnapshotLimits& limits,const std::function<void()>& check){
 const auto& clock=store.timed.clock;
 if(!rules.slot_duration_ms||clock.time_ms<clock.genesis_time_ms)throw std::invalid_argument("Invalid Bellatrix vote clock");
 const auto slot=(clock.time_ms-clock.genesis_time_ms)/rules.slot_duration_ms;
 if(vote.data.slot!=slot)return prepare_bellatrix_store_vote(store,vote,rules,limits.replay_slots,limits.sync_trials,check);
 if(slot==UINT64_MAX||(slot+1)>(UINT64_MAX-clock.genesis_time_ms)/rules.slot_duration_ms)throw std::overflow_error("Bellatrix deferred vote clock");
 const auto future=prepare_bellatrix_store_tick(store,clock.genesis_time_ms+(slot+1)*rules.slot_duration_ms,rules,limits.replay_slots,limits.sync_trials,check);
 if(!prepare_bellatrix_store_vote(future,vote,rules,limits.replay_slots,limits.sync_trials,check))return std::nullopt;
 return store;
}
bool valid_exit_signature(const SignedConsensusVoluntaryExit& exit,const Phase0ForkSlashingContext& context,const Phase0ExitRules& rules,const std::function<void()>& check){
 if(check)check();const auto found=context.registry.validators.find(exit.validator_index);
 if(found==context.registry.validators.end()||exit.epoch>context.registry.epoch)return false;
 const auto& member=found->second;
 if(member.activation_epoch>context.registry.epoch||context.registry.epoch-member.activation_epoch<rules.shard_committee_period)return false;
 const auto version=exit.epoch<context.fork.epoch?context.fork.previous_version:context.fork.current_version;
 return verify_consensus_bls(member.public_key,consensus_signing_root(consensus_voluntary_exit_root(exit.epoch,exit.validator_index),consensus_domain({4,0,0,0},version,context.genesis_root)),exit.signature);
}
std::string durable_snapshot(const BellatrixForkStore& store,const BellatrixSignedBlockArchive& archive,const BellatrixSignedVoteArchive& votes,const BellatrixSignedVoteArchive& slashings,const BellatrixSignedExitArchive& exits,const BellatrixForkBlockRules& rules,const BellatrixStoreSnapshotLimits& limits,const std::function<void()>& cancel){
 // Retain byte-for-byte v1 encoding while both signed archives are empty.
 auto encoded=bellatrix_store_snapshot(store,rules,limits,cancel);if(archive.empty()&&votes.empty()&&slashings.empty()&&exits.empty())return encoded;
 archive_require(archive.size()<store.blocks.size());size_t total=0;Json blocks=Json::array();
 for(const auto& [id,bytes]:archive){
  if(cancel)cancel();archive_require(bytes&&id!=store.timed.view.anchor&&store.blocks.contains(id)&&bytes->size()<=rules.transition.consensus.maximum_block_bytes&&bytes->size()<=limits.envelope_bytes/2-total);total+=bytes->size();
  blocks.push_back(Json::array({hex(Bytes(id.begin(),id.end())),hex(*bytes)}));
 }
 Json document{{"format",!exits.empty()?"nano-bellatrix-durable-v5":!slashings.empty()?"nano-bellatrix-durable-v4":votes.empty()?"nano-bellatrix-durable-v2":"nano-bellatrix-durable-v3"},{"store",Json::parse(encoded)},{"signedBlocks",std::move(blocks)}};
 if(!votes.empty()||!slashings.empty()||!exits.empty()){
  archive_require(votes.size()<=limits.messages);Json messages=Json::array();
  const auto epoch=store.timed.view.current_epoch,oldest=epoch?epoch-1:0;
  for(const auto& [id,bytes]:votes){
   if(cancel)cancel();archive_require(bytes&&bytes->size()>=229&&bytes->size()<=228+uint64_t(rules.transition.consensus.body.validators_per_committee)/8+1&&bytes->size()<=limits.envelope_bytes/2-total);
   const auto target=archive_number(*bytes,92);archive_require(target>=oldest&&target<=epoch);total+=bytes->size();
   messages.push_back(Json::array({hex(Bytes(id.begin(),id.end())),hex(*bytes)}));
  }
  document["signedVotes"]=std::move(messages);
 }
 if(!slashings.empty()||!exits.empty()){
  archive_require(slashings.size()<=limits.equivocations);Json list=Json::array();
  for(const auto& [id,wire]:slashings){if(cancel)cancel();archive_require(wire&&wire->size()<=464+uint64_t(rules.transition.consensus.body.validators_per_committee)*16&&wire->size()<=limits.envelope_bytes/2-total);total+=wire->size();list.push_back(Json::array({hex(Bytes(id.begin(),id.end())),hex(*wire)}));}
  document["signedSlashings"]=std::move(list);
 }
 if(!exits.empty()){
  archive_require(exits.size()<=limits.messages);Json list=Json::array();
  for(const auto& [id,record]:exits){if(cancel)cancel();archive_require(record&&record->ssz.size()==112&&store.blocks.contains(record->admission_root)&&144<=limits.envelope_bytes/2-total);total+=144;list.push_back(Json::array({hex(Bytes(id.begin(),id.end())),hex(Bytes(record->admission_root.begin(),record->admission_root.end())),hex(record->ssz)}));}
  document["signedExits"]=std::move(list);
 }
 auto result=document.dump();archive_require(result.size()<=limits.envelope_bytes);if(cancel)cancel();return result;
}
struct Restored {BellatrixForkStore store;std::shared_ptr<const BellatrixSignedBlockArchive> blocks;std::shared_ptr<const BellatrixSignedVoteArchive> votes,slashings;std::shared_ptr<const BellatrixSignedExitArchive> exits;};
Restored restore_durable_snapshot(std::string_view bytes,const Bytes& digest,const BellatrixForkBlockRules& rules,const BellatrixStoreSnapshotLimits& limits,const std::function<void()>& cancel){
 archive_require(bytes.size()<=limits.envelope_bytes&&digest.size()==32);if(cancel)cancel();archive_require(sha256(Bytes(bytes.begin(),bytes.end()))==digest);
 const auto document=Json::parse(bytes);
 if(document.value("format",Json())=="nano-bellatrix-store-v1")return {bellatrix_store_from_snapshot(bytes,digest,rules,limits,cancel),std::make_shared<const BellatrixSignedBlockArchive>(),std::make_shared<const BellatrixSignedVoteArchive>(),std::make_shared<const BellatrixSignedVoteArchive>(),std::make_shared<const BellatrixSignedExitArchive>()};
 const bool with_exits=document.value("format",Json())=="nano-bellatrix-durable-v5";
 const bool with_slashings=with_exits||document.value("format",Json())=="nano-bellatrix-durable-v4";
 const bool with_votes=with_slashings||document.value("format",Json())=="nano-bellatrix-durable-v3";
 archive_require(document.is_object()&&document.size()==(with_exits?6:with_slashings?5:with_votes?4:3)&&(with_votes||document.at("format")=="nano-bellatrix-durable-v2")&&document.at("signedBlocks").is_array()&&(with_votes||!document.at("signedBlocks").empty())&&document.at("signedBlocks").size()<=limits.blocks);
 const auto serialized=document.at("store").dump();auto store=bellatrix_store_from_snapshot(serialized,sha256(Bytes(serialized.begin(),serialized.end())),rules,limits,cancel);
 auto archive=std::make_shared<BellatrixSignedBlockArchive>();size_t total=0;
 for(const auto& entry:document.at("signedBlocks")){
  if(cancel)cancel();archive_require(entry.is_array()&&entry.size()==2&&entry[0].is_string()&&entry[1].is_string());
  const auto& root=entry[0].get_ref<const std::string&>();const auto& payload=entry[1].get_ref<const std::string&>();
  archive_require(root.size()==66&&payload.size()>=2&&payload.size()%2==0&&(payload.size()-2)/2<=rules.transition.consensus.maximum_block_bytes&&(payload.size()-2)/2<=limits.envelope_bytes/2-total);
  const auto decoded_root=unhex(root);archive_require(decoded_root.size()==32&&hex(decoded_root)==root);const auto id=archive_array<32>(decoded_root,0);
  auto data=std::make_shared<const Bytes>(unhex(payload));archive_require(hex(*data)==payload);total+=data->size();verify_archived_block(id,*data,store,rules,cancel);archive_require(archive->emplace(id,std::move(data)).second);
 }
 auto votes=std::make_shared<BellatrixSignedVoteArchive>();
 if(with_votes){
  const auto& list=document.at("signedVotes");archive_require(list.is_array()&&(with_slashings||!list.empty())&&list.size()<=limits.messages);
  for(const auto& entry:list){
   if(cancel)cancel();archive_require(entry.is_array()&&entry.size()==2&&entry[0].is_string()&&entry[1].is_string());
   const auto& root=entry[0].get_ref<const std::string&>();const auto& payload=entry[1].get_ref<const std::string&>();
   archive_require(root.size()==66&&payload.size()>=460&&payload.size()%2==0&&(payload.size()-2)/2<=228+uint64_t(rules.transition.consensus.body.validators_per_committee)/8+1&&(payload.size()-2)/2<=limits.envelope_bytes/2-total);
   auto data=std::make_shared<const Bytes>(unhex(payload));const auto digest=sha256(*data);archive_require(hex(digest)==root&&hex(*data)==payload);total+=data->size();
   const auto decoded=decode_consensus_attestation_ssz(*data,rules.transition.consensus.body.validators_per_committee,cancel);archive_require(decoded.has_value());
   // Local restore revalidates signature, target ancestry and gossip window;
   // it never publishes the candidate or re-applies a vote to the restored state.
   archive_require(checked_vote(store,*decoded,rules,limits,cancel).has_value());
   archive_require(votes->emplace(archive_array<32>(digest,0),std::move(data)).second);
  }
 }
 auto slashings=std::make_shared<BellatrixSignedVoteArchive>();
 if(with_slashings){
  const auto& list=document.at("signedSlashings");archive_require(list.is_array()&&(with_exits||!list.empty())&&list.size()<=limits.equivocations);
  for(const auto& entry:list){
   if(cancel)cancel();archive_require(entry.is_array()&&entry.size()==2&&entry[0].is_string()&&entry[1].is_string());
   const auto root=entry[0].get<std::string>(),payload=entry[1].get<std::string>();
   archive_require(root.size()==66&&payload.size()>=2&&payload.size()%2==0&&(payload.size()-2)/2<=464+uint64_t(rules.transition.consensus.body.validators_per_committee)*16&&(payload.size()-2)/2<=limits.envelope_bytes/2-total);
   auto wire=std::make_shared<const Bytes>(unhex(payload));const auto digest=sha256(*wire);archive_require(hex(*wire)==payload&&hex(digest)==root);total+=wire->size();
   const auto proof=decode_consensus_attester_slashing_ssz(*wire,rules.transition.consensus.body.validators_per_committee,cancel);
   archive_require(proof&&prepare_bellatrix_store_slashing(store,proof->first,proof->second,rules,cancel).has_value());
   archive_require(slashings->emplace(archive_array<32>(digest,0),std::move(wire)).second);
  }
 }
 auto exits=std::make_shared<BellatrixSignedExitArchive>();
 if(with_exits){
  const auto& list=document.at("signedExits");archive_require(list.is_array()&&!list.empty()&&list.size()<=limits.messages);
  const auto& t=rules.transition.consensus;std::set<uint64_t> indices;
  for(const auto& entry:list){
   if(cancel)cancel();archive_require(entry.is_array()&&entry.size()==3&&entry[0].is_string()&&entry[1].is_string()&&entry[2].is_string());
   const auto root=entry[0].get<std::string>(),admission=entry[1].get<std::string>(),payload=entry[2].get<std::string>();
   archive_require(root.size()==66&&admission.size()==66&&payload.size()==226&&144<=limits.envelope_bytes/2-total);
   auto wire=unhex(payload);const auto digest=sha256(wire);auto admission_bytes=unhex(admission);
   archive_require(admission_bytes.size()==32&&hex(admission_bytes)==admission&&hex(wire)==payload&&hex(digest)==root);total+=144;
   const auto admission_root=archive_array<32>(admission_bytes,0);archive_require(store.blocks.contains(admission_root));
   const auto& block=store.blocks.at(admission_root);
   const auto context=bellatrix_fork_block_context(block.state,block.header,{t.epoch,rules.transition.execution,rules.transition.maximum_state_bytes},t.state,cancel);
   const auto exit=decode_consensus_voluntary_exit_ssz(wire,cancel);archive_require(exit&&valid_exit_signature(*exit,context,t.exit,cancel)&&indices.insert(exit->validator_index).second);
   auto record=std::make_shared<const BellatrixSignedExitRecord>(BellatrixSignedExitRecord{admission_root,std::move(wire)});
   archive_require(exits->emplace(archive_array<32>(digest,0),std::move(record)).second);
  }
 }
 archive_require(durable_snapshot(store,*archive,*votes,*slashings,*exits,rules,limits,cancel)==bytes);return {std::move(store),std::move(archive),std::move(votes),std::move(slashings),std::move(exits)};
}
}
static_assert(std::is_nothrow_move_assignable_v<std::optional<ForkCheckpointToken>>);
static_assert(std::is_nothrow_move_assignable_v<std::shared_ptr<const BellatrixForkStore>>);
namespace {
Json recovery_token_json(const std::optional<ForkCheckpointToken>& token){return token?Json{{"sequence",quantity(Int(std::to_string(token->sequence)))},{"digest",hex(token->digest)}}:Json(nullptr);}
std::optional<ForkCheckpointToken> recovery_token_parse(const Json& j){
 if(j.is_null())return std::nullopt;
 archive_require(j.is_object()&&j.size()==2);
 const ForkCheckpointToken token{u64(quantity(j.at("sequence"))),unhex(j.at("digest").get<std::string>())};
 archive_require(token.sequence&&token.digest.size()==32&&recovery_token_json(token)==j);return token;
}
}
size_t BellatrixDurableReactor::witness_limit()const{
 if(limits_.envelope_bytes>(SIZE_MAX-1024)/6)throw std::length_error("Bellatrix witness size overflow");
 return 1024+6*limits_.envelope_bytes;
}
void BellatrixDurableReactor::witness_prepare(std::string_view snapshot){
 if(!witness_)return;
 if(token_&&token_->sequence==UINT64_MAX)throw std::overflow_error("Bellatrix witness sequence exhausted");
 const ForkCheckpointToken next{token_?token_->sequence+1:1,sha256(Bytes(snapshot.begin(),snapshot.end()))};
 const auto encoded=Json{{"format","nano-bellatrix-recovery-v1"},{"previous",recovery_token_json(token_)},{"next",recovery_token_json(next)},{"snapshot",snapshot}}.dump();
 witness_token_=witness_->commit_fork_checkpoint(chain_id_,genesis_,witness_token_,encoded,witness_limit());
}
void BellatrixDurableReactor::validate_execution_binding()const{
 if(!execution_)return;
 std::lock_guard lock(execution_->mutex_);
 const auto& genesis=execution_->genesis_;
 if(&execution_->store_!=&disk_||genesis.consensus!="beacon"||genesis.evm_revision!=EvmRevision::Paris||genesis.chain_id!=chain_id_||genesis.block.header.hash()!=genesis_)
  throw std::invalid_argument("Bellatrix execution owner/profile mismatch");
 const auto tip=disk_.latest();
 if(!tip||tip->header.hash()!=execution_->tip_.header.hash()||execution_->state_.root()!=tip->header.state_root)
  throw std::invalid_argument("Bellatrix execution cache does not match durable head");
}
BellatrixDurableReactor::~BellatrixDurableReactor(){
 if(execution_){std::lock_guard lock(execution_->mutex_);execution_->beacon_ready_=false;}
}
void BellatrixDurableReactor::invalidate(){
 std::lock_guard lock(mutex_);unavailable_=true;
 if(execution_){std::lock_guard chain_lock(execution_->mutex_);execution_->beacon_ready_=false;}
}
void BellatrixDurableReactor::require_available()const{if(unavailable_||!state_||!token_)throw std::runtime_error("Bellatrix reactor unavailable; reopen and resolve checkpoint required");}
void BellatrixDurableReactor::publish(BellatrixForkStore candidate,std::shared_ptr<const BellatrixSignedBlockArchive> archive,std::shared_ptr<const BellatrixSignedVoteArchive> votes,const std::function<void()>& checkpoint,std::shared_ptr<const BellatrixSignedVoteArchive> slashings,std::shared_ptr<const BellatrixSignedExitArchive> exits){
 if(!slashings)slashings=signed_slashings_;
 if(!exits)exits=signed_exits_;
 auto next=std::make_shared<const BellatrixForkStore>(std::move(candidate));
 const auto oldest=next->timed.view.current_epoch?next->timed.view.current_epoch-1:0;
 if(std::any_of(votes->begin(),votes->end(),[&](const auto& entry){return archive_number(*entry.second,92)<oldest;})){
  auto retained=std::make_shared<BellatrixSignedVoteArchive>();for(const auto& [id,payload]:*votes){if(checkpoint)checkpoint();if(archive_number(*payload,92)>=oldest)retained->emplace(id,payload);}votes=std::move(retained);
 }
 // A head-only sanction can disappear on reorganisation. Drop evidence only
 // when every common offender is slashed in the authenticated finalized state.
 if(!slashings->empty()&&state_&&state_->timed.view.finalized.root!=next->timed.view.finalized.root){
  const auto& finalized=next->blocks.at(next->timed.view.finalized.root);const auto& t=rules_.transition.consensus;
  const auto context=bellatrix_fork_block_context(finalized.state,finalized.header,{t.epoch,rules_.transition.execution,rules_.transition.maximum_state_bytes},t.state,checkpoint);
  auto retained=std::make_shared<BellatrixSignedVoteArchive>();
  for(const auto& [id,wire]:*slashings){
   if(checkpoint)checkpoint();const auto proof=decode_consensus_attester_slashing_ssz(*wire,t.body.validators_per_committee,checkpoint);
   if(!proof)throw std::logic_error("Invalid archived slashing encoding");
   bool needed=false;
   for(auto index:proof->first.indices){
    if(checkpoint)checkpoint();if(!std::binary_search(proof->second.indices.begin(),proof->second.indices.end(),index))continue;
    const auto member=context.registry.validators.find(index);
    if(member==context.registry.validators.end()||!member->second.slashed){needed=true;break;}
   }
   if(needed)retained->emplace(id,wire);
  }
  if(retained->size()!=slashings->size())slashings=std::move(retained);
 }
 if(!exits->empty()&&state_&&state_->timed.view.finalized.root!=next->timed.view.finalized.root){
  const auto& finalized=next->blocks.at(next->timed.view.finalized.root);const auto& t=rules_.transition.consensus;
  const auto context=bellatrix_fork_block_context(finalized.state,finalized.header,{t.epoch,rules_.transition.execution,rules_.transition.maximum_state_bytes},t.state,checkpoint);
  auto retained=std::make_shared<BellatrixSignedExitArchive>();
  for(const auto& [id,wire]:*exits){if(checkpoint)checkpoint();const auto member=context.registry.validators.find(archive_number(wire->ssz,8));if(member==context.registry.validators.end()||member->second.exit_epoch==UINT64_MAX)retained->emplace(id,wire);}
  if(retained->size()!=exits->size())exits=std::move(retained);
 }
 const auto encoded=durable_snapshot(*next,*archive,*votes,*slashings,*exits,rules_,limits_,checkpoint);
 if(checkpoint)checkpoint();
 // A retransmission with identical durable contents is already acknowledged by
 // this checkpoint. Do not invalidate local duty tokens or amplify disk writes
 // merely because another peer returned a block/vote we already retained.
 if(token_&&sha256(Bytes(encoded.begin(),encoded.end()))==token_->digest){
  try{if(witness_)witness_->fork_checkpoint(chain_id_,genesis_,witness_limit(),witness_token_);if(!disk_.fork_checkpoint(chain_id_,genesis_,limits_.envelope_bytes,token_))throw std::runtime_error("Missing duplicate checkpoint");}
  catch(...){unavailable_=true;if(execution_){std::lock_guard lock(execution_->mutex_);execution_->beacon_ready_=false;}throw;}
  return;
 }

 // All candidate preparation/allocation/cancellation occurs before the write.
 // After a successful write, publishing shared_ptr and vector-backed token moves
 // cannot throw. No callback is invoked between commit and publication.
 ForkCheckpointToken committed;
 const auto tip=disk_.latest();if(!tip)throw std::invalid_argument("Bellatrix reactor requires persisted execution genesis");
 const auto& head=bellatrix_fork_head(*next,rules_,checkpoint);
 if(chain_id_!=rules_.transition.chain_id)throw std::invalid_argument("Bellatrix reactor chain/profile mismatch");
 if(state_){
  const auto& old_head=bellatrix_fork_head(*state_,rules_,checkpoint);
  if(tip->header.hash()!=old_head.execution_block.header.hash()){unavailable_=true;throw std::runtime_error("Bellatrix execution tip changed outside reactor");}
  const auto& finalized=state_->timed.view.finalized;
  archive_require(next->timed.view.anchor==state_->timed.view.anchor&&next->timed.view.finalized.epoch>=finalized.epoch);
  auto at=consensus_beacon_header_root(head.header);const auto finalized_slot=state_->blocks.at(finalized.root).header.slot;
  while(next->blocks.at(at).header.slot>finalized_slot){if(checkpoint)checkpoint();at=next->blocks.at(at).header.parent_root;}
  archive_require(at==finalized.root);
 }
 std::vector<ExecutionCheckpointBlock> suffix;auto at=consensus_beacon_header_root(head.header);Int common_height;Bytes common_hash;
 for(;;){
  if(checkpoint)checkpoint();const auto& record=next->blocks.at(at);const auto& header=record.execution_block.header;const auto existing=disk_.block_by_number(header.number);
  if(existing&&existing->header.hash()==header.hash()){common_height=header.number;common_hash=header.hash();break;}
  archive_require(suffix.size()<limits_.blocks);suffix.push_back({record.execution_block,record.execution_state,record.receipts});
  if(at==next->timed.view.anchor){archive_require(header.number>0);const auto parent=disk_.block_by_number(header.number-1);archive_require(parent&&parent->header.hash()==header.parent_hash);common_height=header.number-1;common_hash=header.parent_hash;break;}
  at=record.header.parent_root;
 }
 std::reverse(suffix.begin(),suffix.end());
 if(checkpoint)checkpoint();
 try{
  witness_prepare(encoded);
  if(execution_)committed=execution_->commit_beacon_checkpoint(token_,tip->header.hash(),common_height,common_hash,suffix,next->blocks.at(next->timed.view.finalized.root).execution_block,encoded,limits_.envelope_bytes,limits_.blocks);
  else committed=disk_.commit_execution_checkpoint(chain_id_,genesis_,token_,tip->header.hash(),common_height,common_hash,suffix,encoded,limits_.envelope_bytes,limits_.blocks);
 }
 catch(...){unavailable_=true;if(execution_){std::lock_guard lock(execution_->mutex_);execution_->beacon_ready_=false;}throw;}
 token_=std::move(committed);state_=std::move(next);signed_blocks_=std::move(archive);signed_votes_=std::move(votes);signed_slashings_=std::move(slashings);signed_exits_=std::move(exits);
}
BellatrixDurableReactor::BellatrixDurableReactor(Blockchain& execution,BellatrixForkBlockRules rules,
 BellatrixStoreSnapshotLimits limits,BellatrixForkStore initial,const std::function<void()>& checkpoint,ChainStore* witness):
 BellatrixDurableReactor(execution.store_,execution.genesis_.chain_id,execution.genesis_.block.header.hash(),std::move(rules),limits,std::move(initial),checkpoint,&execution,witness){}
BellatrixDurableReactor::BellatrixDurableReactor(Blockchain& execution,BellatrixForkBlockRules rules,
 BellatrixStoreSnapshotLimits limits,const ForkCheckpointToken& expected,const std::function<void()>& checkpoint):
 BellatrixDurableReactor(execution.store_,execution.genesis_.chain_id,execution.genesis_.block.header.hash(),std::move(rules),limits,expected,checkpoint,&execution){}
BellatrixDurableReactor::BellatrixDurableReactor(ChainStore& disk,Int chain,Bytes genesis,BellatrixForkBlockRules rules,
 BellatrixStoreSnapshotLimits limits,BellatrixForkStore initial,const std::function<void()>& checkpoint,Blockchain* owner,ChainStore* witness):
 disk_(disk),execution_(owner),chain_id_(std::move(chain)),genesis_(std::move(genesis)),rules_(std::move(rules)),limits_(limits),mutex_(disk.altair_reactor_mutex_){std::lock_guard lock(mutex_);validate_execution_binding();
 if(witness){archive_require(witness!=&disk_);witness_=witness;witness_->bind_chain(chain_id_,genesis_,"nano-bellatrix-recovery-v1");archive_require(!witness_->fork_checkpoint(chain_id_,genesis_,witness_limit())&&!disk_.fork_checkpoint(chain_id_,genesis_,limits_.envelope_bytes));}
 publish(std::move(initial),signed_blocks_,signed_votes_,checkpoint);}
BellatrixDurableReactor::BellatrixDurableReactor(ChainStore& disk,Int chain,Bytes genesis,BellatrixForkBlockRules rules,
 BellatrixStoreSnapshotLimits limits,const ForkCheckpointToken& expected,const std::function<void()>& checkpoint,Blockchain* owner):
 disk_(disk),execution_(owner),chain_id_(std::move(chain)),genesis_(std::move(genesis)),rules_(std::move(rules)),limits_(limits),mutex_(disk.altair_reactor_mutex_){std::lock_guard lock(mutex_);
 validate_execution_binding();
 const auto record=disk_.fork_checkpoint(chain_id_,genesis_,limits_.envelope_bytes,expected);
 if(!record)throw std::runtime_error("missing Bellatrix restart checkpoint");
 auto restored=restore_durable_snapshot(record->snapshot,expected.digest,rules_,limits_,checkpoint);
 const auto& head=bellatrix_fork_head(restored.store,rules_,checkpoint);const auto tip=disk_.latest();const auto execution=disk_.state_at(head.execution_block.header.number);
 archive_require(chain_id_==rules_.transition.chain_id&&tip&&tip->header.hash()==head.execution_block.header.hash()&&execution&&execution->root()==head.execution_state.root());
 for(const auto& receipt:head.receipts){if(checkpoint)checkpoint();const auto persisted=disk_.receipt(receipt.transaction_hash);archive_require(persisted&&persisted->to_json()==receipt.to_json());}
 state_=std::make_shared<const BellatrixForkStore>(std::move(restored.store));signed_blocks_=std::move(restored.blocks);signed_votes_=std::move(restored.votes);signed_slashings_=std::move(restored.slashings);signed_exits_=std::move(restored.exits);token_=expected;
 if(execution_){
  auto finalized=state_->blocks.at(state_->timed.view.finalized.root).execution_block;
  const auto canonical=disk_.block_by_number(finalized.header.number);archive_require(canonical&&canonical->header.hash()==finalized.header.hash());
  std::lock_guard lock(execution_->mutex_);execution_->beacon_finalized_=std::move(finalized);execution_->beacon_ready_=true;
 }
}
BellatrixDurableReactor::BellatrixDurableReactor(Blockchain& execution,BellatrixForkBlockRules rules,
 BellatrixStoreSnapshotLimits limits,ChainStore& witness,const std::function<void()>& checkpoint):
 disk_(execution.store_),execution_(&execution),witness_(&witness),chain_id_(execution.genesis_.chain_id),genesis_(execution.genesis_.block.header.hash()),rules_(std::move(rules)),limits_(limits),mutex_(disk_.altair_reactor_mutex_){
 std::lock_guard lock(mutex_);validate_execution_binding();archive_require(witness_!=&disk_);
 witness_->bind_chain(chain_id_,genesis_,"nano-bellatrix-recovery-v1");
 const auto saved=witness_->fork_checkpoint(chain_id_,genesis_,witness_limit());archive_require(bool(saved));
 const auto intent=Json::parse(saved->snapshot);archive_require(intent.is_object()&&intent.size()==4&&intent.at("format")=="nano-bellatrix-recovery-v1"&&intent.dump()==saved->snapshot);
 const auto previous=recovery_token_parse(intent.at("previous")),next=recovery_token_parse(intent.at("next"));
 archive_require(next&&(!previous||previous->sequence<UINT64_MAX)&&next->sequence==(previous?previous->sequence+1:1));
 const auto encoded=intent.at("snapshot").get<std::string>();
 auto restored=restore_durable_snapshot(encoded,next->digest,rules_,limits_,checkpoint);
 // Re-encoding must be identical before any replay publication can write.
 archive_require(durable_snapshot(restored.store,*restored.blocks,*restored.votes,*restored.slashings,*restored.exits,rules_,limits_,checkpoint)==encoded);
 const auto primary=disk_.fork_checkpoint(chain_id_,genesis_,limits_.envelope_bytes);
 const auto actual=primary?std::optional<ForkCheckpointToken>(primary->token):std::nullopt;
 archive_require(actual==next||actual==previous);witness_token_=saved->token;
 if(actual!=next){
  token_=previous;publish(std::move(restored.store),std::move(restored.blocks),std::move(restored.votes),checkpoint,std::move(restored.slashings),std::move(restored.exits));return;
 }
 const auto& head=bellatrix_fork_head(restored.store,rules_,checkpoint);const auto tip=disk_.latest();const auto state=disk_.state_at(head.execution_block.header.number);
 archive_require(chain_id_==rules_.transition.chain_id&&tip&&tip->header.hash()==head.execution_block.header.hash()&&state&&state->root()==head.execution_state.root());
 for(const auto& receipt:head.receipts){if(checkpoint)checkpoint();const auto persisted=disk_.receipt(receipt.transaction_hash);archive_require(persisted&&persisted->to_json()==receipt.to_json());}
 auto finalized=restored.store.blocks.at(restored.store.timed.view.finalized.root).execution_block;
 const auto canonical=disk_.block_by_number(finalized.header.number);archive_require(canonical&&canonical->header.hash()==finalized.header.hash());
 state_=std::make_shared<const BellatrixForkStore>(std::move(restored.store));signed_blocks_=std::move(restored.blocks);signed_votes_=std::move(restored.votes);signed_slashings_=std::move(restored.slashings);signed_exits_=std::move(restored.exits);token_=*next;
 std::lock_guard chain_lock(execution_->mutex_);execution_->beacon_finalized_=std::move(finalized);execution_->beacon_ready_=true;
}
BellatrixPublishedStore BellatrixDurableReactor::current()const{std::lock_guard lock(mutex_);require_available();return {state_,*token_,signed_blocks_,signed_votes_,signed_slashings_,signed_exits_};}
std::optional<Phase0SignedAttestation> BellatrixDurableReactor::sign_attestation(uint64_t index,
 uint64_t slot,const std::function<uint64_t()>& local_clock,const ForkCheckpointToken& expected,BeaconSigningJournal& journal,
 const BeaconSigningJournal::Signer& signer,const std::function<void()>& checkpoint){
 std::lock_guard lock(mutex_);return sign_attestation_locked(index,slot,local_clock,expected,journal,signer,checkpoint);
}
std::optional<Phase0SignedAttestation> BellatrixDurableReactor::sign_attestation_locked(uint64_t index,
 uint64_t slot,const std::function<uint64_t()>& local_clock,const ForkCheckpointToken& expected,BeaconSigningJournal& journal,
 const BeaconSigningJournal::Signer& signer,const std::function<void()>& checkpoint){
 require_available();
 if(expected!=*token_)throw StaleBellatrixDuty("Stale attestation duty request");
 if(!local_clock||!signer)throw std::invalid_argument("Missing duty clock or signing provider");
 const auto& clock=state_->timed.clock;
 // Per-attempt monotonic observations: remaining in the same slot is not
 // sufficient if the independent wall clock moves backwards during signing.
 uint64_t last_time=clock.time_ms;
 auto timely=[&]{
  const auto time=local_clock();
  if(!rules_.slot_duration_ms||time<last_time||time<clock.genesis_time_ms||(time-clock.genesis_time_ms)/rules_.slot_duration_ms!=slot)return false;
  last_time=time;return true;
 };
 if(!timely())return std::nullopt;
 auto duty=prepare_bellatrix_attestation_duty(*state_,rules_,index,slot,limits_.replay_slots,limits_.sync_trials,checkpoint);if(!duty)return std::nullopt;
 // Non-duty keys must not reread the complete durable archive on every poll.
 // Eligible duties still verify the disk commitment before any signature.
 disk_.fork_checkpoint(chain_id_,genesis_,limits_.envelope_bytes,token_);
 if(journal.chain_!=chain_id_||journal.execution_genesis_!=genesis_||journal.validators_genesis_!=duty->genesis_root||journal.public_key_!=duty->public_key||journal.slots_per_epoch_!=rules_.transition.consensus.epoch.ffg.slots_per_epoch||&journal.disk_==&disk_)
  throw std::invalid_argument("Signing journal does not match authenticated duty");
 if(checkpoint)checkpoint();
 if(!timely())return std::nullopt;
 duty->attestation.signature=journal.sign_attestation(duty->attestation.data,duty->fork_version,[&](const ConsensusRoot& root){
  if(!timely())throw std::runtime_error("Attestation duty expired before provider");
  auto signature=signer(root);
  if(!timely())throw std::runtime_error("Attestation duty expired during provider");
  return signature;
 });
 // The journal verifies BLS after the provider returns. Verification and
 // cancellation handling can outlive the duty too; keep its reservation but
 // never return an already expired or cancelled result.
 if(checkpoint)checkpoint();
 if(!timely())throw std::runtime_error("Attestation duty expired after verification");
 return std::move(duty->attestation);
}
std::optional<Bytes> BellatrixDurableReactor::sign_proposal(uint64_t index,uint64_t slot,
 std::span<const uint8_t> body,const std::function<uint64_t()>& local_clock,
 const ForkCheckpointToken& expected,BeaconSigningJournal& journal,
 const BeaconSigningJournal::Signer& signer,const std::function<void()>& checkpoint){
 std::lock_guard lock(mutex_);return sign_proposal_locked(index,slot,body,local_clock,expected,journal,signer,checkpoint);
}
std::optional<Bytes> BellatrixDurableReactor::sign_proposal_locked(uint64_t index,uint64_t slot,
 std::span<const uint8_t> body,const std::function<uint64_t()>& local_clock,
 const ForkCheckpointToken& expected,BeaconSigningJournal& journal,
 const BeaconSigningJournal::Signer& signer,const std::function<void()>& checkpoint){
 require_available();
 if(expected!=*token_)throw StaleBellatrixDuty("Stale proposal duty request");
 disk_.fork_checkpoint(chain_id_,genesis_,limits_.envelope_bytes,token_);
 if(!local_clock||!signer)throw std::invalid_argument("Missing proposal clock or signing provider");
 const auto& clock=state_->timed.clock;const auto& t=rules_.transition.consensus;
 validate_altair_transition_rules(t);
 if(!rules_.slot_duration_ms||clock.time_ms<clock.genesis_time_ms)throw std::invalid_argument("Invalid proposal clock");
 if((clock.time_ms-clock.genesis_time_ms)/rules_.slot_duration_ms!=slot||state_->timed.view.current_epoch!=slot/t.epoch.ffg.slots_per_epoch||state_->timed.view.equivocating_indices.contains(index))return std::nullopt;
 uint64_t last_time=clock.time_ms;
 auto timely=[&]{const auto time=local_clock();if(time<last_time||time<clock.genesis_time_ms||(time-clock.genesis_time_ms)/rules_.slot_duration_ms!=slot)return false;last_time=time;return true;};
 if(!timely())return std::nullopt;
 const auto head=phase0_fork_choice_head(state_->timed.view,{t.epoch.ffg.slots_per_epoch,t.attestation.duties.effective_balance_increment,rules_.proposer_score_boost},checkpoint).head;
 const auto& parent=state_->blocks.at(head);
 if(parent.header.slot>=slot)return std::nullopt;
 if(rules_.native_deposit_votes&&(body.size()<168||!bellatrix_native_deposit_vote_known(*state_,head,body.subspan(96,72),checkpoint)))return std::nullopt;
 const auto parent_context=bellatrix_fork_block_context(parent.state,parent.header,BellatrixEpochRules{t.epoch,rules_.transition.execution,rules_.transition.maximum_state_bytes},t.state,checkpoint);
 validate_bellatrix_execution_record(parent,rules_,checkpoint);
 if(parent_context.block_root!=head)throw std::invalid_argument("Proposal parent binding mismatch");
 auto candidate=prepare_bellatrix_unsigned_block(parent.state,slot,index,head,body,rules_.transition,parent.execution_block.header,parent.execution_state,[&](const Int& height){return bellatrix_execution_history(*state_,head,height,checkpoint);},limits_.replay_slots,limits_.sync_trials,checkpoint);
 if(!candidate)return std::nullopt;
 const auto& message=candidate->message_ssz;
 const ConsensusBeaconHeader header{slot,index,head,archive_array<32>(message,48),bellatrix_beacon_body_root(std::span<const uint8_t>(message).subspan(84),t.body,t.state.sync_committee_size,rules_.transition.execution,t.maximum_block_bytes,checkpoint)};
 // The post-state is derived locally by the complete operation transition;
 // this checks its header binding and resolves the key/domain from that state.
 const auto context=bellatrix_fork_block_context(candidate->post.consensus,header,BellatrixEpochRules{t.epoch,rules_.transition.execution,rules_.transition.maximum_state_bytes},t.state,checkpoint);
 const auto member=context.registry.validators.find(index);
 if(member==context.registry.validators.end())throw std::invalid_argument("Missing derived proposer identity");
 const Bytes key(member->second.public_key.begin(),member->second.public_key.end());
 if(journal.chain_!=chain_id_||journal.execution_genesis_!=genesis_||journal.validators_genesis_!=context.genesis_root||journal.public_key_!=key||journal.slots_per_epoch_!=t.epoch.ffg.slots_per_epoch||&journal.disk_==&disk_)
  throw std::invalid_argument("Signing journal does not match authenticated proposal");
 const auto epoch=slot/t.epoch.ffg.slots_per_epoch;const auto version=epoch<context.fork.epoch?context.fork.previous_version:context.fork.current_version;
 // Allocate the result before reserving/signing, with only the signature empty.
 Bytes wire(100,0);wire[0]=100;wire.insert(wire.end(),message.begin(),message.end());
 if(checkpoint)checkpoint();if(!timely())return std::nullopt;
 const auto signature=journal.sign_proposal(header,version,[&](const ConsensusRoot& root){
  if(!timely())throw std::runtime_error("Proposal expired before provider");
  auto result=signer(root);
  if(!timely())throw std::runtime_error("Proposal expired during provider");
  return result;
 });
 std::copy(signature.begin(),signature.end(),wire.begin()+4);
 if(checkpoint)checkpoint();if(!timely())throw std::runtime_error("Proposal expired after verification");
 return wire;
}
std::optional<Bytes> BellatrixDurableReactor::sign_randao(uint64_t index,uint64_t slot,
 const std::function<uint64_t()>& local_clock,const ForkCheckpointToken& expected,
 BeaconSigningJournal& journal,const BeaconSigningJournal::Signer& signer,const std::function<void()>& checkpoint){
 std::lock_guard lock(mutex_);return sign_randao_locked(index,slot,local_clock,expected,journal,signer,checkpoint);
}
std::optional<Bytes> BellatrixDurableReactor::sign_randao_locked(uint64_t index,uint64_t slot,
 const std::function<uint64_t()>& local_clock,const ForkCheckpointToken& expected,
 BeaconSigningJournal& journal,const BeaconSigningJournal::Signer& signer,const std::function<void()>& checkpoint){
 require_available();
 if(expected!=*token_)throw StaleBellatrixDuty("Stale RANDAO duty request");
 if(!local_clock||!signer)throw std::invalid_argument("Missing RANDAO clock or signing provider");
 const auto& clock=state_->timed.clock;const auto& t=rules_.transition.consensus;validate_altair_transition_rules(t);
 if(!rules_.slot_duration_ms||clock.time_ms<clock.genesis_time_ms)throw std::invalid_argument("Invalid RANDAO clock");
 if((clock.time_ms-clock.genesis_time_ms)/rules_.slot_duration_ms!=slot||state_->timed.view.current_epoch!=slot/t.epoch.ffg.slots_per_epoch||state_->timed.view.equivocating_indices.contains(index))return std::nullopt;
 uint64_t last_time=clock.time_ms;
 auto timely=[&]{const auto time=local_clock();if(time<last_time||time<clock.genesis_time_ms||(time-clock.genesis_time_ms)/rules_.slot_duration_ms!=slot)return false;last_time=time;return true;};
 if(!timely())return std::nullopt;
 const auto head=phase0_fork_choice_head(state_->timed.view,{t.epoch.ffg.slots_per_epoch,t.attestation.duties.effective_balance_increment,rules_.proposer_score_boost},checkpoint).head;
 const auto& parent=state_->blocks.at(head);
 if(consensus_beacon_header_root(parent.header)!=head)throw std::invalid_argument("RANDAO parent binding mismatch");
 const auto identity=prepare_bellatrix_proposer_identity(parent.state,parent.header,slot,BellatrixEpochRules{t.epoch,rules_.transition.execution,rules_.transition.maximum_state_bytes},t.state,t.attestation.duties,limits_.replay_slots,limits_.sync_trials,checkpoint);
 if(!identity||identity->index!=index)return std::nullopt;
 // Non-duty keys must not reread the complete durable archive on every poll.
 // Eligible duties still verify the disk commitment before any signature.
 disk_.fork_checkpoint(chain_id_,genesis_,limits_.envelope_bytes,token_);
 if(journal.chain_!=chain_id_||journal.execution_genesis_!=genesis_||journal.validators_genesis_!=identity->genesis_root||journal.public_key_!=identity->public_key||journal.slots_per_epoch_!=t.epoch.ffg.slots_per_epoch||&journal.disk_==&disk_)
  throw std::invalid_argument("Signing journal does not match authenticated RANDAO duty");
 ConsensusRoot epoch_root{};const auto epoch=slot/t.epoch.ffg.slots_per_epoch;
 for(unsigned i=0;i<8;++i)epoch_root[i]=uint8_t(epoch>>(8*i));
 const auto root=consensus_signing_root(epoch_root,consensus_domain({2,0,0,0},identity->fork_version,identity->genesis_root));
 if(checkpoint)checkpoint();if(!timely())return std::nullopt;
 // Serialize with retirement/import/signing on this key, without fabricating
 // a proposal reservation for an epoch-only RANDAO signature.
 std::lock_guard signing_lock(journal.mutex_);journal.ready();
 if(journal.retired_)throw std::runtime_error("Beacon signing journal permanently retired");
 if(!timely())return std::nullopt;
 auto signature=journal.sign(root,[&](const ConsensusRoot& signing_root){
  auto result=signer(signing_root);
  if(!timely())throw std::runtime_error("RANDAO expired during provider");
  return result;
 });
 if(checkpoint)checkpoint();if(!timely())throw std::runtime_error("RANDAO expired after verification");
 return signature;
}
std::optional<Bytes> BellatrixDurableReactor::propose_local(uint64_t index,uint64_t slot,const std::string& recipient,
 size_t maximum_vote_trials,const std::function<uint64_t()>& local_clock,const ForkCheckpointToken& expected,
 BeaconSigningJournal& journal,const BeaconSigningJournal::Signer& signer,const std::function<void()>& checkpoint){
 std::lock_guard lock(mutex_);return propose_local_locked(index,slot,recipient,maximum_vote_trials,local_clock,expected,journal,signer,checkpoint);
}
std::optional<Bytes> BellatrixDurableReactor::propose_local_locked(uint64_t index,uint64_t slot,const std::string& recipient,
 size_t maximum_vote_trials,const std::function<uint64_t()>& local_clock,const ForkCheckpointToken& expected,
 BeaconSigningJournal& journal,const BeaconSigningJournal::Signer& signer,const std::function<void()>& checkpoint){
 require_available();
 const auto reveal=sign_randao_locked(index,slot,local_clock,expected,journal,signer,checkpoint);
 if(!reveal)return std::nullopt;
 const auto body=prepare_local_body_locked(index,slot,*reveal,recipient,expected,maximum_vote_trials,checkpoint);
 if(!body)return std::nullopt;
 return sign_proposal_locked(index,slot,*body,local_clock,expected,journal,signer,checkpoint);
}
std::optional<uint64_t> BellatrixDurableReactor::local_proposer(uint64_t slot,const ForkCheckpointToken& expected,const std::function<void()>& checkpoint){
 std::lock_guard lock(mutex_);return local_proposer_locked(slot,expected,checkpoint);
}
std::optional<uint64_t> BellatrixDurableReactor::local_proposer_locked(uint64_t slot,const ForkCheckpointToken& expected,const std::function<void()>& checkpoint){
 require_available();
 if(expected!=*token_)throw StaleBellatrixDuty("Stale proposer selection");
 const auto& t=rules_.transition.consensus;const auto& c=state_->timed.clock;
 if((c.time_ms-c.genesis_time_ms)/rules_.slot_duration_ms!=slot)return std::nullopt;
 const auto& parent=bellatrix_fork_head(*state_,rules_,checkpoint);
 const auto identity=prepare_bellatrix_proposer_identity(parent.state,parent.header,slot,BellatrixEpochRules{t.epoch,rules_.transition.execution,rules_.transition.maximum_state_bytes},t.state,t.attestation.duties,limits_.replay_slots,limits_.sync_trials,checkpoint);
 if(!identity||state_->timed.view.equivocating_indices.contains(identity->index))return std::nullopt;
 return identity->index;
}
std::set<uint64_t> BellatrixDurableReactor::local_attesters(uint64_t slot,const ForkCheckpointToken& expected,const std::function<void()>& checkpoint){
 std::lock_guard lock(mutex_);return local_attesters_locked(slot,expected,checkpoint);
}
std::set<uint64_t> BellatrixDurableReactor::local_attesters_locked(uint64_t slot,const ForkCheckpointToken& expected,const std::function<void()>& checkpoint){
 require_available();
 if(expected!=*token_)throw StaleBellatrixDuty("Stale attester selection");
 const auto duties=prepare_bellatrix_attestation_duties(*state_,rules_,slot,limits_.replay_slots,limits_.sync_trials,checkpoint);
 std::set<uint64_t> result;for(const auto& [index,duty]:duties)result.insert(index);return result;
}
// Local callers provide intent (key/slot), not precomputed signed data. Derive
// and verify against the current authenticated state under this same lock.
std::optional<uint64_t> BellatrixDurableReactor::current_proposer(uint64_t slot,const std::function<void()>& checkpoint){
 std::lock_guard lock(mutex_);require_available();return local_proposer_locked(slot,*token_,checkpoint);
}
std::set<uint64_t> BellatrixDurableReactor::current_attesters(uint64_t slot,const std::function<void()>& checkpoint){
 std::lock_guard lock(mutex_);require_available();return local_attesters_locked(slot,*token_,checkpoint);
}
std::optional<Phase0SignedAttestation> BellatrixDurableReactor::attest_current(uint64_t index,uint64_t slot,
 const std::function<uint64_t()>& clock,BeaconSigningJournal& journal,const BeaconSigningJournal::Signer& signer,const std::function<void()>& checkpoint){
 std::lock_guard lock(mutex_);require_available();return sign_attestation_locked(index,slot,clock,*token_,journal,signer,checkpoint);
}
std::optional<Bytes> BellatrixDurableReactor::propose_current(uint64_t index,uint64_t slot,const std::string& recipient,
 size_t trials,const std::function<uint64_t()>& clock,BeaconSigningJournal& journal,const BeaconSigningJournal::Signer& signer,const std::function<void()>& checkpoint){
 std::lock_guard lock(mutex_);require_available();return propose_local_locked(index,slot,recipient,trials,clock,*token_,journal,signer,checkpoint);
}
void BellatrixDurableReactor::tick(uint64_t ms,const std::function<void()>& checkpoint){
 std::lock_guard lock(mutex_);require_available();
 auto next=prepare_bellatrix_store_tick(*state_,ms,rules_,limits_.replay_slots,limits_.sync_trials,checkpoint);
 const auto& previous_clock=state_->timed.clock;const auto previous_slot=(previous_clock.time_ms-previous_clock.genesis_time_ms)/rules_.slot_duration_ms;
 const auto current_slot=(ms-previous_clock.genesis_time_ms)/rules_.slot_duration_ms;
 if(current_slot>previous_slot){
  for(const auto& [id,wire]:*signed_votes_){
   if(checkpoint)checkpoint();const auto vote=decode_consensus_attestation_ssz(*wire,rules_.transition.consensus.body.validators_per_committee,checkpoint);
   if(vote&&vote->data.slot==previous_slot){
    auto applied=prepare_bellatrix_store_vote(next,*vote,rules_,limits_.replay_slots,limits_.sync_trials,checkpoint);
    if(applied)next=std::move(*applied);
   }
  }
 }
 publish(std::move(next),signed_blocks_,signed_votes_,checkpoint);
}
bool BellatrixDurableReactor::block(std::span<const uint8_t> bytes,const std::function<void()>& checkpoint){
 std::lock_guard lock(mutex_);require_available();auto next=prepare_bellatrix_received_block(*state_,bytes,rules_,limits_.replay_slots,limits_.sync_trials,checkpoint);if(!next)return false;
 auto archive=signed_blocks_;
 if(next->blocks.size()>state_->blocks.size()){
  archive_require(next->blocks.size()==state_->blocks.size()+1);auto updated=std::make_shared<BellatrixSignedBlockArchive>(*signed_blocks_);bool added=false;
  for(const auto& [id,_]:next->blocks)if(!state_->blocks.contains(id)){archive_require(!added&&updated->emplace(id,std::make_shared<const Bytes>(bytes.begin(),bytes.end())).second);added=true;}
  archive_require(added);archive=std::move(updated);
 }
 // A duplicate root can bypass signature verification in fork-choice. Never
 // overwrite archived bytes, or backfill a legacy checkpoint from that path.
 publish(std::move(*next),std::move(archive),signed_votes_,checkpoint);return true;
}
bool BellatrixDurableReactor::vote(const Phase0SignedAttestation& vote,const std::function<void()>& checkpoint){
 std::lock_guard lock(mutex_);require_available();auto next=checked_vote(*state_,vote,rules_,limits_,checkpoint);if(!next)return false;
 auto votes=signed_votes_;
 {
  auto bytes=std::make_shared<const Bytes>(encode_consensus_attestation_ssz(vote,rules_.transition.consensus.body.validators_per_committee,checkpoint));
  const auto id=archive_array<32>(sha256(*bytes),0);
  if(!signed_votes_->contains(id)){
   if(signed_votes_->size()>=limits_.messages)throw std::length_error("Bellatrix vote archive capacity");
   auto updated=std::make_shared<BellatrixSignedVoteArchive>(*signed_votes_);updated->emplace(id,std::move(bytes));votes=std::move(updated);
  }
 }
 publish(std::move(*next),signed_blocks_,std::move(votes),checkpoint);return true;
}
bool BellatrixDurableReactor::slashing(const IndexedConsensusAttestation& a,const IndexedConsensusAttestation& b,const std::function<void()>& checkpoint){
 std::lock_guard lock(mutex_);require_available();auto next=prepare_bellatrix_store_slashing(*state_,a,b,rules_,checkpoint);if(!next)return false;
 auto wire=std::make_shared<const Bytes>(encode_consensus_attester_slashing_ssz(a,b,rules_.transition.consensus.body.validators_per_committee,checkpoint));const auto id=archive_array<32>(sha256(*wire),0);
 auto proofs=signed_slashings_;
 if(!proofs->contains(id)){
  // A signer can generate unlimited distinct double-vote pairs. Retain a new
  // proof only if it covers an offender not covered by the existing archive.
  // Fork-choice exclusions alone are insufficient: they can originate in votes
  // or blocks without leaving a pending proof available to the local proposer.
  std::set<uint64_t> uncovered;
  std::set_intersection(a.indices.begin(),a.indices.end(),b.indices.begin(),b.indices.end(),std::inserter(uncovered,uncovered.end()));
  const auto& finalized=state_->blocks.at(state_->timed.view.finalized.root);const auto& t=rules_.transition.consensus;
  const auto context=bellatrix_fork_block_context(finalized.state,finalized.header,{t.epoch,rules_.transition.execution,rules_.transition.maximum_state_bytes},t.state,checkpoint);
  std::erase_if(uncovered,[&](auto index){if(checkpoint)checkpoint();const auto member=context.registry.validators.find(index);return member!=context.registry.validators.end()&&member->second.slashed;});

  for(const auto& [_,saved]:*proofs){
   if(checkpoint)checkpoint();if(uncovered.empty())break;
   const auto decoded=decode_consensus_attester_slashing_ssz(*saved,rules_.transition.consensus.body.validators_per_committee,checkpoint);
   if(!decoded)throw std::logic_error("Invalid archived slashing encoding");
   for(auto index:decoded->first.indices){if(checkpoint)checkpoint();if(std::binary_search(decoded->second.indices.begin(),decoded->second.indices.end(),index))uncovered.erase(index);}
  }
  if(!uncovered.empty()){
   if(proofs->size()>=limits_.equivocations)throw std::length_error("Bellatrix slashing archive capacity");
   auto candidate=std::make_shared<BellatrixSignedVoteArchive>(*proofs);candidate->emplace(id,std::move(wire));proofs=std::move(candidate);
  }
 }
 publish(std::move(*next),signed_blocks_,signed_votes_,checkpoint,std::move(proofs));return true;
}
}

namespace nano {
bool BellatrixDurableReactor::voluntary_exit_ssz(std::span<const uint8_t> wire,const std::function<void()>& check){
 const auto exit=decode_consensus_voluntary_exit_ssz(wire,check);if(!exit)return false;
 std::lock_guard lock(mutex_);require_available();const auto& t=rules_.transition.consensus;
 const auto& head=bellatrix_fork_head(*state_,rules_,check);
 const auto context=bellatrix_fork_block_context(head.state,head.header,{t.epoch,rules_.transition.execution,rules_.transition.maximum_state_bytes},t.state,check);
 if(!valid_exit_signature(*exit,context,t.exit,check))return false;
 const auto& finalized=state_->blocks.at(state_->timed.view.finalized.root);
 const auto final_context=bellatrix_fork_block_context(finalized.state,finalized.header,{t.epoch,rules_.transition.execution,rules_.transition.maximum_state_bytes},t.state,check);
 const auto member=final_context.registry.validators.find(exit->validator_index);
 if(member!=final_context.registry.validators.end()&&member->second.exit_epoch!=UINT64_MAX)return true;
 for(const auto& [_,saved]:*signed_exits_){if(check)check();if(archive_number(saved->ssz,8)==exit->validator_index)return true;}
 if(signed_exits_->size()>=limits_.messages)throw std::length_error("Bellatrix exit archive capacity");
 auto exits=std::make_shared<BellatrixSignedExitArchive>(*signed_exits_);auto record=std::make_shared<const BellatrixSignedExitRecord>(BellatrixSignedExitRecord{consensus_beacon_header_root(head.header),Bytes(wire.begin(),wire.end())});
 const auto id=archive_array<32>(sha256(record->ssz),0);exits->emplace(id,std::move(record));publish(*state_,signed_blocks_,signed_votes_,check,{},std::move(exits));return true;
}
bool BellatrixDurableReactor::vote_ssz(std::span<const uint8_t> bytes,const std::function<void()>& checkpoint){
 auto decoded=decode_consensus_attestation_ssz(bytes,rules_.transition.consensus.body.validators_per_committee,checkpoint);
 if(!decoded){std::lock_guard lock(mutex_);require_available();return false;}
 return vote(*decoded,checkpoint);
}
bool BellatrixDurableReactor::slashing_ssz(std::span<const uint8_t> bytes,const std::function<void()>& checkpoint){
 auto decoded=decode_consensus_attester_slashing_ssz(bytes,rules_.transition.consensus.body.validators_per_committee,checkpoint);
 if(!decoded){std::lock_guard lock(mutex_);require_available();return false;}
 return slashing(decoded->first,decoded->second,checkpoint);
}
}

namespace nano {
Json BellatrixDurableReactor::network_identity()const{
 std::lock_guard lock(mutex_);require_available();const auto& anchor=state_->timed.view.anchor;
 const auto encoded=Json::array({"nano-bellatrix-network-v1",quantity(chain_id_),hex(genesis_),hex(Bytes(anchor.begin(),anchor.end())),bellatrix_fork_profile(rules_)}).dump();const auto digest=sha256(Bytes(encoded.begin(),encoded.end()));
 return Json{{"chainId",quantity(chain_id_)},{"genesisHash",hex(genesis_)},{"consensusIdentity",hex(digest)},{"forkDigest",hex(Bytes(digest.begin(),digest.begin()+4))}};
}
}
