#pragma once
// Included in http.cpp's anonymous namespace; all connection accounting is
// confined to its single event loop. Polls use the shared bounded worker queue.
std::set<std::string> beacon_topics(std::string_view target){
 constexpr std::string_view prefix="/eth/v1/events?";
 if(!target.starts_with(prefix))throw std::invalid_argument("Missing topics");
 std::set<std::string> topics;auto query=target.substr(prefix.size());
 while(!query.empty()){
  const auto end=query.find('&');auto part=query.substr(0,end);query=end==std::string_view::npos?std::string_view{}:query.substr(end+1);
  if(!part.starts_with("topics="))throw std::invalid_argument("Unknown event parameter");
  std::string value(part.substr(7));
  for(size_t at=0;(at=value.find('%',at))!=std::string::npos;){
   if(value.substr(at,3)!="%2C"&&value.substr(at,3)!="%2c")throw std::invalid_argument("Invalid topic encoding");
   value.replace(at,3,",");++at;
  }
  std::string_view rest=value;
  do {
   auto comma=rest.find(',');std::string topic(rest.substr(0,comma));
   if(topic!="block"&&topic!="finalized_checkpoint")throw std::invalid_argument("Unsupported event topic");
   if(!topics.insert(topic).second)throw std::invalid_argument("Duplicate event topic");
   if(comma==std::string_view::npos)break;rest.remove_prefix(comma+1);
  }while(true);
 }
 if(topics.empty())throw std::invalid_argument("Missing topics");return topics;
}
class BeaconEventSession:public std::enable_shared_from_this<BeaconEventSession>{
 beast::tcp_stream stream_;asio::steady_timer timer_;
 http::response<http::empty_body> header_;
 http::response<http::string_body> error_;
 std::optional<http::response_serializer<http::empty_body>> serializer_;
 SimulationQueue& simulations_;std::shared_ptr<RpcConnections> connections_;
 BeaconEventsHandler handler_;std::set<std::string> topics_;
 std::optional<uint64_t> cursor_;std::string chunk_;bool started_=false;
 void close(){beast::error_code ignored;timer_.cancel();stream_.socket().close(ignored);}
 void unavailable(){
  if(started_){close();return;}
  error_.version(11);error_.result(http::status::service_unavailable);error_.set(http::field::content_type,"application/json");error_.body()=Json{{"code",503},{"message","Beacon event source unavailable"}}.dump();error_.keep_alive(false);error_.prepare_payload();
  stream_.expires_after(std::chrono::seconds(5));http::async_write(stream_,error_,[self=shared_from_this()](beast::error_code,size_t){self->close();});
 }
 void poll(){
  if(!simulations_.reserve(1)){unavailable();return;}
  simulations_.dispatch(stream_.get_executor(),[self=shared_from_this()]{
   try{if(self->simulations_.stopping.load())return Json();return self->handler_(self->cursor_);}catch(const std::exception&){return Json();}
  },[self=shared_from_this()](std::optional<Json> result){
   if(!result||result->is_null()){self->unavailable();return;}
   try{
    self->cursor_=result->at("cursor").get<uint64_t>();
    std::string payload;
    for(const auto& event:result->at("events")){
     const auto topic=event.at("topic").get<std::string>();if(!self->topics_.contains(topic))continue;
     payload+="event: "+topic+"\ndata: "+event.at("data").dump()+"\n\n";
     if(payload.size()>256*1024)throw std::length_error("Beacon event output budget");
    }
    if(payload.empty())payload=": keepalive\n\n";
    std::ostringstream length;length<<std::hex<<payload.size();self->chunk_=length.str()+"\r\n"+payload+"\r\n";
    if(!self->started_){
     self->started_=true;self->header_.version(11);self->header_.result(http::status::ok);
     self->header_.set(http::field::content_type,"text/event-stream");self->header_.set(http::field::cache_control,"no-cache");self->header_.chunked(true);self->header_.keep_alive(true);self->serializer_.emplace(self->header_);
     self->stream_.expires_after(std::chrono::seconds(5));
     http::async_write_header(self->stream_,*self->serializer_,[self](beast::error_code ec,size_t){if(ec)self->close();else self->write();});
    }else self->write();
   }catch(const std::exception&){self->close();}
  },[self=shared_from_this()]{self->simulations_.release(1);});
 }
 void write(){
  stream_.expires_after(std::chrono::seconds(5));
  asio::async_write(stream_,asio::buffer(chunk_),[self=shared_from_this()](beast::error_code ec,size_t){
   if(ec||self->simulations_.stopping.load()){self->close();return;}
   self->chunk_.clear();self->timer_.expires_after(std::chrono::seconds(1));
   self->timer_.async_wait([self](beast::error_code error){if(!error)self->poll();});
  });
 }
public:
 BeaconEventSession(beast::tcp_stream stream,SimulationQueue& queue,std::shared_ptr<RpcConnections> connections,BeaconEventsHandler handler,std::set<std::string> topics):stream_(std::move(stream)),timer_(stream_.get_executor()),simulations_(queue),connections_(std::move(connections)),handler_(std::move(handler)),topics_(std::move(topics)){++connections_->active;++connections_->beacon_streams;}
 ~BeaconEventSession(){--connections_->active;--connections_->beacon_streams;}
 void start(){poll();}
};
