// Offline private-fixture recovery only: no RPC, P2P, signing, or validator duties.
#include "nano/consensus_capella_reactor.hpp"
#include "nano/chain.hpp"
#include "private_capture_budget.hpp"
#include <fstream>
#include <iostream>
using namespace nano;
namespace {
void need(bool value,const char* message){if(!value)throw std::runtime_error(message);}
Json read_json(const std::filesystem::path& path){need(std::filesystem::is_regular_file(path)&&std::filesystem::file_size(path)<=1024*1024,"copy recovery input bounds");std::ifstream input(path);Json value;input>>value;return value;}
ForkCheckpointToken token(const Json& j){need(j.is_object()&&j.size()==2,"copy recovery token shape");ForkCheckpointToken t{u64(quantity(j.at("sequence"))),unhex(j.at("digest"))};need(t.sequence&&t.digest.size()==32,"copy recovery token bounds");return t;}
Json wire(const ForkCheckpointToken& t){return {{"sequence",quantity(Int(std::to_string(t.sequence)))},{"digest",hex(t.digest)}};}
}
int main(int argc,char** argv){try{
 need(argc==2&&std::string(argv[1])=="--recover-synthetic-copy","private offline copy recovery requires explicit mode");
 const auto proof=read_json("/recovery/copy-proof.json"),request=read_json("/recovery/request.json");
 need(proof.at("independentCopyVerified")==true&&proof.at("sourceAllWritersStopped")==true&&proof.at("sourceVolume")!=proof.at("copyVolume"),"independent stopped copy required");
 const auto genesis=Genesis::from_json(read_json("/config/genesis.json"));need(genesis.chain_id==1337&&genesis.consensus=="beacon"&&genesis.evm_revision==EvmRevision::Shanghai,"private Shanghai fixture only");
 const auto config=read_json("/recovery/config.json").at("capella"),limits_json=config.at("limits");
 need(config.at("initial").is_null()&&config.value("automaticRecovery",true)==false&&config.at("recoveryWitness")=="/recovery/data/witness","copy recovery configuration binding");
 private_capture::resource_budget(limits_json,request);need(request.at("limits")==limits_json&&request.at("profile")==Json{{"file",config.at("profile").at("file")},{"sha256",config.at("profile").at("sha256")}},"external copy request binding");
 auto number=[](const Json& j){return u64(quantity(j));};
 const CapellaStoreSnapshotLimits limits{size_t(number(limits_json.at("envelopeBytes"))),size_t(number(limits_json.at("blocks"))),size_t(number(limits_json.at("messages"))),size_t(number(limits_json.at("equivocations"))),size_t(number(limits_json.at("stateBytes"))),size_t(number(limits_json.at("totalStateBytes"))),number(limits_json.at("replaySlots")),number(limits_json.at("syncTrials"))};
 const auto profile=config.at("profile");need(profile.at("file")=="/config/profile.json","copy profile path binding");std::ifstream profile_file("/config/profile.json");const std::string encoded((std::istreambuf_iterator<char>(profile_file)),{});need(encoded.size()<=1024*1024,"copy profile bounds");
 const auto rules=load_capella_fork_profile(encoded,unhex(profile.at("sha256")),1024*1024);
 Blockchain owner("/recovery/data/chain",genesis);ChainStore witness("/recovery/data/witness");witness.bind_chain(genesis.chain_id,genesis.block.header.hash(),"nano-capella-recovery-v1");
 const auto primary=owner.store().fork_checkpoint(genesis.chain_id,genesis.block.header.hash(),limits.envelope_bytes);const auto intent=witness.fork_checkpoint(genesis.chain_id,genesis.block.header.hash(),2048+8*limits.envelope_bytes);need(primary&&intent,"copy recovery stores missing");
 const auto floor=token(request.at("minimumCheckpoint"));need(primary->token.sequence>=floor.sequence&&(primary->token.sequence!=floor.sequence||primary->token.digest==floor.digest),"copy below external checkpoint floor");
 const auto next=token(Json::parse(intent->snapshot).at("next"));
 CapellaDurableReactor reactor(owner,rules,limits,witness,std::function<void()>{},next);
 need(reactor.current().token==next,"copy recovery did not finish witnessed intent");
 std::cout<<Json{{"scenario","private-capella-offline-copy-recovery"},{"beforePrimary",wire(primary->token)},{"witnessNext",wire(next)},{"afterPrimary",wire(reactor.current().token)},{"copiedStoresOnly",true},{"validatorDutiesStarted",false},{"signingInvoked",false},{"canonicalReplayStillRequired",true},{"terminalExternalPinsAvailable",false},{"networkQualified",false},{"productionQualified",false}}.dump(2)<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
