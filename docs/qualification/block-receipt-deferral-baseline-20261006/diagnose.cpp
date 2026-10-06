#include "capella_consensus_service.hpp"
#include <thread>
#include <chrono>
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

 auto& source=sources[0];auto& recipient=sources[1];const auto destination=recipient.reactor->current();std::optional<ConsensusRoot> frontier;
 for(const auto& [id,b]:source.reactor->current().state->blocks)if(!destination.state->blocks.contains(id)&&destination.state->blocks.contains(b.header.parent_root)){frontier=id;break;}need(bool(frontier),"No retained missing frontier");
 auto exported=source.reactor->export_message("block",*frontier,8*1024*1024);need(exported.value("available",false),"Frontier export unavailable");auto frame=exported.at("envelope");auto wire=unhex(frame.at("ssz").get<std::string>());
 ChainStore copied(request.at("cloneDatabase").get<std::string>());copied.bind_chain(genesis.chain_id,genesis.block.header.hash(),"beacon");CapellaDurableReactor worker(copied,genesis.chain_id,genesis.block.header.hash(),recipient.rules,recipient.limits,recipient.expected);InboundBudget budget;
 CapellaConsensusService service(worker,budget,16*1024*1024+1024,8*1024*1024,1024,65536,2,16*1024*1024);
 const auto session=service.receipts().at("session").get<std::string>();const auto before=worker.current().token;service.submit_receipt(session,1,frame.dump());Json receipt;const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(10);
 do{receipt=service.receipt(session,1);if(receipt.at("state")=="completed")break;need(std::chrono::steady_clock::now()<until,"Receipt observation deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}while(true);
 const bool admitted_by_receipt=worker.current().state->blocks.contains(*frontier);const auto after_receipt=worker.current().token;service.stop();service.join();
 unsigned manual_attempts=0;bool accepted=admitted_by_receipt;while(!accepted&&manual_attempts<request.at("maximumAttempts").get<unsigned>()){++manual_attempts;accepted=worker.block(wire);}
 need(accepted,"Frontier did not admit after bounded explicit continuations");
 std::cout<<Json{{"scenario","actual-worker-frontier-deferral-classification"},{"frontier",root(*frontier)},{"sourceSlot",source.reactor->current().state->blocks.at(*frontier).header.slot},{"parentSlot",destination.state->blocks.at(source.reactor->current().state->blocks.at(*frontier).header.parent_root).header.slot},{"receipt",receipt},{"admittedByOneReceipt",admitted_by_receipt},{"checkpointChangedByFirstReceipt",before!=after_receipt},{"additionalManualAdmissionAttempts",manual_attempts},{"admittedAfterManualContinuations",accepted},{"actualCoreAndReceiptWorker",true},{"transportAndClockModeled",true},{"originalStoresReadOnly",true},{"chainOnlyMacClone",true},{"noSigningStorageCopied",true},{"productionQualified",false},{"WANRootCauseProven",false}}.dump(2)<<'\n';
 if(!admitted_by_receipt){std::cerr<<"DEFERRED FRONTIER CLASSIFIED AS REJECTED: valid retained block requires external retries\n";return 1;}return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}}
