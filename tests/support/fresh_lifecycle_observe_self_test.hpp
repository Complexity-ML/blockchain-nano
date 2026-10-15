#pragma once
#include "fresh_lifecycle.hpp"
#include "capella_stream_signed_fixture.hpp"
// Only the new observe selector invokes this tiny genuine signed fixture.
// No old action-policy groups or 1026-block stream are replayed.
namespace nano::test::fresh_lifecycle {
inline int observe_self_test(){
 namespace fixture=nano::test::canonical_stream_fixture;
 try{
  const auto began=std::chrono::steady_clock::now();const auto signatures_before=fixture::test_signatures;
  const auto check=[&]{require(std::chrono::steady_clock::now()-began<std::chrono::seconds(60),"observe self-test wall bound");fixture::checkpoint();};
  size_t refusals=0,groups=0,positives=0,archive_reads=0;
  auto bad=[&](auto operation){bool refused=false;try{operation();}catch(const std::exception&){refused=true;}require(refused,"new observe expected refusal");++refusals;};
  fixture::LongFixture generated(1,false);check();const auto& f=generated.f;const auto id=consensus_beacon_header_root(generated.head);const auto& wire=generated.archives.at(id).payload;
  auto state=prepare_capella_resumed_store_tick(f.initial,f.initial.timed.clock.genesis_time_ms+generated.head.slot*f.rules.slot_duration_ms,f.rules,f.limits.replay_slots,f.limits.sync_trials,check);
  const auto admitted=prepare_capella_resumed_fork_block(state,wire,f.rules,f.limits.replay_slots,f.limits.sync_trials,check);require(admitted&&capella_fork_head(*admitted,f.rules,check)->header==generated.head,"genuine one-child admission");
  auto closure=[&](const CapellaForkStore& value){const auto raw=capella_store_snapshot(value,f.rules,f.limits,check);return authenticate_capella_checkpoint_blob_closure(raw,sha256(Bytes(raw.begin(),raw.end())),f.rules,f.limits,{}, {},check);};
  const auto initial=closure(f.initial),current=closure(*admitted);const auto context=current.head_context();const auto observation=current.observation();const auto anchor=f.initial.timed.view.anchor;const auto seed=f.initial.blocks.at(anchor).lease(check)->header;
  const auto initial_description=initial.description(),current_description=current.description();const auto initial_observation=initial.observation();
  auto read=[&](const ConsensusRoot& root)->std::optional<CheckpointArchiveEntry>{check();++archive_reads;const auto found=generated.archives.find(root);if(found==generated.archives.end())return std::nullopt;return found->second;};
  Json request{{"stage","observe"},{"expectedHead",nullptr},{"action",nullptr},{"evidence",nullptr},{"syntheticActionAuthorized",false},{"intentionallyAuthorConflictingHeaders",false},{"crossOwnerEvidenceTransferAuthorized",false}};
  const auto anchor_head=sealed_head("--fresh-audit",request,initial.head_context(),initial_observation,anchor,seed,f.rules,read,check);require(anchor_head==seed&&archive_reads==0,"null observe authentic anchor fast path");++positives;
  const auto head=sealed_head("--fresh-audit",request,context,observation,anchor,seed,f.rules,read,check);require(head==generated.head&&context.block_root==id&&text(head.state_root)==text(generated.head.state_root)&&q(head.slot)==observation.at("headSlot").get<std::string>()&&archive_reads==1,"null observe authentic post-anchor HEAD");++positives;
  std::cout<<Json{{"scenario","fresh-lifecycle-observe-authenticated-head"},{"passed",true},{"completeClosuresDecoded",2},{"positiveHeadChecks",2},{"genuineSignedChildren",1},{"databaseAccessed",false},{"productionQualified",false}}.dump()<<'\n';++groups;
  auto exact=request;exact["expectedHead"]={{"root",text(id)},{"stateRoot",text(head.state_root)},{"slot",q(head.slot)}};
  auto action=exact;action["stage"]="prepare_action";action["action"]="proposer_slashing";action["syntheticActionAuthorized"]=true;action["intentionallyAuthorConflictingHeaders"]=true;
  require(sealed_head("--fresh-action",action,context,observation,anchor,seed,f.rules,read,check)==head,"exact action HEAD remains valid");++positives;
  auto terminal=exact;terminal["stage"]="terminal";require(sealed_head("--fresh-audit",terminal,context,observation,anchor,seed,f.rules,read,check)==head,"exact terminal HEAD remains valid");++positives;
  action["expectedHead"]=nullptr;bad([&]{(void)sealed_head("--fresh-action",action,context,observation,anchor,seed,f.rules,read,check);});
  terminal["expectedHead"]=nullptr;bad([&]{(void)sealed_head("--fresh-audit",terminal,context,observation,anchor,seed,f.rules,read,check);});
  auto unsafe=request;unsafe["crossOwnerEvidenceTransferAuthorized"]=true;bad([&]{(void)sealed_head("--fresh-audit",unsafe,context,observation,anchor,seed,f.rules,read,check);});
  auto wrong=exact;auto changed=id;changed[0]^=1;wrong["expectedHead"]["root"]=text(changed);bad([&]{(void)sealed_head("--fresh-audit",wrong,context,observation,anchor,seed,f.rules,read,check);});
  wrong=exact;changed=head.state_root;changed[0]^=1;wrong["expectedHead"]["stateRoot"]=text(changed);bad([&]{(void)sealed_head("--fresh-audit",wrong,context,observation,anchor,seed,f.rules,read,check);});
  wrong=exact;wrong["expectedHead"]["slot"]=q(head.slot+1);bad([&]{(void)sealed_head("--fresh-audit",wrong,context,observation,anchor,seed,f.rules,read,check);});
  require(refusals==6,"null scope and exact HEAD refusals");std::cout<<Json{{"scenario","fresh-lifecycle-observe-only-null-scope"},{"passed",true},{"exactActionHeadAccepted",true},{"exactTerminalHeadAccepted",true},{"expectedRefusals",6},{"signingInvoked",false},{"productionQualified",false}}.dump()<<'\n';++groups;
  auto corrupt_seed=seed;corrupt_seed.state_root[0]^=1;bad([&]{(void)sealed_head("--fresh-audit",request,initial.head_context(),initial_observation,anchor,corrupt_seed,f.rules,read,check);});
  auto corrupt_read=[&](const ConsensusRoot& root)->std::optional<CheckpointArchiveEntry>{auto e=read(root);require(e&&e->payload.size()>148,"genuine archive field");e->payload[148]^=1;return e;};
  bad([&]{(void)sealed_head("--fresh-audit",request,context,observation,anchor,seed,f.rules,corrupt_read,check);});
  auto corrupt=Json::parse(capella_store_snapshot(*admitted,f.rules,f.limits,check));bool changed_row=false;for(auto& row:corrupt["blocks"])if(row[0]==text(id)){row[1][3]=text(changed);changed_row=true;}require(changed_row,"actual HEAD snapshot row selected");const auto corrupt_raw=corrupt.dump();bad([&]{(void)authenticate_capella_checkpoint_blob_closure(corrupt_raw,sha256(Bytes(corrupt_raw.begin(),corrupt_raw.end())),f.rules,f.limits,{}, {},check);});
  require(initial.description()==initial_description&&current.description()==current_description&&initial.observation()==initial_observation&&current.observation()==observation,"authenticated closures unchanged");
  std::cout<<Json{{"scenario","fresh-lifecycle-observe-corrupt-head-refusal"},{"passed",true},{"expectedRefusals",3},{"envelopeDigestRecomputedForCorruptSnapshot",true},{"authenticatedInputsUnchanged",true},{"productionQualified",false}}.dump()<<'\n';++groups;
  check();const auto signatures=fixture::test_signatures-signatures_before;require(groups==3&&refusals==9&&positives==4&&signatures==4&&archive_reads==5,"new observe exact counters");std::cout<<Json{{"scenario","fresh-lifecycle-observe-summary"},{"passed",true},{"groups",groups},{"expectedRefusals",refusals},{"positiveHeadChecks",positives},{"fixtureBlsSignatures",signatures},{"genuineSignedChildren",1},{"archiveReads",archive_reads},{"operationalSignatures",0},{"envRead",false},{"databaseAccessed",false},{"networkUsed",false},{"oldActionPolicyOrLongStreamExecuted",false},{"runtimeStarted",false},{"productionQualified",false}}.dump()<<'\n';return 0;
 }catch(...){std::cerr<<"{\"scenario\":\"fresh-lifecycle-observe-self-test-refused\",\"passed\":false,\"detailsExported\":false,\"productionQualified\":false}\n";return 1;}
}
} // namespace nano::test::fresh_lifecycle
