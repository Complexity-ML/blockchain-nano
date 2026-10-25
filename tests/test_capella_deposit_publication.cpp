// SOURCE component test. Links the REAL fixed verifier in reactor.cpp.
#include "support/capella_stream_signed_fixture.hpp"
#include "../src/consensus_capella_deposit_publication.hpp"
#include <type_traits>

namespace {
using namespace nano;
namespace f=nano::test::canonical_stream_fixture;
namespace p=nano::deposit_publication_detail;
using Context=CanonicalDepositContext;
constexpr size_t batch_bytes=32*1024*1024;
constexpr ChainStoreMemoryPolicy memory_policy{4*1024*1024,2,16*1024*1024,0};
constexpr DepositCheckpointTreeBudget tree_budget{8192,4*1024*1024};
Bytes raw(const ConsensusRoot& root) {return {root.begin(),root.end()};}
Bytes sha(std::string_view text) {return sha256(Bytes(text.begin(),text.end()));}
template<class Call>void refuses(Call&& call,const char* label) {
 bool rejected=false;
 try {call();} catch(const std::invalid_argument&) {rejected=true;}
 catch(const std::length_error&) {rejected=true;}
 catch(const std::runtime_error&) {rejected=true;}
 f::check(rejected,label);
}
CanonicalDepositContextLimits context_limits() {
 return {128,4*1024*1024,8192,8192,4*1024*1024,1,256,memory_policy};
}
p::DepositPublicationLimits publication_limits(const f::Fixture& fixture) {
 return {fixture.limits,context_limits(),tree_budget,1,fixture.limits.envelope_bytes,
  batch_bytes,{8192,16*1024*1024,256,256*300}};
}

struct Pending {
 Bytes profile;ConsensusRoot network;
 std::map<Bytes,std::string> blobs;
 std::map<std::string,CheckpointDepositLocatorRecord> locators;
 size_t bytes=0;
 Context::Stage stage() {return [this](const Bytes& id,const std::string& value) {
  f::checkpoint();f::check(id.size()==32 && !value.empty() && sha(value)==id,"staged SHA");
  const auto old=blobs.find(id);
  if(old!=blobs.end()) {f::check(old->second==value,"staged bytes immutable");return;}
  f::check(blobs.size()<8192 && value.size()<=16*1024*1024-bytes,"staged bound");
  bytes+=value.size();blobs.emplace(id,value);
 };}
 Context::StageLocator stage_locator() {return [this](CanonicalDepositLookup kind,
  const Bytes& key,const CanonicalDepositLocator& value) {
  f::checkpoint();const auto name=Context::locator_storage_key(profile,network,kind,key);
  CheckpointDepositLocatorRecord record{{profile,raw(network),key,kind==CanonicalDepositLookup::ExecutionHash},
   value.ordinal,raw(value.digest)};
  const auto old=locators.find(name);
  if(old!=locators.end()) {
   f::check(old->second.ordinal==record.ordinal && old->second.digest==record.digest,"staged locator immutable");return;
  }
  f::check(locators.size()<256,"staged locator bound");locators.emplace(name,std::move(record));
 };}
 Context::Fetch fetch() {return [this](const Bytes& id,size_t maximum)->std::optional<std::string> {
  const auto at=blobs.find(id);if(at==blobs.end())return std::nullopt;
  f::check(at->second.size()<=maximum,"staged fetch bound");return at->second;
 };}
 Context::FetchLocator lookup() {return [this](CanonicalDepositLookup kind,const Bytes& key)
  ->std::optional<CanonicalDepositLocator> {
  const auto at=locators.find(Context::locator_storage_key(profile,network,kind,key));
  if(at==locators.end())return std::nullopt;
  ConsensusRoot id{};std::copy(at->second.digest.begin(),at->second.digest.end(),id.begin());
  return CanonicalDepositLocator{at->second.ordinal,id};
 };}
};
struct Candidate {
 std::string bare,outer;
 Bytes tip;
 ConsensusRoot network;
 std::vector<CheckpointBlob> blobs;
 std::vector<CheckpointDepositLocatorRecord> locators;
};
Candidate make_candidate(const f::Fixture& fixture) {
 f::test_stage="real-v4-and-authentic-pending-exit";
 const auto root=fixture.initial.timed.view.anchor;
 const auto anchor=fixture.initial.blocks.at(root).lease(f::checkpoint);
 f::check(anchor->deposit_history && fixture.rules.native_deposit_votes,"fixed native-deposit fixture");
 Pending pending{sha(capella_fork_profile(fixture.rules).dump()),root,{}, {},0};
 const CapellaDepositAncestor original{anchor->header,anchor->execution_block.header,
  anchor->deposit_history->count(),anchor->deposit_history->root()};
 auto context=Context::create_staged(original,pending.profile,context_limits(),pending.fetch(),
  pending.lookup(),pending.stage(),pending.stage_locator(),f::checkpoint);
 auto store=fixture.initial;
 store.blocks.at(root).writable(f::checkpoint).deposit_history=
  std::make_shared<const ExecutionDepositHistory>(anchor->deposit_history->with_cold_storage(context,f::checkpoint));
 store.deposit_ancestry=CapellaDepositAncestry{context,context->anchor_cursor(f::checkpoint),root};
 const auto stage=[&](const CapellaColdReference& ref,std::string_view bytes) {
  f::check(ref.bytes==bytes.size() && ref.digest==sha(bytes),"real consensus row SHA/bytes");
  pending.stage()(ref.digest,std::string(bytes));
 };
 const auto bare=capella_store_deposit_reference_snapshot(store,fixture.rules,fixture.limits,
  context,tree_budget,stage,f::checkpoint);
 const auto& transition=fixture.rules.transition;
 const auto frame=capella_fork_block_context(anchor->state,anchor->header,
  CapellaEpochRules{transition.consensus.epoch,transition.execution,transition.maximum_state_bytes},
  transition.consensus.state,f::checkpoint);
 f::check(frame.registry.epoch==64,"fixed exit fixture reaches epoch64");
 const auto member=frame.registry.validators.at(0);f::TestKey key(0);
 f::check(Bytes(member.public_key.begin(),member.public_key.end())==key.public_key &&
  member.exit_epoch==UINT64_MAX && frame.registry.epoch-member.activation_epoch>=
  transition.consensus.exit.shard_committee_period,"fixed active unit exit identity");
 const auto version=frame.registry.epoch<frame.fork.epoch?frame.fork.previous_version:frame.fork.current_version;
 const auto signature=key.sign(consensus_signing_root(consensus_voluntary_exit_root(frame.registry.epoch,0),
  consensus_domain({4,0,0,0},version,frame.genesis_root)));
 const auto wire=encode_consensus_voluntary_exit_ssz({frame.registry.epoch,0,signature},f::checkpoint);
 Json outer{{"format","nano-capella-durable-v4"},{"store",Json::parse(bare)},
  {"signedVotes",Json::array()},{"signedSlashings",Json::array()},{"blsChanges",Json::array()},
  {"signedExits",Json::array({Json::array({hex(sha256(wire)),hex(raw(root)),hex(wire)})})}};
 Candidate result{bare,outer.dump(),fixture.genesis.block.header.hash(),root,{}, {}};
 for(auto& [id,value]:pending.blobs)result.blobs.push_back({id,std::move(value)});
 for(auto& [_,value]:pending.locators)result.locators.push_back(std::move(value));
 context->close(); // No input Pending/context lifetime is retained by the seal.
 return result;
}
p::SealedDepositDelta seal(const Candidate& candidate,const p::DepositPublicationLimits& limits) {
 return p::SealedDepositDelta::seal(candidate.blobs,candidate.locators,limits.delta,f::checkpoint);
}
std::vector<CheckpointArchiveEntry> signed_archive(const f::Fixture& fixture,const Candidate& candidate) {
 return {{raw(candidate.network),fixture.initial.blocks.at(candidate.network).header().slot,fixture.wire}};
}
void delta_guards(const Candidate& candidate,const p::DepositPublicationLimits& limits) {
 f::test_stage="owned-seal-sha-counts-and-conflicts";
 static_assert(!std::is_default_constructible_v<p::SealedDepositDelta>);
 static_assert(!std::is_default_constructible_v<p::VerifiedDepositPublication>);
 static_assert(!std::is_default_constructible_v<p::PreparedDepositPublication>);
 static_assert(!std::is_copy_constructible_v<p::VerifiedDepositPublication>);
 auto wrong=candidate.blobs;wrong.front().payload[0]^=1;
 refuses([&]{p::SealedDepositDelta::seal(wrong,candidate.locators,limits.delta,f::checkpoint);},"bad blob SHA refused");
 auto narrow=limits.delta;narrow.maximum_blobs=candidate.blobs.size()-1;
 refuses([&]{p::SealedDepositDelta::seal(candidate.blobs,candidate.locators,narrow,f::checkpoint);},"raw count bounded before dedup");
 narrow=limits.delta;narrow.maximum_blob_bytes=0;
 refuses([&]{p::SealedDepositDelta::seal(candidate.blobs,candidate.locators,narrow,f::checkpoint);},"raw byte bound enforced");
 auto locators=candidate.locators;auto conflict=locators.front();++conflict.ordinal;locators.push_back(conflict);
 refuses([&]{p::SealedDepositDelta::seal(candidate.blobs,locators,limits.delta,f::checkpoint);},"conflicting immutable locator refused");
 auto duplicates=candidate.blobs;duplicates.push_back(duplicates.front());
 auto same_locators=candidate.locators;same_locators.push_back(same_locators.front());
 (void)p::SealedDepositDelta::seal(std::move(duplicates),std::move(same_locators),limits.delta,f::checkpoint);
 bool cancelled=false;
 try {p::SealedDepositDelta::seal(candidate.blobs,candidate.locators,limits.delta,
   []{throw ExpiredCapellaDuty("seal cancelled");});}
 catch(const ExpiredCapellaDuty& e) {cancelled=std::string(e.what())=="seal cancelled";}
 f::check(cancelled,"seal cancellation propagates unchanged");
}
void prepared_views(ChainStore& owner,const f::Fixture& fixture,const Candidate& candidate,
 const p::DepositPublicationLimits& limits) {
 f::test_stage="full-verifier-and-dormant-primary-rebind";
 auto bare=p::DepositPublicationFactory::preflight(owner,fixture.genesis.chain_id,candidate.tip,{},
  candidate.tip,candidate.network,fixture.rules,limits,candidate.bare,seal(candidate,limits),f::checkpoint);
 auto prepared=p::DepositPublicationFactory::prepare(std::move(bare),f::checkpoint);
 f::check(prepared.primary_read_usage().attempts==0 && prepared.primary_read_usage().backing_calls==0,
  "preparation performs zero primary read attempts");
 const auto source=prepared.verified_store().deposit_ancestry->context;
 const auto target=prepared.primary_store()->deposit_ancestry->context;
 f::check(source!=target && !source->shares_blob_storage(*target) &&
  !target->shares_blob_storage(*source) && target->usage().cached_blob_records==0,
  "fresh primary context owns no pending cache/owner");
 f::check(prepared.next_token()==ForkCheckpointToken{1,sha(candidate.bare)},"token is sealed whole manifest SHA");
 refuses([&]{prepared.primary_store()->blocks.at(candidate.network).lease(f::checkpoint);},"primary row lease refuses while dormant");
 f::check(prepared.primary_read_usage().attempts>0 && prepared.primary_read_usage().backing_calls==0,
  "dormant read is rejected before primary backing callback");
 for(const auto& value:candidate.blobs)
  f::check(!owner.checkpoint_blob(fixture.genesis.chain_id,candidate.tip,value.digest,batch_bytes),"seal/preflight write no primary CAS");
 f::check(!owner.fork_checkpoint(fixture.genesis.chain_id,candidate.tip,batch_bytes),"preflight writes no token");
 auto close=prepared.close_primary_callback();close();close();
 refuses([&]{prepared.primary_store()->blocks.at(candidate.network).lease(f::checkpoint);},"closed dormant target remains refused");
 auto outer=p::DepositPublicationFactory::preflight(owner,fixture.genesis.chain_id,candidate.tip,{},
  candidate.tip,candidate.network,fixture.rules,limits,candidate.outer,seal(candidate,limits),
  signed_archive(fixture,candidate),f::checkpoint);
 auto outer_prepared=p::DepositPublicationFactory::prepare(std::move(outer),f::checkpoint);
 f::check(outer_prepared.next_token().digest==sha(candidate.outer) &&
  outer_prepared.next_token().digest!=sha(candidate.bare),"outer queue bytes, not store slice, bind the session");
 f::check(outer_prepared.primary_read_usage().attempts==0,"real pending exit also prepares without refetch");
 outer_prepared.close_primary();
}
void boundary_refusals(ChainStore& owner,const f::Fixture& fixture,const Candidate& candidate,
 const p::DepositPublicationLimits& limits) {
 f::test_stage="owner-token-tip-network-queues-and-cancellation";
 auto call=[&](const std::string& encoded,const Bytes& tip,const ConsensusRoot& network,
  std::optional<ForkCheckpointToken> previous={}) {
  return p::DepositPublicationFactory::preflight(owner,fixture.genesis.chain_id,candidate.tip,
   previous,tip,network,fixture.rules,limits,encoded,seal(candidate,limits),f::checkpoint);
 };
 auto wrong_tip=candidate.tip;wrong_tip[0]^=1;
 refuses([&]{call(candidate.bare,wrong_tip,candidate.network);},"wrong current primary tip refused");
 refuses([&]{call(candidate.bare,candidate.tip,candidate.network,ForkCheckpointToken{1,sha(candidate.bare)});},"nonexistent previous token refused");
 auto wrong_network=candidate.network;wrong_network[0]^=1;
 refuses([&]{call(candidate.bare,candidate.tip,wrong_network);},"delta and verifier network scope refused");
 auto malformed=Json::parse(candidate.outer);malformed["signedExits"][0][2]="0x";
 refuses([&]{call(malformed.dump(),candidate.tip,candidate.network);},"real outer malformed queue refused although inner store is valid");
 auto extra=Json::parse(candidate.outer);extra["unknown"]=true;
 refuses([&]{call(extra.dump(),candidate.tip,candidate.network);},"outer extra bytes cannot inherit store-only certificate");
 auto noncanonical=candidate.bare+" ";
 refuses([&]{call(noncanonical,candidate.tip,candidate.network);},"noncanonical outer bytes refused");
 auto archives=signed_archive(fixture,candidate);archives[0].payload[4]^=1;
 refuses([&]{p::DepositPublicationFactory::preflight(owner,fixture.genesis.chain_id,candidate.tip,{},
  candidate.tip,candidate.network,fixture.rules,limits,candidate.outer,seal(candidate,limits),
  archives,f::checkpoint);},"signed archive corruption refuses despite valid manifest and recomputed archive commitment");
 archives=signed_archive(fixture,candidate);++archives[0].slot;
 refuses([&]{p::DepositPublicationFactory::preflight(owner,fixture.genesis.chain_id,candidate.tip,{},
  candidate.tip,candidate.network,fixture.rules,limits,candidate.outer,seal(candidate,limits),
  archives,f::checkpoint);},"archive metadata slot must match real signed header");
 auto certified=call(candidate.bare,candidate.tip,candidate.network);
 bool cancelled=false;
 try {p::DepositPublicationFactory::prepare(std::move(certified),[]{throw ExpiredCapellaDuty("prepare cancelled");});}
 catch(const ExpiredCapellaDuty& e) {cancelled=std::string(e.what())=="prepare cancelled";}
 f::check(cancelled,"prepare cancellation is not turned into a certificate");
}
void real_commit(const f::fs::path& path,const f::Fixture& fixture,const Candidate& candidate,
 const p::DepositPublicationLimits& limits) {
 f::test_stage="real-storage-session-cas-arming-and-primary-only-view";
 f::check(f::fs::create_directory(path),"new owned commit fixture namespace");
 const auto anchor=fixture.initial.blocks.at(candidate.network).lease(f::checkpoint);
 const std::vector<ExecutionCheckpointBlock> suffix{{anchor->execution_block,anchor->execution_state,anchor->receipts}};
 const auto seed=[&](ChainStore& disk) {
  disk.bind_chain(fixture.genesis.chain_id,candidate.tip,"beacon");
  disk.commit(fixture.genesis.block,fixture.genesis.state,{});
 };
 ChainStore owner(path/"primary"),other(path/"other");seed(owner);seed(other);
 auto prepare=[&] {
  auto verified=p::DepositPublicationFactory::preflight(owner,fixture.genesis.chain_id,candidate.tip,{},
   candidate.tip,candidate.network,fixture.rules,limits,candidate.outer,seal(candidate,limits),
   signed_archive(fixture,candidate),f::checkpoint);
  return p::DepositPublicationFactory::prepare(std::move(verified),f::checkpoint);
 };
 auto commit=[&](ChainStore& disk,p::PreparedDepositPublication& session,
  const std::vector<ExecutionCheckpointBlock>& blocks) {
  return disk.commit_prepared_deposit_checkpoint(session,0,candidate.tip,blocks,fixture.limits.blocks);
 };
 auto wrong_owner=prepare();
 refuses([&]{commit(other,wrong_owner,suffix);},"session cannot commit through semantically identical other DB owner");
 refuses([&]{commit(owner,wrong_owner,suffix);},"wrong owner attempt consumes session permanently");
 f::check(!other.fork_checkpoint(fixture.genesis.chain_id,candidate.tip,batch_bytes) &&
  other.latest()->header.hash()==candidate.tip,"wrong owner path makes no primary change");
 auto wrong_head=prepare();
 refuses([&]{commit(owner,wrong_head,{});},"empty suffix/common genesis cannot represent certified head1");
 refuses([&]{commit(owner,wrong_head,suffix);},"bad suffix attempt cannot be retried");
 refuses([&]{wrong_head.primary_store()->blocks.at(candidate.network).lease(f::checkpoint);},"failed commit never arms target");
 auto stale=prepare();
 std::optional<p::PreparedDepositPublication> success(prepare());
 const auto predicted=success->next_token();
 const auto view=success->primary_store();
 std::weak_ptr<Context> overlay=success->verified_store().deposit_ancestry->context;
 const auto lifetime=success->primary_lifetime();
 const auto committed=commit(owner,*success,suffix);
 f::check(committed==predicted && owner.latest()->header.hash()==anchor->execution_block.header.hash(),
  "real same-batch commit returns exact preallocated token and head");
 const auto persisted_archive=owner.checkpoint_archive(raw(candidate.network),batch_bytes);
 f::check(persisted_archive && persisted_archive->slot==anchor->header.slot &&
  persisted_archive->payload==fixture.wire,"same batch retains the exact certified signed archive");
 const auto primary_block=view->blocks.at(candidate.network).lease(f::checkpoint);
 f::check(primary_block->state.ssz==anchor->state.ssz &&
  primary_block->execution_state.root()==anchor->execution_state.root() &&
  success->primary_read_usage().backing_calls>0,"armed view reads real primary bytes");
 refuses([&]{commit(owner,*success,suffix);},"successful session cannot be committed twice");
 refuses([&]{commit(owner,stale,suffix);},"stale previously certified token/tip refuses before arming");
 refuses([&]{stale.primary_store()->blocks.at(candidate.network).lease(f::checkpoint);},"stale failed session remains dormant");
 success.reset();
 f::check(overlay.expired(),"published primary graph retains no preflight context/pending cache");
 f::check(!lifetime.expired() && view->blocks.at(candidate.network).lease(f::checkpoint)->state.ssz==anchor->state.ssz,
  "weak lifetime handle does not own the generation; retained primary view does");
 auto fetch=[&](const Bytes& id,size_t maximum) {return owner.checkpoint_blob(fixture.genesis.chain_id,candidate.tip,id,maximum);};
 const auto profile=sha(capella_fork_profile(fixture.rules).dump());
 auto lookup=[&](CanonicalDepositLookup kind,const Bytes& key)->std::optional<CanonicalDepositLocator> {
  f::check(kind==CanonicalDepositLookup::BeaconRoot || kind==CanonicalDepositLookup::ExecutionHash,"restart locator kind");
  const auto bytes=owner.checkpoint_deposit_locator(fixture.genesis.chain_id,candidate.tip,
   {profile,raw(candidate.network),key,kind==CanonicalDepositLookup::ExecutionHash});
  if(!bytes)return std::nullopt;return Context::decode_locator(*bytes);
 };
 const auto record=owner.fork_checkpoint(fixture.genesis.chain_id,candidate.tip,batch_bytes,committed);
 f::check(record && record->snapshot==candidate.outer,"persisted envelope exact, including queue bytes");
 const auto restored=p::verify_deposit_durable_checkpoint(record->snapshot,record->token.digest,fixture.rules,
  fixture.limits,limits.cache_records,limits.cache_bytes,limits.context,limits.tree,fetch,lookup,f::checkpoint);
 f::check(restored.blocks.at(candidate.network).lease(f::checkpoint)->state.ssz==anchor->state.ssz,
  "ordinary full decoder independently reauthenticates persisted primary envelope");
 lifetime.close();
 refuses([&]{view->blocks.at(candidate.network).lease(f::checkpoint);},"closed primary generation refuses even cached row");
 f::check(primary_block->state.ssz==anchor->state.ssz,"already pinned immutable lease survives generation close");
 // All callbacks are scoped inside the living owners; there is no dangling-DB probe.
}
}

int main(int argc,char**) {
 try {
  f::check(argc==1,"strict no arguments");
  ScopedChainStoreMemoryPolicy policy(memory_policy);
  f::Temp temp;
  auto fixture=f::fixture(512);
  const auto candidate=make_candidate(fixture);
  const auto limits=publication_limits(fixture);
  f::check(!candidate.blobs.empty() && !candidate.locators.empty(),"real staged fixture is nonempty");
  ChainStore owner(temp.path/"primary");
  owner.bind_chain(fixture.genesis.chain_id,candidate.tip,"beacon");
  owner.commit(fixture.genesis.block,fixture.genesis.state,{});
  delta_guards(candidate,limits);
  prepared_views(owner,fixture,candidate,limits);
  boundary_refusals(owner,fixture,candidate,limits);
  real_commit(temp.path/"commit",fixture,candidate,limits);
  f::checkpoint();
  std::cout<<Json{{"passed",true},{"scenario","sealed-deposit-publication-preparation"},
   {"groups",4},{"runtimeActivated",false},{"reactorHandoffQualified",false},
   {"networkConsensusQualified",false},{"productionQualified",false}}.dump()<<'\n';
  return 0;
 } catch(const std::exception& error) {
  std::cerr<<f::test_stage<<": "<<error.what()<<'\n';return 1;
 }
}
