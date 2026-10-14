#pragma once
#include "altair_core_dispatch.hpp"
#include <sstream>
#include <openssl/rand.h>
#include <openssl/sha.h>

// Per-peer/category selection only. Unknown/refused roots remain unacknowledged
// and are retried after the next wrap. Reset on local generation or peer session.
struct CapellaCatalogSelection {std::string generation,after;};

// One stable catalog, bounded pages and one payload/delivery in flight.
// The delivery callback must resolve block dependencies and require consensus
// admission before acknowledging. Root ordering is NOT block execution ordering.
class CapellaCatalogRelay:public std::enable_shared_from_this<CapellaCatalogRelay>{
public:
 static constexpr auto default_timeout=std::chrono::seconds(30);
 using Call=AltairCoreDispatch::Call;
 using Known=std::function<bool(const std::string&)>;
 using Deliver=std::function<void(std::string,Json,Reply)>;
private:
 asio::io_context& io_;Call core_;Deliver deliver_;Known known_;std::string token_,kind_,handle_,cursor_,sequence_,digest_,stage_="open",generation_,request_;
 std::shared_ptr<AltairMessageIngress> ingress_;Reply done_;asio::steady_timer timer_,capture_retry_timer_;std::chrono::steady_clock::duration capture_retry_{};size_t capture_busy_retries_=0;
 const size_t page_size_,maximum_;size_t delivered_=0,scanned_=0,rejected_=0,index_=0;Json entries_,next_;
 bool started_=false,finished_=false,closing_=false;bool desired_success_=false;const bool yield_after_payload_quantum_;
 std::string failure_stage_,reason_,termination_reason_,termination_stage_;bool catalog_exhausted_=false;std::chrono::steady_clock::time_point started_at_=std::chrono::steady_clock::now();
 std::shared_ptr<CapellaCatalogSelection> selection_;std::string started_after_;
 size_t exports_=0,skipped_known_=0,payloads_admitted_=0;std::optional<uint64_t> sync_slot_;
 void failure(std::string reason){termination_reason_=reason;termination_stage_=stage_;if(reason_.empty()){reason_=std::move(reason);failure_stage_=stage_;}close(false);}
 static bool root(const Json& j){if(!j.is_string())return false;const auto& s=j.get_ref<const std::string&>();return s.size()==66&&s.starts_with("0x")&&std::all_of(s.begin()+2,s.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');});}
 static bool quantity(const Json& j){if(!j.is_string())return false;const auto& s=j.get_ref<const std::string&>();return s.size()>=3&&s.size()<=18&&s.starts_with("0x")&&(s.size()==3||s[2]!='0')&&std::all_of(s.begin()+2,s.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');});}
 void complete(bool success,bool closed){if(finished_)return;if(!success&&reason_.empty()){reason_="incomplete";failure_stage_=stage_;}finished_=true;timer_.cancel();capture_retry_timer_.cancel();auto done=std::move(done_);done(Json{{"success",success},{"closed",closed},{"catalogExhausted",catalog_exhausted_},{"terminationReason",termination_reason_.empty()?Json(nullptr):Json(termination_reason_)},{"terminationStage",termination_stage_.empty()?Json(nullptr):Json(termination_stage_)},{"delivered",delivered_},{"scanned",scanned_},{"rejected",rejected_},{"stage",stage_},{"selectionStartAfter",started_after_.empty()?Json(nullptr):Json(started_after_)},{"selectionAfter",selection_&&!selection_->after.empty()?Json(selection_->after):Json(nullptr)},{"failureStage",failure_stage_.empty()?Json(nullptr):Json(failure_stage_)},{"reason",reason_.empty()?Json(nullptr):Json(reason_)},{"captureBusyRetries",capture_busy_retries_},{"exports",exports_},{"payloadsAdmitted",payloads_admitted_},{"skippedKnown",skipped_known_},{"syncSlot",sync_slot_?Json(*sync_slot_):Json(nullptr)},{"elapsedMs",std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-started_at_).count()}});}
 Json release_params(){return Json::array({token_,generation_,request_});}
 void cleanup_request(){if(request_.empty())return;try{core_("nano_capellaCatalogRelease",release_params(),[](Json){});}catch(...){}}
 void cleanup(const std::string& handle){try{core_("nano_capellaCatalogClose",Json::array({token_,handle}),[](Json){});}catch(...){}}
 template<class F>Reply callback(F handler,bool late_open=false){auto once=std::make_shared<std::atomic<bool>>(false);return [self=shared_from_this(),once,handler,late_open](Json value){if(once->exchange(true))return;asio::post(self->io_,[self,handler,late_open,value=std::move(value)]()mutable{
   if(self->finished_){if(late_open&&value.is_object()&&value.contains("handle")&&root(value["handle"]))self->cleanup(value["handle"].template get<std::string>());return;}
   try{handler(*self,std::move(value));}catch(...){self->close(false);}
  });};}
 void call(std::string method,Json params,Reply reply){try{core_(std::move(method),std::move(params),reply);}catch(...){reply(nullptr);}}
 // Only this idempotent operation may be retried; never retry delivery or open.
 // A missing transport response remains bounded by the overall deadline.
 void close_attempt(bool retry){
  call("nano_capellaCatalogRelease",release_params(),callback([retry](auto& self,Json value){
   if(value.is_null()&&retry){self.close_attempt(false);return;}
   if(value!=true&&self.reason_.empty()){self.reason_="release-unconfirmed";self.failure_stage_="close";}self.complete(self.desired_success_&&value==true,value==true);
  }));
 }
 void close(bool success){
  if(finished_||closing_)return;
  if(termination_reason_.empty()){termination_reason_=success?"catalog-exhausted":"invalid-response";termination_stage_=stage_;}
  catalog_exhausted_=success;
  if(!success&&reason_.empty()){reason_="invalid-response";failure_stage_=stage_;}
  if(request_.empty()){complete(false,false);return;}
  capture_retry_timer_.cancel();closing_=true;desired_success_=success&&rejected_==0;stage_="close";
  close_attempt(true);
 }
 void acquire(){
  if(finished_||closing_)return;
  call("nano_capellaCatalogAcquire",Json::array({token_,kind_,generation_,request_}),callback([](auto& s,Json value){
   if(s.closing_)return;
   if(core_capture_busy(value)){
    if(s.capture_retry_==std::chrono::steady_clock::duration::zero()){s.failure("capture-busy");return;}
    ++s.capture_busy_retries_;s.capture_retry_timer_.expires_after(s.capture_retry_);
    s.capture_retry_timer_.async_wait([self=s.shared_from_this()](auto error){if(!error&&!self->finished_&&!self->closing_)self->acquire();});return;
   }
   if(!value.is_object()||!value.contains("handle")||!root(value["handle"])){s.close(false);return;}
   s.handle_=value["handle"].template get<std::string>();if(s.started_after_.empty())s.page();else s.seek();
  },true));
 }
 void page(){
  stage_="page";std::ostringstream count;count<<"0x"<<std::hex<<std::min(page_size_,maximum_-scanned_);
  call("nano_capellaCatalogPage",Json::array({token_,handle_,cursor_.empty()?Json(nullptr):Json(cursor_),count.str()}),callback([](auto& self,Json value){self.got_page(std::move(value));}));
 }
 void exhausted(){
  if(selection_)selection_->after.clear();
  if(!started_after_.empty()){failure("selection-wrap-required");return;}
  close(true);
 }
 void seek(){
  stage_="seek";std::ostringstream count;count<<"0x"<<std::hex<<std::min(page_size_,maximum_);
  call("nano_capellaCatalogSeek",Json::array({token_,handle_,started_after_,count.str()}),callback([](auto& self,Json value){self.got_page(std::move(value));}));
 }
 void got_page(Json value){
  if(!value.is_object()||value.at("kind")!=kind_||!quantity(value.at("sequence"))||!root(value.at("digest"))||!value.at("entries").is_array()||value.at("entries").size()>std::min(page_size_,maximum_-scanned_)){close(false);return;}
  if(sequence_.empty()){sequence_=value.at("sequence").get<std::string>();digest_=value.at("digest").get<std::string>();}
  if(value.at("sequence")!=sequence_||value.at("digest")!=digest_){close(false);return;}
  std::string previous=cursor_;
  for(const auto& item:value.at("entries")){if(!root(item)||(!previous.empty()&&item.get_ref<const std::string&>()<=previous)){close(false);return;}previous=item.get<std::string>();}
  const auto& next=value.at("next");if(!next.is_null()&&(!root(next)||value.at("entries").empty()||next!=previous)){close(false);return;}
  entries_=std::move(value["entries"]);next_=next;index_=0;
  if(entries_.empty()){if(next_.is_null())exhausted();else close(false);return;}export_next();
 }
 void advance(bool acknowledged=true){
  ++scanned_;if(acknowledged)++delivered_;cursor_=entries_.at(index_).get<std::string>();++index_;
  if(index_<entries_.size()){export_next();return;}
  if(next_.is_null()){exhausted();return;}
  if(scanned_==maximum_){failure("entry-limit");return;}
  // Release the stable snapshot after a quantum of admitted payloads, at a page boundary. The peer scheduler
  // can serve another category; admitted-root knowledge survives this yield.
  // Cached entries do not consume the quantum. Overshoot is bounded by one
  // page minus one entry; scan limits still bound the entire attempt.
  if(yield_after_payload_quantum_&&payloads_admitted_>=page_size_){failure("payload-quantum-yield");return;}page();
 }
 void export_next(){
  stage_="export";const auto id=entries_.at(index_).get<std::string>();
  if(selection_)selection_->after=id;
  if(known_&&known_(id)){++skipped_known_;auto next=callback([](auto& self,Json){self.advance();});next(nullptr);return;}
  ++exports_;
  call("nano_capellaExport",Json::array({token_,kind_,id}),callback([id](auto& self,Json value){
   if(value.is_null()){self.failure("export-unconfirmed");return;}
   if(!value.is_object()||value.value("root",Json())!=id){self.failure("export-invalid-root");return;}
   if(value.value("available",Json())!=true){self.failure("export-unavailable");return;}
   auto frame=value.at("envelope");const auto wire=frame.dump();auto message=self.ingress_->decode(wire);
   const auto expected=self.kind_=="sync_committee_message"?AltairWireKind::sync_committee_message:self.kind_=="block"?AltairWireKind::block:self.kind_=="attestation"?AltairWireKind::attestation:self.kind_=="voluntary_exit"?AltairWireKind::voluntary_exit:self.kind_=="bls_to_execution_change"?AltairWireKind::bls_to_execution_change:self.kind_=="proposer_slashing"?AltairWireKind::proposer_slashing:AltairWireKind::attester_slashing;
   if(!message||message->kind!=expected){self.failure("invalid-envelope");return;}
   if(expected==AltairWireKind::sync_committee_message){uint64_t slot=0;for(size_t i=0;i<8;++i)slot|=uint64_t(message->ssz[i])<<(8*i);self.sync_slot_=slot;}
   if(expected==AltairWireKind::proposer_slashing||expected==AltairWireKind::voluntary_exit||expected==AltairWireKind::bls_to_execution_change||expected==AltairWireKind::sync_committee_message){unsigned char digest[32];SHA256(message->ssz.data(),message->ssz.size(),digest);std::string actual="0x";for(auto b:digest){actual+="0123456789abcdef"[b>>4];actual+="0123456789abcdef"[b&15];}if(actual!=id){self.failure("digest-mismatch");return;}}
   self.stage_="delivery";
   auto reply=self.callback([](auto& s,Json ack){
    if(ack.is_object()&&ack.size()==1&&ack.value("yielded",Json())==true){s.failure("dependency-quantum-yield");return;}
    if(s.kind_=="block"&&ack==Json{{"accepted",false},{"reason","dependency-depth-limit"}}){
     // A local preparation bound is not a remote ACK. Keep this attempt failed,
     // but scan later roots so shallow historical ancestors can seed catch-up.
     ++s.rejected_;if(s.reason_.empty()){s.reason_="dependency-depth-limit";s.failure_stage_="delivery";}
     s.advance(false);return;
    }
    if(!ack.is_object()||ack.size()!=1||ack.value("accepted",Json())!=true){
     const bool rejected=ack.is_object()&&ack.size()==1&&ack.value("accepted",Json())==false;
     // Blocks may need incremental context or another branch; votes can expire. A
     // definitive refusal must not starve later entries in this bounded scan.
     // Never cache it as an ACK, retry it here, or hide the failed attempt.
     if(rejected&&(s.kind_=="attestation"||s.kind_=="block")){
      ++s.rejected_;if(s.reason_.empty()){s.reason_="admission-rejected";s.failure_stage_="delivery";}
      s.advance(false);return;
     }
     s.failure(rejected?"admission-rejected":"delivery-unconfirmed");return;
    }
    ++s.payloads_admitted_;s.advance();
   });
   try{self.deliver_(id,std::move(frame),reply);}catch(...){reply(nullptr);}
  }));
 }
public:
 // Executor-confined observation only. In-progress counters are not ACKs or
 // proof of handle release; the owner may snapshot them before cancellation.
 Json progress()const{return Json{{"scanned",scanned_},{"rejected",rejected_},{"delivered",delivered_},{"captureBusyRetries",capture_busy_retries_},{"exports",exports_},{"payloadsAdmitted",payloads_admitted_},{"skippedKnown",skipped_known_},{"stage",stage_},{"selectionStartAfter",started_after_.empty()?Json(nullptr):Json(started_after_)},{"selectionAfter",selection_&&!selection_->after.empty()?Json(selection_->after):Json(nullptr)},{"catalogExhausted",catalog_exhausted_},{"firstFailureReason",reason_.empty()?Json(nullptr):Json(reason_)},{"firstFailureStage",failure_stage_.empty()?Json(nullptr):Json(failure_stage_)},{"terminationReason",termination_reason_.empty()?Json(nullptr):Json(termination_reason_)},{"terminationStage",termination_stage_.empty()?Json(nullptr):Json(termination_stage_)},{"inProgress",!finished_}};}
 CapellaCatalogRelay(asio::io_context& io,Call core,Deliver deliver,std::string token,std::string kind,std::shared_ptr<AltairMessageIngress> ingress,size_t page_size,size_t maximum,Reply done,Known known={},bool yield_after_payload_quantum=false,std::shared_ptr<CapellaCatalogSelection> selection={}):io_(io),core_(std::move(core)),deliver_(std::move(deliver)),known_(std::move(known)),token_(std::move(token)),kind_(std::move(kind)),ingress_(std::move(ingress)),done_(std::move(done)),timer_(io),capture_retry_timer_(io),page_size_(page_size),maximum_(maximum),yield_after_payload_quantum_(yield_after_payload_quantum),selection_(std::move(selection)){
  if(!core_||!deliver_||token_.empty()||!ingress_||!done_||!page_size_||!maximum_||page_size_>maximum_||(kind_!="block"&&kind_!="attestation"&&kind_!="attester_slashing"&&kind_!="voluntary_exit"&&kind_!="bls_to_execution_change"&&kind_!="sync_committee_message"&&kind_!="proposer_slashing"))throw std::invalid_argument("Invalid Capella catalog relay");
 }
 void start(std::chrono::steady_clock::duration timeout=default_timeout,std::chrono::steady_clock::duration capture_retry={}){
  if(capture_retry<std::chrono::steady_clock::duration::zero())throw std::invalid_argument("Invalid capture retry interval");if(started_)throw std::logic_error("Capella catalog relay already started");if(timeout<=std::chrono::steady_clock::duration::zero())throw std::invalid_argument("Invalid Capella relay timeout");capture_retry_=capture_retry;started_=true;started_at_=std::chrono::steady_clock::now();
  timer_.expires_after(timeout);timer_.async_wait([self=shared_from_this()](auto error){if(!error)self->cancel();});
  call("nano_capellaCatalogGeneration",Json::array({token_}),callback([](auto& self,Json generation){
   if(!root(generation)){self.complete(false,false);return;}self.generation_=generation.template get<std::string>();
   unsigned char bytes[32];if(RAND_bytes(bytes,sizeof(bytes))!=1){self.complete(false,false);return;}
   if(self.selection_){/* Request retirement rotates this namespace; selection is bounded by peer/session lifetime, not the request nonce. */self.selection_->generation=self.generation_;self.started_after_=self.selection_->after;self.cursor_=self.started_after_;}
   self.request_="0x";for(auto byte:bytes){self.request_+="0123456789abcdef"[byte>>4];self.request_+="0123456789abcdef"[byte&15];}
   self.acquire();
  }));
 }
 // Called on the same executor as start. In-flight delivery has unknown outcome
 // after cancellation; a late acknowledgement cannot turn it into success.
 void cancel(){if(finished_)return;termination_reason_="cancelled-or-deadline";termination_stage_=stage_;if(reason_.empty()){reason_="cancelled-or-deadline";failure_stage_=stage_;}if(!request_.empty()&&!closing_)cleanup_request();stage_="cancelled";complete(false,false);}
};
