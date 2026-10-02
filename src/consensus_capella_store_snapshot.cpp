#include "nano/consensus_capella_store_snapshot.hpp"
#include <source_location>
#include "consensus_capella_snapshot_hex.hpp"
#include "consensus_capella_snapshot_json.hpp"
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
Json execution(const CapellaForkBlock& b,const std::function<void()>& cancel){Json receipts=Json::array();for(const auto& receipt:b.receipts)receipts.push_back(receipt.to_json());auto out=Json::array({b.execution_block.to_json(true),b.execution_state.to_json(),receipts});if(b.deposit_history)out.push_back(b.deposit_history->snapshot(cancel));return out;}
ConsensusSlashingState registry(const CapellaForkStore& s,const CapellaForkBlockRules& r,const CapellaStoreSnapshotLimits& l,const std::function<void()>& cancel){
 const auto& b=s.blocks.at(s.timed.view.justified.root);
 if(s.timed.view.justified.root==s.timed.view.anchor&&s.timed.view.justified.epoch==b.header.slot/r.transition.consensus.epoch.ffg.slots_per_epoch)return capella_fork_block_context(b.state,b.header,epoch_rules(r),r.transition.consensus.state,cancel).registry;
 return prepare_capella_fork_state_contexts(b.state,b.header,s.timed.view.justified,epoch_rules(r),r.transition.consensus.state,l.replay_slots,l.sync_trials,cancel).target.registry;
}
}
std::string capella_store_snapshot(const CapellaForkStore& s,const CapellaForkBlockRules& r,
 const CapellaStoreSnapshotLimits& l,const std::function<void()>& cancel,CapellaStoreSnapshotCache* cache){
 const auto& v=s.timed.view;const auto& c=s.timed.clock;const auto slots=r.transition.consensus.epoch.ffg.slots_per_epoch;
 require(slots&&r.slot_duration_ms&&r.attestation_due_ms<=r.slot_duration_ms&&c.time_ms>=c.genesis_time_ms&&
  v.current_epoch==(c.time_ms-c.genesis_time_ms)/r.slot_duration_ms/slots&&
  (r.boost_profile==CapellaProposerBoostProfile::Release160Beta0||r.boost_profile==CapellaProposerBoostProfile::ShufflingDependent));
 require(!s.blocks.empty()&&s.blocks.size()<=l.blocks&&s.blocks.size()==v.nodes.size()&&v.latest_messages.size()<=l.messages&&v.equivocating_indices.size()<=l.equivocations);
 require(s.unrealized_justifications.size()==s.blocks.size()&&s.timely.size()+1==s.blocks.size()&&!s.timely.contains(v.anchor));
 require(v.justified_registry==registry(s,r,l,cancel));
 phase0_fork_choice_head(v,{slots,r.transition.consensus.attestation.duties.effective_balance_increment,r.proposer_score_boost},cancel);
 const auto current_profile=profile(r);
 if(cache&&cache->profile_!=current_profile){cache->records_.clear();cache->profile_=current_profile;}
 if(cache)std::erase_if(cache->records_,[&](const auto& item){return !s.blocks.contains(item.first);});
 std::vector<std::string_view> encoded_rows;
 Json blocks=Json::array(),messages=Json::array(),equiv=Json::array(),timely=Json::array(),unrealized=Json::array();size_t bytes=0;
 for(const auto& [id,b]:s.blocks){
  if(cancel)cancel();require(b.state.ssz.size()>=8&&b.state.ssz.size()<=l.state_bytes&&b.state.ssz.size()<=l.total_state_bytes-bytes);bytes+=b.state.ssz.size();
  require(consensus_beacon_header_root(b.header)==id);
  const auto& n=v.nodes.at(id);require(n.slot==b.header.slot&&n.parent==b.header.parent_root);
  uint64_t genesis_seconds=0;for(unsigned i=0;i<8;++i)genesis_seconds|=uint64_t(b.state.ssz[i])<<(8*i);
  require(genesis_seconds<=UINT64_MAX/1000&&genesis_seconds*1000==c.genesis_time_ms&&b.header.slot<=(c.time_ms-c.genesis_time_ms)/r.slot_duration_ms);
  const auto known=cache?cache->records_.find(id):std::map<ConsensusRoot,Json>::iterator{};
  // Reuse only the canonical state encoding after comparing every source byte.
  // The complete row comparison below still covers all execution and metadata.
  Json encoded_state;
  if(cache&&known!=cache->records_.end()&&known->second.is_array()&&known->second.size()==4&&
     known->second[0].is_array()&&known->second[0].size()==6&&known->second[0][2].is_array()&&
     known->second[0][2].size()==2&&known->second[0][2][0].is_string()&&
     snapshot_detail::canonical_hex_matches(known->second[0][2][0].get_ref<const std::string&>(),b.state.ssz,cancel))
   encoded_state=Json::array({known->second[0][2][0],b.state.last_processed_epoch?Json(num(*b.state.last_processed_epoch)):Json(nullptr)});
  else encoded_state=slot_snapshot(b.state);
  auto row=Json::array({root(id),Json::array({num(b.header.slot),num(b.header.proposer_index),root(b.header.parent_root),root(b.header.state_root),root(b.header.body_root)}),std::move(encoded_state),num(n.realized_justified_epoch),num(n.unrealized_justified_epoch),execution(b,cancel)});
  // Compare every serialized field, including state/receipts/deposit history,
  // plus the checkpoint metadata checked below. Root equality alone is unsafe.
  const auto justification=cp(s.unrealized_justifications.at(id));
  const bool validated=cache&&known!=cache->records_.end()&&known->second.is_array()&&known->second.size()==4&&known->second[0]==row&&known->second[1]==justification&&known->second[2]==Json(id==v.anchor)&&known->second[3].is_string();
  if(!validated){
   validate_capella_execution_record(b,r,cancel);
   if(id!=v.anchor){const auto ffg=capella_fork_checkpoints(b.state,epoch_rules(r),r.transition.consensus.state,cancel);require(n.realized_justified_epoch==ffg.realized.current_justified.epoch&&n.unrealized_justified_epoch==ffg.unrealized.current_justified.epoch&&s.unrealized_justifications.at(id)==ffg.unrealized.current_justified);}
   if(cancel)cancel();if(cache)cache->records_.insert_or_assign(id,Json::array({row,justification,id==v.anchor,row.dump()}));
  }
  // String views remain valid: std::map insertions preserve other entries,
  // and this entry is never replaced again during this invocation.
  if(cache)encoded_rows.push_back(cache->records_.at(id)[3].get_ref<const std::string&>());
  // Cached row dumps already contain the complete, validated records. Keep
  // only array cardinality in the enclosing DOM, avoiding another deep copy
  // of every SSZ/execution record when the JSON object is assembled below.
  // The uncached path retains the full DOM and remains the byte-level oracle.
  if(cache)blocks.push_back(nullptr);else blocks.push_back(std::move(row));
 }
 for(const auto& [i,m]:v.latest_messages)messages.push_back(Json::array({num(i),num(m.epoch),root(m.root)}));
 for(auto i:v.equivocating_indices)equiv.push_back(num(i));
 for(const auto& [id,b]:s.timely){require(s.blocks.contains(id));timely.push_back(Json::array({root(id),b}));}
 for(const auto& [id,p]:s.unrealized_justifications){require(s.blocks.contains(id));unrealized.push_back(Json::array({root(id),cp(p)}));}
 Json ancestors=Json::array();require(s.blocks.contains(v.anchor));auto child=s.blocks.at(v.anchor).execution_block.header;
 require(child.number>=0&&child.number<=Int("18446744073709551615")&&s.anchor_execution_ancestors.size()==std::min<uint64_t>(u64(child.number),256));
 for(const auto& ancestor:s.anchor_execution_ancestors){if(cancel)cancel();require(ancestor.number+1==child.number&&ancestor.hash()==child.parent_hash);ancestors.push_back(ancestor.to_json());child=ancestor;}
 for(const auto& [id,b]:s.blocks)if(id!=v.anchor){const auto& parent=s.blocks.at(b.header.parent_root);require(parent.header.slot<b.header.slot&&parent.execution_block.header.number+1==b.execution_block.header.number&&parent.execution_block.header.hash()==b.execution_block.header.parent_hash);}
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
 auto encoded=cache?snapshot_detail::canonical_snapshot_rows(j,encoded_rows,l.envelope_bytes,cancel):j.dump();require(encoded.size()<=l.envelope_bytes);if(cancel)cancel();return encoded;
}
CapellaForkStore capella_store_from_snapshot(std::string_view bytes,const Bytes& digest,const CapellaForkBlockRules& r,
 const CapellaStoreSnapshotLimits& l,const std::function<void()>& cancel){
 require(bytes.size()<=l.envelope_bytes&&digest.size()==32);if(cancel)cancel();require(sha256(Bytes(bytes.begin(),bytes.end()))==digest);
 const auto j=Json::parse(bytes);require(j.is_object()&&j.contains("format")&&j.at("format").is_string());const bool pinned=j.value("format",std::string())=="nano-capella-store-v3";const bool pruned=pinned||j.value("format",std::string())=="nano-capella-store-v2";require(j.is_object()&&j.size()==(pinned?15:pruned?14:13)&&(pruned||j.at("format")=="nano-capella-store-v1")&&j.at("profile")==profile(r));
 require(j.at("blocks").is_array()&&j.at("blocks").size()<=l.blocks&&j.at("messages").is_array()&&j.at("messages").size()<=l.messages&&j.at("equivocations").is_array()&&j.at("equivocations").size()<=l.equivocations);
 require(j.at("timely").is_array()&&j.at("timely").size()<=l.blocks&&j.at("unrealized").is_array()&&j.at("unrealized").size()<=l.blocks);
 CapellaForkStore s;auto& v=s.timed.view;auto& c=s.timed.clock;
 if(pruned)s.network_anchor=root(j.at("networkAnchor"));
 v.anchor=root(j.at("anchor"));v.justified=cp(j.at("justified"));v.finalized=cp(j.at("finalized"));
 const auto& clock=j.at("clock");require(clock.is_array()&&clock.size()==4);c={number(clock[0]),number(clock[1]),cp(clock[2]),cp(clock[3])};
 require(r.slot_duration_ms&&r.transition.consensus.epoch.ffg.slots_per_epoch&&c.time_ms>=c.genesis_time_ms);v.current_epoch=(c.time_ms-c.genesis_time_ms)/r.slot_duration_ms/r.transition.consensus.epoch.ffg.slots_per_epoch;
 if(!j.at("boost").is_null())v.proposer_boost=root(j.at("boost"));size_t total=0;
 for(const auto& b:j.at("blocks")){
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
 require(capella_store_snapshot(s,r,l,cancel)==bytes);return s;
}
}

namespace nano {Json capella_fork_profile(const CapellaForkBlockRules& r){return profile(r);}}
