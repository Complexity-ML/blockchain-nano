#include "../network/altair_core_dispatch.hpp"
#include "../network/altair_receipts.hpp"
#include <iostream>
using D=AltairWork::Decision;
void require_test(bool x,const char* message){if(!x)throw std::runtime_error(message);}
int main(){try{
 unsigned cases=0;
 for(unsigned mode=0;mode<5;++mode){
  asio::io_context io;AltairReceipts ledger(2);const auto session=ledger.session();
  const AltairWireProfile p{"0x539","0x"+std::string(64,'0'),"0x01000001",1024,256,256,256};InboundBudget budget;AltairMessageIngress ingress(p,budget);
  unsigned submits=0,queries=0,cancels=0,forgets=0,workers=0,completed=0;D outcome=D::local_failure;AltairWork::Done worker_done;AltairWork::Cancellation worker_cancel;
  auto local_cancel=std::make_shared<std::atomic<bool>>(false);std::shared_ptr<AltairCoreDispatch> client;
  auto dispatch=[&](auto,auto flag,auto done){++workers;worker_cancel=flag;worker_done=done;if(mode==2)done(D::accepted);return true;};
  auto call=[&](const std::string& method,Json args,Reply reply){
   require_test(args[0]=="token"&&args[1]==session&&args[2]=="0x1","server ticket identity changed");
   if(method=="nano_altairSubmit"){
    ++submits;require_test(submits==1,"Submit repeated uncertain work");const auto decoded=ingress.decode_compact(args[3]);require_test(bool(decoded),"compact wire invalid");
    if(mode==1){reply(nullptr);return;}
    const auto result=ledger.submit(session,1,*decoded,dispatch);reply(mode==0?Json(nullptr):result);return;
   }
   if(method=="nano_altairJob"){
    ++queries;if((mode==0||mode==1||mode==3||mode==4)&&queries<=3){reply(nullptr);return;}
    reply(ledger.get(session,1));return;
   }
   if(method=="nano_altairCancel"){
    ++cancels;require_test(local_cancel->load(),"cancel missing after outage");
    require_test(!client->enqueue({AltairWireKind::attestation,{8}},std::make_shared<std::atomic<bool>>(false),[](D){}),"new worker admitted during reconciliation");
    const auto result=ledger.get(session,1,true);if(mode!=1)require_test(worker_cancel->load(),"real worker not cancelled");
    if(mode!=1&&mode!=2&&completed==0)require_test(ledger.list()["jobs"].size()==1,"unfinished server receipt evicted");reply(result);return;
   }
   require_test(method=="nano_altairForget","unexpected method");++forgets;const auto result=ledger.get(session,1,false,true);reply(mode==2&&forgets<=3?Json(nullptr):result);
  };
  client=std::make_shared<AltairCoreDispatch>(io,call,p,"token",session,0,2,3,std::chrono::milliseconds(1));
  require_test(client->enqueue({AltairWireKind::attestation,{1,2}},local_cancel,[&](D d){++completed;outcome=d;}),"initial enqueue refused");
  asio::steady_timer finish(io);if(mode!=1&&mode!=2&&mode!=4){finish.expires_after(std::chrono::milliseconds(30));finish.async_wait([&](auto error){if(error)return;require_test(completed==0&&client->pending()==1,"client released active worker before completion");require_test(bool(worker_done),"worker did not exist");worker_done(D::accepted);});}
  io.run_for(std::chrono::milliseconds(80));
  if(mode==4){require_test(client->blocked()&&client->pending()==1&&completed==0&&ledger.list()["jobs"].size()==1,"uncompleted worker treated as retired");require_test(forgets==0&&cancels>0,"active worker retired or never reconciled");}
  else{
   require_test(!client->blocked()&&client->pending()==0&&completed==1,"outage left live client permanently blocked");
   require_test(outcome==(mode==1?D::local_failure:D::accepted),"published decision changed after cancellation");require_test(ledger.list()["jobs"].empty(),"receipt drain not confirmed");
   if(mode==1){require_test(ledger.submit(session,1,{AltairWireKind::attestation,{1,2}},dispatch)["state"]=="retired"&&workers==0,"delayed Submit escaped cancellation fence");}
   else require_test(workers==1,"work executed more than once");
  }
  ++cases;
 }
 std::cout<<Json{{"scenario","transport-recovery-real-receipt-ledger"},{"cases",cases},{"realReceiptLedger",true},{"realCoreConnection",false},{"realExecutionWorker",false},{"delayedSubmitFenced",true},{"activeWorkNotRetired",true}}.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
