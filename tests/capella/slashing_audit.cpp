// Stopped private 64-validator fixture only. Read-only stores, external stop token
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
 require(argc==7&&std::string(argv[1])=="--audit","usage: --audit synthetic-genesis node-config stopped-db external-audit-request generated-proof");
 const auto genesis=Genesis::from_json(json(argv[2]));require(genesis.chain_id==1337&&genesis.consensus=="beacon"&&genesis.evm_revision==EvmRevision::Shanghai,"private Beacon Shanghai fixture only");
 const auto c=json(argv[3]).at("capella"),request=json(argv[5]),proof=json(argv[6]);
 require(proof.at("format")=="nano-private-slashing-fixture-v1"&&proof.at("chainId")=="0x539"&&proof.at("genesisHash")==hex(genesis.block.header.hash())&&proof.at("offender")==0&&proof.at("syntheticValidators")==64&&proof.at("productionQualified")==false,"proof fixture identity");
 const auto& l=c.at("limits");CapellaStoreSnapshotLimits limits{size_t(number(l.at("envelopeBytes"))),size_t(number(l.at("blocks"))),size_t(number(l.at("messages"))),size_t(number(l.at("equivocations"))),size_t(number(l.at("stateBytes"))),size_t(number(l.at("totalStateBytes"))),number(l.at("replaySlots")),number(l.at("syncTrials"))};
 require(limits.envelope_bytes<=128*1024*1024&&limits.blocks<=1024,"private audit snapshot budget");
 const auto& p=c.at("profile");const auto rules=load_capella_fork_profile(read(p.at("file"),1024*1024),unhex(p.at("sha256")),1024*1024);
 require(rules.transition.chain_id==genesis.chain_id,"audit chain/profile binding");
 const auto& i=c.at("initial");const auto seed=capella_store_from_snapshot(read(i.at("file"),limits.envelope_bytes),unhex(i.at("sha256")),rules,limits);
 auto context=[&](const CapellaForkBlock& block){return capella_fork_block_context(block.state,block.header,{rules.transition.consensus.epoch,rules.transition.execution,rules.transition.maximum_state_bytes},rules.transition.consensus.state);};
 const auto initial=context(capella_fork_head(seed,rules));require(initial.registry.validators.size()==64&&!initial.registry.validators.at(0).slashed,"initial unslashed64-member fixture required");
 uint64_t words[4]{1,0,0,0};blst_scalar scalar;blst_scalar_from_uint64(&scalar,words);blst_p1 point;blst_sk_to_pk_in_g1(&point,&scalar);std::array<uint8_t,48> public_key;blst_p1_compress(public_key.data(),&point);
 require(initial.registry.validators.at(0).public_key==public_key,"known fixture public key0 required");
 const auto decoded=decode_consensus_attester_slashing_ssz(unhex(proof.at("ssz")),rules.transition.consensus.body.validators_per_committee);require(bool(decoded),"canonical signed proof codec");auto view=seed.timed.view;
 require(process_phase0_fork_slashing(view,initial,decoded->first,decoded->second,rules.transition.consensus.body.validators_per_committee)&&view.equivocating_indices.contains(0),"expected proof signatures/eligibility invalid");
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
 const auto final=context(at->second);const auto& member=final.registry.validators.at(0);require(member.slashed,"offender is not slashed in observed finalized state");
 require(member.balance<initial.registry.validators.at(0).balance,"finalized balance has no net debit");
 size_t included=0,applied=0,blocks=0;ConsensusRoot tip=selected;Json sanction;
 const std::set<std::string> expected_signatures{hex(decoded->first.signature),hex(decoded->second.signature)};
 while(tip!=seed.timed.view.anchor){
  require(++blocks<=limits.blocks,"sanction ancestry budget");const auto b=stopped.state->blocks.find(tip);require(b!=stopped.state->blocks.end(),"canonical sanction ancestor state missing");
  const auto parent=stopped.state->blocks.find(b->second.header.parent_root);require(parent!=stopped.state->blocks.end(),"canonical sanction parent missing");
  const auto now=context(b->second).registry.validators.at(0),prior=context(parent->second).registry.validators.at(0);
  if(now.slashed&&!prior.slashed){++applied;sanction={{"slot",b->second.header.slot},{"root",text(tip)},{"balanceBeforeGwei",prior.balance},{"balanceAfterGwei",now.balance}};}
  require(!prior.slashed||now.slashed,"slashed status reverted on canonical ancestry");
  const auto wire=reactor.signed_block(tip);require(bool(wire),"authenticated signed canonical block missing");const auto message=beacon_block_json(*wire,rules,{});
  for(const auto& operation:message.at("body").at("attester_slashings")){
   const auto& a=operation.at("attestation_1"),&b=operation.at("attestation_2");auto has=[](const Json& v){for(const auto& index:v.at("attesting_indices"))if(index=="0")return true;return false;};
   if(has(a)&&has(b)){require(std::set<std::string>{a.at("signature"),b.at("signature")}==expected_signatures,"canonical offender proof differs from submitted proof");++included;}
  }
  tip=b->second.header.parent_root;
 }
 require(included==1&&applied==1,"expected exactly one canonical proof and one slashed transition");
 require(reactor.signed_slashings().empty(),"finalized sanction proof was not retired");require(reactor.current().token==token,"audit checkpoint changed");
 std::cout<<Json{{"canonicalReplay",canonical},{"requiredFinalized",request.at("requiredFinalized")},{"offender",0},{"finalizedSlashed",true},{"canonicalProofInclusions",included},{"canonicalSlashedTransitions",applied},{"sanction",sanction},{"initialBalanceGwei",initial.registry.validators.at(0).balance},{"finalizedBalanceGwei",member.balance},{"finalizedExitEpoch",member.exit_epoch},{"proofRetired",true},{"externalStopTokenAndWitnessVerified",true},{"readOnlyStores",true},{"productionQualified",false}}.dump(2)<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
