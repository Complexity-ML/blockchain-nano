#include "support/altair_transition_rules.hpp"
#include "nano/consensus_altair_reactor.hpp"
#ifdef NANO_TEST_SYNC_FAULT
extern "C" void nano_test_arm_sync_fault(const char*);
extern "C" unsigned nano_test_sync_failures();
extern "C" void nano_test_disarm_sync_fault();
#endif
using namespace test;using namespace test::phase0_minimal;
int main(){try{
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures",corpus=fixtures/"consensus-altair-fork-head-official",dir=corpus/"chain_no_attestations";
 const auto manifest=load(corpus/"manifest.json");for(const auto& row:manifest.at("cases"))if(row.at("name")=="chain_no_attestations")for(const auto& [path,info]:row.at("files").items())check(hex(sha256(binary(corpus/path))).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");
 const auto transition=altair_transition_rules(fixtures);const auto config=read(corpus/"minimal-config-v1.6.0-beta.0.yaml");const auto duration=parameter(config,"SECONDS_PER_SLOT")*1000;
 const AltairForkBlockRules rules{transition,duration,duration/3,parameter(config,"PROPOSER_SCORE_BOOST"),AltairProposerBoostProfile::Release160Beta0};
 const AltairStoreSnapshotLimits limits{32*1024*1024,100,10000,10000,1024*1024,16*1024*1024,4096,100000};
 const auto initial=initialize_altair_fork_store({binary(dir/"anchor_state.ssz"),std::nullopt},binary(dir/"anchor_block.ssz"),rules);
 const Int chain_id=1337;const Bytes genesis(32,9);Temp directory;const auto path=directory/"db";
 auto disk=std::make_unique<ChainStore>(path);disk->bind_chain(chain_id,genesis,"altair-qualification");
 auto reactor=std::make_unique<AltairDurableReactor>(*disk,chain_id,genesis,rules,limits,initial);
 const auto before=reactor->current();check(!reactor->vote_ssz(Bytes{0,1})&&!reactor->slashing_ssz(Bytes{0,1})&&reactor->current().token==before.token,"malformed wire message committed");check(!reactor->block(Bytes{0,1})&&reactor->current().token==before.token,"invalid block published");
 size_t callbacks=0;auto candidate=prepare_altair_store_tick(initial,1,rules,limits.replay_slots,limits.sync_trials,[&]{++callbacks;});altair_store_snapshot(candidate,rules,limits,[&]{++callbacks;});++callbacks;
 const std::set<size_t> stops{1,2,callbacks/2,callbacks-1,callbacks};
 for(auto stop:stops){size_t calls=0;bool cancelled=false;try{reactor->tick(1,[&]{if(++calls==stop)throw std::runtime_error("cancelled");});}catch(const std::runtime_error& e){cancelled=std::string(e.what())=="cancelled";}check(cancelled&&reactor->current().token==before.token&&reactor->current().state==before.state,"cancelled publication");check(disk->fork_checkpoint(chain_id,genesis,limits.envelope_bytes,before.token).has_value(),"cancelled disk change");}
 reactor->tick(1);const auto accepted=reactor->current();check(accepted.token.sequence==before.token.sequence+1&&accepted.state->timed.clock.time_ms==1&&before.state->timed.clock.time_ms==0,"immutable publication order");
 const auto saved=disk->fork_checkpoint(chain_id,genesis,limits.envelope_bytes,accepted.token);check(altair_store_from_snapshot(saved->snapshot,accepted.token.digest,rules,limits).timed.clock.time_ms==1,"published before commit");
 const auto competing=disk->commit_fork_checkpoint(chain_id,genesis,accepted.token,saved->snapshot,limits.envelope_bytes);
 bool stale=false,blocked=false;try{reactor->tick(2);}catch(const std::invalid_argument&){stale=true;}try{reactor->current();}catch(const std::runtime_error&){blocked=true;}check(stale&&blocked&&accepted.state->timed.clock.time_ms==1,"stale reactor remained usable");
 reactor.reset();disk.reset();disk=std::make_unique<ChainStore>(path);reactor=std::make_unique<AltairDurableReactor>(*disk,chain_id,genesis,rules,limits,competing);reactor->tick(2);check(reactor->current().state->timed.clock.time_ms==2,"reactor restart did not resume");
 bool sync_tested=false;
#ifdef NANO_TEST_SYNC_FAULT
 const auto stable=reactor->current();fs::path wal;for(const auto& e:fs::directory_iterator(path))if(e.path().extension()==".log"&&(wal.empty()||e.path().filename()>wal.filename()))wal=e.path();check(!wal.empty(),"WAL missing");nano_test_arm_sync_fault(wal.c_str());
 bool failed=false,unavailable=false;try{reactor->tick(3);}catch(const std::runtime_error&){failed=true;}try{reactor->current();}catch(const std::runtime_error&){unavailable=true;}
 check(failed&&unavailable&&nano_test_sync_failures()>0&&stable.state->timed.clock.time_ms==2,"disk error published candidate");nano_test_disarm_sync_fault();sync_tested=true;
 reactor.reset();disk.reset();disk=std::make_unique<ChainStore>(path);const auto recovered=disk->fork_checkpoint(chain_id,genesis,limits.envelope_bytes);check(recovered.has_value(),"checkpoint lost on error");
 reactor=std::make_unique<AltairDurableReactor>(*disk,chain_id,genesis,rules,limits,recovered->token);const auto ms=reactor->current().state->timed.clock.time_ms;check(ms==2||ms==3,"torn reactor checkpoint");reactor->tick(4);check(reactor->current().state->timed.clock.time_ms==4,"failed to resume resolved write");
#endif
 std::cout<<Json{{"scenario","altair-durable-reactor-publication"},{"cancellations",stops.size()},{"staleReactorBlocked",true},{"invalidBlockNotCommitted",true},{"syncErrorTested",sync_tested},{"networkActive",false},{"executionBinding",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
