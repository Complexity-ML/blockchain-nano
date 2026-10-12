// Public canonical native fixture projection only. No ChainStore, journal,
// key provider, runtime, clock publication, RPC or P2P instance is constructed.
#include "nano/consensus_capella_store_snapshot.hpp"
#include "nano/consensus_capella_fork_state.hpp"
#include "nano/chain.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <sys/resource.h>
using namespace nano;
namespace {
using Clock=std::chrono::steady_clock;auto began=Clock::now();uint64_t checks=0,last_slot=0;std::string stage="public-input";
constexpr uint64_t wall_budget_ms=60000,rss_budget_kib=131072,downtime_budget_ms=7200000,ordinary_step_ms=120000,maximum_startup_steps=60;
uint64_t elapsed(){return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-began).count();}
uint64_t rss(){rusage u{};if(getrusage(RUSAGE_SELF,&u))throw std::runtime_error("resource observation failed");
#ifdef __APPLE__
 return uint64_t(u.ru_maxrss)/1024;
#else
 return uint64_t(u.ru_maxrss);
#endif
}
void need(bool b,const char* message){if(!b)throw std::runtime_error(message);}
void check(){++checks;need(elapsed()<=wall_budget_ms,"projection wall budget exceeded");if((checks&1023)==0)need(rss()<=rss_budget_kib,"projection RSS budget exceeded");}
std::string read(const char* path){need(std::filesystem::is_regular_file(path)&&std::filesystem::file_size(path)<=1024*1024,"public input bounds");std::ifstream f(path,std::ios::binary);std::string value((std::istreambuf_iterator<char>(f)),{});need(!f.bad(),"public input read failed");return value;}
std::string digest(const std::string& value){return hex(sha256(Bytes(value.begin(),value.end())));}
Json stats(const ConsensusSlashingState& registry,const ConsensusSlashingState& original){
 std::array<uint64_t,3> active{},exited{},slashed{},scheduled{};uint64_t minimum=UINT64_MAX,maximum=0;
 need(registry.validators.size()==64&&original.validators.size()==64,"canonical predeposit registry must retain 64");
 for(const auto& [i,v]:registry.validators){check();need(i<64,"pending validator64 unexpectedly admitted");const auto owner=i<16?0:i<32?1:2;if(v.activation_epoch<=registry.epoch&&registry.epoch<v.exit_epoch)++active[owner];if(v.exit_epoch<=registry.epoch)++exited[owner];if(v.slashed)++slashed[owner];if(original.validators.at(i).exit_epoch==UINT64_MAX&&v.exit_epoch!=UINT64_MAX)++scheduled[owner];minimum=std::min(minimum,v.balance);maximum=std::max(maximum,v.balance);}
 return {{"epoch",registry.epoch},{"registryCount",64},{"activeByOwner",active},{"exitedByOwner",exited},{"slashedByOwner",slashed},{"newlyScheduledExitsByOwner",scheduled},{"balanceMinimumGwei",minimum},{"balanceMaximumGwei",maximum}};
}
}
int main(int argc,char** argv){try{
 need(argc==5,"arguments: initial.json profile.json genesis.json actualWallMs");const auto initial_text=read(argv[1]),profile_text=read(argv[2]),genesis_text=read(argv[3]);
 need(digest(initial_text)=="0x33aee17d626a3e541c7934c5576c025f8819d442555df6fadc90dde49bd045a7"&&digest(profile_text)=="0xe3440eba0a871a8eedf84a6e2cdccee53cc4389e8ce6b979166621c8e893890d"&&digest(genesis_text)=="0x3373c76de09192ad440625613db97c17b50fbfeddb5adb3190c8f0eb83a22005","canonical public input SHA pins differ");
 const auto actual=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();need(actual>=0,"negative observed wall time");const uint64_t wall=std::stoull(argv[4]);need(wall<=uint64_t(actual)&&uint64_t(actual)-wall<=5000,"supplied wall time not a fresh genuine host observation");
 const auto genesis=Genesis::from_json(Json::parse(genesis_text));need(genesis.chain_id==1337&&genesis.consensus=="beacon"&&genesis.evm_revision==EvmRevision::Shanghai&&hex(genesis.block.header.hash())=="0xa055278725b54aed2503a383772972e2920c54aa05713cc1bd3ada016394f790","canonical execution genesis differs");
 const auto rules=load_capella_fork_profile(profile_text,unhex(digest(profile_text)),1024*1024);const auto& t=rules.transition;need(rules.slot_duration_ms==6000&&t.consensus.epoch.ffg.slots_per_epoch==8,"canonical profile differs");
 const CapellaStoreSnapshotLimits limits{134217728,256,2048,1000,4194304,67108864,64,100000};const CapellaEpochRules epochs{t.consensus.epoch,t.execution,t.maximum_state_bytes};
 stage="public-initial-authentication";const auto store=capella_store_from_snapshot(initial_text,unhex(digest(initial_text)),rules,limits,check);const auto pin=capella_fork_head(store,rules,check);const auto& head=*pin;need(head.header.slot==1&&hex(Bytes(head.state.ssz.begin()+8,head.state.ssz.begin()+40))=="0xfe0dc8b10d151359ca3836e844266bbbe45352a35374d87b5f6527a9ffa2d753","canonical bootstrap identity differs");
 const auto initial=capella_fork_block_context(head.state,head.header,epochs,t.consensus.state,check);const auto from=store.timed.clock.time_ms,genesis_ms=store.timed.clock.genesis_time_ms;need(wall>=from&&wall-from<=downtime_budget_ms&&wall>=genesis_ms,"actual wall gap outside finite two-hour policy");const auto target=(wall-genesis_ms)/rules.slot_duration_ms;need(target>=1&&target<=1201,"actual slot gap exceeds finite policy");
 std::cout<<Json{{"event","public-canonical-authenticated"},{"actualWallObservedMs",wall},{"initialClockMs",from},{"clockDowntimeMs",wall-from},{"targetSlot",target},{"maximumDowntimeMs",downtime_budget_ms},{"maximumStartupSteps",maximum_startup_steps},{"ordinaryMaximumForwardStepMs",ordinary_step_ms},{"runtimeReplaySlots",limits.replay_slots},{"initialRegistry",stats(initial.registry,initial.registry)}}.dump()<<std::endl;
 stage="empty-slot-projection";CapellaSlotPreparation frame(head.state,head.header,epochs,t.consensus.state,check);uint64_t chunks=0,last_registry_slot=1;Json projected=stats(initial.registry,initial.registry);
 while(frame.slot()<target){check();auto bounded=frame.slot()+std::min(uint64_t(16),target-frame.slot());auto next=bounded/8*8;if(next<=frame.slot())next=bounded;need(next>frame.slot()&&next-frame.slot()<=16,"aligned bounded chunk failed");frame.advance(next,limits.replay_slots,limits.sync_trials,check);last_slot=frame.slot();need(++chunks<=80,"projection chunk budget exceeded");if(last_slot%8==0){const auto context=frame.vote_context({last_slot/8,consensus_beacon_header_root(head.header)},check);projected=stats(context.registry,initial.registry);last_registry_slot=last_slot;std::cout<<Json{{"event","public-projection-chunk"},{"chunk",chunks},{"slot",last_slot},{"elapsedMs",elapsed()},{"registry",projected}}.dump()<<std::endl;}else std::cout<<Json{{"event","public-projection-partial-slot"},{"chunk",chunks},{"slot",last_slot},{"registrySnapshotSlot",last_registry_slot},{"elapsedMs",elapsed()}}.dump()<<std::endl;}
 stage="future-proposer-availability";need(last_registry_slot/8==target/8,"registry telemetry epoch differs from actual projected proposer epoch");const auto proposer=frame.proposer_identity(t.consensus.attestation.duties,check);need(proposer.has_value(),"no current-slot proposer after actual wall projection");check();need(rss()<=rss_budget_kib,"terminal RSS budget exceeded");
 std::cout<<Json{{"event","public-projection-terminal"},{"result","PROJECTED_PUBLIC_READONLY_ONLY"},{"actualWallObservedMs",wall},{"initialClockMs",from},{"clockDowntimeMs",wall-from},{"projectedSlot",frame.slot()},{"projectionChunks",chunks},{"runtimeMinimumStepCapacity",(wall-from+ordinary_step_ms-1)/ordinary_step_ms},{"registrySnapshotSlot",last_registry_slot},{"projectedRegistry",projected},{"selectedProposer",proposer->index},{"elapsedMs",elapsed()},{"maxRssKiB",rss()},{"clockPublished",false},{"journalsOpened",false},{"keysLoaded",false},{"signingInvoked",false},{"runtimeStarted",false},{"networkStarted",false},{"networkQualified",false},{"productionQualified",false}}.dump()<<std::endl;return 0;
 }catch(const std::exception& e){std::cout<<Json{{"event","public-projection-terminal"},{"result","FAILED_OR_BOUNDED_REFUSAL"},{"stage",stage},{"message",e.what()},{"lastFullyCompletedProjectionSlot",last_slot},{"elapsedMs",elapsed()},{"maxRssKiB",rss()},{"journalsOpened",false},{"keysLoaded",false},{"signingInvoked",false},{"runtimeStarted",false},{"productionQualified",false}}.dump()<<std::endl;return 1;}}
