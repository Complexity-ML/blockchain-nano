#pragma once
#include "altair_archive_relay.hpp"

// Walk the trusted local archive to an authenticated anchor or a block previously
// acknowledged by THIS peer/session. Export is not a peer RPC: the local core
// authenticates roots against archived SSZ. Receiver still performs full admission.
class CapellaBlockDelivery:public std::enable_shared_from_this<CapellaBlockDelivery>{
 struct Entry{std::string root,wire;};
 asio::io_context& io_;AltairCoreDispatch::Call core_;std::function<void(Json,Reply)> send_;
 std::string token_,anchor_,target_;std::shared_ptr<AltairMessageIngress> ingress_;
 std::shared_ptr<AltairRelayProgress> knowledge_;const size_t maximum_,budget_;
 std::vector<Entry> pending_;size_t bytes_=0;std::optional<uint64_t> child_slot_;
 Reply done_;asio::steady_timer timer_;bool started_=false,finished_=false;
 static bool root(const std::string& s){return s.size()==66&&s.starts_with("0x")&&std::all_of(s.begin()+2,s.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');});}
 void finish(bool accepted){if(finished_)return;finished_=true;timer_.cancel();pending_.clear();auto done=std::move(done_);done(Json{{"accepted",accepted}});}
 template<class F>Reply callback(F handler){auto once=std::make_shared<std::atomic<bool>>(false);return [self=shared_from_this(),once,handler](Json value){if(once->exchange(true))return;asio::post(self->io_,[self,handler,value=std::move(value)]()mutable{if(self->finished_)return;try{handler(*self,std::move(value));}catch(...){self->finish(false);}});};}
 void fetch(const std::string& id){
  if(id==anchor_||knowledge_->acknowledged(id)){deliver();return;}
  if(pending_.size()>=maximum_){finish(false);return;}
  auto reply=callback([id](auto& self,Json value){self.exported(id,std::move(value));});
  try{core_("nano_capellaExport",Json::array({token_,"block",id}),reply);}catch(...){reply(nullptr);}
 }
 void exported(const std::string& id,Json value){
  if(!value.is_object()||value.value("available",Json())!=true||value.value("root",Json())!=id){finish(false);return;}
  auto wire=value.at("envelope").dump();if(wire.capacity()>budget_-bytes_){finish(false);return;}
  const auto message=ingress_->decode(wire);if(!message||message->kind!=AltairWireKind::block||message->ssz.size()<184){finish(false);return;}
  const auto& b=message->ssz;auto number=[&](size_t at,size_t n){uint64_t result=0;for(size_t i=0;i<n;++i)result|=uint64_t(b.at(at+i))<<(8*i);return result;};
  if(number(0,4)!=100||number(180,4)!=84){finish(false);return;}
  const auto slot=number(100,8);if(child_slot_&&slot>=*child_slot_){finish(false);return;}
  std::string parent="0x";const char* digits="0123456789abcdef";for(size_t i=116;i<148;++i){parent.push_back(digits[b[i]>>4]);parent.push_back(digits[b[i]&15]);}
  if(value.at("parentRoot")!=parent||parent==id){finish(false);return;}
  child_slot_=slot;bytes_+=wire.capacity();pending_.push_back({id,std::move(wire)});fetch(parent);
 }
 void deliver(){
  if(pending_.empty()){finish(true);return;}
  auto frame=Json::parse(pending_.back().wire);const auto id=pending_.back().root;
  auto reply=callback([id](auto& self,Json value){
   if(!value.is_object()||value.size()!=1||value.value("accepted",Json())!=true){self.finish(false);return;}
   self.knowledge_->acknowledge(id);self.bytes_-=self.pending_.back().wire.capacity();self.pending_.pop_back();self.deliver();
  });try{send_(std::move(frame),reply);}catch(...){reply(nullptr);}
 }
public:
 CapellaBlockDelivery(asio::io_context& io,AltairCoreDispatch::Call core,std::function<void(Json,Reply)> send,std::string token,std::string anchor,std::string target,std::shared_ptr<AltairMessageIngress> ingress,std::shared_ptr<AltairRelayProgress> knowledge,size_t maximum,size_t budget,Reply done):io_(io),core_(std::move(core)),send_(std::move(send)),token_(std::move(token)),anchor_(std::move(anchor)),target_(std::move(target)),ingress_(std::move(ingress)),knowledge_(std::move(knowledge)),maximum_(maximum),budget_(budget),done_(std::move(done)),timer_(io){if(!core_||!send_||token_.empty()||!root(anchor_)||!root(target_)||!ingress_||!knowledge_||!maximum_||!budget_||!done_)throw std::invalid_argument("Invalid Capella block delivery");}
 void start(std::chrono::steady_clock::duration timeout=std::chrono::seconds(30)){
  if(started_||finished_)throw std::logic_error("Capella block delivery already started/stopped");if(timeout<=std::chrono::steady_clock::duration::zero())throw std::invalid_argument("Invalid Capella delivery timeout");started_=true;
  timer_.expires_after(timeout);timer_.async_wait([self=shared_from_this()](auto error){if(!error)self->finish(false);});fetch(target_);
 }
 // Same executor as start; late ACKs are never cached after cancellation.
 void cancel(){finish(false);}
};
