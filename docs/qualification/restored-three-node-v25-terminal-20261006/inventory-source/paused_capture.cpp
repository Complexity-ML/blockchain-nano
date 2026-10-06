// Private synthetic fixture only. Caller must freeze ALL source writers first.
// Read-only capture, never repair/retire journals and never claim runtime health.
#include "nano/consensus_capella_audit.hpp"
#include "nano/consensus_capella_reactor.hpp"
#include "nano/consensus_capella_fork_state.hpp"
#include "nano/consensus_beacon_signer.hpp"
#include "nano/chain.hpp"
#include "blst.h"
#include "../../src/consensus_capella_projection.hpp"
#include <fstream>
#include <iostream>
#include <set>
using namespace nano;
namespace {
void need(bool b,const char* s){if(!b)throw std::runtime_error(s);}
std::string read(const std::string& p,size_t max){need(std::filesystem::is_regular_file(p)&&std::filesystem::file_size(p)<=max,"capture file bounds");std::ifstream f(p,std::ios::binary);std::string s((std::istreambuf_iterator<char>(f)),{});need(!f.bad()&&s.size()<=max,"capture read bounds");return s;}
Json json(const std::string& p){return Json::parse(read(p,1024*1024));}
uint64_t num(const Json& j){return u64(quantity(j));}
ForkCheckpointToken token(const Json& j){need(j.is_object()&&j.size()==2,"capture token shape");ForkCheckpointToken t{num(j.at("sequence")),unhex(j.at("digest"))};need(t.sequence&&t.digest.size()==32&&j.at("digest")==hex(t.digest),"capture token bounds");return t;}
// Private-fixture telemetry from authenticated state; never infer gossip replay or health.
Json participation_diagnostic(const CapellaForkStore& store,const CapellaForkBlockRules& rules){
 const auto& head=capella_fork_head(store,rules);const auto& t=rules.transition;
 capella_beacon_state_root(head.state.ssz,t.consensus.state,t.execution,t.maximum_state_bytes);
 const auto offsets=capella_detail::offsets(t.consensus.state);const auto& b=head.state.ssz;
 const auto previous=capella_detail::offset(b,offsets[4]),current=capella_detail::offset(b,offsets[5]),scores=capella_detail::offset(b,offsets[6]);
 need(current>=previous&&scores>=current&&current-previous==65&&scores-current==65,"private diagnostic participation inventory bounds");
 const auto ctx=capella_fork_block_context(head.state,head.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);
 const auto epoch=head.header.slot/t.consensus.epoch.ffg.slots_per_epoch;Json members=Json::array();std::array<size_t,3> prior{},now{};
 for(uint64_t i=0;i<65;++i){
  const auto& member=ctx.registry.validators.at(i);const size_t owner=i<16?0:i<32?1:2;
  const auto pf=b.at(previous+i),cf=b.at(current+i);if(pf&2)++prior[owner];if(cf&2)++now[owner];
  Json message=nullptr;const auto it=store.timed.view.latest_messages.find(i);if(it!=store.timed.view.latest_messages.end())message={{"epoch",it->second.epoch},{"root",hex(Bytes(it->second.root.begin(),it->second.root.end()))}};
  members.push_back({{"index",i},{"owner",owner},{"previousFlags",pf},{"currentFlags",cf},{"latestForkMessage",message},{"effectiveBalance",member.effective_balance},{"activationEpoch",member.activation_epoch},{"exitEpoch",member.exit_epoch},{"slashed",member.slashed}});
 }
 return {{"stateRoot",hex(Bytes(head.header.state_root.begin(),head.header.state_root.end()))},{"headSlot",head.header.slot},{"stateEpoch",epoch},{"previousEpoch",epoch?Json(epoch-1):Json(nullptr)},{"timelyTargetFlagIndex",1},{"previousTimelyTargetCountsByOwner",prior},{"currentTimelyTargetCountsByOwner",now},{"members",members},{"historicalStandaloneVotesReplayed",false},{"runtimeHealthInferred",false},{"productionQualified",false}};
}
Json wire(const ForkCheckpointToken& t){return {{"sequence",quantity(Int(std::to_string(t.sequence)))},{"digest",hex(t.digest)}};}
void floor(const ForkCheckpointToken& actual,const ForkCheckpointToken& minimum){need(actual.sequence>=minimum.sequence,"capture below external minimum sequence");if(actual.sequence==minimum.sequence)need(actual.digest==minimum.digest,"capture external minimum digest mismatch");}
Bytes public_key(uint64_t index){need(index<=64,"private capture key bounds");uint64_t words[4]{index+1,0,0,0};blst_scalar s;blst_scalar_from_uint64(&s,words);blst_p1 p;blst_sk_to_pk_in_g1(&p,&s);Bytes b(48);blst_p1_compress(b.data(),&p);return b;}
void witness(const ForkCheckpointRecord& primary,const ForkCheckpointRecord& saved,const char* format,size_t fields){const auto intent=Json::parse(saved.snapshot);need(intent.is_object()&&intent.size()==fields&&intent.at("format")==format&&intent.dump()==saved.snapshot,"noncanonical capture witness");need(token(intent.at("next"))==primary.token&&intent.at("snapshot")==primary.snapshot,"capture primary/witness not at common publication boundary");}
}
int main(int argc,char** argv){try{
 need(argc==6&&std::string(argv[1])=="--capture","usage: --capture synthetic-genesis node-config quiescent-db external-request");
 const auto genesis=Genesis::from_json(json(argv[2]));need(genesis.chain_id==1337&&genesis.consensus=="beacon"&&genesis.evm_revision==EvmRevision::Shanghai,"private Beacon Shanghai capture only");
 const auto c=json(argv[3]).at("capella"),req=json(argv[5]);need(c.at("initial").is_null()&&c.value("automaticRecovery",true)==false&&c.contains("recoveryCommitment"),"strict witnessed source configuration required");
 const auto& l=c.at("limits");CapellaStoreSnapshotLimits limits{size_t(num(l.at("envelopeBytes"))),size_t(num(l.at("blocks"))),size_t(num(l.at("messages"))),size_t(num(l.at("equivocations"))),size_t(num(l.at("stateBytes"))),size_t(num(l.at("totalStateBytes"))),num(l.at("replaySlots")),num(l.at("syncTrials"))};need(limits.envelope_bytes<=128*1024*1024&&limits.blocks<=1024,"private capture budget");
 const auto& p=c.at("profile");need(req.at("profile").at("file")==p.at("file")&&req.at("profile").at("sha256")==p.at("sha256")&&req.at("limits")==l,"capture audit/source configuration mismatch");const auto rules=load_capella_fork_profile(read(p.at("file"),1024*1024),unhex(p.at("sha256")),1024*1024);need(rules.transition.chain_id==genesis.chain_id&&rules.native_deposit_votes,"native admitted fixture profile required");
 const auto& a=req.at("anchor");const auto seed=capella_store_from_snapshot(read(a.at("file"),limits.envelope_bytes),unhex(a.at("sha256")),rules,limits);
 auto context=[&](const CapellaForkBlock& b){return capella_fork_block_context(b.state,b.header,{rules.transition.consensus.epoch,rules.transition.execution,rules.transition.maximum_state_bytes},rules.transition.consensus.state);};
 need(context(capella_fork_head(seed,rules)).registry.validators.size()==64,"initial64-member capture anchor required");
 ChainStore disk(argv[4],ChainStore::OpenMode::ReadOnly);disk.bind_chain(genesis.chain_id,genesis.block.header.hash(),"beacon");const auto primary=disk.fork_checkpoint(genesis.chain_id,genesis.block.header.hash(),limits.envelope_bytes);need(bool(primary),"capture primary missing");floor(primary->token,token(req.at("minimumCheckpoint")));
 ChainStore core_witness(c.at("recoveryWitness"),ChainStore::OpenMode::ReadOnly);core_witness.bind_chain(genesis.chain_id,genesis.block.header.hash(),"nano-capella-recovery-v1");const auto saved=core_witness.fork_checkpoint(genesis.chain_id,genesis.block.header.hash(),2048+8*limits.envelope_bytes);need(bool(saved),"capture core witness missing");witness(*primary,*saved,"nano-capella-recovery-v1",5);
 need(sha256(Bytes(primary->snapshot.begin(),primary->snapshot.end()))==primary->token.digest,"capture primary digest mismatch");
 if(req.value("inventoryOnly",false)){
  need(primary->token==token(req.at("minimumCheckpoint")),"inventory source differs from prior stopped audit");
  CapellaDurableReactor inventory_reactor(disk,genesis.chain_id,genesis.block.header.hash(),rules,limits,primary->token);
  const size_t root_budget=65536,diagnostic_budget=4*1024*1024;Json catalogs=Json::object();
  for(const char* kind:{"block","attestation","attester_slashing","voluntary_exit","bls_to_execution_change","sync_committee_message"}){
   auto snapshot=inventory_reactor.capture_inventory(kind,diagnostic_budget);const auto bytes=snapshot->root_bytes();bool fits=true;
   try{(void)inventory_reactor.capture_inventory(kind,root_budget);}catch(const std::length_error&){fits=false;}
   catalogs[kind]={{"rootBytes",bytes},{"allocatedRoots",bytes/sizeof(ConsensusRoot)},{"fitsFullConfiguredRootBudget",fits}};
  }
  const auto state=inventory_reactor.current().state;
  std::cout<<Json{{"format","nano-private-stopped-inventory-diagnostic-v1"},{"restart",wire(primary->token)},{"readOnlyStores",true},{"priorFullAuditRequired",true},{"canonicalAuditRepeated",false},{"productionQualified",false},{"retainedGraphBlocks",state->blocks.size()},{"configuredGraphBlocks",limits.blocks},{"configuredCatalogRootBudget",root_budget},{"offlineDiagnosticRootBudget",diagnostic_budget},{"catalogs",catalogs}}.dump(2)<<'\n';return 0;
 }
 Json audit_request;for(const char* f:{"profile","anchor","limits","requiredFinalized","maximumBlocks","maximumArchiveBytes"})audit_request[f]=req.at(f);audit_request["expectedCheckpoint"]=wire(primary->token);const auto audit=audit_capella_database(genesis,argv[4],audit_request);need(audit.at("canonicalHistoryVerified")==true&&audit.at("externalFinalizedAncestorVerified")==true,"capture canonical audit failed");
 CapellaDurableReactor reactor(disk,genesis.chain_id,genesis.block.header.hash(),rules,limits,primary->token);const auto current=context(capella_fork_head(*reactor.current().state,rules));need(current.registry.validators.size()==65,"65-member admitted capture registry required");
 for(uint64_t n=0;n<65;++n)need(std::ranges::equal(current.registry.validators.at(n).public_key,public_key(n)),"foreign capture registry key");
 const auto& seed_ssz=seed.blocks.at(seed.timed.view.anchor).state.ssz;need(seed_ssz.size()>=40,"capture genesis binding");ConsensusRoot validators_root{};std::copy_n(seed_ssz.begin()+8,32,validators_root.begin());
 const auto& keys=c.at("validator").at("keys"),&minimum=req.at("minimumValidatorKeys");need(keys.is_array()&&minimum.is_array()&&(keys.size()==32||keys.size()==33)&&minimum.size()==keys.size(),"capture key inventory bounds");
 struct ObservedSigner{std::filesystem::path journal,witness;ForkCheckpointToken primary,independent;};std::vector<ObservedSigner> observed;
 std::set<uint64_t> indices;std::set<std::filesystem::path> paths;Json result_keys=Json::array();
 for(const auto& key:keys){const bool pending=key.at("index").is_null();const uint64_t index=pending?64:num(key.at("index"));need(index<=64&&indices.insert(index).second&&key.at("keyEnv")=="NANO_TEST_CAPELLA_NETWORK_KEY_"+std::to_string(index),"foreign/duplicate capture key");const auto pk=public_key(index);
  const Json* old=nullptr;for(const auto& item:minimum){const bool match=pending?(item.at("index").is_null()&&item.at("publicKey")==hex(pk)):(!item.at("index").is_null()&&num(item.at("index"))==index);if(match){need(!old,"duplicate capture minimum key");old=&item;}}need(old,"missing external minimum key");
  const auto jp=std::filesystem::weakly_canonical(key.at("journal").get<std::string>()),wp=std::filesystem::weakly_canonical(key.at("recoveryWitness").get<std::string>());need(paths.insert(jp).second&&paths.insert(wp).second,"shared capture signer paths");
  ChainStore journal(jp,ChainStore::OpenMode::ReadOnly),jw(wp,ChainStore::OpenMode::ReadOnly);journal.bind_chain(genesis.chain_id,genesis.block.header.hash(),"nano-beacon-signing-v1");jw.bind_chain(genesis.chain_id,genesis.block.header.hash(),"nano-beacon-signing-witness-v1");
  const auto actual=journal.fork_checkpoint(genesis.chain_id,genesis.block.header.hash(),4096),independent=jw.fork_checkpoint(genesis.chain_id,genesis.block.header.hash(),16384);need(actual&&independent,"capture signer stores absent");floor(actual->token,token(old->at("restart")));witness(*actual,*independent,"nano-beacon-signing-witness-v1",4);
  BeaconSigningJournal signer(journal,genesis.chain_id,genesis.block.header.hash(),validators_root,pk,rules.transition.consensus.epoch.ffg.slots_per_epoch,actual->token,&jw);need(signer.commitment()==actual->token,"capture signer changed on reopen");const auto target=signer.latest_attestation_target();need(target&&*target>=num(req.at("minimumAttestationTarget")),"capture signer below external attestation floor");
  Json item={{"index",pending?Json(nullptr):key.at("index")},{"restart",wire(actual->token)},{"latestAttestationTarget",*target},{"retired",signer.is_retired()}};if(pending)item["publicKey"]=hex(pk);result_keys.push_back(std::move(item));
  observed.push_back({jp,wp,actual->token,independent->token});
 }
 for(const auto& item:observed){ChainStore j(item.journal,ChainStore::OpenMode::ReadOnly),w(item.witness,ChainStore::OpenMode::ReadOnly);j.bind_chain(genesis.chain_id,genesis.block.header.hash(),"nano-beacon-signing-v1");w.bind_chain(genesis.chain_id,genesis.block.header.hash(),"nano-beacon-signing-witness-v1");need(j.fork_checkpoint(genesis.chain_id,genesis.block.header.hash(),4096,item.primary).has_value()&&w.fork_checkpoint(genesis.chain_id,genesis.block.header.hash(),16384,item.independent).has_value(),"capture signer/witness moved; freeze all writers first");}
 ChainStore final_disk(argv[4],ChainStore::OpenMode::ReadOnly),final_witness(c.at("recoveryWitness"),ChainStore::OpenMode::ReadOnly);final_disk.bind_chain(genesis.chain_id,genesis.block.header.hash(),"beacon");final_witness.bind_chain(genesis.chain_id,genesis.block.header.hash(),"nano-capella-recovery-v1");
 need(final_witness.fork_checkpoint(genesis.chain_id,genesis.block.header.hash(),2048+8*limits.envelope_bytes,saved->token).has_value(),"capture core witness moved");
 need(final_disk.fork_checkpoint(genesis.chain_id,genesis.block.header.hash(),limits.envelope_bytes,primary->token).has_value(),"capture source moved; freeze all writers first");
 const Json diagnostic=req.value("participationDiagnostics",false)?participation_diagnostic(*reactor.current().state,rules):Json(nullptr);
 std::cout<<Json{{"participationDiagnostics",diagnostic},{"format","nano-private-quiescent-recovery-pins-v1"},{"chainId","0x539"},{"genesisHash",hex(genesis.block.header.hash())},{"profileSha256",p.at("sha256")},{"restart",wire(primary->token)},{"validatorKeys",result_keys},{"canonicalReplay",audit},{"readOnlyStores",true},{"primaryWitnessAndSignersVerified",true},{"minimumSequenceBoundsVerified",true},{"historicalTokenDigestAncestryVerified",false},{"sourceQuiescenceExternallyRequired",true},{"runtimeHealthInferred",false},{"productionQualified",false}}.dump(2)<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
