#pragma once
#include "capella_canonical_stream.hpp"
#include "capella_stream_signed_fixture.hpp"
// Only --fresh-stream-self-test invokes this new synthetic fixture. Including
// these definitions never starts a generator, signs, opens a DB or runs a test.
namespace nano::test::lifecycle {
inline int fresh_stream_self_test(){
 namespace f=nano::test::canonical_stream_fixture;
 try{
  f::Temp temporary;uint64_t visits=0,execution_checks=0,archive_reads=0,refusals=0,groups=0;
  {
   const auto path=temporary.path/"metadata.index";canonical_stream_detail::Index index(path,112);canonical_stream_detail::Record opaque{};index.append(opaque,f::checkpoint);index.seal();index.verify(f::checkpoint);
   auto refusal=[&](auto operation,const char* why){bool hit=false;try{operation();}catch(const std::runtime_error& e){hit=std::string(e.what())==why;}f::check(hit,"sealed private index refusal required");++refusals;};
   refusal([&]{index.append(opaque,f::checkpoint);},"stream index append after seal refused");refusal([&]{index.seal();},"stream index reseal refused");
   const auto before=std::filesystem::last_write_time(path);std::filesystem::last_write_time(path,before+std::chrono::seconds(1));refusal([&]{index.verify(f::checkpoint);},"stream sealed index material changed");
   const auto empty_path=temporary.path/"empty.index";canonical_stream_detail::Index empty(empty_path,112);empty.seal();empty.verify(f::checkpoint);const auto empty_time=std::filesystem::last_write_time(empty_path);std::filesystem::last_write_time(empty_path,empty_time+std::chrono::seconds(1));refusal([&]{empty.verify(f::checkpoint);},"stream sealed index material changed");
   std::cout<<Json{{"scenario","fresh-stream-private-index-seal"},{"passed",true},{"positiveIndexVerification",2},{"expectedRefusals",4},{"fileContentChangedByMutation",false},{"wholeSignedReplayNeeded",false},{"productionQualified",false}}.dump()<<'\n';++groups;
  }
  f::LongFixture complete(1026);const auto& config=complete.f;f::check(config.rules.native_deposit_votes,"native deposit validation must remain enabled in real fixture");const auto live=std::min(size_t(128)*1024*1024,config.limits.total_state_bytes);
  CanonicalStreamLimits limits{256,live,1026,16*1024*1024,1026*112};
  CanonicalStreamBoundary boundary{complete.head,complete.finalized,complete.finalized};
  auto source=[&](const f::LongFixture& fixture,int fault=0){return CanonicalStreamSource{
   [&,fault](const ConsensusRoot& id,size_t maximum,const std::function<void()>& check)->std::optional<CheckpointArchiveEntry>{check();++archive_reads;const auto found=fixture.archives.find(id);if(found==fixture.archives.end())return std::nullopt;if(fault==2&&id==consensus_beacon_header_root(fixture.head))return std::nullopt;auto e=found->second;if(fault==3&&id==consensus_beacon_header_root(fixture.head)){f::check(bool(fixture.wrong_branch),"genuine valid wrong-branch fixture required");e=*fixture.wrong_branch;}f::check(e.payload.size()<=maximum,"fixture archive callback respects bound");if(fault==1&&e.slot==2)e.payload[4]^=1;return e;},
   [&](const CapellaForkBlock& record,const std::function<void()>& check){check();++execution_checks;const auto id=consensus_beacon_header_root(record.header);const auto found=fixture.execution.find(id);f::check(found!=fixture.execution.end()&&found->second.first.to_json(true)==record.execution_block.to_json(true)&&found->second.second==record.execution_state.root()&&record.receipts.size()==record.execution_block.transactions.size(),"real fixture canonical execution index/state differs");}};};
  CanonicalStreamVisitor visitor=[&](const CanonicalStreamVisit& item,const std::function<void()>& check){check();f::check(item.ordinal==visits++&&item.root==consensus_beacon_header_root(item.block.header),"actual stream visitor order/root");if(item.parent)f::check(item.block.header.parent_root==consensus_beacon_header_root(item.parent->header)&&!item.signed_wire.empty(),"actual stream parent/wire");else f::check(item.ordinal==0&&item.signed_wire.empty(),"only anchor may omit wire");};
  const auto outcome=stream_capella_canonical_history(config.initial,config.rules,config.limits,limits,boundary,source(complete),temporary.path/"complete.index",visitor,f::checkpoint);
  f::check(outcome.verified_signed_blocks==1026&&visits==1027&&outcome.prunes>0&&outcome.peak_live_blocks<=256&&outcome.peak_live_state_ssz_plus_next_reserve_bytes<=live&&outcome.required_finalized_ancestor_verified&&!outcome.historical_fork_choice_verified&&outcome.head->header==complete.head,"1026 real signed transitions require bounded genuine pruning");
  std::cout<<Json{{"scenario","fresh-stream-real-signed-1026"},{"passed",true},{"signedBlocks",outcome.verified_signed_blocks},{"liveRecordPeak",outcome.peak_live_blocks},{"liveStateSszPeakBytes",outcome.peak_live_state_ssz_bytes},{"liveStateSszPlusNextReservePeakBytes",outcome.peak_live_state_ssz_plus_next_reserve_bytes},{"stateReservationIsProcessRss",false},{"prunes",outcome.prunes},{"actualFinalizedEpoch",complete.finalized.epoch},{"generatorPeakRecords",complete.peak_generator_records},{"generatorPrunes",complete.generator_prunes},{"historicalForkChoiceVerified",false},{"productionQualified",false}}.dump()<<'\n';++groups;
  // Each first-pass refusal uses the same immutable genuine source, and each
  // fresh private index is distinct. Bad signature reaches real BLS admission.
  auto reject=[&](const char* leaf,int fault,const char* reason){bool refused=false;try{auto ignored=stream_capella_canonical_history(config.initial,config.rules,config.limits,limits,boundary,source(complete,fault),temporary.path/leaf,[](const auto&,const auto&){},f::checkpoint);(void)ignored;}catch(const std::runtime_error& e){refused=std::string(e.what())==reason;}f::check(refused,"specific corrupt archive refusal required");++refusals;};
  reject("bad-signature.index",1,"stream canonical signed transition rejected");reject("missing.index",2,"stream canonical signed archive missing");reject("wrong-branch.index",3,"stream archive root/slot/envelope differs");
  bool cancelled=false;uint64_t checks=0;try{auto ignored=stream_capella_canonical_history(config.initial,config.rules,config.limits,limits,boundary,source(complete),temporary.path/"cancelled.index",[](const auto&,const auto&){},[&]{if(++checks==8)throw ExecutionCancelled("new-stream-cancelled");});(void)ignored;}catch(const ExecutionCancelled&){cancelled=true;}f::check(cancelled,"real cancellation propagates");++refusals;
  std::cout<<Json{{"scenario","fresh-stream-corrupt-missing-branch-cancel"},{"passed",true},{"expectedRefusals",4},{"wholeSuccessfulReplaysRepeated",0},{"productionQualified",false}}.dump()<<'\n';++groups;
  // 255 genuine signed blocks + anchor, no attestations, no fabricated FFG.
  // Authority selects a128-live-record window: it must refuse, not retain the
  // whole unfinalized source or promote a supplied checkpoint into replay FFG.
  f::LongFixture stalled(255,false);auto stalled_limits=limits;stalled_limits.maximum_live_blocks=128;stalled_limits.maximum_total_blocks=255;stalled_limits.maximum_private_index_bytes=255*112;const auto anchor=stalled.f.initial.timed.view.finalized;
  bool window=false;try{auto ignored=stream_capella_canonical_history(stalled.f.initial,stalled.f.rules,stalled.f.limits,stalled_limits,{stalled.head,anchor,anchor},source(stalled),temporary.path/"stalled.index",[](const auto&,const auto&){},f::checkpoint);(void)ignored;}catch(const std::runtime_error& e){window=std::string(e.what())=="stream finalized window unavailable before next state";}f::check(window,"unfinalized history must refuse before window growth");++refusals;
  std::cout<<Json{{"scenario","fresh-stream-stalled-finality-bounded-refusal"},{"passed",true},{"sourceSignedBlocks",255},{"selectedLiveWindow",128},{"expectedRefusals",1},{"fabricatedFinality",false},{"productionQualified",false}}.dump()<<'\n';++groups;
  f::check(groups==4&&refusals==9,"all fresh streaming groups and refusals observed");std::cout<<Json{{"scenario","fresh-stream-summary"},{"passed",true},{"groups",groups},{"expectedRefusals",refusals},{"signedBlocksInSuccessfulStream",1026},{"testFixtureBlsSignatures",f::test_signatures},{"sourceArchiveReads",archive_reads},{"canonicalExecutionIndexChecks",execution_checks},{"newDatabasesOpened",0},{"runtimeStarted",false},{"projectSourceOrProfileChanged",false},{"productionQualified",false}}.dump()<<'\n';return 0;
 }catch(...){std::cerr<<"{\"scenario\":\"fresh-stream-self-test-refused\",\"passed\":false,\"detailsExported\":false}\n";return 1;}
}
}
