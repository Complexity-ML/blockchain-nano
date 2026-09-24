#pragma once
#include "persistent_rpc.hpp"
// Local qualification client with the same ten-second per-request deadline as
// PersistentRpc. No retries: an ambiguous write remains visible to the test.
class PersistentWebSocketRpc {
 boost::asio::io_context context_;
 boost::beast::websocket::stream<boost::beast::tcp_stream> socket_{context_};
 boost::asio::steady_timer timer_{context_};
 boost::beast::flat_buffer buffer_;
 uint64_t id_=0;
public:
 explicit PersistentWebSocketRpc(uint16_t port){
  boost::beast::get_lowest_layer(socket_).connect({boost::asio::ip::make_address("127.0.0.1"),port});
  socket_.handshake("127.0.0.1","/");socket_.read_message_max(16*1024*1024);
 }
 nano::Json call(const std::string& method,nano::Json params=nano::Json::array()){
  auto body=nano::Json{{"jsonrpc","2.0"},{"id",++id_},{"method",method},{"params",params}}.dump();
  context_.restart();boost::beast::error_code failure;bool expired=false;
  timer_.expires_after(std::chrono::seconds(10));timer_.async_wait([&](boost::beast::error_code ec){if(ec)return;expired=true;boost::beast::error_code ignored;boost::beast::get_lowest_layer(socket_).socket().cancel(ignored);});
  socket_.async_write(boost::asio::buffer(body),[&](boost::beast::error_code ec,size_t){if(ec){failure=ec;timer_.cancel();return;}socket_.async_read(buffer_,[&](boost::beast::error_code read_ec,size_t){failure=read_ec;timer_.cancel();});});
  context_.run();if(expired)throw std::runtime_error("WebSocket RPC request timed out");if(failure)throw std::runtime_error("WebSocket RPC transport: "+failure.message());
  auto response=nano::Json::parse(boost::beast::buffers_to_string(buffer_.data()));buffer_.consume(buffer_.size());
  if(response.at("id")!=id_)throw std::runtime_error("WebSocket RPC response ID mismatch");
  if(response.contains("error"))throw PersistentRpcError(response.at("error"));return response.at("result");
 }
};
