#include "support/durable_owned_fixture.hpp"
#include "../network/capella_consensus_service.hpp"
#include "../network/capella_block_delivery.hpp"
#include <atomic>
#include <future>
#include <thread>

namespace {
using namespace nano;
using namespace nano::test::durable_owned;
using Clock=std::chrono::steady_clock;
std::string h(const ConsensusRoot& r){return hex(Bytes(r.begin(),r.end()));}
Json token(const ForkCheckpointToken& t){return Json::array({t.sequence,hex(t.digest)});}
Json commitment(std::string_view s){return {{"bytes",s.size()},{"sha256",hex(sha256(Bytes(s.begin(),s.end())))}};}
struct Pair {
 const Fixture& f;Temp directory;
 std::unique_ptr<Blockchain> owner;
 std::unique_ptr<ChainStore> witness;
 std::unique_ptr<CapellaDurableReactor> node;
 explicit Pair(const Fixture& x):f(x){open();}
 ~Pair(){close();}
 void open(){
  owner=std::make_unique<Blockchain>(directory.path/"primary",f.genesis);
  witness=std::make_unique<ChainStore>(directory.path/"witness");
  node=std::make_unique<CapellaDurableReactor>(*owner,f.rules,f.limits,f.initial,checkpoint,witness.get());
 }
 void close(){node.reset();witness.reset();owner.reset();}
 ForkCheckpointRecord primary()const{auto p=owner->store().fork_checkpoint(f.genesis.chain_id,f.genesis.block.header.hash(),f.limits.envelope_bytes);check(bool(p),"primary checkpoint");return *p;}
 ForkCheckpointRecord witnessed()const{auto w=witness->fork_checkpoint(f.genesis.chain_id,f.genesis.block.header.hash(),2048+8*f.limits.envelope_bytes);check(bool(w),"independent witness");return *w;}
 Json finalized()const{const auto c=node->current();return {{"epoch",c.state->timed.view.finalized.epoch},{"root",h(c.state->timed.view.finalized.root)}};}
 Json archive()const{
  Json a=Json::array();const auto c=node->current();
  for(const auto& [id,_]:c.state->blocks){const auto wire=node->signed_block(id,checkpoint);a.push_back(Json::array({h(id),wire?Json(hex(*wire)):Json(nullptr)}));}
  return a;
 }
 Json seal()const{
  const auto p=primary(),w=witnessed();const auto c=node->current();
  return {{"primary",commitment(p.snapshot)},{"primaryToken",token(p.token)},
   {"witness",commitment(w.snapshot)},{"witnessToken",token(w.token)},
   {"snapshot",commitment(capella_store_snapshot(*c.state,f.rules,f.limits,checkpoint))},
   {"anchor",h(c.state->timed.view.anchor)},{"finalized",finalized()},
   {"tip",hex(owner->tip().header.hash())},{"executionRoot",hex(owner->state().root())},{"archive",archive()}};
 }
 void reopen(){
  const auto expected=seal();const auto p=primary().token,w=witnessed().token;
  close();owner=std::make_unique<Blockchain>(directory.path/"primary",f.genesis);
  witness=std::make_unique<ChainStore>(directory.path/"witness");
  check(primary().token==p&&witnessed().token==w,"both externally retained commitments before recovery");
  node=std::make_unique<CapellaDurableReactor>(*owner,f.rules,f.limits,*witness,checkpoint,p);
  check(seal()==expected,"fresh owner/witness/reactor preserve all exact commitments");
 }
 void at(uint64_t slot){node->tick(f.initial.timed.clock.genesis_time_ms+slot*f.rules.slot_duration_ms,checkpoint);}
};
void equal(Pair& a,Pair& b){
 check(a.primary().snapshot==b.primary().snapshot,"exact canonical primary snapshot");
 check(a.witnessed().snapshot==b.witnessed().snapshot,"exact canonical witness snapshot");
 check(capella_store_snapshot(*a.node->current().state,a.f.rules,a.f.limits,checkpoint)==capella_store_snapshot(*b.node->current().state,b.f.rules,b.f.limits,checkpoint),"exact canonical consensus snapshot");
 check(a.seal()==b.seal(),"canonical anchors/finalized/execution/archive/tokens");
}
ConsensusRoot root_of(const Fixture& f,const Bytes& wire){
 const auto& t=f.rules.transition;
 return consensus_beacon_header_root({test_little(wire,100),test_little(wire,108),test_root(wire,116),test_root(wire,148),capella_beacon_body_root(std::span<const uint8_t>(wire).subspan(184),t.consensus.body,t.consensus.state.sync_committee_size,t.execution,t.withdrawals.maximum_per_payload,t.maximum_bls_changes,t.consensus.maximum_block_bytes,checkpoint)});
}
Bytes child(Pair& producer,uint64_t slot){
 producer.at(slot);const auto& f=producer.f;const auto& t=f.rules.transition;
 for(size_t tries=0;!producer.node->prepare_current_duties(slot,checkpoint);++tries)check(tries<8,"finite public preparation");
 const auto index=producer.node->current_proposer(slot,checkpoint);check(bool(index),"public synthetic proposer");
 const auto c=producer.node->current();const auto parent=capella_fork_head(*c.state,f.rules,checkpoint);
 const auto ctx=capella_fork_block_context(parent->state,parent->header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state,checkpoint);
 const auto epoch=slot/t.consensus.epoch.ffg.slots_per_epoch;
 const auto version=epoch<ctx.fork.epoch?ctx.fork.previous_version:ctx.fork.current_version;
 TestKey key(*index);check(key.public_key==Bytes(ctx.registry.validators.at(*index).public_key.begin(),ctx.registry.validators.at(*index).public_key.end()),"synthetic key actually selected");
 ConsensusRoot epoch_root{};for(size_t i=0;i<8;++i)epoch_root[i]=uint8_t(epoch>>(8*i));
 const auto reveal=key.sign(consensus_signing_root(epoch_root,consensus_domain({2,0,0,0},version,ctx.genesis_root)));
 const auto body=producer.node->prepare_local_body(*index,slot,reveal,parent->execution_block.header.coinbase,c.token,{},1,64,checkpoint);
 check(bool(body),"normal full public body");const auto parent_root=consensus_beacon_header_root(parent->header);
 auto history=[&](const Int& n){return capella_execution_history(*c.state,parent_root,n,checkpoint);};
 auto candidate=prepare_capella_unsigned_block(parent->state,slot,*index,parent_root,*body,t,parent->execution_block.header,parent->execution_state,history,f.limits.replay_slots,f.limits.sync_trials,checkpoint);
 check(bool(candidate),"independent complete child transition");const auto& m=candidate->message_ssz;
 const auto body_root=capella_beacon_body_root(std::span<const uint8_t>(m).subspan(84),t.consensus.body,t.consensus.state.sync_committee_size,t.execution,t.withdrawals.maximum_per_payload,t.maximum_bls_changes,t.consensus.maximum_block_bytes,checkpoint);
 const ConsensusBeaconHeader header{test_little(m,0),test_little(m,8),test_root(m,16),test_root(m,48),body_root};
 const auto signature=key.sign(consensus_signing_root(consensus_beacon_header_root(header),consensus_domain({0,0,0,0},version,ctx.genesis_root)));
 Bytes wire(100);test_put(wire,0,100,4);std::copy(signature.begin(),signature.end(),wire.begin()+4);wire.insert(wire.end(),m.begin(),m.end());
 bool accepted=false;for(size_t i=0;i<8&&!accepted;++i){const auto status=producer.node->block_admission(wire,checkpoint);check(status!=CapellaBlockAdmission::rejected,"normal producer child rejected");accepted=status==CapellaBlockAdmission::accepted;}
 check(accepted,"finite producer admission");return wire;
}
struct Endpoint {
 Pair& p;InboundBudget budget;std::unique_ptr<CapellaConsensusService> service;
 std::atomic<size_t> quanta{0},completed{0},duplicates{0};size_t submitted=0;bool closed=false;
 explicit Endpoint(Pair& pair):p(pair){
  const auto block=p.f.rules.transition.consensus.maximum_block_bytes;
  service=std::make_unique<CapellaConsensusService>(*p.node,budget,2*block+8192,block,65536,65536,4,2*block+8192,[this](const auto& check_cancelled){checkpoint();check(++quanta<=64,"bounded real worker quanta");check_cancelled();});
 }
 ~Endpoint(){if(service){service->stop();service->join();}}
 Json frame(const Bytes& b)const{auto e=p.node->network_identity();e["kind"]="block";e["ssz"]=hex(b);return e;}
 std::string session(){return service->receipts().at("session").get<std::string>();}
 CapellaConsensusService::Decision send(const Bytes& b){
  auto promise=std::make_shared<std::promise<CapellaConsensusService::Decision>>();auto future=promise->get_future();auto once=std::make_shared<std::atomic<size_t>>(0);
  check(service->submit(frame(b).dump(),std::make_shared<std::atomic<bool>>(false),[this,promise,once](auto d){if(once->fetch_add(1)==0){++completed;promise->set_value(d);}else ++duplicates;}),"honest job queued");++submitted;
  check(future.wait_for(std::chrono::seconds(10))==std::future_status::ready,"bounded real worker completion");return future.get();
 }
 Json ticket(const Bytes& b,uint64_t sequence){
  const auto id=session();auto value=service->submit_receipt(id,sequence,Json(frame(b).dump()));const auto end=Clock::now()+std::chrono::seconds(10);
  size_t polls=0;while(value.at("state")!="completed"){
   checkpoint();check(++polls<=10000&&Clock::now()<end,"bounded receipt polling");
   // Observation backoff only; the synthetic consensus clock never advances here.
   std::this_thread::sleep_for(std::chrono::milliseconds(1));value=service->receipt(id,sequence);
  }
  check(value.at("decision")=="accepted","real receipt accepted after actual commit");return value;
 }
 void close(){
  service->stop();service->join();closed=true;
  check(completed==submitted&&duplicates==0,"all own callbacks once before DB close");
  const auto ledger=service->receipts();const auto id=ledger.at("session").get<std::string>();
  for(const auto& job:ledger.at("jobs")){
   check(job.at("state")=="completed","no unfinished receipt after worker join");
   service->receipt(id,u64(quantity(job.at("sequence"))),false,true);
  }
  check(service->receipts().at("jobs").empty(),"completed receipt reservations explicitly retired");
  check(!service->submit(frame(p.f.wire).dump(),std::make_shared<std::atomic<bool>>(false),[](auto){}),"stopped worker admits no valid job");
 }
};
void canonical(Pair& p,const std::vector<Bytes>& wires){
 p.at(19);for(const auto& wire:wires){bool accepted=false;for(size_t i=0;i<8&&!accepted;++i){auto d=p.node->block_admission(wire,checkpoint);check(d!=CapellaBlockAdmission::rejected,"canonical signed sequence rejected");accepted=d==CapellaBlockAdmission::accepted;}check(accepted,"finite canonical import");}
}
void emit(const char* scenario,Json fields){
 checkpoint();fields["scenario"]=scenario;fields["result"]="PASS";
 fields["privateInputsRead"]=false;fields["runtimeDutiesAuthorized"]=false;
 fields["networkTransport"]=false;fields["productionQualified"]=false;
 fields["newTestOnlySignaturesCreated"]=test_signatures;
 std::cout<<fields.dump()<<std::endl;
}
void orphan_retry(const Fixture& f,const std::vector<Bytes>& wires){
 test_stage="orphan-before-parent";Pair oracle(f);canonical(oracle,wires);Pair receiver(f);receiver.at(19);
 const auto before=receiver.seal(),finalized=receiver.finalized();size_t quanta=0;
 {
  Endpoint endpoint(receiver);
  check(endpoint.send(wires[1])==CapellaConsensusService::Decision::rejected,"orphan explicitly rejected, not deferred or accepted");
  check(receiver.seal()==before,"orphan rejection preserves exact P/W state and archive");
  check(endpoint.send(wires[0])==CapellaConsensusService::Decision::accepted,"parent accepted");
  check(endpoint.send(wires[1])==CapellaConsensusService::Decision::accepted,"honest retry accepted after parent");
  const auto duplicate=receiver.seal();check(endpoint.send(wires[1])==CapellaConsensusService::Decision::accepted&&receiver.seal()==duplicate,"Capella duplicate ACK without another checkpoint");
  check(endpoint.send(wires[2])==CapellaConsensusService::Decision::accepted,"next honest child accepted");
  endpoint.close();quanta=endpoint.quanta;
 }
 equal(receiver,oracle);receiver.reopen();equal(receiver,oracle);
 check(receiver.finalized()==finalized,"initial finalized epoch/root unchanged");
 emit("capella-sync-orphan-retry",{{"orphanRejectedBeforeParent",true},{"rejectedPairUnchanged",true},{"canonicalSnapshotAndWitnessEqual",true},{"signedArchiveAndExecutionEqual",true},{"duplicateCheckpointStable",true},{"workerCallbacks",5},{"workerQuanta",quanta},{"workersJoined",true},{"receiptReservationsRetired",true},{"finalizedCheckpointUnchanged",receiver.finalized()==finalized},{"finalizedAdvanceQualified",false}});
}
void deferred_reopen(const Fixture& f,const std::vector<Bytes>& wires){
 test_stage="deferred-before-reopen";Pair oracle(f);canonical(oracle,wires);Pair receiver(f);receiver.at(19);
 const auto finalized=receiver.finalized();std::string old_session;
 {
  Endpoint first(receiver);old_session=first.session();first.ticket(wires[0],1);first.close();
 }
 const auto interrupted=receiver.seal();
 check(receiver.node->block_admission(wires[1],checkpoint)==CapellaBlockAdmission::deferred,"actual incomplete child preparation is deferred");
 check(receiver.seal()==interrupted,"deferred has no durable publication");receiver.reopen();
 std::string committed_session;
 {
  Endpoint resumed(receiver);const auto next=resumed.session();check(next!=old_session,"new process-local session after P/W reopen");
  bool unknown=false;try{resumed.service->receipt(old_session,1);}catch(const std::invalid_argument& e){unknown=std::string(e.what()).find("outcome unknown")!=std::string::npos;}
  check(unknown&&resumed.service->receipt(next,1).at("state")=="unknown","old session is unknown outcome, no fabricated persistent receipt");
  // The test observes the server ledger, but deliberately discards its ACK at
  // the client boundary before restarting the service. State transition is real.
  resumed.ticket(wires[1],1);committed_session=next;resumed.close();
 }
 const auto committed=receiver.seal();receiver.reopen();size_t exports=0,sends=0,done=0;
 {
  Endpoint reconnected(receiver);check(reconnected.session()!=committed_session,"new session after lost ACK");
  reconnected.ticket(wires[1],1);check(receiver.seal()==committed,"retry of committed child does not publish twice");
  asio::io_context io;InboundBudget ingress_budget;const auto identity=oracle.node->network_identity();
  const auto block=f.rules.transition.consensus.maximum_block_bytes;
  auto ingress=std::make_shared<AltairMessageIngress>(AltairWireProfile{identity.at("chainId").get<std::string>(),identity.at("genesisHash").get<std::string>(),identity.at("forkDigest").get<std::string>(),2*block+8192,block,65536,65536,identity.at("consensusIdentity").get<std::string>()},ingress_budget);
  auto knowledge=std::make_shared<AltairRelayProgress>(8);
  auto core=[&](std::string method,Json args,Reply reply){checkpoint();check(method=="nano_capellaExport"&&args.size()==3&&args[0]=="public-test-token"&&args[1]=="block"&&++exports<=3,"bounded actual local export");const auto raw=unhex(args[2].get<std::string>());check(raw.size()==32,"requested export root");ConsensusRoot root{};std::copy(raw.begin(),raw.end(),root.begin());reply(oracle.node->export_message("block",root,2*block+8192,{},checkpoint));};
  auto send=[&](Json frame,Reply reply){checkpoint();check(++sends<=3,"bounded real relay submissions");
   auto once=std::make_shared<std::atomic<size_t>>(0);
   const bool queued=reconnected.service->submit(frame.dump(),std::make_shared<std::atomic<bool>>(false),[&,reply,once](auto decision){if(once->fetch_add(1)==0){++reconnected.completed;reply(Json{{"accepted",decision==CapellaConsensusService::Decision::accepted}});}else ++reconnected.duplicates;});
   if(queued)++reconnected.submitted;else reply(nullptr);
  };
  Json result;const auto anchor=f.initial.timed.view.anchor;const auto target=root_of(f,wires[2]);
  auto relay=std::make_shared<CapellaBlockDelivery>(io,core,send,"public-test-token",h(anchor),h(target),ingress,knowledge,8,2*block+8192,[&](Json r){++done;result=std::move(r);});
  relay->start(std::chrono::seconds(10));io.run();
  check(done==1&&result==Json{{"accepted",true}}&&exports==3&&sends==3&&knowledge->acknowledged(h(target))&&knowledge->retained()<=8,"new relay session reconstructs acknowledgements from actual commits");
  reconnected.close();
 }
 equal(receiver,oracle);receiver.reopen();equal(receiver,oracle);
 check(receiver.finalized()==finalized,"initial finalized epoch/root unchanged");
 emit("capella-sync-deferred-session-reopen",{{"actualDeferredBeforeReopen",true},{"deferredPairUnchanged",true},{"freshOwnerWitnessReactor",true},{"oldSessionOutcomeUnknown",true},{"honestResubmission",true},{"lostCommitAckRetried",true},{"duplicateCheckpointStable",true},{"newRelayKnowledge",true},{"realExports",exports},{"realRelaySubmissions",sends},{"canonicalSnapshotAndWitnessEqual",true},{"signedArchiveAndExecutionEqual",true},{"workersJoined",true},{"receiptReservationsRetired",true},{"finalizedCheckpointUnchanged",receiver.finalized()==finalized},{"finalizedAdvanceQualified",false}});
}
}
int main(int argc,char** argv){try{
 const bool all=argc==2&&std::string(argv[1])=="--self-test";
 const std::string group=argc==3&&std::string(argv[1])=="--group"?argv[2]:"";
 check(all||group=="orphan-retry"||group=="deferred-session-reopen","exact new selector");
 ScopedChainStoreMemoryPolicy memory({4*1024*1024,2,16*1024*1024,0});
 test_stage="public-fixture-and-three-signed-blocks";const auto f=fixture();std::vector<Bytes> wires;
 {Pair producer(f);for(uint64_t slot:{2,18,19})wires.push_back(child(producer,slot));}
 check(wires.size()==3&&test_signatures==8,"exact deterministic fixture signatures");
 size_t groups=0;if(all||group=="orphan-retry"){orphan_retry(f,wires);++groups;}
 if(all||group=="deferred-session-reopen"){deferred_reopen(f,wires);++groups;}
 emit("capella-sync-adverse-summary",{{"groups",groups},{"signedBlocks",3},{"publicFixtureDomainReused",true},{"privateCampaign03Used",false},{"actualPeakRssKiB",public_peak_rss_kib()},{"maximumRssKiB",131072},{"maximumTestSignatures",1024},{"maximumWallSeconds",180}});return 0;
}catch(const std::exception& e){std::cerr<<Json{{"diagnostic","capella-sync-adverse-failure-v1"},{"stage",test_stage},{"newTestOnlySignaturesCreated",test_signatures},{"actualPeakRssKiB",public_peak_rss_kib()},{"privateInputsRead",false},{"runtimeDutiesAuthorized",false},{"productionQualified",false}}.dump()<<'\n'<<e.what()<<std::endl;return 1;}}
