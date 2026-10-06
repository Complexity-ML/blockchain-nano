#pragma once
#include "core.hpp"
#include <sstream>
#include <cstdlib>
#include <iostream>
// One sequential transfer; the relay retains a manifest and one fragment, never
// a complete transaction/block body. The core owns bounded authenticated staging.
class BlockDownload:public std::enable_shared_from_this<BlockDownload>{
 asio::io_context& io_;Core& core_;std::string token_,session_,hash_;uint64_t height_;Json manifest_,envelope_;size_t index_=0,fragments_=0;Json offset_="0x0";
 asio::steady_timer pace_,deadline_;bool done_=false,finish_reply_missing_=false;
 std::function<void(Json,Reply)> request_;std::function<void(bool,Json)> finish_;
 static std::string quantity(uint64_t n){std::ostringstream text;text<<"0x"<<std::hex<<n;return text.str();}
 void finish(bool ok){if(done_)return;done_=true;pace_.cancel();deadline_.cancel();if(!session_.empty())core_.call("nano_abortBlockImport",Json::array({token_,session_}),[](Json){});auto cb=std::move(finish_);cb(ok,Json{{"height",height_},{"blockHash",hash_},{"fragments",fragments_},{"finishReplyMissing",finish_reply_missing_},{"proposal",!envelope_.is_null()}});}
 void verify_proposal(bool previous=false){
  core_.call("nano_bftManifest",Json::array({previous}),[self=shared_from_this(),previous](Json local){
   if(self->done_)return;
   if(local.is_object()&&local.contains("proposal")&&local.at("proposal").is_object()&&local.at("proposal").value("hash",Json())==self->hash_){self->finish(true);return;}
   if(!previous)self->verify_proposal(true);else self->finish(false);
  });
 }
 void verify(){
  core_.call("eth_getBlockByNumber",Json::array({quantity(height_),false}),[self=shared_from_this()](Json block){if(self->done_)return;if(block.is_object()&&block.value("hash",Json())==self->hash_)self->finish(true);else if(!self->envelope_.is_null())self->verify_proposal();else self->finish(false);});
 }
 void next(){
  if(done_)return;
  if(index_==manifest_.at("block").at("transactions").size()){
   core_.call(envelope_.is_null()?"nano_finishBlockImport":"nano_finishBftImport",Json::array({token_,session_}),[self=shared_from_this()](Json reply){if(!self->done_){self->finish_reply_missing_=!reply.is_object();self->verify();}});return;
  }
  auto tx=manifest_.at("block").at("transactions").at(index_);
  request_(Json{{"blockHash",hash_},{"transactionHash",tx},{"offset",offset_}},[self=shared_from_this(),tx](Json part){
   if(self->done_)return;
   try{
    if(!part.is_object()||part.value("blockHash",Json())!=self->hash_||part.value("transactionHash",Json())!=tx||part.value("offset",Json())!=self->offset_){self->finish(false);return;}
    auto next=part.at("next");const auto expected_index=self->index_+(next.is_null()?1:0);auto expected_offset=next.is_null()?Json("0x0"):next;
    self->core_.call("nano_appendBlockImport",Json::array({self->token_,self->session_,self->index_,std::move(part)}),[self,expected_index,expected_offset](Json accepted){
     if(self->done_)return;
     if(!accepted.is_object()||accepted.value("transaction",Json())!=expected_index||accepted.value("offset",Json())!=expected_offset){self->finish(false);return;}
     self->index_=expected_index;self->offset_=expected_offset;++self->fragments_;
     if(!self->envelope_.is_null()&&std::getenv("NANO_TEST_NETWORK_TRACE")&&(self->fragments_==1||self->fragments_==20))
      std::cout<<Json{{"event","test-bft-fragment-progress"},{"height",self->height_},{"blockHash",self->hash_},{"fragments",self->fragments_},{"session",self->session_}}.dump()<<std::endl;
     // Existing inbound bulk admission allows four requests/second per peer;
     // leave room for its normal transaction inventory poll as well.
     self->pace_.expires_after(std::chrono::milliseconds(350));self->pace_.async_wait([self](auto ec){if(!ec)self->next();});
    });
   }catch(const std::exception&){self->finish(false);}
  });
 }
public:
 BlockDownload(asio::io_context& io,Core& core,std::string token,uint64_t height,std::function<void(Json,Reply)> request,std::function<void(bool,Json)> finish,Json envelope=nullptr)
 :io_(io),core_(core),token_(std::move(token)),height_(height),envelope_(std::move(envelope)),pace_(io),deadline_(io),request_(std::move(request)),finish_(std::move(finish)){}
 void start(){
  deadline_.expires_after(std::chrono::seconds(55));deadline_.async_wait([self=shared_from_this()](auto ec){if(!ec)self->finish(false);});
  auto accept=[self=shared_from_this()](Json manifest){
   if(self->done_)return;
   try{
    if(!manifest.is_object()||manifest.value("version",Json())!=1||!manifest.contains("block")||manifest.dump().size()>8*1024*1024){self->finish(false);return;}
    const auto& block=manifest.at("block");if(block.at("number")!=quantity(self->height_)||!block.at("transactions").is_array()){self->finish(false);return;}
    self->hash_=block.at("hash").get<std::string>();self->manifest_=std::move(manifest);
    self->core_.call(self->envelope_.is_null()?"nano_beginBlockImport":"nano_beginBftImport",Json::array({self->token_,self->envelope_.is_null()?self->manifest_:self->envelope_}),[self](Json session){
     if(self->done_){if(session.is_object()&&session.contains("id"))self->core_.call("nano_abortBlockImport",Json::array({self->token_,session.at("id")}),[](Json){});return;}
     if(!session.is_object()||!session.contains("id")||!session.at("id").is_string()){self->finish(false);return;}
     self->session_=session.at("id").get<std::string>();self->next();
    });
   }catch(const std::exception&){self->finish(false);}
  };
  if(envelope_.is_null())request_(Json{{"height",height_}},std::move(accept));else accept(Json{{"version",1},{"block",envelope_.at("proposal")}});
 }
};
