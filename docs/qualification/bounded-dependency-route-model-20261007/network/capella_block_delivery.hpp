#pragma once
#include "altair_archive_relay.hpp"
#include "capella_export_cache.hpp"

// Walk the trusted local archive to an authenticated anchor or a block previously
// acknowledged by THIS peer/session. Export is not a peer RPC: the local core
// authenticates roots against archived SSZ. Receiver still performs full admission.
struct CapellaDeliveryQuantum {
 const size_t maximum;size_t admitted=0;
 bool exhausted()const{return admitted>=maximum;}
};

class CapellaBlockDelivery:public std::enable_shared_from_this<CapellaBlockDelivery>{
 struct Entry{std::string root,wire;};
 asio::io_context& io_;AltairCoreDispatch::Call core_;std::function<void(Json,Reply)> send_;
 std::string token_,anchor_,target_,preparation_boundary_;std::shared_ptr<AltairMessageIngress> ingress_;
 std::shared_ptr<AltairRelayProgress> knowledge_;const size_t maximum_,budget_;
 std::vector<std::string> yielded_roots_;
 std::vector<Entry> pending_;size_t bytes_=0;std::optional<uint64_t> child_slot_;
 size_t exports_requested_=0,cache_hits_=0,payloads_sent_=0,payloads_admitted_=0,pending_high_water_=0;
 std::shared_ptr<CapellaDeliveryQuantum> quantum_;std::shared_ptr<CapellaExportCache> exports_;
 Reply done_;asio::steady_timer timer_;bool started_=false,finished_=false;
 bool unavailable_export_=false;
 bool anchor_probe_pending_=false;std::string anchor_probe_root_,anchor_probe_parent_;
 static bool root(const std::string& s){return s.size()==66&&s.starts_with("0x")&&std::all_of(s.begin()+2,s.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');});}
 void finish(bool accepted,bool yielded=false,const char* reason=nullptr){if(finished_)return;finished_=true;timer_.cancel();pending_.clear();auto done=std::move(done_);Json result=yielded?Json{{"yielded",true}}:Json{{"accepted",accepted}};if(reason)result["reason"]=reason;done(std::move(result));}
 template<class F>Reply callback(F handler){auto once=std::make_shared<std::atomic<bool>>(false);return [self=shared_from_this(),once,handler](Json value){if(once->exchange(true))return;asio::post(self->io_,[self,handler,value=std::move(value)]()mutable{if(self->finished_)return;try{handler(*self,std::move(value));}catch(...){self->finish(false);}});};}
 void fetch(const std::string& id){
  if(id==anchor_||knowledge_->acknowledged(id)){deliver();return;}
  if(pending_.size()>=maximum_){preparation_boundary_=id;finish(false,false,"dependency-depth-limit");return;}
  auto reply=callback([id](auto& self,Json value){self.exported(id,std::move(value));});
  if(exports_)if(auto cached=exports_->find(id)){
   ++cache_hits_;
   reply(Json{{"root",id},{"available",true},{"known",true},{"anchor",cached->anchor_at_export},{"parentRoot",cached->parent},{"envelope",Json::parse(cached->wire)}});return;
  }
  ++exports_requested_;try{core_("nano_capellaExport",Json::array({token_,"block",id}),reply);}catch(...){reply(nullptr);}
 }
 void exported(const std::string& id,Json value){
  if(!value.is_object()||value.value("root",Json())!=id){finish(false);return;}
  // The authenticated local core can have pruned to a newer anchor since the
  // daemon started. Its anchor need not have an exportable signed envelope.
  // Probe a signed local anchor through normal remote admission before walking
  // older history. Its local authentication never substitutes for the peer ACK.
  // Stop only an unavailable anchor walk: never claim remote admission of the anchor
  // itself. Every retained descendant still requires the peer's actual ACK.
  if(value.value("anchor",Json())==true&&value.value("available",Json())!=true){
   if(value.value("known",Json())!=true||pending_.empty()){unavailable_export_=value.value("known",Json())==true&&pending_.empty();finish(false);return;}
   deliver();return;
  }
  if(value.value("available",Json())!=true){unavailable_export_=value.value("available",Json())==false;finish(false);return;}
  auto wire=value.at("envelope").dump();if(wire.capacity()>budget_-bytes_){finish(false);return;}
  const auto message=ingress_->decode(wire);if(!message||message->kind!=AltairWireKind::block||message->ssz.size()<184){finish(false);return;}
  const auto& b=message->ssz;auto number=[&](size_t at,size_t n){uint64_t result=0;for(size_t i=0;i<n;++i)result|=uint64_t(b.at(at+i))<<(8*i);return result;};
  if(number(0,4)!=100||number(180,4)!=84){finish(false);return;}
  const auto slot=number(100,8);if(child_slot_&&slot>=*child_slot_){finish(false);return;}
  std::string parent="0x";const char* digits="0123456789abcdef";for(size_t i=116;i<148;++i){parent.push_back(digits[b[i]>>4]);parent.push_back(digits[b[i]&15]);}
  if(value.at("parentRoot")!=parent||parent==id){finish(false);return;}
  const bool signed_anchor=value.value("anchor",Json())==true&&value.value("known",Json())==true;
  if(exports_)exports_->remember(id,wire,parent,signed_anchor);
  child_slot_=slot;bytes_+=wire.capacity();pending_.push_back({id,std::move(wire)});pending_high_water_=std::max(pending_high_water_,pending_.size());
  if(signed_anchor){anchor_probe_pending_=true;anchor_probe_root_=id;anchor_probe_parent_=parent;deliver();}else {preparation_boundary_=parent;fetch(parent);}
 }
 void deliver(){
  preparation_boundary_.clear(); // Traversal progress is not an in-flight delivery retry.
  if(pending_.empty()){finish(true);return;}
  // A local yield never claims admission of the still-pending target. Only
  // authenticated dependency ACKs survive in the peer/session knowledge.
  if(quantum_&&quantum_->exhausted()){
   // Retain only bounded authenticated local root metadata, never an ACK.
   yielded_roots_.reserve(pending_.size());
   for(auto i=pending_.rbegin();i!=pending_.rend();++i)yielded_roots_.push_back(i->root);
   finish(false,true);return;}
  auto frame=Json::parse(pending_.back().wire);const auto id=pending_.back().root;
  auto reply=callback([id](auto& self,Json value){
   if(!value.is_object()||value.size()!=1||value.value("accepted",Json())!=true){
    // Only a definitive refusal may resume parent preparation. No request or
    // unconfirmed payload is automatically retried; no ACK is inferred.
    if(self.anchor_probe_pending_&&id==self.anchor_probe_root_&&value==Json{{"accepted",false}}){self.anchor_probe_pending_=false;self.fetch(self.anchor_probe_parent_);return;}
    self.finish(false);return;
   }
   if(id==self.anchor_probe_root_)self.anchor_probe_pending_=false;
   ++self.payloads_admitted_;self.knowledge_->acknowledge(id);if(self.quantum_)++self.quantum_->admitted;self.bytes_-=self.pending_.back().wire.capacity();self.pending_.pop_back();self.deliver();
  });++payloads_sent_;try{send_(std::move(frame),reply);}catch(...){reply(nullptr);}
 }
public:
 // Local diagnostics only; these counters never substitute for receiver ACKs.
 Json progress()const{return Json{{"target",target_},{"preparationBoundary",preparation_boundary_.empty()?Json(nullptr):Json(preparation_boundary_)},{"exportsRequested",exports_requested_},{"exportCacheHits",cache_hits_},{"sendAttempts",payloads_sent_},{"payloadsAdmitted",payloads_admitted_},{"pendingHighWater",pending_high_water_},{"unavailableExport",unavailable_export_}};}
 // Local authenticated traversal metadata only; never a peer acknowledgement.
 const std::vector<std::string>& yielded_roots()const{return yielded_roots_;}
 const std::string& preparation_boundary()const{return preparation_boundary_;}
 bool unavailable_export()const{return unavailable_export_;}
 CapellaBlockDelivery(asio::io_context& io,AltairCoreDispatch::Call core,std::function<void(Json,Reply)> send,std::string token,std::string anchor,std::string target,std::shared_ptr<AltairMessageIngress> ingress,std::shared_ptr<AltairRelayProgress> knowledge,size_t maximum,size_t budget,Reply done,std::shared_ptr<CapellaDeliveryQuantum> quantum={},std::shared_ptr<CapellaExportCache> exports={}):io_(io),core_(std::move(core)),send_(std::move(send)),token_(std::move(token)),anchor_(std::move(anchor)),target_(std::move(target)),ingress_(std::move(ingress)),knowledge_(std::move(knowledge)),maximum_(maximum),budget_(budget),quantum_(std::move(quantum)),exports_(std::move(exports)),done_(std::move(done)),timer_(io){if(!core_||!send_||token_.empty()||!root(anchor_)||!root(target_)||!ingress_||!knowledge_||!maximum_||!budget_||!done_||(quantum_&&!quantum_->maximum))throw std::invalid_argument("Invalid Capella block delivery");}
 void start(std::chrono::steady_clock::duration timeout=std::chrono::seconds(30)){
  if(started_||finished_)throw std::logic_error("Capella block delivery already started/stopped");if(timeout<=std::chrono::steady_clock::duration::zero())throw std::invalid_argument("Invalid Capella delivery timeout");started_=true;
  timer_.expires_after(timeout);timer_.async_wait([self=shared_from_this()](auto error){if(!error)self->finish(false);});fetch(target_);
 }
 // Same executor as start; late ACKs are never cached after cancellation.
 void cancel(){finish(false);}
};
