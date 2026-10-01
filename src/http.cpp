#include "nano/rpc_timing.hpp"
#include "nano/http.hpp"
#include "nano/websocket.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <csignal>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <set>
#include <sstream>
namespace nano {
namespace asio=boost::asio;
namespace beast=boost::beast;
namespace http=beast::http;
using tcp=asio::ip::tcp;
namespace {
// Prefer the client's highest-quality supported representation. Explicit media
// types override wildcard ranges, including q=0 exclusions.
int beacon_format(std::string_view accept,bool binary,std::string_view json_type="application/json",std::string_view json_wildcard="application/*"){
 if(accept.empty())return 2;
 std::string normalized(accept);for(auto& c:normalized)if(c>='A'&&c<='Z')c=char(c-'A'+'a');accept=normalized;
 struct Choice {int specificity=-1,quality=0;};Choice json,ssz;
 auto trim=[](std::string_view x){while(!x.empty()&&(x.front()==' '||x.front()=='\t'))x.remove_prefix(1);while(!x.empty()&&(x.back()==' '||x.back()=='\t'))x.remove_suffix(1);return x;};
 while(!accept.empty()){
  const auto comma=accept.find(',');auto item=trim(accept.substr(0,comma));accept=comma==std::string_view::npos?std::string_view{}:accept.substr(comma+1);
  const auto semi=item.find(';');auto media=trim(item.substr(0,semi));int quality=1000;
  if(semi!=std::string_view::npos){auto parameter=trim(item.substr(semi+1));if(!parameter.starts_with("q="))continue;auto q=trim(parameter.substr(2));
   if(q.empty()||(q[0]!='0'&&q[0]!='1')||(q.size()>1&&(q[1]!='.'||q.size()>5))){quality=0;}
   else {quality=q[0]=='1'?1000:0;int factor=100;for(size_t i=2;i<q.size();++i){if(q[i]<'0'||q[i]>'9'||(q[0]=='1'&&q[i]!='0')){quality=0;break;}if(q[0]=='0')quality+=(q[i]-'0')*factor;factor/=10;}}
  }
  auto apply=[&](Choice& c,int specificity){if(specificity>c.specificity||(specificity==c.specificity&&quality>c.quality))c={specificity,quality};};
  if(media==json_type)apply(json,2);else if(media=="application/octet-stream")apply(ssz,2);
  else if(media==json_wildcard){apply(json,1);if(binary)apply(ssz,1);}else if(media=="*/*"){apply(json,0);apply(ssz,0);}
 }
 if(binary&&ssz.quality>0&&ssz.quality>=json.quality)return 1;
 return json.quality>0?2:0;
}
#include "beacon_events_http.hpp"
class Session:public std::enable_shared_from_this<Session>{
 beast::tcp_stream stream_;
 beast::flat_buffer buffer_;
 std::optional<http::request_parser<http::string_body>> parser_;
 unsigned requests_=0;
 http::response<http::string_body> response_;
 Rpc& rpc_;
 BeaconReadHandler beacon_;
 BeaconEventsHandler events_;
 SimulationQueue& simulations_;
 std::shared_ptr<RpcConnections> connections_;
 std::shared_ptr<bool> drop_import_ack_;
 struct DisconnectWatch {std::atomic<bool> cancelled{false};bool pending=true;char byte=0;};
 void watch_disconnect(const std::shared_ptr<DisconnectWatch>& watch){
  // Peek without consuming a pipelined request. EOF may be a valid send-side
  // half-close: only a definite socket error cancels the computation.
  stream_.socket().async_receive(asio::buffer(&watch->byte,1),tcp::socket::message_peek,[self=shared_from_this(),watch](beast::error_code ec,size_t){
   watch->pending=false;
   if(ec&&ec!=asio::error::operation_aborted&&ec!=asio::error::eof){watch->cancelled.store(true);beast::error_code ignored;self->stream_.socket().close(ignored);}
  });
 }
public:
 Session(tcp::socket socket,Rpc& rpc,std::shared_ptr<RpcConnections> connections,SimulationQueue& simulations,std::shared_ptr<bool> drop_import_ack,BeaconReadHandler beacon,BeaconEventsHandler events):stream_(std::move(socket)),rpc_(rpc),beacon_(std::move(beacon)),events_(std::move(events)),simulations_(simulations),connections_(std::move(connections)),drop_import_ack_(std::move(drop_import_ack)){++connections_->active;}
 ~Session(){--connections_->active;}
 void start(){response_={};parser_.emplace();parser_->body_limit(16*1024*1024);parser_->header_limit(16384);stream_.expires_after(std::chrono::seconds(15));http::async_read(stream_,buffer_,*parser_,[self=shared_from_this()](beast::error_code ec,size_t){self->respond(ec);});}
 void respond(beast::error_code ec){
  bool keep_alive=false;
  if(ec){if(ec==http::error::body_limit){response_.result(http::status::payload_too_large);response_.body()="Request body too large";}else return;}
  else{
   auto request=parser_->release();keep_alive=request.keep_alive()&&++requests_<1000;
   if((rpc_.local_only()||beacon_||events_)&&request.count(http::field::origin)){response_.version(request.version());response_.result(http::status::forbidden);response_.body()="Browser origins are disabled for local privileged RPC";}
   else {if(beast::websocket::is_upgrade(request)){start_websocket(std::move(stream_),std::move(request),rpc_,connections_,simulations_);return;}response_.version(request.version());
   if(events_&&(request.target()=="/eth/v1/events"||request.target().starts_with("/eth/v1/events?"))){
    unsigned status=400;std::string message="Invalid Beacon event subscription";
    if(request.method()!=http::verb::get){status=405;message="Beacon interface is read-only";}
    else if(!beacon_format(std::string(request[http::field::accept]),false,"text/event-stream","text/*")){status=406;message="Event stream representation unavailable";}
    else if(connections_->beacon_streams>=4){status=503;message="Beacon stream capacity exceeded";}
    else try{
     if(request.count("Last-Event-ID"))throw std::invalid_argument("Event history is not durable; resynchronize via RPC");
     auto topics=beacon_topics(std::string(request.target()));
     std::make_shared<BeaconEventSession>(std::move(stream_),simulations_,connections_,events_,std::move(topics))->start();return;
    }catch(const std::invalid_argument& error){message=error.what();}
    response_.result(static_cast<http::status>(status));response_.body()=Json{{"code",status},{"message",message}}.dump();
   }
   else if(beacon_&&request.target().starts_with("/eth/")){
    if(request.method()!=http::verb::get){response_.result(http::status::method_not_allowed);response_.body()=Json{{"code",405},{"message","Beacon interface is read-only"}}.dump();}
    else if(!simulations_.reserve(1)){response_.result(http::status::service_unavailable);response_.body()=Json{{"code",503},{"message","Beacon read capacity exceeded"}}.dump();}
    else {
     std::string path(request.target()),accept(request[http::field::accept]);
     simulations_.dispatch(stream_.get_executor(),[self=shared_from_this(),path=std::move(path),accept=std::move(accept)]{
      try{
       const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
       auto result=self->beacon_(path,[&]{if(self->simulations_.stopping.load()||std::chrono::steady_clock::now()>=deadline)throw ExecutionCancelled("Beacon read stopped");});
       Json out={{"status",result.status},{"body",std::move(result.body)}};
       if(result.status==200){const auto format=beacon_format(accept,!result.ssz.empty());
        if(!format)return Json{{"status",406},{"body",{{"code",406},{"message","Requested Beacon representation unavailable"}}}};
        if(format==1)out["binary"]=Json::binary(std::move(result.ssz));
        if(!result.version.empty())out["version"]=result.version;
       }
       return out;
      }catch(const ExecutionCancelled&){return Json{{"status",503},{"body",{{"code",503},{"message","Beacon read cancelled or execution budget expired"}}}};}
       catch(const std::exception&){return Json{{"status",503},{"body",{{"code",503},{"message","Beacon state unavailable"}}}};}
     },[self=shared_from_this(),keep_alive](std::optional<Json> result){
      self->response_.result(static_cast<http::status>(result?result->at("status").get<unsigned>():503));
      if(result&&result->contains("binary")){const auto& bytes=result->at("binary").get_binary();self->response_.body().assign(bytes.begin(),bytes.end());self->response_.set(http::field::content_type,"application/octet-stream");}
      else self->response_.body()=result?result->at("body").dump():Json{{"code",503},{"message","Beacon read queue expired"}}.dump();
      if(result&&result->contains("version")){self->response_.set("Eth-Consensus-Version",result->at("version").get<std::string>());self->response_.set("Eth-Execution-Optimistic","false");self->response_.set("Eth-Finalized",result->at("body").at("finalized").get<bool>()?"true":"false");}
      self->write(keep_alive);
     },[self=shared_from_this()]{self->simulations_.release(1);});return;
    }
   }
   else if(request.method()==http::verb::options)response_.result(http::status::no_content);
   else if(request.method()!=http::verb::post){response_.result(http::status::method_not_allowed);response_.body()="Use POST for JSON-RPC";}
   else{
    auto parsed=Json::parse(request.body(),nullptr,false);
    auto cost=parsed.is_discarded()?0:simulation_count(parsed);
    if(cost){
     if(!simulations_.reserve(cost)){response_.result(http::status::service_unavailable);response_.body()="RPC simulation capacity exceeded";}
     else{
      auto executor=stream_.get_executor();auto watch=std::make_shared<DisconnectWatch>();watch_disconnect(watch);
      auto timing_id=rpc_timing::allocate();auto queued=rpc_timing::Clock::now();
      simulations_.dispatch(executor,[self=shared_from_this(),parsed=std::move(parsed),watch,timing_id,queued]{
       rpc_timing::Context context(timing_id);rpc_timing::record("http_queue",queued,timing_id);rpc_timing::Span worker("worker");
       return self->rpc_.handle(parsed,[self,watch]{if(self->simulations_.stopping.load())throw ExecutionCancelled("RPC server stopping");if(watch->cancelled.load())throw ExecutionCancelled("RPC client disconnected");});
      },[self=shared_from_this(),keep_alive,watch](std::optional<Json> result){
       if(watch->pending){beast::error_code ignored;self->stream_.socket().cancel(ignored);}if(watch->cancelled.load())return;
       if(!result){self->response_.result(http::status::service_unavailable);self->response_.body()="RPC simulation queue wait exceeded";}
       else{self->response_.result(result->is_null()?http::status::no_content:http::status::ok);if(!result->is_null())self->response_.body()=result->dump();}
       self->write(keep_alive);
      },[self=shared_from_this(),cost]{self->simulations_.release(cost);});return;
     }
    }else{auto result=rpc_.parse(request.body());
     // Explicit loopback-only fault fixture: commit succeeds, but the actual
     // HTTP connection is reset before any response bytes are written.
     if(*drop_import_ack_&&parsed.is_object()&&parsed.value("method",Json())=="nano_finishBlockImport"&&result.is_object()&&result.contains("result")&&result.at("result").is_object()&&result.at("result").value("success",false)){
      *drop_import_ack_=false;std::cout<<Json{{"event","test-block-import-ack-dropped"},{"blockHash",result.at("result").at("blockHash")}}.dump()<<std::endl;
      beast::error_code ignored;stream_.socket().set_option(asio::socket_base::linger(true,0),ignored);stream_.socket().close(ignored);return;
     }
     response_.result(result.is_null()?http::status::no_content:http::status::ok);if(!result.is_null())response_.body()=result.dump();}
   }
  }
  }
  write(keep_alive);
 }
 void write(bool keep_alive){
  response_.set(http::field::server,"Nano/0.1.0");if(!response_.count(http::field::content_type))response_.set(http::field::content_type,"application/json");if(!rpc_.local_only()&&!beacon_&&!events_)response_.set(http::field::access_control_allow_origin,"*");response_.set(http::field::access_control_allow_methods,"POST, OPTIONS");response_.set(http::field::access_control_allow_headers,"Content-Type");response_.keep_alive(keep_alive);response_.prepare_payload();
  stream_.expires_after(std::chrono::seconds(15));http::async_write(stream_,response_,[self=shared_from_this()](beast::error_code ec,size_t){if(!ec&&self->response_.keep_alive()){self->start();return;}beast::error_code ignored;self->stream_.socket().shutdown(tcp::socket::shutdown_both,ignored);});
 }
};
}
struct HttpServer::Impl {
 asio::io_context context;
 tcp::acceptor acceptor;
 asio::signal_set signals;
 asio::steady_timer validator_timer;
 Rpc& rpc;
 BeaconReadHandler beacon;
 BeaconEventsHandler events;
 std::shared_ptr<RpcConnections> connections=std::make_shared<RpcConnections>();
 SimulationQueue simulations;
 std::shared_ptr<bool> drop_import_ack=std::make_shared<bool>(false);
 Impl(Rpc& rpc,const std::string& host,uint16_t port,std::chrono::milliseconds maximum_queue_wait,BeaconReadHandler beacon,BeaconEventsHandler events):acceptor(context),signals(context,SIGINT,SIGTERM),validator_timer(context),rpc(rpc),beacon(std::move(beacon)),events(std::move(events)),simulations(maximum_queue_wait){
  *drop_import_ack=host=="127.0.0.1"&&std::getenv("NANO_TEST_DROP_BLOCK_IMPORT_ACK_ONCE")!=nullptr;
  tcp::endpoint endpoint(asio::ip::make_address(host),port);if((rpc.requires_private_bind()||beacon||events)&&!endpoint.address().is_loopback())throw std::invalid_argument("Altair internal RPC requires a loopback bind");acceptor.open(endpoint.protocol());acceptor.set_option(asio::socket_base::reuse_address(true));acceptor.bind(endpoint);acceptor.listen(128);
  signals.async_wait([this](beast::error_code ec,int){if(!ec){simulations.stopping.store(true);context.stop();}});accept();tick();
 }
 ~Impl(){simulations.stopping.store(true);simulations.workers.join();}
 void tick(){validator_timer.expires_after(std::chrono::milliseconds(100));validator_timer.async_wait([this](beast::error_code ec){if(!ec){rpc.validator_tick();tick();}});}
 void accept(){acceptor.async_accept([this](beast::error_code ec,tcp::socket socket){if(!ec&&connections->active<128)std::make_shared<Session>(std::move(socket),rpc,connections,simulations,drop_import_ack,beacon,events)->start();if(acceptor.is_open())accept();});}
};
HttpServer::HttpServer(Rpc& rpc,const std::string& host,uint16_t port,std::chrono::milliseconds maximum_queue_wait,BeaconReadHandler beacon,BeaconEventsHandler events):impl_(std::make_unique<Impl>(rpc,host,port,maximum_queue_wait,std::move(beacon),std::move(events))){}
HttpServer::~HttpServer()=default;
uint16_t HttpServer::port()const{return impl_->acceptor.local_endpoint().port();}
void HttpServer::run(){impl_->context.run();}
void HttpServer::stop(){impl_->simulations.stopping.store(true);asio::post(impl_->context,[this]{impl_->context.stop();});}
}
