#pragma once
#include "altair_core_dispatch.hpp"
#include <charconv>

// Single-owner startup recovery on the I/O executor. No peer admission is
// permitted until the callback returns a non-null, verified empty inventory.
// A failed recovery never implies completion of the old workers.
class AltairCoreRecovery:public std::enable_shared_from_this<AltairCoreRecovery>{
 using Call=AltairCoreDispatch::Call;
 enum class Step{status,rotate,inventory,cancel,forget,verify};
 asio::io_context& io_;asio::steady_timer retry_timer_,deadline_;
 Call call_;Reply done_;const std::string token_,chain_,genesis_;
 const unsigned maximum_failures_;const std::chrono::milliseconds retry_,timeout_;
 Step step_=Step::status;Json metadata_;std::string session_,ticket_;
 uint64_t high_water_=0;unsigned failures_=0;bool started_=false,finished_=false;
 static uint64_t quantity(const Json& value){
  if(!value.is_string())throw std::invalid_argument("quantity type");const auto& s=value.get_ref<const std::string&>();
  if(s.size()<3||s.size()>18||!s.starts_with("0x")||(s.size()>3&&s[2]=='0'))throw std::invalid_argument("quantity encoding");
  uint64_t n=0;const auto result=std::from_chars(s.data()+2,s.data()+s.size(),n,16);
  if(result.ec!=std::errc{}||result.ptr!=s.data()+s.size()||!std::all_of(s.begin()+2,s.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');}))throw std::invalid_argument("quantity range");return n;
 }
 static bool hash(const Json& j){if(!j.is_string())return false;const auto& s=j.get_ref<const std::string&>();return s.size()==66&&s.starts_with("0x")&&std::all_of(s.begin()+2,s.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');});}
 static size_t bound(const Json& j){if(!j.is_number_unsigned()||!j.get<uint64_t>()||j.get<uint64_t>()>SIZE_MAX)throw std::invalid_argument("resource bound");return j.get<size_t>();}
 Json metadata(const Json& value){
  if(value.at("chainId")!=chain_||value.at("genesisHash")!=genesis_||!hash(value.at("consensusIdentity"))||!hash(value.at("admissionSession")))throw std::invalid_argument("core identity mismatch");
  const auto& w=value.at("wire");
  AltairWireProfile profile{chain_,genesis_,value.at("forkDigest").get<std::string>(),bound(w.at("frameBytes")),bound(w.at("blockBytes")),bound(w.at("attestationBytes")),bound(w.at("slashingBytes"))};
  InboundBudget budget;AltairMessageIngress validate(profile,budget);bound(value.at("workerJobs"));
  return Json{{"chainId",chain_},{"genesisHash",genesis_},{"forkDigest",profile.fork_digest},{"consensusIdentity",value.at("consensusIdentity")},{"wire",w},{"workerJobs",value.at("workerJobs")}};
 }
 void finish(Json result){if(finished_)return;finished_=true;retry_timer_.cancel();deadline_.cancel();auto done=std::move(done_);done(std::move(result));}
 void schedule(){retry_timer_.expires_after(retry_);retry_timer_.async_wait([self=shared_from_this()](auto error){if(!error&&!self->finished_)self->request();});}
 void response(Step operation,Json value){
  if(finished_)return;
  if(value.is_null()){
   if(++failures_>=maximum_failures_){finish(nullptr);return;}
   // A lost rotation response requires another confirmed fence. Never assume
   // that a newly observed session was created by our previous request.
   if(operation==Step::rotate)step_=Step::status;
   schedule();return;
  }
  try{
   if(operation==Step::status||operation==Step::verify){
    const auto current=metadata(value);if(!metadata_.is_null()&&metadata_!=current)throw std::invalid_argument("core profile changed during recovery");metadata_=current;
    const auto session=value.at("admissionSession").get<std::string>();
    if(operation==Step::verify){if(session!=session_)throw std::invalid_argument("core session changed after drain");auto ready=metadata_;ready["session"]=session_;ready["highWater"]=high_water_;finish(std::move(ready));return;}
    session_=session;step_=Step::rotate;
   }else if(operation==Step::rotate){
    if(!hash(value.at("session"))||value.at("session")==session_)throw std::invalid_argument("rotation not confirmed");
    session_=value.at("session").get<std::string>();const auto high=quantity(value.at("highWater"));if(high<high_water_||high==UINT64_MAX)throw std::invalid_argument("watermark regressed or exhausted");high_water_=high;step_=Step::inventory;
   }else if(operation==Step::inventory){
    if(value.at("session")!=session_||!value.at("jobs").is_array()||value.at("jobs").size()>bound(metadata_.at("workerJobs")))throw std::invalid_argument("invalid inventory");
    const auto high=quantity(value.at("highWater"));if(high<high_water_||high==UINT64_MAX)throw std::invalid_argument("watermark regressed or exhausted");high_water_=high;
    uint64_t previous=0;for(const auto& job:value.at("jobs")){auto seq=quantity(job.at("sequence"));if(!seq||seq<=previous||seq>high)throw std::invalid_argument("invalid ticket sequence");previous=seq;const auto state=job.at("state");if(state!="pending"&&state!="completed")throw std::invalid_argument("invalid ticket state");if(state=="completed"){const auto d=job.at("decision");if(d!="accepted"&&d!="rejected"&&d!="local_failure")throw std::invalid_argument("invalid decision");}}
    if(value.at("jobs").empty())step_=Step::verify;
    else{const auto& first=value.at("jobs").front();ticket_=first.at("sequence").get<std::string>();step_=first.at("state")=="completed"?Step::forget:Step::cancel;}
   }else{
    const auto state=value.at("state");if(state!="retired"){
     if(value.at("sequence")!=ticket_||(state!="pending"&&state!="completed")||(operation==Step::forget&&state!="completed"))throw std::invalid_argument("invalid retirement/cancellation");
    }
    step_=Step::inventory;
   }
   schedule();
  }catch(const std::exception&){finish(nullptr);}
 }
 void request(){
  const auto operation=step_;const char* method=operation==Step::status||operation==Step::verify?"nano_altairStatus":operation==Step::rotate?"nano_altairRotateSession":operation==Step::inventory?"nano_altairJobs":operation==Step::cancel?"nano_altairCancel":"nano_altairForget";
  auto params=Json::array({token_});if(operation==Step::rotate||operation==Step::cancel||operation==Step::forget)params.push_back(session_);if(operation==Step::cancel||operation==Step::forget)params.push_back(ticket_);
  auto once=std::make_shared<std::atomic<bool>>(false);auto callback=[self=shared_from_this(),operation,once](Json result){if(once->exchange(true))return;asio::post(self->io_,[self,operation,result=std::move(result)]()mutable{self->response(operation,std::move(result));});};
  try{call_(method,std::move(params),callback);}catch(...){callback(nullptr);}
 }
public:
 AltairCoreRecovery(asio::io_context& io,Call call,std::string token,std::string chain,std::string genesis,unsigned failures,std::chrono::milliseconds retry,std::chrono::milliseconds timeout,Reply done):
 io_(io),retry_timer_(io),deadline_(io),call_(std::move(call)),done_(std::move(done)),token_(std::move(token)),chain_(std::move(chain)),genesis_(std::move(genesis)),maximum_failures_(failures),retry_(retry),timeout_(timeout){
  if(!call_||!done_||token_.empty()||!maximum_failures_||retry_.count()<=0||timeout_<retry_||timeout_>std::chrono::duration_cast<std::chrono::milliseconds>(asio::steady_timer::duration::max()/2))throw std::invalid_argument("invalid Altair recovery limits");
 }
 void start(){if(started_||finished_)throw std::logic_error("Altair recovery already started");started_=true;deadline_.expires_after(timeout_);deadline_.async_wait([self=shared_from_this()](auto error){if(!error)self->finish(nullptr);});request();}
 void cancel(){finish(nullptr);}
};
