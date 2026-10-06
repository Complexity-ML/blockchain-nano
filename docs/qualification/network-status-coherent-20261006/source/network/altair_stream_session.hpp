#pragma once
#include "altair_stream.hpp"

// Opt-in sequential session: never pipeline frames. Each message obtains its
// own existing budget lease and durable admission before its ACK is written.
// Registration/peer compatibility must be checked by the caller on dispatch.
class AltairStreamSession:public std::enable_shared_from_this<AltairStreamSession>{
 asio::io_context& io_;Stream stream_;std::shared_ptr<AltairMessageIngress> ingress_;
 AltairStreamRequest::Dispatch dispatch_;AltairStreamRequest::Observe observe_;
 std::shared_ptr<AltairStreamRequest> request_;const size_t maximum_;
 std::chrono::steady_clock::duration timeout_;size_t completed_=0;bool started_=false,stopped_=false;
 void next(){
  if(stopped_)return;
  request_=std::make_shared<AltairStreamRequest>(io_,stream_,ingress_,dispatch_,observe_,[self=shared_from_this()](bool ack_written){
   if(self->stopped_)return;
   if(!ack_written){self->stopped_=true;return;}
   if(++self->completed_==self->maximum_){self->stopped_=true;close_stream(self->stream_);return;}
   asio::post(self->io_,[self]{self->next();});
  });
  request_->start(timeout_);
 }
public:
 AltairStreamSession(asio::io_context& io,Stream stream,std::shared_ptr<AltairMessageIngress> ingress,
  AltairStreamRequest::Dispatch dispatch,AltairStreamRequest::Observe observe={},size_t maximum=16,
  std::chrono::steady_clock::duration timeout=std::chrono::seconds(10)):
  io_(io),stream_(std::move(stream)),ingress_(std::move(ingress)),dispatch_(std::move(dispatch)),observe_(std::move(observe)),maximum_(maximum),timeout_(timeout){
  if(!stream_||!ingress_||!dispatch_||!maximum_||maximum_>16||timeout_<=std::chrono::steady_clock::duration::zero()||timeout_>std::chrono::seconds(10))throw std::invalid_argument("Invalid bounded consensus stream session");
 }
 void start(){if(started_||stopped_)throw std::logic_error("Consensus stream session already started");started_=true;next();}
 void cancel(){if(stopped_)return;stopped_=true;if(request_)request_->cancel();if(!stream_->isClosed())stream_->reset();}
};
