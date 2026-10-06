#pragma once
#include "capella_block_delivery.hpp"
#include "capella_catalog_relay.hpp"

// Alternate local authenticated LMD-GHOST priority and a dedicated historical turn.
// Preserve the complete
// historical block catalog. One total deadline and shared dependency quantum.
// The remote receiver still validates every block; a status hint is not an ACK.
class CapellaHeadCatalogRelay:public std::enable_shared_from_this<CapellaHeadCatalogRelay>{
 asio::io_context& io_;AltairCoreDispatch::Call core_;CapellaCatalogRelay::Deliver deliver_;
 std::function<void(Json,Reply)> send_;std::string token_,anchor_;
 std::shared_ptr<AltairMessageIngress> ingress_;std::shared_ptr<AltairRelayProgress> knowledge_;
 size_t dependency_blocks_,dependency_bytes_,page_,maximum_;Reply done_;CapellaCatalogRelay::Known known_;
 std::shared_ptr<CapellaDeliveryQuantum> quantum_;std::shared_ptr<CapellaExportCache> exports_;const bool history_first_;std::optional<Json> history_result_;bool fair_,started_=false,finished_=false;
 asio::steady_timer timer_;std::chrono::steady_clock::time_point deadline_,began_;
 std::shared_ptr<CapellaBlockDelivery> head_;std::shared_ptr<CapellaCatalogRelay> catalog_;
 std::shared_ptr<CapellaCatalogSelection> selection_;
 std::string priority_="unselected",selected_head_;
 void finish(Json result){if(finished_)return;if(result.value("success",false)&&!selected_head_.empty()&&selected_head_!=anchor_&&!knowledge_->acknowledged(selected_head_)){result["success"]=false;if(!result.contains("reason")||result["reason"].is_null()){result["reason"]="head-admission-unconfirmed";result["failureStage"]="head";}}if(result.value("success",false)&&std::chrono::steady_clock::now()>deadline_){result["success"]=false;result["reason"]="head-or-catalog-deadline";result["failureStage"]="catalog";}finished_=true;timer_.cancel();if(head_)head_->cancel();if(catalog_)catalog_->cancel();result["headPriority"]=priority_;if(result.contains("elapsedMs"))result["catalogElapsedMs"]=result["elapsedMs"];result["elapsedMs"]=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-began_).count();auto done=std::move(done_);done(std::move(result));}
 void fail(const char* reason){auto result=history_result_.value_or(Json::object());if(catalog_)result["catalogProgress"]=catalog_->progress();const auto phase=history_result_?"head":catalog_?"catalog":"head";result["success"]=false;result["reason"]=reason;result["stage"]=phase;result["failureStage"]=phase;if(!result.contains("closed"))result["closed"]=!catalog_;finish(std::move(result));}
 void status(){auto reply=callback([self=shared_from_this()](Json value){self->selected(std::move(value));});try{core_("nano_capellaStatus",Json::array({token_}),reply);}catch(...){reply(nullptr);}}
 Reply callback(std::function<void(Json)> handler){auto once=std::make_shared<std::atomic<bool>>(false);return [self=shared_from_this(),once,handler=std::move(handler)](Json value){if(once->exchange(true))return;asio::post(self->io_,[self,handler,value=std::move(value)]()mutable{if(self->finished_)return;try{handler(std::move(value));}catch(...){self->fail("invalid-head-response");}});};}
 void catalog(){
  const auto remaining=deadline_-std::chrono::steady_clock::now();if(remaining<=std::chrono::steady_clock::duration::zero()){fail("head-or-catalog-deadline");return;}
  catalog_=std::make_shared<CapellaCatalogRelay>(io_,core_,deliver_,token_,"block",ingress_,page_,maximum_,callback([self=shared_from_this()](Json result){if(self->history_first_){self->history_result_=std::move(result);self->status();}else self->finish(std::move(result));}),known_,fair_,selection_);catalog_->start(remaining);
 }
 void selected(Json status){
  if(!status.is_object()||!status.contains("headRoot")||!status["headRoot"].is_string()||!status.contains("headSlot")||!status["headSlot"].is_number_unsigned()){fail("head-reference-unavailable");return;}
  const auto id=status["headRoot"].get<std::string>();
  if(id.size()!=66||!id.starts_with("0x")||!std::all_of(id.begin()+2,id.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');})){fail("invalid-head-reference");return;}
  selected_head_=id;
  if(history_first_){
   // History has its own alternating turn under the same total deadline.
   // Status is local authenticated metadata, never a remote admission ACK.
   priority_=id==anchor_||knowledge_->acknowledged(id)?"historical-first-head-confirmed":"historical-first-head-unconfirmed";
   finish(std::move(*history_result_));return;
  }
  if(id==anchor_){priority_="anchor";catalog();return;}
  if(knowledge_->acknowledged(id)){priority_="already-acknowledged";catalog();return;}
  priority_="selected";const auto remaining=deadline_-std::chrono::steady_clock::now();if(remaining<=std::chrono::steady_clock::duration::zero()){fail("head-or-catalog-deadline");return;}
  head_=std::make_shared<CapellaBlockDelivery>(io_,core_,send_,token_,anchor_,id,ingress_,knowledge_,dependency_blocks_,dependency_bytes_,callback([self=shared_from_this()](Json ack){
   if(ack==Json{{"yielded",true}}){self->priority_="dependency-quantum-yield";self->fail("dependency-quantum-yield");return;}
   if(ack==Json{{"accepted",false},{"reason","dependency-depth-limit"}}){
    // A fresh session knows no ancestral ACKs. Seed them through the bounded
    // historical catalog instead of retrying an over-deep head forever.
    self->priority_="dependency-depth-catalog-fallback";self->catalog();return;
   }
   if(ack==Json{{"accepted",false}}){
    // A rejected/incomplete head is never an ACK. Independent historical
    // branches may supply its missing context; preserve the same deadline,
    // admission quantum and peer knowledge while scanning that catalog.
    self->priority_="head-unconfirmed-catalog-fallback";self->catalog();return;
   }
   if(ack!=Json{{"accepted",true}}){self->fail("head-admission-unconfirmed");return;}
   self->priority_="admitted";self->catalog();
  }),quantum_,exports_);head_->start(remaining);
 }
public:
 CapellaHeadCatalogRelay(asio::io_context& io,AltairCoreDispatch::Call core,CapellaCatalogRelay::Deliver deliver,std::function<void(Json,Reply)> send,std::string token,std::string anchor,std::shared_ptr<AltairMessageIngress> ingress,std::shared_ptr<AltairRelayProgress> knowledge,size_t dependency_blocks,size_t dependency_bytes,size_t page,size_t maximum,Reply done,CapellaCatalogRelay::Known known,std::shared_ptr<CapellaDeliveryQuantum> quantum={},bool fair=false,std::shared_ptr<CapellaExportCache> exports={},bool history_first=false,std::shared_ptr<CapellaCatalogSelection> selection={}):io_(io),core_(std::move(core)),deliver_(std::move(deliver)),send_(std::move(send)),token_(std::move(token)),anchor_(std::move(anchor)),ingress_(std::move(ingress)),knowledge_(std::move(knowledge)),dependency_blocks_(dependency_blocks),dependency_bytes_(dependency_bytes),page_(page),maximum_(maximum),done_(std::move(done)),known_(std::move(known)),quantum_(std::move(quantum)),exports_(std::move(exports)),history_first_(history_first),fair_(fair),timer_(io),selection_(std::move(selection)){if(!core_||!deliver_||!send_||token_.empty()||!ingress_||!knowledge_||!dependency_blocks_||!dependency_bytes_||!page_||page_>maximum_||!done_)throw std::invalid_argument("Invalid Capella head catalog relay");}
 void start(std::chrono::steady_clock::duration timeout=std::chrono::seconds(30)){
  if(started_||finished_||timeout<=std::chrono::steady_clock::duration::zero())throw std::logic_error("Invalid Capella head relay start");started_=true;began_=std::chrono::steady_clock::now();deadline_=began_+timeout;timer_.expires_at(deadline_);timer_.async_wait([self=shared_from_this()](auto e){if(!e)self->fail("head-or-catalog-deadline");});
  if(history_first_){priority_="historical-first";catalog();}else status();
 }
 void cancel(){if(!finished_)fail("cancelled");}
};
