#include "nano/http.hpp"
#include "support/persistent_rpc.hpp"
#include "support/persistent_websocket_rpc.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <thread>
#include <barrier>
#include <array>
#include <iostream>
#include <unistd.h>
#include <cstdlib>
using namespace nano;
namespace asio=boost::asio;namespace beast=boost::beast;namespace http=beast::http;using tcp=asio::ip::tcp;
struct Running {HttpServer& server;std::thread thread;explicit Running(HttpServer& s):server(s),thread([&s]{s.run();}){}~Running(){server.stop();thread.join();}};
int main(){try{
 auto path=std::filesystem::temp_directory_path()/("nano-http-"+std::to_string(getpid()));
 const std::string probe="0x00000000000000000000000000000000000040ad";
 auto genesis=Genesis::from_json(Json{{"alloc",{{probe,{{"code","0x60016000556000355b8015601557600190036008565b50602a60005260206000f3"}}}}}});
 {Blockchain chain(path,genesis);Rpc rpc(chain,{},[]{return AdmissionBudget::Clock::time_point{};});HttpServer server(rpc,"127.0.0.1",0);Running running(server);
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
 // Different TCP connections and JSON-RPC batch envelopes share one node budget.
 for(unsigned count:{50u,50u,28u}){Json entries=Json::array();for(unsigned i=0;i<count;++i)entries.push_back("0x00");auto body=Json{{"jsonrpc","2.0"},{"id",1},{"method","nano_receiveTransactions"},{"params",Json::array({entries})}}.dump();auto reply=Json::parse(call(http::verb::post,body).body());if(reply.at("result").at("processed")!=count)throw std::runtime_error("HTTP transaction budget burst");}
 auto blocked=Json::parse(call(http::verb::post,R"({"jsonrpc":"2.0","id":1,"method":"eth_sendRawTransaction","params":["0x00"]})").body());if(blocked.at("error").at("code")!=-32005)throw std::runtime_error("new connection bypasses transaction budget");
 auto batch_blocked=Json::parse(call(http::verb::post,R"([{"jsonrpc":"2.0","id":1,"method":"eth_sendRawTransaction","params":["0x00"]},{"jsonrpc":"2.0","id":2,"method":"eth_sendRawTransaction","params":["0x00"]}])").body());for(const auto& item:batch_blocked)if(item.at("error").at("code")!=-32005)throw std::runtime_error("JSON-RPC batch bypasses transaction budget");
 if(Json::parse(call(http::verb::post,R"({"jsonrpc":"2.0","id":3,"method":"eth_blockNumber"})").body()).at("result")!="0x0"||chain.pending_count()!=0)throw std::runtime_error("overload changed state or blocked reads");
 // Switching transport after HTTP exhaustion must not create fresh credit.
 {asio::io_context context;beast::websocket::stream<beast::tcp_stream> ws(context);
  beast::get_lowest_layer(ws).connect(tcp::endpoint(asio::ip::make_address("127.0.0.1"),server.port()));ws.handshake("localhost","/");
  std::string request=R"({"jsonrpc":"2.0","id":41,"method":"eth_sendRawTransaction","params":["0x00"]})";ws.write(asio::buffer(request));beast::flat_buffer buffer;ws.read(buffer);auto reply=Json::parse(beast::buffers_to_string(buffer.data()));
  if(reply.at("id")!=41||reply.at("error").at("code")!=-32005)throw std::runtime_error("WebSocket transport bypasses HTTP admission budget");
  buffer.consume(buffer.size());request=R"({"jsonrpc":"2.0","id":42,"method":"eth_chainId"})";ws.write(asio::buffer(request));ws.read(buffer);reply=Json::parse(beast::buffers_to_string(buffer.data()));
  if(reply.at("id")!=42||reply.at("result")!="0x539")throw std::runtime_error("WebSocket reads unavailable during admission overload");
  ws.close(beast::websocket::close_code::normal);
 }
 auto call_ws=[&](const Json& request){asio::io_context context;beast::websocket::stream<beast::tcp_stream> ws(context);beast::get_lowest_layer(ws).connect(tcp::endpoint(asio::ip::make_address("127.0.0.1"),server.port()));ws.handshake("localhost","/");auto body=request.dump();ws.write(asio::buffer(body));beast::flat_buffer buffer;ws.read(buffer);auto result=Json::parse(beast::buffers_to_string(buffer.data()));ws.close(beast::websocket::close_code::normal);return result;};
 // A bounded burst fills the simulation lane; control reads use the event loop.
 {
  Json args={{"to",probe},{"gas","0x1c9c380"},{"data",hex(bytes(Int(600000),32))}};
  Json batch=Json::array();for(unsigned i=0;i<4;++i)batch.push_back({{"jsonrpc","2.0"},{"id",i},{"method","eth_call"},{"params",Json::array({args,"latest"})}});
  std::barrier gate(17);std::array<unsigned,16> status{};std::array<std::string,16> errors{};std::vector<std::jthread> clients;
  for(unsigned i=0;i<16;++i)clients.emplace_back([&,i]{gate.arrive_and_wait();try{Json replies;if(i%2){replies=call_ws(batch);bool overloaded=replies.is_array()&&!replies.empty()&&replies[0].contains("error");status[i]=overloaded?503:200;if(overloaded){if(replies.size()!=4)throw std::runtime_error("WS capacity response count");for(const auto& reply:replies)if(reply.at("error").at("code")!=-32005)throw std::runtime_error("WS capacity error");}}else{auto response=call(http::verb::post,batch.dump());status[i]=response.result_int();if(status[i]==200)replies=Json::parse(response.body());}if(status[i]==200){if(replies.size()!=4)throw std::runtime_error("simulation batch response count");for(const auto& reply:replies)if(reply.at("result")!=hex(bytes(Int(42),32)))throw std::runtime_error("simulation batch result");}else if(status[i]!=503)throw std::runtime_error("unexpected capacity response");}catch(const std::exception& e){errors[i]=e.what();}});
  gate.arrive_and_wait();std::this_thread::sleep_for(std::chrono::milliseconds(100));
  auto begin=std::chrono::steady_clock::now();auto identity=Json::parse(call(http::verb::post,R"({"jsonrpc":"2.0","id":100,"method":"eth_chainId"})").body());auto elapsed=std::chrono::steady_clock::now()-begin;
  for(auto& client:clients)client.join();
  for(const auto& error:errors)if(!error.empty())throw std::runtime_error(error);
  auto accepted=std::count(status.begin(),status.end(),200u),rejected=std::count(status.begin(),status.end(),503u);
  if(!accepted||!rejected||accepted+rejected!=16)throw std::runtime_error("bounded simulation queue did not accept and reject work");
  if(identity.at("result")!="0x539"||elapsed>std::chrono::seconds(2))throw std::runtime_error("simulation queue blocks control RPC");
  if(chain.state().get_storage(probe,0)!=0)throw std::runtime_error("queued simulations changed canonical storage");
  // Overweight batches must be rejected before an earlier mutating item runs.
  Json oversized=Json::array({Json{{"jsonrpc","2.0"},{"id",900},{"method","eth_newBlockFilter"}}});
  for(unsigned i=0;i<9;++i)oversized.push_back({{"jsonrpc","2.0"},{"id",901+i},{"method","eth_call"},{"params",Json::array({args})}});
  auto rejected_http=call(http::verb::post,oversized.dump());if(rejected_http.result_int()!=503)throw std::runtime_error("oversized HTTP simulation batch admitted");
  auto rejected_ws=call_ws(oversized);if(rejected_ws.size()!=10)throw std::runtime_error("oversized WS response count");for(const auto& reply:rejected_ws)if(reply.at("error").at("code")!=-32005)throw std::runtime_error("oversized WS simulation batch admitted");
  auto notified=oversized;for(size_t i=1;i<notified.size();++i)notified[i].erase("id");if(call(http::verb::post,notified.dump()).result_int()!=503)throw std::runtime_error("notification batch bypassed HTTP capacity");auto notified_reply=call_ws(notified);if(notified_reply.size()!=1||notified_reply[0].at("id")!=900||notified_reply[0].at("error").at("code")!=-32005)throw std::runtime_error("notification batch bypassed WS capacity or emitted responses");
  auto first_filter=Json::parse(call(http::verb::post,R"({"jsonrpc":"2.0","id":999,"method":"eth_newBlockFilter"})").body()).at("result");if(first_filter!="0x1")throw std::runtime_error("rejected batch executed its filter mutation");
  auto cleanup=Json{{"jsonrpc","2.0"},{"id",1000},{"method","eth_uninstallFilter"},{"params",Json::array({first_filter})}};if(Json::parse(call(http::verb::post,cleanup.dump()).body()).at("result")!=true)throw std::runtime_error("filter cleanup");
  // Exact capacity is allowed after completion and rejection have released it.
  Json boundary=Json::array();for(unsigned i=0;i<8;++i)boundary.push_back({{"jsonrpc","2.0"},{"id",1100+i},{"method","eth_call"},{"params",Json::array({Json{{"to",probe},{"data",hex(bytes(Int(0),32))}}})}});
  auto boundary_http=Json::parse(call(http::verb::post,boundary.dump()).body());auto boundary_ws=call_ws(boundary);if(boundary_http.size()!=8||boundary_ws!=boundary_http)throw std::runtime_error("exact simulation capacity not reusable");for(const auto& reply:boundary_http)if(reply.at("result")!=hex(bytes(Int(42),32)))throw std::runtime_error("exact capacity call failed");
  Json mixed=Json::array({Json{{"jsonrpc","2.0"},{"method","eth_call"},{"params",Json::array({args})}},Json{{"jsonrpc","2.0"},{"id",7},{"method","eth_chainId"}},Json{{"jsonrpc","2.0"},{"id",8},{"method","eth_call"},{"params",Json::array()}}});
  auto replies=Json::parse(call(http::verb::post,mixed.dump()).body());if(replies.size()!=2||replies[0].at("id")!=7||replies[0].at("result")!="0x539"||replies[1].at("error").at("code")!=-32602)throw std::runtime_error("deferred mixed batch semantics");
  auto ws_replies=call_ws(mixed);if(ws_replies!=replies)throw std::runtime_error("WebSocket mixed batch semantics differ from HTTP");
  Json subscribed=Json::array({Json{{"jsonrpc","2.0"},{"id",20},{"method","eth_subscribe"},{"params",Json::array({"newHeads"})}},Json{{"jsonrpc","2.0"},{"id",21},{"method","eth_call"},{"params",Json::array({args})}},Json{{"jsonrpc","2.0"},{"id",22},{"method","eth_unsubscribe"},{"params",Json::array({"0xdeadbeef"})}}});
  auto subscription_replies=call_ws(subscribed);if(subscription_replies.size()!=3||!subscription_replies[0].at("result").is_string()||subscription_replies[1].at("result")!=hex(bytes(Int(42),32))||subscription_replies[2].at("result")!=false)throw std::runtime_error("WebSocket simulation/subscription batch");
  std::cout<<"Simulation queue accepted="<<accepted<<" rejected="<<rejected<<" controlMs="<<std::chrono::duration<double,std::milli>(elapsed).count()<<"\n";
 }
 auto keyfile=path.string()+"-wallet.json";Wallet(bytes(Int(1),32)).save_keyfile(keyfile,"test-only");setenv("NANO_HTTP_TEST_PASSWORD","test-only",1);
 rpc.load_wallets(Json{{"wallets",Json::array({Json{{"path",keyfile},{"password_env","NANO_HTTP_TEST_PASSWORD"}}})}},"127.0.0.1");
 response=call(http::verb::post,R"({"jsonrpc":"2.0","id":1,"method":"eth_accounts"})");if(Json::parse(response.body())["result"].size()!=1||response.count(http::field::access_control_allow_origin))throw std::runtime_error("local HTTP wallet accounts");
 response=call(http::verb::post,R"({"jsonrpc":"2.0","id":1,"method":"eth_accounts"})",true);if(response.result()!=http::status::forbidden)throw std::runtime_error("browser wallet RPC not rejected");
 response=call(http::verb::get,"",true,true);if(response.result()!=http::status::forbidden)throw std::runtime_error("browser wallet websocket not rejected");
 response=call(http::verb::options,"",true);if(response.result()!=http::status::forbidden)throw std::runtime_error("browser wallet preflight not rejected");
 {
  unsigned ticks=0;RpcSimulationLimits limits;limits.timeout=std::chrono::milliseconds(10);limits.now=[&]{return std::chrono::steady_clock::time_point{}+std::chrono::milliseconds(ticks++);};
  Rpc limited(chain,{},AdmissionBudget::Clock::now,limits);HttpServer deadline_server(limited,"127.0.0.1",0);Running deadline_running(deadline_server);
  PersistentRpc http_client(deadline_server.port());PersistentWebSocketRpc ws_client(deadline_server.port());
  Json costly={{"to",probe},{"gas","0x1c9c380"},{"data",hex(bytes(Int(600000),32))}};
  for(unsigned i=0;i<12;++i){bool cancelled=false;try{if(i%2)ws_client.call(i%4==1?"eth_call":"eth_estimateGas",Json::array({costly}));else http_client.call(i%4==0?"eth_call":"eth_estimateGas",Json::array({costly}));}catch(const PersistentRpcError& error){cancelled=error.code==-32002;}if(!cancelled)throw std::runtime_error("transport simulation did not propagate host deadline");}
  Json cheap={{"to","0x00000000000000000000000000000000000040ff"}};
  if(http_client.call("eth_call",Json::array({cheap}))!="0x"||ws_client.call("eth_call",Json::array({cheap}))!="0x")throw std::runtime_error("cancelled simulations leaked shared queue capacity");
  if(chain.state().get_storage(probe,0)!=0)throw std::runtime_error("transport cancellation persisted simulation writes");
  std::cout<<"HTTP/WebSocket cancellation errors=12 capacity reused\n";
 }
 unsetenv("NANO_HTTP_TEST_PASSWORD");std::filesystem::remove(keyfile);

 }
 std::filesystem::remove_all(path);std::cout<<"HTTP integration checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
