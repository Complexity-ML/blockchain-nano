#include "nano/consensus_capella_store_snapshot.hpp"
#include "nano/consensus_capella_cold_records.hpp"
#include <source_location>
#include "consensus_capella_snapshot_hex.hpp"
#include "consensus_capella_snapshot_json.hpp"
#include "consensus_capella_snapshot_accounts.hpp"
#include "nano/consensus_capella_deposits.hpp"
#include "nano/consensus_bellatrix_store_snapshot.hpp"
namespace nano {
namespace {
void require(bool ok,const std::source_location at=std::source_location::current()){if(!ok)throw std::invalid_argument("invalid Capella Store snapshot at line "+std::to_string(at.line()));}
std::string num(uint64_t n){return quantity(Int(std::to_string(n)));}
uint64_t number(const Json& j){return u64(quantity(j));}
std::string root(const ConsensusRoot& r){return hex(Bytes(r.begin(),r.end()));}
ConsensusRoot root(const Json& j){require(j.is_string());const auto& s=j.get_ref<const std::string&>();require(s.size()==66&&s.starts_with("0x"));auto b=unhex(s);require(b.size()==32);ConsensusRoot r{};std::copy(b.begin(),b.end(),r.begin());return r;}
Json cp(const ConsensusCheckpoint& c){return Json::array({num(c.epoch),root(c.root)});}
ConsensusCheckpoint cp(const Json& j){require(j.is_array()&&j.size()==2);return {number(j[0]),root(j[1])};}
CapellaEpochRules epoch_rules(const CapellaForkBlockRules& r){return {r.transition.consensus.epoch,r.transition.execution,r.transition.maximum_state_bytes};}
Json profile(const CapellaForkBlockRules& r){
 const auto& t=r.transition;
 require(t.maximum_bls_changes&&t.withdrawals.maximum_per_payload&&t.withdrawals.maximum_per_payload<=UINT32_MAX/44&&t.withdrawals.maximum_validators_per_sweep&&t.withdrawals.maximum_effective_balance==t.consensus.attestation.duties.maximum_effective_balance&&t.genesis_version==t.consensus.deposit.genesis_fork_version);
 const BellatrixForkBlockRules common{{t.consensus,t.execution,t.maximum_state_bytes,t.seconds_per_slot,t.chain_id,t.replay_policy},r.slot_duration_ms,r.attestation_due_ms,r.proposer_score_boost,static_cast<BellatrixProposerBoostProfile>(r.boost_profile),r.deposit_contract_code_hash,r.native_deposit_votes};
 return Json{{"format","nano-capella-profile-v1"},{"common",bellatrix_fork_profile(common)},{"withdrawals",Json::array({num(t.withdrawals.maximum_effective_balance),num(t.withdrawals.maximum_per_payload),num(t.withdrawals.maximum_validators_per_sweep)})},{"genesisVersion",hex(Bytes(t.genesis_version.begin(),t.genesis_version.end()))},{"maximumBlsChanges",num(t.maximum_bls_changes)}};
}
Json slot_snapshot(const CapellaSlotState& s){return Json::array({hex(s.ssz),s.last_processed_epoch?Json(num(*s.last_processed_epoch)):Json(nullptr)});}
CapellaSlotState slot_restore(const Json& j,size_t budget){require(j.is_array()&&j.size()==2&&j[0].is_string());const auto& hexstate=j[0].get_ref<const std::string&>();require(hexstate.starts_with("0x")&&hexstate.size()%2==0&&(hexstate.size()-2)/2<=budget);CapellaSlotState s{unhex(hexstate),std::nullopt};if(!j[1].is_null())s.last_processed_epoch=number(j[1]);return s;}
Json execution(const CapellaForkBlock& b,const std::function<void()>& cancel,bool matched_accounts=false){Json receipts=Json::array();for(const auto& receipt:b.receipts)receipts.push_back(receipt.to_json());auto out=Json::array({b.execution_block.to_json(true),matched_accounts?Json(nullptr):b.execution_state.to_json(),receipts});if(b.deposit_history)out.push_back(b.deposit_history->snapshot(cancel));return out;}
ConsensusSlashingState registry(const CapellaForkStore& s,const CapellaForkBlockRules& r,const CapellaStoreSnapshotLimits& l,const std::function<void()>& cancel){
 const auto b_pin=s.blocks.at(s.timed.view.justified.root).lease(cancel);const auto& b=*b_pin;
 if(s.timed.view.justified.root==s.timed.view.anchor&&s.timed.view.justified.epoch==b.header.slot/r.transition.consensus.epoch.ffg.slots_per_epoch)return capella_fork_block_context(b.state,b.header,epoch_rules(r),r.transition.consensus.state,cancel).registry;
 return prepare_capella_resumed_target_context(s,r,s.timed.view.justified,l.replay_slots,l.sync_trials,cancel).registry;
}
Json reference_metadata(const ConsensusRoot& id,const CapellaForkRecord& entry,const CapellaForkStore& s){
 const auto& h=entry.header();const auto& node=s.timed.view.nodes.at(id);
 return Json::array({Json::array({num(h.slot),num(h.proposer_index),root(h.parent_root),root(h.state_root),root(h.body_root)}),entry.execution_header().to_json(),num(node.realized_justified_epoch),num(node.unrealized_justified_epoch),cp(s.unrealized_justifications.at(id)),id==s.timed.view.anchor});
}
struct ReusedColdMetadata {ConsensusBeaconHeader header;Header execution;Phase0ForkNode node;};
}
std::string capella_store_snapshot_reference_memo_impl(const CapellaForkStore& s,const CapellaForkBlockRules& r,
 const CapellaStoreSnapshotLimits& l,const std::function<void()>& cancel,CapellaStoreSnapshotCache* cache,
 const std::function<void(const CapellaColdReference&,std::string_view)>* stage,CapellaReferenceSnapshotCache* memo){
 require(!stage||(!cache&&bool(*stage)));require(!memo||stage);
 const auto& v=s.timed.view;const auto& c=s.timed.clock;const auto slots=r.transition.consensus.epoch.ffg.slots_per_epoch;
 require(slots&&r.slot_duration_ms&&r.attestation_due_ms<=r.slot_duration_ms&&c.time_ms>=c.genesis_time_ms&&
  v.current_epoch==(c.time_ms-c.genesis_time_ms)/r.slot_duration_ms/slots&&
  (r.boost_profile==CapellaProposerBoostProfile::Release160Beta0||r.boost_profile==CapellaProposerBoostProfile::ShufflingDependent));
 require(!s.blocks.empty()&&s.blocks.size()<=l.blocks&&s.blocks.size()==v.nodes.size()&&v.latest_messages.size()<=l.messages&&v.equivocating_indices.size()<=l.equivocations);
 require(s.unrealized_justifications.size()==s.blocks.size()&&s.timely.size()+1==s.blocks.size()&&!s.timely.contains(v.anchor));
 require(v.justified_registry==registry(s,r,l,cancel));
 phase0_fork_choice_head(v,{slots,r.transition.consensus.attestation.duties.effective_balance_increment,r.proposer_score_boost},cancel);
 const auto current_profile=profile(r);
 if(memo){
  if(memo->profile_!=current_profile||memo->encoded_bytes_>l.envelope_bytes){memo->records_.clear();memo->encoded_bytes_=0;memo->profile_=current_profile;}
  for(auto it=memo->records_.begin();it!=memo->records_.end();)if(!s.blocks.contains(it->first)){memo->encoded_bytes_-=it->second.encoded_bytes;it=memo->records_.erase(it);}else ++it;
 }
 const auto metadata=[&](const ConsensusRoot& id,const CapellaForkRecord& entry){return reference_metadata(id,entry,s);};
 if(cache&&cache->profile_!=current_profile){cache->records_.clear();cache->profile_=current_profile;}
 if(cache)std::erase_if(cache->records_,[&](const auto& item){return !s.blocks.contains(item.first);});
 std::vector<std::string_view> encoded_rows;
 Json blocks=Json::array(),messages=Json::array(),equiv=Json::array(),timely=Json::array(),unrealized=Json::array();size_t bytes=0;
 for(const auto& [id,entry]:s.blocks){
  if(cancel)cancel();
  if(memo&&entry.cold_reference()){
   const auto known=memo->records_.find(id);const auto& reference=*entry.cold_reference();
   if(known!=memo->records_.end()&&known->second.reference==reference&&known->second.metadata==metadata(id,entry)){
    const auto& certified=known->second;const auto& node=v.nodes.at(id);
    require(reference.root==id&&reference.bytes&&reference.bytes<=l.envelope_bytes&&certified.state_bytes>=8&&certified.state_bytes<=l.state_bytes);
    require(consensus_beacon_header_root(entry.header())==id&&node.slot==entry.header().slot&&node.parent==entry.header().parent_root);
    require(certified.genesis_seconds<=UINT64_MAX/1000&&certified.genesis_seconds*1000==c.genesis_time_ms&&entry.header().slot<=(c.time_ms-c.genesis_time_ms)/r.slot_duration_ms);
    if(memo->reuses_==UINT64_MAX)throw std::overflow_error("Capella reference memo counter exhausted");++memo->reuses_;
    blocks.push_back(Json::array({root(id),hex(reference.digest),num(reference.bytes)}));continue;
   }
  }
  const auto b_pin=entry.lease(cancel);const auto& b=*b_pin;
  if(cancel)cancel();require(b.state.ssz.size()>=8&&b.state.ssz.size()<=l.state_bytes);
  if(!stage){require(b.state.ssz.size()<=l.total_state_bytes-bytes);bytes+=b.state.ssz.size();}
  require(consensus_beacon_header_root(b.header)==id);
  const auto& n=v.nodes.at(id);require(n.slot==b.header.slot&&n.parent==b.header.parent_root);
  uint64_t genesis_seconds=0;for(unsigned i=0;i<8;++i)genesis_seconds|=uint64_t(b.state.ssz[i])<<(8*i);
  require(genesis_seconds<=UINT64_MAX/1000&&genesis_seconds*1000==c.genesis_time_ms&&b.header.slot<=(c.time_ms-c.genesis_time_ms)/r.slot_duration_ms);
  const auto known=cache?cache->records_.find(id):std::map<ConsensusRoot,Json>::iterator{};
  // Reuse only the canonical state encoding after comparing every source byte.
  // The complete row comparison below still covers all execution and metadata.
  bool matched_state=false;
  if(cache&&known!=cache->records_.end()&&known->second.is_array()&&known->second.size()==4&&
     known->second[0].is_array()&&known->second[0].size()==6&&known->second[0][2].is_array()&&
     known->second[0][2].size()==2&&known->second[0][2][0].is_string())
   matched_state=snapshot_detail::canonical_hex_matches(known->second[0][2][0].get_ref<const std::string&>(),b.state.ssz,cancel);
  // Defer the cached SSZ string copy. Every byte was compared above; a cold
  // or changed record materializes the full string before validation/storage.
  Json encoded_state=matched_state?Json::array({nullptr,b.state.last_processed_epoch?Json(num(*b.state.last_processed_epoch)):Json(nullptr)}):slot_snapshot(b.state);
  const bool matched_accounts=matched_state&&known->second[0][5].is_array()&&
   (known->second[0][5].size()==3||known->second[0][5].size()==4)&&
   snapshot_detail::validated_accounts_match(b.execution_state,known->second[0][5][1],cancel);
  auto row=Json::array({root(id),Json::array({num(b.header.slot),num(b.header.proposer_index),root(b.header.parent_root),root(b.header.state_root),root(b.header.body_root)}),std::move(encoded_state),num(n.realized_justified_epoch),num(n.unrealized_justified_epoch),execution(b,cancel,matched_accounts)});
  // Compare every serialized field, including state/receipts/deposit history,
  // plus the checkpoint metadata checked below. Root equality alone is unsafe.
  const auto justification=cp(s.unrealized_justifications.at(id));
  const bool validated=matched_state&&snapshot_detail::same_row_after_ssz_match(known->second[0],row,matched_accounts)&&known->second[1]==justification&&known->second[2]==Json(id==v.anchor)&&known->second[3].is_string();
  if(!validated){
   if(matched_state)row[2][0]=known->second[0][2][0];
   if(matched_accounts)row[5][1]=known->second[0][5][1];
   validate_capella_execution_record(b,r,cancel);
   if(id!=v.anchor){const auto ffg=capella_fork_checkpoints(b.state,epoch_rules(r),r.transition.consensus.state,cancel);require(n.realized_justified_epoch==ffg.realized.current_justified.epoch&&n.unrealized_justified_epoch==ffg.unrealized.current_justified.epoch&&s.unrealized_justifications.at(id)==ffg.unrealized.current_justified);}
   if(cancel)cancel();if(cache)cache->records_.insert_or_assign(id,Json::array({row,justification,id==v.anchor,row.dump()}));
  }
  // String views remain valid: std::map insertions preserve other entries,
  // and this entry is never replaced again during this invocation.
  if(cache)encoded_rows.push_back(cache->records_.at(id)[3].get_ref<const std::string&>());
  // Cached row dumps contain the complete, validated records. The envelope
  // only needs their count, so release the temporary SSZ/execution row DOM
  // immediately instead of retaining it until the envelope is destroyed.
  // The uncached path retains the full DOM and remains the byte-level oracle.
  if(stage){
   const auto payload=row.dump();require(!payload.empty()&&payload.size()<=l.envelope_bytes);
   const auto digest=sha256(Bytes(payload.begin(),payload.end()));
   const CapellaColdReference reference{id,digest,payload.size(),id==v.anchor};
   if(entry.cold_reference()){
    const auto& committed=*entry.cold_reference();require(committed.root==id&&committed.digest==digest&&committed.bytes==payload.size());

   }else (*stage)(reference,payload);
    if(memo){
     auto proof_metadata=metadata(id,entry);const auto charge=Json::array({root(id),hex(digest),num(payload.size()),proof_metadata,num(b.state.ssz.size()),num(genesis_seconds)}).dump().size();
     if(entry.cold_reference()){if(memo->validated_cold_rows_==UINT64_MAX)throw std::overflow_error("Capella reference memo counter exhausted");++memo->validated_cold_rows_;}
     if(const auto old=memo->records_.find(id);old!=memo->records_.end()){memo->encoded_bytes_-=old->second.encoded_bytes;memo->records_.erase(old);}
     if(charge<=l.envelope_bytes){
      while(charge>l.envelope_bytes-memo->encoded_bytes_){auto victim=memo->records_.begin();memo->encoded_bytes_-=victim->second.encoded_bytes;memo->records_.erase(victim);}
      memo->records_.emplace(id,CapellaReferenceSnapshotCache::Entry{reference,std::move(proof_metadata),b.state.ssz.size(),genesis_seconds,charge});memo->encoded_bytes_+=charge;
     }
    }
   blocks.push_back(Json::array({root(id),hex(digest),num(payload.size())}));
  }else if(cache)blocks.push_back(nullptr);else blocks.push_back(std::move(row));
 }
 for(const auto& [i,m]:v.latest_messages)messages.push_back(Json::array({num(i),num(m.epoch),root(m.root)}));
 for(auto i:v.equivocating_indices)equiv.push_back(num(i));
 for(const auto& [id,b]:s.timely){require(s.blocks.contains(id));timely.push_back(Json::array({root(id),b}));}
 for(const auto& [id,p]:s.unrealized_justifications){require(s.blocks.contains(id));unrealized.push_back(Json::array({root(id),cp(p)}));}
 Json ancestors=Json::array();require(s.blocks.contains(v.anchor));auto child=s.blocks.at(v.anchor).execution_header();
 require(child.number>=0&&child.number<=Int("18446744073709551615")&&s.anchor_execution_ancestors.size()==std::min<uint64_t>(u64(child.number),256));
 for(const auto& ancestor:s.anchor_execution_ancestors){if(cancel)cancel();require(ancestor.number+1==child.number&&ancestor.hash()==child.parent_hash);ancestors.push_back(ancestor.to_json());child=ancestor;}
 // Every record was authenticated above, including its retained cold metadata.
 // Link checks use only headers; reloading evicted state here would duplicate
 // the complete cold decoding pass without checking any additional state byte.
 for(const auto& [id,entry]:s.blocks)if(id!=v.anchor){
  if(cancel)cancel();const auto& header=entry.header();const auto& execution_header=entry.execution_header();
  const auto& parent=s.blocks.at(header.parent_root);
  require(parent.header().slot<header.slot&&parent.execution_header().number+1==execution_header.number&&parent.execution_header().hash()==execution_header.parent_hash);
 }
 Json j{{"ancestors",std::move(ancestors)},{"format","nano-capella-store-v1"},{"profile",current_profile},{"anchor",root(v.anchor)},{"justified",cp(v.justified)},{"finalized",cp(v.finalized)},
 {"clock",Json::array({num(c.genesis_time_ms),num(c.time_ms),cp(c.unrealized_justified),cp(c.unrealized_finalized)})},
 {"boost",v.proposer_boost?Json(root(*v.proposer_boost)):Json(nullptr)},{"blocks",std::move(blocks)},{"messages",std::move(messages)},{"equivocations",std::move(equiv)},{"timely",std::move(timely)},{"unrealized",std::move(unrealized)}};
 if(s.network_anchor){require(*s.network_anchor!=v.anchor);j["format"]="nano-capella-store-v2";j["networkAnchor"]=root(*s.network_anchor);}
 require(!r.native_deposit_votes||!s.network_anchor||!s.deposit_ancestors.empty());
 if(!s.deposit_ancestors.empty()){
  require(r.native_deposit_votes&&s.network_anchor.has_value());validate_capella_deposit_ancestors(s,cancel);Json pins=Json::array();
  for(const auto& old:s.deposit_ancestors){if(cancel)cancel();const auto& h=old.beacon;pins.push_back(Json::array({Json::array({num(h.slot),num(h.proposer_index),root(h.parent_root),root(h.state_root),root(h.body_root)}),old.execution.to_json(),num(old.count),root(old.root)}));}
  j["format"]="nano-capella-store-v3";j["depositAncestors"]=std::move(pins);
 }
 auto encoded=stage?Json{{"format","nano-capella-reference-store-v1"},{"snapshot",std::move(j)}}.dump():cache?snapshot_detail::canonical_snapshot_rows(j,encoded_rows,l.envelope_bytes,cancel):j.dump();require(encoded.size()<=l.envelope_bytes);if(cancel)cancel();return encoded;
}
std::string capella_store_snapshot_impl(const CapellaForkStore& s,const CapellaForkBlockRules& r,
 const CapellaStoreSnapshotLimits& l,const std::function<void()>& check,CapellaStoreSnapshotCache* cache,
 const std::function<void(const CapellaColdReference&,std::string_view)>* stage){
 return capella_store_snapshot_reference_memo_impl(s,r,l,check,cache,stage,nullptr);
}
std::string capella_store_reference_snapshot_cached(const CapellaForkStore& s,const CapellaForkBlockRules& r,
 const CapellaStoreSnapshotLimits& l,const std::function<void(const CapellaColdReference&,std::string_view)>& stage,
 CapellaReferenceSnapshotCache& memo,const std::function<void()>& check){
 if(check)check();auto candidate=memo;
 auto result=capella_store_snapshot_reference_memo_impl(s,r,l,check,nullptr,&stage,&candidate);
 memo=std::move(candidate);return result;
}
std::string capella_store_snapshot(const CapellaForkStore& s,const CapellaForkBlockRules& r,
 const CapellaStoreSnapshotLimits& l,const std::function<void()>& check,CapellaStoreSnapshotCache* cache){
 return capella_store_snapshot_impl(s,r,l,check,cache,nullptr);
}
std::string capella_store_reference_snapshot(const CapellaForkStore& s,const CapellaForkBlockRules& r,
 const CapellaStoreSnapshotLimits& l,const std::function<void(const CapellaColdReference&,std::string_view)>& stage,const std::function<void()>& check){
 return capella_store_snapshot_impl(s,r,l,check,nullptr,&stage);
}
static CapellaForkStore restore_store(std::string_view bytes,const Bytes& digest,const CapellaForkBlockRules& r,
 const CapellaStoreSnapshotLimits& l,const std::function<void()>& cancel,
 const std::shared_ptr<CapellaColdRecordCache>& cache={},
 const std::function<std::optional<std::string>(const Bytes&,size_t)>& fetch={},
 CapellaReferenceSnapshotCache* memo=nullptr,
 const std::function<std::optional<ReusedColdMetadata>(const CapellaColdReference&)>& reuse={}){
 require(bytes.size()<=l.envelope_bytes&&digest.size()==32);if(cancel)cancel();require(sha256(Bytes(bytes.begin(),bytes.end()))==digest);
 const auto document=Json::parse(bytes);
 if(cache)require(document.is_object()&&document.size()==2&&document.at("format")=="nano-capella-reference-store-v1"&&document.dump()==bytes&&bool(fetch));
 const auto& j=cache?document.at("snapshot"):document;require(j.is_object()&&j.contains("format")&&j.at("format").is_string());const bool pinned=j.value("format",std::string())=="nano-capella-store-v3";const bool pruned=pinned||j.value("format",std::string())=="nano-capella-store-v2";require(j.is_object()&&j.size()==(pinned?15:pruned?14:13)&&(pruned||j.at("format")=="nano-capella-store-v1")&&j.at("profile")==profile(r));
 require(j.at("blocks").is_array()&&j.at("blocks").size()<=l.blocks&&j.at("messages").is_array()&&j.at("messages").size()<=l.messages&&j.at("equivocations").is_array()&&j.at("equivocations").size()<=l.equivocations);
 require(j.at("timely").is_array()&&j.at("timely").size()<=l.blocks&&j.at("unrealized").is_array()&&j.at("unrealized").size()<=l.blocks);
 CapellaForkStore s;auto& v=s.timed.view;auto& c=s.timed.clock;
 if(pruned)s.network_anchor=root(j.at("networkAnchor"));
 v.anchor=root(j.at("anchor"));v.justified=cp(j.at("justified"));v.finalized=cp(j.at("finalized"));
 const auto& clock=j.at("clock");require(clock.is_array()&&clock.size()==4);c={number(clock[0]),number(clock[1]),cp(clock[2]),cp(clock[3])};
 require(r.slot_duration_ms&&r.transition.consensus.epoch.ffg.slots_per_epoch&&c.time_ms>=c.genesis_time_ms);v.current_epoch=(c.time_ms-c.genesis_time_ms)/r.slot_duration_ms/r.transition.consensus.epoch.ffg.slots_per_epoch;
 if(!j.at("boost").is_null())v.proposer_boost=root(j.at("boost"));size_t total=0;
 for(const auto& b:j.at("blocks")){
  if(cache){
   if(cancel)cancel();require(b.is_array()&&b.size()==3&&b[1].is_string()&&b[2].is_string());
   const auto id=root(b[0]);const auto hash=unhex(b[1].get<std::string>());const auto length=number(b[2]);
   require(hash.size()==32&&hex(hash)==b[1].get<std::string>()&&length&&length<=l.envelope_bytes&&num(length)==b[2].get<std::string>());
   const CapellaColdReference reference{id,hash,size_t(length),id==v.anchor};
   if(reuse){if(const auto known=reuse(reference)){
    // A process-local certificate does not prove present disk availability.
    const auto payload=fetch(reference.digest,reference.bytes);require(payload&&payload->size()==reference.bytes&&sha256(Bytes(payload->begin(),payload->end()))==reference.digest);if(cancel)cancel();
    require(s.blocks.emplace(id,CapellaForkRecord(reference,known->header,known->execution,cache,fetch)).second);require(v.nodes.emplace(id,known->node).second);continue;
   }}
   const auto pin=cache->load(reference,fetch,cancel);
   require(s.blocks.emplace(id,CapellaForkRecord(reference,pin->block.header,pin->block.execution_block.header,cache,fetch)).second);
   require(v.nodes.emplace(id,pin->node).second);continue;
  }
  if(cancel)cancel();require(b.is_array()&&b.size()==6);const auto id=root(b[0]);const auto& h=b[1];require(h.is_array()&&h.size()==5);
  ConsensusBeaconHeader header{number(h[0]),number(h[1]),root(h[2]),root(h[3]),root(h[4])};
  auto state=slot_restore(b[2],std::min(l.state_bytes,l.total_state_bytes-total));total+=state.ssz.size();
  require(b[5].is_array()&&b[5].size()==(r.deposit_contract_code_hash.empty()?3:4)&&b[5][2].is_array());std::vector<Receipt> receipts;for(const auto& receipt:b[5][2]){if(cancel)cancel();receipts.push_back(Receipt::from_json(receipt));}
  auto execution_block=Block::from_json(b[5][0]);auto execution_state=State::from_json(b[5][1]);std::shared_ptr<const ExecutionDepositHistory> deposits;
  if(!r.deposit_contract_code_hash.empty())deposits=std::make_shared<const ExecutionDepositHistory>(ExecutionDepositHistory::from_snapshot(b[5][3],execution_block.header,execution_state,execution_block.header.hash(),r.deposit_contract_code_hash,UINT32_MAX,cancel));
  require(s.blocks.emplace(id,CapellaForkBlock{header,std::move(state),std::move(execution_block),std::move(execution_state),std::move(receipts),std::move(deposits)}).second);v.nodes.emplace(id,Phase0ForkNode{header.slot,header.parent_root,number(b[3]),number(b[4])});
 }
 for(const auto& m:j.at("messages")){require(m.is_array()&&m.size()==3);require(v.latest_messages.emplace(number(m[0]),Phase0LatestMessage{number(m[1]),root(m[2])}).second);}
 for(const auto& i:j.at("equivocations"))require(v.equivocating_indices.insert(number(i)).second);
 for(const auto& t:j.at("timely")){require(t.is_array()&&t.size()==2&&t[1].is_boolean());require(s.timely.emplace(root(t[0]),t[1].get<bool>()).second);}
 for(const auto& p:j.at("unrealized")){require(p.is_array()&&p.size()==2);require(s.unrealized_justifications.emplace(root(p[0]),cp(p[1])).second);}
 require(j.at("ancestors").is_array()&&j.at("ancestors").size()<=256);for(const auto& header:j.at("ancestors"))s.anchor_execution_ancestors.push_back(Header::from_json(header));
 if(pinned){require(r.native_deposit_votes&&j.at("depositAncestors").is_array()&&!j.at("depositAncestors").empty()&&j.at("depositAncestors").size()<=l.envelope_bytes/200);
  for(const auto& row:j.at("depositAncestors")){if(cancel)cancel();require(row.is_array()&&row.size()==4);const auto& h=row[0];require(h.is_array()&&h.size()==5);s.deposit_ancestors.push_back({{number(h[0]),number(h[1]),root(h[2]),root(h[3]),root(h[4])},Header::from_json(row[1]),number(row[2]),root(row[3])});}
 }
 v.justified_registry=registry(s,r,l,cancel);
 const std::function<void(const CapellaColdReference&,std::string_view)> no_hot=[](const CapellaColdReference&,std::string_view){throw std::logic_error("Unexpected hot restored record");};
 require((cache?(memo?capella_store_reference_snapshot_cached(s,r,l,no_hot,*memo,cancel):capella_store_reference_snapshot(s,r,l,no_hot,cancel)):capella_store_snapshot(s,r,l,cancel))==bytes);return s;
}
CapellaForkStore capella_store_from_snapshot(std::string_view bytes,const Bytes& digest,const CapellaForkBlockRules& rules,
 const CapellaStoreSnapshotLimits& limits,const std::function<void()>& check){return restore_store(bytes,digest,rules,limits,check);}
CapellaForkStore capella_store_from_reference_snapshot(std::string_view bytes,const Bytes& digest,const CapellaForkBlockRules& rules,
 const CapellaStoreSnapshotLimits& limits,size_t records,size_t encoded_bytes,
 const std::function<std::optional<std::string>(const Bytes&,size_t)>& fetch,const std::function<void()>& check){
 auto cache=std::make_shared<CapellaColdRecordCache>(rules,limits,records,encoded_bytes);
 return restore_store(bytes,digest,rules,limits,check,cache,fetch);
}
CapellaForkStore capella_store_from_reference_snapshot_reusing(std::string_view bytes,const Bytes& digest,const CapellaForkBlockRules& rules,
 const CapellaStoreSnapshotLimits& limits,size_t records,size_t encoded_bytes,
 const std::function<std::optional<std::string>(const Bytes&,size_t)>& fetch,const CapellaForkStore& source,
 CapellaReferenceSnapshotCache& memo,const std::function<void()>& check,size_t* reused_records){
 if(check)check();auto candidate=memo;size_t reused=0;const auto current_profile=profile(rules);
 if(candidate.profile_!=current_profile||candidate.encoded_bytes_>limits.envelope_bytes){candidate.records_.clear();candidate.encoded_bytes_=0;candidate.profile_=current_profile;}
 auto cache=std::make_shared<CapellaColdRecordCache>(rules,limits,records,encoded_bytes);
 const std::function<std::optional<ReusedColdMetadata>(const CapellaColdReference&)> reuse=[&](const auto& reference)->std::optional<ReusedColdMetadata>{
  if(check)check();if(candidate.profile_!=current_profile)return std::nullopt;
  const auto entry=source.blocks.find(reference.root);const auto certified=candidate.records_.find(reference.root);
  if(entry==source.blocks.end()||certified==candidate.records_.end()||!entry->second.cold_reference()||*entry->second.cold_reference()!=reference)return std::nullopt;
  const auto& proof=certified->second;
  if(proof.reference!=reference||proof.metadata!=reference_metadata(reference.root,entry->second,source)||proof.state_bytes<8||proof.state_bytes>limits.state_bytes||proof.state_bytes>limits.total_state_bytes||reference.bytes>encoded_bytes)return std::nullopt;
  const auto& node=source.timed.view.nodes.at(reference.root);const auto& header=entry->second.header();
  require(consensus_beacon_header_root(header)==reference.root&&node.slot==header.slot&&node.parent==header.parent_root);
  ++reused;return ReusedColdMetadata{header,entry->second.execution_header(),node};
 };
 auto restored=restore_store(bytes,digest,rules,limits,check,cache,fetch,&candidate,reuse);
 memo=std::move(candidate);if(reused_records)*reused_records=reused;return restored;
}
}

namespace nano {Json capella_fork_profile(const CapellaForkBlockRules& r){return profile(r);}}
