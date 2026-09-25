#include "nano/consensus_bellatrix_store_snapshot.hpp"
#include <source_location>
namespace nano {
namespace {
void require(bool ok,const std::source_location at=std::source_location::current()){if(!ok)throw std::invalid_argument("invalid Bellatrix Store snapshot at line "+std::to_string(at.line()));}
std::string num(uint64_t n){return quantity(Int(std::to_string(n)));}
uint64_t number(const Json& j){return u64(quantity(j));}
std::string root(const ConsensusRoot& r){return hex(Bytes(r.begin(),r.end()));}
ConsensusRoot root(const Json& j){require(j.is_string());const auto& s=j.get_ref<const std::string&>();require(s.size()==66&&s.starts_with("0x"));auto b=unhex(s);require(b.size()==32);ConsensusRoot r{};std::copy(b.begin(),b.end(),r.begin());return r;}
Json cp(const ConsensusCheckpoint& c){return Json::array({num(c.epoch),root(c.root)});}
ConsensusCheckpoint cp(const Json& j){require(j.is_array()&&j.size()==2);return {number(j[0]),root(j[1])};}
BellatrixEpochRules epoch_rules(const BellatrixForkBlockRules& r){return {r.transition.consensus.epoch,r.transition.execution,r.transition.maximum_state_bytes};}
Json profile(const BellatrixForkBlockRules& r){return Json::array({altair_transition_profile(r.transition.consensus),num(r.slot_duration_ms),num(r.attestation_due_ms),num(r.proposer_score_boost),num(static_cast<uint64_t>(r.boost_profile)),quantity(r.transition.chain_id),num(static_cast<uint64_t>(r.transition.replay_policy)),num(r.transition.seconds_per_slot),num(r.transition.maximum_state_bytes),num(r.transition.execution.extra_data_bytes),num(r.transition.execution.transaction_bytes),num(r.transition.execution.transactions)});}
Json slot_snapshot(const BellatrixSlotState& s){return Json::array({hex(s.ssz),s.last_processed_epoch?Json(num(*s.last_processed_epoch)):Json(nullptr)});}
BellatrixSlotState slot_restore(const Json& j,size_t budget){require(j.is_array()&&j.size()==2&&j[0].is_string());const auto& hexstate=j[0].get_ref<const std::string&>();require(hexstate.starts_with("0x")&&hexstate.size()%2==0&&(hexstate.size()-2)/2<=budget);BellatrixSlotState s{unhex(hexstate),std::nullopt};if(!j[1].is_null())s.last_processed_epoch=number(j[1]);return s;}
Json execution(const BellatrixForkBlock& b){Json receipts=Json::array();for(const auto& receipt:b.receipts)receipts.push_back(receipt.to_json());return Json::array({b.execution_block.to_json(true),b.execution_state.to_json(),receipts});}
ConsensusSlashingState registry(const BellatrixForkStore& s,const BellatrixForkBlockRules& r,const BellatrixStoreSnapshotLimits& l,const std::function<void()>& cancel){
 const auto& b=s.blocks.at(s.timed.view.justified.root);
 if(s.timed.view.justified.root==s.timed.view.anchor&&s.timed.view.justified.epoch==b.header.slot/r.transition.consensus.epoch.ffg.slots_per_epoch)return bellatrix_fork_block_context(b.state,b.header,epoch_rules(r),r.transition.consensus.state,cancel).registry;
 return prepare_bellatrix_fork_state_contexts(b.state,b.header,s.timed.view.justified,epoch_rules(r),r.transition.consensus.state,l.replay_slots,l.sync_trials,cancel).target.registry;
}
}
std::string bellatrix_store_snapshot(const BellatrixForkStore& s,const BellatrixForkBlockRules& r,
 const BellatrixStoreSnapshotLimits& l,const std::function<void()>& cancel){
 const auto& v=s.timed.view;const auto& c=s.timed.clock;const auto slots=r.transition.consensus.epoch.ffg.slots_per_epoch;
 require(slots&&r.slot_duration_ms&&r.attestation_due_ms<=r.slot_duration_ms&&c.time_ms>=c.genesis_time_ms&&
  v.current_epoch==(c.time_ms-c.genesis_time_ms)/r.slot_duration_ms/slots&&
  (r.boost_profile==BellatrixProposerBoostProfile::Release160Beta0||r.boost_profile==BellatrixProposerBoostProfile::ShufflingDependent));
 require(!s.blocks.empty()&&s.blocks.size()<=l.blocks&&s.blocks.size()==v.nodes.size()&&v.latest_messages.size()<=l.messages&&v.equivocating_indices.size()<=l.equivocations);
 require(s.unrealized_justifications.size()==s.blocks.size()&&s.timely.size()+1==s.blocks.size()&&!s.timely.contains(v.anchor));
 require(v.justified_registry==registry(s,r,l,cancel));
 phase0_fork_choice_head(v,{slots,r.transition.consensus.attestation.duties.effective_balance_increment,r.proposer_score_boost},cancel);
 Json blocks=Json::array(),messages=Json::array(),equiv=Json::array(),timely=Json::array(),unrealized=Json::array();size_t bytes=0;
 for(const auto& [id,b]:s.blocks){
  if(cancel)cancel();require(b.state.ssz.size()<=l.state_bytes&&b.state.ssz.size()<=l.total_state_bytes-bytes);bytes+=b.state.ssz.size();
  require(consensus_beacon_header_root(b.header)==id);validate_bellatrix_execution_record(b,r,cancel);
  const auto& n=v.nodes.at(id);require(n.slot==b.header.slot&&n.parent==b.header.parent_root);
  uint64_t genesis_seconds=0;for(unsigned i=0;i<8;++i)genesis_seconds|=uint64_t(b.state.ssz[i])<<(8*i);
  require(genesis_seconds<=UINT64_MAX/1000&&genesis_seconds*1000==c.genesis_time_ms&&b.header.slot<=(c.time_ms-c.genesis_time_ms)/r.slot_duration_ms);
  if(id!=v.anchor){const auto ffg=bellatrix_fork_checkpoints(b.state,epoch_rules(r),r.transition.consensus.state,cancel);require(n.realized_justified_epoch==ffg.realized.current_justified.epoch&&n.unrealized_justified_epoch==ffg.unrealized.current_justified.epoch&&s.unrealized_justifications.at(id)==ffg.unrealized.current_justified);}
  blocks.push_back(Json::array({root(id),Json::array({num(b.header.slot),num(b.header.proposer_index),root(b.header.parent_root),root(b.header.state_root),root(b.header.body_root)}),slot_snapshot(b.state),num(n.realized_justified_epoch),num(n.unrealized_justified_epoch),execution(b)}));
 }
 for(const auto& [i,m]:v.latest_messages)messages.push_back(Json::array({num(i),num(m.epoch),root(m.root)}));
 for(auto i:v.equivocating_indices)equiv.push_back(num(i));
 for(const auto& [id,b]:s.timely){require(s.blocks.contains(id));timely.push_back(Json::array({root(id),b}));}
 for(const auto& [id,p]:s.unrealized_justifications){require(s.blocks.contains(id));unrealized.push_back(Json::array({root(id),cp(p)}));}
 Json ancestors=Json::array();require(s.blocks.contains(v.anchor));auto child=s.blocks.at(v.anchor).execution_block.header;
 require(child.number>=0&&child.number<=Int("18446744073709551615")&&s.anchor_execution_ancestors.size()==std::min<uint64_t>(u64(child.number),256));
 for(const auto& ancestor:s.anchor_execution_ancestors){if(cancel)cancel();require(ancestor.number+1==child.number&&ancestor.hash()==child.parent_hash);ancestors.push_back(ancestor.to_json());child=ancestor;}
 for(const auto& [id,b]:s.blocks)if(id!=v.anchor){const auto& parent=s.blocks.at(b.header.parent_root);require(parent.header.slot<b.header.slot&&parent.execution_block.header.number+1==b.execution_block.header.number&&parent.execution_block.header.hash()==b.execution_block.header.parent_hash);}
 Json j{{"ancestors",ancestors},{"format","nano-bellatrix-store-v1"},{"profile",profile(r)},{"anchor",root(v.anchor)},{"justified",cp(v.justified)},{"finalized",cp(v.finalized)},
 {"clock",Json::array({num(c.genesis_time_ms),num(c.time_ms),cp(c.unrealized_justified),cp(c.unrealized_finalized)})},
 {"boost",v.proposer_boost?Json(root(*v.proposer_boost)):Json(nullptr)},{"blocks",blocks},{"messages",messages},{"equivocations",equiv},{"timely",timely},{"unrealized",unrealized}};
 auto encoded=j.dump();require(encoded.size()<=l.envelope_bytes);if(cancel)cancel();return encoded;
}
BellatrixForkStore bellatrix_store_from_snapshot(std::string_view bytes,const Bytes& digest,const BellatrixForkBlockRules& r,
 const BellatrixStoreSnapshotLimits& l,const std::function<void()>& cancel){
 require(bytes.size()<=l.envelope_bytes&&digest.size()==32);if(cancel)cancel();require(sha256(Bytes(bytes.begin(),bytes.end()))==digest);
 const auto j=Json::parse(bytes);require(j.is_object()&&j.size()==13&&j.at("format")=="nano-bellatrix-store-v1"&&j.at("profile")==profile(r));
 require(j.at("blocks").is_array()&&j.at("blocks").size()<=l.blocks&&j.at("messages").is_array()&&j.at("messages").size()<=l.messages&&j.at("equivocations").is_array()&&j.at("equivocations").size()<=l.equivocations);
 require(j.at("timely").is_array()&&j.at("timely").size()<=l.blocks&&j.at("unrealized").is_array()&&j.at("unrealized").size()<=l.blocks);
 BellatrixForkStore s;auto& v=s.timed.view;auto& c=s.timed.clock;
 v.anchor=root(j.at("anchor"));v.justified=cp(j.at("justified"));v.finalized=cp(j.at("finalized"));
 const auto& clock=j.at("clock");require(clock.is_array()&&clock.size()==4);c={number(clock[0]),number(clock[1]),cp(clock[2]),cp(clock[3])};
 require(r.slot_duration_ms&&r.transition.consensus.epoch.ffg.slots_per_epoch&&c.time_ms>=c.genesis_time_ms);v.current_epoch=(c.time_ms-c.genesis_time_ms)/r.slot_duration_ms/r.transition.consensus.epoch.ffg.slots_per_epoch;
 if(!j.at("boost").is_null())v.proposer_boost=root(j.at("boost"));size_t total=0;
 for(const auto& b:j.at("blocks")){
  if(cancel)cancel();require(b.is_array()&&b.size()==6);const auto id=root(b[0]);const auto& h=b[1];require(h.is_array()&&h.size()==5);
  ConsensusBeaconHeader header{number(h[0]),number(h[1]),root(h[2]),root(h[3]),root(h[4])};
  auto state=slot_restore(b[2],std::min(l.state_bytes,l.total_state_bytes-total));total+=state.ssz.size();
  require(b[5].is_array()&&b[5].size()==3&&b[5][2].is_array());std::vector<Receipt> receipts;for(const auto& receipt:b[5][2]){if(cancel)cancel();receipts.push_back(Receipt::from_json(receipt));}
  require(s.blocks.emplace(id,BellatrixForkBlock{header,std::move(state),Block::from_json(b[5][0]),State::from_json(b[5][1]),std::move(receipts)}).second);v.nodes.emplace(id,Phase0ForkNode{header.slot,header.parent_root,number(b[3]),number(b[4])});
 }
 for(const auto& m:j.at("messages")){require(m.is_array()&&m.size()==3);require(v.latest_messages.emplace(number(m[0]),Phase0LatestMessage{number(m[1]),root(m[2])}).second);}
 for(const auto& i:j.at("equivocations"))require(v.equivocating_indices.insert(number(i)).second);
 for(const auto& t:j.at("timely")){require(t.is_array()&&t.size()==2&&t[1].is_boolean());require(s.timely.emplace(root(t[0]),t[1].get<bool>()).second);}
 for(const auto& p:j.at("unrealized")){require(p.is_array()&&p.size()==2);require(s.unrealized_justifications.emplace(root(p[0]),cp(p[1])).second);}
 require(j.at("ancestors").is_array()&&j.at("ancestors").size()<=256);for(const auto& header:j.at("ancestors"))s.anchor_execution_ancestors.push_back(Header::from_json(header));
 v.justified_registry=registry(s,r,l,cancel);
 require(bellatrix_store_snapshot(s,r,l,cancel)==bytes);return s;
}
}
