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
 using Observe=std::function<void(Json)>;
private:
 asio::io_context& io_;Stream stream_;std::shared_ptr<AltairMessageIngress> ingress_;Dispatch dispatch_;Observe observe_;std::optional<AltairWireKind> kind_;
 asio::steady_timer timer_;std::shared_ptr<InboundBudget::Lease> lease_;
 Cancellation cancelled_=std::make_shared<std::atomic<bool>>(false);
 bool dispatched_=false,worker_done_=false,finished_=false,started_=false;
 // Diagnostic callbacks cannot change admission, cleanup or acknowledgement.
 void report(const char* reason)noexcept{if(!observe_)return;try{observe_(Json{{"reason",reason},{"kind",kind_?Json(static_cast<unsigned>(*kind_)):Json(nullptr)},{"dispatched",dispatched_},{"workerDone",worker_done_}});}catch(...){} }
 void abort(const char* reason){if(finished_)return;finished_=true;report(reason);cancelled_->store(true);timer_.cancel();if(!stream_->isClosed())stream_->reset();if(!dispatched_||worker_done_)lease_.reset();}
 void completed(Decision decision){
  if(worker_done_)return;worker_done_=true;
  if(finished_){lease_.reset();return;}
  if(decision==Decision::local_failure){abort("core-local-failure");return;}
  write_json(io_,stream_,Json{{"accepted",decision==Decision::accepted}},[self=shared_from_this(),decision](bool ok){
   if(!self->finished_){self->finished_=true;self->report(ok?(decision==Decision::accepted?"accepted-ack-written":"rejected-ack-written"):"ack-write-failed");self->timer_.cancel();if(ok)close_stream(self->stream_);else if(!self->stream_->isClosed())self->stream_->reset();}
   self->lease_.reset();
  });
 }
public:
 AltairStreamRequest(asio::io_context& io,Stream stream,std::shared_ptr<AltairMessageIngress> ingress,Dispatch dispatch,Observe observe={}):
 io_(io),stream_(std::move(stream)),ingress_(std::move(ingress)),dispatch_(std::move(dispatch)),observe_(std::move(observe)),timer_(io){if(!stream_||!ingress_||!dispatch_)throw std::invalid_argument("invalid Altair stream handler");}
 // Call on the I/O executor during shutdown; capacity remains reserved while
 // a dispatched worker is still running.
 void cancel(){abort("cancelled");}
 void start(std::chrono::steady_clock::duration timeout=std::chrono::seconds(10)){
  if(started_||finished_)throw std::logic_error("Altair stream already started");started_=true;
  const auto peer=stream_->remotePeerId();if(!peer){abort("peer-unavailable");return;}
  lease_=ingress_->acquire(peer.value().toBase58());if(!lease_){abort("inbound-budget");return;}
  timer_.expires_after(timeout);timer_.async_wait([self=shared_from_this()](auto ec){if(!ec)self->abort("deadline");});
  read_json_frame(io_,stream_,ingress_->frame_limit(),[self=shared_from_this()](auto frame){
   if(self->finished_)return;if(!frame){self->abort("frame-read-failed");return;}
   std::optional<AltairWireMessage> message;
   try{message=self->ingress_->decode(*frame);}catch(...){self->abort("decode-exception");return;}
   if(!message){self->abort("invalid-envelope");return;}
   self->kind_=message->kind;self->dispatched_=true;
   auto done=[self](Decision d){asio::post(self->io_,[self,d]{self->completed(d);});};
   try{if(self->dispatch_(std::move(*message),self->cancelled_,std::move(done)))return;}catch(...){/* no worker admitted */}
   self->dispatched_=false;self->abort("dispatch-unavailable");
  },timeout);
 }
};
