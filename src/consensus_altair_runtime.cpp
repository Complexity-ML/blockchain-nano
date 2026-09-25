#include "nano/consensus_altair_runtime.hpp"
#include "nano/consensus_altair_anchor.hpp"
#include "nano/consensus_altair_clock.hpp"
#include "../network/altair_rpc_service.hpp"
#include <fstream>
namespace nano {
namespace {
void shape(const Json& j,std::initializer_list<const char*> keys){if(!j.is_object()||j.size()!=keys.size())throw std::invalid_argument("Altair runtime configuration fields");for(auto key:keys)if(!j.contains(key))throw std::invalid_argument("Missing Altair runtime field");}
uint64_t num(const Json& j,bool positive=true){if(!j.is_string())throw std::invalid_argument("Altair runtime quantity required");const auto n=quantity(j);const auto value=u64(n);if(quantity(n)!=j.get<std::string>()||(positive&&!value))throw std::invalid_argument("Altair runtime quantity encoding/range");return value;}
Bytes fixed(const Json& j,size_t size){if(!j.is_string()||j.get_ref<const std::string&>().size()!=2+2*size)throw std::invalid_argument("Altair runtime commitment width");auto b=unhex(j.get<std::string>());if(b.size()!=size||hex(b)!=j.get<std::string>())throw std::invalid_argument("Altair runtime commitment encoding");return b;}
template<size_t N>std::array<uint8_t,N> root(const Json& j){const auto b=fixed(j,N);std::array<uint8_t,N> a;std::copy(b.begin(),b.end(),a.begin());return a;}
std::string profile_file(const std::filesystem::path& file,size_t maximum){
 if(!std::filesystem::is_regular_file(file)||std::filesystem::file_size(file)>maximum)throw std::invalid_argument("Altair profile file size/type");
 std::ifstream stream(file,std::ios::binary);if(!stream)throw std::runtime_error("Cannot open Altair profile");std::string out;std::array<char,16384> part;
 while(stream){stream.read(part.data(),part.size());const auto n=static_cast<size_t>(stream.gcount());if(n>maximum-out.size())throw std::length_error("Altair profile grew beyond bound");out.append(part.data(),n);}
 if(!stream.eof()||stream.bad())throw std::runtime_error("Cannot read Altair profile");return out;
}
struct Owner {std::unique_ptr<ChainStore> disk;std::unique_ptr<AltairDurableReactor> reactor;std::unique_ptr<AltairLocalClock> clock;std::unique_ptr<AltairRpcService> rpc;};
}
AltairRuntime start_altair_runtime(const Json& config,const Int& chain_id,const Bytes& genesis,const std::function<void()>& checkpoint,std::function<uint64_t()> local_clock){
 if(checkpoint)checkpoint();if(chain_id<0||genesis.size()!=32)throw std::invalid_argument("Invalid Altair runtime chain identity");
 shape(config,{"profile","forkChoice","anchor","database","limits","wire","worker","restart","clock"});
 const auto& clock=config.at("clock");shape(clock,{"enabled","intervalMs","maximumForwardStepMs"});
 if(!clock.at("enabled").is_boolean())throw std::invalid_argument("Altair clock enabled must be boolean");
 const auto interval=num(clock.at("intervalMs")),maximum_step=num(clock.at("maximumForwardStepMs"));
 if(interval>uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::duration::max()/2).count()))throw std::invalid_argument("Altair clock interval range");
 const auto& p=config.at("profile");shape(p,{"file","sha256","maximumBytes"});const auto profile_limit=num(p.at("maximumBytes"));
 const auto transition=load_altair_transition_profile(profile_file(p.at("file").get<std::string>(),profile_limit),fixed(p.at("sha256"),32),profile_limit);
 const auto& f=config.at("forkChoice");shape(f,{"slotDurationMs","attestationDueMs","proposerScoreBoost","boostProfile"});
 const auto boost=f.at("boostProfile").get<std::string>();if(boost!="release-1.6.0-beta.0"&&boost!="shuffling-dependent")throw std::invalid_argument("Unknown Altair proposer boost profile");
 const AltairForkBlockRules rules{transition,num(f.at("slotDurationMs")),num(f.at("attestationDueMs"),false),num(f.at("proposerScoreBoost"),false),boost=="shuffling-dependent"?AltairProposerBoostProfile::ShufflingDependent:AltairProposerBoostProfile::Release160Beta0};
 if(rules.attestation_due_ms>rules.slot_duration_ms)throw std::invalid_argument("Invalid Altair attestation deadline");
 const auto& a=config.at("anchor");shape(a,{"stateFile","blockFile","blockRoot","stateRoot","genesisValidatorsRoot","forkVersion"});
 const AltairAnchorCommitment anchor{root<32>(a.at("blockRoot")),root<32>(a.at("stateRoot")),root<32>(a.at("genesisValidatorsRoot")),root<4>(a.at("forkVersion"))};
 const auto& l=config.at("limits");shape(l,{"envelopeBytes","blocks","messages","equivocations","stateBytes","totalStateBytes","replaySlots","syncTrials"});
 const AltairStoreSnapshotLimits limits{num(l.at("envelopeBytes")),num(l.at("blocks")),num(l.at("messages")),num(l.at("equivocations")),num(l.at("stateBytes")),num(l.at("totalStateBytes")),num(l.at("replaySlots")),num(l.at("syncTrials"))};
 const auto& w=config.at("wire");shape(w,{"frameBytes","blockBytes","attestationBytes","slashingBytes"});
 const auto fork=consensus_fork_data_root(anchor.fork_version,anchor.genesis_validators_root);
 const AltairWireProfile wire{quantity(chain_id),hex(genesis),hex(Bytes(fork.begin(),fork.begin()+4)),num(w.at("frameBytes")),num(w.at("blockBytes")),num(w.at("attestationBytes")),num(w.at("slashingBytes"))};
 const auto& worker=config.at("worker");shape(worker,{"jobs","payloadBytes","timeoutMs"});const auto jobs=num(worker.at("jobs")),payload=num(worker.at("payloadBytes")),timeout=num(worker.at("timeoutMs"));
 if(timeout>uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::duration::max()/2).count()))throw std::invalid_argument("Altair timeout range");
 std::optional<ForkCheckpointToken> restart;
 if(!config.at("restart").is_null()){const auto& j=config.at("restart");shape(j,{"sequence","digest"});restart=ForkCheckpointToken{num(j.at("sequence")),fixed(j.at("digest"),32)};}
 const auto state_file=a.at("stateFile").get<std::string>(),block_file=a.at("blockFile").get<std::string>();
 const auto committee=uint64_t(transition.body.validators_per_committee);
 if(wire.attestation_bytes<228+(committee+8)/8||wire.slashing_bytes<464+16*committee)throw std::invalid_argument("Altair wire bounds cannot carry the configured committee");
 const auto largest=std::max({wire.block_bytes,wire.attestation_bytes,wire.slashing_bytes});
 const auto envelope=Json{{"chainId",wire.chain_id},{"genesisHash",wire.genesis_hash},{"forkDigest",wire.fork_digest},{"kind","attester_slashing"},{"ssz","0x"}}.dump().size();
 if(payload<largest||largest>(SIZE_MAX-envelope)/2||wire.frame_bytes<envelope+2*largest||wire.block_bytes<transition.maximum_block_bytes||limits.total_state_bytes<limits.state_bytes)throw std::invalid_argument("Inconsistent Altair runtime resource bounds");
 const std::filesystem::path database=config.at("database").get<std::string>();if(database.empty())throw std::invalid_argument("Empty Altair database path");
 std::optional<AltairForkStore> initial;
 if(!restart)initial=load_altair_anchor(state_file,block_file,anchor,rules,limits.state_bytes,checkpoint);
 else if(!std::filesystem::is_directory(database))throw std::invalid_argument("Missing Altair restart database");
 // Validate local wire configuration before opening/writing storage.
 InboundBudget budget;AltairMessageIngress decoder(wire,budget);
 if(checkpoint)checkpoint();auto owner=std::make_shared<Owner>();owner->disk=std::make_unique<ChainStore>(database);owner->disk->bind_chain(chain_id,genesis,"altair");
 if(restart){
  owner->reactor=std::make_unique<AltairDurableReactor>(*owner->disk,chain_id,genesis,rules,limits,*restart,checkpoint);
  const auto saved=owner->reactor->current().state;const auto& saved_anchor=saved->blocks.at(saved->timed.view.anchor);const auto& s=saved_anchor.state.ssz;
  if(saved->timed.view.anchor!=anchor.block_root||saved_anchor.header.state_root!=anchor.state_root||s.size()<56||!std::equal(anchor.genesis_validators_root.begin(),anchor.genesis_validators_root.end(),s.begin()+8)||!std::equal(anchor.fork_version.begin(),anchor.fork_version.end(),s.begin()+52))throw std::invalid_argument("Altair restart anchor mismatch");
 }else{
  if(owner->disk->fork_checkpoint(chain_id,genesis,limits.envelope_bytes))throw std::invalid_argument("Existing Altair checkpoint requires explicit restart token");
  owner->reactor=std::make_unique<AltairDurableReactor>(*owner->disk,chain_id,genesis,rules,limits,std::move(*initial),checkpoint);
 }
 std::function<void(const std::function<void()>&)> before_message;std::function<bool()> clock_healthy;
 if(clock.at("enabled").get<bool>()){
  owner->clock=std::make_unique<AltairLocalClock>(*owner->reactor,maximum_step,std::move(local_clock));owner->clock->poll(checkpoint);
  before_message=[local=owner->clock.get()](const auto& check){local->poll(check);};clock_healthy=[local=owner->clock.get()]{return local->healthy();};
 }
 owner->rpc=std::make_unique<AltairRpcService>(*owner->reactor,wire,jobs,payload,std::chrono::milliseconds(timeout),std::move(before_message),std::move(clock_healthy));
 if(owner->clock)owner->clock->start(std::chrono::milliseconds(interval));
 return {std::shared_ptr<AltairRpcService>(owner,owner->rpc.get()),std::shared_ptr<AltairDurableReactor>(owner,owner->reactor.get())};
}
}
