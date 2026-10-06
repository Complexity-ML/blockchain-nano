#include DISPATCH_HEADER
#include <iostream>
using D=AltairWork::Decision;
void need(bool b,const char* s){if(!b)throw std::runtime_error(s);}
int main(){try{
 const AltairWireProfile p{"0x539","0x"+std::string(64,'0'),"0x01000001",1024,256,256,256};
 unsigned cases=0;
 for(unsigned mode=0;mode<8;++mode){
  asio::io_context io;unsigned calls=0,submits=0,queries=0,cancels=0,forgets=0,done=0;bool restored=false;D result=D::local_failure;
  auto flag=std::make_shared<std::atomic<bool>>(false);std::shared_ptr<AltairCoreDispatch> client;
  auto call=[&](const std::string& method,Json args,Reply reply){
   ++calls;need(args[0]=="token"&&args[1]=="session"&&args[2]=="0x1","ticket identity changed");
   if(mode==4&&!restored){reply(nullptr);return;}
   if(method=="nano_altairSubmit"){
    ++submits;need(submits==1,"uncertain payload resubmitted");
    if(mode==0||mode==3||mode==7){reply(nullptr);return;}
    if(mode==5){reply(Json{{"state","pending"},{"sequence","0x2"}});return;}
    reply(Json{{"state",mode==2?"completed":"pending"},{"sequence","0x1"},{"decision","accepted"}});return;
   }
   if(method=="nano_altairJob"){
    ++queries;if((mode==0||mode==1||mode==3||mode==7)&&queries<3){reply(nullptr);return;}
    reply(Json{{"state","completed"},{"sequence","0x1"},{"decision","accepted"}});return;
   }
   if(method=="nano_altairCancel"){
    ++cancels;need(flag->load(),"recovery did not cancel outstanding work");
    need(!client->enqueue({AltairWireKind::attestation,{9}},std::make_shared<std::atomic<bool>>(false),[](D){}),"admission resumed before receipt drain");
    reply((mode==0||mode==3||mode==4||mode==7)?Json{{"state","retired"}}:Json{{"state","completed"},{"sequence","0x1"},{"decision","accepted"}});return;
   }
   need(method=="nano_altairForget","unexpected method");++forgets;
   if(mode==2&&forgets<=3){reply(nullptr);return;}
   if(mode==6){reply(Json{{"state","completed"},{"sequence","0x1"},{"decision","rejected"}});return;}
   reply(Json{{"state","retired"}});
  };
  client=std::make_shared<AltairCoreDispatch>(io,call,p,"token","session",0,2,3,std::chrono::milliseconds(1));
  need(client->enqueue({AltairWireKind::attestation,{1,2}},flag,[&](D d){++done;result=d;}),"initial admission refused");
  unsigned queuedDone=0;if(mode==7)need(client->enqueue({AltairWireKind::attestation,{3,4}},std::make_shared<std::atomic<bool>>(false),[&](D d){need(d==D::local_failure,"unissued queue did not cancel");++queuedDone;}),"queued request refused");
  if(mode==4){io.run_for(std::chrono::milliseconds(15));need(client->blocked()&&client->pending()==1&&done==0&&flag->load(),"outage released uncertain worker");need(calls>=3&&calls<15,"unbounded probe loop");restored=true;io.restart();}
  io.run_for(std::chrono::milliseconds(100));
  if(mode==5||mode==6){need(client->blocked()&&client->pending()==1&&done==0,"contradictory receipt released work");need(calls==(mode==5?1:3),"protocol contradiction automatically retried");}
  else{need(!client->blocked()&&client->pending()==0&&done==1,"transport outage permanently froze admission");need(result==((mode==0||mode==3||mode==4||mode==7)?D::local_failure:D::accepted),"recovery changed proven outcome");need(cancels>0||mode==2,"no retirement reconciliation");if(mode==7)need(queuedDone==1&&submits==1,"backlog ticket submitted during recovery");}
  ++cases;
 }
 std::cout<<Json{{"scenario","transport-exhaustion-reconciliation"},{"cases",cases},{"realCoreConnection",false},{"retainedTickets",true},{"protocolContradictionsFailClosed",true}}.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
