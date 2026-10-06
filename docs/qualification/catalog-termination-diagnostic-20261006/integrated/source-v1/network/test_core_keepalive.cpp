#include "core.hpp"
#include <thread>
#include <iostream>
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(){try{
 for(const std::string mode:{"reuse","no-reuse","server-close","lost-reply","rpc-error","idle-close"}){
  asio::io_context server_io;tcp::acceptor listener(server_io,{asio::ip::make_address("127.0.0.1"),0});
  const unsigned total=(mode=="reuse"||mode=="no-reuse")?256:2;const unsigned server_total=mode=="idle-close"?1:total;unsigned accepts=0,received=0;std::exception_ptr error;
  std::thread server([&]{try{while(received<server_total){tcp::socket socket(server_io);listener.accept(socket);++accepts;beast::flat_buffer buffer;
   while(received<server_total){http::request<http::string_body> request;beast::error_code ec;http::read(socket,buffer,request,ec);if(ec)break;
    auto j=Json::parse(request.body());check(j["params"][0]==received,"request repeated or reordered");++received;
    if(mode=="lost-reply"&&received==1){socket.close();break;}
    http::response<http::string_body> response(http::status::ok,11);response.keep_alive(mode!="server-close");
    response.body()=(mode=="rpc-error"&&received==1?Json{{"jsonrpc","2.0"},{"id",1},{"error",{{"code",-32603},{"message","fixture"}}}}:Json{{"jsonrpc","2.0"},{"id",1},{"result",received}}).dump();response.prepare_payload();http::write(socket,response);
    if(!response.keep_alive())break;
   }
  }}catch(...){error=std::current_exception();}});
  asio::io_context io;Core core(io,listener.local_endpoint().port(),mode=="no-reuse"?0:4);unsigned callbacks=0;bool valid=true;
  std::function<void()> next=[&]{core.call("fixture",Json::array({callbacks}),[&](Json reply){++callbacks;const bool missing=((mode=="lost-reply"||mode=="rpc-error")&&callbacks==1)||(mode=="idle-close"&&callbacks==2);valid&=missing?reply.is_null():reply==callbacks;if(callbacks<total)next();});};
  next();io.run();server.join();if(error)std::rethrow_exception(error);
  check(valid&&callbacks==total&&received==server_total,"invalid result or callback count");
  check(accepts==unsigned(mode=="no-reuse"?total:mode=="reuse"||mode=="rpc-error"||mode=="idle-close"?1:2),"HTTP connection reuse/reconnect failed");
  std::cout<<mode<<": "<<received<<" requests, "<<accepts<<" connections\n";
 }
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
