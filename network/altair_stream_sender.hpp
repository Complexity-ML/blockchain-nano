#pragma once
#include "json_stream.hpp"
#include "timed_reply.hpp"

// One in-flight message per peer/session, no queue and no retry after an
// ambiguous outcome. Opt-in protocol agrees on the same 16-frame receive cap.
class AltairStreamSender:public std::enable_shared_from_this<AltairStreamSender>{
public:
 using Open=std::function<void(std::function<void(Stream)>)>;
 using Current=std::function<bool()>;
 using Phase=std::function<void(unsigned)>;
private:
 asio::io_context& io_;Open open_;Current current_;size_t frame_limit_;
 std::chrono::steady_clock::duration timeout_,idle_timeout_;
 asio::steady_timer idle_;Stream stream_;unsigned sent_=0;bool stopped_=false;
 std::shared_ptr<TimedReply> pending_;
 void discard(){idle_.cancel();if(stream_&&!stream_->isClosed())stream_->reset();stream_.reset();sent_=0;}
 void exchange(Json frame,Phase phase){
  auto pending=pending_;auto stream=stream_;
  if(!pending||!pending->active())return;
  if(!current_()||stopped_||!stream||stream->isClosed()){discard();pending->finish(nullptr);return;}
  if(phase)phase(0);
  std::make_shared<JsonWrite>(io_,stream,frame,[self=shared_from_this(),pending,stream,phase](bool ok){
   if(!pending->active())return;
   if(!ok||self->stopped_||!self->current_()){self->discard();pending->finish(nullptr);return;}
   if(phase)phase(1);
   read_json_frame(self->io_,stream,256,[self,pending,stream](auto raw){
    if(!pending->active())return;
    auto ack=raw?Json::parse(*raw,nullptr,false):Json(nullptr);
    const bool valid=ack.is_object()&&ack.size()==1&&ack.contains("accepted")&&ack["accepted"].is_boolean();
    if(!valid||self->stopped_||!self->current_()){self->discard();pending->finish(nullptr);return;}
    if(++self->sent_==16){close_stream(stream);self->stream_.reset();self->sent_=0;}
    else{
     // Receiver's idle read expires at 10s. Retire locally at 5s rather than
     // knowingly reuse a stale stream; network failure is still ambiguous.
     self->idle_.expires_after(self->idle_timeout_);
     self->idle_.async_wait([weak=std::weak_ptr<AltairStreamSender>(self),stream](auto ec){if(!ec)if(auto owner=weak.lock())if(!owner->pending_&&owner->stream_==stream)owner->discard();});
    }
    pending->finish(std::move(ack));
   });
  },frame_limit_)->start();
 }
public:
 AltairStreamSender(asio::io_context& io,Open open,Current current,size_t frame_limit,std::chrono::steady_clock::duration timeout=std::chrono::seconds(30),std::chrono::steady_clock::duration idle_timeout=std::chrono::seconds(5)):io_(io),open_(std::move(open)),current_(std::move(current)),frame_limit_(frame_limit),timeout_(timeout),idle_timeout_(idle_timeout),idle_(io){if(!open_||!current_||!frame_limit_||timeout_<=std::chrono::steady_clock::duration::zero()||timeout_>std::chrono::seconds(30)||idle_timeout_<=std::chrono::steady_clock::duration::zero()||idle_timeout_>std::chrono::seconds(5))throw std::invalid_argument("Invalid consensus stream sender");}
 ~AltairStreamSender(){discard();}
 void stop(){if(stopped_)return;stopped_=true;discard();if(auto pending=pending_)pending->finish(nullptr);}
 void send(Json frame,Reply reply,Phase phase={}){
  if(stopped_||pending_||!current_()){reply(nullptr);return;}
  idle_.cancel();
  pending_=std::make_shared<TimedReply>(io_,[self=shared_from_this(),reply=std::move(reply)](Json value)mutable{if(value.is_null())self->discard();self->pending_.reset();reply(std::move(value));});
  auto pending=pending_;pending->start(timeout_);
  if(stream_&&!stream_->isClosed()){exchange(std::move(frame),std::move(phase));return;}
  discard();
  open_([self=shared_from_this(),pending,frame=std::move(frame),phase=std::move(phase)](Stream stream)mutable{
   if(!pending->active()||self->stopped_||!self->current_()){if(stream&&!stream->isClosed())stream->reset();pending->finish(nullptr);return;}
   if(!stream){pending->finish(nullptr);return;}
   self->stream_=std::move(stream);self->exchange(std::move(frame),std::move(phase));
  });
 }
};
