#include "capella_head_catalog_relay.hpp"
#include <iostream>
struct Channel:std::enable_shared_from_this<Channel>{
 tcp::socket socket;beast::flat_buffer buffer;http::request<http::string_body> request;http::response<http::string_body> response;
 std::function<void(std::shared_ptr<Channel>)> handle;
 Channel(tcp::socket s,decltype(handle) h):socket(std::move(s)),handle(std::move(h)){}
 void read(){request={};http::async_read(socket,buffer,request,[self=shared_from_this()](auto e,size_t){if(!e)self->handle(self);});}
 void reply(Json body){response={http::status::ok,11};response.keep_alive(true);response.set(http::field::content_type,"application/json");response.body()=body.dump();response.prepare_payload();http::async_write(socket,response,[self=shared_from_this()](auto e,size_t){if(!e)self->read();});}
};
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
int main(){try{
 Json refused={{"jsonrpc","2.0"},{"id",1},{"error",{{"code",-32005},{"message","Capella inventory capture busy"}}}};
 need(core_capture_busy(core_capture_busy_hint("nano_capellaCatalogAcquire",refused)),"busy refusal hint lost");
 need(core_capture_busy_hint("nano_capellaSubmit",refused).is_null(),"hint escaped acquire method");
 auto invalid=refused;invalid["id"]=2;need(core_capture_busy_hint("nano_capellaCatalogAcquire",invalid).is_null(),"wrong response identity allowed retry");
 invalid=refused;invalid["error"]["code"]=-32001;need(core_capture_busy_hint("nano_capellaCatalogAcquire",invalid).is_null(),"authentication rejection allowed retry");
 invalid=refused;invalid["result"]=nullptr;need(core_capture_busy_hint("nano_capellaCatalogAcquire",invalid).is_null(),"mixed error/result allowed retry");

 for(const std::string scenario:{"ready","busy-then-ready","unrecognized-error","busy-until-deadline","lost-reply","late-busy","late-handle","head-first-busy","history-first-busy"}){
  const bool wrapped=scenario=="head-first-busy"||scenario=="history-first-busy";
  asio::io_context io;tcp::acceptor acceptor(io,{asio::ip::make_address("127.0.0.1"),0});unsigned acquires=0,releases=0,pages=0,completed=0,closes=0;asio::steady_timer settle(io);std::vector<std::shared_ptr<Channel>> lost;Json firstArgs,result;const std::string gen="0x"+std::string(64,'a'),handle="0x"+std::string(64,'b');
  auto serve=[&](std::shared_ptr<Channel> ch){auto req=Json::parse(ch->request.body());const auto method=req.at("method").get<std::string>();const auto args=req.at("params");need(args[0]=="fixture-token","authentication changed");Json value;
   if(method=="nano_capellaStatus"){need(wrapped,"unexpected head status");value={{"headRoot",gen},{"headSlot",uint64_t(0)}};}
   else if(method=="nano_capellaCatalogGeneration")value=gen;
   else if(method=="nano_capellaCatalogAcquire"){
    ++acquires;if(acquires==1)firstArgs=args;else need(args==firstArgs,"retry changed owner/request/generation");
    if(scenario=="lost-reply"){lost.push_back(ch);return;}
    if(scenario=="late-busy"||scenario=="late-handle"){
     auto timer=std::make_shared<asio::steady_timer>(io);timer->expires_after(std::chrono::milliseconds(160));timer->async_wait([timer,ch,handle,scenario](auto e){if(e)return;ch->reply(scenario=="late-handle"?Json{{"jsonrpc","2.0"},{"id",1},{"result",{{"handle",handle}}}}:Json{{"jsonrpc","2.0"},{"id",1},{"error",{{"code",-32005},{"message","Capella inventory capture busy"}}}});});return;
    }
    if(((scenario=="busy-then-ready"||wrapped)&&acquires==1)||scenario=="busy-until-deadline"||scenario=="unrecognized-error"){
     ch->reply(Json{{"jsonrpc","2.0"},{"id",1},{"error",{{"code",-32005},{"message",scenario=="unrecognized-error"?"Some other resource unavailable":"Capella inventory capture busy"}}}});return;
    }value=Json{{"handle",handle}};
   }else if(method=="nano_capellaCatalogPage"){++pages;value=Json{{"kind","block"},{"sequence","0x1"},{"digest",gen},{"entries",Json::array()},{"next",nullptr}};}
   else if(method=="nano_capellaCatalogRelease"){++releases;need(args[1]==gen&&args[2]==firstArgs[3],"release changed request identity");value=true;}
   else if(method=="nano_capellaCatalogClose"){++closes;need(args[1]==handle,"late handle cleanup identity changed");value=true;}
   else throw std::runtime_error("unexpected request");
   ch->reply(Json{{"jsonrpc","2.0"},{"id",1},{"result",value}});
  };
  std::function<void()> accept;accept=[&]{acceptor.async_accept([&](auto e,tcp::socket s){if(!e)std::make_shared<Channel>(std::move(s),serve)->read();if(acceptor.is_open())accept();});};accept();
  Core core(io,acceptor.local_endpoint().port());AltairWireProfile profile{"0x539","0x"+std::string(64,'0'),"0x01000001",1024,256,256,256};InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(profile,budget);
  auto call=[&](auto m,auto p,auto r){core.call(m,std::move(p),std::move(r));};
  auto deliver=[](auto,auto,auto){throw std::runtime_error("empty inventory delivered a payload");};
  auto done=[&](Json v){++completed;result=std::move(v);settle.expires_after(std::chrono::milliseconds(100));settle.async_wait([&](auto e){if(!e)io.stop();});};
  std::shared_ptr<CapellaCatalogRelay> relay;std::shared_ptr<CapellaHeadCatalogRelay> head;
  if(wrapped){auto knowledge=std::make_shared<AltairRelayProgress>(1);head=std::make_shared<CapellaHeadCatalogRelay>(io,call,deliver,[](auto,auto){throw std::runtime_error("anchor sent payload");},"fixture-token",gen,ingress,knowledge,1,1024,1,1,done,[](auto){return false;},nullptr,false,nullptr,scenario=="history-first-busy");head->start(std::chrono::milliseconds(120),std::chrono::milliseconds(10));}
  else{relay=std::make_shared<CapellaCatalogRelay>(io,call,deliver,"fixture-token","block",ingress,1,1,done);relay->start(std::chrono::milliseconds(120),std::chrono::milliseconds(10));}
  io.run_for(std::chrono::seconds(2));need(completed==1,"relay did not complete once");bool expected=scenario=="ready";
  expected=expected||scenario=="busy-then-ready"||wrapped;

  need(result.at("success")==expected,"capture contention result unexpected");
  if(scenario=="lost-reply"||scenario=="late-busy"||scenario=="late-handle")need(acquires==1,"ambiguous/late response replayed");
  if(scenario=="late-handle")need(closes==1,"late handle not retired");
  need(releases==1,"request reservation was not retired exactly once");
  if(scenario=="unrecognized-error")need(acquires==1&&pages==0,"ambiguous error replayed");

  std::cout<<Json{{"scenario",scenario},{"success",result.at("success")},{"acquires",acquires},{"releases",releases},{"pages",pages},{"completed",completed},{"lateHandleCloses",closes},{"realHttp",true},{"productionQualified",false}}.dump()<<'\n';
 }
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
