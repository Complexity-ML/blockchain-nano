// Offline qualification utility: operate on a COPY of a stopped synthetic node.
#include "nano/chain.hpp"
#include "nano/consensus_bellatrix_store_snapshot.hpp"
#include "nano/consensus_bellatrix_duty.hpp"
#include "nano/consensus_bellatrix_reactor.hpp"
#include "nano/consensus_bellatrix_runtime.hpp"
#include "blst.h"
#include "nano/committee.hpp"
#include "nano/beacon_api.hpp"
#include "nano/consensus_message_ssz.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace nano;
Json load(const char* path){
 const auto n=std::filesystem::file_size(path);
 if(n>16*1024*1024)throw std::length_error("qualification input too large");
 std::ifstream f(path);Json value;f>>value;return value;
}
int main(int argc,char** argv){try{
 if(argc!=4&&argc!=5&&argc!=6&&argc!=7)throw std::invalid_argument("usage: checkpoint-export genesis.json copied-database shutdown-output.json [node-config.json [--admit-fixture|--resume-fixture|--attestation-report|--duty-report|--beacon-report|--replay-archive peer.json]]");
 if(!std::filesystem::is_directory(argv[2]))throw std::invalid_argument("copied database missing");
 const auto genesis=Genesis::from_json(load(argv[1]));
 // The harness stores process output as a JSON string. Require its final
 // shutdown commitment instead of trusting a token found in the database.
 std::istringstream lines(load(argv[3]).get<std::string>());std::string line;Json stopped;
 while(std::getline(lines,line)){auto j=Json::parse(line,nullptr,false);if(j.is_object()&&j.value("event","")=="bellatrix-runtime-stopped")stopped=std::move(j);}
 if(stopped.is_null())throw std::invalid_argument("missing independent shutdown commitment");
 const auto& token=stopped.at("restart");
 ForkCheckpointToken expected{u64(quantity(token.at("sequence"))),unhex(token.at("digest").get<std::string>())};
 auto disk=std::make_unique<ChainStore>(argv[2]);
 const auto record=disk->fork_checkpoint(genesis.chain_id,genesis.block.header.hash(),128*1024*1024,expected);
 if(!record||sha256(Bytes(record->snapshot.begin(),record->snapshot.end()))!=expected.digest)throw std::runtime_error("checkpoint digest mismatch");
 if(argc>=5){
  if(argc==6&&std::string(argv[5])!="--admit-fixture"&&std::string(argv[5])!="--resume-fixture"&&std::string(argv[5])!="--attestation-report"&&std::string(argv[5])!="--duty-report"&&std::string(argv[5])!="--beacon-report")throw std::invalid_argument("unknown replay option");
  if(argc==7&&std::string(argv[5])!="--replay-archive")throw std::invalid_argument("unknown archive replay option");
  auto config=load(argv[4]).at("bellatrix");const auto& c=config.at("limits");
  auto n=[&](const char* name){return u64(quantity(c.at(name)));};
  const BellatrixStoreSnapshotLimits limits{size_t(n("envelopeBytes")),size_t(n("blocks")),size_t(n("messages")),size_t(n("equivocations")),size_t(n("stateBytes")),size_t(n("totalStateBytes")),n("replaySlots"),n("syncTrials")};
  const auto document=Json::parse(record->snapshot);const auto payload=document.contains("store")?document.at("store"):document;
  const auto profile=payload.at("profile").dump(),encoded=payload.dump();
  const auto rules=load_bellatrix_fork_profile(profile,sha256(Bytes(profile.begin(),profile.end())),profile.size());
  const auto state=bellatrix_store_from_snapshot(encoded,sha256(Bytes(encoded.begin(),encoded.end())),rules,limits);
  if(argc==6&&std::string(argv[5])=="--beacon-report"){
   const auto& head=bellatrix_fork_head(state,rules);
   const auto head_root=consensus_beacon_header_root(head.header);
   const auto finality=bellatrix_beacon_read(state,rules,"/eth/v1/beacon/states/head/finality_checkpoints");
   Json block_states=Json::array();const auto& t=rules.transition.consensus;
   for(const auto& [id,block]:state.blocks){
    if(block.header.slot+t.epoch.ffg.slots_per_epoch < head.header.slot)continue;
    const auto cp=bellatrix_fork_checkpoints(block.state,{t.epoch,rules.transition.execution,rules.transition.maximum_state_bytes},t.state);
    block_states.push_back({{"slot",block.header.slot},{"realizedFinalized",cp.realized.finalized.epoch},{"unrealizedFinalized",cp.unrealized.finalized.epoch}});
   }

   std::cout<<Json{{"slotsPerEpoch",rules.transition.consensus.epoch.ffg.slots_per_epoch},{"headSlot",head.header.slot},{"headRoot",hex(Bytes(head_root.begin(),head_root.end()))},{"storeFinalizedEpoch",state.timed.view.finalized.epoch},{"storeJustifiedEpoch",state.timed.view.justified.epoch},{"headFinality",finality.body},{"recentBlockStates",block_states}}.dump()<<'\n';return 0;
  }
  if(argc==6&&std::string(argv[5])=="--duty-report"){
   const auto& trace=stopped.at("validator").at("dutyTrace");
   if(!trace.is_array()||trace.empty())throw std::invalid_argument("missing local duty trace");
   auto root=[](const Json& encoded){const auto bytes=unhex(encoded.get<std::string>());ConsensusRoot r{};if(bytes.size()!=r.size())throw std::invalid_argument("diagnostic root width");std::copy(bytes.begin(),bytes.end(),r.begin());return r;};
   auto ancestor=[&](ConsensusRoot a,ConsensusRoot b){
    for(size_t n=0;n<=state.blocks.size();++n){
     if(a==b)return true;
     const auto& node=state.blocks.at(b);if(b==state.timed.view.anchor||node.header.slot<=state.blocks.at(a).header.slot)return false;
     b=node.header.parent_root;
    }
    throw std::runtime_error("diagnostic ancestry cycle");
   };
   const auto slots=rules.transition.consensus.epoch.ffg.slots_per_epoch;Json rows=Json::array();
   for(const auto& observation:trace){
    const auto head=root(observation.at("signedHead"));const auto epoch=observation.at("slot").get<uint64_t>()/slots;
    Json counts=Json::object();size_t unknown=0;
    for(const auto& message:observation.at("latestMessages")){
     const auto id=root(message.at("root"));if(!state.blocks.contains(id)||!state.blocks.contains(head)){++unknown;continue;}
     const auto voted=message.at("epoch").get<uint64_t>();
     const char* age=voted>epoch?"future":voted==epoch?"current":epoch-voted==1?"previous":"older";
     const char* relation=ancestor(id,head)?"ancestorOrSame":ancestor(head,id)?"descendant":"competing";
     auto& count=counts[message.at("local").get<bool>()?"local":"remote"][age][relation];
     count=count.is_null()?1:count.get<uint64_t>()+1;
    }
    auto row=observation;row.erase("latestMessages");row["knownMessages"]=std::move(counts);row["unknownRoots"]=unknown;rows.push_back(std::move(row));
   }
   std::cout<<Json{{"scope","post-signing observations, unweighted message counts; not an atomic replay of fork choice"},{"traceFailures",stopped.at("validator").at("dutyTraceFailures")},{"observations",rows}}.dump()<<'\n';return 0;
  }
  if(argc==6&&std::string(argv[5])=="--attestation-report"){
   const auto& t=rules.transition.consensus;const auto slots=t.epoch.ffg.slots_per_epoch;
   const BellatrixEpochRules epoch_rules{t.epoch,rules.transition.execution,rules.transition.maximum_state_bytes};
   const auto& head=bellatrix_fork_head(state,rules);std::set<ConsensusRoot> canonical;
   auto root=consensus_beacon_header_root(head.header);
   for(;;){canonical.insert(root);if(root==state.timed.view.anchor)break;root=state.blocks.at(root).header.parent_root;}
   auto little=[](std::span<const uint8_t> b,size_t offset,size_t width){
    if(width>8||offset>b.size()||width>b.size()-offset)throw std::runtime_error("Diagnostic SSZ bounds");
    uint64_t n=0;for(size_t i=0;i<width;++i)n|=uint64_t(b[offset+i])<<(8*i);return n;
   };
   Json rows=Json::array();
   for(const auto& entry:document.at("signedBlocks")){
    const auto wire=unhex(entry.at(1).get<std::string>());if(wire.size()<184)throw std::runtime_error("Diagnostic block bounds");
    const auto bytes=unhex(entry.at(0).get<std::string>());ConsensusRoot block_root{};
    if(bytes.size()!=32)throw std::runtime_error("Diagnostic root width");std::copy(bytes.begin(),bytes.end(),block_root.begin());
    const auto body=std::span<const uint8_t>(wire).subspan(184);
    const auto begin=little(body,208,4),end=little(body,212,4);
    if(begin>end||end>body.size())throw std::runtime_error("Diagnostic list bounds");
    const auto list=body.subspan(begin,end-begin);if(list.empty())continue;
    const auto first=little(list,0,4);if(first%4||first>list.size())throw std::runtime_error("Diagnostic offset table");
    for(size_t i=0;i<first/4;++i){
     const auto a=little(list,i*4,4),b=i+1<first/4?little(list,(i+1)*4,4):list.size();
     if(a<first||a>b||b>list.size())throw std::runtime_error("Diagnostic vote bounds");
     const auto raw=list.subspan(a,b-a);const auto vote=decode_consensus_attestation_ssz(raw,t.body.validators_per_committee);
     if(!vote)throw std::runtime_error("Diagnostic invalid attestation");
     const auto& target=state.blocks.at(vote->data.target.root);
     const auto ctx=prepare_bellatrix_fork_state_contexts(target.state,target.header,vote->data.target,epoch_rules,t.state,limits.replay_slots,limits.sync_trials);
     const auto duties=phase0_attestation_duties(ctx.target.registry,ctx.target.randao,t.attestation.duties,vote->data.target.epoch);
     const auto committee=partition_committee(duties.shuffled,(vote->data.slot%slots)*duties.committees_per_slot+vote->data.index,duties.committees_per_slot*slots);
     if(committee.size()!=vote->aggregation_bits.size())throw std::runtime_error("Diagnostic committee mismatch");
     auto boundary=consensus_beacon_header_root(head.header);
     while(state.blocks.at(boundary).header.slot>vote->data.target.epoch*slots&&boundary!=state.timed.view.anchor)boundary=state.blocks.at(boundary).header.parent_root;
     Json indices=Json::array();for(size_t j=0;j<committee.size();++j)if(vote->aggregation_bits[j])indices.push_back(committee[j]);
     rows.push_back({{"blockSlot",state.blocks.at(block_root).header.slot},{"canonical",canonical.contains(block_root)},{"targetMatchesCanonicalBoundary",boundary==vote->data.target.root},{"voteId",hex(sha256(Bytes(raw.begin(),raw.end())))},{"voteSlot",vote->data.slot},{"sourceEpoch",vote->data.source.epoch},{"targetEpoch",vote->data.target.epoch},{"targetRoot",hex(Bytes(vote->data.target.root.begin(),vote->data.target.root.end()))},{"indices",indices}});
    }
   }
   std::cout<<Json{{"headSlot",head.header.slot},{"attestations",rows}}.dump()<<'\n';return 0;
  }
  if(argc==7){
   const auto peer=load(argv[6]);const auto& archive=peer.at("signedBlocks");
   if(!archive.is_array()||archive.size()>limits.blocks)throw std::invalid_argument("peer archive capacity");
   std::vector<std::pair<uint64_t,Bytes>> pending;
   for(const auto& item:archive){
    auto wire=unhex(item.at(1).get<std::string>());
    if(wire.size()<184||wire.size()>rules.transition.consensus.maximum_block_bytes)throw std::invalid_argument("peer block size");
    uint64_t slot=0;for(size_t i=0;i<8;++i)slot|=uint64_t(wire[100+i])<<(8*i);
    pending.emplace_back(slot,std::move(wire));
   }
   std::stable_sort(pending.begin(),pending.end(),[](const auto& a,const auto& b){return a.first<b.first;});
   auto replay=state;Json results=Json::array();
   for(const auto& [slot,wire]:pending){
    const auto begin=std::chrono::steady_clock::now();Json result{{"slot",slot}};
    try{auto next=prepare_bellatrix_received_block(replay,wire,rules,limits.replay_slots,limits.sync_trials);
     result["accepted"]=bool(next);result["added"]=next&&next->blocks.size()>replay.blocks.size();if(next)replay=std::move(*next);
    }catch(const std::exception& error){result["error"]=error.what();}
    result["milliseconds"]=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-begin).count();results.push_back(std::move(result));
   }
   std::cout<<Json{{"initialBlocks",state.blocks.size()},{"resultBlocks",replay.blocks.size()},{"imports",results}}.dump()<<'\n';return 0;
  }
  const auto& head=bellatrix_fork_head(state,rules);
  const auto slot=(state.timed.clock.time_ms-state.timed.clock.genesis_time_ms)/rules.slot_duration_ms;
  Json report{{"clockSlot",slot},{"headSlot",head.header.slot}};
  report["journals"]=Json::array();
  ConsensusRoot validators_genesis{};std::copy_n(head.state.ssz.begin()+8,32,validators_genesis.begin());
  for(const auto& key:config.at("validator").at("keys")){
   const auto index=u64(quantity(key.at("index")));if(index>=64)throw std::invalid_argument("non-fixture key index");
   uint64_t words[4]{index+1,0,0,0};blst_scalar secret;blst_scalar_from_uint64(&secret,words);blst_p1 pk;blst_sk_to_pk_in_g1(&pk,&secret);Bytes public_key(48);blst_p1_compress(public_key.data(),&pk);
   Json commitment;for(const auto& saved:stopped.at("validatorKeys"))if(saved.at("index")==key.at("index"))commitment=saved.at("restart");
   if(commitment.is_null())throw std::invalid_argument("missing journal shutdown commitment");
   const ForkCheckpointToken token{u64(quantity(commitment.at("sequence"))),unhex(commitment.at("digest").get<std::string>())};
   const auto journal_path=std::filesystem::path(argv[2]).parent_path()/std::filesystem::path(key.at("journal").get<std::string>()).filename();
   ChainStore journal_disk(journal_path);BeaconSigningJournal journal(journal_disk,genesis.chain_id,genesis.block.header.hash(),validators_genesis,public_key,rules.transition.consensus.epoch.ffg.slots_per_epoch,token);
   const auto proposal=journal.latest_proposal_slot(),vote=journal.latest_attestation_target();
   report["journals"].push_back({{"index",index},{"proposalSlot",proposal?Json(*proposal):Json(nullptr)},{"attestationEpoch",vote?Json(*vote):Json(nullptr)}});
  }

  if(argc==6&&std::string(argv[5])=="--resume-fixture"){
   const auto directory=std::filesystem::path(argv[2]).parent_path();
   config["initial"]=nullptr;config["restart"]=stopped.at("restart");
   config["profile"]["file"]=(directory/std::filesystem::path(config["profile"]["file"].get<std::string>()).filename()).string();
   for(auto& key:config["validator"]["keys"]){
    const auto index=u64(quantity(key.at("index")));
    for(const auto& saved:stopped.at("validatorKeys"))if(saved.at("index")==key.at("index"))key["restart"]=saved.at("restart");
    key["journal"]=(directory/std::filesystem::path(key.at("journal").get<std::string>()).filename()).string();
    const auto name=key.at("keyEnv").get<std::string>();
    if(setenv(name.c_str(),hex(bytes(Int(std::to_string(index+1)),32)).c_str(),1))throw std::runtime_error("fixture environment");
   }
   disk.reset();Blockchain chain(argv[2],genesis);
   const auto start=std::chrono::steady_clock::now();
   auto clock=[&]{return state.timed.clock.time_ms+uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count());};
   auto runtime=start_bellatrix_runtime(chain,config,{},clock);
   std::this_thread::sleep_for(std::chrono::seconds(24));
   report["runtime"]=stop_bellatrix_runtime(runtime);report["runtime"].erase("validatorKeys");
   report["newHeadSlot"]=bellatrix_fork_head(*runtime.reactor->current().state,rules).header.slot;
   std::cout<<report.dump()<<'\n';return 0;
  }
  try{const auto& t=rules.transition.consensus;
   const auto identity=prepare_bellatrix_proposer_identity(head.state,head.header,slot,{t.epoch,rules.transition.execution,rules.transition.maximum_state_bytes},t.state,t.attestation.duties,limits.replay_slots,limits.sync_trials);
   report["proposer"]=identity?Json(identity->index):Json(nullptr);
   if(identity&&identity->index<64){
    uint64_t words[4]{identity->index+1,0,0,0};blst_scalar secret;blst_scalar_from_uint64(&secret,words);
    blst_p1 pk;blst_sk_to_pk_in_g1(&pk,&secret);Bytes public_key(48);blst_p1_compress(public_key.data(),&pk);
    if(public_key==identity->public_key){
     // Explicitly recognized public test-fixture key; never load user keys.
     ConsensusRoot epoch{};const auto e=slot/t.epoch.ffg.slots_per_epoch;for(size_t i=0;i<8;++i)epoch[i]=uint8_t(e>>(8*i));
     const auto root=consensus_signing_root(epoch,consensus_domain({2,0,0,0},identity->fork_version,identity->genesis_root));
     static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";
     blst_p2 h,signature;blst_hash_to_g2(&h,root.data(),32,reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);blst_sign_pk_in_g1(&signature,&h,&secret);
     Bytes reveal(96);blst_p2_compress(reveal.data(),&signature);
     disk.reset();Blockchain chain(argv[2],genesis);BellatrixDurableReactor reactor(chain,rules,limits,expected);
     try{const auto body=reactor.prepare_local_body(identity->index,slot,reveal,"0x0000000000000000000000000000000000001234",expected,128);report["bodyBytes"]=body?Json(body->size()):Json(nullptr);
      if(body&&argc==6){
       Json owner,commitment;
       for(const auto& key:config.at("validator").at("keys"))if(u64(quantity(key.at("index")))==identity->index)owner=key;
       for(const auto& saved:stopped.at("validatorKeys"))if(u64(quantity(saved.at("index")))==identity->index)commitment=saved.at("restart");
       if(owner.is_null()||commitment.is_null())throw std::invalid_argument("proposer is not owned by copied node");
       const auto path=std::filesystem::path(argv[2]).parent_path()/std::filesystem::path(owner.at("journal").get<std::string>()).filename();
       ChainStore journal_disk(path);const ForkCheckpointToken token{u64(quantity(commitment.at("sequence"))),unhex(commitment.at("digest").get<std::string>())};
       BeaconSigningJournal journal(journal_disk,genesis.chain_id,genesis.block.header.hash(),identity->genesis_root,public_key,t.epoch.ffg.slots_per_epoch,token);
       auto provider=[&](const ConsensusRoot& digest){blst_p2 h,sig;blst_hash_to_g2(&h,digest.data(),32,reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);blst_sign_pk_in_g1(&sig,&h,&secret);Bytes bytes(96);blst_p2_compress(bytes.data(),&sig);return bytes;};
       const auto wire=reactor.sign_proposal(identity->index,slot,*body,[&]{return state.timed.clock.time_ms;},expected,journal,provider);
       report["proposalSigned"]=bool(wire);if(wire){report["proposalAdmitted"]=reactor.block(*wire);report["newHeadSlot"]=bellatrix_fork_head(*reactor.current().state,rules).header.slot;}
      }
}
     catch(const std::exception& error){report["bodyError"]=error.what();}
    }
   }
  }catch(const std::exception& e){report["proposerError"]=e.what();}
  try{const auto duties=prepare_bellatrix_attestation_duties(state,rules,slot,limits.replay_slots,limits.sync_trials);report["attesters"]=Json::array();for(const auto& [index,duty]:duties)report["attesters"].push_back(index);
  }catch(const std::exception& e){report["attesterError"]=e.what();}
  std::cout<<report.dump()<<'\n';
 }else std::cout<<record->snapshot<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
