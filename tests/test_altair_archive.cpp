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
 const auto transition=altair_transition_rules(fixtures);const auto reference=read(corpus/"minimal-config-v1.6.0-beta.0.yaml");const auto duration=parameter(reference,"SECONDS_PER_SLOT")*1000;
 const AltairForkBlockRules rules{transition,duration,duration/3,parameter(reference,"PROPOSER_SCORE_BOOST"),AltairProposerBoostProfile::Release160Beta0};
 const AltairStoreSnapshotLimits limits{32*1024*1024,100,10000,10000,1024*1024,16*1024*1024,4096,100000};
 const auto initial=initialize_altair_fork_store({binary(dir/"anchor_state.ssz"),std::nullopt},binary(dir/"anchor_block.ssz"),rules);
 Temp temp;const Bytes genesis(32,9);const auto path=temp/"db";auto disk=std::make_unique<ChainStore>(path);disk->bind_chain(1337,genesis,"altair-qualification");auto reactor=std::make_unique<AltairDurableReactor>(*disk,1337,genesis,rules,limits,initial);
 check(reactor->current().signed_blocks->empty(),"anchor invented a signature");unsigned blocks=0;bool sync_tested=false;
 for(const auto& step:load(dir/"steps.json")){
  if(step.contains("tick"))reactor->tick(step.at("tick").get<uint64_t>()*1000);
  else if(step.contains("block")){
   const auto payload=binary(dir/(step.at("block").get<std::string>()+".ssz"));
   if(!blocks){
    const auto before=reactor->current();size_t points=0;
    {ChainStore shadow_disk(temp/"shadow");shadow_disk.bind_chain(1337,genesis,"altair-qualification");AltairDurableReactor shadow(shadow_disk,1337,genesis,rules,limits,*before.state);check(shadow.block(payload,[&]{++points;}),"shadow block failed");}
    size_t seen=0;bool cancelled=false;try{reactor->block(payload,[&]{if(++seen==points)throw std::runtime_error("cancelled");});}catch(const std::runtime_error& e){cancelled=std::string(e.what())=="cancelled";}
    check(cancelled&&reactor->current().token==before.token&&reactor->current().signed_blocks==before.signed_blocks&&disk->fork_checkpoint(1337,genesis,limits.envelope_bytes,before.token).has_value(),"cancelled archive published or changed disk");
   }
#ifdef NANO_TEST_SYNC_FAULT
   if(!sync_tested){
    const auto previous=reactor->current();fs::path wal;for(const auto& entry:fs::directory_iterator(path))if(entry.path().extension()==".log"&&(wal.empty()||entry.path().filename()>wal.filename()))wal=entry.path();check(!wal.empty(),"WAL absent");nano_test_arm_sync_fault(wal.c_str());
    bool failed=false,unavailable=false;try{reactor->block(payload);}catch(const std::runtime_error&){failed=true;}try{reactor->current();}catch(const std::runtime_error&){unavailable=true;}
    check(failed&&unavailable&&nano_test_sync_failures()>0&&previous.signed_blocks->empty(),"failed archive commit was published");nano_test_disarm_sync_fault();
    reactor.reset();disk.reset();disk=std::make_unique<ChainStore>(path);const auto recovered=disk->fork_checkpoint(1337,genesis,limits.envelope_bytes);check(recovered.has_value(),"checkpoint lost");reactor=std::make_unique<AltairDurableReactor>(*disk,1337,genesis,rules,limits,recovered->token);
    const auto state=reactor->current();check(state.state->blocks.size()==state.signed_blocks->size()+1&&state.signed_blocks->size()<=1,"torn state/archive after sync failure");sync_tested=true;
   }
#endif
   check(reactor->block(payload),"official block not admitted");++blocks;const auto published=reactor->current();check(published.signed_blocks->size()==blocks,"new signed block not archived");
   bool found=false;for(const auto& [_,stored]:*published.signed_blocks)if(*stored==payload)found=true;check(found,"archive changed accepted bytes");
  }
 }
 const auto prior=reactor->current();auto forged=*prior.signed_blocks->begin()->second;forged[4]^=1;
 check(reactor->block(forged),"reference duplicate-root handling changed");check(reactor->current().signed_blocks==prior.signed_blocks,"duplicate overwrote original signature");
 const auto saved=reactor->current();const auto record=disk->fork_checkpoint(1337,genesis,limits.envelope_bytes,saved.token);const auto document=Json::parse(record->snapshot);check(document.at("format")=="nano-altair-durable-v2","archive not in durable checkpoint");
 reactor.reset();disk.reset();disk=std::make_unique<ChainStore>(path);reactor=std::make_unique<AltairDurableReactor>(*disk,1337,genesis,rules,limits,saved.token);
 for(const auto& [id,bytes]:*saved.signed_blocks)check(*reactor->current().signed_blocks->at(id)==*bytes,"archive changed on reopen");
 unsigned rejected=0;
 auto refuse=[&](Json value){const auto where=temp/("bad-"+std::to_string(rejected));ChainStore bad(where);bad.bind_chain(1337,genesis,"altair-qualification");const auto token=bad.commit_fork_checkpoint(1337,genesis,std::nullopt,value.dump(),limits.envelope_bytes);bool failed=false;try{AltairDurableReactor candidate(bad,1337,genesis,rules,limits,token);}catch(const std::exception&){failed=true;}check(failed,"corrupted archive accepted even with a recomputed outer digest");++rejected;};
 for(size_t offset:{size_t(4),size_t(100),size_t(184)}){auto changed=document;auto payload=unhex(changed["signedBlocks"][0][1].get<std::string>());payload[offset]^=1;changed["signedBlocks"][0][1]=hex(payload);refuse(changed);}
 auto changed=document;changed["signedBlocks"][0][0]=hex(Bytes(32,0));refuse(changed);changed=document;changed["signedBlocks"].push_back(changed["signedBlocks"][0]);refuse(changed);
 // Legacy authenticated state is readable; missing bytes cannot be recreated
 // through the duplicate-root shortcut, even when resubmitted bytes are valid.
 ChainStore legacy(temp/"legacy");legacy.bind_chain(1337,genesis,"altair-qualification");const auto old=altair_store_snapshot(*saved.state,rules,limits);const auto old_token=legacy.commit_fork_checkpoint(1337,genesis,std::nullopt,old,limits.envelope_bytes);
 AltairDurableReactor migrated(legacy,1337,genesis,rules,limits,old_token);check(migrated.current().signed_blocks->empty(),"legacy checkpoint invented archived payloads");check(migrated.block(*saved.signed_blocks->begin()->second)&&migrated.current().signed_blocks->empty(),"duplicate shortcut backfilled unverified legacy signature");
 std::cout<<Json{{"scenario","altair-signed-block-archive"},{"officialBlocks",blocks},{"corruptArchivesRejected",rejected},{"reopened",true},{"legacyReadWithoutBackfill",true},{"preCommitCancellation",true},{"syncFailureTested",sync_tested},{"automaticPropagation",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
