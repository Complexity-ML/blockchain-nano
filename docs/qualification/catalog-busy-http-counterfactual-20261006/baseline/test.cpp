#include "capella_catalog_relay.hpp"
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
 for(const std::string scenario:{"ready","busy-then-ready","unrecognized-error","busy-until-deadline"}){
  asio::io_context io;tcp::acceptor acceptor(io,{asio::ip::make_address("127.0.0.1"),0});unsigned acquires=0,releases=0,pages=0,completed=0;Json firstArgs,result;const std::string gen="0x"+std::string(64,'a'),handle="0x"+std::string(64,'b');
  auto serve=[&](std::shared_ptr<Channel> ch){auto req=Json::parse(ch->request.body());const auto method=req.at("method").get<std::string>();const auto args=req.at("params");need(args[0]=="fixture-token","authentication changed");Json value;
   if(method=="nano_capellaCatalogGeneration")value=gen;
   else if(method=="nano_capellaCatalogAcquire"){
    ++acquires;if(acquires==1)firstArgs=args;else need(args==firstArgs,"retry changed owner/request/generation");
    if((scenario=="busy-then-ready"&&acquires==1)||scenario=="busy-until-deadline"||scenario=="unrecognized-error"){
     ch->reply(Json{{"jsonrpc","2.0"},{"id",1},{"error",{{"code",-32005},{"message",scenario=="unrecognized-error"?"Some other resource unavailable":"Capella inventory capture busy"}}}});return;
    }value=Json{{"handle",handle}};
   }else if(method=="nano_capellaCatalogPage"){++pages;value=Json{{"kind","block"},{"sequence","0x1"},{"digest",gen},{"entries",Json::array()},{"next",nullptr}};}
   else if(method=="nano_capellaCatalogRelease"){++releases;need(args[1]==gen&&args[2]==firstArgs[3],"release changed request identity");value=true;}
   else throw std::runtime_error("unexpected request");
   ch->reply(Json{{"jsonrpc","2.0"},{"id",1},{"result",value}});
  };
  std::function<void()> accept;accept=[&]{acceptor.async_accept([&](auto e,tcp::socket s){if(!e)std::make_shared<Channel>(std::move(s),serve)->read();if(acceptor.is_open())accept();});};accept();
  Core core(io,acceptor.local_endpoint().port());AltairWireProfile profile{"0x539","0x"+std::string(64,'0'),"0x01000001",1024,256,256,256};InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(profile,budget);
  auto relay=std::make_shared<CapellaCatalogRelay>(io,[&](auto m,auto p,auto r){core.call(m,std::move(p),std::move(r));},[](auto,auto,auto){throw std::runtime_error("empty inventory delivered a payload");},"fixture-token","block",ingress,1,1,[&](Json v){++completed;result=std::move(v);io.stop();});
#ifdef NANO_ENABLE_CAPTURE_BUSY_RETRY
  relay->start(std::chrono::milliseconds(120),std::chrono::milliseconds(10));
#else
  relay->start(std::chrono::milliseconds(120));
#endif
  io.run_for(std::chrono::seconds(2));need(completed==1,"relay did not complete once");bool expected=scenario=="ready";
#ifdef NANO_ENABLE_CAPTURE_BUSY_RETRY
  expected=expected||scenario=="busy-then-ready";
#endif
  need(result.at("success")==expected,"capture contention result unexpected");
  if(scenario=="unrecognized-error")need(acquires==1&&pages==0,"ambiguous error replayed");
#ifndef NANO_ENABLE_CAPTURE_BUSY_RETRY
  need(acquires==1,"baseline replayed acquire");
#endif
  std::cout<<Json{{"scenario",scenario},{"success",result.at("success")},{"acquires",acquires},{"releases",releases},{"pages",pages},{"completed",completed},{"realHttp",true},{"productionQualified",false}}.dump()<<'\n';
 }
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
