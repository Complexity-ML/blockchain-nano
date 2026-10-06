#include "capella_head_catalog_relay.hpp"
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

 auto& source=sources[0];auto& recipient=sources[1];auto baseline=recipient.reactor->current();
 std::set<std::string> frontiers;for(const auto& [id,b]:source.reactor->current().state->blocks)if(!baseline.state->blocks.contains(id)&&baseline.state->blocks.contains(b.header.parent_root))frontiers.insert(root(id));need(!frontiers.empty(),"no real frontiers");
 ChainStore copied(request.at("cloneDatabase").get<std::string>());copied.bind_chain(genesis.chain_id,genesis.block.header.hash(),"beacon");CapellaDurableReactor worker(copied,genesis.chain_id,genesis.block.header.hash(),recipient.rules,recipient.limits,recipient.expected);
 const auto identity=source.reactor->network_identity();const AltairWireProfile profile{identity.at("chainId"),identity.at("genesisHash"),identity.at("forkDigest"),16*1024*1024+1024,8*1024*1024,1024,65536,identity.at("consensusIdentity")};
 asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(profile,budget);auto knowledge=std::make_shared<AltairRelayProgress>(1024);auto exports=std::make_shared<CapellaExportCache>(256,16*1024*1024);
 const auto anchor=root(source.reactor->current().state->timed.view.anchor);const auto head=root(consensus_beacon_header_root(capella_fork_head(*source.reactor->current().state,source.rules).header));
 const auto head_slot=capella_fork_head(*source.reactor->current().state,source.rules).header.slot;
 std::shared_ptr<const CapellaInventorySnapshot> inventory;std::map<std::string,std::string> wire_roots;Json sends=Json::array(),rounds=Json::array();unsigned round=0;
 auto parse_root=[&](const Json& j){const auto bytes=unhex(j.get<std::string>());need(bytes.size()==32,"invalid diagnostic root");ConsensusRoot id{};std::copy(bytes.begin(),bytes.end(),id.begin());return id;};
 auto core=[&](const std::string& method,Json params,Reply reply){need(params[0]=="token","missing modeled authentication");Json value;
  if(method=="nano_capellaStatus")value={{"headRoot",head},{"headSlot",head_slot}};
  else if(method=="nano_capellaCatalogGeneration")value="0x"+std::string(64,'e');
  else if(method=="nano_capellaCatalogAcquire"){inventory=source.reactor->capture_inventory("block",65536);value={{"handle","0x"+std::string(64,'c')}};}
  else if(method=="nano_capellaCatalogPage")value=inventory->page(params[2].is_null()?std::optional<ConsensusRoot>{}:parse_root(params[2]),number(params[3]),65536);
  else if(method=="nano_capellaExport"){
   value=source.reactor->export_message("block",parse_root(params[2]),8*1024*1024);
   if(value.value("available",false))wire_roots[value["envelope"]["ssz"].get<std::string>()]=params[2];
  }else if(method=="nano_capellaCatalogRelease"||method=="nano_capellaCatalogClose")value=true;
  else throw std::runtime_error("unexpected relay core method");reply(std::move(value));
 };
 auto send=[&](Json frame,Reply reply){const auto encoded=frame.at("ssz").get<std::string>();need(wire_roots.contains(encoded),"unknown exported wire");const auto id=wire_roots.at(encoded);const auto before=worker.current().token;bool accepted=worker.block(unhex(encoded));sends.push_back(Json{{"attempt",round},{"root",id},{"frontier",frontiers.contains(id)},{"accepted",accepted},{"checkpointChanged",worker.current().token!=before}});reply(Json{{"accepted",accepted}});};
 const auto maximum_attempts=request.at("maximumAttempts").get<unsigned>();need(maximum_attempts&&maximum_attempts<=32,"relay attempt bound");
 for(round=1;round<=maximum_attempts;++round){
  auto quantum=std::make_shared<CapellaDeliveryQuantum>(CapellaDeliveryQuantum{8});
  auto deliver=[&](std::string id,Json,Reply reply){auto d=std::make_shared<CapellaBlockDelivery>(io,core,send,"token",anchor,id,ingress,knowledge,256,16*1024*1024,std::move(reply),quantum,exports);d->start(std::chrono::seconds(30));};
  Json result;unsigned done=0;auto relay=std::make_shared<CapellaHeadCatalogRelay>(io,core,deliver,send,"token",anchor,ingress,knowledge,256,16*1024*1024,8,1024,[&](Json r){++done;result=std::move(r);},[&](const std::string& id){return knowledge->acknowledged(id);},quantum,true,exports);
  relay->start(std::chrono::seconds(30));io.restart();io.run();need(done==1,"relay completion missing");result["attempt"]=round;result["acknowledgedRoots"]=knowledge->retained();rounds.push_back(result);std::cerr<<result.dump()<<'\n';
  bool all=true;for(const auto& id:frontiers)all=all&&worker.current().state->blocks.contains(parse_root(id));if(all)break;
 }
 Json frontier_outcomes=Json::array();for(const auto& id:frontiers){unsigned attempts=0;for(const auto& row:sends)if(row["root"]==id)++attempts;frontier_outcomes.push_back(Json{{"root",id},{"admissionAttempts",attempts},{"admitted",worker.current().state->blocks.contains(parse_root(id))}});}
 std::cout<<Json{{"scenario","actual-archive-relay-replay"},{"frontiers",frontier_outcomes},{"rounds",rounds},{"sends",sends},{"actualReceiverAdmission",true},{"networkTransportModeled",true},{"httpTicketsAndPacingModeled",false},{"candidateCoreLinked",true},{"noSigningKeysOrJournalsCopied",true},{"productionQualified",false}}.dump(2)<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
