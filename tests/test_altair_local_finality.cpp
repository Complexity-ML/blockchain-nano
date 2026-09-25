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
 const bool no_votes=argc==2&&std::string(argv[1])=="without-votes",minority=argc==2&&std::string(argv[1])=="minority-votes";const bool p2p=argc==3&&std::string(argv[1])=="p2p";check(argc==1||no_votes||minority||p2p,"unexpected scenario argument");
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
  Process receiver({argv[2],std::to_string(remote_http.port()),(temp/"receiver-key").string(),"/ip4/127.0.0.1/tcp/0"},{{"NANO_NETWORK_TOKEN","fixture-token"},{"NANO_TEST_NETWORK_TRACE","1"}});
  const auto address=receiver.event("listen").at("address").get<std::string>();receiver.event("altair-ingress-ready");
  Process sender({argv[2],std::to_string(source_http.port()),(temp/"sender-key").string(),"/ip4/127.0.0.1/tcp/0",address},{{"NANO_NETWORK_TOKEN","fixture-token"},{"NANO_TEST_NETWORK_TRACE","1"}});sender.event("altair-ingress-ready");
  size_t observed_blocks=0;
  until([&]{
   sender.pump(0);receiver.pump(0);check(sender.running()&&receiver.running(),"P2P daemon exited during catchup");
   const auto state=remote.current();
   if(state.signed_blocks->size()!=observed_blocks){observed_blocks=state.signed_blocks->size();std::cout<<Json{{"remoteBlocks",observed_blocks},{"remoteFinalizedEpoch",state.state->timed.view.finalized.epoch}}.dump()<<std::endl;}
   return state.signed_blocks->size()==blocks&&state.state->timed.view.finalized==final.state->timed.view.finalized;
  },"remote received generated blocks and finality",160);
  const auto received=remote.current();const auto remote_head=phase0_fork_choice_head(received.state->timed.view,{slots,t.attestation.duties.effective_balance_increment,rules.proposer_score_boost}).head;
  check(remote_head==final_head&&received.state->timed.view.justified==final.state->timed.view.justified,"remote generated-chain head/justification mismatch");
  for(const auto& [root,bytes]:*final.signed_blocks)check(*received.signed_blocks->at(root)==*bytes,"P2P generated block bytes differ");
  check(received.state->blocks.at(remote_head).state==final.state->blocks.at(final_head).state,"remote finalized chain state differs");
  check(sender.stop().code==0&&receiver.stop().code==0,"generated-chain daemon shutdown failed");
  source_service->stop();remote_service->stop();source_service->join();remote_service->join();
  const auto stopped=remote.current();
  AltairDurableReactor restored(remote_disk,1337,genesis,rules,limits,stopped.token);
  check(restored.current().state->timed.view.finalized==final.state->timed.view.finalized&&restored.current().signed_blocks->size()==blocks,"remote checkpoint recovery lost finality");
  std::cout<<Json{{"scenario","generated-chain-libp2p-catchup"},{"realDaemons",2},{"remoteBlocks",blocks},{"remoteFinalizedEpoch",received.state->timed.view.finalized.epoch},{"exactBlockBytes",true},{"exactHeadState",true},{"remoteCheckpointRestored",true},{"fixtureClock",true},{"autonomousDistributedValidators",false},{"executionBound",false}}.dump()<<std::endl;
 }
 std::cout<<Json{{"scenario","locally-generated-altair-finality"},{"withoutAttestations",no_votes},{"minorityAttesters",minority},{"attestingKeys",no_votes?size_t(0):minority?keys.size()/3:keys.size()},{"validators",keys.size()},{"blocks",blocks},{"signedVotes",signed_votes},{"admittedVotes",admitted_votes},{"restarts",restarts},{"finalizedEpoch",final.state->timed.view.finalized.epoch},{"justifiedEpoch",final.state->timed.view.justified.epoch},{"distributedNetwork",false},{"executionBound",false},{"syncCommitteeParticipation",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
