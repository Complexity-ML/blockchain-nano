#include "core.hpp"
#include <thread>
#include <iostream>
void require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
int main(){try{
 asio::io_context server_io;tcp::acceptor listener(server_io,{asio::ip::make_address("127.0.0.1"),0});
 constexpr unsigned total=100;unsigned requests=0;std::exception_ptr failure;
 std::thread server([&]{try{for(unsigned i=0;i<total;++i){tcp::socket socket(server_io);listener.accept(socket);beast::flat_buffer buffer;http::request<http::string_body> request;http::read(socket,buffer,request);auto body=Json::parse(request.body());require(body["params"][0]=="private-token-do-not-log","authentication changed");++requests;http::response<http::string_body> response(http::status::ok,11);response.keep_alive(false);response.body()=Json{{"jsonrpc","2.0"},{"id",1},{"error",{{"code",-32005},{"message","Capella inventory capture busy"},{"data","private-error-do-not-log"}}}}.dump();response.prepare_payload();http::write(socket,response);}}catch(...){failure=std::current_exception();}});
 asio::io_context io;Json diagnostics=Json::array();auto observer=bounded_core_rpc_trace([&](const Json& row){diagnostics.push_back(row);},4);
 Core core(io,listener.local_endpoint().port(),0,observer);unsigned callbacks=0;bool valid=true;std::function<void()> next;
 next=[&]{const auto kind=callbacks%3==0?"block":callbacks%3==1?"attestation":"private-kind-do-not-log";core.call("nano_capellaCatalogAcquire",Json::array({"private-token-do-not-log",kind,"private-root-do-not-log"}),[&](Json reply){valid&=core_capture_busy(reply);++callbacks;if(callbacks<total)next();});};next();io.run();server.join();if(failure)std::rethrow_exception(failure);
 require(valid&&requests==total&&callbacks==total,"completion or busy hint changed");require(diagnostics.size()==4,"per-connection copy reset diagnostic bound");
 require(diagnostics[0]["inventoryKind"]=="block"&&diagnostics[1]["inventoryKind"]=="attestation"&&diagnostics[2]["inventoryKind"].is_null()&&diagnostics[3]["inventoryKind"]=="block","safe inventory category missing or stale");
 for(const auto& row:diagnostics)require(row["cause"]=="inventory-capture-busy"&&row["phase"]=="rpc-error","busy cause lost");require(diagnostics.dump().find("private-")==std::string::npos,"private data leaked");
 std::cout<<Json{{"realHttp",true},{"requests",requests},{"callbacks",callbacks},{"connections",total},{"diagnostics",diagnostics.size()},{"busyHintsPreserved",true},{"privateDataRedacted",true},{"networkQualified",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
