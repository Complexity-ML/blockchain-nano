#pragma once
#include "nano/consensus_capella_duty.hpp"
#include "nano/consensus_capella_store_snapshot.hpp"
#include "nano/consensus_capella_withdrawals.hpp"
#include "nano/consensus_beacon_journal_auth.hpp"
#include "nano/consensus_attestation.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <climits>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>

namespace nano::maintenance {
struct DutyReadinessKey {
 Bytes public_key;std::optional<uint64_t> configured_index;
 bool retired=false,pending=false;
 BeaconJournalReservations primary_reservations{},candidate_reservations{};
};
struct DutyReadinessBounds {
 uint64_t maximum_gap_slots,maximum_chunk_slots,maximum_chunks,maximum_retargets;
 uint64_t maximum_elapsed_ms,maximum_rss_kib,maximum_observation_age_ms,maximum_registry_entries;
};
class DutyReadinessRefused:public std::runtime_error {
public:explicit DutyReadinessRefused(const char* reason):std::runtime_error(reason){}
};
// PURE, owned inputs precede every callback. Cold records are deliberately
// rejected: the I/O caller must materialize them on authenticated RO replicas.
// This observes potential assignments, never a permission or signature.
inline Json evaluate_duty_readiness(CapellaForkStore store,CapellaForkBlockRules rules,
 CapellaStoreSnapshotLimits limits,std::vector<DutyReadinessKey> keys,DutyReadinessBounds bounds,
 std::function<uint64_t()> now_ms,std::function<void()> checkpoint,std::function<uint64_t()> rss_kib){
 using Clock=std::chrono::steady_clock;const auto began=Clock::now();
 auto require=[](bool ok,const char* why){if(!ok)throw DutyReadinessRefused(why);};
 require(bool(now_ms)&&bool(rss_kib),"missing_observation_callbacks");
 require(bounds.maximum_gap_slots&&bounds.maximum_chunk_slots&&bounds.maximum_chunks&&bounds.maximum_elapsed_ms&&bounds.maximum_observation_age_ms&&bounds.maximum_registry_entries&&bounds.maximum_rss_kib&&bounds.maximum_rss_kib<=131072&&bounds.maximum_elapsed_ms<=uint64_t(INT64_MAX),"invalid_preparation_bounds");
 const auto& t=rules.transition;const auto slots=t.consensus.epoch.ffg.slots_per_epoch;
 require(slots&&rules.slot_duration_ms&&limits.replay_slots&&limits.sync_trials&&bounds.maximum_chunk_slots<=std::min(limits.replay_slots,slots),"chunk_exceeds_actual_profile");
 require(t.consensus.epoch.history.slots_per_epoch==slots,"epoch_profile_mismatch");
 // A value-copy of CapellaForkStore shares mutable hot records. Detach before
 // the first external callback, including now/RSS, so a caller's preexisting
 // writable() reference cannot change this observation during a checkpoint.
 require(!keys.empty()&&keys.size()<=bounds.maximum_registry_entries&&store.blocks.size()<=limits.blocks,"selection_cardinality");
 size_t materialized=0;for(auto& [_,entry]:store.blocks){require(!entry.cold_reference(),"cold_record_requires_owned_materialization");const auto held=entry.lease();require(bool(held)&&held->state.ssz.size()<=limits.state_bytes&&held->state.ssz.size()<=limits.total_state_bytes-materialized,"owned_state_byte_limit");materialized+=held->state.ssz.size();entry=CapellaForkRecord(CapellaForkBlock(*held));}
 uint64_t chunks=0,charged=0,retargets=0,peak=0;
 const std::function<void()> check=[&]{if(checkpoint)checkpoint();const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-began).count();require(elapsed>=0&&uint64_t(elapsed)<bounds.maximum_elapsed_ms,"steady_deadline_exceeded");peak=std::max(peak,rss_kib());require(peak<=bounds.maximum_rss_kib,"rss_limit_exceeded");};
 auto charge=[&](uint64_t n){require(n<=bounds.maximum_gap_slots-charged,"total_slot_work_exceeded");charged+=n;};
 auto work=[&](uint64_t n){require(n<=bounds.maximum_chunks-chunks,"total_chunk_work_exceeded");chunks+=n;check();};
 check();
 std::set<Bytes> unique;for(const auto& key:keys){check();require(key.public_key.size()==48&&unique.insert(key.public_key).second,"configured_key_invalid_or_duplicate");}
 const auto head=capella_fork_head(store,rules,check);const auto head_root=consensus_beacon_header_root(head->header);
 const auto original_profile=capella_fork_profile(rules);const auto genesis=store.timed.clock.genesis_time_ms;
 uint64_t observed=now_ms(),last_wall=observed;
 require(observed>=genesis&&observed>=store.timed.clock.time_ms,"utc_before_current_or_genesis");
 uint64_t slot=(observed-genesis)/rules.slot_duration_ms;require(slot<UINT64_MAX-1&&slot>=head->header.slot,"invalid_current_slot");
 const auto tick_to=[&](uint64_t wall){
  std::optional<CapellaTargetPreparation> tick_target;std::array<const CapellaTargetPreparation*,1> targets{};size_t count=0;
  const auto& c=store.timed.clock;const auto epoch=(wall-genesis)/rules.slot_duration_ms/slots;
  if(epoch>store.timed.view.current_epoch&&c.unrealized_justified.epoch>store.timed.view.justified.epoch){
   require(c.unrealized_justified.epoch<=UINT64_MAX/slots,"tick_target_overflow");const auto start=c.unrealized_justified.epoch*slots;
   require(store.blocks.contains(c.unrealized_justified.root),"tick_target_missing");const auto source_slot=store.blocks.at(c.unrealized_justified.root).header().slot;require(source_slot<=start,"tick_target_future");charge(start-source_slot);
   tick_target.emplace(store,rules,c.unrealized_justified,check);while(!tick_target->ready()){work(1);tick_target->advance(bounds.maximum_chunk_slots,limits.sync_trials,check);}targets[0]=&*tick_target;count=1;
  }
  work(1);store=prepare_capella_contextual_store_tick(store,wall,rules,std::span<const CapellaTargetPreparation* const>(targets.data(),count),bounds.maximum_chunk_slots,limits.sync_trials,check);require(capella_fork_head(store,rules,check)->header==head->header,"ephemeral_tick_changed_head");
 };
 tick_to(observed);CapellaDutyPreparation prepared(store,rules,slot,check);
 charge(slot-head->header.slot);charge(slot>head->header.slot?slot-1-head->header.slot:0);
 uint64_t target_start=slot/slots*slots;
 auto boundary=head_root;while(store.timed.view.nodes.at(boundary).slot>target_start){check();if(boundary==store.timed.view.anchor)break;const auto& node=store.timed.view.nodes.at(boundary);require(store.timed.view.nodes.contains(node.parent)&&store.timed.view.nodes.at(node.parent).slot<node.slot,"invalid_target_ancestry");boundary=node.parent;}
 const auto boundary_slot=store.blocks.at(boundary).header().slot;if(boundary_slot<=target_start)charge(target_start-boundary_slot);charge(slot>head->header.slot?2:1);
 bool done=false;
 for(;;){
  check();if(bounds.maximum_chunks-chunks<3)break;
  work(3);done=prepared.advance(bounds.maximum_chunk_slots,limits.sync_trials,check);
  if(!done)continue;
  const auto fresh=now_ms();require(fresh>=last_wall,"utc_regressed");last_wall=fresh;require(fresh>=genesis,"utc_before_genesis");const auto fresh_slot=(fresh-genesis)/rules.slot_duration_ms;
  require(fresh_slot<UINT64_MAX-1&&fresh_slot>=slot,"invalid_observation_slot");
  if(fresh_slot==slot&&fresh-observed<=bounds.maximum_observation_age_ms)break;
  require(retargets<bounds.maximum_retargets,"observation_expired");++retargets;
  const auto delta=fresh_slot-slot;require(delta<=UINT64_MAX/2,"retarget_overflow");charge(delta);charge(delta);charge((fresh_slot/slots*slots)-target_start);if(slot==head->header.slot&&fresh_slot>slot)charge(1);
  tick_to(fresh);require(prepared.retarget(store,rules,fresh_slot,check),"retarget_refused");slot=fresh_slot;target_start=slot/slots*slots;observed=fresh;done=false;
 }
 Json result={{"format","nano-capella-duty-readiness-evaluation-v1"},{"result",done?"PREPARED_READONLY_ONLY":"UNAVAILABLE_READONLY_ONLY"},{"probeCompleted",true},{"configuredKeys",keys.size()},{"headProjectionReady",done},{"targetEpochReady",false},{"blockStateReady",false},{"syncCommitteeReady",false},{"completeDutyContexts",false},{"normalNonassignmentPossible",true},{"asOfUtcMs",observed},{"slot",slot},{"chargedSlotWork",charged},{"chunks",chunks},{"retargets",retargets},{"maximumRssKiB",bounds.maximum_rss_kib},{"actualPeakRssKiB",peak},{"sourceHeadRoot",hex(Bytes(head_root.begin(),head_root.end()))},{"profile",original_profile},{"keys",Json::array()},{"resumeAuthorized",false},{"headProjectionPersisted",false},{"unsignedBodyConstructionQualified",false},{"newSignatures",0},{"runtimeStarted",false},{"productionQualified",false}};
 if(!done){result["reason"]="preparation_unavailable_or_chunk_limit";check();return result;}
 const auto* target=prepared.target_preparation();const auto* block=prepared.block();
 result["targetEpochReady"]=target&&target->ready();result["blockStateReady"]=bool(block);
 const auto& state=prepared.head_state();const auto root=capella_beacon_state_root(state.ssz,t.consensus.state,t.execution,t.maximum_state_bytes,check);
 auto registry=read_capella_withdrawal_state(state.ssz,root,t.consensus.state,t.execution,t.maximum_state_bytes,slots,check).registry;
 require(registry.validators.size()<=bounds.maximum_registry_entries,"projected_registry_cardinality");
 const auto duties=prepared.duties(store,check);if(block)work(1);const auto proposer=block?block->proposer_for_block(slot,limits.sync_trials,check):std::nullopt;
 work(1);const auto sync=prepared.try_sync_state(limits.sync_trials,check);result["syncCommitteeReady"]=bool(sync);
 std::set<Bytes> sync_keys;
 if(sync){
  const auto sync_root=capella_beacon_state_root(sync->ssz,t.consensus.state,t.execution,t.maximum_state_bytes,check);
  (void)read_capella_withdrawal_state(sync->ssz,sync_root,t.consensus.state,t.execution,t.maximum_state_bytes,slots,check);
  const auto add=[&](size_t a,size_t b){require(b<=SIZE_MAX-a,"sync_offset_overflow");return a+b;};const auto mul=[&](size_t a,size_t b){require(!a||b<=SIZE_MAX/a,"sync_offset_overflow");return a*b;};const auto& l=t.consensus.state;
  const auto at=add(add(add(add(176,mul(64,l.slots_per_historical_root)),96),mul(32,l.epochs_per_historical_vector)),add(mul(8,l.epochs_per_slashings_vector),133));
  require(at<=sync->ssz.size()&&mul(48,l.sync_committee_size)<=sync->ssz.size()-at,"sync_committee_bounds");
  for(size_t i=0;i<l.sync_committee_size;++i){check();sync_keys.emplace(sync->ssz.begin()+at+48*i,sync->ssz.begin()+at+48*(i+1));}
 }
 uint64_t active=0;Int effective=0;std::map<Bytes,uint64_t> indices;
 for(const auto& [index,v]:registry.validators){check();require(indices.emplace(Bytes(v.public_key.begin(),v.public_key.end()),index).second,"duplicate_registry_key");if(v.activation_epoch<=registry.epoch&&registry.epoch<v.exit_epoch){++active;effective+=Int(std::to_string(v.effective_balance));}}
 result["activeValidators"]=active;result["activeEffectiveBalanceGwei"]=effective.get_str();result["projectedStateRoot"]=hex(Bytes(root.begin(),root.end()));result["projectedEpoch"]=registry.epoch;
 const auto reservation_allows=[&](const BeaconJournalReservations& r,const CapellaAttestationDuty& duty){if(r.retired)return false;if(!r.attestation)return true;const auto& a=*r.attestation;const auto& d=duty.attestation.data;if(d.source.epoch<a.source||d.target.epoch<a.target)return false;if(d.target.epoch!=a.target)return true;const auto signing=consensus_signing_root(consensus_attestation_root(d),consensus_domain({1,0,0,0},duty.fork_version,duty.genesis_root));return d.source.epoch==a.source&&a.root&&signing==*a.root;};
 for(const auto& key:keys){
  check();const auto found=indices.find(key.public_key);std::optional<uint64_t> index;if(found!=indices.end())index=found->second;
  require(!key.configured_index||(index&&index==key.configured_index),"configured_index_mismatch");bool is_active=false,slashed=false;uint64_t exit=0;
  if(index){const auto& v=registry.validators.at(*index);is_active=v.activation_epoch<=registry.epoch&&registry.epoch<v.exit_epoch;slashed=v.slashed;exit=v.exit_epoch;}
  const bool retired=key.retired||key.primary_reservations.retired||key.candidate_reservations.retired;
  const bool att=index&&duties.contains(*index),prop=index&&proposer&&proposer->index==*index&&proposer->public_key==key.public_key,sy=index&&sync_keys.contains(key.public_key);
  const bool att_safe=att&&reservation_allows(key.primary_reservations,duties.at(*index))&&reservation_allows(key.candidate_reservations,duties.at(*index));
  const bool prop_reserved=(key.primary_reservations.proposal&&key.primary_reservations.proposal->slot>=slot)||(key.candidate_reservations.proposal&&key.candidate_reservations.proposal->slot>=slot);
  const char* classification=retired?"retired":key.pending?"pending":!index?"unregistered":slashed?"slashed":!is_active?"inactive":!(att||prop||sy)?"nonassigned":"assigned";
  result["keys"].push_back({{"publicKey",hex(key.public_key)},{"configuredIndex",key.configured_index?Json(*key.configured_index):Json(nullptr)},{"registeredIndex",index?Json(*index):Json(nullptr)},{"classification",classification},{"active",is_active},{"slashed",slashed},{"exitEpoch",exit},{"retired",retired},{"pending",key.pending},{"attestationAssigned",att},{"proposalAssigned",prop},{"syncAssigned",sy},{"attestationReservationAllows",att_safe},{"proposalReservationCheckRequired",prop&&prop_reserved},{"attestationEligible",att&&is_active&&!slashed&&!retired&&!key.pending&&att_safe},{"proposalPotentiallyEligible",prop&&is_active&&!slashed&&!retired&&!key.pending&&!prop_reserved},{"syncPotentiallyEligible",sy&&!retired&&!key.pending}});
 }
 const auto finished=now_ms();require(finished>=last_wall&&finished>=observed&&finished-observed<=bounds.maximum_observation_age_ms&&(finished-genesis)/rules.slot_duration_ms==slot,"observation_expired");check();
 result["finishedUtcMs"]=finished;result["completeDutyContexts"]=bool(target&&target->ready()&&block&&proposer&&sync&&active);result["actualPeakRssKiB"]=peak;result["chunks"]=chunks;return result;
}
} // namespace nano::maintenance
