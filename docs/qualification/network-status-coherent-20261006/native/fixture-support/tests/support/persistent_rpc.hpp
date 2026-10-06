#pragma once
#include "nano/core.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
// Local load/test client. One connection per worker, asynchronous deadlines,
// no retry that could mask lost writes. Reconnect only after advertised close.
class PersistentRpcError:public std::runtime_error {
public:
 int code;
 explicit PersistentRpcError(const nano::Json& error):std::runtime_error(error.at("message").get<std::string>()),code(error.at("code").get<int>()){}
};
class PersistentRpc {
 boost::asio::io_context context_;
 boost::beast::tcp_stream stream_{context_};
 boost::beast::flat_buffer buffer_;
 uint16_t port_;
 uint64_t id_=0;
public:
 explicit PersistentRpc(uint16_t port):port_(port){}
 nano::Json call(const std::string& method,std::initializer_list<nano::Json> params,const std::string& origin={}){auto array=nano::Json::array();for(const auto& value:params)array.push_back(value);return call(method,std::move(array),origin);}
 nano::Json call(const std::string& method,nano::Json params=nano::Json::array(),const std::string& origin={}){
  namespace http=boost::beast::http;
  if(!stream_.socket().is_open())stream_.connect({boost::asio::ip::make_address("127.0.0.1"),port_});
  http::request<http::string_body> request(http::verb::post,"/",11);request.set(http::field::host,"127.0.0.1");request.keep_alive(true);if(!origin.empty())request.set(http::field::origin,origin);
  request.body()=nano::Json{{"jsonrpc","2.0"},{"id",++id_},{"method",method},{"params",params}}.dump();request.prepare_payload();
  http::response_parser<http::string_body> parser;parser.body_limit(16*1024*1024);boost::beast::error_code failure;
  context_.restart();stream_.expires_after(std::chrono::seconds(10));
  http::async_write(stream_,request,[&](boost::beast::error_code ec,size_t){if(ec){failure=ec;return;}http::async_read(stream_,buffer_,parser,[&](boost::beast::error_code read_ec,size_t){failure=read_ec;});});
  context_.run();if(failure)throw std::runtime_error("persistent RPC transport: "+failure.message());
  auto response=parser.release();if(!response.keep_alive()){boost::beast::error_code ignored;stream_.socket().close(ignored);}
  if(response.result()!=http::status::ok)throw std::runtime_error("persistent RPC HTTP failure ("+std::to_string(response.result_int())+"): "+response.body());
  auto decoded=nano::Json::parse(response.body());if(decoded.value("jsonrpc",nano::Json())!="2.0"||decoded.value("id",nano::Json())!=id_)throw std::runtime_error("persistent RPC envelope mismatch");
  if(decoded.contains("error"))throw PersistentRpcError(decoded.at("error"));
  return decoded.at("result");
 }
};
