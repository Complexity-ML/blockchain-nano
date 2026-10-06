#pragma once
#include "core.hpp"
#include <utility>

// An exchange deadline includes opening/negotiating the stream. Completion is
// delivered once, including when a library callback arrives after the deadline.
class TimedReply:public std::enable_shared_from_this<TimedReply>{
 asio::steady_timer timer_;Reply reply_;std::function<void()> cancel_;
public:
 TimedReply(asio::io_context& io,Reply reply):timer_(io),reply_(std::move(reply)){}
 bool active()const{return bool(reply_);}
 bool on_cancel(std::function<void()> cancel){if(!active()){cancel();return false;}cancel_=std::move(cancel);return true;}
 void finish(Json value){if(!active())return;timer_.cancel();cancel_={};auto reply=std::exchange(reply_,Reply{});reply(std::move(value));}
 void start(std::chrono::steady_clock::duration timeout=std::chrono::seconds(30)){
  timer_.expires_after(timeout);timer_.async_wait([self=shared_from_this()](auto error){if(error||!self->active())return;auto cancel=std::move(self->cancel_);self->finish(nullptr);if(cancel)cancel();});
 }
};
