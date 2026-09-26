#include "nano/consensus_bellatrix_runtime.hpp"
#include <fstream>
#include <cstdlib>
#include <set>
#include <openssl/crypto.h>
#include "blst.h"
#include "../network/bellatrix_rpc_service.hpp"
namespace nano {
namespace {
void shape(const Json& j,std::initializer_list<const char*> keys){
 if(!j.is_object()||j.size()!=keys.size())throw std::invalid_argument("Bellatrix runtime fields");
 for(auto key:keys)if(!j.contains(key))throw std::invalid_argument("Missing Bellatrix runtime field");
}
uint64_t number(const Json& j){
 if(!j.is_string())throw std::invalid_argument("Bellatrix runtime quantity required");
 const auto n=quantity(j);const auto value=u64(n);
 if(!value||quantity(n)!=j.get<std::string>())throw std::invalid_argument("Bellatrix runtime positive canonical quantity required");return value;
}
Bytes digest(const Json& j){
 if(!j.is_string()||j.get_ref<const std::string&>().size()!=66)throw std::invalid_argument("Bellatrix digest width");
 const auto b=unhex(j.get<std::string>());if(b.size()!=32||hex(b)!=j.get<std::string>())throw std::invalid_argument("Bellatrix digest encoding");return b;
}
std::string read_bounded(const std::filesystem::path& path,size_t maximum,const std::function<void()>& checkpoint){
 if(!std::filesystem::is_regular_file(path)||std::filesystem::file_size(path)>maximum)throw std::invalid_argument("Bellatrix input size/type");
 std::ifstream input(path,std::ios::binary);if(!input)throw std::runtime_error("Cannot read Bellatrix input");
 std::string out;std::array<char,16384> buffer;
 while(input){if(checkpoint)checkpoint();input.read(buffer.data(),buffer.size());const auto n=size_t(input.gcount());if(n>maximum-out.size())throw std::length_error("Bellatrix input grew beyond bound");out.append(buffer.data(),n);}
 if(input.bad()||!input.eof())throw std::runtime_error("Bellatrix input read failed");return out;
}
struct Owner {std::unique_ptr<BellatrixDurableReactor> reactor;std::unique_ptr<BellatrixLocalClock> clock;std::unique_ptr<BellatrixRpcService> rpc;std::unique_ptr<BellatrixValidatorLoop> validator;};
}
BellatrixRuntime start_bellatrix_runtime(Blockchain& chain,const Json& config,const std::function<void()>& checkpoint,std::function<uint64_t()> now){
 if(checkpoint)checkpoint();auto base=config;base.erase("validator");shape(base,{"profile","limits","initial","restart","clock","wire","worker"});
 const auto& genesis=chain.genesis();
 if(genesis.consensus!="beacon"||genesis.evm_revision!=EvmRevision::Paris)throw std::invalid_argument("Bellatrix runtime requires Beacon Paris chain");
 const auto& p=config.at("profile");shape(p,{"file","sha256","maximumBytes"});const auto maximum=number(p.at("maximumBytes"));
 const auto rules=load_bellatrix_fork_profile(read_bounded(p.at("file").get<std::string>(),maximum,checkpoint),digest(p.at("sha256")),maximum);
 if(rules.transition.chain_id!=genesis.chain_id||rules.transition.replay_policy!=ReplayPolicy::RequireChainId)throw std::invalid_argument("Bellatrix network requires matching chain ID and protected transactions");
 const auto& l=config.at("limits");shape(l,{"envelopeBytes","blocks","messages","equivocations","stateBytes","totalStateBytes","replaySlots","syncTrials"});
 const BellatrixStoreSnapshotLimits limits{number(l.at("envelopeBytes")),number(l.at("blocks")),number(l.at("messages")),number(l.at("equivocations")),number(l.at("stateBytes")),number(l.at("totalStateBytes")),number(l.at("replaySlots")),number(l.at("syncTrials"))};
 if(limits.state_bytes<rules.transition.maximum_state_bytes||limits.total_state_bytes<limits.state_bytes)throw std::invalid_argument("Bellatrix state capacity below configured transition");
 const auto& c=config.at("clock");shape(c,{"intervalMs","maximumForwardStepMs"});const auto interval=number(c.at("intervalMs")),step=number(c.at("maximumForwardStepMs"));
 if(interval>uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::duration::max()/2).count()))throw std::invalid_argument("Bellatrix clock interval range");
 const auto& w=config.at("wire");shape(w,{"frameBytes","blockBytes","attestationBytes","slashingBytes"});
 const auto frame=number(w.at("frameBytes")),block=number(w.at("blockBytes")),vote=number(w.at("attestationBytes")),slash=number(w.at("slashingBytes"));
 const auto& worker=config.at("worker");shape(worker,{"jobs","payloadBytes","timeoutMs"});
 const auto jobs=number(worker.at("jobs")),payload=number(worker.at("payloadBytes")),timeout=number(worker.at("timeoutMs"));
 if(timeout>uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::duration::max()/2).count()))throw std::invalid_argument("Bellatrix IPC timeout range");
 const auto committee=uint64_t(rules.transition.consensus.body.validators_per_committee),largest=std::max({block,vote,slash});
 // Reserve room for the complete six-field identity and private RPC wrapper.
 // This IPC bound is an implementation capacity, not a validator registry cap.
 if(vote<228+(committee+8)/8||slash<464+16*committee||block<rules.transition.consensus.maximum_block_bytes||payload<largest||frame<1024||largest>(frame-1024)/2||largest/3+(largest%3!=0)>(16*1024*1024-2048)/4)
  throw std::invalid_argument("Bellatrix transport cannot carry configured consensus messages");
 const bool restart=!config.at("restart").is_null();
 if(restart==!config.at("initial").is_null())throw std::invalid_argument("Choose exactly one Bellatrix initial snapshot or restart token");
 auto owner=std::make_shared<Owner>();
 if(restart){
  const auto& r=config.at("restart");shape(r,{"sequence","digest"});const ForkCheckpointToken token{number(r.at("sequence")),digest(r.at("digest"))};
  owner->reactor=std::make_unique<BellatrixDurableReactor>(chain,rules,limits,token,checkpoint);
 }else{
  if(chain.store().fork_checkpoint(genesis.chain_id,genesis.block.header.hash(),limits.envelope_bytes))throw std::invalid_argument("Existing Bellatrix checkpoint requires explicit restart token");
  const auto& a=config.at("initial");shape(a,{"file","sha256"});
  auto initial=bellatrix_store_from_snapshot(read_bounded(a.at("file").get<std::string>(),limits.envelope_bytes,checkpoint),digest(a.at("sha256")),rules,limits,checkpoint);
  owner->reactor=std::make_unique<BellatrixDurableReactor>(chain,rules,limits,std::move(initial),checkpoint);
 }
 if(!now)now=[](){const auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();if(ms<0)throw std::runtime_error("Negative local time");return uint64_t(ms);};
 owner->clock=std::make_unique<BellatrixLocalClock>(*owner->reactor,step,now);
 owner->clock->poll(checkpoint);chain.require_execution_ready();
 const auto identity=owner->reactor->network_identity();
 const AltairWireProfile wire{identity.at("chainId").get<std::string>(),identity.at("genesisHash").get<std::string>(),identity.at("forkDigest").get<std::string>(),frame,block,vote,slash};
 owner->rpc=std::make_unique<BellatrixRpcService>(*owner->reactor,wire,jobs,payload,std::chrono::milliseconds(timeout),[clock=owner->clock.get()](const auto& check){clock->poll(check);},[clock=owner->clock.get()]{return clock->healthy();},identity.at("consensusIdentity").get<std::string>(),true);
 if(config.contains("validator")){
  const auto& v=config.at("validator");shape(v,{"pollIntervalMs","maximumVoteTrials","keys"});
  const auto poll=number(v.at("pollIntervalMs")),trials=number(v.at("maximumVoteTrials"));
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
  struct JournalOwner {std::unique_ptr<ChainStore> disk;std::unique_ptr<BeaconSigningJournal> journal;};
  const auto current=owner->reactor->current();const auto& anchor=current.state->blocks.at(current.state->timed.view.anchor).state.ssz;
  if(anchor.size()<40)throw std::invalid_argument("Missing authenticated validators genesis");ConsensusRoot validators_genesis{};std::copy_n(anchor.begin()+8,32,validators_genesis.begin());
  const auto& head=bellatrix_fork_head(*current.state,rules,checkpoint);
  const auto registry=bellatrix_fork_block_context(head.state,head.header,{rules.transition.consensus.epoch,rules.transition.execution,rules.transition.maximum_state_bytes},rules.transition.consensus.state,checkpoint).registry;
  std::vector<BellatrixValidatorKey> keys;std::set<std::filesystem::path> paths;std::set<std::string> public_keys;
  for(const auto& k:v.at("keys")){
   if(checkpoint)checkpoint();shape(k,{"index","feeRecipient","keyEnv","journal","restart"});
   if(!k.at("index").is_string())throw std::invalid_argument("Validator index quantity");const auto index=u64(quantity(k.at("index")));if(quantity(Int(std::to_string(index)))!=k.at("index").get<std::string>())throw std::invalid_argument("Validator index encoding");
   const auto fee=address(k.at("feeRecipient").get<std::string>()),env=k.at("keyEnv").get<std::string>();
   if(env.empty())throw std::invalid_argument("Validator signing environment name required");const char* raw=std::getenv(env.c_str());if(!raw)throw std::invalid_argument("Validator signing environment unavailable");
   std::string encoded(raw);struct Wipe {std::string& value;~Wipe(){OPENSSL_cleanse(value.data(),value.size());}} wipe{encoded};
   if(encoded.size()!=66||!encoded.starts_with("0x"))throw std::invalid_argument("Validator BLS scalar encoding");auto secret=std::make_shared<Secret>();auto scalar=unhex(encoded);if(scalar.size()!=32)throw std::invalid_argument("Validator BLS scalar width");
   blst_scalar_from_bendian(&secret->scalar,scalar.data());OPENSSL_cleanse(scalar.data(),scalar.size());if(!blst_sk_check(&secret->scalar))throw std::invalid_argument("Invalid validator BLS scalar");
   blst_p1 pk;blst_sk_to_pk_in_g1(&pk,&secret->scalar);secret->public_key.resize(48);blst_p1_compress(secret->public_key.data(),&pk);
   const auto member=registry.validators.find(index);
   if(member==registry.validators.end()||Bytes(member->second.public_key.begin(),member->second.public_key.end())!=secret->public_key)throw std::invalid_argument("Local signing key does not match authenticated validator index");
   if(!public_keys.insert(hex(secret->public_key)).second)throw std::invalid_argument("Duplicate local validator key");
   const auto input_path=k.at("journal").get<std::string>();if(input_path.empty())throw std::invalid_argument("Empty validator journal path");const auto path=std::filesystem::weakly_canonical(input_path);if(path.empty()||!paths.insert(path).second)throw std::invalid_argument("Duplicate or empty validator journal path");
   std::optional<ForkCheckpointToken> restart;if(!k.at("restart").is_null()){const auto& r=k.at("restart");shape(r,{"sequence","digest"});restart=ForkCheckpointToken{number(r.at("sequence")),digest(r.at("digest"))};}
   auto journal=std::make_shared<JournalOwner>();journal->disk=std::make_unique<ChainStore>(path);
   journal->journal=std::make_unique<BeaconSigningJournal>(*journal->disk,genesis.chain_id,genesis.block.header.hash(),validators_genesis,secret->public_key,rules.transition.consensus.epoch.ffg.slots_per_epoch,restart);
   keys.push_back({index,std::shared_ptr<BeaconSigningJournal>(journal,journal->journal.get()),[secret](const ConsensusRoot& root){return secret->sign(root);},fee});
  }
  owner->validator=std::make_unique<BellatrixValidatorLoop>(*owner->reactor,*owner->clock,rules.slot_duration_ms,rules.transition.consensus.epoch.ffg.slots_per_epoch,trials,std::move(keys),now);
  owner->rpc->set_validator_status([loop=owner->validator.get()]{return loop->status();});
  owner->validator->start(std::chrono::milliseconds(poll));
 }
 owner->clock->start(std::chrono::milliseconds(interval));
 return {std::shared_ptr<BellatrixRpcService>(owner,owner->rpc.get()),owner->validator?std::shared_ptr<BellatrixValidatorLoop>(owner,owner->validator.get()):nullptr,std::shared_ptr<BellatrixDurableReactor>(owner,owner->reactor.get()),std::shared_ptr<BellatrixLocalClock>(owner,owner->clock.get())};
}
Json stop_bellatrix_runtime(BellatrixRuntime& runtime){
 if(!runtime.reactor||!runtime.clock||!runtime.rpc)throw std::invalid_argument("Missing Bellatrix runtime");
 if(runtime.validator)runtime.validator->stop();
 runtime.clock->stop();runtime.rpc->stop();runtime.rpc->join();
 const auto current=runtime.reactor->current();
 return {{"restart",{{"sequence",quantity(Int(std::to_string(current.token.sequence)))},{"digest",hex(current.token.digest)}}},
  {"validatorKeys",runtime.validator?runtime.validator->restart_commitments():Json::array()},
  {"finalized",{{"epoch",quantity(Int(std::to_string(current.state->timed.view.finalized.epoch)))},{"root",hex(Bytes(current.state->timed.view.finalized.root.begin(),current.state->timed.view.finalized.root.end()))}}},
  {"validator",runtime.validator?runtime.validator->status():Json(nullptr)},{"clockHealthy",runtime.clock->healthy()},
  {"justifiedEpoch",quantity(Int(std::to_string(current.state->timed.view.justified.epoch)))}};
}

}
