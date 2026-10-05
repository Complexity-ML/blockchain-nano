#pragma once
#include "capella_block_delivery.hpp"

// An attestation can reach a reconnecting peer before its voted block. Deliver
// that block's authenticated ancestry first; neither local possession nor an
// export response counts as remote admission. Vote and block ACK caches differ.
class CapellaAttestationDelivery:public std::enable_shared_from_this<CapellaAttestationDelivery>{
 asio::io_context& io_;AltairCoreDispatch::Call core_;std::function<void(Json,Reply)> send_;
 std::string token_,anchor_;Json frame_;std::shared_ptr<AltairMessageIngress> ingress_;
 std::shared_ptr<AltairRelayProgress> blocks_;size_t maximum_,budget_;Reply done_;
 std::shared_ptr<CapellaDeliveryQuantum> quantum_;std::shared_ptr<CapellaExportCache> exports_;std::shared_ptr<CapellaBlockDelivery> dependency_;
 asio::steady_timer timer_;bool started_=false,finished_=false;
 void finish(Json value){if(finished_)return;finished_=true;timer_.cancel();auto done=std::move(done_);done(std::move(value));}
 Reply callback(std::function<void(Json)> handler){auto once=std::make_shared<std::atomic<bool>>(false);return [self=shared_from_this(),once,handler=std::move(handler)](Json value){if(once->exchange(true))return;asio::post(self->io_,[self,handler,value=std::move(value)]()mutable{if(self->finished_)return;try{handler(std::move(value));}catch(...){self->finish(Json{{"accepted",false}});}});};}
 void vote(){
  auto reply=callback([self=shared_from_this()](Json ack){self->finish(Json{{"accepted",ack.is_object()&&ack.size()==1&&ack.value("accepted",Json())==true}});});
  try{send_(std::move(frame_),reply);}catch(...){reply(nullptr);}
 }
public:
 CapellaAttestationDelivery(asio::io_context& io,AltairCoreDispatch::Call core,std::function<void(Json,Reply)> send,std::string token,std::string anchor,Json frame,std::shared_ptr<AltairMessageIngress> ingress,std::shared_ptr<AltairRelayProgress> blocks,size_t maximum,size_t budget,Reply done,std::shared_ptr<CapellaDeliveryQuantum> quantum={},std::shared_ptr<CapellaExportCache> exports={}):io_(io),core_(std::move(core)),send_(std::move(send)),token_(std::move(token)),anchor_(std::move(anchor)),frame_(std::move(frame)),ingress_(std::move(ingress)),blocks_(std::move(blocks)),maximum_(maximum),budget_(budget),done_(std::move(done)),quantum_(std::move(quantum)),exports_(std::move(exports)),timer_(io){
  if(!core_||!send_||!ingress_||!blocks_||!done_||token_.empty()||!maximum_||!budget_)throw std::invalid_argument("Invalid Capella attestation delivery");
 }
 void start(std::chrono::steady_clock::duration timeout=std::chrono::seconds(30)){
  if(started_||finished_)throw std::logic_error("Capella attestation delivery already started/stopped");if(timeout<=std::chrono::steady_clock::duration::zero())throw std::invalid_argument("Invalid Capella attestation timeout");started_=true;
  timer_.expires_after(timeout);timer_.async_wait([self=shared_from_this()](auto error){if(!error)self->cancel();});
  const auto message=ingress_->decode(frame_.dump());
  if(!message||message->kind!=AltairWireKind::attestation||message->ssz.size()<229||message->ssz[0]!=228||message->ssz[1]||message->ssz[2]||message->ssz[3]){finish(Json{{"accepted",false}});return;}
  std::string head="0x";for(size_t i=20;i<52;++i){head+="0123456789abcdef"[message->ssz[i]>>4];head+="0123456789abcdef"[message->ssz[i]&15];}
  auto ready=callback([self=shared_from_this()](Json result){
   if(result==Json{{"yielded",true}}){self->finish(std::move(result));return;}
   if(result!=Json{{"accepted",true}}){self->finish(Json{{"accepted",false}});return;}self->vote();
  });
  dependency_=std::make_shared<CapellaBlockDelivery>(io_,core_,send_,token_,anchor_,head,ingress_,blocks_,maximum_,budget_,std::move(ready),quantum_,exports_);dependency_->start(timeout);
 }
 void cancel(){if(finished_)return;finish(Json{{"accepted",false}});if(dependency_)dependency_->cancel();}
};
