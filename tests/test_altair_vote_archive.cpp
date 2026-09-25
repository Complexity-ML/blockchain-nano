#include "support/altair_transition_rules.hpp"
#include "nano/consensus_altair_reactor.hpp"
#ifdef NANO_TEST_SYNC_FAULT
extern "C" void nano_test_arm_sync_fault(const char*);
extern "C" unsigned nano_test_sync_failures();
extern "C" void nano_test_disarm_sync_fault();
#endif
using namespace test;using namespace test::phase0_minimal;
int main(){try{
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures",corpus=fixtures/"consensus-altair-fork-head-official",dir=corpus/"shorter_chain_but_heavier_weight";
 const auto manifest=load(corpus/"manifest.json");for(const auto& row:manifest.at("cases"))if(row.at("name")=="shorter_chain_but_heavier_weight")for(const auto& [path,info]:row.at("files").items())check(hex(sha256(binary(corpus/path))).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");
 const auto transition=altair_transition_rules(fixtures);const auto reference=read(corpus/"minimal-config-v1.6.0-beta.0.yaml");const auto duration=parameter(reference,"SECONDS_PER_SLOT")*1000;
 const AltairForkBlockRules rules{transition,duration,duration/3,parameter(reference,"PROPOSER_SCORE_BOOST"),AltairProposerBoostProfile::Release160Beta0};
 const AltairStoreSnapshotLimits limits{32*1024*1024,100,10000,10000,1024*1024,16*1024*1024,4096,100000};
 const auto initial=initialize_altair_fork_store({binary(dir/"anchor_state.ssz"),std::nullopt},binary(dir/"anchor_block.ssz"),rules);

 Temp temp;const Bytes genesis(32,9);const auto path=temp/"db";auto disk=std::make_unique<ChainStore>(path);disk->bind_chain(1337,genesis,"altair-qualification");auto reactor=std::make_unique<AltairDurableReactor>(*disk,1337,genesis,rules,limits,initial);
 Bytes vote;
 for(const auto& step:load(dir/"steps.json")){
  if(step.contains("tick"))reactor->tick(step.at("tick").get<uint64_t>()*1000);
  else if(step.contains("block"))check(reactor->block(binary(dir/(step.at("block").get<std::string>()+".ssz"))),"official block failed");
  else if(step.contains("attestation")){vote=binary(dir/(step.at("attestation").get<std::string>()+".ssz"));break;}
 }
 check(!vote.empty(),"missing official vote");const auto before=reactor->current();
 auto forged=vote;forged[132]^=1;check(!reactor->vote_ssz(forged)&&reactor->current().token==before.token,"forged signature committed");
 bool cancelled=false;try{reactor->vote_ssz(vote,[]{throw std::runtime_error("cancelled");});}catch(const std::runtime_error& e){cancelled=std::string(e.what())=="cancelled";}check(cancelled&&reactor->current().token==before.token&&reactor->current().signed_votes->empty(),"cancelled vote published");

 bool sync_tested=false;
#ifdef NANO_TEST_SYNC_FAULT
 fs::path wal;for(const auto& entry:fs::directory_iterator(path))if(entry.path().extension()==".log"&&(wal.empty()||entry.path().filename()>wal.filename()))wal=entry.path();check(!wal.empty(),"WAL absent");nano_test_arm_sync_fault(wal.c_str());
 bool failed=false,unavailable=false;try{reactor->vote_ssz(vote);}catch(const std::runtime_error&){failed=true;}try{reactor->current();}catch(const std::runtime_error&){unavailable=true;}
 check(failed&&unavailable&&nano_test_sync_failures()>0&&before.signed_votes->empty(),"failed vote commit was published");nano_test_disarm_sync_fault();
 reactor.reset();disk.reset();disk=std::make_unique<ChainStore>(path);const auto recovered=disk->fork_checkpoint(1337,genesis,limits.envelope_bytes);check(recovered.has_value(),"checkpoint lost after sync failure");reactor=std::make_unique<AltairDurableReactor>(*disk,1337,genesis,rules,limits,recovered->token);
 const auto recovered_state=reactor->current();check(recovered_state.signed_votes->size()<=1&&recovered_state.signed_votes->empty()==(recovered_state.state->timed.view.latest_messages==before.state->timed.view.latest_messages),"torn vote/state after sync failure");sync_tested=true;
#endif
 check(reactor->vote_ssz(vote),"official vote rejected");auto current=reactor->current();check(current.signed_votes->size()==1&&*current.signed_votes->begin()->second==vote,"signed vote not atomically archived");
 check(current.state->timed.view.latest_messages!=before.state->timed.view.latest_messages,"fixture vote had no effect");
 check(reactor->vote_ssz(vote)&&reactor->current().signed_votes==current.signed_votes,"duplicate amplified archive");
 current=reactor->current();check(!reactor->vote_ssz(forged)&&reactor->current().token==current.token,"known vote bypassed signature verification");
 const auto token=current.token;const auto record=disk->fork_checkpoint(1337,genesis,limits.envelope_bytes,token);const auto document=Json::parse(record->snapshot);check(document.at("format")=="nano-altair-durable-v3","vote checkpoint format");
 reactor.reset();disk.reset();disk=std::make_unique<ChainStore>(path);reactor=std::make_unique<AltairDurableReactor>(*disk,1337,genesis,rules,limits,token);
 check(*reactor->current().signed_votes->begin()->second==vote&&reactor->current().state->timed.view.latest_messages==current.state->timed.view.latest_messages,"vote/state changed on reopen");
 unsigned rejected=0;
 auto refuse=[&](Json value){ChainStore bad(temp/("bad-"+std::to_string(rejected)));bad.bind_chain(1337,genesis,"altair-qualification");const auto t=bad.commit_fork_checkpoint(1337,genesis,std::nullopt,value.dump(),limits.envelope_bytes);bool failed=false;try{AltairDurableReactor candidate(bad,1337,genesis,rules,limits,t);}catch(const std::exception&){failed=true;}check(failed,"corrupt vote archive accepted with recomputed digest");++rejected;};
 auto altered=document;altered["signedVotes"][0][1]=hex(forged);altered["signedVotes"][0][0]=hex(sha256(forged));refuse(altered);
 altered=document;altered["signedVotes"][0][0]=hex(Bytes(32,0));refuse(altered);
 altered=document;altered["signedVotes"].push_back(altered["signedVotes"][0]);refuse(altered);
 // Previous-epoch gossip remains exportable; older payloads expire in the
 // same atomic checkpoint as the clock, without deleting fork-choice history.
 const auto epoch_ms=duration*transition.epoch.ffg.slots_per_epoch;
 reactor->tick(epoch_ms);check(reactor->current().signed_votes->size()==1,"previous epoch vote pruned early");
 reactor->tick(2*epoch_ms);const auto expired=reactor->current();check(expired.signed_votes->empty()&&expired.state->timed.view.latest_messages==current.state->timed.view.latest_messages,"expiry altered consensus vote history");
 check(!reactor->vote_ssz(vote)&&reactor->current().token==expired.token,"expired vote re-entered outbox");
 reactor.reset();disk.reset();disk=std::make_unique<ChainStore>(path);reactor=std::make_unique<AltairDurableReactor>(*disk,1337,genesis,rules,limits,expired.token);check(reactor->current().signed_votes->empty(),"expired vote returned on reopen");
 std::cout<<Json{{"scenario","altair-signed-vote-archive"},{"officialVotes",1},{"reopened",true},{"corruptArchivesRejected",rejected},{"duplicateBounded",true},{"expiryAtomic",true},{"syncFailureTested",sync_tested},{"automaticVotePropagation",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
