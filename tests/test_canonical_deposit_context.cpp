#include "support/canonical_deposit_context_fixture.hpp"
#include "nano/consensus_canonical_deposit_context.hpp"

namespace {
using namespace nano;
namespace t=nano::test::deposit_context_fixture;
namespace f=nano::test::canonical_stream_fixture;
namespace fs=std::filesystem;
const auto check=f::checkpoint;
// Headroom for the deliberate forged-v1 imports as well as the honest path.
// Capacity behavior is exercised separately with exact one-row/one-blob caps.
CanonicalDepositContextLimits limits(){return {128,1024*1024,8192,16384,4*1024*1024,1,256,t::memory_policy};}
CapellaDepositAncestor tuple(const t::Fixture& fixture,size_t i){return {fixture.full.blocks.at(fixture.ids.at(i)).header(),fixture.blocks.at(i).header,fixture.histories.at(i).count(),fixture.histories.at(i).root()};}
bool same_tuple(const CapellaDepositAncestor& a,const CapellaDepositAncestor& b){return a.beacon==b.beacon&&a.execution.to_json()==b.execution.to_json()&&a.count==b.count&&a.root==b.root;}
void same_usage(const CanonicalDepositContextUsage& a,const CanonicalDepositContextUsage& b){f::check(a.tuples==b.tuples&&a.tuple_bytes==b.tuple_bytes&&a.blob_records==b.blob_records&&a.blob_bytes==b.blob_bytes&&a.cached_blob_records==b.cached_blob_records&&a.cached_blob_bytes==b.cached_blob_bytes&&a.pinned_blob_records==b.pinned_blob_records,"rejected read/admission preserves usage");}
void within(const CanonicalDepositContextUsage& u,const CanonicalDepositContextLimits& l){f::check(u.tuples<=l.maximum_tuples&&u.tuple_bytes<=l.maximum_tuple_bytes&&u.blob_records<=l.maximum_blob_records&&u.blob_bytes<=l.maximum_blob_bytes&&u.cached_blob_records<=l.maximum_cached_blob_records&&u.cached_blob_bytes<=l.maximum_cached_blob_bytes&&u.pinned_blob_records<=u.cached_blob_records,"declared logical context/cache bounds");}
template<class Operation>void reject_exact(Operation operation,const char* reason,const char* label){bool refused=false;try{operation();}catch(const std::invalid_argument& error){f::check(std::string(error.what())==reason,label);refused=true;}f::check(refused,label);}
Bytes profile(){return sha256(f::bytes_of("public-unit-canonical-deposit-context-v1"));}
std::vector<std::shared_ptr<CanonicalDepositContext>> contexts;
std::shared_ptr<CanonicalDepositContext> create(const fs::path& path,const t::Fixture& fixture,const CanonicalDepositContextLimits& l=limits()){
 auto context=CanonicalDepositContext::create(path,tuple(fixture,0),profile(),l,check);contexts.push_back(context);return context;
}
uint64_t close_calls=0;
void close_one(const std::shared_ptr<CanonicalDepositContext>& context){context->close();++close_calls;bool closed=false;try{context->ensure_open([]{});}catch(const std::invalid_argument& error){closed=std::string(error.what())=="deposit context closed";}f::check(closed,"closed context rejects access");}
void close_all(){std::exception_ptr primary;for(const auto& context:contexts){try{close_one(context);}catch(...){if(!primary)primary=std::current_exception();}}if(primary)std::rethrow_exception(primary);}
}

int main(){
 std::unique_ptr<f::Temp> temporary;std::optional<fs::path> source_db;t::Inventory source_before;Json groups=Json::array();std::optional<std::string> failure;Json secondary=Json::array();
 try{
  ScopedChainStoreMemoryPolicy memory(t::memory_policy);temporary=std::make_unique<f::Temp>();const auto root=temporary->path;
  t::Fixture fixture;f::test_stage="unit-source-database";source_db=root/"source";fixture.persist(*source_db);source_before=t::inventory(*source_db);
  auto unchanged=[&]{f::check(t::inventory(*source_db)==source_before,"all source database paths/full9/SHA unchanged");};
  const auto budget=limits();auto context=create(root/"context",fixture,budget);std::vector<CanonicalDepositContext::Cursor> cursors{context->anchor_cursor(check)};
  auto history=fixture.histories.front().with_cold_storage(context,check);f::check(history.uses_cold_storage()&&history.cold_storage()==context,"history owns the provided cold context");
  std::vector<ExecutionDepositHistory> cold{history};size_t proof_checks=0;
  f::test_stage="incremental-nonzero-history";
  for(size_t i=1;i<=t::execution_blocks;++i){
   f::test_slot=i;const auto before=history.root();
   auto next=history.append_verified_execution(fixture.blocks[i].header,fixture.states[i],fixture.receipts[i],fixture.code_hash,1,check);
   f::check(history.root()==before&&next.count()==fixture.histories[i].count()&&next.root()==fixture.histories[i].root()&&next.execution_hash()==fixture.histories[i].execution_hash(),"cold append equals untouched memory history and contract views");
   cursors.push_back(context->append_admitted(tuple(fixture,i),check));history=std::move(next);cold.push_back(history);within(context->usage(),budget);
   if(history.count()&&(i%7==0||i==t::execution_blocks)){t::same_proof(history.proof(0,check),fixture.histories[i].proof(0));t::same_proof(history.proof(history.count()-1,check),fixture.histories[i].proof(history.count()-1));proof_checks+=2;}
  }
  f::check(context->usage().tuples==t::execution_blocks+1&&history.count()==t::nonzero_deposits,"complete nonzero canonical prefix persisted");
  for(size_t old:{size_t(0),size_t(1),size_t(17),size_t(32),size_t(63),size_t(88)}){
   const auto& expected=fixture.histories[old];const auto prefix=history.authenticated_prefix(expected.count(),expected.execution_hash(),expected.root(),check);
   const auto usage_before=context->usage();f::check(prefix.count()==expected.count()&&prefix.root()==expected.root()&&prefix.execution_hash()==expected.execution_hash(),"old authenticated prefix identity");
   for(uint64_t i=0;i<prefix.count();++i){t::same_proof(prefix.proof(i,check),expected.proof(i));++proof_checks;}
   f::check(context->usage().blob_records==usage_before.blob_records&&context->usage().blob_bytes==usage_before.blob_bytes,"prefix and proofs never write scratch blobs");
  }
  unchanged();groups.push_back({{"case","nonzero-contract-history-and-prefixes"},{"executionHeaders",t::execution_blocks},{"deposits",t::nonzero_deposits},{"fullProofsCompared",proof_checks},{"fullConsensusAdmission",false}});

  f::test_stage="strict-ancestry-and-old-votes";
  const size_t first_warm=t::execution_blocks+1-t::warm_records;const auto anchor=cursors[first_warm];
  for(size_t i=0;i<first_warm;++i){const auto row=context->find_before(anchor,fixture.histories[i].execution_hash(),fixture.histories[i].count(),fixture.histories[i].root(),check);f::check(row&&same_tuple(*row,tuple(fixture,i)),"all old tuples found with exact count/root/hash");}
  f::check(!context->find_before(anchor,fixture.histories[first_warm].execution_hash(),fixture.histories[first_warm].count(),fixture.histories[first_warm].root(),check),"exclusive warm anchor cannot find itself");
  f::check(!context->find_before(anchor,fixture.histories.back().execution_hash(),fixture.histories.back().count(),fixture.histories.back().root(),check),"future canonical rows cannot leak through an older anchor cursor");
  auto legacy=fixture.full;legacy.deposit_ancestors=capella_deposit_ancestors_before(fixture.full,fixture.ids[first_warm],check);legacy.timed.view.anchor=fixture.ids[first_warm];
  auto cold_store=fixture.full;cold_store.timed.view.anchor=fixture.ids[first_warm];cold_store.deposit_ancestors.clear();cold_store.deposit_ancestry=CapellaDepositAncestry{context,anchor,fixture.ids[first_warm]};
  for(size_t i=0;i<first_warm;++i){legacy.blocks.erase(fixture.ids[i]);cold_store.blocks.erase(fixture.ids[i]);}
  for(size_t i=first_warm;i<fixture.ids.size();++i)cold_store.blocks.at(fixture.ids[i]).writable(check).deposit_history=std::make_shared<const ExecutionDepositHistory>(cold[i]);
  validate_capella_deposit_ancestors(legacy,check);validate_capella_deposit_ancestors(cold_store,check);
  size_t votes=0,operation_bytes=0;
  for(uint64_t period:{uint64_t(2),uint64_t(17),uint64_t(64)})for(uint64_t slot:{uint64_t(97),uint64_t(128),uint64_t(129)}){
   auto rules=fixture.rules;rules.transition.consensus.state.eth1_votes_limit=period;const auto expected=capella_native_deposit_vote(fixture.full,fixture.ids.back(),slot,rules,check);
   f::check(capella_native_deposit_vote(legacy,fixture.ids.back(),slot,rules,check)==expected&&capella_native_deposit_vote(cold_store,fixture.ids.back(),slot,rules,check)==expected,"cold period vote exactly equals full and legacy-pruned oracle");++votes;
  }
  for(size_t old:{size_t(1),size_t(17),size_t(32),size_t(63)})for(uint64_t consumed:{uint64_t(0),uint64_t(1)}){
   if(consumed>fixture.histories[old].count())continue;const auto selected=t::vote(fixture.histories[old]);const auto submitted=t::vote(fixture.histories.back());const auto advanced=t::advanced(selected,consumed);
   const auto expected=capella_native_deposit_operations(fixture.full,fixture.ids.back(),advanced,submitted,fixture.rules,check);
   f::check(expected==capella_native_deposit_operations(legacy,fixture.ids.back(),advanced,submitted,fixture.rules,check)&&expected==capella_native_deposit_operations(cold_store,fixture.ids.back(),advanced,submitted,fixture.rules,check),"mandatory old SSZ operations equal memory and legacy-pruned oracles");
   Bytes explicit_proofs;const auto needed=std::min<uint64_t>(16,fixture.histories[old].count()-consumed);for(uint64_t i=0;i<needed;++i){const auto one=t::encoded_deposit(fixture.histories[old].proof(consumed+i));explicit_proofs.insert(explicit_proofs.end(),one.begin(),one.end());}f::check(expected==explicit_proofs,"all1240-byte SSZ proof/data records retain order and exact bytes");operation_bytes+=expected.size();
  }
  const auto bad_cursor=t::advanced(t::vote(fixture.histories[17]),18);reject_exact([&]{capella_native_deposit_operations(cold_store,fixture.ids.back(),bad_cursor,t::vote(fixture.histories.back()),fixture.rules,check);},"native deposit cursor beyond count","corrupt SSZ deposit cursor rejected on the real native operations path");
  bool snapshot_refused=false;try{(void)capella_store_snapshot(cold_store,fixture.rules,CapellaStoreSnapshotLimits{},check);}catch(const std::invalid_argument& e){snapshot_refused=std::string(e.what())=="ephemeral audit deposit context cannot use a legacy snapshot";}f::check(snapshot_refused,"cold store cannot masquerade as a persisted legacy format");
  unchanged();groups.push_back({{"case","old-votes-and-ssz-operations"},{"hotRecords",t::warm_records},{"oldTuples",first_warm},{"periodVotesCompared",votes},{"exactOperationBytes",operation_bytes},{"legacyFormatReuseRefused",true}});

  f::test_stage="sibling-and-tuple-refusals";const auto before_refusal=context->usage();
  for(const auto* sibling:{&*fixture.same_root_sibling,&*fixture.different_root_sibling}){
   const auto& h=*sibling->deposit_history;f::check(!context->find_before(anchor,h.execution_hash(),h.count(),h.root(),check)&&!capella_native_deposit_vote_known(cold_store,fixture.ids.back(),t::vote(h),check),"same-slot/count sibling cannot authenticate a canonical vote");
   t::rejects([&]{context->append_admitted({sibling->header,sibling->execution_block.header,h.count(),h.root()},check);},"sibling cannot extend an already admitted canonical context");
  }
  auto root_bad=fixture.histories[17].root();root_bad[0]^=1;f::check(!context->find_before(anchor,fixture.histories[17].execution_hash(),fixture.histories[17].count()+1,fixture.histories[17].root(),check)&&!context->find_before(anchor,fixture.histories[17].execution_hash(),fixture.histories[17].count(),root_bad,check),"count and root are both required in tuple lookup");
  auto wrong_beacon=tuple(fixture,17).beacon;wrong_beacon.state_root[0]^=1;t::rejects([&]{context->validate_cursor(cursors[17],wrong_beacon,fixture.blocks[17].header,check);},"cursor cannot bind a substituted Beacon header");
  auto wrong_execution=fixture.blocks[17].header;wrong_execution.extra_data.push_back(1);t::rejects([&]{context->validate_cursor(cursors[17],tuple(fixture,17).beacon,wrong_execution,check);},"cursor cannot bind a substituted execution header");
  auto mismatched_history=cold_store;mismatched_history.blocks.at(fixture.ids[first_warm]).writable(check).deposit_history=std::make_shared<const ExecutionDepositHistory>(cold[first_warm-1]);
  f::check(cold[first_warm-1].count()==cold[first_warm].count()&&cold[first_warm-1].root()==cold[first_warm].root()&&cold[first_warm-1].execution_hash()!=fixture.blocks[first_warm].header.hash(),"same-root/count mismatched-history premise");t::rejects([&]{validate_capella_deposit_ancestors(mismatched_history,check);},"cold anchor must also bind its history execution hash");validate_capella_deposit_ancestors(cold_store,check);
  auto aliased_key=cold_store;aliased_key.blocks.at(fixture.ids[first_warm])=cold_store.blocks.at(fixture.ids[first_warm+1]);aliased_key.deposit_ancestry->cursor=cursors[first_warm+1];
  const auto alias=aliased_key.blocks.at(fixture.ids[first_warm]).lease(check);f::check(consensus_beacon_header_root(alias->header)==fixture.ids[first_warm+1]&&aliased_key.deposit_ancestry->anchor_root==fixture.ids[first_warm]&&alias->deposit_history->count()==cold[first_warm].count()&&alias->deposit_history->root()==cold[first_warm].root()&&alias->deposit_history->execution_hash()==alias->execution_block.header.hash(),"aliased key has valid B header/history/cursor but retains A anchor identity");t::rejects([&]{validate_capella_deposit_ancestors(aliased_key,check);},"map key cannot borrow another admitted block and cursor at equal count/root");validate_capella_deposit_ancestors(cold_store,check);
  auto foreign=create(root/"foreign",fixture);t::rejects([&]{foreign->at(cursors[17],check);},"same anchor/profile never authorizes another context cursor");close_one(foreign);
  same_usage(context->usage(),before_refusal);unchanged();groups.push_back({{"case","siblings-count-root-cursor-refusals"},{"sameCountRootSlotSibling",true},{"sameCountSlotDifferentRootSibling",true},{"foreignCursorRefused",true}});

  f::test_stage="holes-order-and-capacity";auto short_limits=budget;short_limits.maximum_tuples=2;auto short_context=create(root/"two-tuples",fixture,short_limits);short_context->append_admitted(tuple(fixture,1),check);const auto full_usage=short_context->usage();
  t::rejects([&]{short_context->append_admitted(tuple(fixture,2),check);},"tuple capacity refuses before extending prefix");same_usage(short_context->usage(),full_usage);
  auto hole=create(root/"hole",fixture);t::rejects([&]{hole->append_admitted(tuple(fixture,2),check);},"missing canonical parent cannot be skipped");f::check(hole->usage().tuples==1,"hole refusal leaves original anchor");hole->append_admitted(tuple(fixture,1),check);const auto before_order=hole->usage();t::rejects([&]{hole->append_admitted(tuple(fixture,3),check);},"out-of-order canonical history rejected");same_usage(hole->usage(),before_order);
  const auto duplicate=context->append_admitted(tuple(fixture,17),check);f::check(duplicate->ordinal()==cursors[17]->ordinal(),"exact duplicate row is idempotent");same_usage(context->usage(),before_refusal);
  auto records_budget=budget;records_budget.maximum_blob_records=1;auto records=create(root/"one-blob",fixture,records_budget);const auto first=records->put_node_blob(Bytes(100,1),check);const auto put_usage=records->usage();t::rejects([&]{records->put_node_blob(Bytes(100,2),check);},"blob record cap refuses before growth");same_usage(records->usage(),put_usage);f::check(*records->lease_node_blob(first,check)==Bytes(100,1),"capacity refusal retained prior blob");
  auto bytes_budget=budget;bytes_budget.maximum_blob_bytes=256;auto byte_context=create(root/"byte-cap",fixture,bytes_budget);byte_context->put_node_blob(Bytes(200,3),check);const auto byte_usage=byte_context->usage();t::rejects([&]{byte_context->put_node_blob(Bytes(100,4),check);},"blob byte capacity refuses before growth");same_usage(byte_context->usage(),byte_usage);for(const auto& old:{short_context,hole,records,byte_context})close_one(old);unchanged();groups.push_back({{"case","holes-order-and-declared-capacity"},{"implicitExpansion",false}});

  f::test_stage="old-format-corruption-and-proof-refusals";const auto snapshot=fixture.histories.back().snapshot();
  auto restore=[&](const Json& value,size_t maximum=32){return ExecutionDepositHistory::from_snapshot_cold(value,fixture.blocks.back().header,fixture.states.back(),fixture.blocks.back().header.hash(),fixture.code_hash,maximum,context,check);};
  const auto restored=restore(snapshot);f::check(restored.snapshot(check)==snapshot&&restored.root()==history.root(),"legacy-v1 bytes read exactly without changing their representation");
  for(uint64_t i=0;i<32;++i)t::same_proof(restored.proof(i,check),fixture.histories.back().proof(i));
  auto forged=snapshot;forged["deposits"].erase(forged["deposits"].begin()+7);reject_exact([&]{restore(forged);},"deposit history differs from execution contract","missing old leaf refuses actual contract commitment");forged=snapshot;std::swap(forged["deposits"][0],forged["deposits"][1]);reject_exact([&]{restore(forged);},"deposit history differs from execution contract","reordered leaves refuse actual contract commitment");
  forged=snapshot;auto encoded=forged["deposits"][0].get<std::string>();encoded[2]=encoded[2]=='0'?'1':'0';forged["deposits"][0]=encoded;reject_exact([&]{restore(forged);},"deposit history differs from execution contract","altered old data refuses actual contract root");reject_exact([&]{restore(snapshot,31);},"invalid deposit history snapshot","legacy input count cap refuses silently expanded tree");
  auto wrong_code=fixture.code_hash;wrong_code[0]^=1;reject_exact([&]{ExecutionDepositHistory::from_snapshot_cold(snapshot,fixture.blocks.back().header,fixture.states.back(),fixture.blocks.back().header.hash(),wrong_code,32,context,check);},"unauthenticated deposit history restore","substituted contract code hash refused");
  auto proof=history.proof(0,check);proof.proof[0][0]^=1;Phase0DepositState registry{};registry.deposit_root=history.root();registry.deposit_count=history.count();const auto before_registry=registry;Phase0DepositRules phase0{1000000000,32000000000,64,{}};f::check(!process_phase0_deposit(registry,phase0,proof)&&registry==before_registry,"altered cold Merkle proof changes no cursor or registry");
  for(uint64_t i=0;i<32;++i)f::check(process_phase0_deposit(registry,phase0,history.proof(i,check)).has_value(),"valid cold proof remains usable after corrupt proof refusal");f::check(registry.registry.validators.size()==2&&registry.next_deposit_index==32,"ordered proofs top up two existing unit identities only");
  f::check(fixture.histories.back().snapshot()==snapshot,"failed cold restores never mutate independent memory oracle");within(context->usage(),budget);unchanged();groups.push_back({{"case","legacy-corruption-code-and-proof-refusals"},{"oldV1BytesPreserved",true},{"proofsConsumed",32},{"publicUnitValidators",2},{"unreachableScratchBlobsMayRemain",true}});

  f::test_stage="typed-cancellation-and-retry";const auto original=history.root();const auto tuple_count=context->usage().tuples;size_t callbacks=0;bool cancelled=false;
  try{history.proof(7,[&]{check();if(++callbacks==8)throw ExecutionCancelled("cold-proof-public-unit-cancel");});}catch(const ExecutionCancelled& e){cancelled=std::string(e.what())=="cold-proof-public-unit-cancel";}f::check(cancelled&&history.root()==original&&context->usage().tuples==tuple_count,"typed proof cancellation retains history and canonical prefix");t::same_proof(history.proof(7,check),fixture.histories.back().proof(7));
  auto cancelled_context=create(root/"cancelled-import",fixture);callbacks=0;cancelled=false;try{fixture.histories.back().with_cold_storage(cancelled_context,[&]{check();++callbacks;if(cancelled_context->usage().blob_records>=2)throw ExecutionCancelled("cold-import-public-unit-cancel");});}catch(const ExecutionCancelled& e){cancelled=std::string(e.what())=="cold-import-public-unit-cancel";}f::check(cancelled&&callbacks>0&&cancelled_context->usage().blob_records>=2&&fixture.histories.back().snapshot()==snapshot,"cancelled cold migration after real scratch writes retains source history");within(cancelled_context->usage(),budget);
  auto retried=fixture.histories.back().with_cold_storage(cancelled_context,check);f::check(retried.root()==original,"cold migration retry after counted orphan scratch writes");close_one(cancelled_context);unchanged();groups.push_back({{"case","typed-cancellation-and-idempotent-retry"},{"sourceHistoryUnchanged",true},{"scratchOrphansCounted",true}});

  f::test_stage="pins-shared-leases-and-provider-close";auto leases=create(root/"leases",fixture);const Bytes one(194,7),two(194,9);const auto one_ref=leases->put_node_blob(one,check),two_ref=leases->put_node_blob(two,check);auto pin=leases->lease_node_blob(one_ref,check);const auto pinned_usage=leases->usage();
  t::rejects([&]{leases->lease_node_blob(two_ref,check);},"one-record cache cannot evict an outstanding lease");same_usage(leases->usage(),pinned_usage);f::check(*pin==one,"pinned bytes remain immutable");
  std::vector<std::future<std::shared_ptr<const Bytes>>> readers;for(unsigned i=0;i<8;++i)readers.push_back(std::async(std::launch::async,[&]{return leases->lease_node_blob(one_ref,check);}));for(auto& reader:readers)f::check(reader.get()==pin,"concurrent readers share the same immutable pinned value");
  // These two reads use an EMPTY cache. Pinned-capacity refusal must not mask
  // the distinct missing-digest/incorrect-length authentication paths.
  auto corrupt_refs=create(root/"bad-refs",fixture);const auto ref=corrupt_refs->put_node_blob(one,check);const auto ref_usage=corrupt_refs->usage();auto wrong_ref=ref;wrong_ref.digest[0]^=1;t::rejects([&]{corrupt_refs->lease_node_blob(wrong_ref,check);},"wrong content hash cannot authenticate a blob");same_usage(corrupt_refs->usage(),ref_usage);wrong_ref=ref;++wrong_ref.bytes;t::rejects([&]{corrupt_refs->lease_node_blob(wrong_ref,check);},"wrong length cannot alias a blob");same_usage(corrupt_refs->usage(),ref_usage);
  pin.reset();auto replacement=leases->lease_node_blob(two_ref,check);f::check(*replacement==two&&leases->usage().cached_blob_records==1,"unpinned cache replacement remains within its budget");std::weak_ptr<const Bytes> weak=replacement;close_one(leases);close_one(leases);f::check(*replacement==two&&leases->usage().cached_blob_records==1&&leases->usage().cached_blob_bytes==two.size()&&leases->usage().pinned_blob_records==1,"close clears cache ownership but charges every surviving immutable lease");
  auto rescued=weak.lock();f::check(rescued&&*rescued==two,"weak lock after close preserves immutable bytes");replacement.reset();f::check(leases->usage().cached_blob_records==1&&leases->usage().cached_blob_bytes==two.size(),"weak-rescued strong lease retains its allocation charge");rescued.reset();f::check(weak.expired()&&leases->usage().cached_blob_records==0&&leases->usage().cached_blob_bytes==0&&leases->usage().pinned_blob_records==0,"last strong release drops charge while weak handle alone keeps no bytes");
  reject_exact([&]{leases->lease_node_blob(two_ref,check);},"deposit context closed","closed provider rejects a cache hit");reject_exact([&]{leases->put_node_blob(one,check);},"deposit context closed","closed provider rejects a write");reject_exact([&]{leases->anchor_cursor(check);},"deposit context closed","closed provider rejects a cursor read");reject_exact([&]{leases->append_admitted(tuple(fixture,1),check);},"deposit context closed","closed provider rejects canonical append");close_one(corrupt_refs);
  auto reentrant=create(root/"reentrant",fixture);const auto reentrant_ref=reentrant->put_node_blob(one,check);bool nested_load_refused=false,nested_close_refused=false;
  const auto outer=reentrant->lease_node_blob(reentrant_ref,[&]{check();(void)reentrant->usage();reentrant->ensure_open([]{});if(!nested_load_refused){try{(void)reentrant->lease_node_blob(reentrant_ref,[]{});}catch(const std::logic_error& error){nested_load_refused=std::string(error.what())=="reentrant deposit context operation";if(nested_load_refused){try{reentrant->close();}catch(const std::logic_error& close_error){nested_close_refused=std::string(close_error.what())=="reentrant deposit context close";}}}}});
  f::check(outer&&*outer==one&&nested_load_refused&&nested_close_refused,"observation callbacks allowed while nested mutation and close refuse without deadlock");reentrant->ensure_open(check);close_one(reentrant);
  unchanged();groups.push_back({{"case","bounded-pins-concurrent-leases-and-close"},{"concurrentReaders",8},{"immutableLeaseSurvivesClose",true},{"weakRescueAfterCloseCharged",true},{"lastStrongReleaseChargeZero",true},{"nestedLoadAndCloseRefused",true},{"lateCacheHitRefused",true},{"inFlightCloseDrainQualified",false}});

  f::test_stage="source-readonly-boundary";
  {ChainStore readonly(*source_db,ChainStore::OpenMode::ReadOnly);const auto tip=readonly.latest();f::check(tip&&tip->header.hash()==fixture.blocks.back().header.hash(),"real RO source execution tip unchanged");const auto state=readonly.state_at(tip->header.number);f::check(state&&state->root()==fixture.states.back().root(),"real RO source execution state unchanged");}
  unchanged();close_all();f::checkpoint();reject_exact([&]{history.root();},"deposit context closed","closed history root cannot bypass its provider");reject_exact([&]{history.proof(0,check);},"deposit context closed","closed history cannot serve a cached proof");reject_exact([&]{history.authenticated_prefix(17,fixture.histories[17].execution_hash(),fixture.histories[17].root(),check);},"deposit context closed","closed history cannot construct an old prefix");reject_exact([&]{history.append_verified_execution(fixture.blocks.back().header,fixture.states.back(),fixture.receipts.back(),fixture.code_hash,1,check);},"deposit context closed","closed history cannot append before checking its provider");unchanged();f::checkpoint();f::check(t::files_opened==t::files_closed,"every database fence descriptor closed");
  f::check(groups.size()==7&&t::fixed_deposit_signatures==2,"all nonzero context regression groups and fixed public signatures observed");
  std::cout<<Json{{"scenario","canonical-deposit-context-regression"},{"passed",true},{"groups",groups},{"sourceDatabaseUnchanged",true},{"databaseFenceFilesOpened",t::files_opened},{"databaseFenceFilesClosed",t::files_closed},{"contextsCreated",contexts.size()},{"explicitContextCloseCalls",close_calls},{"publicUnitDepositSignatures",t::fixed_deposit_signatures},{"wholeDriverPeakRssKiB",f::public_peak_rss_kib()},{"physicalRssIsLogicalCacheBound",false},{"signedBeaconAdmissionQualified",false},{"networkQualified",false},{"productionQualified",false}}.dump()<<std::endl;return 0;
 }catch(const std::exception& error){failure=error.what();}catch(...){failure="unknown public unit failure";}
 try{close_all();}catch(const std::exception& error){secondary.push_back({{"phase","context-close"},{"reason",error.what()}});}
 std::cerr<<Json{{"scenario","canonical-deposit-context-regression"},{"passed",false},{"reason",*failure},{"secondary",secondary},{"testStage",f::test_stage},{"testSlot",f::test_slot},{"completedGroups",groups.size()},{"databaseFenceFilesOpened",t::files_opened},{"databaseFenceFilesClosed",t::files_closed},{"wholeDriverPeakRssKiB",f::public_peak_rss_kib()},{"productionQualified",false}}.dump()<<std::endl;return 1;
}
