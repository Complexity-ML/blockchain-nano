from pathlib import Path
import shutil,json,hashlib
b=Path('/private/tmp');out=b/'nano-v13-relay-replay-20261006';out.mkdir(exist_ok=False);root=b/'nano-restored-three-node-v13-parent-cache-20261006';old=b/'nano-v13-frontier-docker-20261006'
req=json.loads((old/'request.json').read_text());req['cloneDatabase']='/output/recipient-chain';req['maximumAttempts']=32
for name in ['mac-native','mac-docker']:shutil.copyfile(old/(name+'-config.json'),out/(name+'-config.json'))
source=root/'mac-docker/data/chain';expected={k[6:]:h for k,h in json.loads((old/'data-before.json').read_text())['mac-docker'].items() if k.startswith('chain/')}
def hashes(p):return {str(f.relative_to(p)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(p.rglob('*')) if f.is_file()}
assert hashes(source)==expected
import subprocess
subprocess.run(['/bin/cp','-cR',str(source),str(out/'recipient-chain')],check=True);assert hashes(out/'recipient-chain')==expected
(out/'original-data-before.json').write_text((old/'data-before.json').read_text());(out/'request.json').write_text(json.dumps(req,indent=2)+'\n')
s=(old/'diagnose.cpp').read_text();s='#include "capella_head_catalog_relay.hpp"\n'+s;s=s[:s.index(' Json results=Json::array()')]+r'''
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
'''
(out/'diagnose.cpp').write_text(s)
script=(old/'build-run.sh').read_text().replace(' -c /output/diagnose.cpp',' -I/work/network -DBOOST_ERROR_CODE_HEADER_ONLY -pthread -c /output/diagnose.cpp')
(out/'build-run.sh').write_text(script)
print('Prepared actual reactor/catalog/export replay; clone contains chain files only; bounded 32 attempts')
