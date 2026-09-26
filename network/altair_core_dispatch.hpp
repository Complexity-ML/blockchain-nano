#pragma once
#include "core.hpp"
#include "altair_work.hpp"
#include <deque>
#include <limits>

// I/O-executor-confined client. Core::call is injectable for qualification.
// Tickets are admitted in order, with one RPC round trip at a time. Caller must
// reconcile old receipts before construction and supply the observed highWater.
// Uncertain transport exhaustion retains jobs/Done callbacks and blocks further
// admission: it must never advertise worker completion without evidence.
class AltairCoreDispatch:public std::enable_shared_from_this<AltairCoreDispatch>{
public:
 using Call=std::function<void(const std::string&,Json,Reply)>;
private:
 enum class Operation {submit,query,cancel,forget};
 struct Job {Json frame;AltairWork::Cancellation cancelled;AltairWork::Done done;uint64_t sequence=0;bool admitted=false,uncertain=false;std::optional<AltairWork::Decision> result;};
 asio::io_context& io_;asio::steady_timer timer_;Call call_;
 const AltairWireProfile profile_;const std::string token_,session_;
 const size_t maximum_;const unsigned failure_limit_;const std::chrono::milliseconds retry_;
 std::deque<Job> jobs_;uint64_t high_water_;unsigned failures_=0;
 bool busy_=false,scheduled_=false,blocked_=false,closing_=false;
 static std::string quantity(uint64_t value){const char* hex="0123456789abcdef";std::string out;do{out.push_back(hex[value&15]);value>>=4;}while(value);std::reverse(out.begin(),out.end());return "0x"+out;}
 void block(){blocked_=true;for(auto& job:jobs_)job.cancelled->store(true);}
 void schedule(std::chrono::milliseconds delay){
  if(busy_||scheduled_||blocked_||jobs_.empty())return;scheduled_=true;timer_.expires_after(delay);
  timer_.async_wait([self=shared_from_this()](auto error){self->scheduled_=false;if(!error&&!self->blocked_)self->step();});
 }
 void complete(AltairWork::Decision decision){auto done=std::move(jobs_.front().done);jobs_.pop_front();failures_=0;done(decision);schedule(std::chrono::milliseconds(0));}
 void response(Operation operation,Json reply){
  busy_=false;if(blocked_||jobs_.empty())return;auto& job=jobs_.front();
  if(!reply.is_object()||!reply.contains("state")||!reply.at("state").is_string()){
   if(operation==Operation::submit)job.uncertain=true;
   if(++failures_>=failure_limit_){block();return;}schedule(retry_);return;
  }
  failures_=0;
  try{
   const auto state=reply.at("state").get<std::string>();
   if(state=="pending"||state=="completed"){
    if(reply.at("sequence")!=quantity(job.sequence)){block();return;}
    job.admitted=true;job.uncertain=false;
    if(state=="pending"){if(operation==Operation::forget){block();return;}schedule(retry_);return;}
    const auto value=reply.at("decision").get<std::string>();
    if(value!="accepted"&&value!="rejected"&&value!="local_failure"){block();return;}
    const auto decision=value=="accepted"?AltairWork::Decision::accepted:value=="rejected"?AltairWork::Decision::rejected:AltairWork::Decision::local_failure;
    if(job.result&&*job.result!=decision){block();return;}job.result=decision;
    if(operation==Operation::forget){complete(decision);return;}schedule(std::chrono::milliseconds(0));return;
   }
   if(state=="retired"){complete(job.result.value_or(AltairWork::Decision::local_failure));return;}
   if(state=="unknown"&&operation==Operation::query&&!job.admitted){job.uncertain=false;schedule(std::chrono::milliseconds(0));return;}
   if(state=="limited"&&operation==Operation::submit){schedule(retry_);return;}
   block();
  }catch(const std::exception&){block();}
 }
 void step(){
  if(jobs_.empty()||blocked_)return;auto& job=jobs_.front();
  if(!job.sequence){if(job.cancelled->load()){complete(AltairWork::Decision::local_failure);return;}job.sequence=++high_water_;}
  const auto op=job.result?Operation::forget:job.cancelled->load()?Operation::cancel:(job.admitted||job.uncertain)?Operation::query:Operation::submit;
  const std::string method=op==Operation::submit?"nano_altairSubmit":op==Operation::cancel?"nano_altairCancel":op==Operation::forget?"nano_altairForget":"nano_altairJob";
  auto args=Json::array({token_,session_,quantity(job.sequence)});if(op==Operation::submit)args.push_back(job.frame);busy_=true;
  auto once=std::make_shared<std::atomic<bool>>(false);
  auto callback=[self=shared_from_this(),op,once](Json value){if(once->exchange(true))return;asio::post(self->io_,[self,op,value=std::move(value)]()mutable{self->response(op,std::move(value));});};
  try{call_(method,std::move(args),callback);}catch(...){callback(nullptr);}
 }
public:
 AltairCoreDispatch(asio::io_context& io,Call call,AltairWireProfile profile,std::string token,std::string session,uint64_t high_water,size_t maximum,unsigned failures,std::chrono::milliseconds retry):
 io_(io),timer_(io),call_(std::move(call)),profile_(std::move(profile)),token_(std::move(token)),session_(std::move(session)),maximum_(maximum),failure_limit_(failures),retry_(retry),high_water_(high_water){
  InboundBudget budget;AltairMessageIngress validate(profile_,budget);
  if(!call_||token_.empty()||session_.empty()||!maximum_||!failure_limit_||retry_<=std::chrono::milliseconds::zero())throw std::invalid_argument("Invalid Altair core dispatch configuration");
 }
 bool enqueue(AltairWireMessage message,AltairWork::Cancellation cancel,AltairWork::Done done){
  if(blocked_||closing_||!cancel||!done||cancel->load()||jobs_.size()>=maximum_||high_water_==UINT64_MAX)return false;
  const auto unissued=jobs_.size()-(!jobs_.empty()&&jobs_.front().sequence?1:0);if(unissued>=UINT64_MAX-high_water_)return false;
  if(message.kind!=AltairWireKind::block&&message.kind!=AltairWireKind::attestation&&message.kind!=AltairWireKind::attester_slashing)return false;
  const auto bound=message.kind==AltairWireKind::block?profile_.block_bytes:message.kind==AltairWireKind::attestation?profile_.attestation_bytes:profile_.slashing_bytes;
  if(message.ssz.size()>bound||message.ssz.size()>profile_.frame_bytes/2)return false;
  const auto kind=message.kind==AltairWireKind::block?"block":message.kind==AltairWireKind::attestation?"attestation":"attester_slashing";
  auto envelope=Json{{"chainId",profile_.chain_id},{"genesisHash",profile_.genesis_hash},{"forkDigest",profile_.fork_digest},{"kind",kind},{"ssz","0x"}};
  if(!profile_.consensus_identity.empty())envelope["consensusIdentity"]=profile_.consensus_identity;
  const auto overhead=envelope.dump().size();
  if(overhead>profile_.frame_bytes||message.ssz.size()>(profile_.frame_bytes-overhead)/2)return false;
  auto frame=Json{{"chainId",profile_.chain_id},{"genesisHash",profile_.genesis_hash},{"forkDigest",profile_.fork_digest},{"kind",kind},{"sszBase64",altair_base64(message.ssz)}};
  if(!profile_.consensus_identity.empty())frame["consensusIdentity"]=profile_.consensus_identity;
  jobs_.push_back(Job{std::move(frame),std::move(cancel),std::move(done)});
  try{schedule(std::chrono::milliseconds(0));}catch(...){scheduled_=false;blocked_=true;jobs_.pop_back();throw;}return true;
 }
 AltairWork::Dispatch dispatcher(){return [weak=weak_from_this()](auto message,auto cancel,auto done){auto self=weak.lock();return self&&self->enqueue(std::move(message),std::move(cancel),std::move(done));};}
 void stop(){closing_=true;for(auto& job:jobs_)job.cancelled->store(true);schedule(std::chrono::milliseconds(0));}
 bool blocked()const{return blocked_;}
 size_t pending()const{return jobs_.size();}
};
