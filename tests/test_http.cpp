#include "nano/http.hpp"
#include "support/persistent_rpc.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <thread>
#include <iostream>
#include <unistd.h>
#include <cstdlib>
using namespace nano;
namespace asio=boost::asio;namespace beast=boost::beast;namespace http=beast::http;using tcp=asio::ip::tcp;
struct Running {HttpServer& server;std::thread thread;explicit Running(HttpServer& s):server(s),thread([&s]{s.run();}){}~Running(){server.stop();thread.join();}};
int main(){try{
 auto path=std::filesystem::temp_directory_path()/("nano-http-"+std::to_string(getpid()));
 {Blockchain chain(path,Genesis::from_json(Json::object()));Rpc rpc(chain);HttpServer server(rpc,"127.0.0.1",0);Running running(server);
 auto call=[&](http::verb method,std::string body,bool origin=false,bool websocket=false){asio::io_context context;beast::tcp_stream stream(context);stream.connect(tcp::endpoint(asio::ip::make_address("127.0.0.1"),server.port()));http::request<http::string_body> request(method,"/",11);request.set(http::field::host,"localhost");if(origin)request.set(http::field::origin,"https://example.invalid");if(websocket){request.set(http::field::upgrade,"websocket");request.set(http::field::connection,"upgrade");request.set(http::field::sec_websocket_version,"13");request.set(http::field::sec_websocket_key,"dGhlIHNhbXBsZSBub25jZQ==");}request.body()=std::move(body);request.prepare_payload();http::write(stream,request);beast::flat_buffer buffer;http::response<http::string_body> response;http::read(stream,buffer,response);return response;};
 {PersistentRpc persistent(server.port());
  for(unsigned i=0;i<1005;++i){if(persistent.call("eth_chainId")!="0x539")throw std::runtime_error("persistent RPC identity mismatch");}
  bool rejected=false;try{persistent.call("unknown");}catch(const std::runtime_error& e){rejected=std::string(e.what()).find("not found")!=std::string::npos;}
  if(!rejected||persistent.call("eth_blockNumber")!="0x0")throw std::runtime_error("persistent error response contaminates next request");
 }
 auto response=call(http::verb::post,R"({"jsonrpc":"2.0","id":1,"method":"eth_chainId"})");if(response.result()!=http::status::ok||Json::parse(response.body())["result"]!="0x539")throw std::runtime_error("HTTP RPC identity");
 response=call(http::verb::post,"{");if(Json::parse(response.body())["error"]["code"]!=-32700)throw std::runtime_error("HTTP parse error");
 response=call(http::verb::post,R"({"jsonrpc":"2.0","method":"eth_chainId"})");if(response.result()!=http::status::no_content||!response.body().empty())throw std::runtime_error("HTTP notification");
 response=call(http::verb::post,R"([{"jsonrpc":"2.0","method":"eth_chainId"},{"jsonrpc":"2.0","method":"unknown"}])");if(response.result()!=http::status::no_content||!response.body().empty())throw std::runtime_error("HTTP notification-only batch");
 response=call(http::verb::post,"[]");if(Json::parse(response.body())["error"]["code"]!=-32600)throw std::runtime_error("HTTP empty batch");
 response=call(http::verb::options,"");if(response.result()!=http::status::no_content||response[http::field::access_control_allow_origin]!="*")throw std::runtime_error("HTTP preflight");
 response=call(http::verb::get,"");if(response.result()!=http::status::method_not_allowed)throw std::runtime_error("HTTP method enforcement");
 response=call(http::verb::post,R"([{"jsonrpc":"2.0","id":2,"method":"eth_blockNumber"},{"jsonrpc":"2.0","id":3,"method":"unknown"}])");auto result=Json::parse(response.body());if(result.size()!=2||result[0]["result"]!="0x0"||result[1]["error"]["code"]!=-32601)throw std::runtime_error("HTTP batch");
 auto keyfile=path.string()+"-wallet.json";Wallet(bytes(Int(1),32)).save_keyfile(keyfile,"test-only");setenv("NANO_HTTP_TEST_PASSWORD","test-only",1);
 rpc.load_wallets(Json{{"wallets",Json::array({Json{{"path",keyfile},{"password_env","NANO_HTTP_TEST_PASSWORD"}}})}},"127.0.0.1");
 response=call(http::verb::post,R"({"jsonrpc":"2.0","id":1,"method":"eth_accounts"})");if(Json::parse(response.body())["result"].size()!=1||response.count(http::field::access_control_allow_origin))throw std::runtime_error("local HTTP wallet accounts");
 response=call(http::verb::post,R"({"jsonrpc":"2.0","id":1,"method":"eth_accounts"})",true);if(response.result()!=http::status::forbidden)throw std::runtime_error("browser wallet RPC not rejected");
 response=call(http::verb::get,"",true,true);if(response.result()!=http::status::forbidden)throw std::runtime_error("browser wallet websocket not rejected");
 response=call(http::verb::options,"",true);if(response.result()!=http::status::forbidden)throw std::runtime_error("browser wallet preflight not rejected");
 unsetenv("NANO_HTTP_TEST_PASSWORD");std::filesystem::remove(keyfile);

 }
 std::filesystem::remove_all(path);std::cout<<"HTTP integration checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
