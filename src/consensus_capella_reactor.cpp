#include "nano/consensus_capella_reactor.hpp"
#include "nano/consensus_slashing.hpp"
#include "nano/consensus_capella_bootstrap_archive.hpp"
#include "nano/checkpoint_blob_gc.hpp"
#include "nano/chain.hpp"
#include "nano/consensus_bls.hpp"
#include "nano/consensus_message_ssz.hpp"
#include "consensus_checkpoint_json.hpp"
#include "consensus_capella_checkpoint_fragments.hpp"
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
 const auto block_pin=store.blocks.at(id).lease(check);const auto& block=*block_pin;archive_require(header==block.header&&consensus_beacon_header_root(header)==id);
 const auto context=capella_fork_block_context(block.state,header,CapellaEpochRules{r.consensus.epoch,r.execution,r.maximum_state_bytes},r.consensus.state,check);
 const auto proposer=context.registry.validators.find(header.proposer_index);archive_require(proposer!=context.registry.validators.end());
 const auto& state=block.state.ssz;const auto version=archive_array<4>(state,context.registry.epoch<archive_number(state,56)?48:52);
 archive_require(verify_consensus_beacon_header(header,version,archive_array<32>(state,8),proposer->second.public_key,std::span(wire).subspan(4,96)));if(check)check();
}
CapellaForkStore continued_tick(const CapellaForkStore& store,uint64_t time,
 const CapellaForkBlockRules& rules,const CapellaStoreSnapshotLimits& limits,const std::function<void()>& check){
 const auto& clock=store.timed.clock;const auto slots=rules.transition.consensus.epoch.ffg.slots_per_epoch;
 if(!slots||!rules.slot_duration_ms||time<clock.time_ms||clock.time_ms<clock.genesis_time_ms)throw std::invalid_argument("Invalid continued Capella tick");
 std::optional<CapellaTargetPreparation> frame;std::vector<const CapellaTargetPreparation*> targets;
 if((time-clock.genesis_time_ms)/rules.slot_duration_ms/slots>store.timed.view.current_epoch&&clock.unrealized_justified.epoch>store.timed.view.justified.epoch){
  frame.emplace(store,rules,clock.unrealized_justified,check);
  while(!frame->advance(std::min(limits.replay_slots,slots),limits.sync_trials,check)){}
  targets.push_back(&*frame);
 }
 return prepare_capella_contextual_store_tick(store,time,rules,targets,limits.replay_slots,limits.sync_trials,check);
}
std::optional<CapellaForkStore> checked_vote(const CapellaForkStore& store,const Phase0SignedAttestation& vote,const CapellaForkBlockRules& rules,const CapellaStoreSnapshotLimits& limits,const std::function<void()>& check,const CapellaTargetPreparation* prepared=nullptr){
 const auto& clock=store.timed.clock;if(!rules.slot_duration_ms||clock.time_ms<clock.genesis_time_ms)throw std::invalid_argument("Capella vote clock");
 const auto slot=(clock.time_ms-clock.genesis_time_ms)/rules.slot_duration_ms;
 auto apply=[&](const CapellaForkStore& input){return prepared?prepare_capella_prepared_store_vote(input,vote,rules,*prepared,check):prepare_capella_store_vote(input,vote,rules,limits.replay_slots,limits.sync_trials,check);};
 if(vote.data.slot!=slot)return apply(store);
 if(slot==UINT64_MAX||slot+1>(UINT64_MAX-clock.genesis_time_ms)/rules.slot_duration_ms)throw std::overflow_error("Capella deferred vote clock");
 const auto future=continued_tick(store,clock.genesis_time_ms+(slot+1)*rules.slot_duration_ms,rules,limits,check);
 if(!apply(future))return std::nullopt;
 return store;
}
bool valid_bls_change(const CapellaForkStore& store,const ConsensusRoot& root,const SignedCapellaBlsChange& change,const CapellaForkBlockRules& rules,const std::function<void()>& check){
 const auto found=store.blocks.find(root);if(found==store.blocks.end())return false;const auto& t=rules.transition;const auto found_pin=found->second.lease(check);const auto& state=found_pin->state;
 const auto digest=capella_beacon_state_root(state.ssz,t.consensus.state,t.execution,t.maximum_state_bytes,check);
 return prepare_capella_bls_changes(state.ssz,digest,t.consensus.state,t.execution,t.maximum_state_bytes,t.genesis_version,t.maximum_bls_changes,std::span(&change,1),check).has_value();
}
std::optional<std::array<uint8_t,48>> exit_key(const CapellaForkStore& store,const ConsensusRoot& root,const SignedConsensusVoluntaryExit& exit,const CapellaForkBlockRules& rules,const std::function<void()>& check){
 const auto found=store.blocks.find(root);if(found==store.blocks.end())return std::nullopt;
 const auto& t=rules.transition;const auto found_pin=found->second.lease(check);const auto context=capella_fork_block_context(found_pin->state,found_pin->header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state,check);
 const auto member=context.registry.validators.find(exit.validator_index);
 if(member==context.registry.validators.end()||member->second.exit_epoch!=UINT64_MAX||exit.epoch>context.registry.epoch||member->second.activation_epoch>context.registry.epoch||context.registry.epoch-member->second.activation_epoch<t.consensus.exit.shard_committee_period)return std::nullopt;
 const auto version=exit.epoch<context.fork.epoch?context.fork.previous_version:context.fork.current_version;
 if(!verify_consensus_bls(member->second.public_key,consensus_signing_root(consensus_voluntary_exit_root(exit.epoch,exit.validator_index),consensus_domain({4,0,0,0},version,context.genesis_root)),exit.signature))return std::nullopt;
 return member->second.public_key;
}
// Authentication is anchored to a retained signed post-state, with bounded
// epoch projection when admission happens after that state's epoch. Headers
// need not describe a canonical block or the selected proposer at their slot.
std::optional<std::array<uint8_t,48>> proposer_slashing_key(const CapellaForkStore& store,const ConsensusRoot& root,uint64_t epoch,const SignedConsensusBeaconHeader& first,const SignedConsensusBeaconHeader& second,const CapellaForkBlockRules& rules,const CapellaStoreSnapshotLimits& limits,const std::function<void()>& check){
 const auto found=store.blocks.find(root);if(found==store.blocks.end())return std::nullopt;
 const auto& t=rules.transition;const auto slots=t.consensus.epoch.ffg.slots_per_epoch;
 if(!slots||epoch>store.timed.view.current_epoch)return std::nullopt;
 const auto pin=found->second.lease(check);validate_capella_execution_record(*pin,rules,check);
 const CapellaEpochRules epochs{t.consensus.epoch,t.execution,t.maximum_state_bytes};
 auto context=capella_fork_block_context(pin->state,pin->header,epochs,t.consensus.state,check);
 if(context.block_root!=root||context.registry.epoch>epoch)return std::nullopt;
 if(context.registry.epoch<epoch){
  const auto projected=prepare_capella_fork_state_contexts(pin->state,pin->header,{epoch,root},epochs,t.consensus.state,limits.replay_slots,limits.sync_trials,check);
  context.registry=projected.target.registry;
 }
 const auto member=context.registry.validators.find(first.message.proposer_index);
 if(member==context.registry.validators.end()||member->second.slashed||member->second.activation_epoch>epoch||epoch>=member->second.withdrawable_epoch)return std::nullopt;
 const auto version=first.message.slot/slots<context.fork.epoch?context.fork.previous_version:context.fork.current_version;
 if(check)check();if(!verify_proposer_slashing(first,second,member->second.public_key,version,context.genesis_root))return std::nullopt;
 if(check)check();return member->second.public_key;
}
Bytes canonical_proposer_slashing_wire(SignedConsensusBeaconHeader first,SignedConsensusBeaconHeader second,const std::function<void()>& check){
 auto wire=encode_consensus_proposer_slashing_ssz(std::move(first),std::move(second),check);
 if(std::lexicographical_compare(wire.begin()+208,wire.end(),wire.begin(),wire.begin()+208))std::rotate(wire.begin(),wire.begin()+208,wire.end());
 return wire;
}
std::string durable_snapshot(const CapellaForkStore& store,const CapellaSignedVotes& votes,const CapellaSignedVotes& slashings,const CapellaPendingBlsChanges& changes,const CapellaPendingExits& exits,const CapellaForkBlockRules& rules,const CapellaStoreSnapshotLimits& limits,const std::function<void()>& check,CapellaStoreSnapshotCache* cache=nullptr,const std::function<void(const CapellaColdReference&,std::string_view)>* stage=nullptr,CapellaReferenceSnapshotCache* reference_cache=nullptr,const CapellaPendingProposerSlashings& proposers={}){
 const auto encoded=stage?(reference_cache?capella_store_reference_snapshot_cached(store,rules,limits,*stage,*reference_cache,check):capella_store_reference_snapshot(store,rules,limits,*stage,check)):capella_store_snapshot(store,rules,limits,check,cache);if(votes.empty()&&slashings.empty()&&changes.empty()&&exits.empty()&&proposers.empty())return encoded;
 if(votes.size()>limits.messages)throw std::length_error("Capella signed vote capacity");Json list=Json::array();size_t total=0;
 for(const auto& [id,wire]:votes){if(check)check();if(wire.size()>limits.envelope_bytes/2-total)throw std::length_error("Capella signed vote bytes");total+=wire.size();list.push_back(Json::array({hex(id),hex(wire)}));}
 Json document{{"format","nano-capella-durable-v1"},{"signedVotes",std::move(list)}};
 if(!slashings.empty()||!changes.empty()||!exits.empty()||!proposers.empty()){
  if(slashings.size()>limits.equivocations)throw std::length_error("Capella slashing capacity");Json proofs=Json::array();
  for(const auto& [id,wire]:slashings){if(check)check();if(wire.size()>limits.envelope_bytes/2-total)throw std::length_error("Capella slashing bytes");total+=wire.size();proofs.push_back(Json::array({hex(id),hex(wire)}));}
  document["format"]="nano-capella-durable-v2";document["signedSlashings"]=std::move(proofs);
 }
 if(!changes.empty()||!exits.empty()||!proposers.empty()){
  if(changes.size()>limits.messages)throw std::length_error("Capella BLS change capacity");Json operations=Json::array();
  for(const auto& [index,record]:changes){if(check)check();if(record.wire.size()!=172||204>limits.envelope_bytes/2-total)throw std::length_error("Capella BLS change bytes");total+=204;operations.push_back(Json::array({quantity(Int(std::to_string(index))),hex(Bytes(record.admission_root.begin(),record.admission_root.end())),hex(record.wire)}));}
  document["format"]="nano-capella-durable-v3";document["blsChanges"]=std::move(operations);
 }
 if(!exits.empty()||!proposers.empty()){
  if(exits.size()>limits.messages)throw std::length_error("Capella exit capacity");Json operations=Json::array();
  for(const auto& [id,record]:exits){if(check)check();if(id.size()!=32||record.wire.size()!=112||144>limits.envelope_bytes/2-total)throw std::length_error("Capella exit bytes");total+=144;operations.push_back(Json::array({hex(id),hex(Bytes(record.admission_root.begin(),record.admission_root.end())),hex(record.wire)}));}
  document["format"]="nano-capella-durable-v4";document["signedExits"]=std::move(operations);
 }
 if(!proposers.empty()){
  if(!rules.transition.consensus.body.proposer_slashings||proposers.size()>limits.equivocations)throw std::length_error("Capella proposer slashing capacity");Json operations=Json::array();
  for(const auto& [id,record]:proposers){if(check)check();if(id.size()!=32||record.wire.size()!=416||456>limits.envelope_bytes/2-total)throw std::length_error("Capella proposer slashing bytes");total+=456;operations.push_back(Json::array({hex(id),hex(Bytes(record.admission_root.begin(),record.admission_root.end())),quantity(Int(std::to_string(record.admission_epoch))),hex(record.wire)}));}
  document["format"]="nano-capella-durable-v5";document["proposerSlashings"]=std::move(operations);
 }
 // encoded is the validated canonical store above, never a peer-provided value.
 // Preserve the existing Capella envelope exception type.
 try{return checkpoint_detail::durable_json(std::move(document),encoded,limits.envelope_bytes);}
 catch(const std::invalid_argument&){throw std::length_error("Capella durable envelope");}
}
std::optional<CapellaColdCheckpointOptions> cold_options(std::string_view encoded,const CapellaStoreSnapshotLimits& limits){
 if(!encoded.starts_with("{\"cache\":"))return std::nullopt;
 const auto j=Json::parse(encoded);archive_require(j.is_object()&&j.size()==3&&j.at("format")=="nano-capella-cold-checkpoint-v1"&&j.dump()==encoded);
 const auto& c=j.at("cache");archive_require(c.is_array()&&c.size()==2&&c[0].is_string()&&c[1].is_string());
 const auto count=u64(quantity(c[0])),bytes=u64(quantity(c[1]));archive_require(count&&count<=limits.blocks&&bytes&&bytes<=limits.envelope_bytes&&quantity(Int(std::to_string(count)))==c[0].get<std::string>()&&quantity(Int(std::to_string(bytes)))==c[1].get<std::string>());
 return CapellaColdCheckpointOptions{size_t(count),size_t(bytes)};
}
std::string cold_envelope(std::string_view payload,CapellaColdCheckpointOptions options){return Json{{"format","nano-capella-cold-checkpoint-v1"},{"cache",Json::array({quantity(Int(std::to_string(options.cache_records))),quantity(Int(std::to_string(options.cache_bytes)))})},{"checkpoint",Json::parse(payload)}}.dump();}
std::tuple<CapellaForkStore,CapellaSignedVotes,CapellaSignedVotes,CapellaPendingBlsChanges,CapellaPendingExits,CapellaPendingProposerSlashings> restore_durable(std::string_view encoded,const Bytes& digest,const CapellaForkBlockRules& rules,const CapellaStoreSnapshotLimits& limits,const std::function<void()>& check,const std::function<std::optional<std::string>(const Bytes&,size_t)>& fetch={},std::optional<CapellaColdCheckpointOptions> cold=std::nullopt,const CapellaForkStore* source=nullptr,CapellaReferenceSnapshotCache* memo=nullptr){
 if(check)check();archive_require(encoded.size()<=limits.envelope_bytes&&digest.size()==32&&sha256(Bytes(encoded.begin(),encoded.end()))==digest);
 const auto document=Json::parse(encoded);
 if(const auto options=cold_options(encoded,limits)){
  archive_require(!cold&&bool(fetch));const auto inner=document.at("checkpoint").dump();
  return restore_durable(inner,sha256(Bytes(inner.begin(),inner.end())),rules,limits,check,fetch,options,source,memo);
 }
 auto restore_store=[&](std::string_view bytes,const Bytes& committed){
  if(cold&&memo){
   // An empty source certifies via the full decoder; only subsequent identical
   // records can reuse that certificate, with fresh provider digest checks.
   const CapellaForkStore empty{};
   return capella_store_from_reference_snapshot_reusing(bytes,committed,rules,limits,cold->cache_records,cold->cache_bytes,fetch,source?*source:empty,*memo,check);
  }
  return cold?capella_store_from_reference_snapshot(bytes,committed,rules,limits,cold->cache_records,cold->cache_bytes,fetch,check):capella_store_from_snapshot(bytes,committed,rules,limits,check);
 };
 if(document.value("format",std::string())=="nano-capella-fragments-v1"){
  archive_require(!cold&&bool(fetch));const auto expanded=capella_checkpoint_detail::expand_fragment_checkpoint(encoded,digest,limits.envelope_bytes,limits.blocks,fetch,check);
  return restore_durable(expanded,sha256(Bytes(expanded.begin(),expanded.end())),rules,limits,check);
 }
 const bool with_proposers=document.value("format",std::string())=="nano-capella-durable-v5";
 const bool with_exits=with_proposers||document.value("format",std::string())=="nano-capella-durable-v4";
 const bool with_changes=with_exits||document.value("format",std::string())=="nano-capella-durable-v3";
 const bool with_proofs=with_changes||document.value("format",std::string())=="nano-capella-durable-v2";
 if(!with_proofs&&document.value("format",std::string())!="nano-capella-durable-v1")return {restore_store(encoded,digest),{}, {}, {}, {}, {}};
 archive_require(document.size()==(with_proposers?7:with_exits?6:with_changes?5:with_proofs?4:3)&&document.at("signedVotes").is_array()&&(with_proofs||!document.at("signedVotes").empty())&&document.at("signedVotes").size()<=limits.messages);
 const auto inner=document.at("store").dump();auto store=restore_store(inner,sha256(Bytes(inner.begin(),inner.end())));CapellaSignedVotes votes,proofs;
 std::array<std::optional<CapellaTargetPreparation>,2> restored_targets;
 for(const auto& entry:document.at("signedVotes")){
  if(check)check();archive_require(entry.is_array()&&entry.size()==2&&entry[0].is_string()&&entry[1].is_string());
  const auto key=entry[0].get<std::string>(),payload=entry[1].get<std::string>();archive_require(key.size()==66&&payload.size()>=2&&payload.size()%2==0&&(payload.size()-2)/2<=228+uint64_t(rules.transition.consensus.body.validators_per_committee)/8+1);
  auto id=unhex(key),wire=unhex(payload);archive_require(hex(id)==key&&hex(wire)==payload&&sha256(wire)==id);
  const auto vote=decode_consensus_attestation_ssz(wire,rules.transition.consensus.body.validators_per_committee,check);archive_require(bool(vote));
  const auto slots=rules.transition.consensus.epoch.ffg.slots_per_epoch,current=(store.timed.clock.time_ms-store.timed.clock.genesis_time_ms)/rules.slot_duration_ms;
  archive_require(slots&&vote->data.slot<=current&&vote->data.target.epoch==vote->data.slot/slots&&vote->data.target.epoch<=current/slots&&vote->data.target.epoch>=(current/slots?current/slots-1:0)&&store.blocks.contains(vote->data.target.root)&&store.blocks.at(vote->data.target.root).header().slot<=vote->data.target.epoch*slots);
  auto& frame=restored_targets[vote->data.target.epoch%restored_targets.size()];
  if(!frame||!frame->matches(store,rules,vote->data.target,check))frame.emplace(store,rules,vote->data.target,check);
  while(!frame->advance(std::min(limits.replay_slots,slots),limits.sync_trials,check)){};
  archive_require(checked_vote(store,*vote,rules,limits,check,&*frame).has_value());archive_require(votes.emplace(std::move(id),std::move(wire)).second);
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
 if(with_exits){const auto& list=document.at("signedExits");archive_require(list.is_array()&&(with_proposers||!list.empty())&&list.size()<=limits.messages);
  for(const auto& item:list){if(check)check();archive_require(item.is_array()&&item.size()==3&&item[0].is_string()&&item[1].is_string()&&item[2].is_string());
   const auto id_text=item[0].get<std::string>(),root_text=item[1].get<std::string>(),wire_text=item[2].get<std::string>();archive_require(id_text.size()==66&&root_text.size()==66&&wire_text.size()==226);
   const auto id=unhex(id_text),root_bytes=unhex(root_text),wire=unhex(wire_text);archive_require(hex(id)==id_text&&hex(root_bytes)==root_text&&hex(wire)==wire_text&&sha256(wire)==id);
   const auto root=archive_array<32>(root_bytes,0);const auto operation=decode_consensus_voluntary_exit_ssz(wire,check);archive_require(bool(operation));const auto key=exit_key(store,root,*operation,rules,check);archive_require(bool(key));
   archive_require(identities.emplace(operation->validator_index,*key).second&&exits.emplace(id,CapellaPendingExit{root,*key,wire}).second);
  }
 }
 CapellaPendingProposerSlashings proposers;std::set<std::pair<uint64_t,std::array<uint8_t,48>>> proposer_identities;
 if(with_proposers){const auto& list=document.at("proposerSlashings");archive_require(list.is_array()&&!list.empty()&&list.size()<=limits.equivocations);
  for(const auto& item:list){if(check)check();archive_require(item.is_array()&&item.size()==4&&item[0].is_string()&&item[1].is_string()&&item[2].is_string()&&item[3].is_string());
   const auto id_text=item[0].get<std::string>(),root_text=item[1].get<std::string>(),epoch_text=item[2].get<std::string>(),wire_text=item[3].get<std::string>();archive_require(id_text.size()==66&&root_text.size()==66&&epoch_text.size()>=3&&epoch_text.size()<=18&&wire_text.size()==834);
   const auto id=unhex(id_text),root_bytes=unhex(root_text),wire=unhex(wire_text);const auto epoch=u64(quantity(epoch_text));archive_require(hex(id)==id_text&&hex(root_bytes)==root_text&&hex(wire)==wire_text&&quantity(Int(std::to_string(epoch)))==epoch_text&&sha256(wire)==id);
   const auto root=archive_array<32>(root_bytes,0);const auto proof=decode_consensus_proposer_slashing_ssz(wire,check);archive_require(bool(proof)&&canonical_proposer_slashing_wire(proof->first,proof->second,check)==wire);
   const auto key=proposer_slashing_key(store,root,epoch,proof->first,proof->second,rules,limits,check);archive_require(bool(key));
   archive_require(proposer_identities.emplace(proof->first.message.proposer_index,*key).second&&proposers.emplace(id,CapellaPendingProposerSlashing{root,epoch,*key,wire}).second);
  }
 }
 const std::function<void(const CapellaColdReference&,std::string_view)> no_hot=[](const auto&,auto){throw std::logic_error("Unexpected restored hot row");};
 archive_require(durable_snapshot(store,votes,proofs,changes,exits,rules,limits,check,nullptr,cold?&no_hot:nullptr,cold?memo:nullptr,proposers)==encoded);return {std::move(store),std::move(votes),std::move(proofs),std::move(changes),std::move(exits),std::move(proposers)};
}

Json recovery_token_json(const std::optional<ForkCheckpointToken>& token){return token?Json{{"sequence",quantity(Int(std::to_string(token->sequence)))},{"digest",hex(token->digest)}}:Json(nullptr);}
std::optional<ForkCheckpointToken> recovery_token_parse(const Json& j){
 if(j.is_null())return std::nullopt;archive_require(j.is_object()&&j.size()==2);
 ForkCheckpointToken token{u64(quantity(j.at("sequence"))),unhex(j.at("digest").get<std::string>())};archive_require(token.sequence&&token.digest.size()==32&&recovery_token_json(token)==j);return token;
}

}

CapellaCheckpointProvider::Fetch CapellaDurableReactor::checkpoint_reader(ChainStore& store){
 auto& provider=&store==&disk_?primary_provider_:witness_provider_;
 archive_require(&store==&disk_||&store==witness_);
 if(!provider)provider=std::make_unique<CapellaCheckpointProvider>([disk=&store,chain=chain_,genesis=genesis_](const Bytes& digest,size_t bytes){return disk->checkpoint_blob(chain,genesis,digest,bytes);});
 return provider->reader();
}
size_t CapellaDurableReactor::witness_limit()const{
 if(limits_.envelope_bytes>(SIZE_MAX-2048)/8)throw std::length_error("Capella witness size overflow");return 2048+8*limits_.envelope_bytes;
}
void CapellaDurableReactor::witness_prepare(std::string_view snapshot,const std::vector<CheckpointArchiveEntry>& archive,const std::vector<CheckpointBlob>& blobs){
 if(!witness_)return;if(token_&&token_->sequence==UINT64_MAX)throw std::overflow_error("Capella witness sequence exhausted");
 archive_require(archive.size()<=1);Json entries=Json::array();for(const auto& entry:archive){archive_require(entry.root.size()==32&&entry.payload.size()<=limits_.envelope_bytes);entries.push_back(Json::array({hex(entry.root),quantity(Int(std::to_string(entry.slot))),hex(entry.payload)}));}
 const ForkCheckpointToken next{token_?token_->sequence+1:1,sha256(Bytes(snapshot.begin(),snapshot.end()))};
 const auto encoded=Json{{"format","nano-capella-recovery-v1"},{"previous",recovery_token_json(token_)},{"next",recovery_token_json(next)},{"snapshot",snapshot},{"archive",entries}}.dump();
 witness_token_=witness_->commit_fork_checkpoint_with_blobs(chain_,genesis_,witness_token_,encoded,witness_limit(),blobs,limits_.blocks);
}
void CapellaDurableReactor::validate_execution_binding()const{
 if(!execution_)return;std::lock_guard lock(execution_->mutex_);const auto& g=execution_->genesis_;
 if(&execution_->store_!=&disk_||g.consensus!="beacon"||g.evm_revision!=EvmRevision::Shanghai||g.chain_id!=chain_||g.block.header.hash()!=genesis_)throw std::invalid_argument("Capella execution owner/profile mismatch");
 const auto tip=disk_.latest();if(!tip||tip->header.hash()!=execution_->tip_.header.hash()||execution_->state_.root()!=tip->header.state_root)throw std::invalid_argument("Capella execution cache mismatch");
}
void CapellaDurableReactor::fail_closed(){if(!failure_)failure_=std::current_exception();unavailable_=true;std::atomic_store_explicit(&beacon_publication_,std::shared_ptr<const CapellaPublishedStore>{},std::memory_order_release);if(execution_){std::lock_guard lock(execution_->mutex_);execution_->beacon_ready_=false;}}
CapellaDurableReactor::~CapellaDurableReactor(){if(primary_provider_)primary_provider_->close();if(witness_provider_)witness_provider_->close();std::atomic_store_explicit(&beacon_publication_,std::shared_ptr<const CapellaPublishedStore>{},std::memory_order_release);if(execution_){std::lock_guard lock(execution_->mutex_);execution_->beacon_ready_=false;}}
void CapellaDurableReactor::invalidate(){std::lock_guard lock(mutex_);fail_closed();}
void CapellaDurableReactor::require_available()const{
 if(unavailable_||!state_||!token_){
  std::string message="Capella reactor unavailable; reopen required";
  if(failure_)try{std::rethrow_exception(failure_);}catch(const std::exception& error){message+="; initial failure: ";message+=error.what();}catch(...){message+="; initial failure: unknown exception";}
  throw std::runtime_error(message);
 }
}
void CapellaDurableReactor::publish(CapellaForkStore candidate,const std::function<void()>& check,const std::vector<CheckpointArchiveEntry>& archive,std::optional<CapellaSignedVotes> votes,std::optional<CapellaSignedVotes> slashings,std::optional<CapellaPendingBlsChanges> changes,std::optional<CapellaPendingExits> exits,std::optional<CapellaPendingProposerSlashings> proposers){
 publish_impl(std::move(candidate),check,archive,std::move(votes),std::move(slashings),std::move(changes),std::move(exits),false,std::nullopt,std::move(proposers));
}
void CapellaDurableReactor::publish_impl(CapellaForkStore candidate,const std::function<void()>& check,const std::vector<CheckpointArchiveEntry>& archive,std::optional<CapellaSignedVotes> votes,std::optional<CapellaSignedVotes> slashings,std::optional<CapellaPendingBlsChanges> changes,std::optional<CapellaPendingExits> exits,bool force_fragments,std::optional<CapellaColdCheckpointOptions> cold,std::optional<CapellaPendingProposerSlashings> proposers){
 if(unavailable_)require_available();
 if(chain_!=rules_.transition.chain_id||genesis_.size()!=32)throw std::invalid_argument("Capella reactor chain/profile mismatch");
 // Only finalized sanctions retire evidence. An unfinalized inclusion can reorg.
 if(!slashings)slashings=slashings_;
 if(!slashings->empty()&&state_&&state_->timed.view.finalized.root!=candidate.timed.view.finalized.root){
  const auto finalized_pin=candidate.blocks.at(candidate.timed.view.finalized.root).lease(check);const auto& finalized=*finalized_pin;const auto& t=rules_.transition;
  const auto context=capella_fork_block_context(finalized.state,finalized.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state,check);
  for(auto it=slashings->begin();it!=slashings->end();){if(check)check();const auto proof=decode_consensus_attester_slashing_ssz(it->second,t.consensus.body.validators_per_committee,check);if(!proof)throw std::logic_error("invalid retained Capella slashing");bool needed=false;
   for(auto index:proof->first.indices){if(check)check();if(!std::binary_search(proof->second.indices.begin(),proof->second.indices.end(),index))continue;const auto member=context.registry.validators.find(index);if(member==context.registry.validators.end()||!member->second.slashed){needed=true;break;}}
   if(needed)++it;else it=slashings->erase(it);
  }
 }
 if(!changes)changes=bls_changes_;
 if(!changes->empty()){
  const auto finalized_pin=candidate.blocks.at(candidate.timed.view.finalized.root).lease(check);const auto& finalized=*finalized_pin;const auto& t=rules_.transition;
  const auto context=capella_fork_block_context(finalized.state,finalized.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state,check);
  for(auto it=changes->begin();it!=changes->end();){if(check)check();const auto member=context.registry.validators.find(it->first);
   if(member!=context.registry.validators.end()&&member->second.withdrawal_credentials[0]==1){it=changes->erase(it);continue;}
   if(!candidate.blocks.contains(it->second.admission_root)){const auto operation=decode_capella_bls_change(it->second.wire);if(!operation||!valid_bls_change(candidate,candidate.timed.view.finalized.root,*operation,rules_,check)){it=changes->erase(it);continue;}it->second.admission_root=candidate.timed.view.finalized.root;}
   ++it;
  }
 }
 if(!exits)exits=exits_;
 if(!exits->empty()){
  const auto root=candidate.timed.view.finalized.root;const auto finalized_pin=candidate.blocks.at(root).lease(check);const auto& finalized=*finalized_pin;const auto& t=rules_.transition;
  const auto context=capella_fork_block_context(finalized.state,finalized.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state,check);
  for(auto it=exits->begin();it!=exits->end();){if(check)check();const auto operation=decode_consensus_voluntary_exit_ssz(it->second.wire,check);if(!operation)throw std::logic_error("Invalid retained Capella exit");
   const auto member=context.registry.validators.find(operation->validator_index);
   // Only finalized membership or scheduling retires an operation. A head reorg does not.
   if(member!=context.registry.validators.end()&&(member->second.public_key!=it->second.public_key||member->second.exit_epoch!=UINT64_MAX)){it=exits->erase(it);continue;}
   if(!candidate.blocks.contains(it->second.admission_root)){const auto key=exit_key(candidate,root,*operation,rules_,check);if(!key||*key!=it->second.public_key){it=exits->erase(it);continue;}it->second.admission_root=root;}
   ++it;
  }
 }
 if(!proposers)proposers=proposer_slashings_;
 if(!proposers->empty()){
  const auto root=candidate.timed.view.finalized.root;const auto finalized=candidate.blocks.at(root).lease(check);const auto& t=rules_.transition;
  const auto context=capella_fork_block_context(finalized->state,finalized->header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state,check);
  for(auto it=proposers->begin();it!=proposers->end();){if(check)check();const auto proof=decode_consensus_proposer_slashing_ssz(it->second.wire,check);if(!proof)throw std::logic_error("Invalid retained Capella proposer slashing");
   const auto member=context.registry.validators.find(proof->first.message.proposer_index);
   // An unfinalized head inclusion/reindex does not erase evidence. Finalized
   // slashing, withdrawal expiry or replacement identity can retire it. Missing membership retains
   // its independently authenticated admission root while that root is retained.
   if(member!=context.registry.validators.end()&&(member->second.public_key!=it->second.public_key||member->second.slashed||context.registry.epoch>=member->second.withdrawable_epoch)){it=proposers->erase(it);continue;}
   if(!candidate.blocks.contains(it->second.admission_root)){
    const auto key=proposer_slashing_key(candidate,root,context.registry.epoch,proof->first,proof->second,rules_,limits_,check);
    // Pruning must not silently lose evidence whose identity is not decidable.
    if(!key||*key!=it->second.public_key)throw std::invalid_argument("Capella proposer evidence cannot be safely reanchored");
    it->second.admission_root=root;it->second.admission_epoch=context.registry.epoch;
   }
   ++it;
  }
 }
 auto next=std::make_shared<const CapellaForkStore>(std::move(candidate));
 const auto previous=token_?disk_.fork_checkpoint(chain_,genesis_,limits_.envelope_bytes,token_):std::nullopt;
 if(!cold&&previous)cold=cold_options(previous->snapshot,limits_);
 if(cold)archive_require(cold->cache_records&&cold->cache_records<=limits_.blocks&&cold->cache_bytes&&cold->cache_bytes<=limits_.envelope_bytes);
 std::vector<CheckpointBlob> staged;size_t staged_bytes=0;
 const std::function<void(const CapellaColdReference&,std::string_view)> stage=[&](const auto& ref,std::string_view row){if(ref.bytes>limits_.envelope_bytes-staged_bytes)throw std::length_error("Capella cold blob batch budget");staged_bytes+=ref.bytes;staged.push_back({ref.digest,std::string(row)});};
 auto reference_next=reference_cache_;
 const auto flat=durable_snapshot(*next,votes?*votes:votes_,slashings?*slashings:slashings_,*changes,*exits,rules_,limits_,check,cold?nullptr:&cache_,cold?&stage:nullptr,cold?&reference_next:nullptr,*proposers);
 const bool fragmented=!cold&&(force_fragments||(previous&&previous->snapshot.starts_with("{\"expandedBytes\":")));
 auto fragments=cold?capella_checkpoint_detail::FragmentCheckpoint{cold_envelope(flat,*cold),std::move(staged)}:fragmented?capella_checkpoint_detail::fragment_checkpoint(flat,limits_.envelope_bytes,limits_.blocks,check):capella_checkpoint_detail::FragmentCheckpoint{flat,{}};
 if(fragments.manifest.size()>limits_.envelope_bytes)throw std::length_error("Capella cold envelope budget");
 const auto& encoded=fragments.manifest;
 const auto head_pin=capella_fork_head(*next,rules_,check);const auto& head=*head_pin;
 const auto tip=disk_.latest();if(!tip)throw std::invalid_argument("Capella reactor requires persisted execution anchor");
 if(state_){
  if(tip->header.hash()!=capella_fork_head(*state_,rules_,check)->execution_block.header.hash()){fail_closed();throw std::runtime_error("Capella execution tip changed outside reactor");}
  const auto& old=state_->timed.view;const auto& view=next->timed.view;
  if(capella_network_anchor(*next)!=capella_network_anchor(*state_)||view.finalized.epoch<old.finalized.epoch||view.justified.epoch<old.justified.epoch||next->timed.clock.genesis_time_ms!=state_->timed.clock.genesis_time_ms||next->timed.clock.time_ms<state_->timed.clock.time_ms)throw std::invalid_argument("Capella checkpoint regression");
  auto at=consensus_beacon_header_root(head.header);const auto final_slot=state_->blocks.at(old.finalized.root).header().slot;
  while(next->blocks.at(at).header().slot>final_slot){if(check)check();at=next->blocks.at(at).header().parent_root;}
  if(at!=old.finalized.root||(view.finalized.epoch==old.finalized.epoch&&view.finalized.root!=old.finalized.root))throw std::invalid_argument("Capella finalized branch changed");
 }
 // An anchor change must be the qualified transformation of the current store,
 // and the exact durable envelope must remain fully restart-verifiable before I/O.
 if(state_&&next->timed.view.anchor!=state_->timed.view.anchor){
  const auto expected=prepare_capella_pruned_store(*state_,next->timed.view.anchor,rules_,check);
  const std::function<void(const CapellaColdReference&,std::string_view)> compare_only=[](const auto&,auto){};
  const auto expected_snapshot=cold?capella_store_reference_snapshot(expected,rules_,limits_,compare_only,check):capella_store_snapshot(expected,rules_,limits_,check);
  const auto next_snapshot=cold?capella_store_reference_snapshot(*next,rules_,limits_,compare_only,check):capella_store_snapshot(*next,rules_,limits_,check);
  if(expected_snapshot!=next_snapshot)throw std::invalid_argument("Capella pruning changed live state");
  if(!cold)(void)restore_durable(flat,sha256(Bytes(flat.begin(),flat.end())),rules_,limits_,check);
 }
 const auto digest=sha256(Bytes(encoded.begin(),encoded.end()));
 if(token_&&digest==token_->digest){if(check)check();try{if(witness_)archive_require(bool(witness_->fork_checkpoint(chain_,genesis_,witness_limit(),witness_token_)));if(!disk_.fork_checkpoint(chain_,genesis_,limits_.envelope_bytes,token_))throw std::runtime_error("missing Capella checkpoint");}catch(...){fail_closed();throw;}if(cold)reference_cache_=std::move(reference_next);return;}
 std::vector<ExecutionCheckpointBlock> suffix;auto at=consensus_beacon_header_root(head.header);Int common_height;Bytes common_hash;
 for(;;){
  if(check)check();const auto record_pin=next->blocks.at(at).lease(check);const auto& record=*record_pin;const auto& h=record.execution_block.header;const auto existing=disk_.block_by_number(h.number);
  if(existing&&existing->header.hash()==h.hash()){common_height=h.number;common_hash=h.hash();break;}
  if(suffix.size()>=limits_.blocks)throw std::length_error("Capella publication suffix budget");suffix.push_back({record.execution_block,record.execution_state,record.receipts});
  if(at==next->timed.view.anchor){if(h.number<=0)throw std::invalid_argument("Capella execution anchor missing");const auto parent=disk_.block_by_number(h.number-1);if(!parent||parent->header.hash()!=h.parent_hash)throw std::invalid_argument("Capella execution anchor parent mismatch");common_height=h.number-1;common_hash=h.parent_hash;break;}
  at=record.header.parent_root;
 }
 std::reverse(suffix.begin(),suffix.end());if(check)check();
 // Allocate the coherent read view before I/O; no allocation after commit.
 if(token_&&token_->sequence==UINT64_MAX)throw std::overflow_error("Capella publication sequence exhausted");
 auto published_next=next;
 if(cold){
  const auto reader=checkpoint_reader(disk_);
  const auto pending=[&](const Bytes& digest,size_t bytes)->std::optional<std::string>{for(const auto& blob:fragments.blobs)if(blob.digest==digest){archive_require(blob.payload.size()<=bytes);return blob.payload;}return reader(digest,bytes);};
  // Verify every reference and operation before I/O, then prepare the durable
  // read view with the lifetime-guarded PRIMARY reader, not the staging closure.
  (void)restore_durable(encoded,sha256(Bytes(encoded.begin(),encoded.end())),rules_,limits_,check,pending,std::nullopt,next.get(),&reference_next);
  auto prepared=*next;auto cache=std::make_shared<CapellaColdRecordCache>(rules_,limits_,cold->cache_records,cold->cache_bytes);
  const auto payload=Json::parse(encoded).at("checkpoint");const auto& reference_store=payload.at("format")=="nano-capella-reference-store-v1"?payload:payload.at("store");
  for(const auto& ref:reference_store.at("snapshot").at("blocks")){
   const auto root_bytes=unhex(ref[0].get<std::string>());const auto id=archive_array<32>(root_bytes,0);auto& entry=prepared.blocks.at(id);
   entry=CapellaForkRecord(CapellaColdReference{id,unhex(ref[1].get<std::string>()),size_t(u64(quantity(ref[2]))),id==prepared.timed.view.anchor},entry.header(),entry.execution_header(),cache,reader);
  }
  published_next=std::make_shared<const CapellaForkStore>(std::move(prepared));
 }
 const auto publication=std::make_shared<const CapellaPublishedStore>(published_store(published_next,{token_?token_->sequence+1:1,digest},votes?*votes:votes_,*slashings,*proposers,check));
 std::optional<ExecutionStateRetentionAnchor> retention;
 if(historical_state_pruning_){const auto& anchor=next->blocks.at(next->timed.view.anchor).execution_header();if(anchor.number<=common_height)retention=ExecutionStateRetentionAnchor{anchor.number,anchor.hash()};}
 // Resolve cancellable cold-record reads before the first durable write.
 // Once witness intent is persisted, finish the primary commit without caller cancellation.
 const auto finalized_block=next->blocks.at(next->timed.view.finalized.root).lease(check)->execution_block;
 if(check)check();
 ForkCheckpointToken committed;
 try{
  witness_prepare(encoded,archive,fragments.blobs);
  if(execution_)committed=execution_->commit_beacon_checkpoint_with_blobs(token_,tip->header.hash(),common_height,common_hash,suffix,finalized_block,encoded,limits_.envelope_bytes,limits_.blocks,archive,retention,fragments.blobs);
  else committed=disk_.commit_execution_checkpoint_with_blobs(chain_,genesis_,token_,tip->header.hash(),common_height,common_hash,suffix,encoded,limits_.envelope_bytes,limits_.blocks,fragments.blobs,limits_.blocks,archive,retention);
  if(committed!=publication->token)throw std::runtime_error("Capella durable publication token mismatch");
 }
 catch(...){fail_closed();throw;}
 // No allocation or callback after durable success and before publication.
 token_=std::move(committed);state_=std::move(published_next);if(cold)reference_cache_=std::move(reference_next);if(cold)cache_=CapellaStoreSnapshotCache{};if(votes)votes_=std::move(*votes);if(slashings)slashings_=std::move(*slashings);bls_changes_=std::move(*changes);exits_=std::move(*exits);proposer_slashings_=std::move(*proposers);
 std::atomic_store_explicit(&beacon_publication_,publication,std::memory_order_release);
}
CapellaDurableReactor::CapellaDurableReactor(ChainStore& disk,Int chain,Bytes genesis,CapellaForkBlockRules rules,CapellaStoreSnapshotLimits limits,CapellaForkStore initial,const std::function<void()>& check,Blockchain* owner,ChainStore* witness):disk_(disk),execution_(owner),chain_(std::move(chain)),genesis_(std::move(genesis)),rules_(std::move(rules)),limits_(limits),mutex_(disk.altair_reactor_mutex_){std::lock_guard lock(mutex_);validate_execution_binding();if(witness){archive_require(witness!=&disk_);witness_=witness;witness_->bind_chain(chain_,genesis_,"nano-capella-recovery-v1");archive_require(!witness_->fork_checkpoint(chain_,genesis_,witness_limit())&&!disk_.fork_checkpoint(chain_,genesis_,limits_.envelope_bytes));}publish(std::move(initial),check);}
CapellaDurableReactor::CapellaDurableReactor(ChainStore& disk,Int chain,Bytes genesis,CapellaForkBlockRules rules,CapellaStoreSnapshotLimits limits,const ForkCheckpointToken& expected,const std::function<void()>& check,Blockchain* owner):disk_(disk),execution_(owner),chain_(std::move(chain)),genesis_(std::move(genesis)),rules_(std::move(rules)),limits_(limits),mutex_(disk.altair_reactor_mutex_){
 std::lock_guard lock(mutex_);validate_execution_binding();
 if(chain_!=rules_.transition.chain_id||genesis_.size()!=32)throw std::invalid_argument("Capella reactor chain/profile mismatch");
 const auto record=disk_.fork_checkpoint(chain_,genesis_,limits_.envelope_bytes,expected);if(!record)throw std::invalid_argument("Capella restart checkpoint missing");
 CapellaReferenceSnapshotCache startup_reference;auto [restored,votes,slashings,changes,exits,proposers]=restore_durable(record->snapshot,expected.digest,rules_,limits_,check,checkpoint_reader(disk_),std::nullopt,nullptr,&startup_reference);const auto pruning=disk_.state_pruning();if(pruning&&pruning->anchor.number>restored.blocks.at(restored.timed.view.anchor).execution_header().number)throw std::invalid_argument("pruning boundary exceeds authenticated Capella anchor");const auto head_pin=capella_fork_head(restored,rules_,check);const auto& head=*head_pin;const auto tip=disk_.latest();const auto state=disk_.state_at(head.execution_block.header.number);
 if(!tip||!state||tip->header.hash()!=head.execution_block.header.hash()||state->root()!=head.execution_state.root())throw std::invalid_argument("Capella restart execution mismatch");
 for(const auto& receipt:head.receipts){if(check)check();const auto saved=disk_.receipt(receipt.transaction_hash);archive_require(saved&&saved->to_json()==receipt.to_json());}
 auto finalized=restored.blocks.at(restored.timed.view.finalized.root).lease(check)->execution_block;const auto canonical=disk_.block_by_number(finalized.header.number);archive_require(canonical&&canonical->header.hash()==finalized.header.hash());
 if(check)check();state_=std::make_shared<const CapellaForkStore>(std::move(restored));token_=expected;reference_cache_=std::move(startup_reference);votes_=std::move(votes);slashings_=std::move(slashings);bls_changes_=std::move(changes);exits_=std::move(exits);proposer_slashings_=std::move(proposers);
 std::atomic_store_explicit(&beacon_publication_,std::make_shared<const CapellaPublishedStore>(published_store(state_,*token_,votes_,slashings_,proposer_slashings_,check)),std::memory_order_release);
 if(execution_){std::lock_guard chain_lock(execution_->mutex_);execution_->beacon_finalized_=std::move(finalized);execution_->beacon_ready_=true;}
}
CapellaDurableReactor::CapellaDurableReactor(Blockchain& owner,CapellaForkBlockRules rules,CapellaStoreSnapshotLimits limits,CapellaForkStore initial,const std::function<void()>& check,ChainStore* witness):CapellaDurableReactor(owner.store_,owner.genesis_.chain_id,owner.genesis_.block.header.hash(),std::move(rules),limits,std::move(initial),check,&owner,witness){}
CapellaDurableReactor::CapellaDurableReactor(Blockchain& owner,CapellaForkBlockRules rules,CapellaStoreSnapshotLimits limits,const ForkCheckpointToken& expected,const std::function<void()>& check):CapellaDurableReactor(owner.store_,owner.genesis_.chain_id,owner.genesis_.block.header.hash(),std::move(rules),limits,expected,check,&owner){}
CapellaDurableReactor::CapellaDurableReactor(Blockchain& owner,CapellaForkBlockRules rules,CapellaStoreSnapshotLimits limits,ChainStore& witness,const std::function<void()>& check,std::optional<ForkCheckpointToken> recovery_commitment)
 :CapellaDurableReactor(owner,std::move(rules),limits,witness,check,std::move(recovery_commitment),nullptr,std::nullopt,BootstrapArchiveRecoveryTag{}){}
CapellaDurableReactor::CapellaDurableReactor(Blockchain& owner,CapellaForkBlockRules rules,CapellaStoreSnapshotLimits limits,ChainStore& witness,
 CapellaSignedBootstrapArchive proof,const ForkCheckpointToken& expected_next,const ForkCheckpointToken& expected_witness,const std::function<void()>& check)
 :CapellaDurableReactor(owner,std::move(rules),limits,witness,check,expected_next,&proof,expected_witness,BootstrapArchiveRecoveryTag{}){}
CapellaDurableReactor::CapellaDurableReactor(Blockchain& owner,CapellaForkBlockRules rules,CapellaStoreSnapshotLimits limits,ChainStore& witness,const std::function<void()>& check,std::optional<ForkCheckpointToken> recovery_commitment,const CapellaSignedBootstrapArchive* bootstrap,std::optional<ForkCheckpointToken> expected_witness,BootstrapArchiveRecoveryTag):disk_(owner.store_),execution_(&owner),chain_(owner.genesis_.chain_id),genesis_(owner.genesis_.block.header.hash()),rules_(std::move(rules)),limits_(limits),mutex_(disk_.altair_reactor_mutex_),witness_(&witness){
 std::lock_guard lock(mutex_);validate_execution_binding();archive_require(witness_!=&disk_);
 if(bootstrap){
  archive_require(expected_witness&&recovery_commitment&&bootstrap->chain_==chain_&&bootstrap->genesis_==genesis_&&bootstrap->profile_==capella_fork_profile(rules_));
  // An exact existing witness must exist before bind_chain can be reached.
  archive_require(bool(witness_->fork_checkpoint(chain_,genesis_,witness_limit(),expected_witness)));
 }
 witness_->bind_chain(chain_,genesis_,"nano-capella-recovery-v1");
 const auto saved=witness_->fork_checkpoint(chain_,genesis_,witness_limit());archive_require(bool(saved));const auto intent=Json::parse(saved->snapshot);
 archive_require(intent.is_object()&&intent.size()==5&&intent.at("format")== (bootstrap?"nano-capella-bootstrap-archive-recovery-v1":"nano-capella-recovery-v1")&&intent.dump()==saved->snapshot);
 const auto previous=recovery_token_parse(intent.at("previous")),next=recovery_token_parse(intent.at("next"));archive_require(next&&(!previous||previous->sequence<UINT64_MAX)&&next->sequence==(previous?previous->sequence+1:1));
 if(recovery_commitment&&next!=recovery_commitment)throw std::invalid_argument("Capella recovery commitment differs from witness intent");
 const auto encoded=intent.at("snapshot").get<std::string>();const bool fragmented=encoded.starts_with("{\"expandedBytes\":");
 // Certificates are process-local and seeded only by full witnessed decoding.
 // Primary reuse below still reads and hashes every independently stored blob.
 CapellaReferenceSnapshotCache startup_reference;auto [restored,votes,slashings,changes,exits,proposers]=restore_durable(encoded,next->digest,rules_,limits_,check,checkpoint_reader(*witness_),std::nullopt,nullptr,&startup_reference);const auto cold=cold_options(encoded,limits_);const std::function<void(const CapellaColdReference&,std::string_view)> no_hot=[](const auto&,auto){throw std::logic_error("Unexpected hot witness row");};const auto payload=durable_snapshot(restored,votes,slashings,changes,exits,rules_,limits_,check,nullptr,cold?&no_hot:nullptr,cold?&startup_reference:nullptr,proposers);const auto flat=cold?cold_envelope(payload,*cold):payload;archive_require((fragmented?capella_checkpoint_detail::fragment_checkpoint(flat,limits_.envelope_bytes,limits_.blocks,check).manifest:flat)==encoded);
 const auto& entries=intent.at("archive");archive_require(entries.is_array()&&entries.size()<=1);std::vector<CheckpointArchiveEntry> archive;
 for(const auto& entry:entries){archive_require(entry.is_array()&&entry.size()==3&&entry[0].is_string()&&entry[1].is_string()&&entry[2].is_string());const auto root=entry[0].get<std::string>(),payload=entry[2].get<std::string>();archive_require(root.size()==66&&payload.size()>=2&&payload.size()%2==0&&(payload.size()-2)/2<=limits_.envelope_bytes);auto id=unhex(root),wire=unhex(payload);const auto slot=u64(quantity(entry[1]));archive_require(hex(id)==root&&hex(wire)==payload&&quantity(Int(std::to_string(slot)))==entry[1].get<std::string>());const auto key=archive_array<32>(id,0);verify_archived_block(key,wire,restored,rules_,check);archive_require(restored.blocks.at(key).header().slot==slot);archive.push_back({std::move(id),slot,std::move(wire)});}
 const auto primary=disk_.fork_checkpoint(chain_,genesis_,limits_.envelope_bytes);const auto actual=primary?std::optional<ForkCheckpointToken>(primary->token):std::nullopt;archive_require(actual==next||actual==previous);witness_token_=saved->token;
 const bool archive_only=previous&&previous->digest==next->digest&&!archive.empty();
 if(bootstrap){
  archive_require(bool(primary));bootstrap_archive_storage_binding(primary->token);
  archive_require(archive_only&&primary&&primary->snapshot==encoded&&bootstrap->matches(restored,rules_,check)&&
   archive.size()==1&&archive[0].root==bootstrap->entry().root&&archive[0].slot==1&&archive[0].payload==bootstrap->wire_&&saved->token==*expected_witness);
 }
 if(archive_only&&!bootstrap)throw std::invalid_argument("Capella bootstrap archive recovery requires explicit typed authority");
 const bool bootstrap_pending=bootstrap&&actual!=next;
 if(actual!=next&&!bootstrap){
  // Witness intent is authenticated and linked to the previous primary token.
  // Materialize only on this recovery path so all missing PRIMARY rows are
  // included in its existing atomic batch. Recovery peak memory remains open.
  if(cold)for(auto& [_,entry]:restored.blocks)entry.writable(check);
  token_=previous;publish_impl(std::move(restored),check,archive,std::move(votes),std::move(slashings),std::move(changes),std::move(exits),fragmented,cold,std::move(proposers));return;
 }
 if(cold){archive_require(primary&&primary->snapshot==encoded);std::tie(restored,votes,slashings,changes,exits,proposers)=restore_durable(primary->snapshot,next->digest,rules_,limits_,check,checkpoint_reader(disk_),std::nullopt,&restored,&startup_reference);}
 if(fragmented){archive_require(primary&&primary->snapshot==encoded);const auto expanded=capella_checkpoint_detail::expand_fragment_checkpoint(primary->snapshot,next->digest,limits_.envelope_bytes,limits_.blocks,checkpoint_reader(disk_),check);archive_require(expanded==flat);}
 for(const auto& entry:archive){
  if(bootstrap)archive_require(bootstrap_archive_exists(entry)!=bootstrap_pending);
  else{const auto persisted=disk_.checkpoint_archive(entry.root,limits_.envelope_bytes);archive_require(persisted&&persisted->slot==entry.slot&&persisted->payload==entry.payload);}
 }
 const auto pruning=disk_.state_pruning();if(pruning&&pruning->anchor.number>restored.blocks.at(restored.timed.view.anchor).execution_header().number)throw std::invalid_argument("pruning boundary exceeds authenticated Capella anchor");const auto head_pin=capella_fork_head(restored,rules_,check);const auto& head=*head_pin;const auto tip=disk_.latest();const auto state=disk_.state_at(head.execution_block.header.number);archive_require(chain_==rules_.transition.chain_id&&tip&&state&&tip->header.hash()==head.execution_block.header.hash()&&state->root()==head.execution_state.root());
 for(const auto& receipt:head.receipts){if(check)check();const auto persisted=disk_.receipt(receipt.transaction_hash);archive_require(persisted&&persisted->to_json()==receipt.to_json());}
 auto finalized=restored.blocks.at(restored.timed.view.finalized.root).lease(check)->execution_block;const auto canonical=disk_.block_by_number(finalized.header.number);archive_require(canonical&&canonical->header.hash()==finalized.header.hash());
 if(check)check();state_=std::make_shared<const CapellaForkStore>(std::move(restored));token_=bootstrap_pending?*previous:*next;reference_cache_=std::move(startup_reference);votes_=std::move(votes);slashings_=std::move(slashings);bls_changes_=std::move(changes);exits_=std::move(exits);proposer_slashings_=std::move(proposers);
 if(bootstrap_pending){
  // The same raw envelope was fully authenticated from WITNESS and, for cold
  // or fragmented stores, independently from PRIMARY. No blob repair, pruning,
  // operation retirement or witness rewrite is authorized by this path.
  publish_bootstrap_archive_locked(*bootstrap,encoded,*previous,*next,*expected_witness,*expected_witness,true,check);return;
 }
 std::atomic_store_explicit(&beacon_publication_,std::make_shared<const CapellaPublishedStore>(published_store(state_,*token_,votes_,slashings_,proposer_slashings_,check)),std::memory_order_release);
 std::lock_guard chain_lock(owner.mutex_);owner.beacon_finalized_=std::move(finalized);owner.beacon_ready_=true;
}
CapellaDurableReactor::CapellaDurableReactor(Blockchain& owner,CapellaForkBlockRules rules,CapellaStoreSnapshotLimits limits,CapellaForkStore initial,
 CapellaSignedBootstrapArchive proof,ChainStore& witness,const std::function<void()>& check)
 :disk_(owner.store_),execution_(&owner),chain_(owner.genesis_.chain_id),genesis_(owner.genesis_.block.header.hash()),rules_(std::move(rules)),limits_(limits),mutex_(disk_.altair_reactor_mutex_),witness_(&witness){
 std::lock_guard lock(mutex_);validate_execution_binding();
 archive_require(witness_!=&disk_&&proof.chain_==chain_&&proof.genesis_==genesis_&&proof.matches(initial,rules_,check));
 archive_require(!disk_.fork_checkpoint(chain_,genesis_,limits_.envelope_bytes));
 const auto tip=disk_.latest();archive_require(tip&&tip->header.number==0&&tip->header.hash()==genesis_&&disk_.count()==1);
 {std::lock_guard witness_lock(witness_->mutex_);archive_require(!witness_->get("fork-checkpoint/v1/token")&&!witness_->get("fork-checkpoint/v1/payload"));}
 archive_require(!bootstrap_archive_exists(proof.entry()));
 witness_->bind_chain(chain_,genesis_,"nano-capella-recovery-v1");
 // The first atomic PRIMARY publication includes the already signed entry.
 publish(std::move(initial),check,{proof.entry()});
}
void CapellaDurableReactor::bootstrap_archive_storage_binding(const ForkCheckpointToken& primary)const{
 const auto tip=disk_.latest();archive_require(bool(tip));
 {std::lock_guard disk_lock(disk_.mutex_);
  const auto identity=disk_.read("meta/identity"),linked=disk_.read("execution-checkpoint/v1");
  archive_require(identity&&identity->at("chainId")==quantity(chain_)&&identity->at("genesisHash")==hex(genesis_)&&identity->at("consensus")=="beacon"&&
   linked&&*linked==Json{{"digest",hex(primary.digest)},{"tip",hex(tip->header.hash())}}&&!disk_.get("meta/certified-tip"));
 }
 {std::lock_guard witness_lock(witness_->mutex_);const auto identity=witness_->read("meta/identity");
  archive_require(identity&&*identity==Json{{"chainId",quantity(chain_)},{"genesisHash",hex(genesis_)},{"consensus","nano-capella-recovery-v1"}});
 }
}
bool CapellaDurableReactor::bootstrap_archive_exists(const CheckpointArchiveEntry& entry)const{
 const auto existing=disk_.checkpoint_archive(entry.root,limits_.envelope_bytes);
 if(existing)archive_require(existing->slot==entry.slot&&existing->payload==entry.payload);
 // The ordinary storage API authenticates the immutable payload but does not
 // expose the secondary slot-index. Explicit maintenance refuses inconsistency;
 // it never silently reports a missing index as a completed archive operation.
 std::lock_guard disk_lock(disk_.mutex_);
 const auto index=disk_.get("checkpoint-archive-slot/v1/"+hex(bytes(Int(std::to_string(entry.slot)),8))+"/"+hex(entry.root));
 archive_require(existing?index&&index->empty():!index);
 return bool(existing);
}
void CapellaDurableReactor::bootstrap_archive_batch_budget(std::string_view snapshot,const CheckpointArchiveEntry& entry)const{
 // This empty-suffix/no-retention batch has eight Put records. Reserve 4 KiB
 // for all fixed keys, token/tip encodings, length prefixes and the batch header;
 // include variable canonical difficulty/count bytes from the current store.
 // A conservative refusal is preferable to writing an intent that cannot fit.
 size_t required=4096;
 auto add=[&](size_t bytes){if(required>limits_.envelope_bytes||bytes>limits_.envelope_bytes-required)throw std::length_error("Capella bootstrap archive batch budget");required+=bytes;};
 add(snapshot.size());
 const auto encoded=Json{{"slot",quantity(Int(std::to_string(entry.slot)))},{"payload",hex(entry.payload)},{"digest",hex(sha256(entry.payload))}}.dump();add(encoded.size());
 std::lock_guard disk_lock(disk_.mutex_);const auto difficulty=disk_.get("meta/difficulty"),count=disk_.get("meta/count");
 archive_require(difficulty&&count);add(difficulty->size());add(count->size());
}
CapellaBootstrapArchivePlan CapellaDurableReactor::prepare_bootstrap_archive_locked(CapellaSignedBootstrapArchive proof,
 ForkCheckpointToken primary_pin,ForkCheckpointToken witness_pin,const std::function<void()>& check)const{
 require_available();archive_require(execution_);validate_execution_binding();
 archive_require(witness_&&witness_!=&disk_&&token_==primary_pin&&witness_token_==witness_pin&&
  proof.chain_==chain_&&proof.genesis_==genesis_&&proof.matches(*state_,rules_,check));
 const auto primary=disk_.fork_checkpoint(chain_,genesis_,limits_.envelope_bytes,primary_pin);
 const auto witness=witness_->fork_checkpoint(chain_,genesis_,witness_limit(),witness_pin);archive_require(primary&&witness);
 bootstrap_archive_storage_binding(primary_pin);
 const auto prior=Json::parse(witness->snapshot);const auto entry=proof.entry();
 archive_require(prior.is_object()&&prior.size()==5&&prior.dump()==witness->snapshot&&
  recovery_token_parse(prior.at("next"))==primary_pin&&prior.at("snapshot").get<std::string>()==primary->snapshot);
 const auto previous=recovery_token_parse(prior.at("previous"));
 archive_require((!previous||previous->sequence<UINT64_MAX)&&primary_pin.sequence==(previous?previous->sequence+1:1));
 const auto format=prior.at("format").get<std::string>();const auto& prior_archive=prior.at("archive");
 archive_require(prior_archive.is_array()&&prior_archive.size()<=1);
 if(format=="nano-capella-bootstrap-archive-recovery-v1"){
  Json typed_archive=Json::array();typed_archive.push_back(Json::array({hex(entry.root),quantity(Int(1)),hex(entry.payload)}));
  archive_require(previous&&previous->digest==primary_pin.digest&&prior_archive==typed_archive);
 }else archive_require(format=="nano-capella-recovery-v1");
 if(bootstrap_archive_exists(entry)){
  if(check)check();return {std::move(proof),primary->snapshot,primary_pin,witness_pin,primary_pin,witness_pin,false};
 }
 bootstrap_archive_batch_budget(primary->snapshot,entry);
 if(primary_pin.sequence==UINT64_MAX||witness_pin.sequence==UINT64_MAX)throw std::overflow_error("Capella bootstrap commitment sequence exhausted");
 const ForkCheckpointToken next_primary{primary_pin.sequence+1,primary_pin.digest};
 Json entries=Json::array();entries.push_back(Json::array({hex(entry.root),quantity(Int(1)),hex(entry.payload)}));
 const auto intent=Json{{"format","nano-capella-bootstrap-archive-recovery-v1"},{"previous",recovery_token_json(primary_pin)},{"next",recovery_token_json(next_primary)},{"snapshot",primary->snapshot},{"archive",entries}}.dump();
 archive_require(intent.size()<=witness_limit());
 const ForkCheckpointToken next_witness{witness_pin.sequence+1,sha256(Bytes(intent.begin(),intent.end()))};
 if(check)check();return {std::move(proof),primary->snapshot,primary_pin,witness_pin,next_primary,next_witness,true};
}
CapellaBootstrapArchivePlan CapellaDurableReactor::prepare_bootstrap_archive(CapellaSignedBootstrapArchive proof,
 const ForkCheckpointToken& primary,const ForkCheckpointToken& witness,const std::function<void()>& check)const{
 // Capture caller-owned commitments before the first cancellable callback.
 const auto primary_pin=primary,witness_pin=witness;
 std::lock_guard lock(mutex_);return prepare_bootstrap_archive_locked(std::move(proof),primary_pin,witness_pin,check);
}
void CapellaDurableReactor::publish_bootstrap_archive_locked(const CapellaSignedBootstrapArchive& proof,std::string_view snapshot,
 const ForkCheckpointToken& previous_primary,const ForkCheckpointToken& next_primary,const ForkCheckpointToken& current_witness,
 const ForkCheckpointToken& next_witness,bool reuse_witness,const std::function<void()>& check){
 require_available();archive_require(execution_);validate_execution_binding();
 archive_require(witness_&&witness_!=&disk_&&token_==previous_primary&&witness_token_==current_witness&&
  previous_primary.sequence<UINT64_MAX&&next_primary.sequence==previous_primary.sequence+1&&next_primary.digest==previous_primary.digest&&
  proof.chain_==chain_&&proof.genesis_==genesis_&&proof.matches(*state_,rules_,check));
 const auto primary=disk_.fork_checkpoint(chain_,genesis_,limits_.envelope_bytes,previous_primary);
 const auto held=witness_->fork_checkpoint(chain_,genesis_,witness_limit(),current_witness);
 archive_require(primary&&held&&primary->snapshot==snapshot&&sha256(Bytes(snapshot.begin(),snapshot.end()))==previous_primary.digest);
 bootstrap_archive_storage_binding(previous_primary);
 const auto entry=proof.entry();archive_require(!bootstrap_archive_exists(entry));bootstrap_archive_batch_budget(snapshot,entry);
 Json entries=Json::array();entries.push_back(Json::array({hex(entry.root),quantity(Int(1)),hex(entry.payload)}));
 const auto intent=Json{{"format","nano-capella-bootstrap-archive-recovery-v1"},{"previous",recovery_token_json(previous_primary)},{"next",recovery_token_json(next_primary)},{"snapshot",snapshot},{"archive",entries}}.dump();
 archive_require(intent.size()<=witness_limit());
 if(reuse_witness)archive_require(current_witness==next_witness&&held->snapshot==intent);
 else archive_require(current_witness.sequence<UINT64_MAX&&next_witness.sequence==current_witness.sequence+1&&next_witness.digest==sha256(Bytes(intent.begin(),intent.end())));
 const auto head_pin=capella_fork_head(*state_,rules_,check);const auto& head=*head_pin;
 const auto tip=disk_.latest();const auto persisted=disk_.state_at(head.execution_block.header.number);
 archive_require(tip&&persisted&&tip->header.hash()==head.execution_block.header.hash()&&persisted->root()==head.execution_state.root());
 for(const auto& receipt:head.receipts){if(check)check();const auto saved=disk_.receipt(receipt.transaction_hash);archive_require(saved&&saved->to_json()==receipt.to_json());}
 const auto finalized=state_->blocks.at(state_->timed.view.finalized.root).lease(check)->execution_block;
 const std::vector<CheckpointArchiveEntry> archive{entry};
 const std::vector<ExecutionCheckpointBlock> no_suffix;
 // Allocate the same-state read view before the first write. The exact raw
 // envelope and all volatile operations, caches, duties and frames are retained.
 const auto publication=std::make_shared<const CapellaPublishedStore>(published_store(state_,next_primary,votes_,slashings_,proposer_slashings_,check));
 if(check)check();ForkCheckpointToken committed;
 try{
  if(!reuse_witness)witness_token_=witness_->commit_fork_checkpoint(chain_,genesis_,current_witness,intent,witness_limit());
  archive_require(witness_token_==next_witness);
  committed=execution_->commit_beacon_checkpoint_with_blobs(previous_primary,tip->header.hash(),head.execution_block.header.number,
   head.execution_block.header.hash(),no_suffix,finalized,snapshot,limits_.envelope_bytes,limits_.blocks,archive,std::nullopt,{});
  if(committed!=next_primary)throw std::runtime_error("Capella bootstrap publication token mismatch");
 }catch(...){fail_closed();throw;}
 // No allocation or callback between synchronous PRIMARY success and exposure.
 token_=std::move(committed);std::atomic_store_explicit(&beacon_publication_,publication,std::memory_order_release);
}
CapellaBootstrapArchiveCommitments CapellaDurableReactor::apply_bootstrap_archive(CapellaBootstrapArchivePlan plan,const std::function<void()>& check){
 std::lock_guard lock(mutex_);auto current=prepare_bootstrap_archive_locked(plan.proof_,plan.primary_before_,plan.witness_before_,check);
 archive_require(current.snapshot_==plan.snapshot_&&current.primary_after_==plan.primary_after_&&current.witness_after_==plan.witness_after_&&current.work_==plan.work_);
 if(!plan.work_)return {false,*token_,*witness_token_};
 publish_bootstrap_archive_locked(plan.proof_,plan.snapshot_,plan.primary_before_,plan.primary_after_,plan.witness_before_,plan.witness_after_,false,check);
 return {true,*token_,*witness_token_};
}
Json CapellaDurableReactor::network_identity()const{
 // Read an authenticated immutable publication without waiting for admission.
 // fail_closed clears this publication; the fallback retains the latched error.
 const auto published=beacon_snapshot();return network_identity_for_store(*published.state);
}
Json CapellaDurableReactor::network_identity_locked()const{
 require_available();return network_identity_for_store(*state_);
}
Json CapellaDurableReactor::network_identity_for_store(const CapellaForkStore& store)const{
 const auto anchor=capella_network_anchor(store);const auto encoded=Json::array({"nano-capella-network-v1",quantity(chain_),hex(genesis_),hex(Bytes(anchor.begin(),anchor.end())),capella_fork_profile(rules_)}).dump();const auto digest=sha256(Bytes(encoded.begin(),encoded.end()));
 return {{"chainId",quantity(chain_)},{"genesisHash",hex(genesis_)},{"consensusIdentity",hex(digest)},{"forkDigest",hex(Bytes(digest.begin(),digest.begin()+4))}};
}
std::vector<SignedConsensusVoluntaryExit> CapellaDurableReactor::pending_exits_locked(size_t maximum,const std::function<void()>& check)const{
 require_available();std::vector<SignedConsensusVoluntaryExit> result;if(exits_.empty())return result;result.reserve(std::min(maximum,exits_.size()));
 const auto head_pin=capella_fork_head(*state_,rules_,check);const auto& head=*head_pin;const auto& t=rules_.transition;const auto context=capella_fork_block_context(head.state,head.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state,check);
 for(const auto& [id,record]:exits_){if(check)check();if(result.size()>=maximum)break;const auto operation=decode_consensus_voluntary_exit_ssz(record.wire,check);if(!operation)throw std::logic_error("Invalid queued Capella exit");const auto member=context.registry.validators.find(operation->validator_index);if(member==context.registry.validators.end()||member->second.public_key!=record.public_key||member->second.exit_epoch!=UINT64_MAX)continue;result.push_back(*operation);}return result;
}
std::vector<SignedConsensusVoluntaryExit> CapellaDurableReactor::pending_exits()const{std::lock_guard lock(mutex_);return pending_exits_locked(limits_.messages,{});}
bool CapellaDurableReactor::voluntary_exit_ssz(std::span<const uint8_t> input,const std::function<void()>& callback){
 if(input.size()!=112)return false;
 const Bytes owned_wire(input.begin(),input.end());const std::span<const uint8_t> wire=owned_wire;const auto check=callback;
 const auto operation=decode_consensus_voluntary_exit_ssz(wire,check);if(!operation)return false;std::lock_guard lock(mutex_);require_available();if(check)check();
 const Bytes bytes(wire.begin(),wire.end());const auto id=sha256(bytes);if(exits_.contains(id))return exits_.at(id).wire==bytes;
 const auto head_pin=capella_fork_head(*state_,rules_,check);const auto& head=*head_pin;const auto root=consensus_beacon_header_root(head.header);const auto key=exit_key(*state_,root,*operation,rules_,check);if(!key)return false;
 const auto& t=rules_.transition;auto context=capella_fork_block_context(head.state,head.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state,check);
 const auto version=operation->epoch<context.fork.epoch?context.fork.previous_version:context.fork.current_version;
 if(!process_phase0_voluntary_exit(context.registry,t.consensus.exit,*operation,version,context.genesis_root))return false;
 for(const auto& [_,record]:exits_){if(check)check();const auto other=decode_consensus_voluntary_exit_ssz(record.wire,check);if(!other)throw std::logic_error("Invalid queued Capella exit");if(other->validator_index==operation->validator_index&&record.public_key==*key)return false;}
 if(exits_.size()>=limits_.messages)throw std::length_error("Capella exit capacity");auto exits=exits_;exits.emplace(id,CapellaPendingExit{root,*key,bytes});publish(*state_,check,{},std::nullopt,std::nullopt,std::nullopt,std::move(exits));return true;
}
std::vector<SignedCapellaBlsChange> CapellaDurableReactor::pending_bls_changes_locked(size_t maximum,const std::function<void()>& check)const{
 require_available();std::vector<SignedCapellaBlsChange> result;result.reserve(std::min(maximum,bls_changes_.size()));if(bls_changes_.empty())return result;
 const auto head_pin=capella_fork_head(*state_,rules_,check);const auto& head=*head_pin;const auto& t=rules_.transition;const auto context=capella_fork_block_context(head.state,head.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state,check);
 for(const auto& [index,record]:bls_changes_){if(result.size()>=maximum)break;if(check)check();const auto operation=decode_capella_bls_change(record.wire);if(!operation)throw std::logic_error("Invalid queued BLS change");const auto member=context.registry.validators.find(index);if(member==context.registry.validators.end()||member->second.withdrawal_credentials[0]!=0)continue;const auto key_hash=sha256(Bytes(operation->from_bls_pubkey.begin(),operation->from_bls_pubkey.end()));if(!std::equal(key_hash.begin()+1,key_hash.end(),member->second.withdrawal_credentials.begin()+1))continue;result.push_back(*operation);}return result;
}
std::vector<SignedCapellaBlsChange> CapellaDurableReactor::pending_bls_changes()const{std::lock_guard lock(mutex_);return pending_bls_changes_locked(limits_.messages,{});}
bool CapellaDurableReactor::bls_change_ssz(std::span<const uint8_t> input,const std::function<void()>& callback){
 if(input.size()!=172)return false;
 const Bytes owned_wire(input.begin(),input.end());const std::span<const uint8_t> wire=owned_wire;const auto check=callback;
 const auto operation=decode_capella_bls_change(wire);if(!operation)return false;std::lock_guard lock(mutex_);require_available();if(check)check();
 if(const auto it=bls_changes_.find(operation->validator_index);it!=bls_changes_.end())return it->second.wire==Bytes(wire.begin(),wire.end());
 const auto root=consensus_beacon_header_root(capella_fork_head(*state_,rules_,check)->header);if(!valid_bls_change(*state_,root,*operation,rules_,check))return false;
 if(bls_changes_.size()>=limits_.messages)throw std::length_error("Capella BLS change queue capacity");auto changes=bls_changes_;changes.emplace(operation->validator_index,CapellaPendingBlsChange{root,Bytes(wire.begin(),wire.end())});publish(*state_,check,{},std::nullopt,std::nullopt,std::move(changes));return true;
}
bool CapellaDurableReactor::vote_ssz(std::span<const uint8_t> wire,const std::function<void()>& callback){const auto check=callback;const auto decoded=decode_consensus_attestation_ssz(wire,rules_.transition.consensus.body.validators_per_committee,check);return decoded&&vote(*decoded,check);}
bool CapellaDurableReactor::slashing_ssz(std::span<const uint8_t> wire,const std::function<void()>& callback){const auto check=callback;const auto decoded=decode_consensus_attester_slashing_ssz(wire,rules_.transition.consensus.body.validators_per_committee,check);return decoded&&slashing(decoded->first,decoded->second,check);}
Json CapellaDurableReactor::relay_head(const std::function<void()>& check)const{
 if(check)check();const auto published=beacon_snapshot();
 const auto head_pin=capella_fork_head(*published.state,rules_,check);const auto& head=*head_pin;
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
 archive_require(record->slot==found->second.header().slot);verify_archived_block(id,record->payload,*published.state,rules_,check);return record->payload;
}
CapellaPublishedStore CapellaDurableReactor::current()const{std::lock_guard lock(mutex_);require_available();return {state_,*token_};}
CapellaReferenceSnapshotCacheUsage CapellaDurableReactor::reference_snapshot_cache_usage()const{std::lock_guard lock(mutex_);require_available();return reference_cache_.usage();}
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
 auto next=continued_tick(*state_,time,rules_,limits_,check);auto votes=votes_;
 const auto previous=(state_->timed.clock.time_ms-state_->timed.clock.genesis_time_ms)/rules_.slot_duration_ms;
 const auto current=(time-state_->timed.clock.genesis_time_ms)/rules_.slot_duration_ms;const auto epoch=current/rules_.transition.consensus.epoch.history.slots_per_epoch;
 for(auto it=votes.begin();it!=votes.end();){
  if(check)check();const auto vote=decode_consensus_attestation_ssz(it->second,rules_.transition.consensus.body.validators_per_committee,check);if(!vote)throw std::logic_error("Capella stored vote encoding");
  if(current>previous&&vote->data.slot==previous){auto* prepared=prepare_vote_target_locked(next,*vote,true,check);if(prepared){auto applied=prepare_capella_prepared_store_vote(next,*vote,rules_,*prepared,check);if(applied)next=std::move(*applied);}}
  if(vote->data.target.epoch<(epoch?epoch-1:0))it=votes.erase(it);else ++it;
 }
 publish(std::move(next),check,{},std::move(votes));
 if(automatic_pruning_)prune_if_ready_locked(check);
}

bool CapellaDurableReactor::block(std::span<const uint8_t> wire,const std::function<void()>& check){
 return block_admission(wire,check)==CapellaBlockAdmission::accepted;
}
CapellaBlockAdmission CapellaDurableReactor::block_admission(std::span<const uint8_t> wire,const std::function<void()>& callback){
 const auto check=callback;
 std::lock_guard lock(mutex_);bool deferred=false;
 const bool accepted=block_locked(wire,check,&deferred);
 return accepted?CapellaBlockAdmission::accepted:deferred?CapellaBlockAdmission::deferred:CapellaBlockAdmission::rejected;
}
bool CapellaDurableReactor::block_locked(std::span<const uint8_t> input,const std::function<void()>& callback,bool* deferred){
 require_available();
 if(input.size()<404||input.size()>rules_.transition.consensus.maximum_block_bytes)return false;
 // The authenticated block and its durable archive must use this one owned wire.
 const Bytes owned_wire(input.begin(),input.end());const std::span<const uint8_t> wire=owned_wire;const auto check=callback;
 if(check)check();
 // Reconnection may lose the sender's ACK cache. A pruned historical duplicate
 // is acknowledged only from our OWN authenticated finalized archive; it never
 // installs a peer checkpoint or changes the current execution/fork state.
 if(wire.size()>=404&&wire.size()<=rules_.transition.consensus.maximum_block_bytes&&archive_number(wire,100)<state_->blocks.at(state_->timed.view.anchor).header().slot){
  std::exception_ptr interrupted;auto guard=[&]{try{if(check)check();}catch(...){interrupted=std::current_exception();throw;}};
  ConsensusBeaconHeader header;
  try{header=archived_header(wire,rules_,guard);}catch(const std::invalid_argument&){if(interrupted)std::rethrow_exception(interrupted);return false;}
  const auto id=consensus_beacon_header_root(header);
  if(!state_->blocks.contains(id)){const auto prior=signed_block_locked(id,check);return prior&&std::equal(wire.begin(),wire.end(),prior->begin(),prior->end());}
 }
 std::optional<CapellaForkStore> next;
 if(wire.size()<404||wire.size()>rules_.transition.consensus.maximum_block_bytes)return false;
 std::exception_ptr interrupted;auto guard=[&]{try{if(check)check();}catch(...){interrupted=std::current_exception();throw;}};
 ConsensusBeaconHeader header;
 try{header=archived_header(wire,rules_,guard);}catch(const std::invalid_argument&){if(interrupted)std::rethrow_exception(interrupted);return false;}
 const auto id=consensus_beacon_header_root(header);
 // A retained root is already admitted. Reprocessing its embedded votes can
 // require an obsolete target replay and must never replace archive bytes.
 if(state_->blocks.contains(id)){
  // The root is already authenticated and its archived bytes are immutable.
  // Keep the idempotent publication guards without reserializing every row.
  const auto& c=rules_.transition.consensus;
  const auto head=phase0_fork_choice_head(state_->timed.view,{c.epoch.ffg.slots_per_epoch,c.attestation.duties.effective_balance_increment,rules_.proposer_score_boost},check).head;
  if(check)check();
  try{
   const auto tip=disk_.latest();
   if(!tip||tip->header.hash()!=state_->blocks.at(head).execution_header().hash())throw std::runtime_error("Capella execution tip changed outside reactor");
   if(witness_)archive_require(bool(witness_->fork_checkpoint(chain_,genesis_,witness_limit(),witness_token_)));
   if(!disk_.fork_checkpoint(chain_,genesis_,limits_.envelope_bytes,token_))throw std::runtime_error("missing Capella checkpoint");
  }catch(...){fail_closed();throw;}
  if(check)check();return true;
 }else {
  // A full retained graph is temporary admission pressure, not corrupt state.
  // Duplicates above still succeed; votes/ticks can advance safe pruning.
  if(state_->blocks.size()>=limits_.blocks)return false;
  const auto& view=state_->timed.view;const auto& clock=state_->timed.clock;
  const auto slots=rules_.transition.consensus.epoch.ffg.slots_per_epoch;
  if(!slots||!limits_.replay_slots||!rules_.slot_duration_ms||clock.time_ms<clock.genesis_time_ms||view.finalized.epoch>UINT64_MAX/slots)throw std::invalid_argument("Invalid incoming Capella preparation bounds");
  if(!state_->blocks.contains(header.parent_root)||!view.nodes.contains(header.parent_root)||header.slot>(clock.time_ms-clock.genesis_time_ms)/rules_.slot_duration_ms||header.slot<=view.finalized.epoch*slots)return false;
  const auto parent_pin=state_->blocks.at(header.parent_root).lease(check);const auto& parent=*parent_pin;if(header.slot<=parent.header.slot)return false;
  auto ancestor=header.parent_root;const auto finalized_slot=view.finalized.epoch*slots;
  while(view.nodes.at(ancestor).slot>finalized_slot&&ancestor!=view.anchor){if(check)check();const auto& node=view.nodes.at(ancestor);if(view.nodes.at(node.parent).slot>=node.slot)throw std::invalid_argument("Invalid incoming Capella ancestry");ancestor=node.parent;}
  if(ancestor!=view.finalized.root)return false;
  std::erase_if(incoming_preparations_,[&](const auto& entry){
   const auto root=consensus_beacon_header_root(entry.second.parent());
   const auto retained=state_->blocks.find(root);
   return retained==state_->blocks.end()||retained->second.header()!=entry.second.parent();
  });
  auto incoming=std::find_if(incoming_preparations_.begin(),incoming_preparations_.end(),[&](const auto& entry){return entry.first==header.slot&&entry.second.parent()==parent.header;});
  if(incoming==incoming_preparations_.end()){
   const auto capacity=std::min(limits_.blocks,size_t(rules_.transition.consensus.body.attestations)+2);
   if(!capacity)throw std::invalid_argument("Invalid incoming preparation capacity");
   if(incoming_preparations_.size()>=capacity)incoming_preparations_.erase(incoming_preparations_.begin());
   incoming_preparations_.emplace_back(header.slot,CapellaBlockPreparation(parent.state,parent.header,rules_.transition,check));
  }else std::rotate(incoming,std::next(incoming),incoming_preparations_.end());
  auto& preparation=incoming_preparations_.back().second;
  if(local_preparation_&&local_preparation_->matches(*state_,rules_,header.slot,check)&&local_preparation_->ready()){
   const auto* local=local_preparation_->block();
   if(local&&local->parent()==parent.header&&local->prepared_slot()==header.slot-1)preparation=*local;
  }
  if(!preparation.advance_for_block(header.slot,std::min(limits_.replay_slots,slots),limits_.sync_trials,check)){if(deferred)*deferred=true;return false;}
  // Continuations bind the admitted target root/epoch/profile, not the wire
  // currently requesting them. Alternating siblings must not erase that work.
  incoming_target_block_=id;
  std::erase_if(incoming_targets_,[&](const auto& frame){return !frame.matches(*state_,rules_,frame.target(),check);});
  for(;;){
   std::vector<const CapellaTargetPreparation*> targets;
   for(const auto& frame:incoming_targets_)if(frame.ready())targets.push_back(&frame);
   try{
    next=prepare_capella_contextual_received_block(*state_,wire,rules_,preparation,targets,limits_.replay_slots,limits_.sync_trials,check);
    break;
   }catch(const CapellaTargetContextRequired& needed){
    auto found=std::find_if(incoming_targets_.begin(),incoming_targets_.end(),[&](const auto& frame){return frame.target()==needed.target();});
    if(found==incoming_targets_.end()){
     // Ephemeral cache pressure only evicts recomputable work, never an admitted
     // state, archive, signing reservation or durable checkpoint.
     if(incoming_targets_.size()>=size_t(rules_.transition.consensus.body.attestations)+2)incoming_targets_.erase(incoming_targets_.begin());
     incoming_targets_.emplace_back(*state_,rules_,needed.target(),check);
    }else std::rotate(found,std::next(found),incoming_targets_.end());
    auto& frame=incoming_targets_.back();bool reused=false;
    for(const auto& ready:vote_preparations_)if(ready&&ready->ready()&&ready->matches(*state_,rules_,needed.target(),check)){frame=*ready;reused=true;break;}
    // A completed authenticated context needs no more slot work. Rebuild the
    // pointer view after vector growth and finish admission in this same call.
    // Only genuinely unfinished preparation defers the signed block.
    if(reused)continue;
    frame.advance(std::min(limits_.replay_slots,slots),limits_.sync_trials,check);
    if(deferred)*deferred=true;
    return false;
   }
  }
 }
 if(!next)return false;
 std::vector<CheckpointArchiveEntry> archive;
 if(next->blocks.size()!=state_->blocks.size()){
  if(next->blocks.size()!=state_->blocks.size()+1)throw std::logic_error("Capella admission changed multiple blocks");
  for(const auto& [id,record]:next->blocks)if(!state_->blocks.contains(id))archive.push_back({Bytes(id.begin(),id.end()),record.header().slot,Bytes(wire.begin(),wire.end())});
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
  archive_require(record->slot==state_->blocks.at(id).header().slot);verify_archived_block(id,record->payload,*state_,rules_,check);return record->payload;
 }
 // Historical serving is anchored in the current authenticated FINALIZED
 // ancestry, not in a peer's hint or an arbitrary signed archive key. Registry
 // entries/public keys are append-only; the receiver still replays admission.
 const auto anchor_pin=state_->blocks.at(state_->timed.view.anchor).lease(check);const auto& anchor=*anchor_pin;
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

CapellaPublishedStore CapellaDurableReactor::published_store(std::shared_ptr<const CapellaForkStore> state,ForkCheckpointToken token,const CapellaSignedVotes& votes,const CapellaSignedVotes& slashings,const CapellaPendingProposerSlashings& proposers,const std::function<void()>& check)const{
 auto roots=[&](const CapellaSignedVotes& archive,const char* kind,size_t maximum){
  if(check)check();if(archive.size()>maximum)throw std::length_error("Capella published signed inventory exceeds capacity");
  auto snapshot=std::make_shared<CapellaInventorySnapshot>();snapshot->token_=token;snapshot->kind_=kind;snapshot->roots_.reserve(archive.size());
  for(const auto& [key,_]:archive){if(check)check();if(key.size()!=sizeof(ConsensusRoot))throw std::logic_error("Invalid Capella archive root");ConsensusRoot root{};std::copy(key.begin(),key.end(),root.begin());snapshot->roots_.push_back(root);}
  if(check)check();return snapshot;
 };
 // Construct before durable commit: cancellation/allocation cannot publish a
 // new token or a half-installed operation inventory. No unvalidated bytes.
 auto attestations=roots(votes,"attestation",limits_.messages);auto proofs=roots(slashings,"attester_slashing",limits_.equivocations);
 CapellaSignedVotes proposer_wires;for(const auto& [id,record]:proposers){if(check)check();proposer_wires.emplace(id,record.wire);}
 auto proposer_proofs=roots(proposer_wires,"proposer_slashing",limits_.equivocations);
 return {std::move(state),std::move(token),std::move(attestations),std::move(proofs),std::move(proposer_proofs)};
}
std::shared_ptr<const CapellaInventorySnapshot> CapellaDurableReactor::capture_published_inventory(const CapellaPublishedStore& published,std::string_view kind,size_t budget,const std::function<void()>& check)const{
 if(kind=="block")return capture_block_inventory(published,budget,check);
 if(check)check();const auto snapshot=kind=="attestation"?published.attestations:kind=="proposer_slashing"?published.proposer_slashings:published.slashings;
 if(!snapshot||snapshot->kind_!=kind||snapshot->token_!=published.token)throw std::logic_error("Capella signed inventory publication unavailable");
 if(snapshot->roots_.capacity()>budget/sizeof(ConsensusRoot))throw std::length_error("Capella inventory allocation exceeds root budget");
 if(check)check();return snapshot;
}
std::shared_ptr<const CapellaInventorySnapshot> CapellaDurableReactor::capture_block_inventory(const CapellaPublishedStore& published,size_t budget,const std::function<void()>& check)const{
 // A block inventory is a copy of roots from one durably published immutable
 // store. Its token, anchor and roots must all come from that same publication.
 // Ephemeral sync and state-filtered exit/change inventories retain their lock.
 if(check)check();const auto& store=*published.state;
 const auto count=store.blocks.size()-size_t(store.blocks.contains(store.timed.view.anchor));
 if(count>budget/sizeof(ConsensusRoot))throw std::length_error("Capella inventory snapshot exceeds root budget");
 auto snapshot=std::make_shared<CapellaInventorySnapshot>();snapshot->token_=published.token;snapshot->kind_="block";snapshot->roots_.reserve(count);
 for(const auto& [root,_]:store.blocks){if(check)check();if(root!=store.timed.view.anchor)snapshot->roots_.push_back(root);}
 if(snapshot->roots_.capacity()>budget/sizeof(ConsensusRoot))throw std::length_error("Capella inventory allocation exceeds root budget");
 if(check)check();return snapshot;
}

std::shared_ptr<const CapellaInventorySnapshot> CapellaDurableReactor::capture_inventory(std::string_view kind,size_t budget,const std::function<void()>& check)const{
 if(kind=="block"||kind=="attestation"||kind=="attester_slashing"||kind=="proposer_slashing")return capture_published_inventory(beacon_snapshot(),kind,budget,check);
 std::lock_guard lock(mutex_);return capture_inventory_locked(kind,budget,check);
}
std::shared_ptr<const CapellaInventorySnapshot> CapellaDurableReactor::try_capture_inventory(std::string_view kind,size_t budget,const std::function<void()>& check)const{
 if(kind=="block"||kind=="attestation"||kind=="attester_slashing"||kind=="proposer_slashing"){
  if(check)check();
  if(const auto published=std::atomic_load_explicit(&beacon_publication_,std::memory_order_acquire))return capture_published_inventory(*published,kind,budget,check);
  // Preserve try-lock refusal and the original latched error when no durable
  // publication is available. Never turn a fail-closed state into an empty list.
 }
 if(check)check();std::unique_lock lock(mutex_,std::try_to_lock);
 if(!lock.owns_lock())return {};
 return capture_inventory_locked(kind,budget,check);
}
std::shared_ptr<const CapellaInventorySnapshot> CapellaDurableReactor::capture_inventory_locked(std::string_view kind,size_t budget,const std::function<void()>& check)const{
 require_available();if(check)check();
 if(kind!="block"&&kind!="attestation"&&kind!="attester_slashing"&&kind!="proposer_slashing"&&kind!="voluntary_exit"&&kind!="bls_to_execution_change"&&kind!="sync_committee_message")throw std::invalid_argument("Unsupported Capella inventory kind");
 const bool sync=kind=="sync_committee_message";size_t sync_count=0;if(sync)for(const auto& [_,messages]:pending_sync_messages_)if(sync_messages_current_locked(messages,check)){if(messages.wires.size()>budget/sizeof(ConsensusRoot)-sync_count)throw std::length_error("Capella sync inventory exceeds root budget");sync_count+=messages.wires.size();}
 const auto count=sync?sync_count:kind=="block"?state_->blocks.size()-size_t(state_->blocks.contains(state_->timed.view.anchor)):kind=="attestation"?votes_.size():kind=="bls_to_execution_change"?bls_changes_.size():kind=="voluntary_exit"?exits_.size():kind=="proposer_slashing"?proposer_slashings_.size():slashings_.size();
 if(count>budget/sizeof(ConsensusRoot))throw std::length_error("Capella inventory snapshot exceeds root budget");
 auto snapshot=std::make_shared<CapellaInventorySnapshot>();snapshot->token_=*token_;snapshot->kind_=kind;snapshot->roots_.reserve(count);
 if(sync){for(const auto& [_,messages]:pending_sync_messages_)if(sync_messages_current_locked(messages,check))for(const auto& [index,wire]:messages.wires){if(check)check();snapshot->roots_.push_back(archive_array<32>(sha256(wire),0));}std::sort(snapshot->roots_.begin(),snapshot->roots_.end());}
 else if(kind=="block"){for(const auto& [root,_]:state_->blocks){if(check)check();if(root!=state_->timed.view.anchor)snapshot->roots_.push_back(root);}}
 else if(kind=="voluntary_exit")snapshot->roots_=exit_roots_locked(check);
 else if(kind=="proposer_slashing"){for(const auto& [id,_]:proposer_slashings_){if(check)check();snapshot->roots_.push_back(archive_array<32>(id,0));}}
 else if(kind=="bls_to_execution_change")snapshot->roots_=bls_change_roots_locked(check);
 else{for(const auto& [key,_]:kind=="attestation"?votes_:slashings_){if(check)check();ConsensusRoot root{};if(key.size()!=root.size())throw std::logic_error("Invalid Capella archive root");std::copy(key.begin(),key.end(),root.begin());snapshot->roots_.push_back(root);}}
 if(snapshot->roots_.capacity()>budget/sizeof(ConsensusRoot))throw std::length_error("Capella inventory allocation exceeds root budget");return snapshot;
}
Json CapellaInventorySnapshot::seek(const ConsensusRoot& boundary,size_t count,size_t budget,const std::function<void()>& check)const{
 if(check)check();const auto next=std::upper_bound(roots_.begin(),roots_.end(),boundary);
 std::optional<ConsensusRoot> after;if(next!=roots_.begin())after=*std::prev(next);
 return page(after,count,budget,check);
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
 if(kind!="block"&&kind!="attestation"&&kind!="attester_slashing"&&kind!="proposer_slashing"&&kind!="voluntary_exit"&&kind!="bls_to_execution_change")throw std::invalid_argument("Unsupported Capella inventory kind");
 if(after&&!expected)throw std::invalid_argument("Capella continuation needs checkpoint");
 if(expected&&*expected!=*token_)throw std::invalid_argument("Stale Capella inventory checkpoint");
 Json result{{"sequence",quantity(Int(std::to_string(token_->sequence)))},{"digest",hex(token_->digest)},{"kind",kind},{"entries",Json::array()},{"next",hex(Bytes(32,0))}};
 size_t used=result.dump().size();if(used>budget)throw std::length_error("Capella inventory framing exceeds capacity");
 std::string last;bool more=false;
 auto append=[&](const Bytes& key){if(check)check();if(result["entries"].size()>=count){more=true;return false;}Json item=hex(key);const auto size=item.dump().size()+(result["entries"].empty()?0:1);if(size>budget-used){if(result["entries"].empty())throw std::length_error("Capella inventory entry exceeds capacity");more=true;return false;}used+=size;last=item.get<std::string>();result["entries"].push_back(std::move(item));return true;};
 if(kind=="block"){
  if(after&&(!state_->blocks.contains(*after)||*after==state_->timed.view.anchor))throw std::invalid_argument("Unknown Capella inventory cursor");
  for(auto it=after?state_->blocks.upper_bound(*after):state_->blocks.begin();it!=state_->blocks.end();++it){if(check)check();if(it->first==state_->timed.view.anchor)continue;if(!append(Bytes(it->first.begin(),it->first.end())))break;}
 }else if(kind=="proposer_slashing"){
  Bytes cursor;if(after){cursor=Bytes(after->begin(),after->end());if(!proposer_slashings_.contains(cursor))throw std::invalid_argument("Unknown Capella proposer inventory cursor");}
  for(auto it=after?proposer_slashings_.upper_bound(cursor):proposer_slashings_.begin();it!=proposer_slashings_.end();++it)if(!append(it->first))break;
 }else if(kind=="bls_to_execution_change"||kind=="voluntary_exit"){
  const auto roots=kind=="voluntary_exit"?exit_roots_locked(check):bls_change_roots_locked(check);auto it=roots.begin();if(after){it=std::lower_bound(roots.begin(),roots.end(),*after);if(it==roots.end()||*it!=*after)throw std::invalid_argument("Unknown Capella inventory cursor");++it;}for(;it!=roots.end();++it)if(!append(Bytes(it->begin(),it->end())))break;
 }else{
  const auto& archive=kind=="attestation"?votes_:slashings_;Bytes cursor;if(after){cursor=Bytes(after->begin(),after->end());if(!archive.contains(cursor))throw std::invalid_argument("Unknown Capella inventory cursor");}
  for(auto it=after?archive.upper_bound(cursor):archive.begin();it!=archive.end();++it)if(!append(it->first))break;
 }
 result["next"]=more?Json(last):Json(nullptr);if(check)check();return result;
}

Json CapellaDurableReactor::export_message(std::string_view kind,const ConsensusRoot& id,size_t maximum,const std::optional<ForkCheckpointToken>& expected,const std::function<void()>& check)const{
 // Retained block serving uses exactly one durably published immutable view.
 // Do not serialize archive validation behind a concurrent admission writer.
 // Historical exports and mutable operation inventories retain their lock path.
 if(kind=="block"){
  if(check)check();const auto published=beacon_snapshot();
  const auto found=published.state->blocks.find(id);
  if(found!=published.state->blocks.end()){
   if(!maximum)throw std::invalid_argument("Capella export capacity required");
   if(expected&&*expected!=published.token)throw std::invalid_argument("Stale Capella export checkpoint");
   const auto payload=beacon_signed_block(published,id,check);
   const auto& header=found->second.header();
   Json result{{"available",bool(payload)},{"root",hex(Bytes(id.begin(),id.end()))},{"sequence",quantity(Int(std::to_string(published.token.sequence)))},{"digest",hex(published.token.digest)},
    {"known",true},{"anchor",id==published.state->timed.view.anchor},{"parentRoot",hex(Bytes(header.parent_root.begin(),header.parent_root.end()))},{"slot",quantity(Int(std::to_string(header.slot)))}};
   if(payload){if(payload->size()>maximum)throw std::length_error("Capella export exceeds capacity");auto envelope=network_identity_for_store(*published.state);envelope["kind"]=kind;envelope["ssz"]=hex(*payload);result["envelope"]=std::move(envelope);}
   if(check)check();return result;
  }
 }
 std::lock_guard lock(mutex_);require_available();if(check)check();
 if(!maximum)throw std::invalid_argument("Capella export capacity required");
 if(kind!="block"&&kind!="attestation"&&kind!="attester_slashing"&&kind!="proposer_slashing"&&kind!="voluntary_exit"&&kind!="bls_to_execution_change"&&kind!="sync_committee_message")throw std::invalid_argument("Unsupported Capella archive kind");
 if(expected&&*expected!=*token_)throw std::invalid_argument("Stale Capella export checkpoint");
 const Bytes key(id.begin(),id.end());std::optional<Bytes> payload;
 if(kind=="sync_committee_message"){for(const auto& [_,messages]:pending_sync_messages_)if(sync_messages_current_locked(messages,check))for(const auto& [index,wire]:messages.wires){if(check)check();if(sha256(wire)==key){if(wire.size()>maximum)throw std::length_error("Capella export exceeds capacity");payload=wire;break;}}}
 else if(kind=="block")payload=signed_block_locked(id,check);
 else if(kind=="proposer_slashing"){const auto found=proposer_slashings_.find(key);if(found!=proposer_slashings_.end()){if(found->second.wire.size()>maximum)throw std::length_error("Capella export exceeds capacity");payload=found->second.wire;}}
 else if(kind=="voluntary_exit"){const auto found=exits_.find(key);if(found!=exits_.end()){if(found->second.wire.size()>maximum)throw std::length_error("Capella export exceeds capacity");payload=found->second.wire;}}
 else if(kind=="bls_to_execution_change"){for(const auto& [index,record]:bls_changes_){if(check)check();if(sha256(record.wire)==key){if(record.wire.size()>maximum)throw std::length_error("Capella export exceeds capacity");payload=record.wire;break;}}}
 else{const auto& archive=kind=="attestation"?votes_:slashings_;const auto found=archive.find(key);if(found!=archive.end()){
  if(found->second.size()>maximum)throw std::length_error("Capella export exceeds capacity");
  if(sha256(found->second)!=key)throw std::runtime_error("Capella archive digest mismatch");payload=found->second;
 }}
 Json result{{"available",bool(payload)},{"root",hex(key)},{"sequence",quantity(Int(std::to_string(token_->sequence)))},{"digest",hex(token_->digest)},
  {"known",kind=="block"&&state_->blocks.contains(id)},{"anchor",kind=="block"&&id==state_->timed.view.anchor}};
 if(kind=="block"&&state_->blocks.contains(id)){const auto& header=state_->blocks.at(id).header();result["parentRoot"]=hex(Bytes(header.parent_root.begin(),header.parent_root.end()));result["slot"]=quantity(Int(std::to_string(header.slot)));}
 if(kind=="block"&&payload&&!state_->blocks.contains(id)){const auto header=archived_header(*payload,rules_,check);result["parentRoot"]=hex(Bytes(header.parent_root.begin(),header.parent_root.end()));result["slot"]=quantity(Int(std::to_string(header.slot)));result["historical"]=true;}
 if(payload){if(payload->size()>maximum)throw std::length_error("Capella export exceeds capacity");auto envelope=network_identity_locked();envelope["kind"]=kind;envelope["ssz"]=hex(*payload);result["envelope"]=std::move(envelope);}
 if(check)check();return result;
}

CapellaTargetPreparation* CapellaDurableReactor::prepare_vote_target_locked(const CapellaForkStore& store,
 const Phase0SignedAttestation& vote,bool complete,const std::function<void()>& check){
 const auto slots=rules_.transition.consensus.epoch.ffg.slots_per_epoch;
 const auto& clock=store.timed.clock;if(!slots||!rules_.slot_duration_ms||clock.time_ms<clock.genesis_time_ms)throw std::invalid_argument("Invalid vote preparation clock");
 const auto current=(clock.time_ms-clock.genesis_time_ms)/rules_.slot_duration_ms,epoch=current/slots;const auto& d=vote.data;
 if(d.slot>current||d.target.epoch!=d.slot/slots||d.target.epoch>epoch||d.target.epoch<(epoch?epoch-1:0)||!store.blocks.contains(d.target.root)||!store.timed.view.nodes.contains(d.beacon_block_root)||store.blocks.at(d.target.root).header().slot>d.target.epoch*slots)return nullptr;
 auto& primary=vote_preparations_[d.target.epoch%vote_preparations_.size()];
 CapellaTargetPreparation* frame=nullptr;
 if(!primary||primary->target().epoch!=d.target.epoch||!primary->matches(store,rules_,primary->target(),check)){
  primary.emplace(store,rules_,d.target,check);frame=&*primary;
 }else if(primary->target()==d.target)frame=&*primary;
 else{
  // Concurrent branches in one epoch must not replace each other's prefixes.
  // Share the bounded, authenticated block-target cache for secondary roots.
  std::erase_if(incoming_targets_,[&](const auto& entry){return !entry.matches(store,rules_,entry.target(),check);});
  auto found=std::find_if(incoming_targets_.begin(),incoming_targets_.end(),[&](const auto& entry){return entry.target()==d.target;});
  if(found==incoming_targets_.end()){
   if(incoming_targets_.size()>=size_t(rules_.transition.consensus.body.attestations)+2)incoming_targets_.erase(incoming_targets_.begin());
   incoming_targets_.emplace_back(store,rules_,d.target,check);
  }else std::rotate(found,std::next(found),incoming_targets_.end());
  frame=&incoming_targets_.back();
 }
 do{if(frame->advance(std::min(limits_.replay_slots,slots),limits_.sync_trials,check))return frame;}while(complete);
 return nullptr;
}
bool CapellaDurableReactor::vote(const Phase0SignedAttestation& input,const std::function<void()>& callback){
 std::lock_guard lock(mutex_);require_available();
 const auto limit=rules_.transition.consensus.body.validators_per_committee;
 if(!limit||input.aggregation_bits.size()>limit||input.signature.size()!=96)throw std::invalid_argument("invalid attestation encoding bounds");
 const auto vote=input;const auto check=callback;
 auto wire=encode_consensus_attestation_ssz(vote,rules_.transition.consensus.body.validators_per_committee,check);auto id=sha256(wire);
 if(const auto found=votes_.find(id);found!=votes_.end()&&found->second==wire)return true;
 auto* prepared=prepare_vote_target_locked(*state_,vote,false,check);if(!prepared)return false;
 auto next=checked_vote(*state_,vote,rules_,limits_,check,prepared);if(!next)return false;
 if(votes_.size()>=limits_.messages)throw std::length_error("Capella signed vote capacity");auto votes=votes_;votes.emplace(std::move(id),std::move(wire));publish(std::move(*next),check,{},std::move(votes));return true;
}

bool CapellaDurableReactor::proposer_slashing_ssz(std::span<const uint8_t> wire,const std::function<void()>& check){
 const auto owned_check=check;const auto proof=decode_consensus_proposer_slashing_ssz(wire,owned_check);
 return proof&&proposer_slashing(proof->first,proof->second,owned_check);
}
CapellaSignedVotes CapellaDurableReactor::signed_proposer_slashings()const{
 std::lock_guard lock(mutex_);require_available();CapellaSignedVotes result;
 for(const auto& [id,record]:proposer_slashings_)result.emplace(id,record.wire);return result;
}
bool CapellaDurableReactor::proposer_slashing(SignedConsensusBeaconHeader first,SignedConsensusBeaconHeader second,const std::function<void()>& check){
 const auto owned_check=check;std::lock_guard lock(mutex_);require_available();
 if(!rules_.transition.consensus.body.proposer_slashings)return false;
 const auto root=consensus_beacon_header_root(capella_fork_head(*state_,rules_,owned_check)->header);const auto epoch=state_->timed.view.current_epoch;
 const auto key=proposer_slashing_key(*state_,root,epoch,first,second,rules_,limits_,owned_check);if(!key)return false;
 // Dedupe follows full authentication and is by offender identity, not slot or
 // wire hash alone. Reversed/different valid pairs cannot grow the pool.
 for(const auto& [_,record]:proposer_slashings_){if(owned_check)owned_check();const auto proof=decode_consensus_proposer_slashing_ssz(record.wire,owned_check);if(!proof)throw std::logic_error("Invalid retained proposer proof");if(proof->first.message.proposer_index==first.message.proposer_index&&record.public_key==*key)return false;}
 if(proposer_slashings_.size()>=limits_.equivocations)throw std::length_error("Capella proposer slashing capacity");
 auto wire=canonical_proposer_slashing_wire(std::move(first),std::move(second),owned_check);auto pending=proposer_slashings_;const auto id=sha256(wire);pending.emplace(id,CapellaPendingProposerSlashing{root,epoch,*key,std::move(wire)});
 // No economic mutation or attester fork-choice exclusion occurs at admission.
 publish(*state_,owned_check,{},std::nullopt,std::nullopt,std::nullopt,std::nullopt,std::move(pending));return true;
}
bool CapellaDurableReactor::slashing(const IndexedConsensusAttestation& first_input,const IndexedConsensusAttestation& second_input,const std::function<void()>& callback){
 std::lock_guard lock(mutex_);require_available();
 const auto limit=rules_.transition.consensus.body.validators_per_committee;
 auto bounded=[&](const auto& a){return limit&&a.indices.size()<=limit&&a.signature.size()==96&&a.indices.size()<=(UINT32_MAX-228)/8;};
 if(!bounded(first_input)||!bounded(second_input))return false;
 const auto first=first_input,second=second_input;const auto check=callback;
 auto next=prepare_capella_store_slashing(*state_,first,second,rules_,check);if(!next)return false;
 auto wire=encode_consensus_attester_slashing_ssz(first,second,rules_.transition.consensus.body.validators_per_committee,check);const auto id=sha256(wire);auto proofs=slashings_;
 if(!proofs.contains(id)){
  std::set<uint64_t> uncovered;std::set_intersection(first.indices.begin(),first.indices.end(),second.indices.begin(),second.indices.end(),std::inserter(uncovered,uncovered.end()));
  const auto finalized_pin=state_->blocks.at(state_->timed.view.finalized.root).lease(check);const auto& finalized=*finalized_pin;const auto& t=rules_.transition;
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
 if(retained_epoch>UINT64_MAX/slots||state_->blocks.at(anchor).header().slot>retained_epoch*slots)return;
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

namespace nano {
bool CapellaDurableReactor::migrate_fragment_checkpoint(const std::function<void()>& check){
 std::lock_guard lock(mutex_);require_available();if(check)check();
 const auto record=disk_.fork_checkpoint(chain_,genesis_,limits_.envelope_bytes,token_);archive_require(bool(record));
 if(cold_options(record->snapshot,limits_)){(void)restore_durable(record->snapshot,record->token.digest,rules_,limits_,check,checkpoint_reader(disk_));return false;}
 if(record->snapshot.starts_with("{\"expandedBytes\":")){
  (void)restore_durable(record->snapshot,record->token.digest,rules_,limits_,check,checkpoint_reader(disk_));return false;
 }
 publish_impl(*state_,check,{},std::nullopt,std::nullopt,std::nullopt,std::nullopt,true);return true;
}
}

namespace nano {
bool CapellaDurableReactor::migrate_reference_checkpoint(CapellaColdCheckpointOptions options,const std::function<void()>& check){
 std::lock_guard lock(mutex_);require_available();if(check)check();
 const auto record=disk_.fork_checkpoint(chain_,genesis_,limits_.envelope_bytes,token_);archive_require(bool(record));
 if(cold_options(record->snapshot,limits_)==std::optional(options)){
  // Recheck disk availability/digests; reuse certificates only for identical
  // immutable state metadata. A failed/cancelled check cannot mutate the memo.
  auto candidate=reference_cache_;
  (void)restore_durable(record->snapshot,record->token.digest,rules_,limits_,check,checkpoint_reader(disk_),std::nullopt,state_.get(),&candidate);
  reference_cache_=std::move(candidate);return false;
 }
 publish_impl(*state_,check,{},std::nullopt,std::nullopt,std::nullopt,std::nullopt,false,options);return true;
}
}

namespace nano {
AuthenticatedCheckpointBlobClosure authenticate_capella_checkpoint_blob_closure(
 std::string_view encoded,const Bytes& digest,const CapellaForkBlockRules& rules,
 const CapellaStoreSnapshotLimits& limits,
 const std::function<std::optional<std::string>(const Bytes&,size_t)>& reader,
 const std::vector<CheckpointArchiveEntry>& archive,const std::function<void()>& check){
 AuthenticatedCheckpointBlobClosure closure;
 const auto traced=[&](const Bytes& id,size_t bytes)->std::optional<std::string>{
  archive_require(id.size()==32&&bytes&&bytes<=limits.envelope_bytes&&bool(reader));
  const auto [found,added]=closure.references_.emplace(id,bytes);
  archive_require(added||found->second==bytes);
  const auto payload=reader(id,bytes);
  archive_require(payload&&payload->size()==bytes&&sha256(Bytes(payload->begin(),payload->end()))==id);
  if(check)check();return payload;
 };
 // Neither process-local memo nor a previously authenticated Store is supplied.
 // Every live cold record is visited by the complete decoder, not only its head.
 auto [store,votes,slashings,changes,exits,proposers]=restore_durable(encoded,digest,rules,limits,check,traced);
 archive_require(archive.size()<=1);
 for(const auto& entry:archive){
  archive_require(entry.root.size()==32&&entry.payload.size()<=limits.envelope_bytes);
  const auto id=archive_array<32>(entry.root,0);
  verify_archived_block(id,entry.payload,store,rules,check);
  archive_require(store.blocks.at(id).header().slot==entry.slot);
 }
 const auto head=capella_fork_head(store,rules,check);
 auto context=capella_fork_block_context(head->state,head->header,
  {rules.transition.consensus.epoch,rules.transition.execution,rules.transition.maximum_state_bytes},
  rules.transition.consensus.state,check);
 const auto network=capella_network_anchor(store);
 closure.execution_head_={{"number",quantity(head->execution_block.header.number)},{"hash",hex(head->execution_block.header.hash())},{"stateRoot",hex(head->execution_state.root())}};
 closure.identity_={{"profile",capella_fork_profile(rules)},
  {"networkAnchor",hex(Bytes(network.begin(),network.end()))},
  {"genesisValidatorsRoot",hex(Bytes(context.genesis_root.begin(),context.genesis_root.end()))},
  {"genesisTimeMs",quantity(Int(std::to_string(store.timed.clock.genesis_time_ms)))}};
 if(check)check();closure.head_context_=std::move(context);return closure;
}
}
