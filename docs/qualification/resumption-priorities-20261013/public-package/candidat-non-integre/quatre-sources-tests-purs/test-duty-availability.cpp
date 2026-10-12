// Actual pure preparation/selection code, no reactor ABI, DB, signer or network.
#include "public-duty-context.hpp"
namespace {
Json results=Json::array();
template<class E,class F> void expect(F f,const char* message){bool caught=false;try{f();}catch(const E&){caught=true;}need(caught,message);}
void done(const char* name){results.push_back({{"case",name},{"result","PASS"}});}
CapellaEpochRules epochs(const CapellaForkBlockRules& r){return {r.transition.consensus.epoch,r.transition.execution,r.transition.maximum_state_bytes};}
void reach_blocked(CapellaDutyPreparation& frame){for(size_t i=0;i<4;++i){need(!frame.advance(16,100000,check),"exit8 projection must never become ready");}need(frame.prepared_slot()==49&&!frame.ready(),"blocked frame retains complete prefix49");}
CapellaForkStore alternate_parent(const PublicDutyContext& original,bool keep_exits){
 auto store=original.store;const auto pin=capella_fork_head(store,original.rules,check);auto candidate=*pin;auto& bytes=candidate.state.ssz;const auto& t=original.rules.transition;const auto& l=t.consensus.state;
 const size_t historical=176+64*l.slots_per_historical_root;const auto first=number(bytes,historical+88,4),end=number(bytes,historical+92,4),count=(end-first)/121;
 for(size_t i=0;i<count;++i){if(!keep_exits){put(bytes,first+121*i+105,UINT64_MAX);put(bytes,first+121*i+113,UINT64_MAX);}}
 if(keep_exits){put(bytes,first+105,9);put(bytes,first+113,265);}
 candidate.header.state_root=capella_beacon_state_root(bytes,l,t.execution,t.maximum_state_bytes,check);const auto old=consensus_beacon_header_root(pin->header),id=consensus_beacon_header_root(candidate.header);
 const auto context=capella_fork_block_context(candidate.state,candidate.header,epochs(original.rules),l,check);validate_capella_execution_record(candidate,original.rules,check);
 auto node=store.timed.view.nodes.at(old);store.blocks.clear();store.blocks.emplace(id,CapellaForkRecord(candidate));store.timed.view.nodes.clear();store.timed.view.nodes.emplace(id,node);
 store.timed.view.anchor=id;store.timed.view.justified={0,id};store.timed.view.finalized={0,id};store.timed.view.justified_registry=context.registry;
 store.timed.clock.unrealized_justified={0,id};store.timed.clock.unrealized_finalized={0,id};store.unrealized_justifications.clear();store.unrealized_justifications.emplace(id,ConsensusCheckpoint{0,id});
 need(id!=old&&capella_fork_head(store,original.rules,check)->header==candidate.header,"alternate trusted synthetic parent must authenticate");return store;
}
}
int main(int argc,char** argv){try{
 need(argc==3,"arguments: public initial.json public profile.json");const auto ctx=load_public_duty_context(argv[1],argv[2]);const auto head=capella_fork_head(ctx.store,ctx.rules,check);
 const CapellaStoreSnapshotLimits limits{134217728,256,2048,1000,4194304,67108864,64,100000};const auto before=capella_store_snapshot(ctx.store,ctx.rules,limits,check);
 {
  CapellaSlotPreparation frame(head->state,head->header,epochs(ctx.rules),ctx.rules.transition.consensus.state,check);
  bool typed=false;try{frame.advance(64,64,100000,check);}catch(const EmptyAltairSyncActiveSet& e){typed=std::string(e.what())=="empty Altair sync active set";}
  need(typed&&frame.slot()==1,"nonlocal projection must still throw typed refusal without committing failed chunk");done("real-selection-typed-error-keeps-message-and-rejection");
 }
 CapellaDutyPreparation blocked(ctx.store,ctx.rules,64,check);reach_blocked(blocked);done("real-duty-preparation-defers-empty-selection-before-signing");
 {
  uint64_t callbacks=0;need(!blocked.advance(16,100000,[&]{++callbacks;check();}),"blocked must defer");need(callbacks==1&&blocked.prepared_slot()==49,"blocked poll must only check cancellation and not reproject");done("repeated-blocked-poll-no-replay-work");
  expect<std::logic_error>([&]{blocked.head_state();},"blocked head access");expect<std::logic_error>([&]{blocked.block();},"blocked block access");expect<std::logic_error>([&]{blocked.target_preparation();},"blocked target access");expect<std::logic_error>([&]{blocked.sync_state(100000,check);},"blocked sync access");
  need(blocked.duties(ctx.store,check).empty()&&!blocked.target_context(ctx.store,{8,consensus_beacon_header_root(head->header)},check),"blocked must emit no duty or targetcontext");done("blocked-ready-and-accessors-refuse-derived-duty-data");
 }
 {
  uint64_t callbacks=0;expect<std::invalid_argument>([&]{blocked.advance(0,100000,[&]{++callbacks;});},"blocked zero replay budget");expect<std::invalid_argument>([&]{blocked.advance(16,0,[&]{++callbacks;});},"blocked zero sync budget");need(callbacks==0&&!blocked.ready(),"bounds validated before fast path/callback");
  CapellaDutyPreparation fresh(ctx.store,ctx.rules,64,check);expect<std::invalid_argument>([&]{fresh.advance(0,100000,check);},"fresh zero budget");need(!fresh.advance(16,100000,check)&&fresh.prepared_slot()==17,"zero bounds must not block fresh frame");done("positive-budgets-required-on-fresh-and-blocked-frames");
 }
 {
  CapellaDutyPreparation frame(ctx.store,ctx.rules,64,check);uint64_t callbacks=0;
  expect<EmptyAltairSyncActiveSet>([&]{frame.advance(16,100000,[&]{if(++callbacks==2)throw EmptyAltairSyncActiveSet();check();});},"typed callback inside preparation must propagate");
  need(callbacks==2&&frame.prepared_slot()==1,"typed cancellation must not commit failed prefix");need(!frame.advance(16,100000,check)&&frame.prepared_slot()==17,"typed callback must not cache unavailable head");done("typed-callback-error-is-not-cached-as-selection-refusal");
  expect<EmptyAltairSyncActiveSet>([&]{blocked.advance(16,100000,[]{throw EmptyAltairSyncActiveSet();});},"blocked callback must still propagate");need(blocked.prepared_slot()==49&&!blocked.ready(),"existing blocked state remains unavailable");done("blocked-fastpath-keeps-typed-cancellation-visible");
 }
 {
  CapellaDutyPreparation frame(ctx.store,ctx.rules,64,check);uint64_t callbacks=0;
  expect<std::invalid_argument>([&]{frame.advance(16,100000,[&]{if(++callbacks==2)throw std::invalid_argument("distinct callback failure");check();});},"untyped callback must propagate");
  need(!frame.advance(16,100000,check)&&frame.prepared_slot()==17,"other callback errors must not block frame");done("untyped-callback-error-propagates-without-caching");
 }
 {
  const auto store=alternate_parent(ctx,false);CapellaDutyPreparation frame(store,ctx.rules,64,check);bool failed=false;
  try{for(size_t i=0;i<8;++i){if(frame.advance(16,1,check))break;}}catch(const std::length_error& e){failed=std::string(e.what())=="Altair sync sampling work budget";}
  need(failed&&frame.prepared_slot()==49,"real nonempty sampling budget failure must still throw");
  size_t polls=0;while(!frame.advance(16,100000,check)){need(++polls<12,"nonempty recovery bounded");}need(frame.ready(),"work error must not cache unavailable head");done("real-sampling-resource-error-not-converted-to-availability");
 }
 {
  auto profile=ctx.rules;profile.proposer_score_boost+=1;need(!blocked.matches(ctx.store,profile,64,check)&&!blocked.retarget(ctx.store,profile,65,check),"blocked must preserve exact profile binding");
  need(!blocked.retarget(ctx.store,ctx.rules,63,check)&&!blocked.retarget(ctx.store,ctx.rules,UINT64_MAX,check),"backwards/overflow retarget rejected");
  need(blocked.retarget(ctx.store,ctx.rules,65,check)&&blocked.matches(ctx.store,ctx.rules,65,check)&&!blocked.matches(ctx.store,ctx.rules,64,check),"same immutable head can move to next slot without reprojection");uint64_t callbacks=0;need(!blocked.advance(16,100000,[&]{++callbacks;check();})&&callbacks==1&&blocked.prepared_slot()==49,"blocked retarget must preserve prefix/cache");done("blocked-retarget-preserves-head-profile-and-monotone-slot");
 }
 {
  const auto newstore=alternate_parent(ctx,true);need(!blocked.matches(newstore,ctx.rules,65,check)&&!blocked.retarget(newstore,ctx.rules,65,check),"new trusted parent must reject old continuation");
  CapellaDutyPreparation fresh(newstore,ctx.rules,64,check);need(fresh.prepared_slot()==1,"new continuation cannot inherit old blocked prefix");size_t polls=0;while(!fresh.advance(16,100000,check)){need(++polls<12,"new parent pure preparation bounded");}need(fresh.ready()&&fresh.prepared_slot()==64,"fresh independently bound context can prepare");done("new-trusted-synthetic-parent-rejects-old-cache-and-new-object-resets");
 }
 need(before==capella_store_snapshot(ctx.store,ctx.rules,limits,check),"source Store changed across all cases");check();need(rss()<=131072,"terminal RSS cap");
 std::cout<<Json{{"format","nano-duty-projection-availability-candidate-pure-tests-v1"},{"result","PASS"},{"cases",results},{"caseCount",results.size()},{"elapsedMs",elapsed()},{"maxRssKiB",rss()},{"sourceStoreUnchanged",true},{"signaturesCreated",0},{"journalDatabaseOpened",false},{"reactorConstructed",false},{"newPeerHeadAdmitted",false},{"runtimeStarted",false},{"networkStarted",false},{"syncStatePostReadyBoundaryQualified",false},{"providerFailureQualified",false},{"generalReplayVariantsQualified",false},{"productionQualified",false}}.dump()<<std::endl;return 0;
 }catch(const std::exception& e){std::cout<<Json{{"result","FAIL"},{"exception",e.what()},{"completedCases",results},{"elapsedMs",elapsed()},{"maxRssKiB",rss()}}.dump()<<std::endl;return 1;}}
