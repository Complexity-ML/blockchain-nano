#pragma once
#include "altair_core_dispatch.hpp"
#include <sstream>

// One stable catalog, bounded pages and one payload/delivery in flight.
// The delivery callback must resolve block dependencies and require consensus
// admission before acknowledging. Root ordering is NOT block execution ordering.
class CapellaCatalogRelay:public std::enable_shared_from_this<CapellaCatalogRelay>{
public:
 using Call=AltairCoreDispatch::Call;
 using Deliver=std::function<void(std::string,Json,Reply)>;
private:
 asio::io_context& io_;Call core_;Deliver deliver_;std::string token_,kind_,handle_,cursor_,sequence_,digest_,stage_="open";
 std::shared_ptr<AltairMessageIngress> ingress_;Reply done_;asio::steady_timer timer_;
 const size_t page_size_,maximum_;size_t delivered_=0,index_=0;Json entries_,next_;
 bool started_=false,finished_=false,closing_=false;bool desired_success_=false;
 static bool root(const Json& j){if(!j.is_string())return false;const auto& s=j.get_ref<const std::string&>();return s.size()==66&&s.starts_with("0x")&&std::all_of(s.begin()+2,s.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');});}
 static bool quantity(const Json& j){if(!j.is_string())return false;const auto& s=j.get_ref<const std::string&>();return s.size()>=3&&s.size()<=18&&s.starts_with("0x")&&(s.size()==3||s[2]!='0')&&std::all_of(s.begin()+2,s.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');});}
 void complete(bool success,bool closed){if(finished_)return;finished_=true;timer_.cancel();auto done=std::move(done_);done(Json{{"success",success},{"closed",closed},{"delivered",delivered_},{"stage",stage_}});}
 void cleanup(const std::string& handle){try{core_("nano_capellaCatalogClose",Json::array({token_,handle}),[](Json){});}catch(...){}}
 template<class F>Reply callback(F handler,bool late_open=false){auto once=std::make_shared<std::atomic<bool>>(false);return [self=shared_from_this(),once,handler,late_open](Json value){if(once->exchange(true))return;asio::post(self->io_,[self,handler,late_open,value=std::move(value)]()mutable{
   if(self->finished_){if(late_open&&value.is_object()&&value.contains("handle")&&root(value["handle"]))self->cleanup(value["handle"].template get<std::string>());return;}
   try{handler(*self,std::move(value));}catch(...){self->close(false);}
  });};}
 void call(std::string method,Json params,Reply reply){try{core_(std::move(method),std::move(params),reply);}catch(...){reply(nullptr);}}
 // Only this idempotent operation may be retried; never retry delivery or open.
 // A missing transport response remains bounded by the overall deadline.
 void close_attempt(bool retry){
  call("nano_capellaCatalogClose",Json::array({token_,handle_}),callback([retry](auto& self,Json value){
   if(value.is_null()&&retry){self.close_attempt(false);return;}
   self.complete(self.desired_success_&&value==true,value==true);
  }));
 }
 void close(bool success){
  if(finished_||closing_)return;
  if(handle_.empty()){complete(false,false);return;}
  closing_=true;desired_success_=success;stage_="close";
  close_attempt(true);
 }
 void page(){
  stage_="page";std::ostringstream count;count<<"0x"<<std::hex<<std::min(page_size_,maximum_-delivered_);
  call("nano_capellaCatalogPage",Json::array({token_,handle_,cursor_.empty()?Json(nullptr):Json(cursor_),count.str()}),callback([](auto& self,Json value){self.got_page(std::move(value));}));
 }
 void got_page(Json value){
  if(!value.is_object()||value.at("kind")!=kind_||!quantity(value.at("sequence"))||!root(value.at("digest"))||!value.at("entries").is_array()||value.at("entries").size()>std::min(page_size_,maximum_-delivered_)){close(false);return;}
  if(sequence_.empty()){sequence_=value.at("sequence").get<std::string>();digest_=value.at("digest").get<std::string>();}
  if(value.at("sequence")!=sequence_||value.at("digest")!=digest_){close(false);return;}
  std::string previous=cursor_;
  for(const auto& item:value.at("entries")){if(!root(item)||(!previous.empty()&&item.get_ref<const std::string&>()<=previous)){close(false);return;}previous=item.get<std::string>();}
  const auto& next=value.at("next");if(!next.is_null()&&(!root(next)||value.at("entries").empty()||next!=previous)){close(false);return;}
  entries_=std::move(value["entries"]);next_=next;index_=0;
  if(entries_.empty()){close(next_.is_null());return;}export_next();
 }
 void export_next(){
  stage_="export";const auto id=entries_.at(index_).get<std::string>();
  call("nano_capellaExport",Json::array({token_,kind_,id}),callback([id](auto& self,Json value){
   if(!value.is_object()||value.value("available",Json())!=true||value.value("root",Json())!=id){self.close(false);return;}
   auto frame=value.at("envelope");const auto wire=frame.dump();auto message=self.ingress_->decode(wire);
   const auto expected=self.kind_=="block"?AltairWireKind::block:self.kind_=="attestation"?AltairWireKind::attestation:AltairWireKind::attester_slashing;
   if(!message||message->kind!=expected){self.close(false);return;}
   self.stage_="delivery";
   auto reply=self.callback([](auto& s,Json ack){
    if(!ack.is_object()||ack.size()!=1||ack.value("accepted",Json())!=true){s.close(false);return;}
    ++s.delivered_;s.cursor_=s.entries_.at(s.index_).template get<std::string>();++s.index_;
    if(s.index_<s.entries_.size()){s.export_next();return;}
    if(s.next_.is_null()){s.close(true);return;}
    if(s.delivered_==s.maximum_){s.close(false);return;}s.page();
   });
   try{self.deliver_(id,std::move(frame),reply);}catch(...){reply(nullptr);}
  }));
 }
public:
 CapellaCatalogRelay(asio::io_context& io,Call core,Deliver deliver,std::string token,std::string kind,std::shared_ptr<AltairMessageIngress> ingress,size_t page_size,size_t maximum,Reply done):io_(io),core_(std::move(core)),deliver_(std::move(deliver)),token_(std::move(token)),kind_(std::move(kind)),ingress_(std::move(ingress)),done_(std::move(done)),timer_(io),page_size_(page_size),maximum_(maximum){
  if(!core_||!deliver_||token_.empty()||!ingress_||!done_||!page_size_||!maximum_||page_size_>maximum_||(kind_!="block"&&kind_!="attestation"&&kind_!="attester_slashing"))throw std::invalid_argument("Invalid Capella catalog relay");
 }
 void start(std::chrono::steady_clock::duration timeout=std::chrono::seconds(30)){
  if(started_)throw std::logic_error("Capella catalog relay already started");if(timeout<=std::chrono::steady_clock::duration::zero())throw std::invalid_argument("Invalid Capella relay timeout");started_=true;
  timer_.expires_after(timeout);timer_.async_wait([self=shared_from_this()](auto error){if(!error)self->cancel();});
  call("nano_capellaCatalogOpen",Json::array({token_,kind_}),callback([](auto& self,Json value){if(!value.is_object()||!root(value.at("handle"))){self.complete(false,false);return;}self.handle_=value.at("handle").template get<std::string>();self.page();},true));
 }
 // Called on the same executor as start. In-flight delivery has unknown outcome
 // after cancellation; a late acknowledgement cannot turn it into success.
 void cancel(){if(finished_)return;if(!handle_.empty()&&!closing_)cleanup(handle_);stage_="cancelled";complete(false,false);}
};
