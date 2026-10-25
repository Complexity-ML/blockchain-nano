// Component checkpoint codec/primary batch; no reactor activation or migration.
#include "support/canonical_deposit_context_fixture.hpp"
#include "nano/consensus_canonical_deposit_context.hpp"
#include "nano/consensus_capella_store_snapshot.hpp"
#include "nano/consensus_capella_cold_records.hpp"
#include <optional>

namespace {
using namespace nano;
namespace f=nano::test::canonical_stream_fixture;
namespace d=nano::test::deposit_context_fixture;
using Context=CanonicalDepositContext;
constexpr size_t batch_budget=4*1024*1024;
constexpr DepositCheckpointTreeBudget tree_budget{8192,4*1024*1024};
Bytes raw(const ConsensusRoot& x){return {x.begin(),x.end()};}
Bytes sha(std::string_view x){return sha256(Bytes(x.begin(),x.end()));}
CanonicalDepositContextLimits limits(){return {128,4*1024*1024,8192,8192,4*1024*1024,1,256,d::memory_policy};}
template<class Call>void refuses(Call call,const char* message){d::rejects(std::move(call),message);}

struct Primary {
 struct Shared {std::shared_ptr<ChainStore> db;Bytes genesis;size_t fetches=0,lookups=0;};
 std::shared_ptr<Shared> shared;
 std::unique_ptr<CapellaCheckpointProvider> provider;
 Primary(const f::fs::path& path,const Bytes& genesis,ChainStore::OpenMode mode,const Bytes& profile,const ConsensusRoot& network):shared(std::make_shared<Shared>()){
  shared->db=std::make_shared<ChainStore>(path,mode);shared->genesis=genesis;
  if(mode==ChainStore::OpenMode::ReadWrite)shared->db->bind_chain(1337,genesis,"beacon");
  else shared->db->require_existing_chain_binding(1337,genesis,"beacon");
  provider=std::make_unique<CapellaCheckpointProvider>([s=shared](const Bytes& key,size_t bound){
   ++s->fetches;f::check(bool(s->db),"primary provider backing alive");return s->db->checkpoint_blob(1337,s->genesis,key,bound);
  },[s=shared,profile,network](CanonicalDepositLookup kind,const Bytes& key)->std::optional<CanonicalDepositLocator>{
   ++s->lookups;f::check(bool(s->db),"locator provider backing alive");
   const CheckpointDepositLocatorKey lookup{profile,raw(network),key,kind==CanonicalDepositLookup::ExecutionHash};
   const auto value=s->db->checkpoint_deposit_locator(1337,s->genesis,lookup);
   if(!value)return {};return Context::decode_locator(*value);
  });
 }
 Context::Fetch fetch()const{return provider->reader();}
 Context::FetchLocator lookup()const{return provider->locator_reader();}
 ForkCheckpointRecord checkpoint(const ForkCheckpointToken& token)const{
  const auto record=shared->db->fork_checkpoint(1337,shared->genesis,batch_budget,token);
  f::check(record&&record->token==token,"independently retained checkpoint token");return *record;
 }
 void assert_paired_close(const Bytes& digest,const Bytes& locator_key){
  const auto read=fetch();const auto locate=lookup();const auto fetches=shared->fetches,lookups=shared->lookups;
  provider->close();f::check(bool(shared->db),"paired gate closes before the primary backing database is released");
  refuses([&]{read(digest,256);},"saved CAS reader refuses after paired provider close");
  refuses([&]{locate(CanonicalDepositLookup::BeaconRoot,locator_key);},"saved locator reader refuses after same paired provider close");
  f::check(shared->fetches==fetches&&shared->lookups==lookups,"neither backing callback reached after common gate close, with database still alive");
 }
 void close(){if(provider)provider->close();shared->db.reset();}
 ~Primary(){close();}
};

struct Pending {
 Bytes profile;ConsensusRoot network;
 std::map<Bytes,std::string> cas;
 std::map<std::string,CheckpointDepositLocatorRecord> locators;
 size_t writes=0,locator_writes=0,total_bytes=0;
 Pending(Bytes p,ConsensusRoot n):profile(std::move(p)),network(n){}
 Context::Stage stage(){return [this](const Bytes& key,const std::string& value){
  f::checkpoint();f::check(key.size()==32&&!value.empty()&&sha(value)==key,"staged CAS commitment");
  const auto at=cas.find(key);if(at!=cas.end()){f::check(at->second==value,"immutable staged CAS");return;}
  f::check(value.size()<=batch_budget-total_bytes&&cas.size()<8192,"bounded candidate CAS staging");
  total_bytes+=value.size();cas.emplace(key,value);++writes;
 };}
 Context::StageLocator stage_locator(){return [this](CanonicalDepositLookup kind,const Bytes& key,const CanonicalDepositLocator& value){
  f::checkpoint();const auto name=Context::locator_storage_key(profile,network,kind,key);
  const CheckpointDepositLocatorRecord record{{profile,raw(network),key,kind==CanonicalDepositLookup::ExecutionHash},value.ordinal,raw(value.digest)};
  const auto at=locators.find(name);if(at!=locators.end()){f::check(at->second.ordinal==record.ordinal&&at->second.digest==record.digest,"immutable staged locator");return;}
  f::check(locators.size()<256,"bounded candidate locator staging");locators.emplace(name,record);++locator_writes;
 };}
 Context::Fetch fetch(Context::Fetch primary){return [this,primary=std::move(primary)](const Bytes& key,size_t bound)->std::optional<std::string>{
  const auto at=cas.find(key);if(at!=cas.end()){f::check(at->second.size()<=bound,"candidate fetch bound");return at->second;}return primary(key,bound);
 };}
 Context::FetchLocator lookup(Context::FetchLocator primary){return [this,primary=std::move(primary)](CanonicalDepositLookup kind,const Bytes& key)->std::optional<CanonicalDepositLocator>{
  const auto at=locators.find(Context::locator_storage_key(profile,network,kind,key));if(at==locators.end())return primary(kind,key);
  ConsensusRoot digest{};std::copy(at->second.digest.begin(),at->second.digest.end(),digest.begin());return CanonicalDepositLocator{at->second.ordinal,digest};
 };}
 std::vector<CheckpointBlob> blobs()const{std::vector<CheckpointBlob> out;for(const auto& [key,value]:cas)out.push_back({key,value});return out;}
 std::vector<CheckpointDepositLocatorRecord> records()const{std::vector<CheckpointDepositLocatorRecord> out;for(const auto& [_,record]:locators)out.push_back(record);return out;}
 void clear(){cas.clear();locators.clear();total_bytes=0;}
};

struct Owners {
 std::vector<std::shared_ptr<Context>> contexts;
 std::vector<std::shared_ptr<Primary>> primary;
 std::shared_ptr<Context> own(std::shared_ptr<Context> context){contexts.push_back(context);return context;}
 std::shared_ptr<Primary> open(const f::fs::path& path,const Bytes& genesis,ChainStore::OpenMode mode,const Bytes& profile,const ConsensusRoot& network){auto p=std::make_shared<Primary>(path,genesis,mode,profile,network);primary.push_back(p);return p;}
 void close_contexts(){std::exception_ptr first;for(const auto& c:contexts)try{c->close();}catch(...){if(!first)first=std::current_exception();}if(first)std::rethrow_exception(first);}
 void close(){std::exception_ptr first;try{close_contexts();}catch(...){first=std::current_exception();}for(const auto& p:primary)try{p->close();}catch(...){if(!first)first=std::current_exception();}if(first)std::rethrow_exception(first);}
 ~Owners(){try{close();}catch(...){}}
};
CapellaDepositAncestor tuple(const d::Fixture& fixture,size_t i){return {fixture.full.blocks.at(fixture.ids.at(i)).header(),fixture.blocks.at(i).header,fixture.histories.at(i).count(),fixture.histories.at(i).root()};}
ForkCheckpointToken commit(Primary& primary,const std::optional<ForkCheckpointToken>& previous,const std::string& snapshot,const Pending& pending,size_t bytes=batch_budget,size_t max_blobs=8192,size_t max_locators=256){
 return primary.shared->db->commit_fork_checkpoint_with_deposit_blobs(1337,primary.shared->genesis,previous,snapshot,bytes,pending.blobs(),max_blobs,pending.records(),max_locators);
}
std::string manifest(const std::shared_ptr<Context>& context,const Json& history){return Json{{"format","public-deposit-checkpoint-component-v1"},{"context",context->checkpoint_descriptor(f::checkpoint)},{"history",history}}.dump();}
std::shared_ptr<Context> restore_context(Owners& owners,const Json& descriptor,const Bytes& profile,const ConsensusRoot& network,const std::shared_ptr<Primary>& p){return owners.own(Context::from_checkpoint(descriptor,profile,network,limits(),p->fetch(),p->lookup(),{},{},f::checkpoint));}
ExecutionDepositHistory restore_history(const Json& reference,size_t i,const d::Fixture& fixture,const std::shared_ptr<Context>& context,const std::function<void()>& check=f::checkpoint,const DepositCheckpointTreeBudget& budget=tree_budget){return ExecutionDepositHistory::from_checkpoint_reference(reference,fixture.blocks.at(i).header,fixture.states.at(i),fixture.blocks.at(i).header.hash(),fixture.code_hash,d::nonzero_deposits,context,budget,check);}

void budget_and_locator_codec(const f::fs::path& path,const d::Fixture& fixture){
 f::test_stage="typed-primary-batch-and-legacy-budget";Owners owners;auto p=owners.open(path,fixture.blocks[0].header.hash(),ChainStore::OpenMode::ReadWrite,Bytes(32,0x45),fixture.ids[0]);
 const auto legacy=p->shared->db->commit_fork_checkpoint(1337,p->shared->genesis,{},"{}",2);const auto before=p->checkpoint(legacy).snapshot;
 bool bounded=false;try{p->shared->db->commit_fork_checkpoint_with_deposit_blobs(1337,p->shared->genesis,legacy,"{}",2,{},0,{},0);}catch(const std::length_error&){bounded=true;}
 f::check(bounded&&p->checkpoint(legacy).snapshot==before,"empty typed delta still bounds complete batch, without token mutation");
 const auto legacy_again=p->shared->db->commit_fork_checkpoint(1337,p->shared->genesis,legacy,"{}",2);f::check(legacy_again.sequence==legacy.sequence+1,"old payload-only API remains compatible");
 const Bytes profile(32,0x45);const auto network=fixture.ids[0];const auto key=fixture.blocks[10].header.hash();ConsensusRoot digest{};digest[1]=0x91;const uint64_t ordinal=0x0102030405060708ULL;
 const auto actual=Context::encode_locator({ordinal,digest});std::string expected(40,'\0');for(size_t i=0;i<8;++i)expected[i]=char(ordinal>>(8*i));std::copy(digest.begin(),digest.end(),expected.begin()+8);
 f::check(actual==expected&&Context::decode_locator(actual)==CanonicalDepositLocator{ordinal,digest},"independent little-endian40-byte locator oracle");
 const auto prefix="checkpoint-deposit-locator/v4/"+hex(profile).substr(2)+"/"+hex(raw(network)).substr(2);
 f::check(Context::locator_storage_key(profile,network,CanonicalDepositLookup::BeaconRoot,key)==prefix+"/b/"+hex(key).substr(2)&&Context::locator_storage_key(profile,network,CanonicalDepositLookup::ExecutionHash,key)==prefix+"/e/"+hex(key).substr(2),"typed namespace matches explicit chain-local codec");
 refuses([&]{Context::decode_locator(actual.substr(1));},"short locator refused");refuses([&]{Context::decode_locator(actual+"x");},"long locator refused");owners.close();
}

void execution_atomic_batch(const f::fs::path& path,const d::Fixture& fixture){
 f::test_stage="typed-execution-token-tip-cas-and-primary-batch";
 const auto genesis=fixture.blocks[0].header.hash();const Bytes profile(32,0x43);
 const CheckpointBlob a{sha("execution-before"),"execution-before"},b{sha("execution-after"),"execution-after"};
 const CheckpointDepositLocatorRecord first{{profile,raw(fixture.ids[0]),raw(fixture.ids[0]),false},0,a.digest};
 const CheckpointDepositLocatorRecord next{{profile,raw(fixture.ids[0]),fixture.blocks[1].header.hash(),true},1,b.digest};
 ForkCheckpointToken token;const std::string before="before-execution",after="after-execution";
 {
  ChainStore disk(path);disk.bind_chain(1337,genesis,"beacon");disk.commit(fixture.blocks[0],fixture.states[0],fixture.receipts[0]);
  const auto call=[&](const std::optional<ForkCheckpointToken>& expected,const Bytes& tip,const std::vector<CheckpointBlob>& blobs,const std::vector<CheckpointDepositLocatorRecord>& locators,size_t quota){return disk.commit_execution_checkpoint_with_deposit_blobs(1337,genesis,expected,tip,0,genesis,{{fixture.blocks[1],fixture.states[1],fixture.receipts[1]}},after,batch_budget,1,blobs,2,locators,quota);};
  token=disk.commit_execution_checkpoint_with_deposit_blobs(1337,genesis,{},genesis,0,genesis,{},before,batch_budget,0,{a},1,{first},1);
  const auto unchanged=[&]{const auto checkpoint=disk.fork_checkpoint(1337,genesis,batch_budget,token);f::check(checkpoint&&checkpoint->snapshot==before&&disk.latest()->header.hash()==genesis&&!disk.state_at(1)&&!disk.checkpoint_blob(1337,genesis,b.digest,batch_budget)&&!disk.checkpoint_deposit_locator(1337,genesis,next.lookup),"rejected execution batch leaves tip/token/state/CAS/locator unchanged");};
  auto stale=token;++stale.sequence;refuses([&]{call(stale,genesis,{b},{next},1);},"stale execution checkpoint token refused");unchanged();
  auto wrong_tip=genesis;wrong_tip[0]^=1;refuses([&]{call(token,wrong_tip,{b},{next},1);},"stale execution tip refused");unchanged();
  refuses([&]{call(token,genesis,{b},{next},0);},"execution locator quota refused");unchanged();
  auto conflict=first;conflict.digest[0]^=1;refuses([&]{call(token,genesis,{b},{next,conflict},2);},"execution immutable locator conflict refused");unchanged();
  auto malformed=next;malformed.lookup.profile_digest.pop_back();refuses([&]{call(token,genesis,{b},{malformed},1);},"execution malformed locator scope refused");unchanged();
  token=call(token,genesis,{b},{next},1);f::check(token.sequence==2&&disk.latest()->header.hash()==fixture.blocks[1].header.hash()&&disk.state_at(1)->root()==fixture.states[1].root(),"successful typed execution publishes exact real block and state");
 }
 const auto inventory=d::inventory(path);
 {
  ChainStore disk(path,ChainStore::OpenMode::ReadOnly);disk.require_existing_chain_binding(1337,genesis,"beacon");
  const auto checkpoint=disk.fork_checkpoint(1337,genesis,batch_budget,token);const auto locator=disk.checkpoint_deposit_locator(1337,genesis,next.lookup);
  f::check(checkpoint&&checkpoint->snapshot==after&&disk.latest()->header.hash()==fixture.blocks[1].header.hash()&&disk.state_at(1)->root()==fixture.states[1].root()&&disk.checkpoint_blob(1337,genesis,a.digest,batch_budget)==a.payload&&disk.checkpoint_blob(1337,genesis,b.digest,batch_budget)==b.payload&&locator&&Context::decode_locator(*locator).ordinal==1,"restart binds exact retained token, execution, old/new CAS and typed locator");
  refuses([&]{disk.checkpoint_deposit_locator(1338,genesis,next.lookup);},"wrong chain cannot address typed locator");auto wrong_genesis=genesis;wrong_genesis[0]^=1;refuses([&]{disk.checkpoint_deposit_locator(1337,wrong_genesis,next.lookup);},"wrong network cannot address typed locator");
  auto other_profile=next.lookup;other_profile.profile_digest[0]^=1;f::check(!disk.checkpoint_deposit_locator(1337,genesis,other_profile),"other profile namespace does not alias the committed locator");
 }
 f::check(d::inventory(path)==inventory,"readonly execution checkpoint reopening changes no physical primary file");
}

struct Published {
 f::fs::path path;Bytes genesis,profile;ConsensusRoot network;ForkCheckpointToken token;std::string snapshot;Json descriptor,history,history10;
};
Published stage_commit_history(const f::fs::path& path,const d::Fixture& fixture){
 f::test_stage="staged-history-primary-atomic-publication";Owners owners;const auto genesis=fixture.blocks[0].header.hash();const Bytes profile(32,0x46);const auto network=fixture.ids[0];auto p=owners.open(path,genesis,ChainStore::OpenMode::ReadWrite,profile,network);
 auto initial=std::make_shared<Pending>(profile,network);auto c0=owners.own(Context::create_staged(tuple(fixture,0),profile,limits(),initial->fetch(p->fetch()),initial->lookup(p->lookup()),initial->stage(),initial->stage_locator(),f::checkpoint));
 const auto h0=fixture.histories[0].checkpoint_reference(c0,tree_budget,f::checkpoint);const auto s0=manifest(c0,h0);const auto token0=commit(*p,{},s0,*initial);
 auto published=restore_context(owners,Json::parse(s0).at("context"),profile,network,p);const auto descriptor0=published->checkpoint_descriptor(f::checkpoint);
 auto pending=std::make_shared<Pending>(profile,network);auto candidate=owners.own(published->fork_staged(pending->fetch(p->fetch()),pending->lookup(p->lookup()),pending->stage(),pending->stage_locator(),f::checkpoint));
 auto current=restore_history(h0,0,fixture,candidate);
 Json history10;
 for(size_t i=1;i<=d::execution_blocks;++i){f::test_slot=i;current=current.append_verified_execution(fixture.blocks[i].header,fixture.states[i],fixture.receipts[i],fixture.code_hash,1,f::checkpoint);candidate->append_admitted(tuple(fixture,i),f::checkpoint);if(i==10)history10=current.checkpoint_reference(candidate,tree_budget,f::checkpoint);}
 const auto reference=current.checkpoint_reference(candidate,tree_budget,f::checkpoint);const auto snapshot=manifest(candidate,reference);const auto descriptor=candidate->checkpoint_descriptor(f::checkpoint);
 f::check(current.count()==32&&current.root()==fixture.histories.back().root()&&p->checkpoint(token0).snapshot==s0&&published->checkpoint_descriptor(f::checkpoint)==descriptor0,"staging preserves old published context and primary token");
 const CheckpointDepositLocatorKey future{profile,raw(network),raw(fixture.ids.back()),false};f::check(!p->shared->db->checkpoint_deposit_locator(1337,genesis,future),"future locator is absent before atomic commit");
 Bytes new_blob;for(const auto& [key,_]:pending->cas)if(!initial->cas.contains(key)){new_blob=key;break;}f::check(!new_blob.empty()&&!p->shared->db->checkpoint_blob(1337,genesis,new_blob,batch_budget),"new staged CAS absent from primary");
 auto wrong=token0;++wrong.sequence;refuses([&]{commit(*p,wrong,snapshot,*pending);},"stale token refuses full candidate");
 refuses([&]{commit(*p,token0,snapshot,*pending,batch_budget,8192,pending->locators.size()-1);},"locator quota refuses before any primary write");
 f::check(p->checkpoint(token0).snapshot==s0&&!p->shared->db->checkpoint_blob(1337,genesis,new_blob,batch_budget)&&!p->shared->db->checkpoint_deposit_locator(1337,genesis,future),"refused batch leaves token, CAS and locators unchanged");
 auto conflict=*pending;const auto old=initial->records().front();auto changed=old;changed.digest[0]^=1;conflict.locators[Context::locator_storage_key(profile,network,old.lookup.execution_hash?CanonicalDepositLookup::ExecutionHash:CanonicalDepositLookup::BeaconRoot,old.lookup.key)]=changed;
 refuses([&]{commit(*p,token0,snapshot,conflict);},"existing locator cannot be overwritten by a conflicting digest");f::check(p->checkpoint(token0).snapshot==s0,"conflicting locator preserves exact previous checkpoint");
 const auto token=commit(*p,token0,snapshot,*pending);f::check(token.sequence==token0.sequence+1&&p->checkpoint(token).snapshot==snapshot,"one primary batch publishes token, CAS and typed locators");
 const auto found=p->shared->db->checkpoint_deposit_locator(1337,genesis,future);f::check(found&&Context::decode_locator(*found).ordinal==96,"published locator available only after commit");
 refuses([&]{published->cursor_for_root(fixture.ids.back(),f::checkpoint);},"new durable locator cannot extend old published-tip authority");
 pending->clear();owners.close();return {path,genesis,profile,network,token,snapshot,descriptor,reference,history10};
}

void restart_closure_and_refusals(const Published& publication,const d::Fixture& fixture){
 f::test_stage="durable-restart-deep-closure-and-domain-refusals";const auto before=d::inventory(publication.path);Owners owners;auto p=owners.open(publication.path,publication.genesis,ChainStore::OpenMode::ReadOnly,publication.profile,publication.network);f::check(p->checkpoint(publication.token).snapshot==publication.snapshot,"restart authenticates independently retained token");
 auto context=restore_context(owners,publication.descriptor,publication.profile,publication.network,p);auto history=restore_history(publication.history,96,fixture,context);
 f::check(history.snapshot(f::checkpoint)==fixture.histories.back().snapshot(f::checkpoint),"durable history exact old-v1 export and root equal memory oracle");
 for(uint64_t i=0;i<32;++i)d::same_proof(history.proof(i,f::checkpoint),fixture.histories.back().proof(i,f::checkpoint));
 const auto old=context->find_before(context->tip_cursor(f::checkpoint),fixture.blocks[17].header.hash(),17,fixture.histories[17].root(),f::checkpoint);f::check(old&&old->beacon.slot==17,"old nonzero canonical tuple survives restart");
 const auto prefix=history.authenticated_prefix(17,fixture.blocks[17].header.hash(),fixture.histories[17].root(),f::checkpoint);for(uint64_t i=0;i<17;++i)d::same_proof(prefix.proof(i,f::checkpoint),fixture.histories[17].proof(i,f::checkpoint));
 size_t tuple_refs=0;context->visit_checkpoint_closure([&](const CanonicalDepositBlobRef&){++tuple_refs;},f::checkpoint);std::vector<CanonicalDepositBlobRef> nodes;history.authenticate_checkpoint_closure(tree_budget,[&](const auto& ref){nodes.push_back(ref);},f::checkpoint);f::check(tuple_refs>=97&&!nodes.empty(),"complete tuple and history closures really traversed");
 auto deep=std::find_if(nodes.begin(),nodes.end(),[](const auto& ref){return ref.bytes==194;});f::check(deep!=nodes.end()&&hex(raw(deep->digest))!=publication.history[5][0].get<std::string>(),"deep leaf distinct from known descriptor root");const auto missing=raw(deep->digest);const auto source=p->fetch();
 auto bad_fetch=[&](bool corrupt){return Context::Fetch([=](const Bytes& key,size_t maximum)->std::optional<std::string>{auto value=source(key,maximum);if(key==missing){if(!corrupt)return {};f::check(bool(value),"existing deep corruption premise");(*value)[value->size()-1]^=1;}return value;});};
 for(bool corrupt:{false,true}){auto changed=owners.own(Context::from_checkpoint(publication.descriptor,publication.profile,publication.network,limits(),bad_fetch(corrupt),p->lookup(),{},{},f::checkpoint));refuses([&]{restore_history(publication.history,96,fixture,changed);},"full restore refuses missing/corrupt deep leaf, not merely descriptor root");}
 auto wrong_profile=publication.profile;wrong_profile[0]^=1;refuses([&]{Context::from_checkpoint(publication.descriptor,wrong_profile,publication.network,limits(),p->fetch(),p->lookup(),{},{},f::checkpoint);},"wrong externally supplied profile refused");
 auto wrong_network=publication.network;wrong_network[0]^=1;refuses([&]{restore_context(owners,publication.descriptor,publication.profile,wrong_network,p);},"wrong external network root refused");
 const auto durable_lookup=p->lookup();const auto key17=fixture.blocks[17].header.hash();
 for(bool corrupt:{false,true}){const auto lookup=[durable_lookup,key17,corrupt](CanonicalDepositLookup kind,const Bytes& key)->std::optional<CanonicalDepositLocator>{auto found=durable_lookup(kind,key);if(kind==CanonicalDepositLookup::ExecutionHash&&key==key17){if(!corrupt)return {};f::check(bool(found),"existing locator corruption premise");found->digest[0]^=1;}return found;};refuses([&]{Context::from_checkpoint(publication.descriptor,publication.profile,publication.network,limits(),p->fetch(),lookup,{},{},f::checkpoint);},"missing/corrupt durable lookup cannot authenticate a canonical row");}
 auto short_limits=limits();short_limits.maximum_tuples=96;refuses([&]{Context::from_checkpoint(publication.descriptor,publication.profile,publication.network,short_limits,p->fetch(),p->lookup(),{},{},f::checkpoint);},"external context quota is not learned from descriptor");
 for(size_t field:{size_t(1),size_t(3),size_t(4)}){auto changed=publication.history;auto value=changed[field].get<std::string>();value.back()=value.back()=='0'?'1':'0';changed[field]=value;refuses([&]{restore_history(changed,96,fixture,context);},"history execution/mixed/tree-root mutation refused");}
 auto wrong_count=publication.history;wrong_count[2]="0x21";refuses([&]{restore_history(wrong_count,96,fixture,context);},"history count beyond external deposit cap refused");
 refuses([&]{restore_history(fixture.histories.back().snapshot(f::checkpoint),96,fixture,context);},"old-v1 history is not autodetected as explicit-v2 reference");
 const auto sibling=*fixture.same_root_sibling;refuses([&]{ExecutionDepositHistory::from_checkpoint_reference(publication.history10,sibling.execution_block.header,sibling.execution_state,sibling.execution_block.header.hash(),fixture.code_hash,32,context,tree_budget,f::checkpoint);},"same count/root sibling cannot replace external execution binding");
 size_t checks=0;bool expired=false;try{restore_history(publication.history,96,fixture,context,[&]{if(++checks==5)throw ExpiredCapellaDuty("checkpoint component cancelled");});}catch(const ExpiredCapellaDuty& error){expired=std::string(error.what())=="checkpoint component cancelled";}
 f::check(expired&&checks==5&&p->checkpoint(publication.token).snapshot==publication.snapshot,"typed cancellation preserves original primary commitment");
 auto held=context->lease_node_blob(*deep,f::checkpoint);const auto held_value=*held;p->assert_paired_close(raw(deep->digest),raw(fixture.ids.back()));p->close();refuses([&]{context->lease_node_blob(*deep,f::checkpoint);},"closed durable provider refuses even cached node read");f::check(*held==held_value,"immutable byte lease survives provider close");owners.close();f::check(d::inventory(publication.path)==before,"read-only restart/refusal/close paths leave physical primary unchanged");
}

// Imported/genesis storage can contain an opaque branch hash. Matching its
// contract root does not prove the explicit CAS tree is a dense [0,count) prefix.
void dense_prefix_reference(Owners& owners,const std::shared_ptr<Context>& source,const std::shared_ptr<Pending>& pending,const std::shared_ptr<Primary>& primary,const d::Fixture& fixture){
 const auto deposit=fixture.histories[2].proof(1,f::checkpoint);std::array<ConsensusRoot,33> zeros{};
 for(size_t depth=1;depth<=32;++depth)zeros[depth]=consensus_signing_root(zeros[depth-1],zeros[depth-1]);
 std::map<Bytes,std::string> forged;
 const auto remember=[&](const Bytes& wire){const auto digest=sha256(wire);ConsensusRoot root{};std::copy(digest.begin(),digest.end(),root.begin());forged.emplace(digest,std::string(wire.begin(),wire.end()));return CanonicalDepositBlobRef{root,uint64_t(wire.size())};};
 const auto head=[](uint8_t kind,uint8_t depth){return Bytes{'N','A','N','O','D','P',1,0,kind,depth};};
 auto leaf=head(0,0);leaf.insert(leaf.end(),deposit.message.public_key.begin(),deposit.message.public_key.end());leaf.insert(leaf.end(),deposit.message.withdrawal_credentials.begin(),deposit.message.withdrawal_credentials.end());for(unsigned i=0;i<8;++i)leaf.push_back(uint8_t(deposit.message.amount_gwei>>(8*i)));leaf.insert(leaf.end(),deposit.signature.begin(),deposit.signature.end());
 auto reference=remember(leaf);auto tree=consensus_deposit_data_root(deposit);ConsensusRoot opaque_branch{};
 for(unsigned depth=1;depth<=32;++depth){
  auto wire=head(1,uint8_t(depth));const auto child=[&]{wire.push_back(1);wire.push_back(uint8_t(depth-1));wire.insert(wire.end(),tree.begin(),tree.end());wire.insert(wire.end(),reference.digest.begin(),reference.digest.end());for(unsigned i=0;i<8;++i)wire.push_back(uint8_t(reference.bytes>>(8*i)));};
  if(depth==1){wire.push_back(0);child();tree=consensus_signing_root(zeros[0],tree);opaque_branch=tree;}
  else{child();wire.push_back(0);tree=consensus_signing_root(tree,zeros[depth-1]);}
  reference=remember(wire);
 }
 // Preserve count2 and contract code; only its opaque branch[1] is changed.
 // The two actual static EVM views below prove this is not merely a bad hash,
 // malformed encoding, or descriptor that fails contract authentication early.
 auto state=fixture.states[2];const auto original_state_root=state.root();f::check(state.get_storage(ethereum_deposit_contract,32)==2,"fixed contract count storage layout");state.set_storage(ethereum_deposit_contract,1,integer(raw(opaque_branch)));
 auto header=fixture.blocks[2].header;header.state_root=state.root();ConsensusRoot count_chunk{};count_chunk[0]=2;const auto mixed=consensus_signing_root(tree,count_chunk);
 const auto view=[&](const char* signature){State isolated=state;ExecutionContext execution;execution.revision=EvmRevision::Paris;execution.address=ethereum_deposit_contract;execution.code=state.get(ethereum_deposit_contract).code;execution.data=keccak(Bytes(signature,signature+std::char_traits<char>::length(signature)));execution.data.resize(4);execution.is_static=true;execution.number=header.number;execution.timestamp=header.timestamp;execution.checkpoint=f::checkpoint;const auto result=EVM(isolated).execute(execution);f::check(result.success,"opaque imported branch contract view succeeds");return result.return_data;};
 Bytes count_view(96);count_view[31]=32;count_view[63]=8;count_view[64]=2;
 f::check(view("get_deposit_root()")==raw(mixed)&&view("get_deposit_count()")==count_view&&keccak(state.get(ethereum_deposit_contract).code)==fixture.code_hash&&header.state_root==state.root(),"sparse imported tree has coherent real contract views, code, header and state root");
 const auto underlying=pending->fetch(primary->fetch());const auto lookup=pending->lookup(primary->lookup());
 const auto fetch=[forged=std::move(forged),underlying](const Bytes& key,size_t maximum)->std::optional<std::string>{const auto at=forged.find(key);if(at==forged.end())return underlying(key,maximum);f::check(at->second.size()<=maximum,"sparse fixture CAS bound");return at->second;};
 auto context=owners.own(source->fork_staged(fetch,lookup,{},{},f::checkpoint));
 const auto encoded=Json::array({"nano-execution-deposit-history-v2",hex(header.hash()),"0x2",hex(raw(mixed)),hex(raw(tree)),Json::array({hex(raw(reference.digest)),quantity(Int(std::to_string(reference.bytes)))})});
 bool dense_refusal=false;try{ExecutionDepositHistory::from_checkpoint_reference(encoded,header,state,header.hash(),fixture.code_hash,32,context,tree_budget,f::checkpoint);}catch(const std::invalid_argument& error){dense_refusal=std::string(error.what())=="deposit checkpoint tree is not a dense prefix";}
 f::check(dense_refusal&&fixture.states[2].root()==original_state_root,"full v2 restore rejects leaf0 hole with valid leaf1/count2, not a hash or contract-view mismatch");
}

void tree_budget_and_cancel(const f::fs::path& cancelled_path,const Published& publication,const d::Fixture& fixture){
 f::test_stage="full-tree-budget-independent-of-deduplicated-emissions";Owners owners;const auto before=d::inventory(publication.path);const Bytes profile(32,0x47);auto primary=owners.open(publication.path,publication.genesis,ChainStore::OpenMode::ReadOnly,profile,publication.network);
 auto overlay=std::make_shared<Pending>(profile,publication.network);
 auto context=owners.own(Context::create_staged(tuple(fixture,0),profile,limits(),overlay->fetch(primary->fetch()),overlay->lookup(primary->lookup()),overlay->stage(),overlay->stage_locator(),f::checkpoint));
 const auto reference=fixture.histories.back().checkpoint_reference(context,tree_budget,f::checkpoint);
 f::check(reference==publication.history&&context->usage().blob_records==0&&context->usage().blob_bytes==0,"prepopulated primary CAS reuses the complete real tree with zero new blob emissions");
 const auto root_bytes=u64(quantity(reference[5][1]));f::check(root_bytes>0,"nonempty externally pinned tree root");
 const auto restore=[&](const Json& ref,const DepositCheckpointTreeBudget& budget){return restore_history(ref,96,fixture,context,f::checkpoint,budget);};
 for(const DepositCheckpointTreeBudget budget:std::array<DepositCheckpointTreeBudget,3>{{{0,tree_budget.maximum_encoded_bytes},{tree_budget.maximum_visits,0},{tree_budget.maximum_visits,root_bytes-1}}}){
  const auto fetches=primary->shared->fetches;refuses([&]{restore(reference,budget);},"zero/insufficient tree byte budget refuses before fetching the root");f::check(primary->shared->fetches==fetches,"tree budget charged before first fetch despite CAS hit");
 }
 const auto visits_before=primary->shared->fetches;refuses([&]{restore(reference,{1,tree_budget.maximum_encoded_bytes});},"one-visit tree budget refuses nontrivial full closure");f::check(primary->shared->fetches>visits_before&&primary->shared->fetches-visits_before<=1,"visit budget charged before every fetch/expand, including cache hit");
 for(const char* bad:{"0x0","0x10000000000000000"}){auto invalid=reference;invalid[5][1]=bad;const auto fetches=primary->shared->fetches;refuses([&]{restore(invalid,tree_budget);},"zero/overflow encoded reference length refused");f::check(primary->shared->fetches==fetches,"invalid reference length refuses before tree fetch");}
 auto malformed=reference;malformed[5]=Json::array({reference[5][0]});refuses([&]{restore(malformed,tree_budget);},"malformed nonempty reference refused");
 const auto recovered=restore(reference,tree_budget);f::check(recovered.snapshot(f::checkpoint)==fixture.histories.back().snapshot(f::checkpoint),"explicit sufficient tree budget authenticates unchanged memory oracle");
 dense_prefix_reference(owners,context,overlay,primary,fixture);
 // A failed candidate is discarded in full. It never becomes a readonly
 // published context merely because some node CAS writes already succeeded.
 f::test_stage="typed-cancel-after-real-private-staging";auto empty=owners.open(cancelled_path,publication.genesis,ChainStore::OpenMode::ReadWrite,publication.profile,publication.network);auto pending=std::make_shared<Pending>(publication.profile,publication.network);std::shared_ptr<Context> cancelled;
 const auto check=[&]{f::checkpoint();if(cancelled&&cancelled->usage().blob_records>=2)throw ExpiredCapellaDuty("cancel after two private node emissions");};
 cancelled=owners.own(Context::create_staged(tuple(fixture,0),publication.profile,limits(),pending->fetch(empty->fetch()),pending->lookup(empty->lookup()),pending->stage(),pending->stage_locator(),check));
 const auto original=fixture.histories.back().snapshot(f::checkpoint);bool typed=false;
 try{fixture.histories.back().checkpoint_reference(cancelled,tree_budget,check);}catch(const ExpiredCapellaDuty& error){typed=std::string(error.what())=="cancel after two private node emissions";}
 f::check(typed&&cancelled->usage().blob_records>=2&&pending->writes>=2&&!empty->shared->db->fork_checkpoint(1337,publication.genesis,batch_budget)&&fixture.histories.back().snapshot(f::checkpoint)==original,"typed cancellation after real stage writes leaves primary and memory input unpublished/unchanged");
 for(const auto& [key,_]:pending->cas)f::check(!empty->shared->db->checkpoint_blob(1337,publication.genesis,key,batch_budget),"cancelled private blob absent from primary");
 cancelled->close();pending->clear();owners.close();f::check(d::inventory(publication.path)==before,"emission/quota/refusal checks never write the published primary");
}

void store_v4_roundtrip(const f::fs::path& path){
 f::test_stage="real-valid-anchor-store-v4-codec";auto fixture=f::fixture();const auto root=fixture.initial.timed.view.anchor;const auto anchor=fixture.initial.blocks.at(root).lease(f::checkpoint);const auto legacy=capella_store_snapshot(fixture.initial,fixture.rules,fixture.limits,f::checkpoint);const auto profile_text=Json::parse(legacy).at("profile").dump();const auto profile=sha(profile_text);const auto genesis=fixture.genesis.block.header.hash();Owners owners;auto p=owners.open(path,genesis,ChainStore::OpenMode::ReadWrite,profile,root);auto pending=std::make_shared<Pending>(profile,root);
 f::check(fixture.rules.native_deposit_votes&&fixture.rules.deposit_contract_code_hash.size()==32&&bool(anchor->deposit_history),"real signed anchor supports native-deposit Store v4 profile");
 const CapellaDepositAncestor tuple0{anchor->header,anchor->execution_block.header,anchor->deposit_history->count(),anchor->deposit_history->root()};
 const auto bind_store=[&](const std::shared_ptr<Context>& context){auto store=fixture.initial;store.blocks.at(root).writable(f::checkpoint).deposit_history=std::make_shared<const ExecutionDepositHistory>(anchor->deposit_history->with_cold_storage(context,f::checkpoint));store.deposit_ancestry=CapellaDepositAncestry{context,context->anchor_cursor(f::checkpoint),root};return store;};
 // Abort after a genuine block Stage callback. The candidate and all pending
 // material are discarded; a later successful attempt gets new owners.
 auto failed_pending=std::make_shared<Pending>(profile,root);auto failed_context=owners.own(Context::create_staged(tuple0,profile,limits(),failed_pending->fetch(p->fetch()),failed_pending->lookup(p->lookup()),failed_pending->stage(),failed_pending->stage_locator(),f::checkpoint));
 const auto failed_store=bind_store(failed_context);size_t failed_stages=0;bool late=false;
 try{capella_store_deposit_reference_snapshot(failed_store,fixture.rules,fixture.limits,failed_context,tree_budget,[&](const CapellaColdReference& ref,std::string_view bytes){failed_pending->stage()(ref.digest,std::string(bytes));++failed_stages;throw ExpiredCapellaDuty("stop after consensus block stage");},f::checkpoint);}catch(const ExpiredCapellaDuty& error){late=std::string(error.what())=="stop after consensus block stage";}
 f::check(late&&failed_stages==1&&!p->shared->db->fork_checkpoint(1337,genesis,batch_budget),"late writer cancellation leaves real staged block private and no primary checkpoint");for(const auto& [key,_]:failed_pending->cas)f::check(!p->shared->db->checkpoint_blob(1337,genesis,key,batch_budget),"failed v4 writer publishes no partial CAS");failed_context->close();failed_pending->clear();
 auto scratch=owners.own(Context::create(path.parent_path()/"unpublished-audit-scratch",tuple0,profile,limits(),f::checkpoint));const auto scratch_store=bind_store(scratch);size_t scratch_stages=0;refuses([&]{capella_store_deposit_reference_snapshot(scratch_store,fixture.rules,fixture.limits,scratch,tree_budget,[&](const CapellaColdReference&,std::string_view){++scratch_stages;},f::checkpoint);},"scratch audit context is not a durable checkpoint descriptor");f::check(scratch_stages==0,"scratch context refuses before external block staging");scratch->close();
 auto context=owners.own(Context::create_staged(tuple0,profile,limits(),pending->fetch(p->fetch()),pending->lookup(p->lookup()),pending->stage(),pending->stage_locator(),f::checkpoint));
 auto stage=[&](const CapellaColdReference& ref,std::string_view bytes){f::check(sha(bytes)==ref.digest&&bytes.size()==ref.bytes,"staged real consensus row commitment");pending->stage()(ref.digest,std::string(bytes));};
 auto store=bind_store(context);
 const auto encoded=capella_store_deposit_reference_snapshot(store,fixture.rules,fixture.limits,context,tree_budget,stage,f::checkpoint);const auto document=Json::parse(encoded);f::check(document.at("format")=="nano-capella-reference-store-v2"&&document.at("snapshot").at("format")=="nano-capella-store-v4"&&document.at("snapshot").size()==15,"explicit real Store v4, not legacy auto-detection");
 f::check(!p->shared->db->fork_checkpoint(1337,genesis,batch_budget),"writer only stages before primary publication");const auto token=commit(*p,{},encoded,*pending);owners.close();const auto before=d::inventory(path);auto ro=owners.open(path,genesis,ChainStore::OpenMode::ReadOnly,profile,root);f::check(ro->checkpoint(token).snapshot==encoded,"published Store manifest externally authenticated");
 auto restore=[&](const std::string& bytes,const Bytes& digest){return capella_store_from_deposit_reference_snapshot(bytes,digest,fixture.rules,fixture.limits,1,fixture.limits.envelope_bytes,limits(),tree_budget,ro->fetch(),ro->lookup(),f::checkpoint);};
 auto restored=restore(encoded,sha(encoded));f::check(restored.deposit_ancestry&&restored.blocks.at(root).lease(f::checkpoint)->state.ssz==anchor->state.ssz&&restored.blocks.at(root).lease(f::checkpoint)->execution_state.root()==anchor->execution_state.root(),"real Store SSZ and execution survive v4 restart");owners.own(restored.deposit_ancestry->context);
 size_t references=0;authenticate_capella_deposit_checkpoint_closure(restored,tree_budget,[&](const CanonicalDepositBlobRef&){++references;},f::checkpoint);f::check(references>0,"Store closure authenticates canonical tuple bindings");
 auto bad_digest=sha(encoded);bad_digest[0]^=1;const auto fetches=ro->shared->fetches;refuses([&]{restore(encoded,bad_digest);},"external manifest digest refused before provider");f::check(ro->shared->fetches==fetches,"wrong digest performs no fetch");
 auto mixed=document;mixed["format"]="nano-capella-reference-store-v1";const auto mixed_text=mixed.dump();refuses([&]{restore(mixed_text,sha(mixed_text));},"old/new store wrapper mix refused even with matching supplied digest");
 auto extra=document;extra["snapshot"]["unknown"]=true;const auto extra_text=extra.dump();refuses([&]{restore(extra_text,sha(extra_text));},"unknown v4 key refused");
 refuses([&]{capella_store_from_reference_snapshot(encoded,sha(encoded),fixture.rules,fixture.limits,1,fixture.limits.envelope_bytes,ro->fetch(),f::checkpoint);},"legacy reader rejects new format");
 ro->close();refuses([&]{restored.blocks.at(root).lease(f::checkpoint);},"closed provider refuses cached durable consensus row");owners.close();f::check(d::inventory(path)==before,"real Store v4 reads and refusals do not mutate primary");
}
}

int main(){try{
 ScopedChainStoreMemoryPolicy policy(d::memory_policy);f::Temp temporary;d::Fixture fixture;
 budget_and_locator_codec(temporary.path/"budget",fixture);
 execution_atomic_batch(temporary.path/"execution",fixture);
 const auto publication=stage_commit_history(temporary.path/"history",fixture);
 restart_closure_and_refusals(publication,fixture);
 tree_budget_and_cancel(temporary.path/"cancelled",publication,fixture);
 store_v4_roundtrip(temporary.path/"store");
 f::checkpoint();f::check(d::files_opened==d::files_closed,"all physical fence descriptors closed");
 std::cout<<Json{{"scenario","canonical-deposit-checkpoint-component"},{"passed",true},{"groups",6},{"nonzeroDeposits",32},{"nonzeroExecutionHeaders",96},{"syntheticBeaconTuples",97},{"authenticatedBeaconAdmissionsQualified",false},{"separateValidSignedStoreSingleton",true},{"historyProofsCompared",49},{"realPrimaryBatchExercised",true},{"executionPrimaryBatchExercised",true},{"explicitStoreV4Exercised",true},{"deduplicatedEmissionCounterIsClosureBound",false},{"readOnlyPrimaryUnchanged",true},{"databaseFenceFilesOpened",d::files_opened},{"databaseFenceFilesClosed",d::files_closed},{"runtimeConnected",false},{"migrationQualified",false},{"inFlightCloseDrainQualified",false},{"allAllocationsBounded",false},{"networkConsensusQualified",false},{"physicalPowerLossQualified",false},{"productionQualified",false}}.dump()<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<Json{{"scenario","canonical-deposit-checkpoint-component"},{"passed",false},{"stage",f::test_stage},{"reason",error.what()},{"productionQualified",false}}.dump()<<'\n';return 1;}}
