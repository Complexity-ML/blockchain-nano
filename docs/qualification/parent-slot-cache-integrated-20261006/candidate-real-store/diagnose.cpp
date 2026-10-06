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

 auto& source=sources[0];auto& recipient=sources[1];const auto baseline=recipient.reactor->current();
 std::optional<ConsensusRoot> honest;
 for(const auto& [id,b]:source.reactor->current().state->blocks)if(!baseline.state->blocks.contains(id)&&baseline.state->blocks.contains(b.header.parent_root)){honest=id;break;}
 need(bool(honest),"no missing frontier");const auto h=source.reactor->current().state->blocks.at(*honest).header;
 need(h.slot-baseline.state->blocks.at(h.parent_root).header.slot<=256,"frontier gap beyond diagnostic bound");
 const auto valid=source.reactor->signed_block(*honest);need(bool(valid),"frontier wire missing");
 const auto oracle=prepare_capella_received_block(*baseline.state,*valid,recipient.rules,256,recipient.limits.sync_trials);need(bool(oracle),"valid frontier oracle rejected");
 const CapellaForkBlock* alternate=nullptr;ConsensusRoot alternate_root{};
 for(const auto& [id,b]:baseline.state->blocks)if(b.header.parent_root!=h.parent_root&&baseline.state->blocks.contains(b.header.parent_root)&&b.header.slot>baseline.state->blocks.at(b.header.parent_root).header.slot&&b.header.slot-baseline.state->blocks.at(b.header.parent_root).header.slot<=16&&(!alternate||b.header.slot>alternate->header.slot)){alternate=&b;alternate_root=id;}
 need(alternate,"no short alternate parent fixture");auto noise=recipient.reactor->signed_block(alternate_root);need(bool(noise)&&noise->size()>352,"alternate wire missing");(*noise)[352]^=1;
 need(!prepare_capella_received_block(*baseline.state,*noise,recipient.rules,256,recipient.limits.sync_trials),"altered block unexpectedly valid");
 const auto run=[&](const std::string& mode,bool interfere){
  ChainStore disk(request.at(mode+"Database").get<std::string>());disk.bind_chain(genesis.chain_id,genesis.block.header.hash(),"beacon");
  CapellaDurableReactor worker(disk,genesis.chain_id,genesis.block.header.hash(),recipient.rules,recipient.limits,recipient.expected);
  const auto before=worker.current().token;bool admitted=false;size_t rounds=0;
  for(;rounds<100&&!admitted;++rounds){if(interfere)need(!worker.block(*noise),"invalid block admitted");admitted=worker.block(*valid);if(!admitted)need(worker.current().token==before,"incomplete work published state");}
  const bool under_interference=admitted;size_t quiet=0;
  while(!admitted&&quiet++<100)admitted=worker.block(*valid);
  need(admitted,"frontier never progresses even without interference");const auto after=worker.current();
  need(after.state->blocks.at(*honest).state==oracle->blocks.at(*honest).state&&after.state->blocks.at(*honest).execution_state.root()==oracle->blocks.at(*honest).execution_state.root(),"incremental transition differs from full oracle");
  need(worker.signed_block(*honest)==valid,"archive differs from authenticated wire");need(worker.block(*valid)&&worker.current().token==after.token,"duplicate changed checkpoint");
  return Json{{"mode",mode},{"rounds",rounds},{"admittedDuringInterference",under_interference},{"quietRecoveryAttempts",quiet},{"fullOracleEqual",true},{"invalidBlockRejected",true},{"duplicateCheckpointStable",true}};
 };
 const auto control=run("control",false),interference=run("interference",true);
 std::cout<<Json{{"scenario","actual-durable-parent-preparation-interference"},{"honestSlot",h.slot},{"honestParentSlot",baseline.state->blocks.at(h.parent_root).header.slot},{"noiseSlot",alternate->header.slot},{"noiseParentSlot",baseline.state->blocks.at(alternate->header.parent_root).header.slot},{"control",control},{"interference",interference},{"counterexampleObserved",!interference.at("admittedDuringInterference").get<bool>()},{"originalStoresReadOnly",true},{"copiesSameMac",true},{"signingKeysOrJournalsCopied",false},{"activeNetworkCauseProven",false},{"productionQualified",false}}.dump(2)<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
