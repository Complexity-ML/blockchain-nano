#pragma once
#include "json.hpp"
#include <utility>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
using Json=nlohmann::json;
namespace asio=boost::asio;namespace beast=boost::beast;namespace http=beast::http;using tcp=asio::ip::tcp;
using Reply=std::function<void(Json)>;
class CoreRequest:public std::enable_shared_from_this<CoreRequest>{
 beast::tcp_stream stream_;beast::flat_buffer buffer_;http::request<http::string_body> request_;http::response_parser<http::string_body> parser_;Reply reply_;
 void finish(Json value){auto cb=std::exchange(reply_,Reply{});beast::error_code ignored;stream_.socket().close(ignored);if(cb)cb(std::move(value));}
public:
 CoreRequest(asio::io_context& io,uint16_t port,const std::string& method,Json params,Reply reply):stream_(io),reply_(std::move(reply)){
  request_.method(http::verb::post);request_.target("/");request_.version(11);request_.set(http::field::host,"127.0.0.1:"+std::to_string(port));request_.set(http::field::content_type,"application/json");request_.body()=Json{{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",std::move(params)}}.dump();request_.prepare_payload();parser_.body_limit(16*1024*1024);
 }
 void start(uint16_t port){stream_.expires_after(std::chrono::seconds(5));stream_.async_connect(tcp::endpoint(asio::ip::make_address("127.0.0.1"),port),[self=shared_from_this()](beast::error_code ec){if(ec){self->finish(nullptr);return;}http::async_write(self->stream_,self->request_,[self](beast::error_code ec,size_t){if(ec){self->finish(nullptr);return;}http::async_read(self->stream_,self->buffer_,self->parser_,[self](beast::error_code ec,size_t){if(ec){self->finish(nullptr);return;}auto response=self->parser_.release();auto json=Json::parse(response.body(),nullptr,false);if(response.result()!=http::status::ok||json.is_discarded()||!json.contains("result")){self->finish(nullptr);return;}self->finish(std::move(json["result"]));});});});}
};
class Core {
 asio::io_context& io_;uint16_t port_;
public:
 Core(asio::io_context& io,uint16_t port):io_(io),port_(port){}
 void call(const std::string& method,Json params,Reply reply){std::make_shared<CoreRequest>(io_,port_,method,std::move(params),std::move(reply))->start(port_);}
};
