#include "nano/consensus_capella_reactor.hpp"
#include "nano/chain.hpp"
#include "nano/consensus_bls.hpp"
#include "nano/consensus_message_ssz.hpp"
#include "consensus_checkpoint_json.hpp"
namespace nano {
namespace {
void archive_require(bool value){if(!value)throw std::invalid_argument("Invalid Capella durable archive");}
uint64_t archive_number(std::span<const uint8_t> bytes,size_t at,size_t width=8){uint64_t value=0;for(size_t i=0;i<width;++i)value|=uint64_t(bytes[at+i])<<(8*i);return value;}
template<size_t N>std::array<uint8_t,N> archive_array(std::span<const uint8_t> bytes,size_t at){std::array<uint8_t,N> out{};std::copy_n(bytes.begin()+at,N,out.begin());return out;}
ConsensusBeaconHeader archived_header(std::span<const uint8_t> wire,const CapellaForkBlockRules& rules,const std::function<void()>& check){
 const auto& r=rules.transition;
 archive_require(wire.size()>=404&&wire.size()<=r.consensus.maximum_block_bytes);
 archive_require(archive_number(wire,0,4)==100&&archive_number(wire,180,4)==84);
 return {archive_number(wire,100),archive_number(wire,108),archive_array<32>(wire,116),archive_array<32>(wire,148),capella_beacon_body_root(std::span(wire).subspan(184),r.consensus.body,r.consensus.state.sync_committee_size,r.execution,r.withdrawals.maximum_per_payload,r.maximum_bls_changes,r.consensus.maximum_block_bytes,check)};
}
void verify_archived_block(const ConsensusRoot& id,const Bytes& wire,const CapellaForkStore& store,const CapellaForkBlockRules& rules,const std::function<void()>& check){
 const auto& r=rules.transition;archive_require(store.blocks.contains(id));const auto header=archived_header(wire,rules,check);
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
bool valid_bls_change(const CapellaForkStore& store,const ConsensusRoot& root,const SignedCapellaBlsChange& change,const CapellaForkBlockRules& rules,const std::function<void()>& check){
 const auto found=store.blocks.find(root);if(found==store.blocks.end())return false;const auto& t=rules.transition;const auto& state=found->second.state;
 const auto digest=capella_beacon_state_root(state.ssz,t.consensus.state,t.execution,t.maximum_state_bytes,check);
 return prepare_capella_bls_changes(state.ssz,digest,t.consensus.state,t.execution,t.maximum_state_bytes,t.genesis_version,t.maximum_bls_changes,std::span(&change,1),check).has_value();
}
std::optional<std::array<uint8_t,48>> exit_key(const CapellaForkStore& store,const ConsensusRoot& root,const SignedConsensusVoluntaryExit& exit,const CapellaForkBlockRules& rules,const std::function<void()>& check){
 const auto found=store.blocks.find(root);if(found==store.blocks.end())return std::nullopt;
 const auto& t=rules.transition;const auto context=capella_fork_block_context(found->second.state,found->second.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state,check);
 const auto member=context.registry.validators.find(exit.validator_index);
 if(member==context.registry.validators.end()||member->second.exit_epoch!=UINT64_MAX||exit.epoch>context.registry.epoch||member->second.activation_epoch>context.registry.epoch||context.registry.epoch-member->second.activation_epoch<t.consensus.exit.shard_committee_period)return std::nullopt;
 const auto version=exit.epoch<context.fork.epoch?context.fork.previous_version:context.fork.current_version;
 if(!verify_consensus_bls(member->second.public_key,consensus_signing_root(consensus_voluntary_exit_root(exit.epoch,exit.validator_index),consensus_domain({4,0,0,0},version,context.genesis_root)),exit.signature))return std::nullopt;
 return member->second.public_key;
}
std::string durable_snapshot(const CapellaForkStore& store,const CapellaSignedVotes& votes,const CapellaSignedVotes& slashings,const CapellaPendingBlsChanges& changes,const CapellaPendingExits& exits,const CapellaForkBlockRules& rules,const CapellaStoreSnapshotLimits& limits,const std::function<void()>& check,CapellaStoreSnapshotCache* cache=nullptr){
 const auto encoded=capella_store_snapshot(store,rules,limits,check,cache);if(votes.empty()&&slashings.empty()&&changes.empty()&&exits.empty())return encoded;
 if(votes.size()>limits.messages)throw std::length_error("Capella signed vote capacity");Json list=Json::array();size_t total=0;
 for(const auto& [id,wire]:votes){if(check)check();if(wire.size()>limits.envelope_bytes/2-total)throw std::length_error("Capella signed vote bytes");total+=wire.size();list.push_back(Json::array({hex(id),hex(wire)}));}
 Json document{{"format","nano-capella-durable-v1"},{"signedVotes",std::move(list)}};
 if(!slashings.empty()||!changes.empty()||!exits.empty()){
  if(slashings.size()>limits.equivocations)throw std::length_error("Capella slashing capacity");Json proofs=Json::array();
  for(const auto& [id,wire]:slashings){if(check)check();if(wire.size()>limits.envelope_bytes/2-total)throw std::length_error("Capella slashing bytes");total+=wire.size();proofs.push_back(Json::array({hex(id),hex(wire)}));}
  document["format"]="nano-capella-durable-v2";document["signedSlashings"]=std::move(proofs);
 }
 if(!changes.empty()||!exits.empty()){
  if(changes.size()>limits.messages)throw std::length_error("Capella BLS change capacity");Json operations=Json::array();
  for(const auto& [index,record]:changes){if(check)check();if(record.wire.size()!=172||204>limits.envelope_bytes/2-total)throw std::length_error("Capella BLS change bytes");total+=204;operations.push_back(Json::array({quantity(Int(std::to_string(index))),hex(Bytes(record.admission_root.begin(),record.admission_root.end())),hex(record.wire)}));}
  document["format"]="nano-capella-durable-v3";document["blsChanges"]=std::move(operations);
 }
 if(!exits.empty()){
  if(exits.size()>limits.messages)throw std::length_error("Capella exit capacity");Json operations=Json::array();
  for(const auto& [id,record]:exits){if(check)check();if(id.size()!=32||record.wire.size()!=112||144>limits.envelope_bytes/2-total)throw std::length_error("Capella exit bytes");total+=144;operations.push_back(Json::array({hex(id),hex(Bytes(record.admission_root.begin(),record.admission_root.end())),hex(record.wire)}));}
  document["format"]="nano-capella-durable-v4";document["signedExits"]=std::move(operations);
 }
 // encoded is the validated canonical store above, never a peer-provided value.
 // Preserve the existing Capella envelope exception type.
 try{return checkpoint_detail::durable_json(std::move(document),encoded,limits.envelope_bytes);}
 catch(const std::invalid_argument&){throw std::length_error("Capella durable envelope");}
}
std::tuple<CapellaForkStore,CapellaSignedVotes,CapellaSignedVotes,CapellaPendingBlsChanges,CapellaPendingExits> restore_durable(std::string_view encoded,const Bytes& digest,const CapellaForkBlockRules& rules,const CapellaStoreSnapshotLimits& limits,const std::function<void()>& check){
 if(check)check();archive_require(encoded.size()<=limits.envelope_bytes&&digest.size()==32&&sha256(Bytes(encoded.begin(),encoded.end()))==digest);
 const auto document=Json::parse(encoded);
 const bool with_exits=document.value("format",std::string())=="nano-capella-durable-v4";
 const bool with_changes=with_exits||document.value("format",std::string())=="nano-capella-durable-v3";
 const bool with_proofs=with_changes||document.value("format",std::string())=="nano-capella-durable-v2";
 if(!with_proofs&&document.value("format",std::string())!="nano-capella-durable-v1")return {capella_store_from_snapshot(encoded,digest,rules,limits,check),{}, {}, {}, {}};
 archive_require(document.size()==(with_exits?6:with_changes?5:with_proofs?4:3)&&document.at("signedVotes").is_array()&&(with_proofs||!document.at("signedVotes").empty())&&document.at("signedVotes").size()<=limits.messages);
 const auto inner=document.at("store").dump();auto store=capella_store_from_snapshot(inner,sha256(Bytes(inner.begin(),inner.end())),rules,limits,check);CapellaSignedVotes votes,proofs;
 for(const auto& entry:document.at("signedVotes")){
  if(check)check();archive_require(entry.is_array()&&entry.size()==2&&entry[0].is_string()&&entry[1].is_string());
  const auto key=entry[0].get<std::string>(),payload=entry[1].get<std::string>();archive_require(key.size()==66&&payload.size()>=2&&payload.size()%2==0&&(payload.size()-2)/2<=228+uint64_t(rules.transition.consensus.body.validators_per_committee)/8+1);
  auto id=unhex(key),wire=unhex(payload);archive_require(hex(id)==key&&hex(wire)==payload&&sha256(wire)==id);
  const auto vote=decode_consensus_attestation_ssz(wire,rules.transition.consensus.body.validators_per_committee,check);archive_require(vote&&checked_vote(store,*vote,rules,limits,check).has_value());archive_require(votes.emplace(std::move(id),std::move(wire)).second);
 }
 if(with_proofs){
  const auto& list=document.at("signedSlashings");archive_require(list.is_array()&&(with_changes||!list.empty())&&list.size()<=limits.equivocations);
  for(const auto& entry:list){
   if(check)check();archive_require(entry.is_array()&&entry.size()==2&&entry[0].is_string()&&entry[1].is_string());const auto key=entry[0].get<std::string>(),payload=entry[1].get<std::string>();
   archive_require(key.size()==66&&payload.size()>=2&&payload.size()%2==0&&(payload.size()-2)/2<=464+uint64_t(rules.transition.consensus.body.validators_per_committee)*16);
   auto id=unhex(key),wire=unhex(payload);archive_require(hex(id)==key&&hex(wire)==payload&&sha256(wire)==id);
   const auto decoded=decode_consensus_attester_slashing_ssz(wire,rules.transition.consensus.body.validators_per_committee,check);archive_require(bool(decoded));
   const auto applied=prepare_capella_store_slashing(store,decoded->first,decoded->second,rules,check);archive_require(applied&&applied->timed.view.equivocating_indices==store.timed.view.equivocating_indices);
   archive_require(proofs.emplace(std::move(id),std::move(wire)).second);
  }
 }
 CapellaPendingBlsChanges changes;
 if(with_changes){
  const auto& list=document.at("blsChanges");archive_require(list.is_array()&&(with_exits||!list.empty())&&list.size()<=limits.messages);
  for(const auto& item:list){if(check)check();archive_require(item.is_array()&&item.size()==3&&item[0].is_string()&&item[1].is_string()&&item[2].is_string());
   const auto index=u64(quantity(item[0]));const auto root_text=item[1].get<std::string>(),wire_text=item[2].get<std::string>();archive_require(root_text.size()==66&&wire_text.size()==346);
   const auto root_bytes=unhex(root_text),wire=unhex(wire_text);archive_require(root_bytes.size()==32&&hex(root_bytes)==root_text&&hex(wire)==wire_text&&quantity(Int(std::to_string(index)))==item[0].get<std::string>());const auto root=archive_array<32>(root_bytes,0);const auto operation=decode_capella_bls_change(wire);
   archive_require(operation&&operation->validator_index==index&&valid_bls_change(store,root,*operation,rules,check));archive_require(changes.emplace(index,CapellaPendingBlsChange{root,wire}).second);
  }
 }
 CapellaPendingExits exits;std::set<std::pair<uint64_t,std::array<uint8_t,48>>> identities;
 if(with_exits){const auto& list=document.at("signedExits");archive_require(list.is_array()&&!list.empty()&&list.size()<=limits.messages);
  for(const auto& item:list){if(check)check();archive_require(item.is_array()&&item.size()==3&&item[0].is_string()&&item[1].is_string()&&item[2].is_string());
   const auto id_text=item[0].get<std::string>(),root_text=item[1].get<std::string>(),wire_text=item[2].get<std::string>();archive_require(id_text.size()==66&&root_text.size()==66&&wire_text.size()==226);
   const auto id=unhex(id_text),root_bytes=unhex(root_text),wire=unhex(wire_text);archive_require(hex(id)==id_text&&hex(root_bytes)==root_text&&hex(wire)==wire_text&&sha256(wire)==id);
   const auto root=archive_array<32>(root_bytes,0);const auto operation=decode_consensus_voluntary_exit_ssz(wire,check);archive_require(bool(operation));const auto key=exit_key(store,root,*operation,rules,check);archive_require(bool(key));
   archive_require(identities.emplace(operation->validator_index,*key).second&&exits.emplace(id,CapellaPendingExit{root,*key,wire}).second);
  }
 }
 archive_require(durable_snapshot(store,votes,proofs,changes,exits,rules,limits,check)==encoded);return {std::move(store),std::move(votes),std::move(proofs),std::move(changes),std::move(exits)};
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
void CapellaDurableReactor::fail_closed(){if(!failure_)failure_=std::current_exception();unavailable_=true;std::atomic_store_explicit(&beacon_publication_,std::shared_ptr<const CapellaPublishedStore>{},std::memory_order_release);if(execution_){std::lock_guard lock(execution_->mutex_);execution_->beacon_ready_=false;}}
CapellaDurableReactor::~CapellaDurableReactor(){std::atomic_store_explicit(&beacon_publication_,std::shared_ptr<const CapellaPublishedStore>{},std::memory_order_release);if(execution_){std::lock_guard lock(execution_->mutex_);execution_->beacon_ready_=false;}}
void CapellaDurableReactor::invalidate(){std::lock_guard lock(mutex_);fail_closed();}
void CapellaDurableReactor::require_available()const{
 if(unavailable_||!state_||!token_){
  std::string message="Capella reactor unavailable; reopen required";
  if(failure_)try{std::rethrow_exception(failure_);}catch(const std::exception& error){message+="; initial failure: ";message+=error.what();}catch(...){message+="; initial failure: unknown exception";}
  throw std::runtime_error(message);
 }
}
void CapellaDurableReactor::publish(CapellaForkStore candidate,const std::function<void()>& check,const std::vector<CheckpointArchiveEntry>& archive,std::optional<CapellaSignedVotes> votes,std::optional<CapellaSignedVotes> slashings,std::optional<CapellaPendingBlsChanges> changes,std::optional<CapellaPendingExits> exits){
 if(unavailable_)require_available();
 if(chain_!=rules_.transition.chain_id||genesis_.size()!=32)throw std::invalid_argument("Capella reactor chain/profile mismatch");
 // Only finalized sanctions retire evidence. An unfinalized inclusion can reorg.
 if(!slashings)slashings=slashings_;
 if(!slashings->empty()&&state_&&state_->timed.view.finalized.root!=candidate.timed.view.finalized.root){
  const auto& finalized=candidate.blocks.at(candidate.timed.view.finalized.root);const auto& t=rules_.transition;
  const auto context=capella_fork_block_context(finalized.state,finalized.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state,check);
  for(auto it=slashings->begin();it!=slashings->end();){if(check)check();const auto proof=decode_consensus_attester_slashing_ssz(it->second,t.consensus.body.validators_per_committee,check);if(!proof)throw std::logic_error("invalid retained Capella slashing");bool needed=false;
   for(auto index:proof->first.indices){if(check)check();if(!std::binary_search(proof->second.indices.begin(),proof->second.indices.end(),index))continue;const auto member=context.registry.validators.find(index);if(member==context.registry.validators.end()||!member->second.slashed){needed=true;break;}}
   if(needed)++it;else it=slashings->erase(it);
  }
 }
 if(!changes)changes=bls_changes_;
 if(!changes->empty()){
  const auto& finalized=candidate.blocks.at(candidate.timed.view.finalized.root);const auto& t=rules_.transition;
  const auto context=capella_fork_block_context(finalized.state,finalized.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state,check);
  for(auto it=changes->begin();it!=changes->end();){if(check)check();const auto member=context.registry.validators.find(it->first);
   if(member!=context.registry.validators.end()&&member->second.withdrawal_credentials[0]==1){it=changes->erase(it);continue;}
   if(!candidate.blocks.contains(it->second.admission_root)){const auto operation=decode_capella_bls_change(it->second.wire);if(!operation||!valid_bls_change(candidate,candidate.timed.view.finalized.root,*operation,rules_,check)){it=changes->erase(it);continue;}it->second.admission_root=candidate.timed.view.finalized.root;}
   ++it;
  }
 }
 if(!exits)exits=exits_;
 if(!exits->empty()){
  const auto root=candidate.timed.view.finalized.root;const auto& finalized=candidate.blocks.at(root);const auto& t=rules_.transition;
  const auto context=capella_fork_block_context(finalized.state,finalized.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state,check);
  for(auto it=exits->begin();it!=exits->end();){if(check)check();const auto operation=decode_consensus_voluntary_exit_ssz(it->second.wire,check);if(!operation)throw std::logic_error("Invalid retained Capella exit");
   const auto member=context.registry.validators.find(operation->validator_index);
   // Only finalized membership or scheduling retires an operation. A head reorg does not.
   if(member!=context.registry.validators.end()&&(member->second.public_key!=it->second.public_key||member->second.exit_epoch!=UINT64_MAX)){it=exits->erase(it);continue;}
   if(!candidate.blocks.contains(it->second.admission_root)){const auto key=exit_key(candidate,root,*operation,rules_,check);if(!key||*key!=it->second.public_key){it=exits->erase(it);continue;}it->second.admission_root=root;}
   ++it;
  }
 }
 auto next=std::make_shared<const CapellaForkStore>(std::move(candidate));
 const auto encoded=durable_snapshot(*next,votes?*votes:votes_,slashings?*slashings:slashings_,*changes,*exits,rules_,limits_,check,&cache_);
 const auto& head=capella_fork_head(*next,rules_,check);
 const auto tip=disk_.latest();if(!tip)throw std::invalid_argument("Capella reactor requires persisted execution anchor");
 if(state_){
  if(tip->header.hash()!=capella_fork_head(*state_,rules_,check).execution_block.header.hash()){fail_closed();throw std::runtime_error("Capella execution tip changed outside reactor");}
  const auto& old=state_->timed.view;const auto& view=next->timed.view;
  if(capella_network_anchor(*next)!=capella_network_anchor(*state_)||view.finalized.epoch<old.finalized.epoch||view.justified.epoch<old.justified.epoch||next->timed.clock.genesis_time_ms!=state_->timed.clock.genesis_time_ms||next->timed.clock.time_ms<state_->timed.clock.time_ms)throw std::invalid_argument("Capella checkpoint regression");
  auto at=consensus_beacon_header_root(head.header);const auto final_slot=state_->blocks.at(old.finalized.root).header.slot;
  while(next->blocks.at(at).header.slot>final_slot){if(check)check();at=next->blocks.at(at).header.parent_root;}
  if(at!=old.finalized.root||(view.finalized.epoch==old.finalized.epoch&&view.finalized.root!=old.finalized.root))throw std::invalid_argument("Capella finalized branch changed");
 }
 // An anchor change must be the qualified transformation of the current store,
 // and the exact durable envelope must remain fully restart-verifiable before I/O.
 if(state_&&next->timed.view.anchor!=state_->timed.view.anchor){
  const auto expected=prepare_capella_pruned_store(*state_,next->timed.view.anchor,rules_,check);
  if(capella_store_snapshot(expected,rules_,limits_,check)!=capella_store_snapshot(*next,rules_,limits_,check))throw std::invalid_argument("Capella pruning changed live state");
  (void)restore_durable(encoded,sha256(Bytes(encoded.begin(),encoded.end())),rules_,limits_,check);
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
 // Allocate the coherent read view before I/O; no allocation after commit.
 if(token_&&token_->sequence==UINT64_MAX)throw std::overflow_error("Capella publication sequence exhausted");
 const auto publication=std::make_shared<const CapellaPublishedStore>(CapellaPublishedStore{next,{token_?token_->sequence+1:1,digest}});
 std::optional<ExecutionStateRetentionAnchor> retention;
 if(historical_state_pruning_){const auto& anchor=next->blocks.at(next->timed.view.anchor).execution_block.header;if(anchor.number<=common_height)retention=ExecutionStateRetentionAnchor{anchor.number,anchor.hash()};}
 ForkCheckpointToken committed;
 try{
  witness_prepare(encoded,archive);
  if(execution_)committed=execution_->commit_beacon_checkpoint(token_,tip->header.hash(),common_height,common_hash,suffix,next->blocks.at(next->timed.view.finalized.root).execution_block,encoded,limits_.envelope_bytes,limits_.blocks,archive,retention);
  else committed=disk_.commit_execution_checkpoint(chain_,genesis_,token_,tip->header.hash(),common_height,common_hash,suffix,encoded,limits_.envelope_bytes,limits_.blocks,archive,retention);
  if(committed!=publication->token)throw std::runtime_error("Capella durable publication token mismatch");
 }
 catch(...){fail_closed();throw;}
 // No allocation or callback after durable success and before publication.
 token_=std::move(committed);state_=std::move(next);if(votes)votes_=std::move(*votes);if(slashings)slashings_=std::move(*slashings);bls_changes_=std::move(*changes);exits_=std::move(*exits);
 std::atomic_store_explicit(&beacon_publication_,publication,std::memory_order_release);
}
CapellaDurableReactor::CapellaDurableReactor(ChainStore& disk,Int chain,Bytes genesis,CapellaForkBlockRules rules,CapellaStoreSnapshotLimits limits,CapellaForkStore initial,const std::function<void()>& check,Blockchain* owner,ChainStore* witness):disk_(disk),execution_(owner),chain_(std::move(chain)),genesis_(std::move(genesis)),rules_(std::move(rules)),limits_(limits),mutex_(disk.altair_reactor_mutex_){std::lock_guard lock(mutex_);validate_execution_binding();if(witness){archive_require(witness!=&disk_);witness_=witness;witness_->bind_chain(chain_,genesis_,"nano-capella-recovery-v1");archive_require(!witness_->fork_checkpoint(chain_,genesis_,witness_limit())&&!disk_.fork_checkpoint(chain_,genesis_,limits_.envelope_bytes));}publish(std::move(initial),check);}
CapellaDurableReactor::CapellaDurableReactor(ChainStore& disk,Int chain,Bytes genesis,CapellaForkBlockRules rules,CapellaStoreSnapshotLimits limits,const ForkCheckpointToken& expected,const std::function<void()>& check,Blockchain* owner):disk_(disk),execution_(owner),chain_(std::move(chain)),genesis_(std::move(genesis)),rules_(std::move(rules)),limits_(limits),mutex_(disk.altair_reactor_mutex_){
 std::lock_guard lock(mutex_);validate_execution_binding();
 if(chain_!=rules_.transition.chain_id||genesis_.size()!=32)throw std::invalid_argument("Capella reactor chain/profile mismatch");
 const auto record=disk_.fork_checkpoint(chain_,genesis_,limits_.envelope_bytes,expected);if(!record)throw std::invalid_argument("Capella restart checkpoint missing");
 auto [restored,votes,slashings,changes,exits]=restore_durable(record->snapshot,expected.digest,rules_,limits_,check);const auto pruning=disk_.state_pruning();if(pruning&&pruning->anchor.number>restored.blocks.at(restored.timed.view.anchor).execution_block.header.number)throw std::invalid_argument("pruning boundary exceeds authenticated Capella anchor");const auto& head=capella_fork_head(restored,rules_,check);const auto tip=disk_.latest();const auto state=disk_.state_at(head.execution_block.header.number);
 if(!tip||!state||tip->header.hash()!=head.execution_block.header.hash()||state->root()!=head.execution_state.root())throw std::invalid_argument("Capella restart execution mismatch");
 for(const auto& receipt:head.receipts){if(check)check();const auto saved=disk_.receipt(receipt.transaction_hash);archive_require(saved&&saved->to_json()==receipt.to_json());}
 auto finalized=restored.blocks.at(restored.timed.view.finalized.root).execution_block;const auto canonical=disk_.block_by_number(finalized.header.number);archive_require(canonical&&canonical->header.hash()==finalized.header.hash());
 if(check)check();state_=std::make_shared<const CapellaForkStore>(std::move(restored));token_=expected;votes_=std::move(votes);slashings_=std::move(slashings);bls_changes_=std::move(changes);exits_=std::move(exits);
 std::atomic_store_explicit(&beacon_publication_,std::make_shared<const CapellaPublishedStore>(CapellaPublishedStore{state_,*token_}),std::memory_order_release);
 if(execution_){std::lock_guard chain_lock(execution_->mutex_);execution_->beacon_finalized_=std::move(finalized);execution_->beacon_ready_=true;}
}
CapellaDurableReactor::CapellaDurableReactor(Blockchain& owner,CapellaForkBlockRules rules,CapellaStoreSnapshotLimits limits,CapellaForkStore initial,const std::function<void()>& check,ChainStore* witness):CapellaDurableReactor(owner.store_,owner.genesis_.chain_id,owner.genesis_.block.header.hash(),std::move(rules),limits,std::move(initial),check,&owner,witness){}
CapellaDurableReactor::CapellaDurableReactor(Blockchain& owner,CapellaForkBlockRules rules,CapellaStoreSnapshotLimits limits,const ForkCheckpointToken& expected,const std::function<void()>& check):CapellaDurableReactor(owner.store_,owner.genesis_.chain_id,owner.genesis_.block.header.hash(),std::move(rules),limits,expected,check,&owner){}
CapellaDurableReactor::CapellaDurableReactor(Blockchain& owner,CapellaForkBlockRules rules,CapellaStoreSnapshotLimits limits,ChainStore& witness,const std::function<void()>& check,std::optional<ForkCheckpointToken> recovery_commitment):disk_(owner.store_),execution_(&owner),chain_(owner.genesis_.chain_id),genesis_(owner.genesis_.block.header.hash()),rules_(std::move(rules)),limits_(limits),mutex_(disk_.altair_reactor_mutex_),witness_(&witness){
 std::lock_guard lock(mutex_);validate_execution_binding();archive_require(witness_!=&disk_);witness_->bind_chain(chain_,genesis_,"nano-capella-recovery-v1");
 const auto saved=witness_->fork_checkpoint(chain_,genesis_,witness_limit());archive_require(bool(saved));const auto intent=Json::parse(saved->snapshot);
 archive_require(intent.is_object()&&intent.size()==5&&intent.at("format")=="nano-capella-recovery-v1"&&intent.dump()==saved->snapshot);
 const auto previous=recovery_token_parse(intent.at("previous")),next=recovery_token_parse(intent.at("next"));archive_require(next&&(!previous||previous->sequence<UINT64_MAX)&&next->sequence==(previous?previous->sequence+1:1));
 if(recovery_commitment&&next!=recovery_commitment)throw std::invalid_argument("Capella recovery commitment differs from witness intent");
 const auto encoded=intent.at("snapshot").get<std::string>();auto [restored,votes,slashings,changes,exits]=restore_durable(encoded,next->digest,rules_,limits_,check);archive_require(durable_snapshot(restored,votes,slashings,changes,exits,rules_,limits_,check)==encoded);
 const auto& entries=intent.at("archive");archive_require(entries.is_array()&&entries.size()<=1);std::vector<CheckpointArchiveEntry> archive;
 for(const auto& entry:entries){archive_require(entry.is_array()&&entry.size()==3&&entry[0].is_string()&&entry[1].is_string()&&entry[2].is_string());const auto root=entry[0].get<std::string>(),payload=entry[2].get<std::string>();archive_require(root.size()==66&&payload.size()>=2&&payload.size()%2==0&&(payload.size()-2)/2<=limits_.envelope_bytes);auto id=unhex(root),wire=unhex(payload);const auto slot=u64(quantity(entry[1]));archive_require(hex(id)==root&&hex(wire)==payload&&quantity(Int(std::to_string(slot)))==entry[1].get<std::string>());const auto key=archive_array<32>(id,0);verify_archived_block(key,wire,restored,rules_,check);archive_require(restored.blocks.at(key).header.slot==slot);archive.push_back({std::move(id),slot,std::move(wire)});}
 const auto primary=disk_.fork_checkpoint(chain_,genesis_,limits_.envelope_bytes);const auto actual=primary?std::optional<ForkCheckpointToken>(primary->token):std::nullopt;archive_require(actual==next||actual==previous);witness_token_=saved->token;
 if(actual!=next){token_=previous;publish(std::move(restored),check,archive,std::move(votes),std::move(slashings),std::move(changes),std::move(exits));return;}
 for(const auto& entry:archive){const auto persisted=disk_.checkpoint_archive(entry.root,limits_.envelope_bytes);archive_require(persisted&&persisted->slot==entry.slot&&persisted->payload==entry.payload);}
 const auto pruning=disk_.state_pruning();if(pruning&&pruning->anchor.number>restored.blocks.at(restored.timed.view.anchor).execution_block.header.number)throw std::invalid_argument("pruning boundary exceeds authenticated Capella anchor");const auto& head=capella_fork_head(restored,rules_,check);const auto tip=disk_.latest();const auto state=disk_.state_at(head.execution_block.header.number);archive_require(chain_==rules_.transition.chain_id&&tip&&state&&tip->header.hash()==head.execution_block.header.hash()&&state->root()==head.execution_state.root());
 for(const auto& receipt:head.receipts){if(check)check();const auto persisted=disk_.receipt(receipt.transaction_hash);archive_require(persisted&&persisted->to_json()==receipt.to_json());}
 auto finalized=restored.blocks.at(restored.timed.view.finalized.root).execution_block;const auto canonical=disk_.block_by_number(finalized.header.number);archive_require(canonical&&canonical->header.hash()==finalized.header.hash());
 if(check)check();state_=std::make_shared<const CapellaForkStore>(std::move(restored));token_=*next;votes_=std::move(votes);slashings_=std::move(slashings);bls_changes_=std::move(changes);exits_=std::move(exits);
 std::atomic_store_explicit(&beacon_publication_,std::make_shared<const CapellaPublishedStore>(CapellaPublishedStore{state_,*token_}),std::memory_order_release);
 std::lock_guard chain_lock(owner.mutex_);owner.beacon_finalized_=std::move(finalized);owner.beacon_ready_=true;
}
Json CapellaDurableReactor::network_identity()const{
 std::lock_guard lock(mutex_);return network_identity_locked();
}
Json CapellaDurableReactor::network_identity_locked()const{
 require_available();const auto anchor=capella_network_anchor(*state_);const auto encoded=Json::array({"nano-capella-network-v1",quantity(chain_),hex(genesis_),hex(Bytes(anchor.begin(),anchor.end())),capella_fork_profile(rules_)}).dump();const auto digest=sha256(Bytes(encoded.begin(),encoded.end()));
 return {{"chainId",quantity(chain_)},{"genesisHash",hex(genesis_)},{"consensusIdentity",hex(digest)},{"forkDigest",hex(Bytes(digest.begin(),digest.begin()+4))}};
}
std::vector<SignedConsensusVoluntaryExit> CapellaDurableReactor::pending_exits_locked(size_t maximum,const std::function<void()>& check)const{
 require_available();std::vector<SignedConsensusVoluntaryExit> result;if(exits_.empty())return result;result.reserve(std::min(maximum,exits_.size()));
 const auto& head=capella_fork_head(*state_,rules_,check);const auto& t=rules_.transition;const auto context=capella_fork_block_context(head.state,head.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state,check);
 for(const auto& [id,record]:exits_){if(check)check();if(result.size()>=maximum)break;const auto operation=decode_consensus_voluntary_exit_ssz(record.wire,check);if(!operation)throw std::logic_error("Invalid queued Capella exit");const auto member=context.registry.validators.find(operation->validator_index);if(member==context.registry.validators.end()||member->second.public_key!=record.public_key||member->second.exit_epoch!=UINT64_MAX)continue;result.push_back(*operation);}return result;
}
std::vector<SignedConsensusVoluntaryExit> CapellaDurableReactor::pending_exits()const{std::lock_guard lock(mutex_);return pending_exits_locked(limits_.messages,{});}
bool CapellaDurableReactor::voluntary_exit_ssz(std::span<const uint8_t> wire,const std::function<void()>& check){
 const auto operation=decode_consensus_voluntary_exit_ssz(wire,check);if(!operation)return false;std::lock_guard lock(mutex_);require_available();if(check)check();
 const Bytes bytes(wire.begin(),wire.end());const auto id=sha256(bytes);if(exits_.contains(id))return exits_.at(id).wire==bytes;
 const auto& head=capella_fork_head(*state_,rules_,check);const auto root=consensus_beacon_header_root(head.header);const auto key=exit_key(*state_,root,*operation,rules_,check);if(!key)return false;
 const auto& t=rules_.transition;auto context=capella_fork_block_context(head.state,head.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state,check);
 const auto version=operation->epoch<context.fork.epoch?context.fork.previous_version:context.fork.current_version;
 if(!process_phase0_voluntary_exit(context.registry,t.consensus.exit,*operation,version,context.genesis_root))return false;
 for(const auto& [_,record]:exits_){if(check)check();const auto other=decode_consensus_voluntary_exit_ssz(record.wire,check);if(!other)throw std::logic_error("Invalid queued Capella exit");if(other->validator_index==operation->validator_index&&record.public_key==*key)return false;}
 if(exits_.size()>=limits_.messages)throw std::length_error("Capella exit capacity");auto exits=exits_;exits.emplace(id,CapellaPendingExit{root,*key,bytes});publish(*state_,check,{},std::nullopt,std::nullopt,std::nullopt,std::move(exits));return true;
}
std::vector<SignedCapellaBlsChange> CapellaDurableReactor::pending_bls_changes_locked(size_t maximum,const std::function<void()>& check)const{
 require_available();std::vector<SignedCapellaBlsChange> result;result.reserve(std::min(maximum,bls_changes_.size()));if(bls_changes_.empty())return result;
 const auto& head=capella_fork_head(*state_,rules_,check);const auto& t=rules_.transition;const auto context=capella_fork_block_context(head.state,head.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state,check);
 for(const auto& [index,record]:bls_changes_){if(result.size()>=maximum)break;if(check)check();const auto operation=decode_capella_bls_change(record.wire);if(!operation)throw std::logic_error("Invalid queued BLS change");const auto member=context.registry.validators.find(index);if(member==context.registry.validators.end()||member->second.withdrawal_credentials[0]!=0)continue;const auto key_hash=sha256(Bytes(operation->from_bls_pubkey.begin(),operation->from_bls_pubkey.end()));if(!std::equal(key_hash.begin()+1,key_hash.end(),member->second.withdrawal_credentials.begin()+1))continue;result.push_back(*operation);}return result;
}
std::vector<SignedCapellaBlsChange> CapellaDurableReactor::pending_bls_changes()const{std::lock_guard lock(mutex_);return pending_bls_changes_locked(limits_.messages,{});}
bool CapellaDurableReactor::bls_change_ssz(std::span<const uint8_t> wire,const std::function<void()>& check){
 const auto operation=decode_capella_bls_change(wire);if(!operation)return false;std::lock_guard lock(mutex_);require_available();if(check)check();
 if(const auto it=bls_changes_.find(operation->validator_index);it!=bls_changes_.end())return it->second.wire==Bytes(wire.begin(),wire.end());
 const auto root=consensus_beacon_header_root(capella_fork_head(*state_,rules_,check).header);if(!valid_bls_change(*state_,root,*operation,rules_,check))return false;
 if(bls_changes_.size()>=limits_.messages)throw std::length_error("Capella BLS change queue capacity");auto changes=bls_changes_;changes.emplace(operation->validator_index,CapellaPendingBlsChange{root,Bytes(wire.begin(),wire.end())});publish(*state_,check,{},std::nullopt,std::nullopt,std::move(changes));return true;
}
bool CapellaDurableReactor::vote_ssz(std::span<const uint8_t> wire,const std::function<void()>& check){const auto decoded=decode_consensus_attestation_ssz(wire,rules_.transition.consensus.body.validators_per_committee,check);return decoded&&vote(*decoded,check);}
bool CapellaDurableReactor::slashing_ssz(std::span<const uint8_t> wire,const std::function<void()>& check){const auto decoded=decode_consensus_attester_slashing_ssz(wire,rules_.transition.consensus.body.validators_per_committee,check);return decoded&&slashing(decoded->first,decoded->second,check);}
Json CapellaDurableReactor::relay_head(const std::function<void()>& check)const{
 if(check)check();const auto published=beacon_snapshot();
 const auto& head=capella_fork_head(*published.state,rules_,check);
 const auto root=consensus_beacon_header_root(head.header);if(check)check();
 return {{"headRoot",hex(Bytes(root.begin(),root.end()))},{"headSlot",head.header.slot}};
}
CapellaPublishedStore CapellaDurableReactor::beacon_snapshot()const{
 if(const auto published=std::atomic_load_explicit(&beacon_publication_,std::memory_order_acquire))return *published;
 // Preserve the original latched failure diagnostic when unavailable. This
 // path cannot return an unauthenticated or partially committed publication.
 return current();
}
std::optional<Bytes> CapellaDurableReactor::beacon_signed_block(const CapellaPublishedStore& published,const ConsensusRoot& id,const std::function<void()>& check)const{
 if(check)check();const auto found=published.state->blocks.find(id);if(found==published.state->blocks.end())return std::nullopt;
 // ChainStore serializes its own DB access. Authentication uses the captured
 // immutable store, so expensive validation does not hold the admission lock.
 const auto record=disk_.checkpoint_archive(Bytes(id.begin(),id.end()),limits_.envelope_bytes);if(!record)return std::nullopt;
 archive_require(record->slot==found->second.header.slot);verify_archived_block(id,record->payload,*published.state,rules_,check);return record->payload;
}
CapellaPublishedStore CapellaDurableReactor::current()const{std::lock_guard lock(mutex_);require_available();return {state_,*token_};}
CapellaSignedVotes CapellaDurableReactor::signed_votes()const{std::lock_guard lock(mutex_);require_available();return votes_;}
CapellaSignedVotes CapellaDurableReactor::signed_slashings()const{std::lock_guard lock(mutex_);require_available();return slashings_;}
void CapellaDurableReactor::tick(uint64_t time,const std::function<void()>& check){
 std::lock_guard lock(mutex_);require_available();tick_locked(time,check);
}
bool CapellaDurableReactor::tick_if_needed(uint64_t time,const std::function<void()>& check){
 std::lock_guard lock(mutex_);require_available();if(check)check();const auto& c=state_->timed.clock;
 if(time<c.time_ms||c.time_ms<c.genesis_time_ms)throw std::invalid_argument("Capella clock moved backwards");
 const auto previous=c.time_ms-c.genesis_time_ms,current=time-c.genesis_time_ms;
 if(previous/rules_.slot_duration_ms==current/rules_.slot_duration_ms&&(previous%rules_.slot_duration_ms<rules_.attestation_due_ms)==(current%rules_.slot_duration_ms<rules_.attestation_due_ms))return false;
 tick_locked(time,check);return true;
}
void CapellaDurableReactor::tick_locked(uint64_t time,const std::function<void()>& check){
 auto next=prepare_capella_store_tick(*state_,time,rules_,limits_.replay_slots,limits_.sync_trials,check);auto votes=votes_;
 const auto previous=(state_->timed.clock.time_ms-state_->timed.clock.genesis_time_ms)/rules_.slot_duration_ms;
 const auto current=(time-state_->timed.clock.genesis_time_ms)/rules_.slot_duration_ms;const auto epoch=current/rules_.transition.consensus.epoch.history.slots_per_epoch;
 for(auto it=votes.begin();it!=votes.end();){
  if(check)check();const auto vote=decode_consensus_attestation_ssz(it->second,rules_.transition.consensus.body.validators_per_committee,check);if(!vote)throw std::logic_error("Capella stored vote encoding");
  if(current>previous&&vote->data.slot==previous){auto applied=prepare_capella_store_vote(next,*vote,rules_,limits_.replay_slots,limits_.sync_trials,check);if(applied)next=std::move(*applied);}
  if(vote->data.target.epoch<(epoch?epoch-1:0))it=votes.erase(it);else ++it;
 }
 publish(std::move(next),check,{},std::move(votes));
 if(automatic_pruning_)prune_if_ready_locked(check);
}

bool CapellaDurableReactor::block(std::span<const uint8_t> wire,const std::function<void()>& check){
 std::lock_guard lock(mutex_);require_available();if(check)check();
 // Reconnection may lose the sender's ACK cache. A pruned historical duplicate
 // is acknowledged only from our OWN authenticated finalized archive; it never
 // installs a peer checkpoint or changes the current execution/fork state.
 if(wire.size()>=404&&wire.size()<=rules_.transition.consensus.maximum_block_bytes&&archive_number(wire,100)<state_->blocks.at(state_->timed.view.anchor).header.slot){
  std::exception_ptr interrupted;auto guard=[&]{try{if(check)check();}catch(...){interrupted=std::current_exception();throw;}};
  ConsensusBeaconHeader header;
  try{header=archived_header(wire,rules_,guard);}catch(const std::invalid_argument&){if(interrupted)std::rethrow_exception(interrupted);return false;}
  const auto id=consensus_beacon_header_root(header);
  if(!state_->blocks.contains(id)){const auto prior=signed_block_locked(id,check);return prior&&std::equal(wire.begin(),wire.end(),prior->begin(),prior->end());}
 }
 auto next=prepare_capella_received_block(*state_,wire,rules_,limits_.replay_slots,limits_.sync_trials,check);if(!next)return false;
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
 const auto record=disk_.checkpoint_archive(Bytes(id.begin(),id.end()),limits_.envelope_bytes);if(!record)return std::nullopt;
 if(state_->blocks.contains(id)){
  archive_require(record->slot==state_->blocks.at(id).header.slot);verify_archived_block(id,record->payload,*state_,rules_,check);return record->payload;
 }
 // Historical serving is anchored in the current authenticated FINALIZED
 // ancestry, not in a peer's hint or an arbitrary signed archive key. Registry
 // entries/public keys are append-only; the receiver still replays admission.
 const auto& anchor=state_->blocks.at(state_->timed.view.anchor);
 const auto header=archived_header(record->payload,rules_,check);
 archive_require(record->slot==header.slot&&consensus_beacon_header_root(header)==id);
 if(header.slot>=anchor.header.slot)return std::nullopt;
 bool proven=false;
 if(const auto cached=historical_headers_.find(id);cached!=historical_headers_.end()){archive_require(cached->second==header);proven=true;}
 auto parent=anchor.header.parent_root;auto previous_slot=anchor.header.slot;
 // Pick the closest already authenticated ancestor. A finalized prefix cannot
 // reorg. Each cached header came from a hash-linked walk under mutex_, and the
 // current record's body/signature is always checked again before export.
 for(const auto& [root,known]:historical_headers_)if(known.slot>header.slot&&known.slot<previous_slot){parent=known.parent_root;previous_slot=known.slot;}
 size_t used=0;
 for(size_t count=0;!proven&&count<limits_.blocks;++count){
  if(check)check();const auto item=parent==id?record:disk_.checkpoint_archive(Bytes(parent.begin(),parent.end()),limits_.envelope_bytes);
  if(!item||item->payload.size()>limits_.envelope_bytes-used)return std::nullopt;used+=item->payload.size();
  const auto candidate=parent==id?header:archived_header(item->payload,rules_,check);
  archive_require(item->slot==candidate.slot&&candidate.slot<previous_slot&&consensus_beacon_header_root(candidate)==parent);
  // A verified finalized prefix remains safe even if this request exhausts
  // its budget. Later bounded requests can resume without trusting a peer.
  historical_headers_.insert_or_assign(parent,candidate);
  if(historical_headers_.size()>limits_.blocks){const auto oldest=std::min_element(historical_headers_.begin(),historical_headers_.end(),[](const auto& a,const auto& b){return a.second.slot<b.second.slot;});historical_headers_.erase(oldest);}
  if(parent==id){proven=true;break;}
  if(candidate.slot<=header.slot)return std::nullopt;
  parent=candidate.parent_root;previous_slot=candidate.slot;
 }
 if(!proven)return std::nullopt;
 const auto& r=rules_.transition;const auto context=capella_fork_block_context(anchor.state,anchor.header,CapellaEpochRules{r.consensus.epoch,r.execution,r.maximum_state_bytes},r.consensus.state,check);
 const auto proposer=context.registry.validators.find(header.proposer_index);archive_require(proposer!=context.registry.validators.end());
 const auto epoch=header.slot/r.consensus.epoch.ffg.slots_per_epoch;
 const auto version=archive_array<4>(anchor.state.ssz,epoch<archive_number(anchor.state.ssz,56)?48:52);
 archive_require(verify_consensus_beacon_header(header,version,archive_array<32>(anchor.state.ssz,8),proposer->second.public_key,std::span(record->payload).subspan(4,96)));if(check)check();return record->payload;
}

std::vector<ConsensusRoot> CapellaDurableReactor::exit_roots_locked(const std::function<void()>& check)const{
 std::vector<ConsensusRoot> roots;roots.reserve(exits_.size());for(const auto& operation:pending_exits_locked(limits_.messages,check)){if(check)check();roots.push_back(archive_array<32>(sha256(encode_consensus_voluntary_exit_ssz(operation)),0));}std::sort(roots.begin(),roots.end());return roots;
}
std::vector<ConsensusRoot> CapellaDurableReactor::bls_change_roots_locked(const std::function<void()>& check)const{
 std::vector<ConsensusRoot> roots;roots.reserve(bls_changes_.size());for(const auto& operation:pending_bls_changes_locked(limits_.messages,check)){if(check)check();const auto digest=sha256(encode_capella_bls_change(operation));roots.push_back(archive_array<32>(digest,0));}std::sort(roots.begin(),roots.end());return roots;
}

bool CapellaDurableReactor::sync_messages_current_locked(const PendingSyncMessages& messages,const std::function<void()>& check)const{
 const auto& clock=state_->timed.clock;
 const auto slot=(clock.time_ms-clock.genesis_time_ms)/rules_.slot_duration_ms;
 if(messages.slot<slot||messages.slot-slot>1)return false;
 const auto& c=rules_.transition.consensus;
 const auto head=phase0_fork_choice_head(state_->timed.view,{c.epoch.ffg.slots_per_epoch,c.attestation.duties.effective_balance_increment,rules_.proposer_score_boost},check).head;
 return head==messages.parent;
}

std::shared_ptr<const CapellaInventorySnapshot> CapellaDurableReactor::capture_inventory(std::string_view kind,size_t budget,const std::function<void()>& check)const{
 std::lock_guard lock(mutex_);require_available();if(check)check();
 if(kind!="block"&&kind!="attestation"&&kind!="attester_slashing"&&kind!="voluntary_exit"&&kind!="bls_to_execution_change"&&kind!="sync_committee_message")throw std::invalid_argument("Unsupported Capella inventory kind");
 const bool sync=kind=="sync_committee_message";size_t sync_count=0;if(sync)for(const auto& [_,messages]:pending_sync_messages_)if(sync_messages_current_locked(messages,check)){if(messages.wires.size()>budget/sizeof(ConsensusRoot)-sync_count)throw std::length_error("Capella sync inventory exceeds root budget");sync_count+=messages.wires.size();}
 const auto count=sync?sync_count:kind=="block"?state_->blocks.size()-size_t(state_->blocks.contains(state_->timed.view.anchor)):kind=="attestation"?votes_.size():kind=="bls_to_execution_change"?bls_changes_.size():kind=="voluntary_exit"?exits_.size():slashings_.size();
 if(count>budget/sizeof(ConsensusRoot))throw std::length_error("Capella inventory snapshot exceeds root budget");
 auto snapshot=std::make_shared<CapellaInventorySnapshot>();snapshot->token_=*token_;snapshot->kind_=kind;snapshot->roots_.reserve(count);
 if(sync){for(const auto& [_,messages]:pending_sync_messages_)if(sync_messages_current_locked(messages,check))for(const auto& [index,wire]:messages.wires){if(check)check();snapshot->roots_.push_back(archive_array<32>(sha256(wire),0));}std::sort(snapshot->roots_.begin(),snapshot->roots_.end());}
 else if(kind=="block"){for(const auto& [root,_]:state_->blocks){if(check)check();if(root!=state_->timed.view.anchor)snapshot->roots_.push_back(root);}}
 else if(kind=="voluntary_exit")snapshot->roots_=exit_roots_locked(check);
 else if(kind=="bls_to_execution_change")snapshot->roots_=bls_change_roots_locked(check);
 else{for(const auto& [key,_]:kind=="attestation"?votes_:slashings_){if(check)check();ConsensusRoot root{};if(key.size()!=root.size())throw std::logic_error("Invalid Capella archive root");std::copy(key.begin(),key.end(),root.begin());snapshot->roots_.push_back(root);}}
 if(snapshot->roots_.capacity()>budget/sizeof(ConsensusRoot))throw std::length_error("Capella inventory allocation exceeds root budget");return snapshot;
}
Json CapellaInventorySnapshot::page(const std::optional<ConsensusRoot>& after,size_t count,size_t budget,const std::function<void()>& check)const{
 if(check)check();if(!count||!budget)throw std::invalid_argument("Capella inventory page capacity required");
 auto it=roots_.begin();if(after){it=std::lower_bound(roots_.begin(),roots_.end(),*after);if(it==roots_.end()||*it!=*after)throw std::invalid_argument("Unknown Capella snapshot cursor");++it;}
 Json result{{"sequence",quantity(Int(std::to_string(token_.sequence)))},{"digest",hex(token_.digest)},{"kind",kind_},{"entries",Json::array()},{"next",hex(Bytes(32,0))}};
 size_t used=result.dump().size();if(used>budget)throw std::length_error("Capella snapshot framing exceeds capacity");std::string last;
 for(;it!=roots_.end()&&result["entries"].size()<count;++it){if(check)check();const auto encoded=hex(Bytes(it->begin(),it->end()));const size_t size=encoded.size()+2+(result["entries"].empty()?0:1);if(size>budget-used){if(result["entries"].empty())throw std::length_error("Capella snapshot entry exceeds capacity");break;}used+=size;last=encoded;result["entries"].push_back(encoded);}
 result["next"]=it==roots_.end()?Json(nullptr):Json(last);if(check)check();return result;
}

Json CapellaDurableReactor::inventory(std::string_view kind,const std::optional<ConsensusRoot>& after,size_t count,size_t budget,const std::optional<ForkCheckpointToken>& expected,const std::function<void()>& check)const{
 std::lock_guard lock(mutex_);require_available();if(check)check();
 if(!count||!budget)throw std::invalid_argument("Capella inventory capacity required");
 if(kind!="block"&&kind!="attestation"&&kind!="attester_slashing"&&kind!="voluntary_exit"&&kind!="bls_to_execution_change")throw std::invalid_argument("Unsupported Capella inventory kind");
 if(after&&!expected)throw std::invalid_argument("Capella continuation needs checkpoint");
 if(expected&&*expected!=*token_)throw std::invalid_argument("Stale Capella inventory checkpoint");
 Json result{{"sequence",quantity(Int(std::to_string(token_->sequence)))},{"digest",hex(token_->digest)},{"kind",kind},{"entries",Json::array()},{"next",hex(Bytes(32,0))}};
 size_t used=result.dump().size();if(used>budget)throw std::length_error("Capella inventory framing exceeds capacity");
 std::string last;bool more=false;
 auto append=[&](const Bytes& key){if(check)check();if(result["entries"].size()>=count){more=true;return false;}Json item=hex(key);const auto size=item.dump().size()+(result["entries"].empty()?0:1);if(size>budget-used){if(result["entries"].empty())throw std::length_error("Capella inventory entry exceeds capacity");more=true;return false;}used+=size;last=item.get<std::string>();result["entries"].push_back(std::move(item));return true;};
 if(kind=="block"){
  if(after&&(!state_->blocks.contains(*after)||*after==state_->timed.view.anchor))throw std::invalid_argument("Unknown Capella inventory cursor");
  for(auto it=after?state_->blocks.upper_bound(*after):state_->blocks.begin();it!=state_->blocks.end();++it){if(check)check();if(it->first==state_->timed.view.anchor)continue;if(!append(Bytes(it->first.begin(),it->first.end())))break;}
 }else if(kind=="bls_to_execution_change"||kind=="voluntary_exit"){
  const auto roots=kind=="voluntary_exit"?exit_roots_locked(check):bls_change_roots_locked(check);auto it=roots.begin();if(after){it=std::lower_bound(roots.begin(),roots.end(),*after);if(it==roots.end()||*it!=*after)throw std::invalid_argument("Unknown Capella inventory cursor");++it;}for(;it!=roots.end();++it)if(!append(Bytes(it->begin(),it->end())))break;
 }else{
  const auto& archive=kind=="attestation"?votes_:slashings_;Bytes cursor;if(after){cursor=Bytes(after->begin(),after->end());if(!archive.contains(cursor))throw std::invalid_argument("Unknown Capella inventory cursor");}
  for(auto it=after?archive.upper_bound(cursor):archive.begin();it!=archive.end();++it)if(!append(it->first))break;
 }
 result["next"]=more?Json(last):Json(nullptr);if(check)check();return result;
}

Json CapellaDurableReactor::export_message(std::string_view kind,const ConsensusRoot& id,size_t maximum,const std::optional<ForkCheckpointToken>& expected,const std::function<void()>& check)const{
 std::lock_guard lock(mutex_);require_available();if(check)check();
 if(!maximum)throw std::invalid_argument("Capella export capacity required");
 if(kind!="block"&&kind!="attestation"&&kind!="attester_slashing"&&kind!="voluntary_exit"&&kind!="bls_to_execution_change"&&kind!="sync_committee_message")throw std::invalid_argument("Unsupported Capella archive kind");
 if(expected&&*expected!=*token_)throw std::invalid_argument("Stale Capella export checkpoint");
 const Bytes key(id.begin(),id.end());std::optional<Bytes> payload;
 if(kind=="sync_committee_message"){for(const auto& [_,messages]:pending_sync_messages_)if(sync_messages_current_locked(messages,check))for(const auto& [index,wire]:messages.wires){if(check)check();if(sha256(wire)==key){if(wire.size()>maximum)throw std::length_error("Capella export exceeds capacity");payload=wire;break;}}}
 else if(kind=="block")payload=signed_block_locked(id,check);
 else if(kind=="voluntary_exit"){const auto found=exits_.find(key);if(found!=exits_.end()){if(found->second.wire.size()>maximum)throw std::length_error("Capella export exceeds capacity");payload=found->second.wire;}}
 else if(kind=="bls_to_execution_change"){for(const auto& [index,record]:bls_changes_){if(check)check();if(sha256(record.wire)==key){if(record.wire.size()>maximum)throw std::length_error("Capella export exceeds capacity");payload=record.wire;break;}}}
 else{const auto& archive=kind=="attestation"?votes_:slashings_;const auto found=archive.find(key);if(found!=archive.end()){
  if(found->second.size()>maximum)throw std::length_error("Capella export exceeds capacity");
  if(sha256(found->second)!=key)throw std::runtime_error("Capella archive digest mismatch");payload=found->second;
 }}
 Json result{{"available",bool(payload)},{"root",hex(key)},{"sequence",quantity(Int(std::to_string(token_->sequence)))},{"digest",hex(token_->digest)},
  {"known",kind=="block"&&state_->blocks.contains(id)},{"anchor",kind=="block"&&id==state_->timed.view.anchor}};
 if(kind=="block"&&state_->blocks.contains(id)){const auto& header=state_->blocks.at(id).header;result["parentRoot"]=hex(Bytes(header.parent_root.begin(),header.parent_root.end()));result["slot"]=quantity(Int(std::to_string(header.slot)));}
 if(kind=="block"&&payload&&!state_->blocks.contains(id)){const auto header=archived_header(*payload,rules_,check);result["parentRoot"]=hex(Bytes(header.parent_root.begin(),header.parent_root.end()));result["slot"]=quantity(Int(std::to_string(header.slot)));result["historical"]=true;}
 if(payload){if(payload->size()>maximum)throw std::length_error("Capella export exceeds capacity");auto envelope=network_identity_locked();envelope["kind"]=kind;envelope["ssz"]=hex(*payload);result["envelope"]=std::move(envelope);}
 if(check)check();return result;
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

namespace nano {
bool CapellaDurableReactor::prune_finalized(const std::function<void()>& check){
 std::lock_guard lock(mutex_);require_available();if(check)check();
 const auto anchor=state_->timed.view.finalized.root;
 if(anchor==state_->timed.view.anchor)return false;
 auto candidate=prepare_capella_pruned_store(*state_,anchor,rules_,check);
 // publish reconciles pending BLS admission roots and verifies all retained
 // signed votes/proofs in the exact new envelope before committing it.
 publish(std::move(candidate),check);return true;
}
}

namespace nano {
void CapellaDurableReactor::set_automatic_pruning(bool enabled){std::lock_guard lock(mutex_);require_available();automatic_pruning_=enabled;}
void CapellaDurableReactor::prune_if_ready_locked(const std::function<void()>& check){
 const auto& view=state_->timed.view;const auto anchor=view.finalized.root;
 if(anchor==view.anchor)return;
 const auto lookahead=rules_.transition.consensus.attestation.duties.min_seed_lookahead;
 const auto slots=rules_.transition.consensus.epoch.ffg.slots_per_epoch;
 if(!slots||lookahead>=view.current_epoch)return;
 const auto retained_epoch=view.current_epoch-lookahead-1;
 if(retained_epoch>UINT64_MAX/slots||state_->blocks.at(anchor).header.slot>retained_epoch*slots)return;
 auto candidate=prepare_capella_pruned_store(*state_,anchor,rules_,check);
 // A recent vote on another branch can still be needed for authenticated replay.
 // Wait for normal epoch expiry rather than silently discarding signed evidence.
 for(const auto& [id,wire]:votes_){if(check)check();const auto vote=decode_consensus_attestation_ssz(wire,rules_.transition.consensus.body.validators_per_committee,check);if(!vote)throw std::logic_error("invalid retained Capella vote");if(!candidate.blocks.contains(vote->data.target.root)||!candidate.blocks.contains(vote->data.beacon_block_root))return;}
 publish(std::move(candidate),check);
}
}

namespace nano {
void CapellaDurableReactor::set_historical_state_pruning(bool enabled){std::lock_guard lock(mutex_);require_available();historical_state_pruning_=enabled;}
}
