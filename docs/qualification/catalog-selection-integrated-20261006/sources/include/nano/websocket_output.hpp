#pragma once
#include "types.hpp"
#include <deque>
#include <memory>
#include <ostream>
#include <streambuf>
namespace nano {
// All access is confined to the server event loop. Limits count serialized
// payload bytes, including the buffer referenced by an active async_write.
struct WebSocketOutputBudget {
 size_t connection_limit=8*1024*1024,total_limit=64*1024*1024,message_limit=256,used=0;
};
struct RpcConnections {
 size_t active=0,beacon_streams=0;
 std::shared_ptr<WebSocketOutputBudget> output=std::make_shared<WebSocketOutputBudget>();
};
class WebSocketOutput {
 class Buffer:public std::streambuf {
  size_t limit_;
  void append(const char* data,size_t size){if(size>limit_-text.size())throw std::length_error("WebSocket output budget exceeded");text.append(data,size);}
  std::streamsize xsputn(const char* data,std::streamsize size)override{if(size<0)throw std::length_error("Invalid output size");append(data,static_cast<size_t>(size));return size;}
  int_type overflow(int_type value)override{if(traits_type::eq_int_type(value,traits_type::eof()))return traits_type::not_eof(value);char c=traits_type::to_char_type(value);append(&c,1);return value;}
 public:
  std::string text;
  explicit Buffer(size_t limit):limit_(limit){}
 };
 std::shared_ptr<WebSocketOutputBudget> budget_;
 std::deque<std::string> messages_;
 size_t bytes_=0;
 void release(size_t size){bytes_-=size;budget_->used-=size;}
public:
 explicit WebSocketOutput(std::shared_ptr<WebSocketOutputBudget> budget):budget_(std::move(budget)){}
 WebSocketOutput(const WebSocketOutput&)=delete;
 WebSocketOutput& operator=(const WebSocketOutput&)=delete;
 ~WebSocketOutput(){budget_->used-=bytes_;}
 bool empty()const{return messages_.empty();}
 size_t size()const{return messages_.size();}
 size_t bytes()const{return bytes_;}
 const std::string& front()const{return messages_.front();}
 bool push(const Json& value){
  if(messages_.size()>=budget_->message_limit||bytes_>budget_->connection_limit||budget_->used>budget_->total_limit)return false;
  auto available=std::min(budget_->connection_limit-bytes_,budget_->total_limit-budget_->used);
  try{
   // Stream into a bounded sink, rather than allocating an unbounded dump()
   // and discovering only afterwards that the response exceeds the budget.
   Buffer buffer(available);std::ostream stream(&buffer);stream.exceptions(std::ios::badbit|std::ios::failbit);stream<<value;
   auto size=buffer.text.size();messages_.push_back(std::move(buffer.text));bytes_+=size;budget_->used+=size;return true;
  }catch(const std::exception&){return false;}
 }
 void pop_front(){release(messages_.front().size());messages_.pop_front();}
 void discard_pending(bool keep_front){while(messages_.size()>(keep_front?1u:0u)){release(messages_.back().size());messages_.pop_back();}}
};
}
