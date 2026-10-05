#include "nano/consensus_capella_runtime.hpp"
#include "nano/local_monotonic_time.hpp"
#include <fstream>
#include <cstdlib>
#include <set>
#include <openssl/crypto.h>
#include "blst.h"
#include "../network/capella_consensus_service.hpp"
#include "../network/capella_inventory_sessions.hpp"
namespace nano {
namespace {
void shape(const Json& j,std::initializer_list<const char*> keys){
 if(!j.is_object()||j.size()!=keys.size())throw std::invalid_argument("Capella runtime fields");
 for(auto key:keys)if(!j.contains(key))throw std::invalid_argument("Missing Capella runtime field");
}
uint64_t number(const Json& j){
 if(!j.is_string())throw std::invalid_argument("Capella runtime quantity required");
 const auto n=quantity(j);const auto value=u64(n);
 if(!value||quantity(n)!=j.get<std::string>())throw std::invalid_argument("Capella runtime positive canonical quantity required");return value;
}
Bytes digest(const Json& j){
 if(!j.is_string()||j.get_ref<const std::string&>().size()!=66)throw std::invalid_argument("Capella digest width");
 const auto b=unhex(j.get<std::string>());if(b.size()!=32||hex(b)!=j.get<std::string>())throw std::invalid_argument("Capella digest encoding");return b;
}
std::string read_bounded(const std::filesystem::path& path,size_t maximum,const std::function<void()>& checkpoint){
 if(!std::filesystem::is_regular_file(path)||std::filesystem::file_size(path)>maximum)throw std::invalid_argument("Capella input size/type");
 std::ifstream input(path,std::ios::binary);if(!input)throw std::runtime_error("Cannot read Capella input");
 std::string out;std::array<char,16384> buffer;
 while(input){if(checkpoint)checkpoint();input.read(buffer.data(),buffer.size());const auto n=size_t(input.gcount());if(n>maximum-out.size())throw std::length_error("Capella input grew beyond bound");out.append(buffer.data(),n);}
 if(input.bad()||!input.eof())throw std::runtime_error("Capella input read failed");return out;
}
struct Owner {InboundBudget decoder_budget;std::unique_ptr<ChainStore> witness;std::unique_ptr<CapellaDurableReactor> reactor;std::unique_ptr<CapellaLocalClock> clock;std::unique_ptr<CapellaValidatorLoop> validator;std::unique_ptr<CapellaConsensusService> admission;std::unique_ptr<CapellaInventorySessions> catalogs;};
}
CapellaRuntime start_capella_runtime(Blockchain& chain,const Json& config,const std::function<void()>& checkpoint,std::function<uint64_t()> now){
 if(checkpoint)checkpoint();auto base=config;base.erase("validator");base.erase("admission");base.erase("catalogs");base.erase("recoveryWitness");base.erase("recoveryCommitment");base.erase("automaticRecovery");base.erase("network");base.erase("automaticPruning");base.erase("historicalStatePruning");shape(base,{"profile","limits","initial","restart","clock"});
 const auto& genesis=chain.genesis();
 if(genesis.consensus!="beacon"||genesis.evm_revision!=EvmRevision::Shanghai)throw std::invalid_argument("Capella runtime requires Beacon Shanghai chain");
 const auto& p=config.at("profile");shape(p,{"file","sha256","maximumBytes"});const auto maximum=number(p.at("maximumBytes"));
 const auto rules=load_capella_fork_profile(read_bounded(p.at("file").get<std::string>(),maximum,checkpoint),digest(p.at("sha256")),maximum);
 if(rules.transition.chain_id!=genesis.chain_id||rules.transition.replay_policy!=ReplayPolicy::RequireChainId)throw std::invalid_argument("Capella network requires matching chain ID and protected transactions");
 const auto& l=config.at("limits");shape(l,{"envelopeBytes","blocks","messages","equivocations","stateBytes","totalStateBytes","replaySlots","syncTrials"});
 const CapellaStoreSnapshotLimits limits{number(l.at("envelopeBytes")),number(l.at("blocks")),number(l.at("messages")),number(l.at("equivocations")),number(l.at("stateBytes")),number(l.at("totalStateBytes")),number(l.at("replaySlots")),number(l.at("syncTrials"))};
 if(limits.state_bytes<rules.transition.maximum_state_bytes||limits.total_state_bytes<limits.state_bytes)throw std::invalid_argument("Capella state capacity below configured transition");
 const auto& c=config.at("clock");auto clock_fields=c;clock_fields.erase("maximumWallClockDriftMs");shape(clock_fields,{"intervalMs","maximumForwardStepMs"});const auto interval=number(c.at("intervalMs")),step=number(c.at("maximumForwardStepMs"));
 std::optional<uint64_t> wall_drift;
 if(c.contains("maximumWallClockDriftMs")){
  wall_drift=number(c.at("maximumWallClockDriftMs"));
  if(*wall_drift>=rules.attestation_due_ms||*wall_drift>=step)throw std::invalid_argument("Capella wall drift budget must be below duty deadline and recovery step");
 }
 if(interval>uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::duration::max()/2).count()))throw std::invalid_argument("Capella clock interval range");
 const bool restart=!config.at("restart").is_null();
 const bool witnessed=config.contains("recoveryWitness");
 std::optional<ForkCheckpointToken> recovery_commitment;
 if(config.contains("recoveryCommitment")){
  if(!witnessed)throw std::invalid_argument("Capella recovery commitment requires witness");
  const auto& r=config.at("recoveryCommitment");shape(r,{"sequence","digest"});
  recovery_commitment=ForkCheckpointToken{number(r.at("sequence")),digest(r.at("digest"))};
  if(!std::filesystem::is_regular_file(std::filesystem::path(config.at("recoveryWitness").get<std::string>())/"CURRENT"))throw std::invalid_argument("Capella recovery commitment requires existing witness");
 }
 if(config.contains("automaticRecovery")&&!config.at("automaticRecovery").is_boolean())throw std::invalid_argument("Capella automaticRecovery must be boolean");
 const bool automatic=config.value("automaticRecovery",false);
 if(automatic&&(!witnessed||restart||config.at("initial").is_null()))throw std::invalid_argument("Automatic Capella recovery requires witness and bootstrap snapshot, without restart token");
 if(witnessed){if(restart)throw std::invalid_argument("Witness recovery cannot use a restart token");}
 else if(restart==!config.at("initial").is_null())throw std::invalid_argument("Choose exactly one Capella initial snapshot or restart token");
 auto owner=std::make_shared<Owner>();std::set<std::filesystem::path> paths;
 if(witnessed){const auto path=config.at("recoveryWitness").get<std::string>();if(path.empty())throw std::invalid_argument("Empty Capella recovery witness path");paths.insert(std::filesystem::weakly_canonical(path));owner->witness=std::make_unique<ChainStore>(path);}
 bool recover=witnessed&&config.at("initial").is_null();
 if(automatic){
  owner->witness->bind_chain(genesis.chain_id,genesis.block.header.hash(),"nano-capella-recovery-v1");
  if(limits.envelope_bytes>(SIZE_MAX-2048)/8)throw std::length_error("Capella witness size overflow");
  // Either publication boundary forbids falling back to bootstrap. The reactor
  // must verify the witness and its exact relationship to the primary store.
  const auto primary=chain.store().fork_checkpoint(genesis.chain_id,genesis.block.header.hash(),limits.envelope_bytes);
  const auto witness=owner->witness->fork_checkpoint(genesis.chain_id,genesis.block.header.hash(),2048+8*limits.envelope_bytes);
  recover=bool(primary)||bool(witness);
 }
 if(recovery_commitment&&!recover)throw std::invalid_argument("Capella recovery commitment forbids bootstrap");
 if(recover){
  owner->reactor=std::make_unique<CapellaDurableReactor>(chain,rules,limits,*owner->witness,checkpoint,recovery_commitment);
 }else if(restart){
  const auto& r=config.at("restart");shape(r,{"sequence","digest"});const ForkCheckpointToken token{number(r.at("sequence")),digest(r.at("digest"))};
  owner->reactor=std::make_unique<CapellaDurableReactor>(chain,rules,limits,token,checkpoint);
 }else{
  if(chain.store().fork_checkpoint(genesis.chain_id,genesis.block.header.hash(),limits.envelope_bytes))throw std::invalid_argument("Existing Capella checkpoint requires explicit restart token");
  const auto& a=config.at("initial");shape(a,{"file","sha256"});
  auto initial=capella_store_from_snapshot(read_bounded(a.at("file").get<std::string>(),limits.envelope_bytes,checkpoint),digest(a.at("sha256")),rules,limits,checkpoint);
  owner->reactor=std::make_unique<CapellaDurableReactor>(chain,rules,limits,std::move(initial),checkpoint,owner->witness.get());
 }
 if(config.contains("automaticPruning")&&!config.at("automaticPruning").is_boolean())throw std::invalid_argument("Capella automaticPruning must be boolean");
 owner->reactor->set_automatic_pruning(config.value("automaticPruning",true));
 if(config.contains("historicalStatePruning")&&!config.at("historicalStatePruning").is_boolean())throw std::invalid_argument("Capella historicalStatePruning must be boolean");
 owner->reactor->set_historical_state_pruning(config.value("historicalStatePruning",false));
 if(wall_drift){
  auto time=std::make_shared<LocalMonotonicTime>(*wall_drift,std::move(now));
  now=[time](){return time->now();};
 }else if(!now)now=[](){const auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();if(ms<0)throw std::runtime_error("Negative local time");return uint64_t(ms);};
 owner->clock=std::make_unique<CapellaLocalClock>(*owner->reactor,step,now);
 owner->clock->poll(checkpoint);chain.require_execution_ready();
 if(config.contains("admission")){
  const auto& a=config.at("admission");shape(a,{"frameBytes","blockBytes","attestationBytes","slashingBytes","jobs","payloadBytes"});
  const auto frame=number(a.at("frameBytes")),block=number(a.at("blockBytes")),vote=number(a.at("attestationBytes")),slash=number(a.at("slashingBytes")),jobs=number(a.at("jobs")),bytes=number(a.at("payloadBytes"));
  const auto committee=uint64_t(rules.transition.consensus.body.validators_per_committee),largest=std::max({block,vote,slash});
  if(vote<228+(committee+8)/8||slash<464+16*committee||block<rules.transition.consensus.maximum_block_bytes||bytes<largest||frame<1024||largest>(frame-1024)/2)throw std::invalid_argument("Capella admission cannot carry configured messages");
  owner->admission=std::make_unique<CapellaConsensusService>(*owner->reactor,owner->decoder_budget,frame,block,vote,slash,jobs,bytes,[clock=owner->clock.get()](const auto& check){clock->poll(check);});
 }
 if(config.contains("catalogs")){
  const auto& c=config.at("catalogs");shape(c,{"handles","rootBytes","ttlMs"});const auto handles=number(c.at("handles")),bytes=number(c.at("rootBytes")),ttl=number(c.at("ttlMs"));
  if(std::max({limits.blocks,limits.messages,limits.equivocations})>bytes/sizeof(ConsensusRoot))throw std::invalid_argument("Capella catalog budget below configured inventory capacity");
  if(ttl>uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::duration::max()/2).count()))throw std::invalid_argument("Capella catalog TTL range");
  owner->catalogs=std::make_unique<CapellaInventorySessions>(*owner->reactor,handles,bytes,std::chrono::milliseconds(ttl));
 }
 Json network=Json::object();
 if(config.contains("network")){
  const auto& n=config.at("network");shape(n,{"enabled","pageEntries","maximumEntries","dependencyBlocks","dependencyBytes","acknowledgedRoots"});
  if(!n.at("enabled").is_boolean())throw std::invalid_argument("Capella network enable flag");network["enabled"]=n.at("enabled");
  for(const auto* field:{"pageEntries","maximumEntries","dependencyBlocks","dependencyBytes","acknowledgedRoots"})network[field]=number(n.at(field));
  if(network.at("pageEntries")>network.at("maximumEntries"))throw std::invalid_argument("Capella network page exceeds scan budget");
  if(n.at("enabled")==true&&(!owner->admission||!owner->catalogs))throw std::invalid_argument("Capella network requires admission and catalogs");
  const auto& anchor=owner->reactor->current().state->timed.view.anchor;network["anchorRoot"]=hex(Bytes(anchor.begin(),anchor.end()));
 }
 if(config.contains("validator")){
  const auto& v=config.at("validator");shape(v,{"pollIntervalMs","maximumTransactionTrials","maximumVoteTrials","keys"});
  const auto poll=number(v.at("pollIntervalMs")),transactions=number(v.at("maximumTransactionTrials")),trials=number(v.at("maximumVoteTrials"));
  if(poll>rules.slot_duration_ms||!v.at("keys").is_array()||v.at("keys").empty())throw std::invalid_argument("Validator poll interval or keys");
  struct Secret {
   blst_scalar scalar{};Bytes public_key;
   ~Secret(){OPENSSL_cleanse(&scalar,sizeof(scalar));}
   Bytes sign(const ConsensusRoot& root)const{
    static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";
    blst_p2 point,result;blst_hash_to_g2(&point,root.data(),root.size(),reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);blst_sign_pk_in_g1(&result,&point,&scalar);
    Bytes signature(96);blst_p2_compress(signature.data(),&result);return signature;
   }
  };
  struct JournalOwner {std::unique_ptr<ChainStore> witness;std::unique_ptr<ChainStore> disk;std::unique_ptr<BeaconSigningJournal> journal;};
  const auto current=owner->reactor->current();const auto& anchor=current.state->blocks.at(current.state->timed.view.anchor).state.ssz;
  if(anchor.size()<40)throw std::invalid_argument("Missing authenticated validators genesis");ConsensusRoot validators_genesis{};std::copy_n(anchor.begin()+8,32,validators_genesis.begin());
  const auto& head=capella_fork_head(*current.state,rules,checkpoint);
  const auto registry=capella_fork_block_context(head.state,head.header,{rules.transition.consensus.epoch,rules.transition.execution,rules.transition.maximum_state_bytes},rules.transition.consensus.state,checkpoint).registry;
  std::vector<CapellaValidatorKey> keys;std::set<std::string> public_keys;
  for(const auto& k:v.at("keys")){
   if(checkpoint)checkpoint();auto fields=k;fields.erase("recoveryWitness");shape(fields,{"index","feeRecipient","keyEnv","journal","restart"});
   const bool automatic_index=k.at("index").is_null();uint64_t index=0;
   if(!automatic_index){if(!k.at("index").is_string())throw std::invalid_argument("Validator index quantity");index=u64(quantity(k.at("index")));if(quantity(Int(std::to_string(index)))!=k.at("index").get<std::string>())throw std::invalid_argument("Validator index encoding");}
   const auto fee=address(k.at("feeRecipient").get<std::string>()),env=k.at("keyEnv").get<std::string>();
   if(env.empty())throw std::invalid_argument("Validator signing environment name required");const char* raw=std::getenv(env.c_str());if(!raw)throw std::invalid_argument("Validator signing environment unavailable");
   std::string encoded(raw);struct Wipe {std::string& value;~Wipe(){OPENSSL_cleanse(value.data(),value.size());}} wipe{encoded};
   if(encoded.size()!=66||!encoded.starts_with("0x"))throw std::invalid_argument("Validator BLS scalar encoding");auto secret=std::make_shared<Secret>();auto scalar=unhex(encoded);if(scalar.size()!=32)throw std::invalid_argument("Validator BLS scalar width");
   blst_scalar_from_bendian(&secret->scalar,scalar.data());OPENSSL_cleanse(scalar.data(),scalar.size());if(!blst_sk_check(&secret->scalar))throw std::invalid_argument("Invalid validator BLS scalar");
   blst_p1 pk;blst_sk_to_pk_in_g1(&pk,&secret->scalar);secret->public_key.resize(48);blst_p1_compress(secret->public_key.data(),&pk);
   const auto member=registry.validators.find(index);
   if(!automatic_index&&(member==registry.validators.end()||Bytes(member->second.public_key.begin(),member->second.public_key.end())!=secret->public_key))throw std::invalid_argument("Local signing key does not match authenticated validator index");
   if(!public_keys.insert(hex(secret->public_key)).second)throw std::invalid_argument("Duplicate local validator key");
   const auto input_path=k.at("journal").get<std::string>();if(input_path.empty())throw std::invalid_argument("Empty validator journal path");const auto path=std::filesystem::weakly_canonical(input_path);if(path.empty()||!paths.insert(path).second)throw std::invalid_argument("Duplicate or empty validator journal path");
   std::optional<ForkCheckpointToken> restart;if(!k.at("restart").is_null()){const auto& r=k.at("restart");shape(r,{"sequence","digest"});restart=ForkCheckpointToken{number(r.at("sequence")),digest(r.at("digest"))};}
   auto journal=std::make_shared<JournalOwner>();
   if(k.contains("recoveryWitness")){
    const auto witness_path=k.at("recoveryWitness").get<std::string>();
    if(witness_path.empty()||!paths.insert(std::filesystem::weakly_canonical(witness_path)).second)throw std::invalid_argument("Duplicate or empty validator witness path");
    journal->witness=std::make_unique<ChainStore>(witness_path);
   }
   journal->disk=std::make_unique<ChainStore>(path);
   journal->journal=std::make_unique<BeaconSigningJournal>(*journal->disk,genesis.chain_id,genesis.block.header.hash(),validators_genesis,secret->public_key,rules.transition.consensus.epoch.ffg.slots_per_epoch,restart,journal->witness.get());
   keys.push_back({index,std::shared_ptr<BeaconSigningJournal>(journal,journal->journal.get()),[secret](const ConsensusRoot& root){return secret->sign(root);},fee,automatic_index?secret->public_key:Bytes{}});
  }
  owner->validator=std::make_unique<CapellaValidatorLoop>(*owner->reactor,*owner->clock,rules.slot_duration_ms,rules.transition.consensus.epoch.ffg.slots_per_epoch,transactions,trials,std::move(keys),now);
  if(const char* trace=std::getenv("NANO_TEST_NETWORK_TRACE");trace&&std::string_view(trace)=="1")owner->validator->set_duty_trace_limit(128);
  owner->validator->start(std::chrono::milliseconds(poll));
 }
 owner->clock->start(std::chrono::milliseconds(interval));
 auto sync_tracker=std::make_shared<BeaconSyncTracker>();
 return {owner->validator?std::shared_ptr<CapellaValidatorLoop>(owner,owner->validator.get()):nullptr,std::shared_ptr<CapellaDurableReactor>(owner,owner->reactor.get()),std::shared_ptr<CapellaLocalClock>(owner,owner->clock.get()),[owner,rules,sync_tracker,now](std::string_view path,const std::function<void()>& check){
  if(check)check();
  if(!owner->clock->healthy())throw std::runtime_error("Capella clock unavailable");
  if(path!="/eth/v1/node/syncing")return owner->reactor->beacon_read(path,check);
  const auto observation=sync_tracker->current();
  if(!observation||observation->peer_slots.empty())return BeaconReadResponse{503,{{"code",503},{"message","Beacon peer observation unavailable"}}};
  const auto snapshot=owner->reactor->current();
  const auto local_time=now(),genesis_time=snapshot.state->timed.clock.genesis_time_ms;
  if(local_time<snapshot.state->timed.clock.time_ms||local_time<genesis_time)throw std::runtime_error("Beacon clock discontinuity");
  const auto wall_slot=(local_time-genesis_time)/rules.slot_duration_ms;
  const auto& head=capella_fork_head(*snapshot.state,rules,check);uint64_t target=head.header.slot;
  if(target>wall_slot)throw std::runtime_error("Beacon head in future");
  for(const auto slot:observation->peer_slots){if(slot>wall_slot)throw std::runtime_error("Beacon peer claims future slot");target=std::max(target,slot);}
  if(check)check();
  return BeaconReadResponse{200,{{"data",{{"head_slot",std::to_string(head.header.slot)},{"sync_distance",std::to_string(target-head.header.slot)},{"is_syncing",target>head.header.slot},{"is_optimistic",false},{"el_offline",false}}}}};
 },owner->admission?std::shared_ptr<CapellaConsensusService>(owner,owner->admission.get()):nullptr,owner->catalogs?std::shared_ptr<CapellaInventorySessions>(owner,owner->catalogs.get()):nullptr,std::move(network),sync_tracker};
}
Json stop_capella_runtime(CapellaRuntime& runtime){
 if(!runtime.reactor||!runtime.clock)throw std::invalid_argument("Missing Capella runtime");
 if(runtime.catalogs)runtime.catalogs->stop();
 if(runtime.admission){runtime.admission->stop();runtime.admission->join();}
 if(runtime.validator)runtime.validator->stop();
 runtime.clock->stop();
 const auto current=runtime.reactor->current();
 return {{"restart",{{"sequence",quantity(Int(std::to_string(current.token.sequence)))},{"digest",hex(current.token.digest)}}},
  {"syncAdmission",runtime.reactor->sync_admission_stats()},
  {"validatorKeys",runtime.validator?runtime.validator->restart_commitments():Json::array()},
  {"finalized",{{"epoch",quantity(Int(std::to_string(current.state->timed.view.finalized.epoch)))},{"root",hex(Bytes(current.state->timed.view.finalized.root.begin(),current.state->timed.view.finalized.root.end()))}}},
  {"validator",runtime.validator?runtime.validator->status(true):Json(nullptr)},{"clockHealthy",runtime.clock->healthy()},
  {"justifiedEpoch",quantity(Int(std::to_string(current.state->timed.view.justified.epoch)))}};
}

}
