// Candidate diagnostic only. No ChainStore, signer, runtime or network instance.
#include "nano/consensus_capella_store_snapshot.hpp"
#include "nano/consensus_capella_fork_state.hpp"
#include "nano/consensus_capella_withdrawals.hpp"
#include "nano/chain.hpp"
#include <openssl/sha.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <filesystem>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <vector>
#include <set>
#include <string>
#include <sys/resource.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using namespace nano;
namespace {
using Clock=std::chrono::steady_clock;
const auto started=Clock::now();
std::string stage="request";uint64_t last_slot=0,chunks=0;
struct Refusal:std::runtime_error{using std::runtime_error::runtime_error;};
void need(bool ok,const char* why){if(!ok)throw Refusal(why);}
void shape(const Json& j,std::initializer_list<const char*> names){
 need(j.is_object()&&j.size()==names.size(),"input_shape");for(const auto* n:names)need(j.contains(n),"input_field_missing");
}
std::string text(const Json& j){need(j.is_string(),"string_required");return j.get<std::string>();}
uint64_t number(const Json& j){
 const auto s=text(j);const auto n=u64(quantity(s));need(quantity(Int(std::to_string(n)))==s,"quantity_not_canonical");return n;
}
size_t size(const Json& j){const auto n=number(j);need(n<=SIZE_MAX,"size_range");return size_t(n);}
std::string sha(std::string_view b){std::array<unsigned char,32> h{};need(SHA256(reinterpret_cast<const unsigned char*>(b.data()),b.size(),h.data())!=nullptr,"sha256_failed");const auto out=hex(Bytes(h.begin(),h.end()));return out.substr(2);}
std::string hash_text(const Json& j){const auto s=text(j);need(s.size()==64&&std::all_of(s.begin(),s.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');}),"sha256_format");return s;}
Bytes digest(const Json& j){return unhex("0x"+hash_text(j));}
ConsensusRoot root(const Json& j){const auto s=text(j);need(s.size()==66&&s.starts_with("0x"),"root_format");const auto b=unhex(s);need(b.size()==32&&hex(b)==s,"root_not_canonical");ConsensusRoot r{};std::copy(b.begin(),b.end(),r.begin());return r;}
std::string root_text(const ConsensusRoot& r){return hex(Bytes(r.begin(),r.end()));}
Json cp(const ConsensusCheckpoint& c){return {{"epoch",quantity(Int(std::to_string(c.epoch)))},{"root",root_text(c.root)}};}
ConsensusCheckpoint checkpoint_value(const Json& j){shape(j,{"epoch","root"});return {number(j.at("epoch")),root(j.at("root"))};}
ForkCheckpointToken token(const Json& j){shape(j,{"sequence","digest"});auto t=ForkCheckpointToken{number(j.at("sequence")),Bytes{}};const auto r=root(j.at("digest"));t.digest.assign(r.begin(),r.end());need(t.sequence>0,"zero_commitment_sequence");return t;}
uint64_t elapsed(){const auto d=std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-started).count();need(d>=0,"steady_regressed");return uint64_t(d);}
uint64_t rss(){rusage u{};need(getrusage(RUSAGE_SELF,&u)==0&&u.ru_maxrss>=0,"rss_observation_failed");
#ifdef __APPLE__
 return (uint64_t(u.ru_maxrss)+1023)/1024;
#else
 return uint64_t(u.ru_maxrss);
#endif
}
struct Budget{
 uint64_t elapsed_ms,rss_kib,calls=0;
 void check(){need(elapsed()<elapsed_ms,"steady_deadline_expired");if((++calls&255)==0)need(rss()<=rss_kib,"rss_budget_exceeded");}
 void fence(){check();need(rss()<=rss_kib,"rss_budget_exceeded");}
};
bool same(const struct stat& a,const struct stat& b){
#ifdef __APPLE__
 const bool times=a.st_mtimespec.tv_sec==b.st_mtimespec.tv_sec&&a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec&&a.st_ctimespec.tv_sec==b.st_ctimespec.tv_sec&&a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
 const bool times=a.st_mtim.tv_sec==b.st_mtim.tv_sec&&a.st_mtim.tv_nsec==b.st_mtim.tv_nsec&&a.st_ctim.tv_sec==b.st_ctim.tv_sec&&a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
 return times&&a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_uid==b.st_uid&&a.st_gid==b.st_gid&&a.st_mode==b.st_mode&&a.st_nlink==b.st_nlink&&a.st_size==b.st_size;
}
class Input{
 int fd_=-1;std::filesystem::path path_;struct stat initial_{};std::string hash_;size_t bytes_;
 std::string read(const std::function<void()>& check)const{
  std::string b;std::array<char,65536> chunk{};
  while(b.size()<bytes_){if(check)check();const auto want=std::min(chunk.size(),bytes_-b.size());const auto n=pread(fd_,chunk.data(),want,off_t(b.size()));if(n<0&&errno==EINTR)continue;need(n>0,"input_read_failed");b.append(chunk.data(),size_t(n));}
  struct stat after{},named{};need(fstat(fd_,&after)==0&&lstat(path_.c_str(),&named)==0&&same(initial_,after)&&same(after,named)&&std::filesystem::canonical(path_)==path_,"input_identity_changed");need(sha(b)==hash_,"input_sha256_mismatch");if(check)check();return b;
 }
public:
 std::string bytes;
 Input(std::string path,std::string hash,size_t expected,size_t maximum,const std::function<void()>& check):path_(std::move(path)),hash_(std::move(hash)),bytes_(expected){
  need(bytes_&&bytes_<=maximum&&bytes_<=size_t(std::numeric_limits<off_t>::max()),"input_bytes_budget");need(path_.is_absolute()&&std::filesystem::canonical(path_)==path_,"input_path_not_canonical");fd_=open(path_.c_str(),O_RDONLY|O_NOFOLLOW|O_CLOEXEC|O_NONBLOCK);need(fd_>=0,"input_open_failed");
  try{need(fstat(fd_,&initial_)==0&&S_ISREG(initial_.st_mode)&&initial_.st_nlink==1&&(initial_.st_mode&0022)==0&&initial_.st_size>=0&&uint64_t(initial_.st_size)==bytes_,"input_file_metadata");bytes=read(check);}catch(...){close(fd_);fd_=-1;throw;}
 }
 Input(const Input&)=delete;Input& operator=(const Input&)=delete;~Input(){if(fd_>=0)close(fd_);}
 void readback(const std::function<void()>& check)const{need(read(check)==bytes,"input_bytes_changed");}
};
Json parse(const std::string& b){
 std::vector<std::set<std::string>> object_keys;
 try{return Json::parse(b,[&](int,Json::parse_event_t event,Json& value){
  if(event==Json::parse_event_t::object_start)object_keys.emplace_back();
  else if(event==Json::parse_event_t::key){need(!object_keys.empty()&&object_keys.back().insert(value.get<std::string>()).second,"duplicate_json_key");}
  else if(event==Json::parse_event_t::object_end){need(!object_keys.empty(),"json_object_stack");object_keys.pop_back();}
  return true;
 });}catch(const Json::exception&){throw Refusal("invalid_json");}
}
void artifact(const Json& j){shape(j,{"path","sha256","bytes"});text(j.at("path"));hash_text(j.at("sha256"));need(number(j.at("bytes"))>0,"empty_artifact");}
uint64_t word(const Bytes& b,size_t at,size_t width=8){need(at<=b.size()&&width<=b.size()-at,"ssz_field_bounds");uint64_t n=0;for(size_t i=0;i<width;++i)n|=uint64_t(b[at+i])<<(8*i);return n;}
size_t add(size_t a,size_t b){need(b<=SIZE_MAX-a,"layout_overflow");return a+b;}
size_t mul(size_t a,size_t b){need(!a||b<=SIZE_MAX/a,"layout_overflow");return a*b;}
struct Observation{ConsensusSlashingState registry;Json stats,committees;ConsensusRoot state_root;};
Observation observe(const CapellaSlotState& state,const CapellaForkBlockRules& r,const ConsensusSlashingState* original,const std::function<void()>& check){
 const auto& t=r.transition;const auto& l=t.consensus.state;const auto state_root=capella_beacon_state_root(state.ssz,l,t.execution,t.maximum_state_bytes,check);
 auto registry=read_capella_withdrawal_state(state.ssz,state_root,l,t.execution,t.maximum_state_bytes,t.consensus.epoch.ffg.slots_per_epoch,check).registry;
 uint64_t active=0,slashed=0,exited=0,lost=0,new_exits=0,zero=0,min_balance=UINT64_MAX,max_balance=0,min_score=UINT64_MAX,max_score=0;Int balance_total=0,effective_active=0;
 const auto historical=add(176,mul(64,l.slots_per_historical_root));const auto previous=add(add(add(historical,96),mul(32,l.epochs_per_historical_vector)),mul(8,l.epochs_per_slashings_vector));
 const auto score_at=word(state.ssz,add(previous,129),4);const auto execution_field=add(add(previous,229),mul(96,l.sync_committee_size));need(score_at<=state.ssz.size()&&mul(registry.validators.size(),8)<=state.ssz.size()-score_at&&score_at+mul(registry.validators.size(),8)==word(state.ssz,execution_field,4),"inactivity_cardinality");
 std::map<Bytes,const ConsensusSlashingValidator*> by_key;
 for(const auto& [i,v]:registry.validators){if(check)check();const bool a=v.activation_epoch<=registry.epoch&&registry.epoch<v.exit_epoch;active+=a;slashed+=v.slashed;exited+=v.exit_epoch<=registry.epoch;zero+=v.balance==0;min_balance=std::min(min_balance,v.balance);max_balance=std::max(max_balance,v.balance);balance_total+=Int(std::to_string(v.balance));if(a)effective_active+=Int(std::to_string(v.effective_balance));
  need(i<registry.validators.size(),"registry_index_bounds");const auto score=word(state.ssz,score_at+size_t(i)*8);min_score=std::min(min_score,score);max_score=std::max(max_score,score);need(by_key.emplace(Bytes(v.public_key.begin(),v.public_key.end()),&v).second,"duplicate_validator_public_key");
  if(original){const auto found=original->validators.find(i);need(found!=original->validators.end()&&found->second.public_key==v.public_key,"projected_registry_identity_changed");const auto& old=found->second;lost+=(old.activation_epoch<=original->epoch&&original->epoch<old.exit_epoch)&&!a;new_exits+=old.exit_epoch==UINT64_MAX&&v.exit_epoch!=UINT64_MAX;}
 }
 const auto current=add(previous,133),width=mul(48,add(l.sync_committee_size,1));Json committees=Json::object();
 for(size_t which=0;which<2;++which){std::set<Bytes> distinct;uint64_t known=0,active_positions=0,slashed_positions=0;const auto at=add(current,mul(which,width));need(at<=state.ssz.size()&&width<=state.ssz.size()-at,"sync_committee_bounds");for(size_t i=0;i<l.sync_committee_size;++i){if(check)check();const auto begin=at+48*i;Bytes key(state.ssz.begin()+begin,state.ssz.begin()+begin+48);distinct.insert(key);const auto v=by_key.find(key);if(v!=by_key.end()){++known;active_positions+=v->second->activation_epoch<=registry.epoch&&registry.epoch<v->second->exit_epoch;slashed_positions+=v->second->slashed;}}
  committees[which?"next":"current"]={{"positions",l.sync_committee_size},{"distinctPublicKeys",distinct.size()},{"knownRegistryPositions",known},{"activeAtObservedEpochPositions",active_positions},{"slashedPositions",slashed_positions}};
 }
 Json stats{{"epoch",registry.epoch},{"validators",registry.validators.size()},{"active",active},{"exited",exited},{"slashed",slashed},{"formerlyActiveNowInactive",lost},{"newlyScheduledExits",new_exits},{"zeroBalances",zero},{"balanceMinimumGwei",registry.validators.empty()?Json(nullptr):Json(min_balance)},{"balanceMaximumGwei",max_balance},{"balanceTotalGwei",balance_total.get_str()},{"activeEffectiveBalanceGwei",effective_active.get_str()},{"inactivityScoreMinimum",registry.validators.empty()?Json(nullptr):Json(min_score)},{"inactivityScoreMaximum",max_score}};
 return {std::move(registry),std::move(stats),std::move(committees),state_root};
}
void retained_checkpoints(const CapellaForkStore& s,const ConsensusRoot& head,const std::function<void()>& check){
 auto id=head;bool justified=false,finalized=false;size_t visited=0;
 for(;;){if(check)check();need(++visited<=s.blocks.size(),"retained_ancestry_bound");const auto entry=s.blocks.find(id);const auto node=s.timed.view.nodes.find(id);need(entry!=s.blocks.end()&&node!=s.timed.view.nodes.end(),"retained_ancestor_missing");const auto& h=entry->second.header();need(consensus_beacon_header_root(h)==id&&node->second.slot==h.slot&&node->second.parent==h.parent_root,"retained_ancestor_binding");justified|=id==s.timed.view.justified.root;finalized|=id==s.timed.view.finalized.root;if(id==s.timed.view.anchor)break;const auto parent=s.blocks.find(h.parent_root);need(parent!=s.blocks.end()&&parent->second.header().slot<h.slot,"retained_ancestor_order");id=h.parent_root;}
 need(justified&&finalized,"store_checkpoint_not_on_retained_head_path");
}
uint64_t wall_now(){const auto n=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();need(n>=0,"negative_wall_clock");return uint64_t(n);}
Json safety(){return {{"resumeAuthorized",false},{"clockPublished",false},{"dbOpened",false},{"journalsOpened",false},{"keysLoaded",false},{"newSignatures",0},{"runtimeStarted",false},{"networkStarted",false},{"dutyReadinessQualified",false},{"productionQualified",false}};}
}
int main(int argc,char** argv){try{
 need(argc==4&&std::string(argv[1])=="--request","usage_request_path_external_sha_required");const auto expected_request=hash_text(Json(argv[3]));const std::filesystem::path request_path(argv[2]);need(request_path.is_absolute()&&std::filesystem::is_regular_file(request_path),"request_file_required");const auto request_size=std::filesystem::file_size(request_path);need(request_size&&request_size<=65536,"request_64k_bound");Input request(argv[2],expected_request,size_t(request_size),65536,{});const auto j=parse(request.bytes);
 shape(j,{"format","artifacts","expected","snapshotLimits","projectionLimits","ordinaryClockStepMs"});need(j.at("format")=="nano-capella-current-utc-projection-request-v1","request_format");const auto& p=j.at("projectionLimits");shape(p,{"maximumHeadGapSlots","maximumChunkSlots","maximumChunks","maximumElapsedMs","maximumRssKiB","maximumInputBytes","maximumObservationAgeMs"});const auto max_gap=number(p.at("maximumHeadGapSlots")),chunk=number(p.at("maximumChunkSlots")),max_chunks=number(p.at("maximumChunks")),max_age=number(p.at("maximumObservationAgeMs"));const auto max_input=size(p.at("maximumInputBytes"));Budget budget{number(p.at("maximumElapsedMs")),number(p.at("maximumRssKiB"))};need(max_gap&&chunk&&max_chunks&&max_input&&max_age&&budget.elapsed_ms&&budget.rss_kib,"positive_projection_bounds_required");need(budget.elapsed_ms<=uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(Clock::duration::max()/2).count())&&budget.rss_kib<=SIZE_MAX/1024,"projection_budget_range");const std::function<void()> check=[&]{budget.check();};budget.fence();
 const auto& a=j.at("artifacts");shape(a,{"snapshot","profile","genesis","extraction"});std::set<std::string> paths;size_t input_bytes=0;for(const char* n:{"snapshot","profile","genesis","extraction"}){artifact(a.at(n));const auto length=size(a.at(n).at("bytes"));need(input_bytes<=max_input&&length<=max_input-input_bytes,"aggregate_input_budget");input_bytes+=length;need(paths.insert(text(a.at(n).at("path"))).second,"duplicate_input_path");}
 auto input=[&](const char* n){const auto& row=a.at(n);return Input(text(row.at("path")),hash_text(row.at("sha256")),size(row.at("bytes")),max_input,check);};stage="public-input-authentication";auto snapshot=input("snapshot"),profile=input("profile"),genesis_file=input("genesis"),extraction=input("extraction");
 const auto binding=parse(extraction.bytes);shape(binding,{"format","artifacts","binding"});need(binding.at("format")=="nano-capella-current-projection-extraction-v1","external_extraction_binding_required");shape(binding.at("artifacts"),{"snapshot","profile","genesis"});for(const char* n:{"snapshot","profile","genesis"}){shape(binding.at("artifacts").at(n),{"sha256","bytes"});need(binding.at("artifacts").at(n).at("sha256")==a.at(n).at("sha256")&&binding.at("artifacts").at(n).at("bytes")==a.at(n).at("bytes"),"extraction_artifact_binding_mismatch");}
 const auto& expected=j.at("expected");shape(expected,{"primary","witness","chainId","genesisHash","genesisValidatorsRoot","networkAnchor","storageAnchor","headRoot","headStateRoot","executionBlockHash","clockGenesisTimeMs","clockPublishedTimeMs","storeCheckpoints"});need(binding.at("binding")==expected,"external_current_binding_mismatch");const auto primary=token(expected.at("primary")),witness=token(expected.at("witness"));need(primary.digest!=witness.digest,"independent_primary_witness_required");
 const auto& l=j.at("snapshotLimits");shape(l,{"envelopeBytes","blocks","messages","equivocations","stateBytes","totalStateBytes","replaySlots","syncTrials"});const CapellaStoreSnapshotLimits limits{size(l.at("envelopeBytes")),size(l.at("blocks")),size(l.at("messages")),size(l.at("equivocations")),size(l.at("stateBytes")),size(l.at("totalStateBytes")),number(l.at("replaySlots")),number(l.at("syncTrials"))};need(limits.envelope_bytes&&limits.blocks&&limits.messages&&limits.equivocations&&limits.state_bytes&&limits.total_state_bytes&&limits.replay_slots&&limits.sync_trials&&chunk<=limits.replay_slots,"snapshot_or_chunk_limits");
 stage="domain-authentication";const auto genesis=Genesis::from_json(parse(genesis_file.bytes));const auto rules=load_capella_fork_profile(profile.bytes,digest(a.at("profile").at("sha256")),max_input);const auto& t=rules.transition;const auto slots=t.consensus.epoch.ffg.slots_per_epoch;const auto expected_genesis=root(expected.at("genesisHash"));const auto expected_chain_id=quantity(text(expected.at("chainId")));need(quantity(expected_chain_id)==text(expected.at("chainId")),"chain_id_not_canonical");need(genesis.consensus=="beacon"&&genesis.evm_revision==EvmRevision::Shanghai&&genesis.chain_id==expected_chain_id&&t.chain_id==genesis.chain_id&&t.replay_policy==ReplayPolicy::RequireChainId&&genesis.block.header.hash()==Bytes(expected_genesis.begin(),expected_genesis.end()),"genesis_or_profile_domain");need(slots&&rules.slot_duration_ms&&limits.state_bytes>=t.maximum_state_bytes&&limits.total_state_bytes>=limits.state_bytes,"state_capacity_or_slot_profile");
 stage="current-snapshot-authentication";const auto store=capella_store_from_snapshot(snapshot.bytes,digest(a.at("snapshot").at("sha256")),rules,limits,check);const auto head=capella_fork_head(store,rules,check);const auto head_root=consensus_beacon_header_root(head->header);const CapellaEpochRules epochs{t.consensus.epoch,t.execution,t.maximum_state_bytes};const auto head_context=capella_fork_block_context(head->state,head->header,epochs,t.consensus.state,check);validate_capella_execution_record(*head,rules,check);const auto network_anchor=store.network_anchor.value_or(store.timed.view.anchor);
 need(network_anchor==root(expected.at("networkAnchor"))&&store.timed.view.anchor==root(expected.at("storageAnchor"))&&head_root==root(expected.at("headRoot"))&&head->header.state_root==root(expected.at("headStateRoot"))&&head_context.genesis_root==root(expected.at("genesisValidatorsRoot"))&&hex(head->execution_block.header.hash())==text(expected.at("executionBlockHash")),"current_head_domain_binding");need(store.timed.clock.genesis_time_ms==number(expected.at("clockGenesisTimeMs"))&&store.timed.clock.time_ms==number(expected.at("clockPublishedTimeMs")),"published_clock_binding");const auto& cps=expected.at("storeCheckpoints");shape(cps,{"justified","finalized","unrealizedJustified","unrealizedFinalized"});need(store.timed.view.justified==checkpoint_value(cps.at("justified"))&&store.timed.view.finalized==checkpoint_value(cps.at("finalized"))&&store.timed.clock.unrealized_justified==checkpoint_value(cps.at("unrealizedJustified"))&&store.timed.clock.unrealized_finalized==checkpoint_value(cps.at("unrealizedFinalized")),"store_checkpoint_binding");retained_checkpoints(store,head_root,check);
 stage="current-utc-observation";const auto wall=wall_now(),from=store.timed.clock.time_ms,genesis_ms=store.timed.clock.genesis_time_ms;need(wall>=from&&wall>=genesis_ms,"wall_before_published_or_genesis");const auto target=(wall-genesis_ms)/rules.slot_duration_ms;need(target>=head->header.slot&&target!=UINT64_MAX&&target-head->header.slot<max_gap,"head_gap_or_next_slot_budget");const auto step=number(j.at("ordinaryClockStepMs"));need(step&&step/rules.slot_duration_ms+uint64_t(step%rules.slot_duration_ms!=0)<=limits.replay_slots,"ordinary_step_replay_capacity");const auto gap=wall-from,minimum_steps=gap/step+uint64_t(gap%step!=0);auto initial=observe(head->state,rules,nullptr,check);need(initial.stats.at("active").get<uint64_t>()>0,"no_active_source_participants");CapellaSlotPreparation frame(head->state,head->header,epochs,t.consensus.state,check);last_slot=frame.slot();
 auto advance=[&](uint64_t to){while(frame.slot()<to){budget.fence();need(chunks<max_chunks,"chunk_count_exceeded");const auto start=frame.slot();auto next=start+std::min(chunk,to-start);if(next<to&&next/slots*slots>start)next=next/slots*slots;frame.advance(next,chunk,limits.sync_trials,check);last_slot=frame.slot();++chunks;}};
 const auto source_checkpoints=capella_fork_checkpoints(head->state,epochs,t.consensus.state,check);
 stage="empty-slot-projection";advance(target);auto projected=observe(frame.state(),rules,&initial.registry,check);need(projected.stats.at("active").get<uint64_t>()>0,"no_active_projected_participants");const auto proposer=frame.proposer_identity(t.consensus.attestation.duties,check);need(proposer.has_value(),"no_current_slot_proposer");const auto checkpoints=capella_fork_checkpoints(frame.state(),epochs,t.consensus.state,check);
 stage="next-slot-projection";advance(target+1);auto next=observe(frame.state(),rules,&initial.registry,check);need(next.stats.at("active").get<uint64_t>()>0,"no_active_next_slot_participants");stage="public-input-readback";for(const auto* f:{&request,&snapshot,&profile,&genesis_file,&extraction})f->readback(check);budget.fence();const auto finished=wall_now();need(finished>=wall&&finished-wall<=max_age,"projection_observation_stale");
 auto result=safety();result.update(Json{{"event","current-utc-projection-terminal"},{"result","PROJECTED_READONLY_ONLY"},{"externalExtractionDeclarationAccepted",true},{"extractionIndependentlyVerifiedByThisTool",false},{"sourceHeadStateRootVerified",true},{"headExecutionBindingVerified",true},{"retainedCheckpointPathVerified",true},{"projectedStateRoot",root_text(projected.state_root)},{"nextSlotStateRoot",root_text(next.state_root)},{"asOfUtcMs",wall},{"finishedUtcMs",finished},{"publishedClockMs",from},{"clockGapMs",gap},{"headSlot",head->header.slot},{"projectedSlot",target},{"nextSlotProjected",target+1},{"chunks",chunks},{"elapsedMs",elapsed()},{"maxRssKiB",rss()},{"minimumClockStepCapacity",minimum_steps},{"twoHourSixtyStepCapacityForDeclaredStep",gap<7200000&&minimum_steps<=60},{"sourceStoreCheckpoints",cps},{"sourceHeadSszCheckpoints",{{"justified",cp(source_checkpoints.realized.current_justified)},{"finalized",cp(source_checkpoints.realized.finalized)}}},{"projectedSszCheckpoints",{{"justified",cp(checkpoints.realized.current_justified)},{"finalized",cp(checkpoints.realized.finalized)}}},{"initialRegistry",initial.stats},{"projectedRegistry",projected.stats},{"nextSlotRegistry",next.stats},{"projectedSyncCommittees",projected.committees},{"nextSlotSyncCommittees",next.committees},{"selectedProposerIndex",proposer->index},{"sourceInputsReadbackUnchanged",true},{"ordinaryClockRulesChanged",false},{"startupAllowanceActivated",false}});std::cout<<result.dump()<<std::endl;return 0;
 }catch(const Refusal& e){auto j=safety();j.update(Json{{"event","current-utc-projection-terminal"},{"result","REFUSED"},{"reason",e.what()},{"stage",stage},{"lastFullyPreparedSlot",last_slot},{"chunks",chunks},{"elapsedMs",elapsed()}});std::cout<<j.dump()<<std::endl;return 1;
 }catch(const EmptyAltairSyncActiveSet&){auto j=safety();j.update(Json{{"event","current-utc-projection-terminal"},{"result","REFUSED"},{"reason","empty_future_sync_active_set"},{"stage",stage},{"lastFullyPreparedSlot",last_slot},{"chunks",chunks},{"elapsedMs",elapsed()}});std::cout<<j.dump()<<std::endl;return 1;
 }catch(const std::bad_alloc&){auto j=safety();j.update(Json{{"event","current-utc-projection-terminal"},{"result","REFUSED"},{"reason","allocation_exhausted"},{"stage",stage}});std::cout<<j.dump()<<std::endl;return 1;
 }catch(const std::exception&){auto j=safety();j.update(Json{{"event","current-utc-projection-terminal"},{"result","REFUSED"},{"reason","core_or_input_validation_refused"},{"stage",stage},{"lastFullyPreparedSlot",last_slot},{"chunks",chunks},{"elapsedMs",elapsed()}});std::cout<<j.dump()<<std::endl;return 1;}}
