#include "nano/consensus_capella_reactor.hpp"
#include "nano/consensus_message_ssz.hpp"
#include "nano/chain.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
using namespace nano;
static void need(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
static std::string read(const std::string& path,size_t maximum){need(std::filesystem::is_regular_file(path)&&std::filesystem::file_size(path)<=maximum,"diagnostic file bounds");std::ifstream f(path,std::ios::binary);std::string s((std::istreambuf_iterator<char>(f)),{});need(!f.bad()&&s.size()<=maximum,"diagnostic read bounds");return s;}
static uint64_t number(const Json& x){return u64(quantity(x));}
static ForkCheckpointToken token(const Json& x){return {number(x.at("sequence")),unhex(x.at("digest"))};}
static std::string root(const ConsensusRoot& x){return hex(Bytes(x.begin(),x.end()));}
struct Source {std::unique_ptr<ChainStore> disk;std::unique_ptr<CapellaDurableReactor> reactor;CapellaForkBlockRules rules;CapellaStoreSnapshotLimits limits;ForkCheckpointToken expected;std::string name;};
int main(int argc,char** argv){try{
 need(argc==2,"usage: diagnose offline-request.json");const auto request=Json::parse(read(argv[1],1024*1024));
 const auto genesis=Genesis::from_json(Json::parse(read(request.at("genesis"),1024*1024)));
 need(genesis.chain_id==1337&&genesis.consensus=="beacon"&&genesis.evm_revision==EvmRevision::Shanghai,"private synthetic fixture only");
 const auto maximum=number(request.at("maximumSamples")),work=number(request.at("maximumSlotsPerSample"));need(maximum&&maximum<=16&&work&&work<=128,"diagnostic sampling bounds");
 std::vector<Source> sources;
 for(const auto& item:request.at("sources")){
  Source s;s.name=item.at("name");const auto c=Json::parse(read(item.at("config"),1024*1024)).at("capella");const auto& l=c.at("limits");
  s.limits={size_t(number(l.at("envelopeBytes"))),size_t(number(l.at("blocks"))),size_t(number(l.at("messages"))),size_t(number(l.at("equivocations"))),size_t(number(l.at("stateBytes"))),size_t(number(l.at("totalStateBytes"))),number(l.at("replaySlots")),number(l.at("syncTrials"))};
  need(s.limits.envelope_bytes<=128*1024*1024&&s.limits.blocks<=1024,"diagnostic store bounds");
  const auto& p=c.at("profile");s.rules=load_capella_fork_profile(read(p.at("file"),1024*1024),unhex(p.at("sha256")),1024*1024);
  s.expected=token(item.at("checkpoint"));s.disk=std::make_unique<ChainStore>(item.at("database").get<std::string>(),ChainStore::OpenMode::ReadOnly);
  s.disk->bind_chain(genesis.chain_id,genesis.block.header.hash(),"beacon");
  const auto checkpoint=s.disk->fork_checkpoint(genesis.chain_id,genesis.block.header.hash(),s.limits.envelope_bytes,s.expected);need(bool(checkpoint)&&checkpoint->token==s.expected,"diagnostic checkpoint mismatch");
  s.reactor=std::make_unique<CapellaDurableReactor>(*s.disk,genesis.chain_id,genesis.block.header.hash(),s.rules,s.limits,s.expected);sources.push_back(std::move(s));
 }
 need(sources.size()==2&&capella_fork_profile(sources[0].rules)==capella_fork_profile(sources[1].rules),"two compatible stopped sources required");
 Json results=Json::array();
 for(size_t sender=0;sender<2;++sender){auto& source=sources[sender];auto& destination=sources[1-sender];const auto store=destination.reactor->current();const auto recipient_votes=destination.reactor->signed_votes();
  const auto count=destination.rules.transition.consensus.epoch.ffg.slots_per_epoch;const auto& clock=store.state->timed.clock;
  need(count&&destination.rules.slot_duration_ms&&clock.time_ms>=clock.genesis_time_ms,"diagnostic clock bounds");const auto current=(clock.time_ms-clock.genesis_time_ms)/destination.rules.slot_duration_ms,epoch=current/count;size_t n=0;
  auto catalogue=source.reactor->signed_votes();
  std::vector<std::pair<Bytes,Bytes>> samples(catalogue.begin(),catalogue.end());
  std::sort(samples.begin(),samples.end(),[&](const auto& a,const auto& b){
   const auto av=decode_consensus_attestation_ssz(a.second,destination.rules.transition.consensus.body.validators_per_committee);
   const auto bv=decode_consensus_attestation_ssz(b.second,destination.rules.transition.consensus.body.validators_per_committee);
   need(bool(av)&&bool(bv),"authenticated catalogue vote decoding failed");
   return av->data.slot!=bv->data.slot?av->data.slot>bv->data.slot:a.first<b.first;
  });
  for(const auto& [id,bytes]:samples){
   if(n++>=maximum)break;const auto decoded=decode_consensus_attestation_ssz(bytes,destination.rules.transition.consensus.body.validators_per_committee);need(bool(decoded),"authenticated source vote failed decoding");const auto& vote=*decoded;const auto& d=vote.data;
   const bool target_known=store.state->blocks.contains(d.target.root),beacon_known=store.state->timed.view.nodes.contains(d.beacon_block_root);
   Json row{{"sender",source.name},{"recipient",destination.name},{"wireDigest",hex(id)},{"slot",d.slot},{"recipientSlot",current},{"targetEpoch",d.target.epoch},{"recipientEpoch",epoch},{"targetRoot",root(d.target.root)},{"beaconRoot",root(d.beacon_block_root)},{"targetKnown",target_known},{"beaconKnown",beacon_known},{"signatureOutcome","not-isolated"},{"fullOracleAttempted",false},{"catalogueSize",catalogue.size()},{"samplingPolicy","newest-slot-then-wire-digest"}};
   std::string reason;
   const auto duplicate=recipient_votes.find(id);
   if(duplicate!=recipient_votes.end()&&duplicate->second==bytes)reason="already-admitted-identical-wire";
   else if(d.slot>current)reason="future-slot";
   else if(d.target.epoch!=d.slot/count)reason="slot-target-epoch-mismatch";
   else if(d.target.epoch>epoch||d.target.epoch<(epoch?epoch-1:0))reason="outside-gossip-epoch-window";
   else if(!target_known||!beacon_known)reason="missing-retained-reference";
   else if(d.target.epoch>UINT64_MAX/count||store.state->blocks.at(d.target.root).header.slot>d.target.epoch*count)reason="invalid-target-boundary";
   else{
    const auto gap=d.target.epoch*count-store.state->blocks.at(d.target.root).header.slot;row["targetPreparationSlots"]=gap;
    if(gap>work)reason="beyond-diagnostic-work-bound";
    else{
     row["fullOracleAttempted"]=true;
     try{
      auto input=*store.state;
      if(d.slot==current){need(current!=UINT64_MAX&&current+1<=(UINT64_MAX-clock.genesis_time_ms)/destination.rules.slot_duration_ms,"diagnostic clock overflow");input=prepare_capella_store_tick(input,clock.genesis_time_ms+(current+1)*destination.rules.slot_duration_ms,destination.rules,work,destination.limits.sync_trials);}
      const auto accepted=prepare_capella_store_vote(input,vote,destination.rules,work,destination.limits.sync_trials);
      reason=accepted?"full-oracle-admissible":"full-oracle-rejected";
     }catch(const std::length_error&){reason="full-oracle-work-bound";}catch(const std::invalid_argument&){reason="full-oracle-invalid-context";}
    }
   }
   row["classification"]=reason;results.push_back(std::move(row));
  }
 }
 for(const auto& s:sources)need(s.disk->fork_checkpoint(genesis.chain_id,genesis.block.header.hash(),s.limits.envelope_bytes,s.expected).has_value(),"source moved during diagnostic");
 std::cout<<Json{{"scenario","offline-vote-refusal-classification"},{"rows",results},{"storesReadOnly",true},{"sourceQuiescenceExternallyRequired",true},{"activeNetworkCauseProven",false},{"networkQualified",false},{"productionQualified",false}}.dump(2)<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
