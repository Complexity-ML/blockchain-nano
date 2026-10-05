// Stopped private native-deposit fixture only. Read-only stores, external stop token
// and externally observed finality; never authorizes recovery or public slashing.
#include "nano/consensus_capella_audit.hpp"
#include "nano/consensus_capella_reactor.hpp"
#include "nano/consensus_capella_fork_state.hpp"
#include "nano/consensus_message_ssz.hpp"
#include "nano/chain.hpp"
#include "../../src/beacon_block_json.hpp"
#include "blst.h"
#include <fstream>
#include <iostream>
using namespace nano;
namespace {
void require(bool b,const char* message){if(!b)throw std::runtime_error(message);}
std::string read(const std::string& path,size_t maximum){require(std::filesystem::is_regular_file(path)&&std::filesystem::file_size(path)<=maximum,"audit file size/type");std::ifstream in(path,std::ios::binary);std::string bytes((std::istreambuf_iterator<char>(in)),{});require(!in.bad()&&bytes.size()<=maximum,"audit read budget");return bytes;}
Json json(const std::string& p){return Json::parse(read(p,1024*1024));}
uint64_t number(const Json& v){return u64(quantity(v));}
ConsensusRoot root(const Json& value){const auto text=value.get<std::string>();const auto bytes=unhex(text);require(bytes.size()==32&&hex(bytes)==text,"canonical audit root required");ConsensusRoot r{};std::copy(bytes.begin(),bytes.end(),r.begin());return r;}
std::string text(const ConsensusRoot& r){return hex(Bytes(r.begin(),r.end()));}
}
int main(int argc,char** argv){try{
 require(argc==7&&std::string(argv[1])=="--audit","usage: --audit synthetic-genesis node-config stopped-db external-audit-request deposit-metadata");
 const auto genesis=Genesis::from_json(json(argv[2]));require(genesis.chain_id==1337&&genesis.consensus=="beacon"&&genesis.evm_revision==EvmRevision::Shanghai,"private Beacon Shanghai fixture only");
 const auto c=json(argv[3]).at("capella"),request=json(argv[5]),deposit=json(argv[6]);
 require(deposit.at("kind")=="native_deposit_transaction"&&deposit.at("validatorIndex")==64&&deposit.at("amountGwei")==32000000000ull&&deposit.at("productionQualified")==false,"deposit fixture identity");
 require(deposit.at("withdrawalRecipient")=="0x000000000000000000000000000000000000002c","private withdrawal recipient required");
 const auto pk=unhex(deposit.at("publicKey"));require(pk.size()==48,"deposit public key width");
 uint64_t words[4]{65,0,0,0};blst_scalar scalar;blst_scalar_from_uint64(&scalar,words);blst_p1 point;blst_sk_to_pk_in_g1(&point,&scalar);std::array<uint8_t,48> public_key;blst_p1_compress(public_key.data(),&point);require(std::equal(pk.begin(),pk.end(),public_key.begin()),"known public fixture key65 required");
 const auto& l=c.at("limits");CapellaStoreSnapshotLimits limits{size_t(number(l.at("envelopeBytes"))),size_t(number(l.at("blocks"))),size_t(number(l.at("messages"))),size_t(number(l.at("equivocations"))),size_t(number(l.at("stateBytes"))),size_t(number(l.at("totalStateBytes"))),number(l.at("replaySlots")),number(l.at("syncTrials"))};
 require(limits.envelope_bytes<=128*1024*1024&&limits.blocks<=1024,"private audit snapshot budget");
 const auto& p=c.at("profile");const auto rules=load_capella_fork_profile(read(p.at("file"),1024*1024),unhex(p.at("sha256")),1024*1024);require(rules.transition.chain_id==genesis.chain_id&&rules.native_deposit_votes,"native deposit profile required");
 const auto& i=c.at("initial");const auto seed=capella_store_from_snapshot(read(i.at("file"),limits.envelope_bytes),unhex(i.at("sha256")),rules,limits);
 auto context=[&](const CapellaForkBlock& block){return capella_fork_block_context(block.state,block.header,{rules.transition.consensus.epoch,rules.transition.execution,rules.transition.maximum_state_bytes},rules.transition.consensus.state);};
 const auto initial=context(capella_fork_head(seed,rules));require(initial.registry.validators.size()==64,"initial64-member fixture required");
 for(const auto& member:initial.registry.validators)require(member.second.public_key!=public_key,"new key already registered in seed");
 // The standard runtime audit first verifies canonical signed transitions,
 // balances, receipts and externally observed checkpoint epoch/root binding.
 const auto canonical=audit_capella_database(genesis,argv[4],request);require(canonical.at("canonicalHistoryVerified")==true&&canonical.at("externalFinalizedAncestorVerified")==true,"canonical replay/finality binding");
 const auto& expected=request.at("expectedCheckpoint");const ForkCheckpointToken token{number(expected.at("sequence")),unhex(expected.at("digest"))};
 ChainStore disk(argv[4],ChainStore::OpenMode::ReadOnly);disk.bind_chain(genesis.chain_id,genesis.block.header.hash(),"beacon");
 CapellaDurableReactor reactor(disk,genesis.chain_id,genesis.block.header.hash(),rules,limits,token);const auto stopped=reactor.current();
 ChainStore witness(c.at("recoveryWitness"),ChainStore::OpenMode::ReadOnly);
 witness.bind_chain(genesis.chain_id,genesis.block.header.hash(),"nano-capella-recovery-v1");
 const auto saved=witness.fork_checkpoint(genesis.chain_id,genesis.block.header.hash(),2048+8*limits.envelope_bytes);require(bool(saved),"independent stopped witness missing");
 const auto intent=Json::parse(saved->snapshot);require(intent.is_object()&&intent.size()==5&&intent.at("format")=="nano-capella-recovery-v1"&&intent.dump()==saved->snapshot,"noncanonical witness intent");
 require(number(intent.at("next").at("sequence"))==token.sequence&&unhex(intent.at("next").at("digest"))==token.digest,"independent witness intent differs from external stop token");
 const auto encoded=intent.at("snapshot").get<std::string>();require(encoded.size()<=limits.envelope_bytes&&sha256(Bytes(encoded.begin(),encoded.end()))==token.digest,"witness snapshot differs from committed primary digest");

 const auto selected=root(request.at("requiredFinalized").at("root"));const auto at=stopped.state->blocks.find(selected);require(at!=stopped.state->blocks.end(),"observed finalized state missing");
 const auto final=context(at->second);require(final.registry.validators.size()==65,"finalized registry is not65 members");
 const auto& member=final.registry.validators.at(64);require(member.public_key==public_key&&!member.slashed,"finalized new member identity/status");
 ConsensusRoot withdrawal{};withdrawal[0]=1;withdrawal[31]=44;require(member.withdrawal_credentials==withdrawal,"finalized withdrawal credentials mismatch");
 const auto cutoff=number(request.at("restartCutoffSlot"));
 const auto before=root(request.at("proposalBefore").at("root")),after=root(request.at("proposalAfter").at("root"));require(before!=after,"distinct proposals required");
 const auto archive_blocks=number(request.at("maximumBlocks"));require(archive_blocks>0&&archive_blocks<=1024,"deposit archive block budget");
 size_t included=0,registered=0,executed=0;bool before_verified=false,after_verified=false;ConsensusRoot tip=selected;std::vector<std::pair<ConsensusRoot,Bytes>> reverse;
 while(tip!=seed.timed.view.anchor){
  require(reverse.size()<archive_blocks,"deposit ancestry budget");const auto archive=disk.checkpoint_archive(Bytes(tip.begin(),tip.end()),limits.envelope_bytes);require(bool(archive),"canonical signed deposit archive missing");const auto message=beacon_block_json(archive->payload,rules,{}).at("message");reverse.push_back({tip,archive->payload});tip=root(message.at("parent_root"));
 }
 std::reverse(reverse.begin(),reverse.end());auto replay=seed;Json registration;
 for(const auto& [id,wire]:reverse){
  const auto message=beacon_block_json(wire,rules,{}).at("message");const auto slot=number(message.at("slot"));
  require(rules.slot_duration_ms&&slot<=(UINT64_MAX-replay.timed.clock.genesis_time_ms)/rules.slot_duration_ms,"deposit replay clock overflow");
  replay=prepare_capella_store_tick(replay,replay.timed.clock.genesis_time_ms+slot*rules.slot_duration_ms,rules,limits.replay_slots,limits.sync_trials);
  const auto parent=root(message.at("parent_root"));require(replay.blocks.contains(parent),"replayed deposit parent missing");const auto prior=context(replay.blocks.at(parent));
  auto next=prepare_capella_fork_block(replay,wire,rules,limits.replay_slots,limits.sync_trials);require(next&&next->blocks.contains(id),"signed deposit transition rejected");const auto now=context(next->blocks.at(id));
  require(now.registry.validators.size()>=64&&now.registry.validators.size()<=65,"unexpected registry size on canonical ancestry");
  for(size_t k=0;k<64;++k)require(now.registry.validators.at(k).public_key==initial.registry.validators.at(k).public_key,"old registry identity changed");
  if(now.registry.validators.size()==65){require(now.registry.validators.at(64).public_key==public_key,"wrong new registry member");if(prior.registry.validators.size()==64){++registered;registration={{"slot",slot},{"root",text(id)}};}}
  require(prior.registry.validators.size()!=65||now.registry.validators.size()==65,"registry registration reverted");
  for(const auto& tx:message.at("body").at("execution_payload").at("transactions"))if(tx==deposit.at("raw"))++executed;
  for(const auto& op:message.at("body").at("deposits"))if(op.at("data").at("pubkey")==deposit.at("publicKey")){require(number(op.at("data").at("amount"))==32000000000ull,"deposit amount mismatch");++included;}
  if(id==before||id==after){require(number(message.at("proposer_index"))==64,"selected block not proposed by new member");require(now.registry.validators.size()==65&&now.registry.validators.at(64).activation_epoch<=slot/rules.transition.consensus.epoch.ffg.slots_per_epoch,"new proposer not activated");if(id==before){require(slot<=cutoff,"pre-restart proposal after cutoff");before_verified=true;}else{require(slot>cutoff,"post-restart proposal before cutoff");after_verified=true;}}
  replay=std::move(*next);
 }
 require(replay.blocks.contains(selected)&&replay.blocks.at(selected).state==at->second.state,"replayed finalized deposit state differs from retained state");
 require(included==1&&registered==1&&executed==1&&before_verified&&after_verified,"deposit/registration/proposals incomplete");require(reactor.current().token==token,"audit checkpoint changed");
 std::cout<<Json{{"canonicalReplay",canonical},{"requiredFinalized",request.at("requiredFinalized")},{"newValidator",64},{"initialRegistrySize",64},{"finalizedRegistrySize",65},{"canonicalSignedDepositTransactions",executed},{"canonicalDepositInclusions",included},{"canonicalRegistrationTransitions",registered},{"registration",registration},{"activationEpoch",member.activation_epoch},{"activationEligibilityEpoch",member.activation_eligibility_epoch},{"finalizedBalanceGwei",member.balance},{"signedFinalizedProposalBeforeRestart",before_verified},{"signedFinalizedProposalAfterRestart",after_verified},{"externalStopTokenAndWitnessVerified",true},{"perKeyJournalWitnessAuditQualified",false},{"readOnlyStores",true},{"productionQualified",false}}.dump(2)<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
