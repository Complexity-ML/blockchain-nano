// Synthesized public trusted-context diagnostic, linked to qualified core85.
// The source fixture is hash pinned. No DB, journal, signer, runtime or network.
#include "nano/consensus_capella_store_snapshot.hpp"
#include "nano/consensus_capella_duty.hpp"
#include <fstream>
#include <iostream>
#include <chrono>
#include <sys/resource.h>
using namespace nano;
namespace {
auto started=std::chrono::steady_clock::now();uint64_t checks=0;
void need(bool b,const char* why){if(!b)throw std::runtime_error(why);}
uint64_t elapsed(){return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-started).count();}
uint64_t rss(){rusage u{};need(getrusage(RUSAGE_SELF,&u)==0,"resource observation");return uint64_t(u.ru_maxrss)/1024;}
void check(){++checks;need(elapsed()<60000,"60-second wall budget");if((checks&1023)==0)need(rss()<=131072,"128-MiB RSS budget");}
std::string read(const char* p){need(std::filesystem::is_regular_file(p)&&std::filesystem::file_size(p)<=1048576,"public input bounds");std::ifstream f(p,std::ios::binary);std::string s((std::istreambuf_iterator<char>(f)),{});need(!f.bad(),"public read");return s;}
Bytes hash(const std::string& s){return sha256(Bytes(s.begin(),s.end()));}
uint64_t number(const Bytes& b,size_t at,size_t n=8){need(at<=b.size()&&n<=b.size()-at,"SSZ bounds");uint64_t v=0;for(size_t i=0;i<n;++i)v|=uint64_t(b[at+i])<<(8*i);return v;}
void put(Bytes& b,size_t at,uint64_t n){need(at<=b.size()&&8<=b.size()-at,"SSZ write bounds");for(size_t i=0;i<8;++i)b[at+i]=uint8_t(n>>(8*i));}
std::string root(const ConsensusRoot& r){return hex(Bytes(r.begin(),r.end()));}
}
int main(int argc,char** argv){try{
 need(argc==4,"arguments: pinned initial.json pinned profile.json output-public-state.ssz");
 const auto initial=read(argv[1]),profile=read(argv[2]);
 need(hex(hash(initial))=="0x33aee17d626a3e541c7934c5576c025f8819d442555df6fadc90dde49bd045a7","initial SHA pin");
 need(hex(hash(profile))=="0xe3440eba0a871a8eedf84a6e2cdccee53cc4389e8ce6b979166621c8e893890d","profile SHA pin");
 const auto rules=load_capella_fork_profile(profile,hash(profile),1048576);const auto& t=rules.transition;
 need(rules.slot_duration_ms==6000&&t.consensus.epoch.ffg.slots_per_epoch==8&&t.consensus.epoch.sync.epochs_per_sync_committee_period==8,"profile timing");
 const CapellaStoreSnapshotLimits limits{134217728,256,2048,1000,4194304,67108864,64,100000};
 const CapellaEpochRules epochs{t.consensus.epoch,t.execution,t.maximum_state_bytes};
 auto store=capella_store_from_snapshot(initial,hash(initial),rules,limits,check);
 const auto original=capella_fork_head(store,rules,check);need(original->header.slot==1&&store.blocks.size()==1,"single public trusted anchor");
 auto candidate=*original;auto& bytes=candidate.state.ssz;const auto& l=t.consensus.state;
 const size_t historical=176+64*l.slots_per_historical_root;
 const auto validators=number(bytes,historical+88,4),balances=number(bytes,historical+92,4),count=(balances-validators)/121;
 need(count==64&&validators+count*121==balances,"exact 64-validator public registry");
 // All validators have valid positive balances but planned exits at E8.
 // No epoch, balance, fork rule or signature of the original fixture changes.
 // This is a NEW synthesized test anchor, never a claimed real signed block.
 for(size_t i=0;i<count;++i){check();put(bytes,validators+121*i+105,8);put(bytes,validators+121*i+113,264);}
 candidate.header.state_root=capella_beacon_state_root(bytes,l,t.execution,t.maximum_state_bytes,check);
 const auto old_root=consensus_beacon_header_root(original->header),new_root=consensus_beacon_header_root(candidate.header);
 need(new_root!=old_root,"new synthetic trusted root must differ");
 validate_capella_execution_record(candidate,rules,check);
 const auto context=capella_fork_block_context(candidate.state,candidate.header,epochs,l,check);
 size_t active=0;for(const auto& [i,v]:context.registry.validators){need(v.balance>0&&!v.slashed&&v.exit_epoch==8,"positive unslashed scheduled exits");if(v.activation_epoch<=context.registry.epoch&&context.registry.epoch<v.exit_epoch)++active;}
 need(active==64,"all validators active at original epoch0");
 // Rebind the single-anchor graph to this explicitly trusted synthetic header.
 auto node=store.timed.view.nodes.at(old_root);store.timed.view.nodes.clear();store.timed.view.nodes.emplace(new_root,node);
 store.blocks.clear();store.blocks.emplace(new_root,CapellaForkRecord(candidate));store.timely.clear();store.unrealized_justifications.clear();
 store.timed.view.anchor=new_root;store.network_anchor=new_root;
 store.timed.view.justified={0,new_root};store.timed.view.finalized={0,new_root};store.timed.view.justified_registry=context.registry;
 store.timed.view.latest_messages.clear();store.timed.view.equivocating_indices.clear();store.timed.view.proposer_boost.reset();
 store.timed.clock.unrealized_justified={0,new_root};store.timed.clock.unrealized_finalized={0,new_root};
 store.timed.clock.time_ms=store.timed.clock.genesis_time_ms+64*6000;store.timed.view.current_epoch=8;
 need(capella_fork_head(store,rules,check)->header==candidate.header,"synthetic head authenticated by real core");
 {std::ofstream out(argv[3],std::ios::binary);out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());need(bool(out),"write public derived SSZ evidence");}
 CapellaDutyPreparation frame(store,rules,64,check);size_t successful_chunks=0;std::string failure;
 const auto before=capella_store_snapshot(store,rules,limits,check);
 try{while(!frame.advance(16,100000,check)){need(++successful_chunks<=4,"finite chunk cap");}}catch(const std::invalid_argument& e){failure=e.what();}
 need(failure=="empty Altair sync active set","real pure preparation must reach expected empty future committee");
 need(frame.prepared_slot()==49,"failed bounded chunk keeps prior complete prefix");
 need(before==capella_store_snapshot(store,rules,limits,check),"input store changed during failed pure preparation");
 need(capella_fork_head(store,rules,check)->header==candidate.header,"input remains valid after refusal");check();need(rss()<=131072,"terminal RSS budget");
 std::cout<<Json{{"format","nano-pure-duty-projection-counterexample-v1"},{"result","EXPECTED_PROJECTION_REFUSAL_REPRODUCED"},{"initialSha256",hex(hash(initial))},{"profileSha256",hex(hash(profile))},{"syntheticStateSha256",hex(sha256(bytes))},{"syntheticStateRoot",root(candidate.header.state_root)},{"syntheticHeadRoot",root(new_root)},{"originalHeadRoot",root(old_root)},{"sourceEpoch",0},{"sourceActiveCount",active},{"plannedExitEpoch",8},{"projectedTargetSlot",64},{"firstUnavailableSelectionEpoch",8},{"lastCompletePrefixSlot",frame.prepared_slot()},{"completedChunks",successful_chunks},{"exceptionClass","std::invalid_argument"},{"exception",failure},{"sourceStoreUnchanged",true},{"coreStateStillValid",true},{"elapsedMs",elapsed()},{"maxRssKiB",rss()},{"databaseOpened",false},{"signerInvoked",false},{"runtimeStarted",false},{"networkStarted",false},{"productionQualified",false}}.dump()<<std::endl;return 0;
 }catch(const std::exception& e){std::cout<<Json{{"result","HARNESS_DIAGNOSTIC_FAILURE"},{"exception",e.what()},{"elapsedMs",elapsed()},{"maxRssKiB",rss()}}.dump()<<std::endl;return 2;}}
