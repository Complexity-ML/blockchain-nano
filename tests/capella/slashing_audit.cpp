// Stopped private fixture: original member0 or admitted member64. External stop token
// and externally observed finality; never authorizes recovery or public slashing.
#include "nano/consensus_capella_audit.hpp"
#include "nano/consensus_capella_reactor.hpp"
#include "nano/consensus_capella_fork_state.hpp"
#include "nano/consensus_message_ssz.hpp"
#include "nano/chain.hpp"
#include "nano/consensus_bls.hpp"
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
 require(argc==7&&std::string(argv[1])=="--audit","usage: --audit synthetic-genesis node-config stopped-db external-audit-request generated-proof");
 const auto genesis=Genesis::from_json(json(argv[2]));require(genesis.chain_id==1337&&genesis.consensus=="beacon"&&genesis.evm_revision==EvmRevision::Shanghai,"private Beacon Shanghai fixture only");
 const auto c=json(argv[3]).at("capella"),request=json(argv[5]),proof=json(argv[6]);
 require(proof.at("format")=="nano-private-slashing-fixture-v1"&&proof.at("chainId")=="0x539"&&proof.at("genesisHash")==hex(genesis.block.header.hash())&&((proof.at("offender")==0&&proof.at("syntheticValidators")==64)||(proof.at("offender")==64&&proof.at("syntheticValidators")==65))&&proof.at("productionQualified")==false,"proof fixture identity");
 const auto offender=number(proof.at("offender"));const auto& l=c.at("limits");CapellaStoreSnapshotLimits limits{size_t(number(l.at("envelopeBytes"))),size_t(number(l.at("blocks"))),size_t(number(l.at("messages"))),size_t(number(l.at("equivocations"))),size_t(number(l.at("stateBytes"))),size_t(number(l.at("totalStateBytes"))),number(l.at("replaySlots")),number(l.at("syncTrials"))};
 require(limits.envelope_bytes<=128*1024*1024&&limits.blocks<=1024,"private audit snapshot budget");
 const auto& p=c.at("profile");const auto rules=load_capella_fork_profile(read(p.at("file"),1024*1024),unhex(p.at("sha256")),1024*1024);
 require(rules.transition.chain_id==genesis.chain_id,"audit chain/profile binding");
 const auto& i=request.contains("anchor")?request.at("anchor"):c.at("initial");const auto seed=capella_store_from_snapshot(read(i.at("file"),limits.envelope_bytes),unhex(i.at("sha256")),rules,limits);
 auto context=[&](const CapellaForkBlock& block){return capella_fork_block_context(block.state,block.header,{rules.transition.consensus.epoch,rules.transition.execution,rules.transition.maximum_state_bytes},rules.transition.consensus.state);};
 const auto initial=context((*capella_fork_head(seed,rules)));require(initial.registry.validators.size()==64&&!initial.registry.validators.at(0).slashed,"initial unslashed64-member fixture required");
 uint64_t words[4]{offender+1,0,0,0};blst_scalar scalar;blst_scalar_from_uint64(&scalar,words);blst_p1 point;blst_sk_to_pk_in_g1(&point,&scalar);std::array<uint8_t,48> public_key;blst_p1_compress(public_key.data(),&point);
 if(offender==0)require(initial.registry.validators.at(0).public_key==public_key,"known fixture public key0 required");
 else require(rules.native_deposit_votes&&!initial.registry.validators.contains(64),"native admitted fixture seed required");
 const auto decoded=decode_consensus_attester_slashing_ssz(unhex(proof.at("ssz")),rules.transition.consensus.body.validators_per_committee);require(bool(decoded),"canonical signed proof codec");auto view=seed.timed.view;
 if(offender==0)require(process_phase0_fork_slashing(view,initial,decoded->first,decoded->second,rules.transition.consensus.body.validators_per_committee)&&view.equivocating_indices.contains(0),"expected proof signatures/eligibility invalid");
 else{
  require(is_slashable_attestation_data(decoded->first.data,decoded->second.data),"expected proof signatures/eligibility invalid");
  const std::vector<Bytes> public_keys{Bytes(public_key.begin(),public_key.end())};
  for(const auto* vote:{&decoded->first,&decoded->second}){
   require(vote->indices==std::vector<uint64_t>{64},"expected proof signatures/eligibility invalid");
   const auto version=vote->data.target.epoch<initial.fork.epoch?initial.fork.previous_version:initial.fork.current_version;
   const auto signing=consensus_signing_root(consensus_attestation_root(vote->data),consensus_domain({1,0,0,0},version,initial.genesis_root));
   require(verify_consensus_bls_fast_aggregate(public_keys,signing,vote->signature),"expected proof signatures/eligibility invalid");
  }
 }
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
 const auto final=context(*at->second.lease());const auto& member=final.registry.validators.at(offender);require(member.public_key==public_key&&final.registry.validators.size()==(offender==0?64:65),"finalized offender identity/registry mismatch");require(member.slashed,"offender is not slashed in observed finalized state");
 std::optional<uint64_t> initial_balance;if(offender==0)initial_balance=initial.registry.validators.at(0).balance;size_t registrations=0;
 size_t included=0,applied=0,blocks=0;ConsensusRoot tip=selected;Json sanction;
 const std::set<std::string> expected_signatures{hex(decoded->first.signature),hex(decoded->second.signature)};
 // Finality may retire old intermediate states. Reconstruct them from the
 // authenticated signed archive rather than requiring their retention.
 std::vector<std::pair<ConsensusRoot,Bytes>> reverse;
 while(tip!=seed.timed.view.anchor){
  require(++blocks<=number(request.at("maximumBlocks"))&&blocks<=1024,"sanction ancestry budget");
  const auto archive=disk.checkpoint_archive(Bytes(tip.begin(),tip.end()),limits.envelope_bytes);require(bool(archive),"canonical signed sanction archive missing");
  const auto message=beacon_block_json(archive->payload,rules,{}).at("message");
  reverse.push_back({tip,archive->payload});tip=root(message.at("parent_root"));
 }
 std::reverse(reverse.begin(),reverse.end());auto replay=seed;
 for(const auto& [id,wire]:reverse){
  const auto message=beacon_block_json(wire,rules,{}).at("message");const auto slot=number(message.at("slot"));
  require(rules.slot_duration_ms&&slot<=(UINT64_MAX-replay.timed.clock.genesis_time_ms)/rules.slot_duration_ms,"sanction replay clock overflow");
  replay=prepare_capella_store_tick(replay,replay.timed.clock.genesis_time_ms+slot*rules.slot_duration_ms,rules,limits.replay_slots,limits.sync_trials);
  const auto parent=root(message.at("parent_root"));require(replay.blocks.contains(parent),"replayed sanction parent missing");const auto prior_registry=context((*replay.blocks.at(parent).lease())).registry;
  auto next=prepare_capella_fork_block(replay,wire,rules,limits.replay_slots,limits.sync_trials);require(next&&next->blocks.contains(id),"signed sanction transition rejected");
  const auto now_registry=context((*next->blocks.at(id).lease())).registry;
  if(now_registry.validators.contains(offender)){
   const auto& now=now_registry.validators.at(offender);require(now.public_key==public_key,"canonical offender identity changed");
   if(!prior_registry.validators.contains(offender)){require(offender==64&&!now.slashed,"invalid admitted registry transition");++registrations;initial_balance=now.balance;}
   else{const auto& prior=prior_registry.validators.at(offender);
    if(now.slashed&&!prior.slashed){++applied;sanction={{"slot",slot},{"root",text(id)},{"balanceBeforeGwei",prior.balance},{"balanceAfterGwei",now.balance}};}
    require(!prior.slashed||now.slashed,"slashed status reverted on canonical ancestry");
   }
  }else require(!prior_registry.validators.contains(offender),"offender removed from canonical registry");
  for(const auto& operation:message.at("body").at("attester_slashings")){
   const auto& a=operation.at("attestation_1"),&b=operation.at("attestation_2");auto has=[&](const Json& v){for(const auto& index:v.at("attesting_indices"))if(index==std::to_string(offender))return true;return false;};
   if(has(a)&&has(b)){require(std::set<std::string>{a.at("signature"),b.at("signature")}==expected_signatures,"canonical offender proof differs from submitted proof");++included;}
  }
  replay=std::move(*next);
 }
 require(replay.blocks.contains(selected)&&replay.blocks.at(selected).lease()->state==at->second.lease()->state,"replayed finalized sanction state differs from retained state");
 require(included==1&&applied==1,"expected exactly one canonical proof and one slashed transition");
 require(initial_balance&&member.balance<*initial_balance,"finalized balance has no net debit");require(offender==0||registrations==1,"expected single canonical member admission");
 require(reactor.signed_slashings().empty(),"finalized sanction proof was not retired");require(reactor.current().token==token,"audit checkpoint changed");
 std::cout<<Json{{"canonicalReplay",canonical},{"requiredFinalized",request.at("requiredFinalized")},{"offender",offender},{"canonicalRegistrationTransitions",registrations},{"finalizedSlashed",true},{"canonicalProofInclusions",included},{"canonicalSlashedTransitions",applied},{"sanction",sanction},{"initialBalanceGwei",*initial_balance},{"finalizedBalanceGwei",member.balance},{"finalizedExitEpoch",member.exit_epoch},{"proofRetired",true},{"externalStopTokenAndWitnessVerified",true},{"readOnlyStores",true},{"productionQualified",false}}.dump(2)<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
