// Private synthetic V37 diagnostic only. Every source store is mounted and opened read-only.
// Pure empty-slot projection; no clock publication, signing journal, duty loop, RPC or P2P.
#include "nano/consensus_capella_reactor.hpp"
#include "nano/consensus_capella_fork_state.hpp"
#include "nano/chain.hpp"
#include "consensus_capella_projection.hpp"
#include <fstream>
#include <iostream>
#include <chrono>
#include <sys/resource.h>
#include <algorithm>
#include <set>
using namespace nano;
namespace {
constexpr uint64_t downtime_limit_ms=604800000,elapsed_limit_ms=600000,rss_limit_kib=1572864;
using Clock=std::chrono::steady_clock;
auto began=Clock::now();uint64_t checks=0,last_slot=0;std::string stage="input";
void need(bool x,const char* message){if(!x)throw std::runtime_error(message);}
uint64_t elapsed(){return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-began).count();}
uint64_t rss(){rusage u{};need(getrusage(RUSAGE_SELF,&u)==0,"resource observation failed");return uint64_t(u.ru_maxrss);}
void check(){++checks;if(elapsed()>elapsed_limit_ms)throw std::runtime_error("bounded projection wall budget exceeded");if((checks&1023)==0&&rss()>rss_limit_kib)throw std::runtime_error("bounded projection RSS budget exceeded");}
std::string read(const std::string& p,size_t maximum){need(std::filesystem::is_regular_file(p)&&std::filesystem::file_size(p)<=maximum,"input file bounds");std::ifstream f(p,std::ios::binary);std::string s((std::istreambuf_iterator<char>(f)),{});need(!f.bad()&&s.size()<=maximum,"input read bounds");return s;}
Json json(const std::string& p){return Json::parse(read(p,1024*1024));}
uint64_t number(const Json& j){return u64(quantity(j));}
ForkCheckpointToken token(const Json& j){need(j.is_object()&&j.size()==2,"token shape");auto t=ForkCheckpointToken{number(j.at("sequence")),unhex(j.at("digest"))};need(t.sequence&&t.digest.size()==32,"token bounds");return t;}
Json wire(const ForkCheckpointToken& t){return {{"sequence",quantity(Int(std::to_string(t.sequence)))},{"digest",hex(t.digest)}};}
uint64_t word(const Bytes& b,size_t at){need(at<=b.size()&&8<=b.size()-at,"projected field bounds");uint64_t n=0;for(size_t i=0;i<8;++i)n|=uint64_t(b[at+i])<<(8*i);return n;}
Json stats(const ConsensusSlashingState& registry,const ConsensusSlashingState& initial,const CapellaSlotState& state,const AltairStateLimits& limits){
 need(registry.validators.size()==65&&initial.validators.size()==65,"synthetic admitted registry count");
 const auto offsets=capella_detail::offsets(limits);const auto at=capella_detail::offset(state.ssz,offsets[6]);
 std::array<uint64_t,3> active{},exited{},slashed{},new_exits{};uint64_t min_balance=UINT64_MAX,max_balance=0,min_score=UINT64_MAX,max_score=0,zero=0;
 std::array<Int,3> totals{};
 for(const auto& [index,v]:registry.validators){check();need(index<=64,"synthetic registry identity bounds");const size_t owner=index<16?0:index<32?1:2;
  if(v.activation_epoch<=registry.epoch&&registry.epoch<v.exit_epoch)++active[owner];else if(v.exit_epoch<=registry.epoch)++exited[owner];
  if(v.slashed)++slashed[owner];if(initial.validators.at(index).exit_epoch==UINT64_MAX&&v.exit_epoch!=UINT64_MAX)++new_exits[owner];
  min_balance=std::min(min_balance,v.balance);max_balance=std::max(max_balance,v.balance);totals[owner]+=Int(std::to_string(v.balance));if(!v.balance)++zero;
  const auto score=word(state.ssz,at+8*index);min_score=std::min(min_score,score);max_score=std::max(max_score,score);
 }
 return {{"epoch",registry.epoch},{"validators",registry.validators.size()},{"activeByOwner",active},{"exitedByOwner",exited},{"slashedByOwner",slashed},{"newlyScheduledExitsByOwner",new_exits},{"balanceMinimumGwei",min_balance},{"balanceMaximumGwei",max_balance},{"balanceTotalsGweiByOwner",Json::array({totals[0].get_str(),totals[1].get_str(),totals[2].get_str()})},{"zeroBalances",zero},{"inactivityScoreMinimum",min_score},{"inactivityScoreMaximum",max_score}};
}
}
int main(int argc,char** argv){try{
 need(argc==2&&std::string(argv[1])=="--project-preserved-synthetic-v37","explicit readonly projection mode required");
 const auto genesis=Genesis::from_json(json("/source/config-origin/genesis.json"));need(genesis.chain_id==1337&&genesis.consensus=="beacon"&&genesis.evm_revision==EvmRevision::Shanghai,"private Beacon Shanghai fixture only");
 const auto capsule=json("/source/private-audit/capture.json"),config=json("/source/private-audit/config.json").at("capella"),l=config.at("limits");
 need(capsule.at("readOnlyStores")==true&&capsule.at("primaryWitnessAndSignersVerified")==true&&capsule.at("minimumSequenceBoundsVerified")==true&&capsule.at("canonicalReplay").at("canonicalHistoryVerified")==true&&capsule.at("canonicalReplay").at("externalFinalizedAncestorVerified")==true,"source audited capsule binding");
 need(capsule.at("genesisHash")==hex(genesis.block.header.hash())&&capsule.at("profileSha256")==config.at("profile").at("sha256"),"source domain binding");
 const CapellaStoreSnapshotLimits limits{size_t(number(l.at("envelopeBytes"))),size_t(number(l.at("blocks"))),size_t(number(l.at("messages"))),size_t(number(l.at("equivocations"))),size_t(number(l.at("stateBytes"))),size_t(number(l.at("totalStateBytes"))),number(l.at("replaySlots")),number(l.at("syncTrials"))};
 need(limits.replay_slots==1024&&limits.blocks==4096&&limits.envelope_bytes==134217728&&limits.state_bytes==4194304&&limits.total_state_bytes==67108864&&limits.sync_trials==100000,"unchanged V37 projection resource limits");
 const auto profile=read("/source/config-origin/profile.json",1024*1024);
 const auto rules=load_capella_fork_profile(profile,unhex(config.at("profile").at("sha256")),1024*1024);const auto& t=rules.transition;
 need(rules.native_deposit_votes&&t.consensus.epoch.ffg.slots_per_epoch==8&&rules.slot_duration_ms==6000,"unchanged synthetic profile");
 const auto ordinary_step=number(config.at("clock").at("maximumForwardStepMs"));need(ordinary_step==6144000&&ordinary_step/rules.slot_duration_ms==limits.replay_slots,"unchanged ordinary clock step and replay bound");
 const CapellaEpochRules epoch_rules{t.consensus.epoch,t.execution,t.maximum_state_bytes};
 const auto request=json("/source/private-audit/request.json");
 need(request.at("profile").at("sha256")==config.at("profile").at("sha256")&&request.at("limits")==l,"source external profile and limits binding");
 stage="initial-seed-authentication";
 const auto seed=capella_store_from_snapshot(read("/source/config-origin/initial.json",limits.envelope_bytes),unhex(request.at("anchor").at("sha256")),rules,limits,check);
 const auto seed_pin=capella_fork_head(seed,rules,check);const auto seed_context=capella_fork_block_context(seed_pin->state,seed_pin->header,epoch_rules,t.consensus.state,check);
 need(seed_context.registry.validators.size()==64&&seed_pin->state.ssz.size()>=40,"original synthetic seed domain bounds");
 const Bytes seed_validators_root(seed_pin->state.ssz.begin()+8,seed_pin->state.ssz.begin()+40);
 std::cout<<Json{{"event","seed-domain-authenticated"},{"genesisValidatorsRoot",hex(seed_validators_root)},{"slotsPerEpoch",8},{"initialSha256",request.at("anchor").at("sha256")},{"profileSha256",config.at("profile").at("sha256")},{"genesisHash",hex(genesis.block.header.hash())},{"elapsedMs",elapsed()}}.dump()<<std::endl;
 ChainStore disk("/source/recovered-data/chain",ChainStore::OpenMode::ReadOnly);disk.bind_chain(genesis.chain_id,genesis.block.header.hash(),"beacon");
 const auto primary=disk.fork_checkpoint(genesis.chain_id,genesis.block.header.hash(),limits.envelope_bytes,token(capsule.at("restart")));need(bool(primary),"audited exact primary checkpoint missing");
 ChainStore witness("/source/recovered-data/witness",ChainStore::OpenMode::ReadOnly);witness.bind_chain(genesis.chain_id,genesis.block.header.hash(),"nano-capella-recovery-v1");
 const auto saved=witness.fork_checkpoint(genesis.chain_id,genesis.block.header.hash(),2048+8*limits.envelope_bytes);need(bool(saved),"independent core witness missing");const auto intent=Json::parse(saved->snapshot);need(token(intent.at("next"))==primary->token&&intent.at("snapshot")==primary->snapshot,"exact primary/witness boundary mismatch");
 stage="authenticated-readonly-restore";
 CapellaDurableReactor reactor(disk,genesis.chain_id,genesis.block.header.hash(),rules,limits,primary->token,check);
 const auto publication=reactor.current();const auto head_pin=capella_fork_head(*publication.state,rules,check);const auto& head=*head_pin;
 const auto initial=capella_fork_block_context(head.state,head.header,epoch_rules,t.consensus.state,check);
 need(head.state.ssz.size()>=40,"validators domain state bounds");const Bytes validators_root(head.state.ssz.begin()+8,head.state.ssz.begin()+40);need(validators_root==seed_validators_root,"persisted head validators domain differs from authenticated seed");
 std::cout<<Json{{"event","source-authenticated"},{"checkpoint",wire(primary->token)},{"genesisValidatorsRoot",hex(validators_root)},{"slotsPerEpoch",8},{"profileSha256",config.at("profile").at("sha256")},{"headSlot",head.header.slot},{"durableClockMs",publication.state->timed.clock.time_ms},{"elapsedMs",elapsed()},{"maxRssKiB",rss()}}.dump()<<std::endl;
 const auto wall_count=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();need(wall_count>=0,"negative observed wall clock");const auto wall=uint64_t(wall_count),from=publication.state->timed.clock.time_ms,genesis_ms=publication.state->timed.clock.genesis_time_ms;
 need(wall>=from&&wall-from<=downtime_limit_ms&&wall>=genesis_ms,"measured downtime outside explicit seven-day projection allowance");
 const auto wall_slot=(wall-genesis_ms)/rules.slot_duration_ms,slots=t.consensus.epoch.ffg.slots_per_epoch,boundary=wall_slot/slots*slots;
 need(boundary>=head.header.slot&&boundary-head.header.slot<=downtime_limit_ms/rules.slot_duration_ms,"head projection gap outside explicit allowance");
 stage="bounded-empty-slot-projection";CapellaSlotPreparation frame(head.state,head.header,epoch_rules,t.consensus.state,check);last_slot=frame.slot();uint64_t chunks=0;
 std::cout<<Json{{"event","projection-start"},{"actualWallObservedMs",wall},{"durableClockMs",from},{"clockDowntimeMs",wall-from},{"headSlot",head.header.slot},{"observedWallSlot",wall_slot},{"projectedEpochBoundarySlot",boundary},{"headGapSlots",boundary-head.header.slot},{"maximumChunkSlots",limits.replay_slots},{"maximumElapsedMs",elapsed_limit_ms},{"maximumRssKiB",rss_limit_kib},{"maximumDowntimeMs",downtime_limit_ms},{"sourceStartupAllowance",config.at("clock").at("startupCatchup")},{"initialRegistry",stats(initial.registry,initial.registry,head.state,t.consensus.state)}}.dump()<<std::endl;
 while(frame.slot()<boundary){check();frame.advance(boundary,limits.replay_slots,limits.sync_trials,check);last_slot=frame.slot();++chunks;
  const auto epoch_boundary=last_slot/slots*slots;need(epoch_boundary==last_slot||last_slot==boundary,"synthetic chunk epoch alignment");
  const auto context=frame.vote_context({last_slot/slots,consensus_beacon_header_root(head.header)},check);
  std::cout<<Json{{"event","projection-chunk"},{"chunk",chunks},{"slot",last_slot},{"elapsedMs",elapsed()},{"maxRssKiB",rss()},{"registry",stats(context.registry,initial.registry,frame.state(),t.consensus.state)}}.dump()<<std::endl;
 }
 const auto context=frame.vote_context({boundary/slots,consensus_beacon_header_root(head.header)},check);const auto projected=stats(context.registry,initial.registry,frame.state(),t.consensus.state);
 stage="readonly-proposer-availability";if(frame.slot()<wall_slot)frame.advance(wall_slot,limits.replay_slots,limits.sync_trials,check);last_slot=frame.slot();
 Json proposer=nullptr;std::string proposer_error;try{const auto identity=frame.proposer_identity(t.consensus.epoch.ffg,check);if(identity)proposer={{"index",identity->index},{"owner",identity->index<16?0:identity->index<32?1:2}};}catch(const std::exception& e){proposer_error=e.what();}
 stage="final-source-recheck";need(disk.fork_checkpoint(genesis.chain_id,genesis.block.header.hash(),limits.envelope_bytes,primary->token).has_value()&&witness.fork_checkpoint(genesis.chain_id,genesis.block.header.hash(),2048+8*limits.envelope_bytes,saved->token).has_value(),"source checkpoint changed during projection");check();
 std::cout<<Json{{"event","projection-terminal"},{"result","PROJECTED_READONLY_ONLY"},{"asOfWallObservedMs",wall},{"projectionFinishedWallMs",std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()},{"clockDowntimeMs",wall-from},{"projectedSlot",frame.slot()},{"chunks",chunks},{"projectedRegistry",projected},{"selectedProposer",proposer},{"proposerDiagnostic",proposer_error.empty()?Json(nullptr):Json(proposer_error)},{"elapsedMs",elapsed()},{"maxRssKiB",rss()},{"sourceReadOnly",true},{"sourceCheckpointUnchanged",true},{"clockPublished",false},{"dutiesStarted",false},{"signingInvoked",false},{"networkStarted",false},{"networkQualified",false},{"productionQualified",false}}.dump()<<std::endl;return 0;
 }catch(const std::exception& e){std::cout<<Json{{"event","projection-terminal"},{"result","FAILED_OR_BOUNDED_REFUSAL"},{"stage",stage},{"message",e.what()},{"lastCommittedProjectionSlot",last_slot},{"elapsedMs",elapsed()},{"maxRssKiB",rss()},{"checkpointCalls",checks},{"dutiesStarted",false},{"signingInvoked",false},{"networkStarted",false},{"networkQualified",false},{"productionQualified",false}}.dump()<<std::endl;return 1;}}
