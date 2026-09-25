#pragma once
#include "altair_work.hpp"
#include "json_stream.hpp"
#include <atomic>

// Libp2p Stream adapter. Registration/handshake and an actual bounded worker
// queue are caller responsibilities. io_context, ingress/budget and dispatcher
// must outlive pending requests. No consensus work runs here on the I/O thread.
class AltairStreamRequest:public std::enable_shared_from_this<AltairStreamRequest>{
public:
 using Decision=AltairWork::Decision;
 using Cancellation=AltairWork::Cancellation;
 using Done=AltairWork::Done;
 // Return true only after enqueueing; false/throw means no worker was started.
 // Done must mean all work (including durable commit) has ended. It may run on
 // any thread; exactly-once completion is enforced on the I/O executor.
 using Dispatch=AltairWork::Dispatch;
private:
 asio::io_context& io_;Stream stream_;std::shared_ptr<AltairMessageIngress> ingress_;Dispatch dispatch_;
 asio::steady_timer timer_;std::shared_ptr<InboundBudget::Lease> lease_;
 Cancellation cancelled_=std::make_shared<std::atomic<bool>>(false);
 bool dispatched_=false,worker_done_=false,finished_=false,started_=false;
 void abort(){if(finished_)return;finished_=true;cancelled_->store(true);timer_.cancel();if(!stream_->isClosed())stream_->reset();if(!dispatched_||worker_done_)lease_.reset();}
 void completed(Decision decision){
  if(worker_done_)return;worker_done_=true;
  if(finished_){lease_.reset();return;}
  if(decision==Decision::local_failure){abort();return;}
  write_json(io_,stream_,Json{{"accepted",decision==Decision::accepted}},[self=shared_from_this()](bool ok){
   if(!self->finished_){self->finished_=true;self->timer_.cancel();if(ok)close_stream(self->stream_);else if(!self->stream_->isClosed())self->stream_->reset();}
   self->lease_.reset();
  });
 }
public:
 AltairStreamRequest(asio::io_context& io,Stream stream,std::shared_ptr<AltairMessageIngress> ingress,Dispatch dispatch):
 io_(io),stream_(std::move(stream)),ingress_(std::move(ingress)),dispatch_(std::move(dispatch)),timer_(io){if(!stream_||!ingress_||!dispatch_)throw std::invalid_argument("invalid Altair stream handler");}
 // Call on the I/O executor during shutdown; capacity remains reserved while
 // a dispatched worker is still running.
 void cancel(){abort();}
 void start(std::chrono::steady_clock::duration timeout=std::chrono::seconds(10)){
  if(started_||finished_)throw std::logic_error("Altair stream already started");started_=true;
  const auto peer=stream_->remotePeerId();if(!peer){abort();return;}
  lease_=ingress_->acquire(peer.value().toBase58());if(!lease_){abort();return;}
  timer_.expires_after(timeout);timer_.async_wait([self=shared_from_this()](auto ec){if(!ec)self->abort();});
  read_json_frame(io_,stream_,ingress_->frame_limit(),[self=shared_from_this()](auto frame){
   if(self->finished_)return;if(!frame){self->abort();return;}
   std::optional<AltairWireMessage> message;
   try{message=self->ingress_->decode(*frame);}catch(...){self->abort();return;}
   if(!message){self->abort();return;}
   self->dispatched_=true;
   auto done=[self](Decision d){asio::post(self->io_,[self,d]{self->completed(d);});};
   try{if(self->dispatch_(std::move(*message),self->cancelled_,std::move(done)))return;}catch(...){/* no worker admitted */}
   self->dispatched_=false;self->abort();
  },timeout);
 }
};
