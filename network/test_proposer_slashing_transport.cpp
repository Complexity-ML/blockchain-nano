#include "altair_core_dispatch.hpp"
#include "capella_catalog_relay.hpp"
#include "capella_relay_schedule.hpp"
#include "core_rpc_diagnostic.hpp"
#include <iostream>
#include <set>
#include <stdexcept>

using D=AltairWork::Decision;
using namespace std::chrono_literals;
void check(bool yes,const char* why){if(!yes)throw std::runtime_error(why);}
std::string root(char c){return "0x"+std::string(64,c);}
std::string hex_bytes(const std::vector<uint8_t>& b){std::string s="0x";for(auto x:b){s+="0123456789abcdef"[x>>4];s+="0123456789abcdef"[x&15];}return s;}
std::string digest(const std::vector<uint8_t>& b){unsigned char d[32];SHA256(b.data(),b.size(),d);return hex_bytes(std::vector<uint8_t>(d,d+32));}
Json envelope(const AltairWireProfile& p,const std::vector<uint8_t>& b,bool compact=false){
 Json j={{"chainId",p.chain_id},{"genesisHash",p.genesis_hash},{"forkDigest",p.fork_digest},{"kind","proposer_slashing"}};
 if(!p.consensus_identity.empty())j["consensusIdentity"]=p.consensus_identity;
 j[compact?"sszBase64":"ssz"]=compact?altair_base64(b):hex_bytes(b);return j;
}
void row(const char* name){std::cout<<Json{{"scenario",name},{"result","PASS"},{"realConsensusAdmission",false},{"newSignatures",0}}.dump()<<'\n';}
int main(){try{
 static_assert(unsigned(AltairWireKind::sync_committee_message)==5);
 static_assert(unsigned(AltairWireKind::proposer_slashing)==6);
 const AltairWireProfile capella{"0x539",root('a'),"0x01020304",4096,512,256,512,root('b'),true,true,true};
 const std::vector<uint8_t> proof(416,7);unsigned cases=0;
 {
  InboundBudget budget;AltairMessageIngress ingress(capella,budget);
  for(bool compact:{false,true}){auto j=envelope(capella,proof,compact);auto decoded=compact?ingress.decode_compact(j):ingress.decode(j.dump());check(decoded&&decoded->kind==AltairWireKind::proposer_slashing&&decoded->ssz==proof,"exact proposer wire changed");}
  for(size_t size:{size_t(0),size_t(415),size_t(417),size_t(513)})for(bool compact:{false,true}){auto j=envelope(capella,std::vector<uint8_t>(size,7),compact);check(!(compact?ingress.decode_compact(j):ingress.decode(j.dump())),"non416 proposer wire accepted");}
  auto malformed=envelope(capella,proof);malformed["ssz"].get_ref<std::string&>()[4]='G';check(!ingress.decode(malformed.dump()),"bad hex proposer wire accepted");
  auto compact=envelope(capella,proof,true);compact["sszBase64"].get_ref<std::string&>().back()='!';check(!ingress.decode_compact(compact),"bad base64 proposer wire accepted");
  row("proposer-exact-hex-and-compact-lengths");++cases;
 }
 {
  InboundBudget budget;AltairMessageIngress ingress(capella,budget);unsigned dispatched=0;
  auto deliver=[&](const auto&){++dispatched;return true;};
  for(const char* field:{"chainId","genesisHash","forkDigest","consensusIdentity"}){auto j=envelope(capella,proof);j[field]="0x0";check(ingress.receive(std::string("peer-")+field,j.dump(),deliver)==AltairMessageIngress::Result::rejected,"foreign identity entered handler");}
  auto j=envelope(capella,proof);j["extra"]=true;check(ingress.receive("peer",j.dump(),deliver)==AltairMessageIngress::Result::rejected,"extra field entered handler");
  auto raw=envelope(capella,proof).dump();raw.insert(1,"\"kind\":\"proposer_slashing\",");check(ingress.receive("peer",raw,deliver)==AltairMessageIngress::Result::rejected,"duplicate field entered handler");
  auto low=capella;low.slashing_bytes=415;AltairMessageIngress limited(low,budget);check(limited.receive("low-cap",envelope(low,proof).dump(),deliver)==AltairMessageIngress::Result::rejected,"slashing byte cap bypassed");
  auto small=capella;small.frame_bytes=envelope(capella,proof).dump().size()-1;AltairMessageIngress framed(small,budget);check(framed.receive("small-frame",envelope(small,proof).dump(),deliver)==AltairMessageIngress::Result::rejected,"frame byte cap bypassed");
  check(dispatched==0&&budget.active()==0,"invalid wire queued or leaked lease");row("proposer-identity-caps-invalid-never-dispatched");++cases;
 }
 {
  for(bool bellatrix:{false,true}){auto legacy=capella;legacy.proposer_slashings=false;if(!bellatrix)legacy.consensus_identity.clear();InboundBudget budget;AltairMessageIngress ingress(legacy,budget);for(bool compact:{false,true}){auto j=envelope(legacy,proof,compact);check(!(compact?ingress.decode_compact(j):ingress.decode(j.dump())),"legacy profile accepted proposer kind");}}
  auto no_identity=capella;no_identity.consensus_identity.clear();InboundBudget budget;AltairMessageIngress ingress(no_identity,budget);check(!ingress.decode(envelope(no_identity,proof).dump())&&!ingress.decode_compact(envelope(no_identity,proof,true)),"identityless capability admitted proposer kind");
  row("proposer-altair-bellatrix-closed");++cases;
 }
 {
  // A lost Submit reply is reconciled by the same ticket; the new kind never
  // falls through to attester_slashing and does not cause another remote job.
  asio::io_context io;InboundBudget budget;AltairMessageIngress ingress(capella,budget);unsigned submits=0,queries=0,forgets=0,done=0;
  auto call=[&](std::string method,Json args,Reply reply){check(args[0]=="token"&&args[1]=="session"&&args[2]=="0x1","proposer ticket identity changed");
   if(method=="nano_altairSubmit"){++submits;auto decoded=ingress.decode_compact(args[3]);check(decoded&&decoded->kind==AltairWireKind::proposer_slashing&&decoded->ssz==proof,"proposer IPC kind or bytes changed");reply(nullptr);}
   else if(method=="nano_altairJob"){++queries;reply(Json{{"state","completed"},{"sequence","0x1"},{"decision","accepted"}});}
   else if(method=="nano_altairForget"){++forgets;reply(Json{{"state","retired"}});}else throw std::runtime_error("unexpected proposer ticket method");
  };
  auto client=std::make_shared<AltairCoreDispatch>(io,call,capella,"token","session",0,2,3,1ms);auto cancel=std::make_shared<std::atomic<bool>>(false);auto finish=[&](D d){check(d==D::accepted,"proposer receipt changed outcome");++done;};
  for(size_t n:{size_t(415),size_t(417)})check(!client->enqueue({AltairWireKind::proposer_slashing,std::vector<uint8_t>(n,7)},cancel,finish),"invalid proposer IPC queued");
  auto stopped=std::make_shared<std::atomic<bool>>(true);check(!client->enqueue({AltairWireKind::proposer_slashing,proof},stopped,finish),"cancelled proposer IPC queued");
  check(client->pending()==0,"rejected proposer IPC occupied queue");check(client->enqueue({AltairWireKind::proposer_slashing,proof},cancel,finish),"valid proposer IPC refused");io.run();
  check(submits==1&&queries==1&&forgets==1&&done==1&&client->pending()==0&&!client->blocked(),"proposer ticket replayed or leaked");
  for(unsigned mode=0;mode<3;++mode){auto forbidden=capella;if(mode==0)forbidden.proposer_slashings=false;else if(mode==1)forbidden.consensus_identity.clear();else forbidden.slashing_bytes=415;auto other=std::make_shared<AltairCoreDispatch>(io,call,forbidden,"token","session",0,2,3,1ms);check(!other->enqueue({AltairWireKind::proposer_slashing,proof},cancel,finish)&&other->pending()==0,"forbidden proposer IPC queued");}
  row("proposer-ipc-receipt-reconciliation-and-rejection");++cases;
 }
 {
  const auto id=digest(proof);
  for(const std::string mode:{"accepted","cached","wrong-digest","rejected"}){
   asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(capella,budget);std::set<std::string> known;if(mode=="cached")known.insert(id);const auto listed=mode=="wrong-digest"?root('1'):id;unsigned exports=0,sends=0,releases=0,done=0;Json result;
   auto call=[&](std::string method,Json args,Reply reply){check(args[0]=="token","catalog authentication lost");
    if(method=="nano_capellaCatalogGeneration")reply(root('e'));
    else if(method=="nano_capellaCatalogAcquire"){check(args[1]=="proposer_slashing","catalog kind changed");reply(Json{{"handle",root('c')}});}
    else if(method=="nano_capellaCatalogPage")reply(Json{{"kind","proposer_slashing"},{"sequence","0x1"},{"digest",root('d')},{"entries",Json::array({listed})},{"next",nullptr}});
    else if(method=="nano_capellaExport"){++exports;check(args[1]=="proposer_slashing"&&args[2]==listed,"export selector changed");reply(Json{{"root",listed},{"available",true},{"envelope",envelope(capella,proof)}});}
    else if(method=="nano_capellaCatalogRelease"){++releases;reply(true);}else throw std::runtime_error("unexpected proposer catalog method");
   };
   auto deliver=[&](std::string root,Json frame,Reply reply){check(root==id&&frame==envelope(capella,proof),"proposer catalog changed payload");++sends;const bool accepted=mode!="rejected";if(accepted)known.insert(id);reply(Json{{"accepted",accepted}});};
   const unsigned attempts=mode=="accepted"?2:1;
   for(unsigned attempt=0;attempt<attempts;++attempt){auto relay=std::make_shared<CapellaCatalogRelay>(io,call,deliver,"token","proposer_slashing",ingress,1,1,[&](Json j){++done;result=std::move(j);},[&](const std::string& r){return known.contains(r);});relay->start(1s);io.restart();io.run();}
   const bool accepted=mode=="accepted"||mode=="cached";
   check(done==attempts&&releases==attempts&&result["success"]==accepted&&result["closed"]==true,"proposer catalog result or release wrong");
   if(mode=="wrong-digest")check(exports==1&&sends==0&&known.empty()&&result["reason"]=="digest-mismatch","wrong SHA reached proposer delivery");
   else if(mode=="rejected")check(exports==1&&sends==1&&known.empty()&&result["reason"]=="admission-rejected","refused proposer proof counted as ACK");
   else check(exports==(mode=="cached"?0u:1u)&&sends==(mode=="cached"?0u:1u)&&result["skippedKnown"]==1,"known proposer root exported or sent twice");
  }
  row("proposer-catalog-sha-export-ack-and-dedupe");++cases;
 }
 {
  check(std::string(CapellaRelaySchedule::name(6))=="proposer_slashing","seventh category mislabeled");
  for(unsigned phase=0;phase<CapellaRelaySchedule::catchup_opportunities;++phase){CapellaRelaySchedule schedule;for(unsigned i=0;i<phase;++i)schedule.next(true);std::set<unsigned> seen;for(unsigned i=0;i<CapellaRelaySchedule::catchup_opportunities;++i)seen.insert(schedule.next(true));check(seen.size()==CapellaRelaySchedule::category_count&&seen.contains(6),"proposer category starved by catch-up");}
  Json record;CoreRpcTrace trace=[&](const Json& j){record=j;};auto kind=core_rpc_inventory_kind("nano_capellaCatalogAcquire",Json::array({"PRIVATE_TOKEN","proposer_slashing"}));check(kind=="proposer_slashing","new diagnostic category lost");core_rpc_trace(trace,"nano_capellaCatalogAcquire","read-error",0,nullptr,{},kind);check(record["inventoryKind"]=="proposer_slashing"&&record.dump().find("PRIVATE_TOKEN")==std::string::npos,"diagnostic leaked parameter or lost category");
  row("proposer-fair-category-and-redacted-diagnostics");++cases;
 }
 std::cout<<Json{{"scenario","proposer-slashing-transport-summary"},{"result","PASS"},{"groups",cases},{"realConsensusAdmission",false},{"dbOpened",false},{"newSignatures",0},{"wanQualification",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
