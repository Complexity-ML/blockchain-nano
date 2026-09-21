#include "nano/http.hpp"
#include "nano/websocket.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <csignal>
namespace nano {
namespace asio=boost::asio;
namespace beast=boost::beast;
namespace http=beast::http;
using tcp=asio::ip::tcp;
namespace {
class Session:public std::enable_shared_from_this<Session>{
 beast::tcp_stream stream_;
 beast::flat_buffer buffer_;
 std::optional<http::request_parser<http::string_body>> parser_;
 unsigned requests_=0;
 http::response<http::string_body> response_;
 Rpc& rpc_;
 std::shared_ptr<size_t> connections_;
public:
 Session(tcp::socket socket,Rpc& rpc,std::shared_ptr<size_t> connections):stream_(std::move(socket)),rpc_(rpc),connections_(std::move(connections)){++*connections_;}
 ~Session(){--*connections_;}
 void start(){response_={};parser_.emplace();parser_->body_limit(16*1024*1024);parser_->header_limit(16384);stream_.expires_after(std::chrono::seconds(15));http::async_read(stream_,buffer_,*parser_,[self=shared_from_this()](beast::error_code ec,size_t){self->respond(ec);});}
 void respond(beast::error_code ec){
  bool keep_alive=false;
  if(ec){if(ec==http::error::body_limit){response_.result(http::status::payload_too_large);response_.body()="Request body too large";}else return;}
  else{
   auto request=parser_->release();keep_alive=request.keep_alive()&&++requests_<1000;
   if(rpc_.local_only()&&request.count(http::field::origin)){response_.version(request.version());response_.result(http::status::forbidden);response_.body()="Browser origins are disabled for local privileged RPC";}
   else {if(beast::websocket::is_upgrade(request)){start_websocket(std::move(stream_),std::move(request),rpc_,connections_);return;}response_.version(request.version());
   if(request.method()==http::verb::options)response_.result(http::status::no_content);
   else if(request.method()!=http::verb::post){response_.result(http::status::method_not_allowed);response_.body()="Use POST for JSON-RPC";}
   else{auto result=rpc_.parse(request.body());response_.result(result.is_null()?http::status::no_content:http::status::ok);if(!result.is_null())response_.body()=result.dump();}
  }
  }
  response_.set(http::field::server,"Nano/0.1.0");response_.set(http::field::content_type,"application/json");if(!rpc_.local_only())response_.set(http::field::access_control_allow_origin,"*");response_.set(http::field::access_control_allow_methods,"POST, OPTIONS");response_.set(http::field::access_control_allow_headers,"Content-Type");response_.keep_alive(keep_alive);response_.prepare_payload();
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
 std::shared_ptr<size_t> connections=std::make_shared<size_t>(0);
 Impl(Rpc& rpc,const std::string& host,uint16_t port):acceptor(context),signals(context,SIGINT,SIGTERM),validator_timer(context),rpc(rpc){
  tcp::endpoint endpoint(asio::ip::make_address(host),port);acceptor.open(endpoint.protocol());acceptor.set_option(asio::socket_base::reuse_address(true));acceptor.bind(endpoint);acceptor.listen(128);
  signals.async_wait([this](beast::error_code ec,int){if(!ec)context.stop();});accept();tick();
 }
 void tick(){validator_timer.expires_after(std::chrono::milliseconds(100));validator_timer.async_wait([this](beast::error_code ec){if(!ec){rpc.validator_tick();tick();}});}
 void accept(){acceptor.async_accept([this](beast::error_code ec,tcp::socket socket){if(!ec&&*connections<128)std::make_shared<Session>(std::move(socket),rpc,connections)->start();if(acceptor.is_open())accept();});}
};
HttpServer::HttpServer(Rpc& rpc,const std::string& host,uint16_t port):impl_(std::make_unique<Impl>(rpc,host,port)){}
HttpServer::~HttpServer()=default;
uint16_t HttpServer::port()const{return impl_->acceptor.local_endpoint().port();}
void HttpServer::run(){impl_->context.run();}
void HttpServer::stop(){asio::post(impl_->context,[this]{impl_->context.stop();});}
}
