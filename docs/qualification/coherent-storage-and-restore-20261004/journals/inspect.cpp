#include "nano/chain.hpp"
#include "nano/consensus_beacon_signer.hpp"
#include "nano/consensus_capella_reactor.hpp"
#include <fstream>
#include <iostream>
using namespace nano;
static void need(bool ok,const char* m){if(!ok)throw std::runtime_error(m);}
static Json load(const char* p){std::ifstream f(p);need(bool(f),"input missing");return Json::parse(f);}
static std::string raw(const std::string& p){std::ifstream f(p);need(bool(f),"profile missing");return {std::istreambuf_iterator<char>(f),{}};}
static uint64_t num(const Json& j){return u64(quantity(j));}
static ForkCheckpointToken token(const Json& j){return {num(j.at("sequence")),unhex(j.at("digest"))};}
int main(int argc,char** argv){try{
 need(argc==5,"usage genesis node fixture-directory independent-receipt");
 const auto g=Genesis::from_json(load(argv[1]));const auto c=load(argv[2]).at("capella"),l=c.at("limits"),receipt=load(argv[4]);const std::filesystem::path root(argv[3]);
 CapellaStoreSnapshotLimits limits{num(l.at("envelopeBytes")),num(l.at("blocks")),num(l.at("messages")),num(l.at("equivocations")),num(l.at("stateBytes")),num(l.at("totalStateBytes")),num(l.at("replaySlots")),num(l.at("syncTrials"))};
 const auto rules=load_capella_fork_profile(raw(c.at("profile").at("file")),unhex(c.at("profile").at("sha256")),limits.envelope_bytes);
 const auto expected=token(receipt.at("restart"));ChainStore disk(root/"chain",ChainStore::OpenMode::ReadOnly);disk.bind_chain(g.chain_id,g.block.header.hash(),"beacon");
 const auto primary=disk.fork_checkpoint(g.chain_id,g.block.header.hash(),limits.envelope_bytes,expected);need(bool(primary),"primary not found");
 ChainStore witness(root/"core-witness",ChainStore::OpenMode::ReadOnly);witness.bind_chain(g.chain_id,g.block.header.hash(),"nano-capella-recovery-v1");
 const auto w=witness.fork_checkpoint(g.chain_id,g.block.header.hash(),2048+8*limits.envelope_bytes);need(bool(w),"core witness missing");const auto intent=Json::parse(w->snapshot);
 need(intent.is_object()&&intent.size()==5&&intent.at("format")=="nano-capella-recovery-v1"&&intent.dump()==w->snapshot,"core witness format");need(token(intent.at("next"))==expected&&intent.at("snapshot").get<std::string>()==primary->snapshot,"core witness/external commitment mismatch");
 need(token(intent.at("previous")).sequence+1==expected.sequence,"core witness sequence");
 CapellaDurableReactor restored(disk,g.chain_id,g.block.header.hash(),rules,limits,expected);const auto current=restored.current();const auto& head=capella_fork_head(*current.state,rules);const auto& t=rules.transition;
 const auto context=capella_fork_block_context(head.state,head.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);
 const auto configured=c.at("validator").at("keys"),retained=receipt.at("validatorKeys");need(configured.size()==retained.size()&&retained.size()==22,"exact external signer set required");
 Json out=Json::array();size_t rejected=0;
 for(const auto& entry:retained){const auto index=num(entry.at("index"));const auto member=context.registry.validators.find(index);need(member!=context.registry.validators.end(),"index not registered");size_t matches=0;for(const auto& k:configured)matches+=num(k.at("index"))==index;need(matches==1,"configured key missing or duplicate");
  const auto checkpoint=token(entry.at("restart"));const auto key="key-"+std::to_string(index);ChainStore journal(root/key,ChainStore::OpenMode::ReadOnly),recovery(root/(key+"-witness"),ChainStore::OpenMode::ReadOnly);
  const Bytes pk(member->second.public_key.begin(),member->second.public_key.end());BeaconSigningJournal guard(journal,g.chain_id,g.block.header.hash(),context.genesis_root,pk,t.consensus.epoch.ffg.slots_per_epoch,checkpoint,&recovery);
  need(guard.commitment()==checkpoint&&!guard.is_retired(),"journal differs from external commitment");
  auto reject=[&](const ForkCheckpointToken& pin,const ConsensusRoot& genesis,ChainStore& w){bool denied=false;try{BeaconSigningJournal bad(journal,g.chain_id,g.block.header.hash(),genesis,pk,t.consensus.epoch.ffg.slots_per_epoch,pin,&w);}catch(const std::invalid_argument& e){denied=std::string(e.what())=="Invalid beacon signing journal";}need(denied,"bad journal binding not rejected for expected reason");++rejected;};
  auto wrong=checkpoint;need(wrong.digest.size()==32,"token width");wrong.digest[0]^=1;reject(wrong,context.genesis_root,recovery);
  auto wrong_genesis=context.genesis_root;wrong_genesis[0]^=1;reject(checkpoint,wrong_genesis,recovery);
  const auto other=num(retained.at((out.size()+1)%retained.size()).at("index"));need(other!=index,"wrong-witness key is same");ChainStore swapped(root/("key-"+std::to_string(other)+"-witness"),ChainStore::OpenMode::ReadOnly);reject(checkpoint,context.genesis_root,swapped);
  const auto proposal=guard.latest_proposal_slot(),vote=guard.latest_attestation_target();out.push_back({{"index",index},{"externalCommitmentMatched",true},{"primaryWitnessMatched",true},{"latestProposal",proposal?Json(*proposal):Json(nullptr)},{"latestAttestationTarget",vote?Json(*vote):Json(nullptr)}});
 }
 std::cout<<Json{{"coreExternalCommitmentMatched",true},{"coreWitnessMatchesCommittedSnapshot",true},{"signerCount",out.size()},{"negativeBindingsRejected",rejected},{"signers",out},{"readOnly",true},{"signaturesRequested",0},{"dutiesResumed",false},{"historicalForkChoiceVerified",false},{"productionQualified",false}}.dump(2)<<'\n';
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
