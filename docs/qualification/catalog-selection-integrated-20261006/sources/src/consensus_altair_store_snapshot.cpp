#include "nano/consensus_altair_store_snapshot.hpp"
#include <source_location>
namespace nano {
namespace {
void require(bool ok,const std::source_location at=std::source_location::current()){if(!ok)throw std::invalid_argument("invalid Altair Store snapshot at line "+std::to_string(at.line()));}
std::string num(uint64_t n){return quantity(Int(std::to_string(n)));}
uint64_t number(const Json& j){return u64(quantity(j));}
std::string root(const ConsensusRoot& r){return hex(Bytes(r.begin(),r.end()));}
ConsensusRoot root(const Json& j){require(j.is_string());const auto& s=j.get_ref<const std::string&>();require(s.size()==66&&s.starts_with("0x"));auto b=unhex(s);require(b.size()==32);ConsensusRoot r{};std::copy(b.begin(),b.end(),r.begin());return r;}
Json cp(const ConsensusCheckpoint& c){return Json::array({num(c.epoch),root(c.root)});}
ConsensusCheckpoint cp(const Json& j){require(j.is_array()&&j.size()==2);return {number(j[0]),root(j[1])};}
Json profile(const AltairForkBlockRules& r){return Json::array({num(r.slot_duration_ms),num(r.attestation_due_ms),num(r.proposer_score_boost),num(static_cast<uint64_t>(r.boost_profile))});}
ConsensusSlashingState registry(const AltairForkStore& s,const AltairForkBlockRules& r,const AltairStoreSnapshotLimits& l,const std::function<void()>& cancel){
 const auto& b=s.blocks.at(s.timed.view.justified.root);
 if(s.timed.view.justified.root==s.timed.view.anchor&&s.timed.view.justified.epoch==b.header.slot/r.transition.epoch.ffg.slots_per_epoch)return altair_fork_block_context(b.state,b.header,r.transition.epoch,r.transition.state,cancel).registry;
 return prepare_altair_fork_state_contexts(b.state,b.header,s.timed.view.justified,r.transition.epoch,r.transition.state,l.replay_slots,l.sync_trials,cancel).target.registry;
}
}
std::string altair_store_snapshot(const AltairForkStore& s,const AltairForkBlockRules& r,
 const AltairStoreSnapshotLimits& l,const std::function<void()>& cancel){
 const auto& v=s.timed.view;const auto& c=s.timed.clock;const auto slots=r.transition.epoch.ffg.slots_per_epoch;
 require(slots&&r.slot_duration_ms&&r.attestation_due_ms<=r.slot_duration_ms&&c.time_ms>=c.genesis_time_ms&&
  v.current_epoch==(c.time_ms-c.genesis_time_ms)/r.slot_duration_ms/slots&&
  (r.boost_profile==AltairProposerBoostProfile::Release160Beta0||r.boost_profile==AltairProposerBoostProfile::ShufflingDependent));
 require(!s.blocks.empty()&&s.blocks.size()<=l.blocks&&s.blocks.size()==v.nodes.size()&&v.latest_messages.size()<=l.messages&&v.equivocating_indices.size()<=l.equivocations);
 require(s.unrealized_justifications.size()==s.blocks.size()&&s.timely.size()+1==s.blocks.size()&&!s.timely.contains(v.anchor));
 require(v.justified_registry==registry(s,r,l,cancel));
 phase0_fork_choice_head(v,{slots,r.transition.attestation.duties.effective_balance_increment,r.proposer_score_boost},cancel);
 Json blocks=Json::array(),messages=Json::array(),equiv=Json::array(),timely=Json::array(),unrealized=Json::array();size_t bytes=0;
 for(const auto& [id,b]:s.blocks){
  if(cancel)cancel();require(b.state.ssz.size()<=l.state_bytes&&b.state.ssz.size()<=l.total_state_bytes-bytes);bytes+=b.state.ssz.size();
  require(consensus_beacon_header_root(b.header)==id);altair_fork_block_context(b.state,b.header,r.transition.epoch,r.transition.state,cancel);
  const auto& n=v.nodes.at(id);require(n.slot==b.header.slot&&n.parent==b.header.parent_root);
  blocks.push_back(Json::array({root(id),Json::array({num(b.header.slot),num(b.header.proposer_index),root(b.header.parent_root),root(b.header.state_root),root(b.header.body_root)}),altair_slot_snapshot(b.state,r.transition),num(n.realized_justified_epoch),num(n.unrealized_justified_epoch)}));
 }
 for(const auto& [i,m]:v.latest_messages)messages.push_back(Json::array({num(i),num(m.epoch),root(m.root)}));
 for(auto i:v.equivocating_indices)equiv.push_back(num(i));
 for(const auto& [id,b]:s.timely){require(s.blocks.contains(id));timely.push_back(Json::array({root(id),b}));}
 for(const auto& [id,p]:s.unrealized_justifications){require(s.blocks.contains(id));unrealized.push_back(Json::array({root(id),cp(p)}));}
 Json j{{"format","nano-altair-store-v1"},{"profile",profile(r)},{"anchor",root(v.anchor)},{"justified",cp(v.justified)},{"finalized",cp(v.finalized)},
 {"clock",Json::array({num(c.genesis_time_ms),num(c.time_ms),cp(c.unrealized_justified),cp(c.unrealized_finalized)})},
 {"boost",v.proposer_boost?Json(root(*v.proposer_boost)):Json(nullptr)},{"blocks",blocks},{"messages",messages},{"equivocations",equiv},{"timely",timely},{"unrealized",unrealized}};
 auto encoded=j.dump();require(encoded.size()<=l.envelope_bytes);if(cancel)cancel();return encoded;
}
AltairForkStore altair_store_from_snapshot(std::string_view bytes,const Bytes& digest,const AltairForkBlockRules& r,
 const AltairStoreSnapshotLimits& l,const std::function<void()>& cancel){
 require(bytes.size()<=l.envelope_bytes&&digest.size()==32);if(cancel)cancel();require(sha256(Bytes(bytes.begin(),bytes.end()))==digest);
 const auto j=Json::parse(bytes);require(j.is_object()&&j.size()==12&&j.at("format")=="nano-altair-store-v1"&&j.at("profile")==profile(r));
 require(j.at("blocks").is_array()&&j.at("blocks").size()<=l.blocks&&j.at("messages").is_array()&&j.at("messages").size()<=l.messages&&j.at("equivocations").is_array()&&j.at("equivocations").size()<=l.equivocations);
 require(j.at("timely").is_array()&&j.at("timely").size()<=l.blocks&&j.at("unrealized").is_array()&&j.at("unrealized").size()<=l.blocks);
 AltairForkStore s;auto& v=s.timed.view;auto& c=s.timed.clock;
 v.anchor=root(j.at("anchor"));v.justified=cp(j.at("justified"));v.finalized=cp(j.at("finalized"));
 const auto& clock=j.at("clock");require(clock.is_array()&&clock.size()==4);c={number(clock[0]),number(clock[1]),cp(clock[2]),cp(clock[3])};
 require(r.slot_duration_ms&&r.transition.epoch.ffg.slots_per_epoch&&c.time_ms>=c.genesis_time_ms);v.current_epoch=(c.time_ms-c.genesis_time_ms)/r.slot_duration_ms/r.transition.epoch.ffg.slots_per_epoch;
 if(!j.at("boost").is_null())v.proposer_boost=root(j.at("boost"));size_t total=0;
 for(const auto& b:j.at("blocks")){
  if(cancel)cancel();require(b.is_array()&&b.size()==5);const auto id=root(b[0]);const auto& h=b[1];require(h.is_array()&&h.size()==5);
  ConsensusBeaconHeader header{number(h[0]),number(h[1]),root(h[2]),root(h[3]),root(h[4])};
  auto state=altair_slot_from_snapshot(b[2],r.transition,header.state_root,std::min(l.state_bytes,l.total_state_bytes-total));total+=state.ssz.size();
  require(s.blocks.emplace(id,AltairForkBlock{header,std::move(state)}).second);v.nodes.emplace(id,Phase0ForkNode{header.slot,header.parent_root,number(b[3]),number(b[4])});
 }
 for(const auto& m:j.at("messages")){require(m.is_array()&&m.size()==3);require(v.latest_messages.emplace(number(m[0]),Phase0LatestMessage{number(m[1]),root(m[2])}).second);}
 for(const auto& i:j.at("equivocations"))require(v.equivocating_indices.insert(number(i)).second);
 for(const auto& t:j.at("timely")){require(t.is_array()&&t.size()==2&&t[1].is_boolean());require(s.timely.emplace(root(t[0]),t[1].get<bool>()).second);}
 for(const auto& p:j.at("unrealized")){require(p.is_array()&&p.size()==2);require(s.unrealized_justifications.emplace(root(p[0]),cp(p[1])).second);}
 v.justified_registry=registry(s,r,l,cancel);
 require(altair_store_snapshot(s,r,l,cancel)==bytes);return s;
}
}
