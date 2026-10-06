from pathlib import Path
import json,shutil,hashlib
repo=Path('/Users/boris/Dev/blockchain-nano');root=Path('/private/tmp/nano-catalog-termination-integrated-20261006');old=root/'source-v1';old.mkdir();shutil.copytree(root/'network',old/'network');shutil.copy2(root/'source-manifest.json',old/'manifest.json')
for f in root.glob('arm64-cmake-*'):shutil.copy2(f,old/f.name)
p=repo/'network/CMakeLists.txt';s=p.read_text().replace('target_include_directories(nano-capella-catalog-termination-tests PRIVATE ../third_party)','target_include_directories(nano-capella-catalog-termination-tests PRIVATE . ../third_party)');p.write_text(s)
p=repo/'network/capella_catalog_relay.hpp';s=p.read_text();needle='public:\n CapellaCatalogRelay';assert needle in s
s=s.replace(needle,'''public:
 // Executor-confined observation only. In-progress counters are not ACKs or
 // proof of handle release; the owner may snapshot them before cancellation.
 Json progress()const{return Json{{"scanned",scanned_},{"rejected",rejected_},{"delivered",delivered_},{"exports",exports_},{"payloadsAdmitted",payloads_admitted_},{"skippedKnown",skipped_known_},{"stage",stage_},{"catalogExhausted",catalog_exhausted_},{"firstFailureReason",reason_.empty()?Json(nullptr):Json(reason_)},{"firstFailureStage",failure_stage_.empty()?Json(nullptr):Json(failure_stage_)},{"terminationReason",termination_reason_.empty()?Json(nullptr):Json(termination_reason_)},{"terminationStage",termination_stage_.empty()?Json(nullptr):Json(termination_stage_)},{"inProgress",!finished_}};}
 CapellaCatalogRelay''');p.write_text(s)
p=repo/'network/capella_head_catalog_relay.hpp';s=p.read_text();a='void fail(const char* reason){auto result=history_result_.value_or(Json::object());';assert a in s;s=s.replace(a,a+'if(catalog_)result["catalogProgress"]=catalog_->progress();');p.write_text(s)
p=repo/'network/test_capella_catalog_termination.cpp';s=p.read_text().replace('#include <capella_catalog_relay.hpp>','#include <capella_head_catalog_relay.hpp>');needle=' std::cout<<"5 catalog termination scenarios passed; no network qualification\\n";return 0;';assert needle in s
s=s.replace(needle,''' for(bool cancel:{false,true}){
  asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(profile,budget);auto knowledge=std::make_shared<AltairRelayProgress>(8);Json result;Reply late;unsigned done=0,sends=0;
  auto core=[&](std::string method,Json p,Reply reply){
   if(method=="nano_capellaCatalogGeneration")reply(root('e'));
   else if(method=="nano_capellaCatalogAcquire")reply(Json{{"handle",root('c')}});
   else if(method=="nano_capellaCatalogRelease")reply(true);
   else if(method=="nano_capellaCatalogPage")reply(Json{{"kind","block"},{"sequence","0x1"},{"digest",root('d')},{"entries",Json::array({root('1'),root('2')})},{"next",nullptr}});
   else if(method=="nano_capellaExport")reply(Json{{"available",true},{"root",p[2]},{"envelope",{{"chainId",profile.chain_id},{"genesisHash",profile.genesis_hash},{"forkDigest",profile.fork_digest},{"consensusIdentity",profile.consensus_identity},{"kind","block"},{"ssz","0x0102"}}}});
   else throw std::runtime_error("history timeout incorrectly requested source head");
  };
  auto relay=std::make_shared<CapellaHeadCatalogRelay>(io,core,[&](std::string id,Json,Reply reply){++sends;if(id==root('1'))reply(Json{{"accepted",false},{"reason","dependency-depth-limit"}});else late=reply;},[](Json,Reply){throw std::runtime_error("history timeout incorrectly sent head");},"token",root('0'),ingress,knowledge,4,8192,2,8,[&](Json x){++done;result=std::move(x);},[](const std::string&){return false;},nullptr,false,nullptr,true);
  relay->start(20ms);if(cancel){io.poll();check(bool(late),"cancel fixture did not reach second delivery");relay->cancel();io.restart();}io.run();
  check(done==1&&sends==2&&result.at("success")==false&&result.at("closed")==false,"wrapper claimed success or release after interrupted history");
  const auto& progress=result.at("catalogProgress");
  check(progress.at("scanned")==1&&progress.at("rejected")==1&&progress.at("exports")==2&&progress.at("payloadsAdmitted")==0&&progress.at("firstFailureReason")=="dependency-depth-limit"&&progress.at("inProgress")==true&&!progress.at("catalogExhausted").get<bool>(),"wrapper erased counters or reclassified rejected/unknown delivery");
  check(knowledge->retained()==0,"interrupted history invented ACK knowledge");late(Json{{"accepted",true}});io.restart();io.run();check(done==1&&knowledge->retained()==0,"late reply altered finished owner or ACK knowledge");
 }
 std::cout<<"7 catalog/wrapper termination scenarios passed; no network qualification\\n";return 0;''');p.write_text(s)
shutil.rmtree(root/'network');shutil.copytree(repo/'network',root/'network');(root/'source-manifest.json').write_text(json.dumps({str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted((root/'network').rglob('*')) if p.is_file()},indent=2)+'\n')
for arch in ['arm64','amd64']:
 p=root/(arch+'-cmake.sh');s=p.read_text().replace('mkdir "$out"','mkdir -p "$out"');p.write_text(s)
print('Frozen source v2 prepared; v1 and failed CMake attempt retained.')
