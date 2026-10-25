// Real signed Capella/FFG/execution integration, opt-in nonzero fixed fixture.
#include "support/capella_stream_signed_fixture.hpp"
#include "nano/consensus_capella_canonical_stream.hpp"
#include "nano/consensus_canonical_deposit_context.hpp"
#include "nano/consensus_capella_store_snapshot.hpp"
#include "nano/consensus_capella_deposits.hpp"

namespace {
using namespace nano;
namespace f=nano::test::canonical_stream_fixture;
constexpr uint64_t descendants=96,topups=16;
constexpr size_t live_blocks=48;
CanonicalDepositContextLimits context_limits(){return {descendants+1,1024*1024,8192,8192,2*1024*1024,1,256,{4*1024*1024,2,16*1024*1024,0}};}
Bytes proof_bytes(const Phase0Deposit& deposit){Bytes out;for(const auto& root:deposit.proof)out.insert(out.end(),root.begin(),root.end());out.insert(out.end(),deposit.message.public_key.begin(),deposit.message.public_key.end());out.insert(out.end(),deposit.message.withdrawal_credentials.begin(),deposit.message.withdrawal_credentials.end());for(unsigned i=0;i<8;++i)out.push_back(uint8_t(deposit.message.amount_gwei>>(8*i)));out.insert(out.end(),deposit.signature.begin(),deposit.signature.end());return out;}
}

int main(){try{
 f::test_stage="real-signed-nonzero-fixture";f::Temp temporary;
 f::LongFixture fixture(descendants,true,topups);const auto& config=fixture.f;
 f::check(fixture.archives.size()==descendants&&fixture.native_histories.size()==descendants+1&&fixture.native_states.size()==descendants+1&&fixture.finalized.epoch>0&&fixture.generator_prunes>0,"complete signed nonzero fixture with real finality/pruning");
 const auto head_id=consensus_beacon_header_root(fixture.head);const auto& expected_history=fixture.native_histories.at(head_id);
 f::check(expected_history.count()==topups&&!expected_history.uses_cold_storage(),"separate legacy memory oracle retained by producer");
 const auto original_anchor=config.initial.timed.view.anchor;
 const auto legacy=capella_store_snapshot(config.initial,config.rules,config.limits,f::checkpoint);
 const auto restored=capella_store_from_snapshot(legacy,sha256(f::bytes_of(legacy)),config.rules,config.limits,f::checkpoint);
 f::check(capella_store_snapshot(restored,config.rules,config.limits,f::checkpoint)==legacy&&!restored.deposit_ancestry&&restored.blocks.at(original_anchor).lease(f::checkpoint)->deposit_history&&!restored.blocks.at(original_anchor).lease(f::checkpoint)->deposit_history->uses_cold_storage(),"legacy store format remains read as legacy inline data");
 uint64_t archive_reads=0,execution_checks=0,visits=0,transactions=0;
 CanonicalStreamSource source{
  [&](const ConsensusRoot& id,size_t maximum,const std::function<void()>& check)->std::optional<CheckpointArchiveEntry>{check();++archive_reads;const auto found=fixture.archives.find(id);if(found==fixture.archives.end())return std::nullopt;f::check(found->second.payload.size()<=maximum,"nonzero archive callback byte bound");return found->second;},
  [&](const CapellaForkBlock& block,const std::function<void()>& check){
   check();++execution_checks;const auto id=consensus_beacon_header_root(block.header);const auto found=fixture.execution.find(id);const auto oracle=fixture.native_histories.find(id);
   f::check(found!=fixture.execution.end()&&oracle!=fixture.native_histories.end()&&block.execution_block.to_json(true)==found->second.first.to_json(true)&&block.execution_state.root()==found->second.second&&block.receipts.size()==block.execution_block.transactions.size(),"full real execution exactly equals independent producer map");
   f::check(block.deposit_history&&block.deposit_history->count()==oracle->second.count()&&block.deposit_history->root()==oracle->second.root()&&block.deposit_history->execution_hash()==oracle->second.execution_hash(),"real signed replay deposit history equals memory oracle at every admitted block");
   if(id!=original_anchor)transactions+=block.execution_block.transactions.size();
  }};
 CanonicalStreamVisitor visitor=[&](const CanonicalStreamVisit& item,const std::function<void()>& check){check();f::check(item.ordinal==visits++&&item.root==consensus_beacon_header_root(item.block.header),"signed nonzero stream order/root");if(item.parent)f::check(item.block.header.parent_root==consensus_beacon_header_root(item.parent->header)&&!item.signed_wire.empty(),"signed nonzero parent/wire");else f::check(item.ordinal==0&&item.signed_wire.empty(),"only trusted anchor omits wire");};
 auto snapshot_limits=config.limits;snapshot_limits.blocks=64;
 CanonicalStreamLimits bounds{live_blocks,config.limits.total_state_bytes,descendants,fixture.archive_bytes,descendants*112};bounds.deposit_context_limits=context_limits();
 f::test_stage="real-cold-signed-nonzero-replay";
 const auto outcome=stream_capella_canonical_history(config.initial,config.rules,snapshot_limits,bounds,{fixture.head,fixture.finalized,fixture.finalized},source,temporary.path/"signed-nonzero.index",visitor,f::checkpoint);
 f::check(outcome.verified_signed_blocks==descendants&&visits==descendants+1&&execution_checks==descendants+1&&transactions==topups&&outcome.prunes>0&&outcome.peak_live_blocks<=live_blocks&&outcome.peak_live_state_ssz_plus_next_reserve_bytes<=bounds.maximum_live_state_bytes&&outcome.required_finalized_ancestor_verified&&!outcome.historical_fork_choice_verified,"actual signed96/16 execution with bounded pruning, no weaker fallback");
 f::check(outcome.head&&outcome.head->header==fixture.head&&outcome.head->state.ssz==fixture.native_states.at(head_id)&&outcome.head->execution_state.root()==fixture.execution.at(head_id).second,"complete signed consensus SSZ and execution roots unchanged by cold context");
 f::check(outcome.head->deposit_history&&outcome.head->deposit_history->uses_cold_storage()&&outcome.head->deposit_history->count()==topups&&outcome.largest_deposit_history_count==topups&&outcome.peak_deposit_ancestor_records==0,"real stream uses cold deposit tree and no growing inline ancestor vector");
 auto context=outcome.head->deposit_history->cold_storage();f::check(bool(context),"returned head retains live authenticated context");const auto usage=context->usage();const auto limit=context_limits();
 f::check(usage.tuples==descendants+1&&usage.tuple_bytes<=limit.maximum_tuple_bytes&&usage.blob_records>0&&usage.blob_records<=limit.maximum_blob_records&&usage.blob_bytes<=limit.maximum_blob_bytes&&usage.cached_blob_records<=1&&usage.cached_blob_bytes<=256,"signed stream context rows/blobs/cache stay within declared budgets");

 // The real Eth1 period switched at slot64. Its selected execution history is
 // slot63; it predates the final warm window and contains all16 actual topups.
 f::test_stage="actual-old-selected-prefix-proof";
 const auto old=std::find_if(fixture.archives.begin(),fixture.archives.end(),[](const auto& item){return item.second.slot==63;});
 f::check(old!=fixture.archives.end()&&outcome.replay_anchor_slot>63,"selected nonzero period history lies strictly outside actual warm window");
 const auto& old_history=fixture.native_histories.at(old->first);f::check(old_history.count()==topups,"old selected root contains16 executed topups");
 const auto old_prefix=outcome.head->deposit_history->authenticated_prefix(old_history.count(),old_history.execution_hash(),old_history.root(),f::checkpoint);
 const auto before_proofs=context->usage();size_t proofs=0;
 for(uint64_t i=0;i<topups;++i){f::check(proof_bytes(old_prefix.proof(i,f::checkpoint))==proof_bytes(old_history.proof(i,f::checkpoint)),"all33 old proof elements and deposit data exactly equal memory oracle");++proofs;}
 f::check(context->usage().blob_records==before_proofs.blob_records&&context->usage().blob_bytes==before_proofs.blob_bytes,"old proof query writes no cold nodes");
 const auto& rules=config.rules.transition;const size_t historical=176+64*rules.consensus.state.slots_per_historical_root;
 f::check(f::test_little(outcome.head->state.ssz,historical+80)==topups,"real signed block path consumed all16 mandatory deposit operations");
 const auto validators=f::test_little(outcome.head->state.ssz,historical+88,4),balances=f::test_little(outcome.head->state.ssz,historical+92,4);
 f::check(balances>=validators&&(balances-validators)/121==64,"topups introduced no new validator identity");
 // This source is a fully valid signed bootstrap, not a synthetic unit record.
 // Without the new guards both writers could otherwise serialize it as legacy.
 f::test_stage="valid-anchor-cold-serialization-refusals";auto cold_without_cursor=config.initial;
 auto& cold_anchor=cold_without_cursor.blocks.at(original_anchor).writable(f::checkpoint);cold_anchor.deposit_history=std::make_shared<const ExecutionDepositHistory>(cold_anchor.deposit_history->with_cold_storage(context,f::checkpoint));
 f::check(!cold_without_cursor.deposit_ancestry&&cold_anchor.deposit_history->uses_cold_storage(),"valid cold-history-only anchor premise");
 auto refuses=[&](auto operation,const char* reason){bool refused=false;try{operation();}catch(const std::invalid_argument& error){refused=std::string(error.what())==reason;}return refused;};
 const auto legacy_cold_refused=refuses([&]{(void)capella_store_snapshot(cold_without_cursor,config.rules,config.limits,f::checkpoint);},"cold deposit history cannot use a legacy store snapshot");
 auto cold_with_cursor=cold_without_cursor;cold_with_cursor.deposit_ancestry=CapellaDepositAncestry{context,context->anchor_cursor(f::checkpoint),original_anchor};CapellaReferenceSnapshotCache memo;const auto memo_before=memo.usage();size_t stages=0;
 const auto cached_cold_refused=refuses([&]{(void)capella_store_reference_snapshot_cached(cold_with_cursor,config.rules,config.limits,[&](const CapellaColdReference&,std::string_view){++stages;},memo,f::checkpoint);},"ephemeral audit deposit context cannot use a legacy snapshot");
 const auto memo_after=memo.usage();f::check(legacy_cold_refused&&cached_cold_refused&&stages==0&&memo_before.records==memo_after.records&&memo_before.encoded_bytes==memo_after.encoded_bytes&&memo_before.reuses==memo_after.reuses&&memo_before.validated_cold_rows==memo_after.validated_cold_rows,"valid cold anchors refuse exact legacy writer paths before staging or memo publication");
 const auto result=Json{{"scenario","capella-cold-deposit-signed-replay"},{"passed",true},{"signedDescendants",descendants},{"transactions",transactions},{"topups",topups},{"publicRegistrySize",64},{"successfulVisitsIncludingAnchor",visits},{"canonicalExecutionChecks",execution_checks},{"sourceArchiveReads",archive_reads},{"actualPrunes",outcome.prunes},{"liveRecordPeak",outcome.peak_live_blocks},{"liveSszPeakBytes",outcome.peak_live_state_ssz_bytes},{"liveSszPlusNextReservePeakBytes",outcome.peak_live_state_ssz_plus_next_reserve_bytes},{"inlineDepositAncestorPeak",outcome.peak_deposit_ancestor_records},{"largestDepositHistoryCount",outcome.largest_deposit_history_count},{"coldTuples",usage.tuples},{"coldTupleBytes",usage.tuple_bytes},{"coldBlobRecords",usage.blob_records},{"coldBlobBytes",usage.blob_bytes},{"coldCachedRecords",usage.cached_blob_records},{"coldCachedBytes",usage.cached_blob_bytes},{"oldSelectedSlot",63},{"finalWarmAnchorSlot",outcome.replay_anchor_slot},{"oldFullProofsCompared",proofs},{"legacyColdHistoryWriterRefused",legacy_cold_refused},{"cachedColdCursorWriterRefused",cached_cold_refused},{"legacyFragmentsStaged",stages},{"headRoot",hex(Bytes(head_id.begin(),head_id.end()))},{"executionBlockHash",hex(outcome.head->execution_block.header.hash())},{"executionStateRoot",hex(outcome.head->execution_state.root())},{"historicalForkChoiceVerified",false},{"actualFinalizedEpochFromProducer",fixture.finalized.epoch},{"wholeDriverPeakRssKiB",f::public_peak_rss_kib()},{"fixtureBlsSignatures",f::test_signatures},{"wholeProcessRssIsLogicalBudget",false},{"networkFinalityQualified",false},{"productionQualified",false}};
 f::test_stage="close-signed-cold-context";context->close();bool closed=false;try{context->ensure_open([]{});}catch(const std::invalid_argument& error){closed=std::string(error.what())=="deposit context closed";}f::check(closed,"signed stream context closed");f::checkpoint();std::cout<<result.dump()<<std::endl;return 0;
}catch(const std::exception& error){std::cerr<<Json{{"scenario","capella-cold-deposit-signed-replay"},{"passed",false},{"reason",error.what()},{"testStage",f::test_stage},{"testSlot",f::test_slot},{"elapsedMs",std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-f::started).count()},{"wholeDriverPeakRssKiB",f::public_peak_rss_kib()},{"productionQualified",false}}.dump()<<std::endl;return 1;}}
