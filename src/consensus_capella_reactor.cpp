#include "nano/consensus_capella_reactor.hpp"
#include "nano/chain.hpp"
#include "nano/consensus_bls.hpp"
#include "nano/consensus_message_ssz.hpp"
namespace nano {
namespace {
void archive_require(bool value){if(!value)throw std::invalid_argument("Invalid Capella durable archive");}
uint64_t archive_number(std::span<const uint8_t> bytes,size_t at,size_t width=8){uint64_t value=0;for(size_t i=0;i<width;++i)value|=uint64_t(bytes[at+i])<<(8*i);return value;}
template<size_t N>std::array<uint8_t,N> archive_array(std::span<const uint8_t> bytes,size_t at){std::array<uint8_t,N> out{};std::copy_n(bytes.begin()+at,N,out.begin());return out;}
void verify_archived_block(const ConsensusRoot& id,const Bytes& wire,const CapellaForkStore& store,const CapellaForkBlockRules& rules,const std::function<void()>& check){
 const auto& r=rules.transition;
 archive_require(id!=store.timed.view.anchor&&store.blocks.contains(id)&&wire.size()>=404&&wire.size()<=r.consensus.maximum_block_bytes);
 archive_require(archive_number(wire,0,4)==100&&archive_number(wire,180,4)==84);
 const ConsensusBeaconHeader header{archive_number(wire,100),archive_number(wire,108),archive_array<32>(wire,116),archive_array<32>(wire,148),capella_beacon_body_root(std::span(wire).subspan(184),r.consensus.body,r.consensus.state.sync_committee_size,r.execution,r.withdrawals.maximum_per_payload,r.maximum_bls_changes,r.consensus.maximum_block_bytes,check)};
 const auto& block=store.blocks.at(id);archive_require(header==block.header&&consensus_beacon_header_root(header)==id);
 const auto context=capella_fork_block_context(block.state,header,CapellaEpochRules{r.consensus.epoch,r.execution,r.maximum_state_bytes},r.consensus.state,check);
 const auto proposer=context.registry.validators.find(header.proposer_index);archive_require(proposer!=context.registry.validators.end());
 const auto& state=block.state.ssz;const auto version=archive_array<4>(state,context.registry.epoch<archive_number(state,56)?48:52);
 archive_require(verify_consensus_beacon_header(header,version,archive_array<32>(state,8),proposer->second.public_key,std::span(wire).subspan(4,96)));if(check)check();
}
std::optional<CapellaForkStore> checked_vote(const CapellaForkStore& store,const Phase0SignedAttestation& vote,const CapellaForkBlockRules& rules,const CapellaStoreSnapshotLimits& limits,const std::function<void()>& check){
 const auto& clock=store.timed.clock;if(!rules.slot_duration_ms||clock.time_ms<clock.genesis_time_ms)throw std::invalid_argument("Capella vote clock");
 const auto slot=(clock.time_ms-clock.genesis_time_ms)/rules.slot_duration_ms;
 if(vote.data.slot!=slot)return prepare_capella_store_vote(store,vote,rules,limits.replay_slots,limits.sync_trials,check);
 if(slot==UINT64_MAX||slot+1>(UINT64_MAX-clock.genesis_time_ms)/rules.slot_duration_ms)throw std::overflow_error("Capella deferred vote clock");
 const auto future=prepare_capella_store_tick(store,clock.genesis_time_ms+(slot+1)*rules.slot_duration_ms,rules,limits.replay_slots,limits.sync_trials,check);
 if(!prepare_capella_store_vote(future,vote,rules,limits.replay_slots,limits.sync_trials,check))return std::nullopt;
 return store;
}
std::string durable_snapshot(const CapellaForkStore& store,const CapellaSignedVotes& votes,const CapellaSignedVotes& slashings,const CapellaForkBlockRules& rules,const CapellaStoreSnapshotLimits& limits,const std::function<void()>& check,CapellaStoreSnapshotCache* cache=nullptr){
 const auto encoded=capella_store_snapshot(store,rules,limits,check,cache);if(votes.empty()&&slashings.empty())return encoded;
 if(votes.size()>limits.messages)throw std::length_error("Capella signed vote capacity");Json list=Json::array();size_t total=0;
 for(const auto& [id,wire]:votes){if(check)check();if(wire.size()>limits.envelope_bytes/2-total)throw std::length_error("Capella signed vote bytes");total+=wire.size();list.push_back(Json::array({hex(id),hex(wire)}));}
 Json document{{"format","nano-capella-durable-v1"},{"store",Json::parse(encoded)},{"signedVotes",list}};
 if(!slashings.empty()){
  if(slashings.size()>limits.equivocations)throw std::length_error("Capella slashing capacity");Json proofs=Json::array();
  for(const auto& [id,wire]:slashings){if(check)check();if(wire.size()>limits.envelope_bytes/2-total)throw std::length_error("Capella slashing bytes");total+=wire.size();proofs.push_back(Json::array({hex(id),hex(wire)}));}
  document["format"]="nano-capella-durable-v2";document["signedSlashings"]=std::move(proofs);
 }
 const auto result=document.dump();
 if(result.size()>limits.envelope_bytes)throw std::length_error("Capella durable envelope");return result;
}
std::tuple<CapellaForkStore,CapellaSignedVotes,CapellaSignedVotes> restore_durable(std::string_view encoded,const Bytes& digest,const CapellaForkBlockRules& rules,const CapellaStoreSnapshotLimits& limits,const std::function<void()>& check){
 if(check)check();archive_require(encoded.size()<=limits.envelope_bytes&&digest.size()==32&&sha256(Bytes(encoded.begin(),encoded.end()))==digest);
 const auto document=Json::parse(encoded);
 const bool with_proofs=document.value("format",std::string())=="nano-capella-durable-v2";
 if(!with_proofs&&document.value("format",std::string())!="nano-capella-durable-v1")return {capella_store_from_snapshot(encoded,digest,rules,limits,check),{}, {}};
 archive_require(document.size()==(with_proofs?4:3)&&document.at("signedVotes").is_array()&&(with_proofs||!document.at("signedVotes").empty())&&document.at("signedVotes").size()<=limits.messages);
 const auto inner=document.at("store").dump();auto store=capella_store_from_snapshot(inner,sha256(Bytes(inner.begin(),inner.end())),rules,limits,check);CapellaSignedVotes votes,proofs;
 for(const auto& entry:document.at("signedVotes")){
  if(check)check();archive_require(entry.is_array()&&entry.size()==2&&entry[0].is_string()&&entry[1].is_string());
  const auto key=entry[0].get<std::string>(),payload=entry[1].get<std::string>();archive_require(key.size()==66&&payload.size()>=2&&payload.size()%2==0&&(payload.size()-2)/2<=228+uint64_t(rules.transition.consensus.body.validators_per_committee)/8+1);
  auto id=unhex(key),wire=unhex(payload);archive_require(hex(id)==key&&hex(wire)==payload&&sha256(wire)==id);
  const auto vote=decode_consensus_attestation_ssz(wire,rules.transition.consensus.body.validators_per_committee,check);archive_require(vote&&checked_vote(store,*vote,rules,limits,check).has_value());archive_require(votes.emplace(std::move(id),std::move(wire)).second);
 }
 if(with_proofs){
  const auto& list=document.at("signedSlashings");archive_require(list.is_array()&&!list.empty()&&list.size()<=limits.equivocations);
  for(const auto& entry:list){
   if(check)check();archive_require(entry.is_array()&&entry.size()==2&&entry[0].is_string()&&entry[1].is_string());const auto key=entry[0].get<std::string>(),payload=entry[1].get<std::string>();
   archive_require(key.size()==66&&payload.size()>=2&&payload.size()%2==0&&(payload.size()-2)/2<=464+uint64_t(rules.transition.consensus.body.validators_per_committee)*16);
   auto id=unhex(key),wire=unhex(payload);archive_require(hex(id)==key&&hex(wire)==payload&&sha256(wire)==id);
   const auto decoded=decode_consensus_attester_slashing_ssz(wire,rules.transition.consensus.body.validators_per_committee,check);archive_require(bool(decoded));
   const auto applied=prepare_capella_store_slashing(store,decoded->first,decoded->second,rules,check);archive_require(applied&&applied->timed.view.equivocating_indices==store.timed.view.equivocating_indices);
   archive_require(proofs.emplace(std::move(id),std::move(wire)).second);
  }
 }
 archive_require(durable_snapshot(store,votes,proofs,rules,limits,check)==encoded);return {std::move(store),std::move(votes),std::move(proofs)};
}

Json recovery_token_json(const std::optional<ForkCheckpointToken>& token){return token?Json{{"sequence",quantity(Int(std::to_string(token->sequence)))},{"digest",hex(token->digest)}}:Json(nullptr);}
std::optional<ForkCheckpointToken> recovery_token_parse(const Json& j){
 if(j.is_null())return std::nullopt;archive_require(j.is_object()&&j.size()==2);
 ForkCheckpointToken token{u64(quantity(j.at("sequence"))),unhex(j.at("digest").get<std::string>())};archive_require(token.sequence&&token.digest.size()==32&&recovery_token_json(token)==j);return token;
}

}

size_t CapellaDurableReactor::witness_limit()const{
 if(limits_.envelope_bytes>(SIZE_MAX-2048)/8)throw std::length_error("Capella witness size overflow");return 2048+8*limits_.envelope_bytes;
}
void CapellaDurableReactor::witness_prepare(std::string_view snapshot,const std::vector<CheckpointArchiveEntry>& archive){
 if(!witness_)return;if(token_&&token_->sequence==UINT64_MAX)throw std::overflow_error("Capella witness sequence exhausted");
 archive_require(archive.size()<=1);Json entries=Json::array();for(const auto& entry:archive){archive_require(entry.root.size()==32&&entry.payload.size()<=limits_.envelope_bytes);entries.push_back(Json::array({hex(entry.root),quantity(Int(std::to_string(entry.slot))),hex(entry.payload)}));}
 const ForkCheckpointToken next{token_?token_->sequence+1:1,sha256(Bytes(snapshot.begin(),snapshot.end()))};
 const auto encoded=Json{{"format","nano-capella-recovery-v1"},{"previous",recovery_token_json(token_)},{"next",recovery_token_json(next)},{"snapshot",snapshot},{"archive",entries}}.dump();
 witness_token_=witness_->commit_fork_checkpoint(chain_,genesis_,witness_token_,encoded,witness_limit());
}
void CapellaDurableReactor::validate_execution_binding()const{
 if(!execution_)return;std::lock_guard lock(execution_->mutex_);const auto& g=execution_->genesis_;
 if(&execution_->store_!=&disk_||g.consensus!="beacon"||g.evm_revision!=EvmRevision::Shanghai||g.chain_id!=chain_||g.block.header.hash()!=genesis_)throw std::invalid_argument("Capella execution owner/profile mismatch");
 const auto tip=disk_.latest();if(!tip||tip->header.hash()!=execution_->tip_.header.hash()||execution_->state_.root()!=tip->header.state_root)throw std::invalid_argument("Capella execution cache mismatch");
}
void CapellaDurableReactor::fail_closed(){unavailable_=true;if(execution_){std::lock_guard lock(execution_->mutex_);execution_->beacon_ready_=false;}}
CapellaDurableReactor::~CapellaDurableReactor(){if(execution_){std::lock_guard lock(execution_->mutex_);execution_->beacon_ready_=false;}}
void CapellaDurableReactor::invalidate(){std::lock_guard lock(mutex_);fail_closed();}
void CapellaDurableReactor::require_available()const{if(unavailable_||!state_||!token_)throw std::runtime_error("Capella reactor unavailable; reopen required");}
void CapellaDurableReactor::publish(CapellaForkStore candidate,const std::function<void()>& check,const std::vector<CheckpointArchiveEntry>& archive,std::optional<CapellaSignedVotes> votes,std::optional<CapellaSignedVotes> slashings){
 if(unavailable_)throw std::runtime_error("Capella reactor unavailable; reopen required");
 if(chain_!=rules_.transition.chain_id||genesis_.size()!=32)throw std::invalid_argument("Capella reactor chain/profile mismatch");
 auto next=std::make_shared<const CapellaForkStore>(std::move(candidate));
 const auto encoded=durable_snapshot(*next,votes?*votes:votes_,slashings?*slashings:slashings_,rules_,limits_,check,&cache_);
 const auto& head=capella_fork_head(*next,rules_,check);
 const auto tip=disk_.latest();if(!tip)throw std::invalid_argument("Capella reactor requires persisted execution anchor");
 if(state_){
  if(tip->header.hash()!=capella_fork_head(*state_,rules_,check).execution_block.header.hash()){fail_closed();throw std::runtime_error("Capella execution tip changed outside reactor");}
  const auto& old=state_->timed.view;const auto& view=next->timed.view;
  if(view.anchor!=old.anchor||view.finalized.epoch<old.finalized.epoch||view.justified.epoch<old.justified.epoch||next->timed.clock.genesis_time_ms!=state_->timed.clock.genesis_time_ms||next->timed.clock.time_ms<state_->timed.clock.time_ms)throw std::invalid_argument("Capella checkpoint regression");
  auto at=consensus_beacon_header_root(head.header);const auto final_slot=state_->blocks.at(old.finalized.root).header.slot;
  while(next->blocks.at(at).header.slot>final_slot){if(check)check();at=next->blocks.at(at).header.parent_root;}
  if(at!=old.finalized.root||(view.finalized.epoch==old.finalized.epoch&&view.finalized.root!=old.finalized.root))throw std::invalid_argument("Capella finalized branch changed");
 }
 const auto digest=sha256(Bytes(encoded.begin(),encoded.end()));
 if(token_&&digest==token_->digest){if(check)check();try{if(witness_)archive_require(bool(witness_->fork_checkpoint(chain_,genesis_,witness_limit(),witness_token_)));if(!disk_.fork_checkpoint(chain_,genesis_,limits_.envelope_bytes,token_))throw std::runtime_error("missing Capella checkpoint");}catch(...){fail_closed();throw;}return;}
 std::vector<ExecutionCheckpointBlock> suffix;auto at=consensus_beacon_header_root(head.header);Int common_height;Bytes common_hash;
 for(;;){
  if(check)check();const auto& record=next->blocks.at(at);const auto& h=record.execution_block.header;const auto existing=disk_.block_by_number(h.number);
  if(existing&&existing->header.hash()==h.hash()){common_height=h.number;common_hash=h.hash();break;}
  if(suffix.size()>=limits_.blocks)throw std::length_error("Capella publication suffix budget");suffix.push_back({record.execution_block,record.execution_state,record.receipts});
  if(at==next->timed.view.anchor){if(h.number<=0)throw std::invalid_argument("Capella execution anchor missing");const auto parent=disk_.block_by_number(h.number-1);if(!parent||parent->header.hash()!=h.parent_hash)throw std::invalid_argument("Capella execution anchor parent mismatch");common_height=h.number-1;common_hash=h.parent_hash;break;}
  at=record.header.parent_root;
 }
 std::reverse(suffix.begin(),suffix.end());if(check)check();
 ForkCheckpointToken committed;
 try{
  witness_prepare(encoded,archive);
  if(execution_)committed=execution_->commit_beacon_checkpoint(token_,tip->header.hash(),common_height,common_hash,suffix,next->blocks.at(next->timed.view.finalized.root).execution_block,encoded,limits_.envelope_bytes,limits_.blocks,archive);
  else committed=disk_.commit_execution_checkpoint(chain_,genesis_,token_,tip->header.hash(),common_height,common_hash,suffix,encoded,limits_.envelope_bytes,limits_.blocks,archive);
 }
 catch(...){fail_closed();throw;}
 // No allocation or callback after durable success and before publication.
 token_=std::move(committed);state_=std::move(next);if(votes)votes_=std::move(*votes);if(slashings)slashings_=std::move(*slashings);
}
CapellaDurableReactor::CapellaDurableReactor(ChainStore& disk,Int chain,Bytes genesis,CapellaForkBlockRules rules,CapellaStoreSnapshotLimits limits,CapellaForkStore initial,const std::function<void()>& check,Blockchain* owner,ChainStore* witness):disk_(disk),execution_(owner),chain_(std::move(chain)),genesis_(std::move(genesis)),rules_(std::move(rules)),limits_(limits),mutex_(disk.altair_reactor_mutex_){std::lock_guard lock(mutex_);validate_execution_binding();if(witness){archive_require(witness!=&disk_);witness_=witness;witness_->bind_chain(chain_,genesis_,"nano-capella-recovery-v1");archive_require(!witness_->fork_checkpoint(chain_,genesis_,witness_limit())&&!disk_.fork_checkpoint(chain_,genesis_,limits_.envelope_bytes));}publish(std::move(initial),check);}
CapellaDurableReactor::CapellaDurableReactor(ChainStore& disk,Int chain,Bytes genesis,CapellaForkBlockRules rules,CapellaStoreSnapshotLimits limits,const ForkCheckpointToken& expected,const std::function<void()>& check,Blockchain* owner):disk_(disk),execution_(owner),chain_(std::move(chain)),genesis_(std::move(genesis)),rules_(std::move(rules)),limits_(limits),mutex_(disk.altair_reactor_mutex_){
 std::lock_guard lock(mutex_);validate_execution_binding();
 if(chain_!=rules_.transition.chain_id||genesis_.size()!=32)throw std::invalid_argument("Capella reactor chain/profile mismatch");
 const auto record=disk_.fork_checkpoint(chain_,genesis_,limits_.envelope_bytes,expected);if(!record)throw std::invalid_argument("Capella restart checkpoint missing");
 auto [restored,votes,slashings]=restore_durable(record->snapshot,expected.digest,rules_,limits_,check);const auto& head=capella_fork_head(restored,rules_,check);const auto tip=disk_.latest();const auto state=disk_.state_at(head.execution_block.header.number);
 if(!tip||!state||tip->header.hash()!=head.execution_block.header.hash()||state->root()!=head.execution_state.root())throw std::invalid_argument("Capella restart execution mismatch");
 for(const auto& receipt:head.receipts){if(check)check();const auto saved=disk_.receipt(receipt.transaction_hash);archive_require(saved&&saved->to_json()==receipt.to_json());}
 auto finalized=restored.blocks.at(restored.timed.view.finalized.root).execution_block;const auto canonical=disk_.block_by_number(finalized.header.number);archive_require(canonical&&canonical->header.hash()==finalized.header.hash());
 if(check)check();state_=std::make_shared<const CapellaForkStore>(std::move(restored));token_=expected;votes_=std::move(votes);slashings_=std::move(slashings);
 if(execution_){std::lock_guard chain_lock(execution_->mutex_);execution_->beacon_finalized_=std::move(finalized);execution_->beacon_ready_=true;}
}
CapellaDurableReactor::CapellaDurableReactor(Blockchain& owner,CapellaForkBlockRules rules,CapellaStoreSnapshotLimits limits,CapellaForkStore initial,const std::function<void()>& check,ChainStore* witness):CapellaDurableReactor(owner.store_,owner.genesis_.chain_id,owner.genesis_.block.header.hash(),std::move(rules),limits,std::move(initial),check,&owner,witness){}
CapellaDurableReactor::CapellaDurableReactor(Blockchain& owner,CapellaForkBlockRules rules,CapellaStoreSnapshotLimits limits,const ForkCheckpointToken& expected,const std::function<void()>& check):CapellaDurableReactor(owner.store_,owner.genesis_.chain_id,owner.genesis_.block.header.hash(),std::move(rules),limits,expected,check,&owner){}
CapellaDurableReactor::CapellaDurableReactor(Blockchain& owner,CapellaForkBlockRules rules,CapellaStoreSnapshotLimits limits,ChainStore& witness,const std::function<void()>& check):disk_(owner.store_),execution_(&owner),chain_(owner.genesis_.chain_id),genesis_(owner.genesis_.block.header.hash()),rules_(std::move(rules)),limits_(limits),mutex_(disk_.altair_reactor_mutex_),witness_(&witness){
 std::lock_guard lock(mutex_);validate_execution_binding();archive_require(witness_!=&disk_);witness_->bind_chain(chain_,genesis_,"nano-capella-recovery-v1");
 const auto saved=witness_->fork_checkpoint(chain_,genesis_,witness_limit());archive_require(bool(saved));const auto intent=Json::parse(saved->snapshot);
 archive_require(intent.is_object()&&intent.size()==5&&intent.at("format")=="nano-capella-recovery-v1"&&intent.dump()==saved->snapshot);
 const auto previous=recovery_token_parse(intent.at("previous")),next=recovery_token_parse(intent.at("next"));archive_require(next&&(!previous||previous->sequence<UINT64_MAX)&&next->sequence==(previous?previous->sequence+1:1));
 const auto encoded=intent.at("snapshot").get<std::string>();auto [restored,votes,slashings]=restore_durable(encoded,next->digest,rules_,limits_,check);archive_require(durable_snapshot(restored,votes,slashings,rules_,limits_,check)==encoded);
 const auto& entries=intent.at("archive");archive_require(entries.is_array()&&entries.size()<=1);std::vector<CheckpointArchiveEntry> archive;
 for(const auto& entry:entries){archive_require(entry.is_array()&&entry.size()==3&&entry[0].is_string()&&entry[1].is_string()&&entry[2].is_string());const auto root=entry[0].get<std::string>(),payload=entry[2].get<std::string>();archive_require(root.size()==66&&payload.size()>=2&&payload.size()%2==0&&(payload.size()-2)/2<=limits_.envelope_bytes);auto id=unhex(root),wire=unhex(payload);const auto slot=u64(quantity(entry[1]));archive_require(hex(id)==root&&hex(wire)==payload&&quantity(Int(std::to_string(slot)))==entry[1].get<std::string>());const auto key=archive_array<32>(id,0);verify_archived_block(key,wire,restored,rules_,check);archive_require(restored.blocks.at(key).header.slot==slot);archive.push_back({std::move(id),slot,std::move(wire)});}
 const auto primary=disk_.fork_checkpoint(chain_,genesis_,limits_.envelope_bytes);const auto actual=primary?std::optional<ForkCheckpointToken>(primary->token):std::nullopt;archive_require(actual==next||actual==previous);witness_token_=saved->token;
 if(actual!=next){token_=previous;publish(std::move(restored),check,archive,std::move(votes),std::move(slashings));return;}
 for(const auto& entry:archive){const auto persisted=disk_.checkpoint_archive(entry.root,limits_.envelope_bytes);archive_require(persisted&&persisted->slot==entry.slot&&persisted->payload==entry.payload);}
 const auto& head=capella_fork_head(restored,rules_,check);const auto tip=disk_.latest();const auto state=disk_.state_at(head.execution_block.header.number);archive_require(chain_==rules_.transition.chain_id&&tip&&state&&tip->header.hash()==head.execution_block.header.hash()&&state->root()==head.execution_state.root());
 for(const auto& receipt:head.receipts){if(check)check();const auto persisted=disk_.receipt(receipt.transaction_hash);archive_require(persisted&&persisted->to_json()==receipt.to_json());}
 auto finalized=restored.blocks.at(restored.timed.view.finalized.root).execution_block;const auto canonical=disk_.block_by_number(finalized.header.number);archive_require(canonical&&canonical->header.hash()==finalized.header.hash());
 if(check)check();state_=std::make_shared<const CapellaForkStore>(std::move(restored));token_=*next;votes_=std::move(votes);slashings_=std::move(slashings);
 std::lock_guard chain_lock(owner.mutex_);owner.beacon_finalized_=std::move(finalized);owner.beacon_ready_=true;
}
CapellaPublishedStore CapellaDurableReactor::current()const{std::lock_guard lock(mutex_);require_available();return {state_,*token_};}
CapellaSignedVotes CapellaDurableReactor::signed_votes()const{std::lock_guard lock(mutex_);require_available();return votes_;}
CapellaSignedVotes CapellaDurableReactor::signed_slashings()const{std::lock_guard lock(mutex_);require_available();return slashings_;}
void CapellaDurableReactor::tick(uint64_t time,const std::function<void()>& check){
 std::lock_guard lock(mutex_);require_available();auto next=prepare_capella_store_tick(*state_,time,rules_,limits_.replay_slots,limits_.sync_trials,check);auto votes=votes_;
 const auto previous=(state_->timed.clock.time_ms-state_->timed.clock.genesis_time_ms)/rules_.slot_duration_ms;
 const auto current=(time-state_->timed.clock.genesis_time_ms)/rules_.slot_duration_ms;const auto epoch=current/rules_.transition.consensus.epoch.history.slots_per_epoch;
 for(auto it=votes.begin();it!=votes.end();){
  if(check)check();const auto vote=decode_consensus_attestation_ssz(it->second,rules_.transition.consensus.body.validators_per_committee,check);if(!vote)throw std::logic_error("Capella stored vote encoding");
  if(current>previous&&vote->data.slot==previous){auto applied=prepare_capella_store_vote(next,*vote,rules_,limits_.replay_slots,limits_.sync_trials,check);if(applied)next=std::move(*applied);}
  if(vote->data.target.epoch<(epoch?epoch-1:0))it=votes.erase(it);else ++it;
 }
 publish(std::move(next),check,{},std::move(votes));
}

bool CapellaDurableReactor::block(std::span<const uint8_t> wire,const std::function<void()>& check){
 std::lock_guard lock(mutex_);require_available();auto next=prepare_capella_received_block(*state_,wire,rules_,limits_.replay_slots,limits_.sync_trials,check);if(!next)return false;
 std::vector<CheckpointArchiveEntry> archive;
 if(next->blocks.size()!=state_->blocks.size()){
  if(next->blocks.size()!=state_->blocks.size()+1)throw std::logic_error("Capella admission changed multiple blocks");
  for(const auto& [id,record]:next->blocks)if(!state_->blocks.contains(id))archive.push_back({Bytes(id.begin(),id.end()),record.header.slot,Bytes(wire.begin(),wire.end())});
  if(archive.size()!=1)throw std::logic_error("Capella admission missing new block");
 }
 // Duplicate roots are not reauthenticated by on_block: never backfill or
 // replace archive bytes through that fast path.
 publish(std::move(*next),check,archive);return true;
}
std::optional<Bytes> CapellaDurableReactor::signed_block(const ConsensusRoot& id,const std::function<void()>& check)const{
 std::lock_guard lock(mutex_);return signed_block_locked(id,check);
}
std::optional<Bytes> CapellaDurableReactor::signed_block_locked(const ConsensusRoot& id,const std::function<void()>& check)const{
 require_available();if(check)check();
 if(id==state_->timed.view.anchor||!state_->blocks.contains(id))return std::nullopt;
 const auto record=disk_.checkpoint_archive(Bytes(id.begin(),id.end()),limits_.envelope_bytes);if(!record)return std::nullopt;
 archive_require(record->slot==state_->blocks.at(id).header.slot);verify_archived_block(id,record->payload,*state_,rules_,check);return record->payload;
}

bool CapellaDurableReactor::vote(const Phase0SignedAttestation& vote,const std::function<void()>& check){
 std::lock_guard lock(mutex_);require_available();auto wire=encode_consensus_attestation_ssz(vote,rules_.transition.consensus.body.validators_per_committee,check);auto id=sha256(wire);
 if(const auto found=votes_.find(id);found!=votes_.end()&&found->second==wire)return true;
 auto next=checked_vote(*state_,vote,rules_,limits_,check);if(!next)return false;
 if(votes_.size()>=limits_.messages)throw std::length_error("Capella signed vote capacity");auto votes=votes_;votes.emplace(std::move(id),std::move(wire));publish(std::move(*next),check,{},std::move(votes));return true;
}

bool CapellaDurableReactor::slashing(const IndexedConsensusAttestation& first,const IndexedConsensusAttestation& second,const std::function<void()>& check){
 std::lock_guard lock(mutex_);require_available();auto next=prepare_capella_store_slashing(*state_,first,second,rules_,check);if(!next)return false;
 auto wire=encode_consensus_attester_slashing_ssz(first,second,rules_.transition.consensus.body.validators_per_committee,check);const auto id=sha256(wire);auto proofs=slashings_;
 if(!proofs.contains(id)){
  std::set<uint64_t> uncovered;std::set_intersection(first.indices.begin(),first.indices.end(),second.indices.begin(),second.indices.end(),std::inserter(uncovered,uncovered.end()));
  const auto& finalized=state_->blocks.at(state_->timed.view.finalized.root);const auto& t=rules_.transition;
  const auto context=capella_fork_block_context(finalized.state,finalized.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state,check);
  std::erase_if(uncovered,[&](auto index){if(check)check();const auto member=context.registry.validators.find(index);return member!=context.registry.validators.end()&&member->second.slashed;});
  for(const auto& [_,saved]:proofs){
   if(check)check();if(uncovered.empty())break;const auto decoded=decode_consensus_attester_slashing_ssz(saved,t.consensus.body.validators_per_committee,check);if(!decoded)throw std::logic_error("Capella stored slashing encoding");
   for(auto index:decoded->first.indices)if(std::binary_search(decoded->second.indices.begin(),decoded->second.indices.end(),index))uncovered.erase(index);
  }
  // Different signed pairs for already covered offenders must not grow storage.
  if(!uncovered.empty()){if(proofs.size()>=limits_.equivocations)throw std::length_error("Capella slashing capacity");proofs.emplace(id,std::move(wire));}
 }
 publish(std::move(*next),check,{},std::nullopt,std::move(proofs));return true;
}
}
