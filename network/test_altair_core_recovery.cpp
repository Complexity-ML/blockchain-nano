#include "altair_core_recovery.hpp"
#include <iostream>
using namespace std::chrono_literals;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
struct Fake {
 std::string session="0x"+std::string(64,'a');unsigned rotations=0,status_calls=0,cancels=0,forgets=0;bool job=true,completed=false;std::string mode;
 Json status(){return Json{{"chainId","0x539"},{"genesisHash","0x"+std::string(64,'9')},{"consensusIdentity","0x"+std::string(64,'d')},{"forkDigest","0x01020304"},{"admissionSession",session},{"workerJobs",size_t(4)},{"wire",{{"frameBytes",size_t(4096)},{"blockBytes",size_t(1024)},{"attestationBytes",size_t(512)},{"slashingBytes",size_t(1024)}}}};}
 Json receipt(){Json j{{"state",completed?"completed":"pending"},{"sequence","0x1"}};if(completed)j["decision"]="accepted";return j;}
 void call(const std::string& method,const Json& p,Reply done){
  check(p.at(0)=="token","missing auth");if(mode=="unavailable"){done(nullptr);return;}if(mode=="silent")return;
  Json reply;
  if(method=="nano_altairStatus"){
   ++status_calls;reply=status();if(mode=="bad-profile")reply["consensusIdentity"]=nullptr;
   if(mode=="changed-profile"&&status_calls>1)reply["consensusIdentity"]="0x"+std::string(64,'e');
   if(mode=="final-session"&&status_calls>1)reply["admissionSession"]="0x"+std::string(64,'e');
  }else if(method=="nano_altairRotateSession"){
   check(p.at(1)==session,"rotation from unobserved session");session="0x"+std::string(64,char('b'+rotations++));reply={{"session",session},{"highWater","0x1"}};
   if(mode=="lost-rotation"&&rotations==1){done(nullptr);return;}
  }else if(method=="nano_altairJobs"){
   reply={{"session",session},{"highWater","0x1"},{"jobs",job?Json::array({receipt()}):Json::array()}};
   if(mode=="wrong-session")reply["session"]="0x"+std::string(64,'f');
   if(mode=="regression")reply["highWater"]="0x0";
   if(mode=="exhausted")reply["highWater"]="0xffffffffffffffff";
   if(mode=="duplicate-sequence")reply["jobs"].push_back(receipt());
   if(mode=="malformed-sequence")reply["jobs"][0]["sequence"]="0x01";
  }else if(method=="nano_altairCancel"){
   check(p.at(1)==session&&p.at(2)=="0x1","wrong cancellation");++cancels;if(mode!="stalled")completed=true;reply=receipt();
  }else if(method=="nano_altairForget"){
   check(completed,"retired unfinished worker");check(p.at(1)==session&&p.at(2)=="0x1","wrong retirement");++forgets;reply=job?receipt():Json{{"state","retired"}};job=false;
   if(mode=="lost-forget"&&forgets==1){done(nullptr);return;}
  }else throw std::runtime_error("unexpected method");
  done(reply);if(mode=="duplicate-callback")done(reply);
 }
};
int main(){try{
 unsigned scenarios=0;
 for(const auto& mode:{"normal","lost-rotation","lost-forget","duplicate-callback","unavailable","silent","bad-profile","changed-profile","final-session","wrong-session","regression","exhausted","duplicate-sequence","malformed-sequence","stalled"}){
  Fake fake;fake.mode=mode;asio::io_context io;unsigned calls=0;Json result;
  auto recovery=std::make_shared<AltairCoreRecovery>(io,[&](const auto& m,auto p,auto cb){fake.call(m,p,cb);},"token","0x539","0x"+std::string(64,'9'),3,1ms,100ms,[&](Json value){++calls;result=std::move(value);});
  recovery->start();io.run();check(calls==1,"recovery completion count");
  const bool success=fake.mode=="normal"||fake.mode=="lost-rotation"||fake.mode=="lost-forget"||fake.mode=="duplicate-callback";
  check(result.is_object()==success,"incorrect readiness result");
  if(success){check(!fake.job&&fake.completed&&fake.cancels==1&&result.at("session")==fake.session&&result.at("highWater")==1,"ready before confirmed drain");if(fake.mode=="lost-rotation")check(fake.rotations==2,"lost rotation was not fenced again");}
  ++scenarios;
 }
 // A cancellation must finish once; a delayed valid HTTP response cannot revive it.
 {asio::io_context io;Reply delayed;unsigned completions=0;auto recovery=std::make_shared<AltairCoreRecovery>(io,[&](const auto&,auto,auto cb){delayed=cb;},"token","0x539","0x"+std::string(64,'9'),3,1ms,100ms,[&](Json result){check(result.is_null(),"cancel became ready");++completions;});recovery->start();recovery->cancel();Fake fake;delayed(fake.status());io.run();check(completions==1,"late reply completed twice");++scenarios;}
 std::cout<<scenarios<<" Altair recovery scenarios passed; no P2P activation claimed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
