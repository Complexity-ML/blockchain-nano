// Public offline tests of source authentication, not execution/store/runtime trust.
// No DB, campaign identity, signer callback, verifier substitution, or wall spoofing.
#include "support/altair_transition_rules.hpp"
#include "nano/consensus_capella_fork_store.hpp"
#include "nano/consensus_capella_store_snapshot.hpp"
#include "nano/consensus_bls.hpp"
#include "nano/committee.hpp"
#include "../src/consensus_capella_target_source_auth.hpp"
#include "blst.h"
#include <openssl/crypto.h>
#include <chrono>
#include <sys/resource.h>
#include <type_traits>
#include <algorithm>
#include <array>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>
static_assert(std::is_constructible_v<nano::capella_detail::TargetSourceFingerprintHasher,const std::function<void()>&>);
static_assert(!std::is_constructible_v<nano::capella_detail::TargetSourceFingerprintHasher,std::function<void()>&&>);
static_assert(!std::is_constructible_v<nano::capella_detail::TargetSourceFingerprintHasher,const std::function<void()>&&>);
using namespace nano;using namespace test;using namespace test::phase0_minimal;
namespace {
const auto began=std::chrono::steady_clock::now();size_t categories=0,signatures=0;
uint64_t elapsed(){return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-began).count();}
uint64_t rss(){rusage r{};check(::getrusage(RUSAGE_SELF,&r)==0,"RSS observation");
#ifdef __APPLE__
 return uint64_t(r.ru_maxrss)/1024;
#else
 return uint64_t(r.ru_maxrss);
#endif
}
uint64_t cpu(){rusage r{};check(::getrusage(RUSAGE_SELF,&r)==0,"CPU observation");return uint64_t(r.ru_utime.tv_sec+r.ru_stime.tv_sec)*1000000+uint64_t(r.ru_utime.tv_usec+r.ru_stime.tv_usec);}
void budget(){check(elapsed()<60000&&rss()<=131072,"source authentication resource budget");}
void passed(const char* name){budget();++categories;std::cout<<Json{{"scenario",name},{"result","PASS"},{"productionQualified",false}}.dump()<<std::endl;}
void put(Bytes& bytes,size_t at,uint64_t value,size_t width=8){check(at<=bytes.size()&&width<=bytes.size()-at,"test field bounds");for(size_t i=0;i<width;++i)bytes[at+i]=uint8_t(value>>(8*i));}
CapellaEpochRules epochs(const CapellaForkBlockRules& r){return {r.transition.consensus.epoch,r.transition.execution,r.transition.maximum_state_bytes};}
template<class E,class F>void raises(F&& action,const char* label){bool rejected=false;try{action();}catch(const E&){rejected=true;}check(rejected,label);}
struct Cancellation:std::runtime_error{Cancellation():std::runtime_error("target-source-test-cancel"){} }; 
std::string failure(const std::function<void()>& f){try{f();}catch(const std::length_error& e){return std::string("length:")+e.what();}catch(const std::invalid_argument& e){return std::string("invalid:")+e.what();}throw std::runtime_error("changed source did not fail authentication");}
struct Key {
 blst_scalar scalar{};Bytes public_key;
 explicit Key(uint64_t index){uint64_t words[4]{700001+index,0,0,0};blst_scalar_from_uint64(&scalar,words);blst_p1 p;blst_sk_to_pk_in_g1(&p,&scalar);public_key.resize(48);blst_p1_compress(public_key.data(),&p);}
 ~Key(){OPENSSL_cleanse(&scalar,sizeof(scalar));}
 Bytes sign(const ConsensusRoot& root)const{budget();check(++signatures<=8,"synthetic signature bound");static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";blst_p2 h,s;blst_hash_to_g2(&h,root.data(),root.size(),reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);blst_sign_pk_in_g1(&s,&h,&scalar);Bytes out(96);blst_p2_compress(out.data(),&s);return out;}
};
ConsensusRoot leaf(std::span<const uint8_t> b){check(b.size()<=32,"SSZ leaf");ConsensusRoot r{};std::copy(b.begin(),b.end(),r.begin());return r;}
ConsensusRoot pair(const ConsensusRoot& a,const ConsensusRoot& b){Bytes bytes(a.begin(),a.end());bytes.insert(bytes.end(),b.begin(),b.end());const auto hash=sha256(bytes);ConsensusRoot out{};std::copy(hash.begin(),hash.end(),out.begin());return out;}
ConsensusRoot registry_root(std::span<const uint8_t> bytes,uint64_t limit){
 check(bytes.size()==64*121&&limit>=64&&(limit&(limit-1))==0,"64 validator list fixture");std::array<ConsensusRoot,64> rows{};
 for(size_t i=0;i<64;++i){const auto b=bytes.subspan(i*121,121);std::array<ConsensusRoot,8> fields{pair(leaf(b.first(32)),leaf(b.subspan(32,16))),leaf(b.subspan(48,32)),leaf(b.subspan(80,8)),leaf(b.subspan(88,1)),leaf(b.subspan(89,8)),leaf(b.subspan(97,8)),leaf(b.subspan(105,8)),leaf(b.subspan(113,8))};for(size_t n=8;n>1;n/=2)for(size_t j=0;j<n/2;++j)fields[j]=pair(fields[j*2],fields[j*2+1]);rows[i]=fields[0];}
 for(size_t n=64;n>1;n/=2)for(size_t j=0;j<n/2;++j)rows[j]=pair(rows[j*2],rows[j*2+1]);ConsensusRoot zero{},root=rows[0];for(unsigned i=0;i<6;++i)zero=pair(zero,zero);for(uint64_t n=64;n<limit;n*=2){root=pair(root,zero);zero=pair(zero,zero);}ConsensusRoot count{};count[0]=64;return pair(root,count);
}
struct Fixture {CapellaForkBlockRules rules;CapellaSlotState golden;Bytes post;CapellaForkStore store;ConsensusCheckpoint target;};
CapellaForkStore store_for(const CapellaSlotState& state,const CapellaForkBlockRules& r){
 const auto root=capella_beacon_state_root(state.ssz,r.transition.consensus.state,r.transition.execution,r.transition.maximum_state_bytes,budget);
 const ConsensusBeaconHeader h{little(state.ssz,64,8),little(state.ssz,72,8),root_at(state.ssz,80),root,root_at(state.ssz,144)};
 const auto ctx=capella_fork_block_context(state,h,epochs(r),r.transition.consensus.state,budget);check(little(state.ssz,40,8)==h.slot,"public fixture must name an actual block slot");
 CapellaForkStore out;const auto cp=ConsensusCheckpoint{h.slot/r.transition.consensus.epoch.ffg.slots_per_epoch,ctx.block_root};out.blocks.emplace(cp.root,CapellaForkBlock{h,state,{}, {},{}, {}});
 out.network_anchor=cp.root;auto& v=out.timed.view;v.anchor=cp.root;v.justified=cp;v.finalized=cp;v.current_epoch=cp.epoch;v.justified_registry=ctx.registry;v.nodes.emplace(cp.root,Phase0ForkNode{h.slot,h.parent_root,cp.epoch,cp.epoch});out.timed.clock={little(state.ssz,0,8)*1000,little(state.ssz,0,8)*1000+(h.slot+1)*r.slot_duration_ms,cp,cp};return out;
}
Fixture fixture(){
 const auto fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures",folder=fixtures/"consensus-capella-slots-official",refs=fixtures/"consensus-bellatrix-containers-official";
 const auto m=load(folder/"manifest.json");check(m.at("commit")=="194a91fd9e6f97118156421aac4c80e6e47a9a7f","official Capella corpus revision");bool found=false;
 for(const auto& row:m.at("cases"))if(row.at("name")=="slots_1"){found=true;for(const auto& [name,pin]:row.at("files").items()){const auto b=binary(folder/"slots_1"/name);check(b.size()==pin.at("bytes")&&hex(sha256(b)).substr(2)==pin.at("sha256").get<std::string>(),"official slot bytes pin");}}check(found,"slots_1 fixture missing");
 auto common=altair_transition_rules(fixtures);const auto bp=read(refs/"minimal-bellatrix.yaml"),cp=read(fixtures/"consensus-capella-withdrawals-official/minimal-capella.yaml");auto b=[&](const char* k){return parameter(bp,k);};
 check(hex(sha256(binary(refs/"minimal-bellatrix.yaml"))).substr(2)=="96b8fbfa1a4af173be05f0dc5563af3b9df9e8327277d8f1957a4fdfa49b7cd9","Bellatrix preset pin");
 check(hex(sha256(binary(fixtures/"consensus-capella-withdrawals-official/minimal-capella.yaml"))).substr(2)=="23fa8aeb37009976ba3f88959f91024f6dd19bde3c999011d7d6eff4ab61e523","Capella preset pin");
 check(hex(sha256(binary(fixtures/"consensus-altair-fork-head-official/minimal-config-v1.6.0-beta.0.yaml"))).substr(2)=="97457b240b976230b57ddddb4f3ce61688d3f7e5d983803c1d5b8cc4b159bfd3","minimal clock preset pin");
 common.epoch.rewards.inactivity_penalty_quotient=b("INACTIVITY_PENALTY_QUOTIENT_BELLATRIX");common.epoch.slashing.minimum_penalty_quotient=b("MIN_SLASHING_PENALTY_QUOTIENT_BELLATRIX");common.epoch.slashing.proportional_slashing_multiplier=b("PROPORTIONAL_SLASHING_MULTIPLIER_BELLATRIX");
 const auto seconds=parameter(read(fixtures/"consensus-altair-fork-head-official/minimal-config-v1.6.0-beta.0.yaml"),"SECONDS_PER_SLOT");
 CapellaTransitionRules tr{common,{uint32_t(b("MAX_EXTRA_DATA_BYTES")),uint32_t(b("MAX_BYTES_PER_TRANSACTION")),uint32_t(b("MAX_TRANSACTIONS_PER_PAYLOAD"))},4*1024*1024,seconds,{common.deposit.maximum_effective_balance,parameter(cp,"MAX_WITHDRAWALS_PER_PAYLOAD"),parameter(cp,"MAX_VALIDATORS_PER_WITHDRAWALS_SWEEP")},common.deposit.genesis_fork_version,uint32_t(parameter(cp,"MAX_BLS_TO_EXECUTION_CHANGES")),1337,ReplayPolicy::RequireChainId};
 CapellaForkBlockRules rules{tr,seconds*1000,seconds*1000/3,40};CapellaSlotState state{binary(folder/"slots_1/pre.ssz"),std::nullopt};auto store=store_for(state,rules);return {rules,state,binary(folder/"slots_1/post.ssz"),store,store.timed.view.justified};
}
void same_context(const Phase0ForkVoteContext& a,const Phase0ForkVoteContext& b){check(a.checkpoint==b.checkpoint&&a.registry==b.registry&&a.randao==b.randao&&a.fork.previous_version==b.fork.previous_version&&a.fork.current_version==b.fork.current_version&&a.fork.epoch==b.fork.epoch&&a.genesis_root==b.genesis_root,"prepared context differs from complete oracle");}
const CapellaForkBlock& record(const Fixture& f){return *f.store.blocks.at(f.target.root).lease(budget);}
void golden(Fixture& f){
 check(prepare_capella_slots(f.golden,epochs(f.rules),f.rules.transition.consensus.state,1,1,100000,budget).ssz==f.post,"golden official transition differs");CapellaTargetPreparation frame(f.store,f.rules,f.target,budget);
 check(frame.ready()&&frame.matches(f.store,f.rules,f.target,budget),"golden source not recognized");const auto& r=record(f);const auto full=prepare_capella_fork_state_contexts(r.state,r.header,f.target,epochs(f.rules),f.rules.transition.consensus.state,64,100000,budget).target;
 for(unsigned i=0;i<3;++i){check(frame.matches(f.store,f.rules,f.target,budget),"repeat source mismatch");same_context(frame.context(f.store,f.rules,budget),full);}
 passed("target-source-golden-full-oracle");
}
Fixture expanded(const Fixture& original){auto f=original;auto state=f.golden;state.ssz.insert(state.ssz.end(),64*1024,0);f.store=store_for(state,f.rules);f.target=f.store.timed.view.justified;check(record(f).state.ssz.size()>65536,"chunk-crossing state required");return f;}
void corrupted_source(const Fixture& original){
 auto f=expanded(original);auto& alias=f.store.blocks.at(f.target.root).writable();const auto* address=&alias;CapellaTargetPreparation frame(f.store,f.rules,f.target,budget);const auto saved=alias.state;
 for(size_t at:{size_t(0),size_t(65535),size_t(65536),saved.ssz.size()-1}){alias.state.ssz[at]^=1;check(f.store.blocks.at(f.target.root).lease().get()==address,"HOT prealias detached unexpectedly");const auto expected=failure([&]{capella_fork_block_context(alias.state,alias.header,epochs(f.rules),f.rules.transition.consensus.state,budget);});check(failure([&]{frame.matches(f.store,f.rules,f.target,budget);})==expected,"HOT mismatch lost full validation refusal");check(alias.state.ssz[at]==uint8_t(saved.ssz[at]^1)&&frame.ready()&&frame.target()==f.target,"refusal changed source or frame");alias.state=saved;check(frame.matches(f.store,f.rules,f.target,budget),"restored source certificate corrupted");}
 passed("target-source-hot-prealias-byte-boundaries");
}
void metadata(const Fixture& f){
 auto s=f.store;auto& b=s.blocks.at(f.target.root).writable();CapellaTargetPreparation frame(s,f.rules,f.target,budget);b.state.last_processed_epoch=0;
 check(capella_detail::target_source_fingerprint(b.state,b.header,f.rules)!=capella_detail::target_source_fingerprint(f.golden,b.header,f.rules),"absent and zero metadata collide");raises<std::invalid_argument>([&]{frame.matches(s,f.rules,f.target,budget);},"zero metadata accepted at epoch0");b.state.last_processed_epoch=1;raises<std::invalid_argument>([&]{frame.matches(s,f.rules,f.target,budget);},"other metadata accepted");b.state.last_processed_epoch.reset();check(frame.matches(s,f.rules,f.target,budget),"absent metadata retry failed");
 auto boundary=prepare_capella_slots(f.golden,epochs(f.rules),f.rules.transition.consensus.state,8,64,100000,budget);put(boundary.ssz,64,8);std::copy(f.target.root.begin(),f.target.root.end(),boundary.ssz.begin()+80);std::fill(boundary.ssz.begin()+112,boundary.ssz.begin()+144,0);check(boundary.last_processed_epoch==0,"epoch-boundary fixture metadata");auto at8=store_for(boundary,f.rules);const auto cp=at8.timed.view.justified;auto& b8=at8.blocks.at(cp.root).writable();CapellaTargetPreparation frame8(at8,f.rules,cp,budget);b8.state.last_processed_epoch.reset();raises<std::invalid_argument>([&]{frame8.matches(at8,f.rules,cp,budget);},"absent metadata accepted at epoch1");b8.state.last_processed_epoch=0;check(frame8.matches(at8,f.rules,cp,budget),"epoch1 metadata retry failed");passed("target-source-processing-metadata-presence");
}
void guards(const Fixture& f){
 CapellaTargetPreparation frame(f.store,f.rules,f.target,budget);std::vector<CapellaForkBlockRules> variations;auto add=[&](auto change){auto r=f.rules;change(r);variations.push_back(r);};
 add([](auto& r){++r.transition.chain_id;});add([](auto& r){r.transition.replay_policy=ReplayPolicy::AllowLegacyUnprotected;});add([](auto& r){++r.transition.consensus.epoch.inactivity.score_bias;});add([](auto& r){++r.transition.withdrawals.maximum_per_payload;});add([](auto& r){r.native_deposit_votes=true;r.deposit_contract_code_hash=Bytes(32,1);});add([](auto& r){r.deposit_contract_code_hash=Bytes(32,1);});add([](auto& r){++r.transition.maximum_state_bytes;});
 for(const auto& r:variations)check(!frame.matches(f.store,r,f.target,budget),"changed complete profile reused certificate");
 auto invalid=f.rules;invalid.native_deposit_votes=true;invalid.deposit_contract_code_hash.clear();const auto invalid_oracle=failure([&]{capella_fork_profile(invalid);});check(invalid_oracle.starts_with("invalid:")&&failure([&]{frame.matches(f.store,invalid,f.target,budget);})==invalid_oracle,"invalid native-deposit profile refusal differs from complete profile oracle");
 for(unsigned field=0;field<5;++field){auto s=f.store;auto& h=s.blocks.at(f.target.root).writable().header;if(field==0)++h.slot;else if(field==1)++h.proposer_index;else if(field==2)h.parent_root[0]^=1;else if(field==3)h.state_root[0]^=1;else h.body_root[0]^=1;check(!frame.matches(s,f.rules,f.target,budget),"changed header reused certificate");}
 auto target=f.target;++target.epoch;check(!frame.matches(f.store,f.rules,target,budget),"different target epoch reused frame");target=f.target;target.root[0]^=1;check(!frame.matches(f.store,f.rules,target,budget),"different target root reused frame");auto missing=f.store;missing.blocks.clear();check(!frame.matches(missing,f.rules,f.target,budget),"missing parent reused frame");raises<std::invalid_argument>([&]{CapellaTargetPreparation bad(f.store,f.rules,{1,f.target.root},budget);},"future target constructed");passed("target-source-profile-header-parent-target-guards");
}
void independent(const Fixture& f){
 CapellaTargetPreparation frame(f.store,f.rules,f.target,budget);auto copy=f.store;const auto before=f.store.blocks.at(f.target.root).lease();copy.blocks.at(f.target.root)=CapellaForkBlock(*before);check(copy.blocks.at(f.target.root).lease().get()!=before.get()&&frame.matches(copy,f.rules,f.target,budget),"independent same bytes refused");same_context(frame.context(copy,f.rules,budget),frame.context(f.store,f.rules,budget));
 auto cow=f.store;auto& b=cow.blocks.at(f.target.root).writable();check(&b!=before.get()&&frame.matches(cow,f.rules,f.target,budget),"HOT COW same bytes refused");b.state.ssz[0]^=1;raises<std::invalid_argument>([&]{frame.matches(cow,f.rules,f.target,budget);},"COW corruption accepted");check(frame.matches(f.store,f.rules,f.target,budget),"COW corrupted original");passed("target-source-independent-record-hot-cow");
}
void sizes(const Fixture& f){
 auto s=f.store;auto& b=s.blocks.at(f.target.root).writable();CapellaTargetPreparation frame(s,f.rules,f.target,budget);const auto saved=b.state;
 for(size_t count:{size_t(0),size_t(1),saved.ssz.size()-1}){b.state.ssz.resize(count);const auto expected=failure([&]{capella_fork_block_context(b.state,b.header,epochs(f.rules),f.rules.transition.consensus.state,budget);});check(failure([&]{frame.matches(s,f.rules,f.target,budget);})==expected,"truncation changed oracle refusal");b.state=saved;}
 b.state.ssz.resize(f.rules.transition.maximum_state_bytes+1);size_t callbacks=0;const auto expected=failure([&]{capella_fork_block_context(b.state,b.header,epochs(f.rules),f.rules.transition.consensus.state);});check(failure([&]{frame.matches(s,f.rules,f.target,[&]{budget();++callbacks;});})==expected&&callbacks<64,"maximum-state refusal scanned an unbounded source");b.state=saved;check(frame.matches(s,f.rules,f.target,budget),"size failure poisoned frame");passed("target-source-truncation-maximum-state-fallback");
}
void cancellations(const Fixture& original){
 auto f=expanded(original);auto& b=f.store.blocks.at(f.target.root).writable();CapellaTargetPreparation frame(f.store,f.rules,f.target,budget);const auto saved=b.state;size_t calls=0;check(frame.matches(f.store,f.rules,f.target,[&]{budget();++calls;}),"counted source match");
 for(size_t stop:{size_t(1),calls/2,calls}){size_t n=0;raises<Cancellation>([&]{frame.matches(f.store,f.rules,f.target,[&]{budget();if(++n==stop)throw Cancellation();});},"stream cancellation lost");check(b.state==saved&&frame.target()==f.target&&frame.ready()&&frame.matches(f.store,f.rules,f.target,budget),"cancelled source match changed state");}
 b.state.ssz.back()^=1;const auto changed=b.state;size_t fallback=0;failure([&]{frame.matches(f.store,f.rules,f.target,[&]{budget();++fallback;});});check(fallback>calls,"fallback full oracle not observed");size_t n=0;raises<Cancellation>([&]{frame.matches(f.store,f.rules,f.target,[&]{budget();if(++n==fallback)throw Cancellation();});},"full fallback cancellation lost");check(b.state==changed&&frame.target()==f.target&&frame.ready(),"fallback cancellation changed state");b.state=saved;passed("target-source-stream-and-fallback-cancellation");
}
void owned_copy(const Fixture& f){
 const auto& parent=record(f).header;
 // These callbacks change only the borrowed ORIGINAL, never the owned copy
 // currently being authenticated. They are serialized, not a concurrent race.
 auto initially_bad=f.golden;initially_bad.ssz[0]^=1;bool repaired_original=false;
 raises<std::invalid_argument>([&]{CapellaSlotPreparation rejected(initially_bad,parent,epochs(f.rules),f.rules.transition.consensus.state,[&]{budget();if(!repaired_original){initially_bad.ssz[0]=f.golden.ssz[0];repaired_original=true;}});},"constructor authenticated repaired borrowed input instead of corrupt owned copy");
 check(repaired_original&&initially_bad==f.golden,"borrowed input repair control did not occur");
 auto initially_good=f.golden;bool corrupted_original=false;
 CapellaSlotPreparation preserved(initially_good,parent,epochs(f.rules),f.rules.transition.consensus.state,[&]{budget();if(!corrupted_original){initially_good.ssz[0]^=1;corrupted_original=true;}});
 check(corrupted_original&&initially_good!=f.golden&&preserved.state()==f.golden,"constructor certified borrowed corruption instead of owned golden copy");capella_fork_block_context(preserved.state(),preserved.parent(),epochs(f.rules),f.rules.transition.consensus.state,budget);
 auto input=f.golden;CapellaSlotPreparation slots(input,parent,epochs(f.rules),f.rules.transition.consensus.state,budget);check(slots.state()==input,"constructor owned copy differs");input.ssz[0]^=1;input.last_processed_epoch=0;check(slots.state()==f.golden&&slots.parent()==parent,"original mutation changed owned state");capella_fork_block_context(slots.state(),slots.parent(),epochs(f.rules),f.rules.transition.consensus.state,budget);passed("target-source-construction-owned-copy");
}
void progression(const Fixture& original){
 auto f=original;f.store=prepare_capella_store_tick(f.store,f.store.timed.clock.genesis_time_ms+9*f.rules.slot_duration_ms,f.rules,64,100000,budget);CapellaTargetPreparation frame(f.store,f.rules,f.target,budget);const ConsensusCheckpoint future{1,f.target.root};check(frame.retarget(f.store,f.rules,future,budget)&&!frame.ready(),"monotone target did not reset readiness");raises<std::invalid_argument>([&]{frame.context(f.store,f.rules,budget);},"unfinished target gave context");raises<std::invalid_argument>([&]{frame.advance(0,100000,budget);},"zero slots accepted");raises<std::invalid_argument>([&]{frame.advance(4,0,budget);},"zero trials accepted");
 const auto source=record(f).state;raises<Cancellation>([&]{frame.advance(4,100000,[]{throw Cancellation();});},"advance cancellation ignored");check(!frame.ready()&&record(f).state==source&&frame.target()==future,"cancelled advance changed source/target");check(!frame.advance(4,100000,budget)&&!frame.ready(),"partial continuation ready early");check(frame.advance(4,100000,budget)&&frame.ready()&&frame.matches(f.store,f.rules,future,budget),"completed target lost source certificate");
 const auto& b=record(f);same_context(frame.context(f.store,f.rules,budget),prepare_capella_fork_state_contexts(b.state,b.header,future,epochs(f.rules),f.rules.transition.consensus.state,64,100000,budget).target);check(!frame.retarget(f.store,f.rules,f.target,budget)&&!frame.retarget(f.store,f.rules,{2,f.target.root},budget),"rewind/future target accepted");auto other=future;other.root[0]^=1;check(!frame.retarget(f.store,f.rules,other,budget),"other parent retarget accepted");raises<std::invalid_argument>([&]{frame.advance(0,100000,budget);},"ready zero budget accepted");passed("target-source-retarget-advance-readiness-budgets");
}
ConsensusRoot one_shot(const CapellaSlotState& state,const ConsensusBeaconHeader& h,const CapellaForkBlockRules& r){
 Bytes encoded;auto number=[&](uint64_t n){for(unsigned i=0;i<8;++i)encoded.push_back(uint8_t(n>>(8*i)));};auto frame=[&](std::string_view s){number(s.size());encoded.insert(encoded.end(),s.begin(),s.end());};frame("Nano/Capella/TargetSourceAuthentication/v1");frame(capella_fork_profile(r).dump());number(h.slot);number(h.proposer_index);for(const auto& root:{h.parent_root,h.state_root,h.body_root})encoded.insert(encoded.end(),root.begin(),root.end());encoded.push_back(state.last_processed_epoch?1:0);if(state.last_processed_epoch)number(*state.last_processed_epoch);number(state.ssz.size());encoded.insert(encoded.end(),state.ssz.begin(),state.ssz.end());const auto digest=sha256(encoded);ConsensusRoot out{};std::copy(digest.begin(),digest.end(),out.begin());return out;
}
void encoding(const Fixture& f){
 const auto& h=record(f).header;std::set<ConsensusRoot> seen;
 for(size_t size:{size_t(0),size_t(1),size_t(65535),size_t(65536),size_t(65537)}){CapellaSlotState s{Bytes(size,0x5a),std::nullopt};for(unsigned metadata=0;metadata<3;++metadata){if(metadata)s.last_processed_epoch=metadata-1;const auto digest=capella_detail::target_source_fingerprint(s,h,f.rules,budget);check(digest==one_shot(s,h,f.rules)&&seen.insert(digest).second,"stream/one-shot framing or presence collision");}}
 auto state=f.golden;const auto original=capella_detail::target_source_fingerprint(state,h,f.rules);auto r=f.rules;++r.transition.chain_id;check(capella_detail::target_source_fingerprint(state,h,r)!=original,"profile framing collision");auto header=h;header.parent_root[0]^=1;check(capella_detail::target_source_fingerprint(state,header,f.rules)!=original,"header framing collision");passed("target-source-explicit-streaming-encoding-vectors");
}
void admission(const Fixture& original){
 auto f=original;auto state=f.golden;const auto& l=f.rules.transition.consensus.state;const size_t historical=176+64*l.slots_per_historical_root,previous=historical+96+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector;const auto validators=little(state.ssz,historical+88,4),balances=little(state.ssz,historical+92,4);check((balances-validators)/121==64,"synthetic vote registry width");std::map<Bytes,Bytes> remap;
 for(size_t i=0;i<64;++i){const auto at=validators+121*i;Bytes old(state.ssz.begin()+at,state.ssz.begin()+at+48);const auto key=Key(i).public_key;remap.emplace(old,key);std::copy(key.begin(),key.end(),state.ssz.begin()+at);}
 const auto domain=registry_root(std::span<const uint8_t>(state.ssz).subspan(validators,64*121),l.validator_registry_limit);check(domain!=root_at(state.ssz,8),"synthetic vote must change registry domain");std::copy(domain.begin(),domain.end(),state.ssz.begin()+8);
 for(size_t start:std::array<size_t,2>{previous+133,previous+133+48*size_t(l.sync_committee_size+1)}){std::vector<Bytes> keys;for(size_t i=0;i<l.sync_committee_size;++i){Bytes old(state.ssz.begin()+start+48*i,state.ssz.begin()+start+48*(i+1));keys.push_back(remap.at(old));std::copy(keys.back().begin(),keys.back().end(),state.ssz.begin()+start+48*i);}const auto aggregate=aggregate_consensus_bls_keys(keys);check(bool(aggregate),"synthetic sync aggregate");std::copy(aggregate->begin(),aggregate->end(),state.ssz.begin()+start+48*l.sync_committee_size);}
 f.store=store_for(state,f.rules);f.target=f.store.timed.view.justified;CapellaTargetPreparation prepared(f.store,f.rules,f.target,budget);const auto ctx=prepared.context(f.store,f.rules,budget);const auto& d=f.rules.transition.consensus.attestation.duties;const auto duties=phase0_attestation_duties(ctx.registry,ctx.randao,d,0,budget);const auto members=partition_committee(duties.shuffled,0,duties.committees_per_slot*d.slots_per_epoch);check(!members.empty(),"vote committee missing");
 Phase0SignedAttestation vote{{0,0,f.target.root,{},f.target},std::vector<bool>(members.size()),{}};vote.aggregation_bits[0]=true;Key key(members[0]);check(key.public_key==Bytes(ctx.registry.validators.at(members[0]).public_key.begin(),ctx.registry.validators.at(members[0]).public_key.end()),"synthetic signing key bound");vote.signature=key.sign(consensus_signing_root(consensus_attestation_root(vote.data),consensus_domain({1,0,0,0},ctx.fork.current_version,ctx.genesis_root)));
 const auto oracle=prepare_capella_store_vote(f.store,vote,f.rules,64,100000,budget),fast=prepare_capella_prepared_store_vote(f.store,vote,f.rules,prepared,budget);check(oracle&&fast&&oracle->timed.view.latest_messages==fast->timed.view.latest_messages&&!fast->timed.view.latest_messages.empty(),"genuine admission differs");
 auto reject=[&](const Phase0SignedAttestation& bad,const CapellaForkStore& store){check(prepared.matches(store,f.rules,f.target,budget),"adverse vote needs a positive source match");check(!prepare_capella_store_vote(store,bad,f.rules,64,100000,budget)&&!prepare_capella_prepared_store_vote(store,bad,f.rules,prepared,budget)&&store.timed.view.latest_messages.empty(),"forged vote accepted after source hit");};
 auto bad=vote;bad.signature[0]^=1;reject(bad,f.store);bad=vote;bad.aggregation_bits.push_back(false);reject(bad,f.store);bad=vote;bad.aggregation_bits[0]=false;reject(bad,f.store);bad=vote;bad.data.index=duties.committees_per_slot;reject(bad,f.store);bad=vote;bad.data.beacon_block_root[0]^=1;reject(bad,f.store);
 // Known but wrong target ancestry must still reject even under a valid BLS signature.
 bad=vote;ConsensusRoot alternate=f.target.root;alternate[0]^=1;auto branch=f.store;branch.timed.view.nodes.emplace(alternate,Phase0ForkNode{0,{},0,0});bad.data.beacon_block_root=alternate;bad.signature=key.sign(consensus_signing_root(consensus_attestation_root(bad.data),consensus_domain({1,0,0,0},ctx.fork.current_version,ctx.genesis_root)));reject(bad,branch);
 // A separately authenticated source has no eligible validator at epoch0.
 auto ineligible=state;for(size_t i=0;i<64;++i)put(ineligible.ssz,validators+121*i+105,0);const auto closed=store_for(ineligible,f.rules);const auto cp=closed.timed.view.justified;CapellaTargetPreparation closed_frame(closed,f.rules,cp,budget);auto closed_vote=vote;closed_vote.data.target=cp;closed_vote.data.beacon_block_root=cp.root;closed_vote.signature=key.sign(consensus_signing_root(consensus_attestation_root(closed_vote.data),consensus_domain({1,0,0,0},ctx.fork.current_version,ctx.genesis_root)));check(closed_frame.matches(closed,f.rules,cp,budget),"ineligible fixture source match");check(!prepare_capella_prepared_store_vote(closed,closed_vote,f.rules,closed_frame,budget)&&!prepare_capella_store_vote(closed,closed_vote,f.rules,64,100000,budget),"empty eligible registry vote accepted");
 passed("target-source-genuine-forged-vote-real-admission");
}
void benchmark(const Fixture& original){
 auto f=expanded(original);const auto& b=record(f);const auto source_before=b.state;CapellaTargetPreparation frame(f.store,f.rules,f.target,budget);constexpr unsigned iterations=8;auto measure=[&](auto operation){const auto start=std::chrono::steady_clock::now();const auto cpu_start=cpu();for(unsigned i=0;i<iterations;++i){budget();operation();}return Json{{"iterations",iterations},{"wallNanoseconds",std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-start).count()},{"cpuMicroseconds",cpu()-cpu_start}};};
 const auto full=measure([&]{capella_fork_block_context(b.state,b.header,epochs(f.rules),f.rules.transition.consensus.state,budget);});const auto hits=measure([&]{check(frame.matches(f.store,f.rules,f.target,budget),"benchmark source mismatch");});const auto ctx=measure([&]{frame.context(f.store,f.rules,budget);});check(record(f).state==source_before&&cpu()<60000000,"benchmark altered source or CPU budget exceeded");passed("target-source-same-source-bounded-benchmark");std::cout<<Json{{"scenario","capella-target-source-authentication-summary"},{"result","PASS"},{"categories",categories},{"sourceBytes",b.state.ssz.size()},{"freshFullAuthentication",full},{"repeatedSourceMatches",hits},{"preparedContextStillFullyProjected",ctx},{"performanceImprovementRequired",false},{"naturalChainDeadlineProven",false},{"coldRecordFixtureQualified",false},{"syntheticSignatures",signatures},{"elapsedMs",elapsed()},{"cpuMicroseconds",cpu()},{"maxRssKiB",rss()},{"runtimeStarted",false},{"networkQualified",false},{"productionQualified",false}}.dump()<<std::endl;
}
}
int main(int argc,char** argv){try{check(argc==2&&std::string(argv[1])=="--self-test","expected --self-test");auto f=fixture();golden(f);corrupted_source(f);metadata(f);guards(f);independent(f);sizes(f);cancellations(f);owned_copy(f);progression(f);encoding(f);admission(f);benchmark(f);check(categories==12,"source authentication category count");return 0;}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
