#include "nano/websocket.hpp"
#include <boost/asio.hpp>
#include <deque>
namespace nano {
namespace asio=boost::asio;namespace beast=boost::beast;namespace http=beast::http;namespace websocket=beast::websocket;
namespace {
class WebSocket:public std::enable_shared_from_this<WebSocket>{
 websocket::stream<beast::tcp_stream> socket_;
 beast::flat_buffer buffer_;
 http::request<http::string_body> handshake_;
 asio::steady_timer timer_;
 Rpc& rpc_;
 std::shared_ptr<size_t> connections_;
 std::map<std::string,RpcStream> subscriptions_;
 std::deque<std::string> output_;
 bool stopped_=false;
 void stop(){if(stopped_)return;stopped_=true;timer_.cancel();beast::error_code ignored;auto& socket=beast::get_lowest_layer(socket_).socket();socket.cancel(ignored);socket.close(ignored);}
 Json request(const Json& value){
  if(value.is_array()){if(value.empty())return rpc_.handle(value);Json result=Json::array();for(const auto& item:value){auto reply=item.is_array()?rpc_.handle(Json(nullptr)):request(item);if(!reply.is_null())result.push_back(std::move(reply));}return result.empty()?Json(nullptr):result;}
  if(!value.is_object()||value.value("jsonrpc",Json())!="2.0"||!value.contains("method")||!value["method"].is_string())return rpc_.handle(value);
  auto method=value["method"].get<std::string>();if(method!="eth_subscribe"&&method!="eth_unsubscribe")return rpc_.handle(value);
  Json id=value.value("id",Json());if(!id.is_null()&&!id.is_string()&&!id.is_number_integer())return rpc_.handle(value);
  Json response={{"jsonrpc","2.0"},{"id",id}};
  try{
   auto params=value.value("params",Json::array());if(!params.is_array())throw RpcError(-32602,"Expected positional parameters");
   if(method=="eth_subscribe"){
    if(params.empty()||params.size()>2)throw RpcError(-32602,"Invalid subscription parameters");
    if(subscriptions_.size()>=64)throw RpcError(-32005,"Too many subscriptions");
    auto type=params[0].get<std::string>();if(type!="logs"&&params.size()!=1)throw RpcError(-32602,"Unexpected subscription parameters");
    auto subscription=rpc_.stream(type,params.size()==2?params[1]:Json::object());auto key=subscription.id;subscriptions_.emplace(key,std::move(subscription));response["result"]=key;
   }else{if(params.size()!=1)throw RpcError(-32602,"Expected subscription ID");response["result"]=subscriptions_.erase(params[0].get<std::string>())!=0;}
  }catch(const RpcError& e){response["error"]={{"code",e.code},{"message",e.what()}};}
  catch(const std::exception&){response["error"]={{"code",-32602},{"message","Invalid params"}};}
  return value.contains("id")?response:Json(nullptr);
 }
 void send(Json value){
  if(stopped_||value.is_null())return;
  if(output_.size()>=256){stop();return;}
  bool idle=output_.empty();output_.push_back(value.dump());if(idle)write();
 }
 void write(){socket_.text(true);socket_.async_write(asio::buffer(output_.front()),[self=shared_from_this()](beast::error_code ec,size_t){if(ec){self->stop();return;}self->output_.pop_front();if(!self->stopped_&&!self->output_.empty())self->write();});}
 void read(){socket_.async_read(buffer_,[self=shared_from_this()](beast::error_code ec,size_t){
  if(ec){self->stop();return;}
  if(!self->socket_.got_text()){self->stop();return;}
  auto text=beast::buffers_to_string(self->buffer_.data());self->buffer_.consume(self->buffer_.size());auto parsed=Json::parse(text,nullptr,false);
  self->send(parsed.is_discarded()?self->rpc_.parse(text):self->request(parsed));if(!self->stopped_)self->read();
 });}
 void tick(){timer_.expires_after(std::chrono::milliseconds(100));timer_.async_wait([self=shared_from_this()](beast::error_code ec){
  if(ec||self->stopped_)return;
  try{for(auto& [id,subscription]:self->subscriptions_){for(auto& event:self->rpc_.poll(subscription)){self->send({{"jsonrpc","2.0"},{"method","eth_subscription"},{"params",{{"subscription",id},{"result",std::move(event)}}}});if(self->stopped_)return;}}}
  catch(const std::exception&){self->stop();return;}
  self->tick();
 });}
public:
 WebSocket(beast::tcp_stream stream,Rpc& rpc,std::shared_ptr<size_t> connections):socket_(std::move(stream)),timer_(socket_.get_executor()),rpc_(rpc),connections_(std::move(connections)){++*connections_;}
 ~WebSocket(){--*connections_;}
 void start(http::request<http::string_body> request){
  beast::get_lowest_layer(socket_).expires_never();socket_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));socket_.read_message_max(16*1024*1024);
  handshake_=std::move(request);socket_.async_accept(handshake_,[self=shared_from_this()](beast::error_code ec){if(ec){self->stop();return;}self->handshake_={};self->read();self->tick();});
 }
};
}
void start_websocket(beast::tcp_stream stream,http::request<http::string_body> request,Rpc& rpc,std::shared_ptr<size_t> connections){std::make_shared<WebSocket>(std::move(stream),rpc,std::move(connections))->start(std::move(request));}
}
