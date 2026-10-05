// Private synthetic fixture only. Caller must freeze ALL source writers first.
// Read-only capture, never repair/retire journals and never claim runtime health.
#include "nano/consensus_capella_audit.hpp"
#include "nano/consensus_capella_reactor.hpp"
#include "nano/consensus_capella_fork_state.hpp"
#include "nano/consensus_beacon_signer.hpp"
#include "nano/chain.hpp"
#include "blst.h"
#include "/work/src/consensus_capella_projection.hpp"
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
 // Offline structural counters only. Existing separate canonical audit verifies
 // the stopped database; this tool never signs, writes or claims replay.
 const auto document=Json::parse(primary->snapshot);
 const auto& store=document.contains("store")?document.at("store"):document;
 auto count=[&](const char* field){need(store.at(field).is_array(),"counter field not array");return store.at(field).size();};

 const auto anchor=store.at("anchor").get<std::string>(),finalized=store.at("finalized").at(1).get<std::string>();
 const auto slots=rules.transition.consensus.epoch.ffg.slots_per_epoch,lookahead=rules.transition.consensus.attestation.duties.min_seed_lookahead;
 const auto genesis_ms=num(store.at("clock").at(0)),time_ms=num(store.at("clock").at(1));need(slots&&rules.slot_duration_ms&&time_ms>=genesis_ms,"diagnostic clock invalid");
 const auto epoch=(time_ms-genesis_ms)/rules.slot_duration_ms/slots;
 struct Node{uint64_t slot;std::string parent;};std::map<std::string,Node> nodes;size_t state_bytes=0;uint64_t oldest=UINT64_MAX,newest=0;
 for(const auto& row:store.at("blocks")){need(row.is_array()&&row.size()==6,"diagnostic block shape");const auto id=row.at(0).get<std::string>();const auto& header=row.at(1);const auto slot=num(header.at(0));need(nodes.emplace(id,Node{slot,header.at(2).get<std::string>()}).second,"diagnostic duplicate root");oldest=std::min(oldest,slot);newest=std::max(newest,slot);const auto& ss= row.at(2).at(0).get_ref<const std::string&>();need(ss.starts_with("0x")&&ss.size()%2==0,"diagnostic state hex");state_bytes+=(ss.size()-2)/2;}
 need(nodes.contains(anchor)&&nodes.contains(finalized),"diagnostic checkpoints missing");
 std::vector<std::pair<uint64_t,std::string>> ordered;std::map<std::string,size_t> children;
 for(const auto& [id,node]:nodes){ordered.emplace_back(node.slot,id);if(id!=anchor){need(nodes.contains(node.parent)&&nodes.at(node.parent).slot<node.slot,"diagnostic ancestry mismatch");++children[node.parent];}}
 std::sort(ordered.begin(),ordered.end());std::set<std::string> retained{finalized};for(const auto& [slot,id]:ordered)if(id!=finalized&&retained.contains(nodes.at(id).parent))retained.insert(id);
 size_t pending_votes=0,required_outside=0;if(document.contains("signedVotes"))for(const auto& row:document.at("signedVotes")){++pending_votes;const auto vote=decode_consensus_attestation_ssz(unhex(row.at(1)),rules.transition.consensus.body.validators_per_committee);need(bool(vote),"diagnostic signed vote decode");auto root_text=[](const ConsensusRoot& r){return hex(Bytes(r.begin(),r.end()));};if(!retained.contains(root_text(vote->data.target.root))||!retained.contains(root_text(vote->data.beacon_block_root)))++required_outside;}
 const bool age_ready=lookahead<epoch&&(epoch-lookahead-1)<=UINT64_MAX/slots&&nodes.at(finalized).slot<=(epoch-lookahead-1)*slots;
 size_t branch_points=0,leaves=0;for(const auto& [id,node]:nodes){const auto children_count=children[id];if(children_count>1)++branch_points;if(children_count==0)++leaves;}
 std::cout<<Json{{"format","nano-capella-stopped-pruning-eligibility-v1"},{"restart",wire(primary->token)},
 {"blocks",nodes.size()},{"blockLimit",limits.blocks},{"snapshotBytes",primary->snapshot.size()},{"envelopeLimit",limits.envelope_bytes},{"sszStateBytes",state_bytes},{"totalStateLimit",limits.total_state_bytes},
 {"anchorRoot",anchor},{"anchorSlot",nodes.at(anchor).slot},{"finalizedRoot",finalized},{"finalizedSlot",nodes.at(finalized).slot},{"finalizedEpoch",num(store.at("finalized").at(0))},{"currentEpoch",epoch},{"minSeedLookahead",lookahead},{"oldestSlot",oldest},{"newestSlot",newest},{"branchPoints",branch_points},{"leafRoots",leaves},
 {"anchorAlreadyFinalized",anchor==finalized},{"finalizedAgeReady",age_ready},{"descendantsOfFinalized",retained.size()},{"removableByFinalizedAncestry",nodes.size()-retained.size()},{"signedVotes",pending_votes},{"voteContextsOutsideCandidate",required_outside},
 {"structuralEligibilityOnly",true},{"pruningApplied",false},{"sourceDigestAndWitnessVerified",true},{"readOnly",true},{"canonicalReplayPerformed",false},{"runtimeHealthInferred",false},{"productionQualified",false}}.dump(2)<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
