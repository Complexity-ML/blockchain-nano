#include "altair_core_dispatch.hpp"
#include <iostream>
using D=AltairWork::Decision;
void check(bool ok,const char* what){if(!ok)throw std::runtime_error(what);}
int main(){try{
 const AltairWireProfile profile{"0x539","0x"+std::string(64,'0'),"0x01000001",1024,256,256,256};
 auto message=[](){return AltairWireMessage{AltairWireKind::attestation,{1,2}};};
 unsigned cases=0;
 for(unsigned mode=0;mode<7;++mode){
  asio::io_context io;auto flag=std::make_shared<std::atomic<bool>>(false);unsigned calls=0,submits=0,queries=0,forgets=0,completed=0;
  std::vector<std::string> submitted;D decision=D::local_failure;
  auto call=[&](const std::string& method,Json args,Reply reply){
   ++calls;check(args[0]=="token"&&args[1]=="session"&&args[2]=="0x1","ticket identity changed");
   if(mode==4){reply(nullptr);return;} // inaccessible core must retain capacity
   if(method=="nano_altairSubmit"){
    ++submits;submitted.push_back(args.at(3).get<std::string>());
    if(mode==0||mode==1){if(submits==1){reply(nullptr);return;}}
    if(mode==2){flag->store(true);reply(nullptr);return;}
    if(mode==5){reply(Json{{"state","pending"},{"sequence","0x2"}});return;}
    if(mode==6){reply(Json{{"state","limited"}});flag->store(true);return;}
    reply(Json{{"state","pending"},{"sequence","0x1"}});return;
   }
   if(method=="nano_altairCancel"){check(mode==2||mode==6,"unexpected cancellation");reply(Json{{"state","retired"}});return;}
   if(method=="nano_altairJob"){
    ++queries;if(mode==1&&queries==1){reply(Json{{"state","unknown"}});return;}
    reply(Json{{"state","completed"},{"sequence","0x1"},{"decision","accepted"}});
    reply(Json{{"state","completed"},{"sequence","0x1"},{"decision","rejected"}});return;
   }
   check(method=="nano_altairForget","unexpected method");++forgets;
   if(mode==3&&forgets==1){reply(nullptr);return;}
   reply(Json{{"state","retired"}});
  };
  auto client=std::make_shared<AltairCoreDispatch>(io,call,profile,"token","session",0,1,3,std::chrono::milliseconds(1));
  check(client->enqueue(message(),flag,[&](D d){++completed;decision=d;}),"initial admission");
  check(!client->enqueue(message(),std::make_shared<std::atomic<bool>>(false),[](D){}),"queue bound bypassed");
  io.run();
  if(mode==4||mode==5){check(client->blocked()&&client->pending()==1&&completed==0&&flag->load(),"unknown worker completion released capacity");if(mode==4)check(calls==3,"transport retry bound");}
  else{check(!client->blocked()&&client->pending()==0&&completed==1,"missing or duplicate completion");check(decision==((mode==2||mode==6)?D::local_failure:D::accepted),"wrong outcome");}
  if(mode==0)check(submits==1,"lost Submit reply repeated admitted work");
  if(mode==1)check(submits==2&&submitted[0]==submitted[1],"unknown admission changed payload/sequence");
  if(mode==3)check(forgets==2,"lost Forget reply not reconciled");
  ++cases;
 }
 // Admission ordering and shutdown of a queued request with no remote work.
 {
  asio::io_context io;Reply held;unsigned calls=0,done=0;std::vector<D> results;
  auto client=std::make_shared<AltairCoreDispatch>(io,[&](const auto&,auto,Reply cb){++calls;held=std::move(cb);},profile,"token","session",0,2,3,std::chrono::milliseconds(1));
  auto first=std::make_shared<std::atomic<bool>>(false),second=std::make_shared<std::atomic<bool>>(false);
  auto finish=[&](D d){++done;results.push_back(d);};check(client->enqueue(message(),first,finish)&&client->enqueue(message(),second,finish),"two local jobs");
  io.poll();check(calls==1&&done==0,"concurrent ticket admission");client->stop();held(Json{{"state","retired"}});io.restart();io.run();
  check(done==2&&calls==1&&client->pending()==0&&results==std::vector<D>(2,D::local_failure),"queued shutdown leaked work");++cases;
 }
 std::cout<<Json{{"scenario","altair-core-ticket-dispatch"},{"cases",cases},{"realCoreConnection",false},{"unknownWorkRetainsCapacity",true}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
