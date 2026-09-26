#pragma once
#include "altair_core_dispatch.hpp"
#include "timed_reply.hpp"
#include <deque>
#include <map>
#include <set>
#include <sstream>
struct AltairRelayProgress {
 std::string cursor="0x",epoch;bool retry_votes=false;
 // Session-scoped delivery knowledge, not consensus state or a validator cap.
 // Eviction merely permits retransmission. A peer/session reset replaces this
 // object, so a restarted core must acknowledge the vote again.
 explicit AltairRelayProgress(size_t capacity=2048):capacity_(capacity){}
 bool acknowledged(const std::string& root)const{return known_.contains(root);}
 void acknowledge(const std::string& root){
  if(!capacity_||known_.contains(root))return;
  if(order_.size()>=capacity_){known_.erase(order_.front());order_.pop_front();}
  const auto [entry,inserted]=known_.insert(root);
  try{order_.push_back(root);}catch(...){if(inserted)known_.erase(entry);throw;}
 }
 size_t retained()const{return known_.size();}
private:
 size_t capacity_;std::set<std::string> known_;std::deque<std::string> order_;
};
// One bounded attempt: inventory -> durable export -> peer acknowledgement.
// Caller serializes attempts and keeps per-peer progress only for that session.
class AltairArchiveRelay:public std::enable_shared_from_this<AltairArchiveRelay>{
 using Call=AltairCoreDispatch::Call;
 asio::io_context& io_;Call core_;std::function<void(Json,Reply)> send_;const std::string token_;
 std::shared_ptr<AltairMessageIngress> ingress_;std::shared_ptr<AltairRelayProgress> progress_;Reply done_;asio::steady_timer deadline_;
 const AltairWireKind kind_;const size_t cached_scan_budget_;size_t cached_skips_=0;
 const char* entries()const{return kind_==AltairWireKind::block?"blocks":kind_==AltairWireKind::attester_slashing?"slashings":kind_==AltairWireKind::voluntary_exit?"exits":"votes";}
 bool finished_=false,started_=false;std::string root_;
 std::string stage_="inventory";std::chrono::steady_clock::time_point began_,stage_started_;
 std::map<std::string,int64_t> stage_us_;
 void enter_stage(const std::string& next){const auto now=std::chrono::steady_clock::now();stage_us_[stage_]+=std::chrono::duration_cast<std::chrono::microseconds>(now-stage_started_).count();stage_started_=now;stage_=next;}
 static bool root(const Json& j){if(!j.is_string())return false;const auto& s=j.get_ref<const std::string&>();return s.size()==66&&s.starts_with("0x")&&std::all_of(s.begin()+2,s.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');});}
 void finish(bool ok){if(finished_)return;finished_=true;enter_stage(stage_);deadline_.cancel();auto done=std::move(done_);done(Json{{"success",ok},{"stage",stage_},{"stageUs",stage_us_},{"elapsedMs",std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-began_).count()}});}
 template<class F>Reply callback(F handler){auto once=std::make_shared<std::atomic<bool>>(false);return [self=shared_from_this(),once,handler](Json result){if(once->exchange(true))return;asio::post(self->io_,[self,handler,result=std::move(result)]()mutable{if(!self->finished_){try{handler(*self,std::move(result));}catch(...){self->finish(false);}}});};}
 void request_inventory(){
  enter_stage("inventory");
  auto reply=callback([](auto& self,Json result){self.inventory(std::move(result));});
  const auto count=kind_==AltairWireKind::attestation?std::min(size_t(32),cached_scan_budget_-cached_skips_):size_t(1);
  std::ostringstream encoded_count;encoded_count<<"0x"<<std::hex<<count;
  try{core_(kind_==AltairWireKind::block?"nano_altairInventory":kind_==AltairWireKind::attester_slashing?"nano_altairSlashingInventory":kind_==AltairWireKind::voluntary_exit?"nano_altairExitInventory":"nano_altairVoteInventory",Json::array({token_,progress_->cursor,encoded_count.str()}),reply);}catch(...){reply(nullptr);}
 }
 void inventory(Json page){
  if(!page.is_object()){progress_->cursor="0x";progress_->epoch.clear();finish(false);return;}
  if(!root(page.at("catalogEpoch"))||!page.at(entries()).is_array()||page.at(entries()).size()>(kind_==AltairWireKind::attestation?std::min(size_t(32),cached_scan_budget_-cached_skips_):size_t(1))||page.at("missingSignedBlocks")!=0){finish(false);return;}
  const auto epoch=page.at("catalogEpoch").get<std::string>();
  if(!progress_->epoch.empty()&&progress_->epoch!=epoch){progress_->cursor="0x";progress_->epoch=epoch;progress_->retry_votes=false;finish(false);return;}progress_->epoch=epoch;
  if(page.at(entries()).empty()){
   // A negative response is not a delivery acknowledgement. Revisit deferred
   // votes after one complete pass, allowing newer eligible votes through first.
   if(kind_!=AltairWireKind::block&&progress_->retry_votes){progress_->cursor="0x";progress_->retry_votes=false;}
   finish(true);return;
  }
  std::set<std::string> page_roots;
  for(const auto& item:page.at(entries())){
   if(!item.is_object()||!item.contains("root")||!root(item.at("root"))||item.at("root")==progress_->cursor||!page_roots.insert(item.at("root").get<std::string>()).second){finish(false);return;}
  }
  for(const auto& item:page.at(entries())){
   root_=item.at("root").get<std::string>();
   if(kind_==AltairWireKind::attestation&&progress_->acknowledged(root_)){
    progress_->cursor=root_;enter_stage("known");++cached_skips_;
    continue;
   }
   // Only acknowledged entries may be skipped. Export/send at most one new
   // item, retaining the cursor on its predecessor if the reply is lost.
   enter_stage("payload");auto reply=callback([](auto& self,Json result){self.payload(std::move(result));});try{core_(kind_==AltairWireKind::block?"nano_altairBlock":kind_==AltairWireKind::attester_slashing?"nano_altairSlashing":kind_==AltairWireKind::voluntary_exit?"nano_altairExit":"nano_altairVote",Json::array({token_,root_}),reply);}catch(...){reply(nullptr);}return;
  }
  // A page amortizes local RPC latency but does not increase the scan budget.
  if(cached_skips_<cached_scan_budget_)request_inventory();else finish(true);

 }
 void payload(Json value){
  if(!value.is_object()||value.value("available",Json())!=true||value.value("root",Json())!=root_){finish(false);return;}
  const auto& envelope=value.at("envelope");auto message=ingress_->decode_compact(envelope);if(!message||message->kind!=kind_){finish(false);return;}
  std::string encoded="0x";encoded.reserve(2+message->ssz.size()*2);const char* digits="0123456789abcdef";for(auto b:message->ssz){encoded.push_back(digits[b>>4]);encoded.push_back(digits[b&15]);}
  auto frame=envelope;frame.erase("sszBase64");frame["ssz"]=std::move(encoded);
  enter_stage("ack");auto reply=callback([](auto& self,Json result){const bool accepted=result.is_object()&&result.size()==1&&result.value("accepted",Json())==true;if(accepted){self.progress_->cursor=self.root_;if(self.kind_==AltairWireKind::attestation)self.progress_->acknowledge(self.root_);}
   else if(self.kind_!=AltairWireKind::block&&result.is_object()&&result.size()==1&&result.value("accepted",Json())==false){self.progress_->cursor=self.root_;self.progress_->retry_votes=true;}
   self.finish(accepted);});try{send_(std::move(frame),reply);}catch(...){reply(nullptr);}
 }
public:
 AltairArchiveRelay(asio::io_context& io,Call core,std::function<void(Json,Reply)> send,std::string token,std::shared_ptr<AltairMessageIngress> ingress,std::shared_ptr<AltairRelayProgress> progress,Reply done,AltairWireKind kind=AltairWireKind::block,size_t cached_scan_budget=32):io_(io),core_(std::move(core)),send_(std::move(send)),token_(std::move(token)),ingress_(std::move(ingress)),progress_(std::move(progress)),done_(std::move(done)),deadline_(io),kind_(kind),cached_scan_budget_(cached_scan_budget){if(!cached_scan_budget_||(kind_!=AltairWireKind::block&&kind_!=AltairWireKind::attestation&&kind_!=AltairWireKind::attester_slashing&&kind_!=AltairWireKind::voluntary_exit)||!core_||!send_||token_.empty()||!ingress_||!progress_||!done_)throw std::invalid_argument("invalid Altair relay");}
 void start(std::chrono::steady_clock::duration timeout=std::chrono::seconds(30)){if(timeout<=std::chrono::steady_clock::duration::zero())throw std::invalid_argument("invalid relay deadline");if(started_)throw std::logic_error("Altair relay already started");started_=true;began_=std::chrono::steady_clock::now();stage_started_=began_;deadline_.expires_after(timeout);deadline_.async_wait([self=shared_from_this()](auto error){if(!error)self->finish(false);});request_inventory();}
};
