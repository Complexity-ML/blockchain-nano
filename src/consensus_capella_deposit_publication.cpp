#include "consensus_capella_deposit_publication.hpp"
#include <algorithm>
#include <atomic>
#include <limits>
#include <set>
#include <type_traits>

namespace nano::deposit_publication_detail {
namespace {
void require(bool ok,const char* why) {if(!ok)throw std::invalid_argument(why);}
void checkpoint(const Checkpoint& check) {if(check)check();}
Bytes digest(std::string_view text) {return sha256(Bytes(text.begin(),text.end()));}
Bytes raw(const ConsensusRoot& root) {return {root.begin(),root.end()};}
size_t sum(size_t a,size_t b) {
 if(b>std::numeric_limits<size_t>::max()-a)throw std::length_error("deposit delta size overflow");
 return a+b;
}
bool same_limits(const DepositDeltaLimits& a,const DepositDeltaLimits& b) {
 return a.maximum_blobs==b.maximum_blobs && a.maximum_blob_bytes==b.maximum_blob_bytes &&
  a.maximum_locators==b.maximum_locators && a.maximum_locator_bytes==b.maximum_locator_bytes;
}
std::string locator_key(const CheckpointDepositLocatorRecord& value) {
 require(value.lookup.profile_digest.size()==32 && value.lookup.network_anchor.size()==32 &&
  value.lookup.key.size()==32 && value.digest.size()==32,"deposit delta locator width");
 ConsensusRoot network{};
 std::copy(value.lookup.network_anchor.begin(),value.lookup.network_anchor.end(),network.begin());
 return CanonicalDepositContext::locator_storage_key(value.lookup.profile_digest,network,
  value.lookup.execution_hash?CanonicalDepositLookup::ExecutionHash:CanonicalDepositLookup::BeaconRoot,
  value.lookup.key);
}
CanonicalDepositLocator locator_value(const CheckpointDepositLocatorRecord& value) {
 ConsensusRoot id{};std::copy(value.digest.begin(),value.digest.end(),id.begin());
 return {value.ordinal,id};
}
}
struct DepositPrimaryGeneration {
 std::shared_ptr<CapellaCheckpointProvider> provider;
 std::atomic<bool> armed{false},closed{false};
 std::atomic<uint64_t> attempts{0},backing_calls{0};
 void before_read() {
  attempts.fetch_add(1,std::memory_order_relaxed);
  require(!closed.load(std::memory_order_acquire) && armed.load(std::memory_order_acquire),
   "deposit publication primary generation dormant or closed");
  backing_calls.fetch_add(1,std::memory_order_relaxed);
 }
 void close() {
  closed.store(true,std::memory_order_release);
  provider->close();
 }
};
namespace {
CanonicalDepositContext::Fetch primary_fetch(const std::shared_ptr<DepositPrimaryGeneration>& generation) {
 auto reader=generation->provider->reader();
 return [generation,reader=std::move(reader)](const Bytes& id,size_t maximum) {
  generation->before_read();return reader(id,maximum);
 };
}
CanonicalDepositContext::FetchLocator primary_lookup(const std::shared_ptr<DepositPrimaryGeneration>& generation) {
 auto reader=generation->provider->locator_reader();
 return [generation,reader=std::move(reader)](CanonicalDepositLookup kind,const Bytes& key) {
  generation->before_read();return reader(kind,key);
 };
}
}

struct SealedDepositDeltaData {
 DepositDeltaLimits limits;
 std::vector<CheckpointBlob> blobs;
 std::vector<CheckpointDepositLocatorRecord> locators;
 std::vector<std::string> locator_keys;
 Bytes commitment;
};
SealedDepositDelta::SealedDepositDelta(std::shared_ptr<const SealedDepositDeltaData> data)
 :data_(std::move(data)) {}
SealedDepositDelta SealedDepositDelta::seal(std::vector<CheckpointBlob> blobs,
 std::vector<CheckpointDepositLocatorRecord> locators,const DepositDeltaLimits& limits,
 const Checkpoint& check) {
 checkpoint(check);
 require(blobs.size()<=limits.maximum_blobs && locators.size()<=limits.maximum_locators,
  "deposit delta raw record capacity");
 size_t blob_bytes=0,locator_bytes=0;
 for(const auto& blob:blobs) {
  checkpoint(check);
  require(blob.digest.size()==32 && !blob.payload.empty(),"deposit delta blob width/empty");
  blob_bytes=sum(blob_bytes,blob.payload.size());
  require(blob_bytes<=limits.maximum_blob_bytes,"deposit delta raw blob byte capacity");
  require(digest(blob.payload)==blob.digest,"deposit delta blob digest");
  checkpoint(check);
 }
 for(const auto& value:locators) {
  checkpoint(check);
  locator_bytes=sum(locator_bytes,sum(locator_key(value).size(),40));
  require(locator_bytes<=limits.maximum_locator_bytes,"deposit delta raw locator byte capacity");
 }
 std::sort(blobs.begin(),blobs.end(),[](const auto& a,const auto& b){return a.digest<b.digest;});
 std::vector<CheckpointBlob> unique_blobs;
 unique_blobs.reserve(blobs.size());
 for(auto& blob:blobs) {
  checkpoint(check);
  if(!unique_blobs.empty() && unique_blobs.back().digest==blob.digest) {
   require(unique_blobs.back().payload==blob.payload,"deposit delta duplicate blob conflict");
  } else unique_blobs.push_back(std::move(blob));
 }
 std::sort(locators.begin(),locators.end(),[](const auto& a,const auto& b){return locator_key(a)<locator_key(b);});
 auto data=std::make_shared<SealedDepositDeltaData>();data->limits=limits;
 data->blobs=std::move(unique_blobs);data->locators.reserve(locators.size());
 data->locator_keys.reserve(locators.size());
 for(auto& value:locators) {
  checkpoint(check);auto key=locator_key(value);
  if(!data->locator_keys.empty() && data->locator_keys.back()==key) {
   require(data->locators.back().ordinal==value.ordinal && data->locators.back().digest==value.digest,
    "deposit delta duplicate locator conflict");
  } else {
   data->locator_keys.push_back(std::move(key));data->locators.push_back(std::move(value));
  }
 }
 Json commitment=Json::array({"nano-deposit-publication-delta-v1",Json::array(),Json::array()});
 for(const auto& blob:data->blobs) {
  checkpoint(check);commitment[1].push_back(Json::array({hex(blob.digest),blob.payload.size()}));
 }
 for(size_t i=0;i<data->locators.size();++i) {
  checkpoint(check);const auto& value=data->locators[i];
  commitment[2].push_back(Json::array({data->locator_keys[i],value.ordinal,hex(value.digest)}));
 }
 data->commitment=digest(commitment.dump());checkpoint(check);
 return SealedDepositDelta(std::move(data));
}

struct DepositPublicationState {
 ChainStore* owner=nullptr;
 Int chain;
 Bytes genesis,expected_tip,profile;
 ConsensusRoot network{};
 std::optional<ForkCheckpointToken> previous;
 CapellaForkBlockRules rules;
 DepositPublicationLimits limits;
 std::string encoded;
 std::shared_ptr<const SealedDepositDeltaData> delta;
 std::vector<CheckpointArchiveEntry> archives;
 Bytes archive_commitment;
 std::shared_ptr<DepositPrimaryGeneration> primary;
 std::shared_ptr<const CapellaForkStore> verified,prepared;
 Header execution_head;
 ForkCheckpointToken predicted,return_token;
 std::atomic<bool> consumed{false};
};
namespace {
void verify_primary_boundary(const DepositPublicationState& state,const Checkpoint& check) {
 checkpoint(check);
 state.owner->require_existing_chain_binding(state.chain,state.genesis,"beacon");
 checkpoint(check);
 const auto old=state.owner->fork_checkpoint(state.chain,state.genesis,
  state.limits.maximum_batch_bytes,state.previous);
 checkpoint(check);
 require(bool(old)==bool(state.previous) && (!old || old->token==*state.previous),
  "deposit publication previous token mismatch");
 const auto tip=state.owner->latest();checkpoint(check);
 require(tip && tip->header.hash()==state.expected_tip,"deposit publication primary tip mismatch");
}
CanonicalDepositContext::Fetch overlay_fetch(const DepositPublicationState& state) {
 auto base=state.primary->provider->reader();auto delta=state.delta;
 return [delta,base=std::move(base)](const Bytes& id,size_t maximum)->std::optional<std::string> {
  require(id.size()==32 && maximum,"deposit overlay read shape");
  const auto it=std::lower_bound(delta->blobs.begin(),delta->blobs.end(),id,
   [](const auto& blob,const auto& key){return blob.digest<key;});
  if(it!=delta->blobs.end() && it->digest==id) {
   require(it->payload.size()<=maximum,"deposit overlay read capacity");return it->payload;
  }
  return base(id,maximum);
 };
}
CanonicalDepositContext::FetchLocator overlay_lookup(const DepositPublicationState& state) {
 auto base=state.primary->provider->locator_reader();auto delta=state.delta;
 const auto profile=state.profile;const auto network=state.network;
 return [delta,profile,network,base=std::move(base)](CanonicalDepositLookup kind,const Bytes& key)
  ->std::optional<CanonicalDepositLocator> {
  const auto encoded=CanonicalDepositContext::locator_storage_key(profile,network,kind,key);
  const auto it=std::lower_bound(delta->locator_keys.begin(),delta->locator_keys.end(),encoded);
  if(it!=delta->locator_keys.end() && *it==encoded)
   return locator_value(delta->locators[size_t(it-delta->locator_keys.begin())]);
  return base(kind,key);
 };
}
}

VerifiedDepositPublication::VerifiedDepositPublication(std::unique_ptr<DepositPublicationState> state)
 :state_(std::move(state)) {}
VerifiedDepositPublication::~VerifiedDepositPublication()=default;
VerifiedDepositPublication::VerifiedDepositPublication(VerifiedDepositPublication&&) noexcept=default;
VerifiedDepositPublication& VerifiedDepositPublication::operator=(VerifiedDepositPublication&&) noexcept=default;
PreparedDepositPublication::PreparedDepositPublication(std::unique_ptr<DepositPublicationState> state)
 :state_(std::move(state)) {}
PreparedDepositPublication::~PreparedDepositPublication()=default;
PreparedDepositPublication::PreparedDepositPublication(PreparedDepositPublication&&) noexcept=default;
PreparedDepositPublication& PreparedDepositPublication::operator=(PreparedDepositPublication&&) noexcept=default;

VerifiedDepositPublication DepositPublicationFactory::preflight(ChainStore& owner,Int chain,
 Bytes genesis,std::optional<ForkCheckpointToken> previous,Bytes expected_tip,
 const ConsensusRoot& network,CapellaForkBlockRules rules,DepositPublicationLimits limits,
 std::string encoded,SealedDepositDelta&& sealed,const Checkpoint& check) {
 return preflight(owner,std::move(chain),std::move(genesis),std::move(previous),
  std::move(expected_tip),network,std::move(rules),limits,std::move(encoded),std::move(sealed),
  std::vector<CheckpointArchiveEntry>{},check);
}
VerifiedDepositPublication DepositPublicationFactory::preflight(ChainStore& owner,Int chain,
 Bytes genesis,std::optional<ForkCheckpointToken> previous,Bytes expected_tip,
 const ConsensusRoot& network,CapellaForkBlockRules rules,DepositPublicationLimits limits,
 std::string encoded,SealedDepositDelta&& sealed,std::vector<CheckpointArchiveEntry> archives,
 const Checkpoint& check) {
 checkpoint(check);
 require(sealed.data_ && same_limits(sealed.data_->limits,limits.delta),"deposit publication sealed limits mismatch");
 require(chain==rules.transition.chain_id && genesis.size()==32 && expected_tip.size()==32 &&
  (!previous || (previous->digest.size()==32 && previous->sequence && previous->sequence<UINT64_MAX)),
  "deposit publication chain/token/tip shape");
 require(rules.native_deposit_votes && rules.deposit_contract_code_hash.size()==32 &&
  limits.store.envelope_bytes && limits.store.blocks && limits.cache_records &&
  limits.cache_records<=limits.store.blocks && limits.cache_bytes &&
  limits.cache_bytes<=limits.store.envelope_bytes && limits.tree.maximum_visits &&
  limits.tree.maximum_encoded_bytes && limits.maximum_batch_bytes && !encoded.empty() &&
  encoded.size()<=limits.store.envelope_bytes && encoded.size()<=limits.maximum_batch_bytes,
  "deposit publication explicit budgets/profile");
 require(archives.size()<=limits.store.blocks,"deposit publication raw archive capacity");
 size_t archive_bytes=0;
 Json archive_commitment=Json::array({"nano-deposit-publication-archives-v1",Json::array()});
 std::set<Bytes> archive_roots;
 for(const auto& entry:archives) {
  checkpoint(check);
  require(entry.root.size()==32 && !entry.payload.empty() && archive_roots.insert(entry.root).second,
   "deposit publication raw archive shape/duplicate");
  archive_bytes=sum(archive_bytes,entry.payload.size());
  require(archive_bytes<=limits.maximum_batch_bytes,"deposit publication raw archive byte capacity");
  const auto hash=sha256(entry.payload);checkpoint(check);
  archive_commitment[1].push_back(Json::array({hex(entry.root),entry.slot,hex(hash),entry.payload.size()}));
 }
 auto state=std::make_unique<DepositPublicationState>();
 state->owner=&owner;state->chain=std::move(chain);state->genesis=std::move(genesis);
 state->previous=std::move(previous);state->expected_tip=std::move(expected_tip);
 state->profile=digest(capella_fork_profile(rules).dump());state->network=network;
 state->rules=std::move(rules);state->limits=limits;state->encoded=std::move(encoded);
 state->delta=std::move(sealed.data_);
 state->archives=std::move(archives);state->archive_commitment=digest(archive_commitment.dump());
 for(const auto& value:state->delta->locators) {
  checkpoint(check);
  require(value.lookup.profile_digest==state->profile && value.lookup.network_anchor==raw(network),
   "deposit publication delta locator scope");
 }
 verify_primary_boundary(*state,check);
 state->primary=std::make_shared<DepositPrimaryGeneration>();
 const auto profile=state->profile;const auto genesis_pin=state->genesis;
 const auto chain_pin=state->chain;
 state->primary->provider=std::make_shared<CapellaCheckpointProvider>(
  [&owner,chain_pin,genesis_pin](const Bytes& id,size_t maximum) {
   return owner.checkpoint_blob(chain_pin,genesis_pin,id,maximum);
  },
  [&owner,chain_pin,genesis_pin,profile,network](CanonicalDepositLookup kind,const Bytes& key)
   ->std::optional<CanonicalDepositLocator> {
   require((kind==CanonicalDepositLookup::BeaconRoot || kind==CanonicalDepositLookup::ExecutionHash) &&
    key.size()==32,"deposit publication primary locator shape");
   CheckpointDepositLocatorKey lookup{profile,raw(network),key,kind==CanonicalDepositLookup::ExecutionHash};
   const auto value=owner.checkpoint_deposit_locator(chain_pin,genesis_pin,lookup);
   if(!value)return std::nullopt;return CanonicalDepositContext::decode_locator(*value);
  });
 const auto encoded_digest=digest(state->encoded);
 auto restored=verify_deposit_durable_checkpoint(state->encoded,encoded_digest,state->rules,
  limits.store,limits.cache_records,limits.cache_bytes,limits.context,limits.tree,
  overlay_fetch(*state),overlay_lookup(*state),check);
 checkpoint(check);
 require(restored.deposit_ancestry && restored.deposit_ancestry->context &&
  restored.deposit_ancestry->cursor && restored.deposit_ancestors.empty() &&
  capella_network_anchor(restored)==network,"deposit publication requires verified v4 context");
 verify_deposit_publication_archives(state->archives,restored,state->rules,limits.store,check);
 checkpoint(check);
 const auto head=capella_fork_head(restored,state->rules,check);
 state->execution_head=head->execution_block.header;
 state->verified=std::make_shared<const CapellaForkStore>(std::move(restored));
 state->predicted={state->previous?state->previous->sequence+1:1,encoded_digest};
 state->return_token=state->predicted;
 verify_primary_boundary(*state,check);checkpoint(check);
 return VerifiedDepositPublication(std::move(state));
}

PreparedDepositPublication DepositPublicationFactory::prepare(VerifiedDepositPublication&& verified,
 const Checkpoint& check) {
 checkpoint(check);
 auto state=std::move(verified.state_);
 require(state && state->verified && state->primary && !state->primary->closed.load(),
  "deposit publication missing/closed certificate");
 const auto& source=*state->verified;
 const auto& ancestry=*source.deposit_ancestry;
 auto fetch=primary_fetch(state->primary);auto lookup=primary_lookup(state->primary);
 auto [context,cursor]=ancestry.context->clone_preflight_primary(fetch,lookup,ancestry.cursor,check);
 auto prepared=std::make_shared<CapellaForkStore>(source);
 prepared->deposit_ancestry=CapellaDepositAncestry{context,cursor,ancestry.anchor_root};
 auto cache=std::make_shared<CapellaColdRecordCache>(state->rules,state->limits.store,
  state->limits.cache_records,state->limits.cache_bytes,context,state->limits.tree);
 for(auto& [id,record]:prepared->blocks) {
  checkpoint(check);
  require(record.cold_reference().has_value(),"deposit publication requires all verified cold records");
  const auto reference=*record.cold_reference();const auto header=record.header();
  const auto execution=record.execution_header();
  require(reference.root==id && consensus_beacon_header_root(header)==id,
   "deposit publication certified record metadata");
  record=CapellaForkRecord(reference,header,execution,cache,fetch);
 }
 checkpoint(check);context->ensure_open(check);
 require(state->primary->attempts.load()==0 && state->primary->backing_calls.load()==0,
  "deposit publication preparation attempted primary read");
 state->prepared=std::move(prepared);checkpoint(check);
 return PreparedDepositPublication(std::move(state));
}

const CapellaForkStore& PreparedDepositPublication::verified_store() const {
 require(state_ && state_->verified,"deposit publication moved session");return *state_->verified;
}
std::shared_ptr<const CapellaForkStore> PreparedDepositPublication::primary_store() const {
 require(state_ && state_->prepared,"deposit publication moved session");return state_->prepared;
}
const ForkCheckpointToken& PreparedDepositPublication::next_token() const {
 require(bool(state_),"deposit publication moved session");return state_->predicted;
}
DepositPrimaryReadUsage PreparedDepositPublication::primary_read_usage() const {
 require(bool(state_),"deposit publication moved session");
 return {state_->primary->attempts.load(),state_->primary->backing_calls.load()};
}
DepositPrimaryLifetime::DepositPrimaryLifetime(const std::shared_ptr<DepositPrimaryGeneration>& generation)
 :generation_(generation) {}
bool DepositPrimaryLifetime::expired() const noexcept {return generation_.expired();}
void DepositPrimaryLifetime::close() const {
 if(const auto generation=generation_.lock())generation->close();
}
DepositPrimaryLifetime PreparedDepositPublication::primary_lifetime() const {
 require(bool(state_),"deposit publication moved session");return DepositPrimaryLifetime(state_->primary);
}
void PreparedDepositPublication::close_primary() {
 require(bool(state_),"deposit publication moved session");state_->primary->close();
}
std::function<void()> PreparedDepositPublication::close_primary_callback() const {
 require(bool(state_),"deposit publication moved session");
 return [generation=std::weak_ptr<DepositPrimaryGeneration>(state_->primary)] {
  if(const auto live=generation.lock())live->close();
 };
}
void PreparedDepositPublication::consume_for_commit(ChainStore& owner) {
 require(bool(state_),"deposit publication moved session");
 require(!state_->consumed.exchange(true),"deposit publication session already consumed");
 require(state_->owner==&owner && state_->prepared && !state_->primary->closed.load() &&
  !state_->primary->armed.load(),"deposit publication wrong owner or unavailable session");
 state_->prepared->deposit_ancestry->context->ensure_open({});
}
const Int& PreparedDepositPublication::chain_id() const noexcept {return state_->chain;}
const Bytes& PreparedDepositPublication::genesis_hash() const noexcept {return state_->genesis;}
const Bytes& PreparedDepositPublication::expected_primary_tip() const noexcept {return state_->expected_tip;}
const std::optional<ForkCheckpointToken>& PreparedDepositPublication::expected_previous_token() const noexcept {return state_->previous;}
const Header& PreparedDepositPublication::certified_execution_header() const noexcept {return state_->execution_head;}
const std::string& PreparedDepositPublication::manifest() const noexcept {return state_->encoded;}
const std::vector<CheckpointBlob>& PreparedDepositPublication::blobs() const noexcept {return state_->delta->blobs;}
const std::vector<CheckpointDepositLocatorRecord>& PreparedDepositPublication::locators() const noexcept {return state_->delta->locators;}
const std::vector<CheckpointArchiveEntry>& PreparedDepositPublication::archives() const noexcept {return state_->archives;}
size_t PreparedDepositPublication::maximum_bytes() const noexcept {return state_->limits.maximum_batch_bytes;}
size_t PreparedDepositPublication::maximum_blobs() const noexcept {return state_->limits.delta.maximum_blobs;}
size_t PreparedDepositPublication::maximum_locators() const noexcept {return state_->limits.delta.maximum_locators;}
const ForkCheckpointToken& PreparedDepositPublication::predicted_token() const noexcept {return state_->predicted;}
void PreparedDepositPublication::arm_after_commit() noexcept {state_->primary->armed.store(true,std::memory_order_release);}
ForkCheckpointToken PreparedDepositPublication::take_return_token() noexcept {
 static_assert(std::is_nothrow_move_constructible_v<ForkCheckpointToken>);
 return std::move(state_->return_token);
}
}
