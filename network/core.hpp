#pragma once
#include "json.hpp"
#include "core_rpc_diagnostic.hpp"
#include <utility>
#include <algorithm>
#include <chrono>
#include <optional>
#include <vector>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
using Json=nlohmann::json;
namespace asio=boost::asio;namespace beast=boost::beast;namespace http=beast::http;using tcp=asio::ip::tcp;
using Reply=std::function<void(Json)>;
// This hint is emitted only for an authenticated-loopback RPC refusal which
// occurs before a capture reservation. A transport failure is never a hint.
inline Json core_capture_busy_hint(const std::string& method,const Json& response){
 if(method!="nano_capellaCatalogAcquire"||!response.is_object()||response.contains("result")||response.value("jsonrpc",Json())!="2.0"||response.value("id",Json())!=1)return nullptr;
 const auto error=response.value("error",Json());
 if(!error.is_object()||error.value("code",Json())!=-32005||error.value("message",Json())!="Capella inventory capture busy")return nullptr;
 return Json{{"nanoCoreCaptureBusy",true}};
}
inline bool core_capture_busy(const Json& value){return value==Json{{"nanoCoreCaptureBusy",true}};}
class CoreRequest:public std::enable_shared_from_this<CoreRequest>{
 beast::tcp_stream stream_;beast::flat_buffer buffer_;http::request<http::string_body> request_;
 std::optional<http::response_parser<http::string_body>> parser_;Reply reply_;CoreRpcTrace trace_;std::string method_;
 std::function<void(std::shared_ptr<CoreRequest>)> recycle_;
 void finish(Json value,bool reusable=false){
  auto cb=std::exchange(reply_,Reply{});if(!cb)return;
  if(reusable){stream_.expires_never();recycle_(shared_from_this());}
  else {beast::error_code ignored;stream_.socket().close(ignored);}
  cb(std::move(value));
 }
 void write(){http::async_write(stream_,request_,[self=shared_from_this()](beast::error_code ec,size_t){
  if(ec){core_rpc_trace(self->trace_,self->method_,"write-error");self->finish(nullptr);return;}
  http::async_read(self->stream_,self->buffer_,*self->parser_,[self](beast::error_code ec,size_t){
   if(ec){core_rpc_trace(self->trace_,self->method_,"read-error");self->finish(nullptr);return;}
   auto response=self->parser_->release();auto json=Json::parse(response.body(),nullptr,false);
   const bool reusable=response.keep_alive()&&self->buffer_.size()==0;
   if(response.result()!=http::status::ok||!json.is_object()||!json.contains("result")){core_rpc_trace(self->trace_,self->method_,response.result()!=http::status::ok?"http-error":json.is_object()&&json.contains("error")?"rpc-error":"malformed-response",response.result_int(),json,response.body());self->finish(response.result()==http::status::ok?core_capture_busy_hint(self->method_,json):Json(nullptr),reusable);return;}
   self->finish(std::move(json["result"]),reusable);
  });
 });}
public:
 CoreRequest(asio::io_context& io,std::function<void(std::shared_ptr<CoreRequest>)> recycle,CoreRpcTrace trace={}):stream_(io),trace_(std::move(trace)),recycle_(std::move(recycle)){}
 void start(uint16_t port,const std::string& method,Json params,Reply reply){
  if(reply_)throw std::logic_error("Core HTTP connection already busy");reply_=std::move(reply);method_=method;request_={};parser_.emplace();parser_->body_limit(16*1024*1024);
  request_.method(http::verb::post);request_.target("/");request_.version(11);request_.keep_alive(true);request_.set(http::field::host,"127.0.0.1:"+std::to_string(port));request_.set(http::field::content_type,"application/json");request_.body()=Json{{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",std::move(params)}}.dump();request_.prepare_payload();
  stream_.expires_after(std::chrono::seconds(5));
  if(stream_.socket().is_open()){write();return;}
  stream_.async_connect(tcp::endpoint(asio::ip::make_address("127.0.0.1"),port),[self=shared_from_this()](beast::error_code ec){if(ec){core_rpc_trace(self->trace_,self->method_,"connect-error");self->finish(nullptr);return;}self->write();});
 }
};
class Core {
 struct Pool {
  using Clock=std::chrono::steady_clock;
  asio::io_context& io;uint16_t port;size_t maximum_idle;CoreRpcTrace trace;
  struct Idle {Clock::time_point since;std::shared_ptr<CoreRequest> connection;};
  std::vector<Idle> idle;
  Pool(asio::io_context& io,uint16_t port,size_t maximum,CoreRpcTrace observer):io(io),port(port),maximum_idle(maximum),trace(std::move(observer)){}
 };
 std::shared_ptr<Pool> pool_;
public:
 // Same io_context executor as the daemon. Idle sockets expire before the
 // core server's 15-second keep-alive timeout; active requests are never shared.
 Core(asio::io_context& io,uint16_t port,size_t maximum_idle=4,CoreRpcTrace trace={}):pool_(std::make_shared<Pool>(io,port,maximum_idle,std::move(trace))){}
 void call(const std::string& method,Json params,Reply reply){
  const auto now=Pool::Clock::now();auto& idle=pool_->idle;
  std::erase_if(idle,[&](const auto& item){return now-item.since>=std::chrono::seconds(10);});
  std::shared_ptr<CoreRequest> connection;
  if(!idle.empty()){connection=std::move(idle.back().connection);idle.pop_back();}
  else {std::weak_ptr<Pool> pool=pool_;connection=std::make_shared<CoreRequest>(pool_->io,[pool](auto channel){if(auto p=pool.lock();p&&p->idle.size()<p->maximum_idle)p->idle.push_back({Pool::Clock::now(),std::move(channel)});},pool_->trace);}
  // Never retry a failed request: the server may already have committed it.
  connection->start(pool_->port,method,std::move(params),std::move(reply));
 }
};
