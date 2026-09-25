#include "support/altair_transition_rules.hpp"
#include "nano/consensus_altair_reactor.hpp"
#include "nano/consensus_altair_duty.hpp"
#include "nano/consensus_message_ssz.hpp"
#include "blst.h"
#include "nano/consensus_altair_runtime.hpp"
#include "../network/altair_rpc_service.hpp"
#include "nano/http.hpp"
using namespace test;using namespace test::phase0_minimal;
namespace {
struct RunningHttp {HttpServer& server;std::thread thread;explicit RunningHttp(HttpServer& s):server(s),thread([&s]{s.run();}){}~RunningHttp(){server.stop();thread.join();}};
void put32(Bytes& b,size_t at,uint32_t n){for(unsigned i=0;i<4;++i)b.at(at+i)=uint8_t(n>>(8*i));}
Bytes body_with_votes(const Bytes& base,const std::vector<Phase0SignedAttestation>& votes,const AltairTransitionRules& r){
 const auto fixed=220+(r.state.sync_committee_size+7)/8+96;
 check(base.size()>=fixed&&votes.size()<=r.body.attestations,"body fixture or attestation capacity");
 Bytes b(base.begin(),base.begin()+fixed),list(4*votes.size());
 // No SyncCommittee signatures supplied in this bounded local campaign.
 std::fill(b.begin()+220,b.end(),0);b[220+(r.state.sync_committee_size+7)/8]=0xc0;
 for(size_t i=0;i<votes.size();++i){put32(list,4*i,uint32_t(list.size()));const auto bytes=encode_consensus_attestation_ssz(votes[i],r.body.validators_per_committee);list.insert(list.end(),bytes.begin(),bytes.end());}
 put32(b,200,fixed);put32(b,204,fixed);put32(b,208,fixed);put32(b,212,fixed+list.size());put32(b,216,fixed+list.size());b.insert(b.end(),list.begin(),list.end());return b;
}
struct Key {
 blst_scalar secret;Bytes public_key;std::unique_ptr<ChainStore> disk;std::unique_ptr<BeaconSigningJournal> journal;
 Bytes sign(const ConsensusRoot& root)const{static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";blst_p2 h,s;blst_hash_to_g2(&h,root.data(),root.size(),reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);blst_sign_pk_in_g1(&s,&h,&secret);Bytes bytes(96);blst_p2_compress(bytes.data(),&s);return bytes;}
};
}
int main(int argc,char** argv){try{
 const bool no_votes=argc==2&&std::string(argv[1])=="without-votes",minority=argc==2&&std::string(argv[1])=="minority-votes";const bool adversarial=argc==2&&std::string(argv[1])=="adversarial-finality";const bool p2p_restart=argc==3&&std::string(argv[1])=="p2p-restart";const bool p2p=argc==3&&(std::string(argv[1])=="p2p"||p2p_restart);check(argc==1||no_votes||minority||p2p||adversarial,"unexpected scenario argument");
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures",corpus=fixtures/"consensus-altair-fork-head-official",dir=corpus/"shorter_chain_but_heavier_weight";
 const auto manifest=load(corpus/"manifest.json");
 for(const auto& row:manifest.at("cases"))if(row.at("name")=="shorter_chain_but_heavier_weight")for(const auto& [path,info]:row.at("files").items())check(hex(sha256(binary(corpus/path))).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");
 const auto t=altair_transition_rules(fixtures);const auto ref=read(corpus/"minimal-config-v1.6.0-beta.0.yaml");const auto duration=parameter(ref,"SECONDS_PER_SLOT")*1000,slots=t.epoch.ffg.slots_per_epoch;
 const AltairForkBlockRules rules{t,duration,duration/3,parameter(ref,"PROPOSER_SCORE_BOOST"),AltairProposerBoostProfile::Release160Beta0};
 const AltairStoreSnapshotLimits limits{64*1024*1024,128,10000,10000,1024*1024,16*1024*1024,4096,100000};
 const auto initial=initialize_altair_fork_store({binary(dir/"anchor_state.ssz"),std::nullopt},binary(dir/"anchor_block.ssz"),rules);
 Bytes base;for(const auto& step:load(dir/"steps.json"))if(step.contains("block")){auto b=binary(dir/(step.at("block").get<std::string>()+".ssz"));base=Bytes(b.begin()+184,b.end());break;}
 const auto context=altair_fork_block_context(initial.blocks.at(initial.timed.view.anchor).state,initial.blocks.at(initial.timed.view.anchor).header,t.epoch,t.state);
 Temp temp;const auto execution_genesis=Genesis::from_json(Json{{"alloc",Json::object()}});const Bytes genesis=execution_genesis.block.header.hash();auto disk=std::make_unique<ChainStore>(temp/"core");disk->bind_chain(1337,genesis,"altair-qualification");auto reactor=std::make_unique<AltairDurableReactor>(*disk,1337,genesis,rules,limits,initial);
 std::map<uint64_t,std::unique_ptr<Key>> keys;
 for(const auto& [index,v]:context.registry.validators){auto key=std::make_unique<Key>();uint64_t words[4]{index+1,0,0,0};blst_scalar_from_uint64(&key->secret,words);blst_p1 p;blst_sk_to_pk_in_g1(&p,&key->secret);key->public_key.resize(48);blst_p1_compress(key->public_key.data(),&p);check(std::equal(key->public_key.begin(),key->public_key.end(),v.public_key.begin()),"fixture key mapping");key->disk=std::make_unique<ChainStore>(temp/("key-"+std::to_string(index)));key->journal=std::make_unique<BeaconSigningJournal>(*key->disk,1337,genesis,context.genesis_root,key->public_key,slots);keys.emplace(index,std::move(key));}
 check(keys.size()==64,"qualification expects 64 fixture validators");
 for(const auto& [index,v]:context.registry.validators)check(!v.slashed&&v.activation_epoch==0&&v.effective_balance==t.attestation.duties.maximum_effective_balance,"qualification requires equal initial active stake");
 std::vector<Phase0SignedAttestation> pending;size_t blocks=0,signed_votes=0,admitted_votes=0,restarts=0;uint64_t prior_finalized=0;
 for(uint64_t slot=1;slot<=4*slots+1;++slot){
  const auto now=initial.timed.clock.genesis_time_ms+slot*duration;reactor->tick(now);
  for(const auto& vote:pending){check(reactor->vote(vote),"generated vote rejected at slot "+std::to_string(slot));++admitted_votes;}
  const auto before=reactor->current();const auto head=phase0_fork_choice_head(before.state->timed.view,{slots,t.attestation.duties.effective_balance_increment,rules.proposer_score_boost}).head;
  const auto& parent=before.state->blocks.at(head);const auto identity=prepare_altair_proposer_identity(parent.state,parent.header,slot,t.epoch,t.state,t.attestation.duties,limits.replay_slots,limits.sync_trials);check(identity.has_value(),"no proposer");auto& key=*keys.at(identity->index);auto signer=[&](const ConsensusRoot& root){return key.sign(root);};
  const auto reveal=reactor->sign_randao(identity->index,slot,[&]{return now;},before.token,*key.journal,signer);check(reveal.has_value(),"RANDAO failed");auto body=body_with_votes(base,pending,t);std::copy(reveal->begin(),reveal->end(),body.begin());
  const auto block=reactor->sign_proposal(identity->index,slot,body,[&]{return now;},before.token,*key.journal,signer);check(block.has_value(),"proposal failed at slot "+std::to_string(slot));check(reactor->block(*block),"generated block admission failed");++blocks;
  if(slot==2*slots+1){const auto saved=reactor->current();reactor.reset();disk.reset();disk=std::make_unique<ChainStore>(temp/"core");disk->bind_chain(1337,genesis,"altair-qualification");reactor=std::make_unique<AltairDurableReactor>(*disk,1337,genesis,rules,limits,saved.token);check(reactor->current().signed_blocks->size()==saved.signed_blocks->size(),"restart lost signed blocks");for(auto& [index,k]:keys){const auto token=k->journal->commitment();k->journal.reset();k->disk.reset();k->disk=std::make_unique<ChainStore>(temp/("key-"+std::to_string(index)));k->journal=std::make_unique<BeaconSigningJournal>(*k->disk,1337,genesis,context.genesis_root,k->public_key,slots,token);}++restarts;}
  pending.clear();const auto current=reactor->current();
  if(!no_votes)for(auto& [index,k]:keys){if(minority&&index>=keys.size()/3)continue;const auto duty=prepare_altair_attestation_duty(*current.state,rules,index,slot,limits.replay_slots,limits.sync_trials);if(!duty)continue;auto vote=reactor->sign_attestation(index,slot,[&]{return now;},current.token,*k->journal,[&](const ConsensusRoot& root){return k->sign(root);});check(vote.has_value(),"assigned attester did not sign");pending.push_back(std::move(*vote));++signed_votes;}
  check(current.state->timed.view.finalized.epoch>=prior_finalized,"finality regressed");prior_finalized=current.state->timed.view.finalized.epoch;
  if(slot%slots==0)std::cout<<Json{{"slot",slot},{"finalized",prior_finalized},{"justified",current.state->timed.view.justified.epoch}}.dump()<<std::endl;
 }
 const auto final=reactor->current();if(no_votes||minority)check(final.state->timed.view.finalized.epoch==0&&final.state->timed.view.justified.epoch==0,"chain finalized without sufficient attestations");else check(final.state->timed.view.finalized.epoch>=2&&final.state->timed.view.justified.epoch>=3,"generated chain did not finalize");
 const auto final_head=phase0_fork_choice_head(final.state->timed.view,{slots,t.attestation.duties.effective_balance_increment,rules.proposer_score_boost}).head;
 const auto realized=altair_fork_checkpoints(final.state->blocks.at(final_head).state,t.epoch,t.state).realized;
 // At genesis the BeaconState checkpoint uses the zero root, while the
 // fork-choice Store is anchored to the actual genesis block root.
 if(realized.finalized.epoch==0)check(realized.finalized.root==ConsensusRoot{}&&final.state->timed.view.finalized==ConsensusCheckpoint{0,initial.timed.view.anchor},"genesis finality binding changed");
 else check(realized.finalized==final.state->timed.view.finalized,"Store finality differs from realized block state");check(final.signed_blocks->size()==blocks,"generated block archive mismatch");
 if(adversarial){
  // Model a signing key used outside the local anti-slashing journal. These
  // are valid BLS signatures and complete valid transitions, not malformed
  // packets that could be refused before the finalized-ancestry check.
  auto forge=[&](const AltairForkBlock& parent,uint64_t slot,uint8_t graffiti){
   const auto selected=prepare_altair_proposer_identity(parent.state,parent.header,slot,t.epoch,t.state,t.attestation.duties,limits.replay_slots,limits.sync_trials);check(selected.has_value(),"adversarial proposer selection failed");auto& key=*keys.at(selected->index);
   ConsensusRoot epoch_root{};const auto epoch=slot/slots;for(unsigned i=0;i<8;++i)epoch_root[i]=uint8_t(epoch>>(8*i));
   const auto reveal=key.sign(consensus_signing_root(epoch_root,consensus_domain({2,0,0,0},selected->fork_version,selected->genesis_root)));
   auto body=body_with_votes(base,{},t);std::copy(reveal.begin(),reveal.end(),body.begin());body[168]^=graffiti;
   const auto parent_root=consensus_beacon_header_root(parent.header);
   const auto built=prepare_altair_unsigned_block(parent.state,slot,selected->index,parent_root,body,t,limits.replay_slots,limits.sync_trials);check(built.has_value(),"adversarial construction invalid");
   const ConsensusBeaconHeader header{slot,selected->index,parent_root,root_at(built->message_ssz,48),altair_beacon_body_root(body,t.body,t.state.sync_committee_size)};
   const auto signature=key.sign(consensus_signing_root(consensus_beacon_header_root(header),consensus_domain({0,0,0,0},selected->fork_version,selected->genesis_root)));
   Bytes wire(100,0);wire[0]=100;std::copy(signature.begin(),signature.end(),wire.begin()+4);wire.insert(wire.end(),built->message_ssz.begin(),built->message_ssz.end());
   const auto validated=prepare_altair_state_transition(parent.state,wire,t,limits.replay_slots,limits.sync_trials);check(validated&&*validated==built->post_state,"adversarial signature or transition invalid");return wire;
  };
  const auto slot=final.state->blocks.at(final_head).header.slot;
  const auto conflicting=forge(final.state->blocks.at(initial.timed.view.anchor),slot,0x55);
  auto unfinalized=prepare_altair_store_tick(initial,final.state->timed.clock.time_ms,rules,limits.replay_slots,limits.sync_trials);
  check(prepare_altair_received_block(unfinalized,conflicting,rules,limits.replay_slots,limits.sync_trials).has_value(),"conflicting branch rejected even without finalized checkpoint");
  const auto before=reactor->current();
  check(!reactor->block(conflicting)&&reactor->current().token==before.token&&reactor->current().signed_blocks==before.signed_blocks,"valid incompatible branch rewrote finalized history");
  // A competing block descending from the finalized checkpoint must remain
  // admissible: finality must not disable ordinary unfinalized fork handling.
  const auto& tip=final.state->blocks.at(final_head);const auto& parent=final.state->blocks.at(tip.header.parent_root);
  const auto competing=forge(parent,slot,0xaa);check(reactor->block(competing),"valid unfinalized competing block prohibited");
  const auto accepted=reactor->current();check(accepted.signed_blocks->size()==before.signed_blocks->size()+1&&accepted.state->timed.view.finalized==before.state->timed.view.finalized,"competing tip changed finalized checkpoint");
  reactor.reset();disk.reset();disk=std::make_unique<ChainStore>(temp/"core");disk->bind_chain(1337,genesis,"altair-qualification");reactor=std::make_unique<AltairDurableReactor>(*disk,1337,genesis,rules,limits,accepted.token);
  check(!reactor->block(conflicting)&&reactor->current().token==accepted.token&&reactor->current().signed_blocks->size()==accepted.signed_blocks->size(),"reopened coordinator accepted finalized-history rewrite");
  // Encode the actual two conflicting SignedBeaconHeaders as proposer-slashing
  // evidence, then include them through the normal full block transition.
  auto signed_header=[&](const Bytes& wire){Bytes out(wire.begin()+100,wire.begin()+180);const auto body_root=altair_beacon_body_root(std::span<const uint8_t>(wire).subspan(184),t.body,t.state.sync_committee_size);out.insert(out.end(),body_root.begin(),body_root.end());out.insert(out.end(),wire.begin()+4,wire.begin()+100);check(out.size()==208,"signed header encoding");return out;};
  Bytes proof=signed_header(*final.signed_blocks->at(final_head));const auto second=signed_header(competing);proof.insert(proof.end(),second.begin(),second.end());
  const auto offender=little(competing,108,8),next_slot=slot+1,now=initial.timed.clock.genesis_time_ms+next_slot*duration;
  reactor->tick(now);const auto slash_before=reactor->current();const auto slash_head=phase0_fork_choice_head(slash_before.state->timed.view,{slots,t.attestation.duties.effective_balance_increment,rules.proposer_score_boost}).head;
  const auto& slash_parent=slash_before.state->blocks.at(slash_head);const auto selected=prepare_altair_proposer_identity(slash_parent.state,slash_parent.header,next_slot,t.epoch,t.state,t.attestation.duties,limits.replay_slots,limits.sync_trials);check(selected.has_value(),"missing evidence proposer");auto& reporter=*keys.at(selected->index);unsigned report_calls=0;auto report_signer=[&](const ConsensusRoot& root){++report_calls;return reporter.sign(root);};
  const auto reveal=reactor->sign_randao(selected->index,next_slot,[&]{return now;},slash_before.token,*reporter.journal,report_signer);check(reveal.has_value(),"evidence RANDAO failed");
  auto clean=body_with_votes(base,{},t);std::copy(reveal->begin(),reveal->end(),clean.begin());
  auto attach=[&](const Bytes& evidence){auto body=clean;const auto fixed=body.size();body.insert(body.end(),evidence.begin(),evidence.end());put32(body,200,fixed);for(size_t at=204;at<=216;at+=4)put32(body,at,body.size());return body;};
  const auto control=prepare_altair_unsigned_block(slash_parent.state,next_slot,selected->index,slash_head,clean,t,limits.replay_slots,limits.sync_trials);check(control.has_value(),"slashing control block invalid");
  const ConsensusBeaconHeader control_header{next_slot,selected->index,slash_head,root_at(control->message_ssz,48),altair_beacon_body_root(clean,t.body,t.state.sync_committee_size)};
  const auto control_context=altair_fork_block_context(control->post_state,control_header,t.epoch,t.state);const auto& original=control_context.registry.validators.at(offender);check(!original.slashed,"offender already slashed");
  auto refuse_proof=[&](const Bytes& evidence){const auto token=reporter.journal->commitment();const auto calls=report_calls;check(!reactor->sign_proposal(selected->index,next_slot,attach(evidence),[&]{return now;},slash_before.token,*reporter.journal,report_signer)&&report_calls==calls&&reporter.journal->commitment()==token&&reactor->current().token==slash_before.token,"bad proof changed state or reached signer");};
  auto damaged=proof;damaged[112]^=1;refuse_proof(damaged);auto repeated=proof;repeated.insert(repeated.end(),proof.begin(),proof.end());refuse_proof(repeated);
  const auto signed_slashing=reactor->sign_proposal(selected->index,next_slot,attach(proof),[&]{return now;},slash_before.token,*reporter.journal,report_signer);check(signed_slashing&&reactor->block(*signed_slashing),"valid double-sign evidence block rejected");
  const auto slash_after=reactor->current();const ConsensusBeaconHeader slash_header{next_slot,selected->index,slash_head,root_at(*signed_slashing,148),altair_beacon_body_root(std::span<const uint8_t>(*signed_slashing).subspan(184),t.body,t.state.sync_committee_size)};const auto slash_root=consensus_beacon_header_root(slash_header);
  const auto punished=altair_fork_block_context(slash_after.state->blocks.at(slash_root).state,slash_header,t.epoch,t.state);const auto& offender_after=punished.registry.validators.at(offender);
  const auto penalty=original.effective_balance/t.epoch.slashing.minimum_penalty_quotient,reward=original.effective_balance/t.epoch.slashing.whistleblower_reward_quotient;
  check(offender_after.slashed&&offender_after.exit_epoch<UINT64_MAX&&offender_after.withdrawable_epoch>=next_slot/slots+t.epoch.slashing.epochs_per_slashings_vector,"slashing lifecycle not applied");
  for(const auto& [index,v]:control_context.registry.validators){__int128 expected=v.balance;if(index==offender)expected-=penalty;if(index==selected->index)expected+=reward;check(static_cast<__int128>(punished.registry.validators.at(index).balance)==expected,"slashing balance delta differs from configured economics");}
  const auto bucket=(next_slot/slots)%t.epoch.slashing.epochs_per_slashings_vector;check(punished.registry.slashings.at(bucket)==control_context.registry.slashings.at(bucket)+original.effective_balance,"slashing vector did not record effective stake");
  check(slash_after.state->timed.view.finalized==accepted.state->timed.view.finalized,"slashing rewrote finalized checkpoint");
  reactor.reset();disk.reset();disk=std::make_unique<ChainStore>(temp/"core");disk->bind_chain(1337,genesis,"altair-qualification");reactor=std::make_unique<AltairDurableReactor>(*disk,1337,genesis,rules,limits,slash_after.token);
  const auto persisted=altair_fork_block_context(reactor->current().state->blocks.at(slash_root).state,slash_header,t.epoch,t.state);check(persisted.registry.validators.at(offender)==offender_after,"slashing state lost after database reopen");
  bool continued=false;
  for(uint64_t follow=next_slot+1;follow<=next_slot+slots&&!continued;++follow){
   const auto follow_time=initial.timed.clock.genesis_time_ms+follow*duration;reactor->tick(follow_time);const auto state=reactor->current();
   const auto head=phase0_fork_choice_head(state.state->timed.view,{slots,t.attestation.duties.effective_balance_increment,rules.proposer_score_boost}).head;const auto& parent=state.state->blocks.at(head);
   const auto identity=prepare_altair_proposer_identity(parent.state,parent.header,follow,t.epoch,t.state,t.attestation.duties,limits.replay_slots,limits.sync_trials);if(!identity)continue;
   auto& key=*keys.at(identity->index);unsigned calls=0;auto signer=[&](const ConsensusRoot& root){++calls;return key.sign(root);};
   const auto randao=reactor->sign_randao(identity->index,follow,[&]{return follow_time;},state.token,*key.journal,signer);check(randao.has_value(),"post-slashing RANDAO failed");auto body=body_with_votes(base,{},t);std::copy(randao->begin(),randao->end(),body.begin());
   const auto expected=prepare_altair_unsigned_block(parent.state,follow,identity->index,head,body,t,limits.replay_slots,limits.sync_trials);check(expected.has_value(),"post-slashing clean transition failed");
   auto replay=body;replay.insert(replay.end(),proof.begin(),proof.end());put32(replay,200,body.size());for(size_t at=204;at<=216;at+=4)put32(replay,at,replay.size());const auto guard=key.journal->commitment();const auto before_calls=calls;
   check(!reactor->sign_proposal(identity->index,follow,replay,[&]{return follow_time;},state.token,*key.journal,signer)&&key.journal->commitment()==guard&&calls==before_calls&&reactor->current().token==state.token,"replayed slashing proof reached signer or changed state");
   const auto next=reactor->sign_proposal(identity->index,follow,body,[&]{return follow_time;},state.token,*key.journal,signer);check(next&&reactor->block(*next),"chain did not continue after slashing/replay refusal");
   const ConsensusBeaconHeader h{follow,identity->index,head,root_at(expected->message_ssz,48),altair_beacon_body_root(body,t.body,t.state.sync_committee_size)};
   check(reactor->current().state->blocks.at(consensus_beacon_header_root(h)).state==expected->post_state&&reactor->current().state->timed.view.finalized==accepted.state->timed.view.finalized,"post-slashing continuation state mismatch");continued=true;
  }
  check(continued,"no post-slashing proposer within bounded slots");
  std::cout<<Json{{"scenario","generated-double-proposal-slashing"},{"offender",offender},{"reporter",selected->index},{"penalty",penalty},{"reporterReward",reward},{"allBalanceDeltasVerified",true},{"laterProofReplayRefused",true},{"chainContinuedAfterSlashing",continued},{"invalidSignatureRefused",true},{"duplicateProofRefused",true},{"exitQueued",true},{"persistedAfterReopen",true}}.dump()<<std::endl;
  std::cout<<Json{{"scenario","signed-branch-finality-attack"},{"independentlyValidTransitions",2},{"incompatibleBranchAcceptedWithoutFinality",true},{"finalizedRewriteRefused",true},{"refusalAfterDatabaseReopen",true},{"unfinalizedCompetingBlockAccepted",true},{"attackerBypassedLocalSigningJournal",true},{"finalizedEpoch",accepted.state->timed.view.finalized.epoch}}.dump()<<std::endl;
 }
 if(p2p){
  const auto& anchor=initial.blocks.at(initial.timed.view.anchor);const auto fork=consensus_fork_data_root(context.fork.current_version,context.genesis_root);
  const auto identity=hex(altair_consensus_identity(rules,{initial.timed.view.anchor,anchor.header.state_root,context.genesis_root,context.fork.current_version},1337,genesis));
  const AltairWireProfile profile{"0x539",hex(genesis),hex(Bytes(fork.begin(),fork.begin()+4)),2*t.maximum_block_bytes+1024,t.maximum_block_bytes,512,40000};
  ChainStore remote_disk(temp/"remote-consensus");remote_disk.bind_chain(1337,genesis,"altair-qualification");AltairDurableReactor remote(remote_disk,1337,genesis,rules,limits,initial);remote.tick(final.state->timed.clock.time_ms);
  auto endpoint=[&](auto& r){return std::make_shared<AltairRpcService>(r,profile,4,2*t.maximum_block_bytes,10s,[](const auto& check){if(check)check();},[]{return true;},identity,true);};
  auto source_service=endpoint(*reactor),remote_service=endpoint(remote);
  Blockchain source_execution(temp/"source-execution",execution_genesis),remote_execution(temp/"remote-execution",execution_genesis);
  Rpc source_rpc(source_execution,"fixture-token",AdmissionBudget::Clock::now,RpcSimulationLimits{},source_service),remote_rpc(remote_execution,"fixture-token",AdmissionBudget::Clock::now,RpcSimulationLimits{},remote_service);
  HttpServer source_http(source_rpc,"127.0.0.1",0),remote_http(remote_rpc,"127.0.0.1",0);RunningHttp source_running(source_http),remote_running(remote_http);
  std::unique_ptr<Process> sender,receiver;
  auto connect=[&](bool drop_reply){
   Env env{{"NANO_NETWORK_TOKEN","fixture-token"},{"NANO_TEST_NETWORK_TRACE","1"}};if(drop_reply)env["NANO_TEST_DROP_ALTAIR_REPLY_ONCE"]="1";
   receiver=std::make_unique<Process>(std::vector<std::string>{argv[2],std::to_string(remote_http.port()),(temp/"receiver-key").string(),"/ip4/127.0.0.1/tcp/0"},env);
   const auto address=receiver->event("listen").at("address").get<std::string>();receiver->event("altair-ingress-ready");
   sender=std::make_unique<Process>(std::vector<std::string>{argv[2],std::to_string(source_http.port()),(temp/"sender-key").string(),"/ip4/127.0.0.1/tcp/0",address},Env{{"NANO_NETWORK_TOKEN","fixture-token"},{"NANO_TEST_NETWORK_TRACE","1"}});sender->event("altair-ingress-ready");
  };
  connect(p2p_restart);size_t blocks_before_restart=0;
  if(p2p_restart){
   until([&]{sender->pump(0);receiver->pump(0);check(sender->running()&&receiver->running(),"daemon exited before injected failure");return remote.current().signed_blocks->size()>=8;},"receiver persisted prefix before SIGKILL",50);
   receiver->event("test-altair-reply-dropped");
   receiver->signal(SIGKILL);check(receiver->wait(10).code==-SIGKILL,"receiver did not terminate by SIGKILL");check(sender->stop().code==0,"sender shutdown for reconnect failed");
   receiver.reset();sender.reset();const auto persisted=remote.current();blocks_before_restart=persisted.signed_blocks->size();check(blocks_before_restart>=8&&blocks_before_restart<blocks,"failure did not interrupt partial catchup");
   std::this_thread::sleep_for(500ms);check(remote.current().token==persisted.token,"core changed after transport shutdown");
   // Transport replay starts with fresh cursors and the same P2P identities.
   // Existing durable blocks must survive and duplicate delivery must be safe.
   connect(false);const auto resumed=remote.current();check(resumed.signed_blocks->size()>=blocks_before_restart,"transport restart lost persisted blocks");
   for(const auto& [root,bytes]:*persisted.signed_blocks)check(*resumed.signed_blocks->at(root)==*bytes,"transport restart changed persisted prefix");
   std::cout<<Json{{"transportRestarted",true},{"killedReceiver",true},{"lostReplyObserved",true},{"persistedPrefix",blocks_before_restart}}.dump()<<std::endl;
  }
  size_t observed_blocks=0;
  until([&]{
   sender->pump(0);receiver->pump(0);check(sender->running()&&receiver->running(),"P2P daemon exited during catchup");
   const auto state=remote.current();
   if(state.signed_blocks->size()!=observed_blocks){observed_blocks=state.signed_blocks->size();std::cout<<Json{{"remoteBlocks",observed_blocks},{"remoteFinalizedEpoch",state.state->timed.view.finalized.epoch}}.dump()<<std::endl;}
   return state.signed_blocks->size()==blocks&&state.state->timed.view.finalized==final.state->timed.view.finalized;
  },"remote received generated blocks and finality",160);
  const auto received=remote.current();const auto remote_head=phase0_fork_choice_head(received.state->timed.view,{slots,t.attestation.duties.effective_balance_increment,rules.proposer_score_boost}).head;
  check(remote_head==final_head&&received.state->timed.view.justified==final.state->timed.view.justified,"remote generated-chain head/justification mismatch");
  for(const auto& [root,bytes]:*final.signed_blocks)check(*received.signed_blocks->at(root)==*bytes,"P2P generated block bytes differ");
  check(received.state->blocks.at(remote_head).state==final.state->blocks.at(final_head).state,"remote finalized chain state differs");
  check(sender->stop().code==0&&receiver->stop().code==0,"generated-chain daemon shutdown failed");
  source_service->stop();remote_service->stop();source_service->join();remote_service->join();
  const auto stopped=remote.current();
  AltairDurableReactor restored(remote_disk,1337,genesis,rules,limits,stopped.token);
  check(restored.current().state->timed.view.finalized==final.state->timed.view.finalized&&restored.current().signed_blocks->size()==blocks,"remote checkpoint recovery lost finality");
  std::cout<<Json{{"scenario","generated-chain-libp2p-catchup"},{"realDaemons",2},{"transportRestarted",p2p_restart},{"persistedPrefixBeforeKill",blocks_before_restart},{"remoteBlocks",blocks},{"remoteFinalizedEpoch",received.state->timed.view.finalized.epoch},{"exactBlockBytes",true},{"exactHeadState",true},{"remoteCheckpointRestored",true},{"fixtureClock",true},{"autonomousDistributedValidators",false},{"executionBound",false}}.dump()<<std::endl;
 }
 std::cout<<Json{{"scenario","locally-generated-altair-finality"},{"withoutAttestations",no_votes},{"minorityAttesters",minority},{"attestingKeys",no_votes?size_t(0):minority?keys.size()/3:keys.size()},{"validators",keys.size()},{"blocks",blocks},{"signedVotes",signed_votes},{"admittedVotes",admitted_votes},{"restarts",restarts},{"finalizedEpoch",final.state->timed.view.finalized.epoch},{"justifiedEpoch",final.state->timed.view.justified.epoch},{"distributedNetwork",false},{"executionBound",false},{"syncCommitteeParticipation",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
