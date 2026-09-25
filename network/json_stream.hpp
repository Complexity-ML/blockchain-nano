#pragma once
#include "core.hpp"
#include "json_frame.hpp"
#include <optional>
#include <libp2p/connection/stream.hpp>
#include <libp2p/basic/write.hpp>
using Stream=std::shared_ptr<libp2p::connection::Stream>;
using RawJsonReply=std::function<void(std::optional<std::string>)>;
class JsonRead:public std::enable_shared_from_this<JsonRead>{
 Stream stream_;asio::steady_timer timer_;std::array<uint8_t,65536> buffer_;std::string data_;Reply reply_;RawJsonReply raw_reply_;JsonFrame frame_;
 bool active()const{return bool(reply_)||bool(raw_reply_);}
 void finish(Json value){timer_.cancel();auto reply=std::exchange(reply_,Reply{});if(reply)reply(std::move(value));}
 void finish_raw(std::optional<std::string> value){timer_.cancel();auto reply=std::exchange(raw_reply_,RawJsonReply{});if(reply)reply(std::move(value));}
 void fail(){if(raw_reply_)finish_raw(std::nullopt);else finish(nullptr);}
 void next(){stream_->readSome(buffer_,buffer_.size(),[self=shared_from_this()](auto result){
  if(!self->active())return;
  if(!result||result.value()==0){self->fail();return;}
  if(result.value()>self->buffer_.size()){self->stream_->reset();self->fail();return;}
  Json json;
  try{
   std::string_view chunk(reinterpret_cast<const char*>(self->buffer_.data()),result.value());
   const bool complete=self->frame_.feed(chunk);self->data_.append(chunk);
   if(!complete){asio::post(self->timer_.get_executor(),[self]{if(self->active())self->next();});return;}
   if(!self->raw_reply_){json=Json::parse(self->data_,nullptr,false);if(json.is_discarded()){self->stream_->reset();self->fail();return;}}
  }catch(const std::exception&){self->stream_->reset();self->fail();return;}
  // Do not swallow application callback failures as frame/parser errors.
  if(self->raw_reply_)self->finish_raw(std::move(self->data_));else self->finish(std::move(json));
 });}
public:
 JsonRead(asio::io_context& io,Stream stream,Reply reply):stream_(std::move(stream)),timer_(io),reply_(std::move(reply)){}
 JsonRead(asio::io_context& io,Stream stream,RawJsonReply reply,size_t frame_limit):stream_(std::move(stream)),timer_(io),raw_reply_(std::move(reply)),frame_(frame_limit){}
 void start(std::chrono::steady_clock::duration timeout=std::chrono::seconds(10)){timer_.expires_after(timeout);timer_.async_wait([self=shared_from_this()](auto ec){if(!ec){self->stream_->reset();self->fail();}});next();}
};
inline void read_json_frame(asio::io_context& io,Stream stream,size_t limit,RawJsonReply reply,std::chrono::steady_clock::duration timeout=std::chrono::seconds(10)){
 std::make_shared<JsonRead>(io,std::move(stream),std::move(reply),limit)->start(timeout);
}
inline void read_json(asio::io_context& io,Stream stream,Reply reply){std::make_shared<JsonRead>(io,std::move(stream),std::move(reply))->start();}
class JsonWrite:public std::enable_shared_from_this<JsonWrite>{
 Stream stream_;asio::steady_timer timer_;std::string data_;std::function<void(bool)> done_;size_t maximum_;
 void finish(bool ok){timer_.cancel();auto done=std::exchange(done_,std::function<void(bool)>{});if(done)done(ok);}
public:
 JsonWrite(asio::io_context& io,Stream stream,const Json& value,std::function<void(bool)> done,size_t maximum=JsonFrame::maximum_size):stream_(std::move(stream)),timer_(io),data_(value.dump()),done_(std::move(done)),maximum_(maximum){}
 void start(std::chrono::steady_clock::duration timeout=std::chrono::seconds(10)){if(data_.size()>maximum_){stream_->reset();finish(false);return;}timer_.expires_after(timeout);timer_.async_wait([self=shared_from_this()](auto ec){if(!ec){self->stream_->reset();self->finish(false);}});libp2p::write(stream_,libp2p::BytesIn(reinterpret_cast<const uint8_t*>(data_.data()),data_.size()),[self=shared_from_this()](auto result){self->finish(bool(result));});}
};
inline void write_json(asio::io_context& io,Stream stream,const Json& json,std::function<void(bool)> done){std::make_shared<JsonWrite>(io,std::move(stream),json,std::move(done))->start();}
inline void close_stream(Stream stream){stream->close([stream](auto){});}
