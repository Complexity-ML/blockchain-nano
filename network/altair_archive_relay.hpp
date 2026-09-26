#pragma once
#include "altair_core_dispatch.hpp"
#include "timed_reply.hpp"
struct AltairRelayProgress {std::string cursor="0x",epoch;bool retry_votes=false;};
// One bounded attempt: inventory -> durable export -> peer acknowledgement.
// Caller serializes attempts and keeps per-peer progress only for that session.
class AltairArchiveRelay:public std::enable_shared_from_this<AltairArchiveRelay>{
 using Call=AltairCoreDispatch::Call;
 asio::io_context& io_;Call core_;std::function<void(Json,Reply)> send_;const std::string token_;
 std::shared_ptr<AltairMessageIngress> ingress_;std::shared_ptr<AltairRelayProgress> progress_;Reply done_;asio::steady_timer deadline_;
 const AltairWireKind kind_;
 const char* entries()const{return kind_==AltairWireKind::block?"blocks":"votes";}
 bool finished_=false,started_=false;std::string root_;
 static bool root(const Json& j){if(!j.is_string())return false;const auto& s=j.get_ref<const std::string&>();return s.size()==66&&s.starts_with("0x")&&std::all_of(s.begin()+2,s.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');});}
 void finish(bool ok){if(finished_)return;finished_=true;deadline_.cancel();auto done=std::move(done_);done(Json{{"success",ok}});}
 template<class F>Reply callback(F handler){auto once=std::make_shared<std::atomic<bool>>(false);return [self=shared_from_this(),once,handler](Json result){if(once->exchange(true))return;asio::post(self->io_,[self,handler,result=std::move(result)]()mutable{if(!self->finished_){try{handler(*self,std::move(result));}catch(...){self->finish(false);}}});};}
 void inventory(Json page){
  if(!page.is_object()){progress_->cursor="0x";progress_->epoch.clear();finish(false);return;}
  if(!root(page.at("catalogEpoch"))||!page.at(entries()).is_array()||page.at(entries()).size()>1||page.at("missingSignedBlocks")!=0){finish(false);return;}
  const auto epoch=page.at("catalogEpoch").get<std::string>();
  if(!progress_->epoch.empty()&&progress_->epoch!=epoch){progress_->cursor="0x";progress_->epoch=epoch;progress_->retry_votes=false;finish(false);return;}progress_->epoch=epoch;
  if(page.at(entries()).empty()){
   // A negative response is not a delivery acknowledgement. Revisit deferred
   // votes after one complete pass, allowing newer eligible votes through first.
   if(kind_==AltairWireKind::attestation&&progress_->retry_votes){progress_->cursor="0x";progress_->retry_votes=false;}
   finish(true);return;
  }
  const auto& item=page.at(entries())[0];if(!root(item.at("root"))||item.at("root")==progress_->cursor){finish(false);return;}root_=item.at("root").get<std::string>();
  auto reply=callback([](auto& self,Json result){self.payload(std::move(result));});try{core_(kind_==AltairWireKind::block?"nano_altairBlock":"nano_altairVote",Json::array({token_,root_}),reply);}catch(...){reply(nullptr);}
 }
 void payload(Json value){
  if(!value.is_object()||value.value("available",Json())!=true||value.value("root",Json())!=root_){finish(false);return;}
  const auto& envelope=value.at("envelope");auto message=ingress_->decode_compact(envelope);if(!message||message->kind!=kind_){finish(false);return;}
  std::string encoded="0x";encoded.reserve(2+message->ssz.size()*2);const char* digits="0123456789abcdef";for(auto b:message->ssz){encoded.push_back(digits[b>>4]);encoded.push_back(digits[b&15]);}
  auto frame=envelope;frame.erase("sszBase64");frame["ssz"]=std::move(encoded);
  auto reply=callback([](auto& self,Json result){const bool accepted=result.is_object()&&result.size()==1&&result.value("accepted",Json())==true;if(accepted)self.progress_->cursor=self.root_;
   else if(self.kind_==AltairWireKind::attestation&&result.is_object()&&result.size()==1&&result.value("accepted",Json())==false){self.progress_->cursor=self.root_;self.progress_->retry_votes=true;}
   self.finish(accepted);});try{send_(std::move(frame),reply);}catch(...){reply(nullptr);}
 }
public:
 AltairArchiveRelay(asio::io_context& io,Call core,std::function<void(Json,Reply)> send,std::string token,std::shared_ptr<AltairMessageIngress> ingress,std::shared_ptr<AltairRelayProgress> progress,Reply done,AltairWireKind kind=AltairWireKind::block):io_(io),core_(std::move(core)),send_(std::move(send)),token_(std::move(token)),ingress_(std::move(ingress)),progress_(std::move(progress)),done_(std::move(done)),deadline_(io),kind_(kind){if((kind_!=AltairWireKind::block&&kind_!=AltairWireKind::attestation)||!core_||!send_||token_.empty()||!ingress_||!progress_||!done_)throw std::invalid_argument("invalid Altair relay");}
 void start(std::chrono::steady_clock::duration timeout=std::chrono::seconds(30)){if(timeout<=std::chrono::steady_clock::duration::zero())throw std::invalid_argument("invalid relay deadline");if(started_)throw std::logic_error("Altair relay already started");started_=true;deadline_.expires_after(timeout);deadline_.async_wait([self=shared_from_this()](auto error){if(!error)self->finish(false);});auto reply=callback([](auto& self,Json result){self.inventory(std::move(result));});try{core_(kind_==AltairWireKind::block?"nano_altairInventory":"nano_altairVoteInventory",Json::array({token_,progress_->cursor,"0x1"}),reply);}catch(...){reply(nullptr);}}
};
