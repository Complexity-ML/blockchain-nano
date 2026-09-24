#include "nano/http.hpp"
#include "support/process.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <atomic>
using namespace test;
namespace asio=boost::asio;namespace beast=boost::beast;namespace http=beast::http;
using tcp=asio::ip::tcp;
int main(){try{
 for(bool websocket:{false,true}){
  Temp root;const std::string probe="0x00000000000000000000000000000000000040ad";
  auto genesis=Genesis::from_json({{"config",{{"evmFork","Shanghai"}}},{"alloc",{{probe,{{"code","0x60016000555b600556"}}}}}});
  Blockchain chain(root/"db",genesis);auto original=chain.state().root();
  std::atomic<unsigned> polls=0;std::atomic<bool> started=false,release=false;
  RpcSimulationLimits limits;limits.timeout=std::chrono::seconds(60);
  limits.now=[&]{if(++polls==20){started=true;while(!release.load())std::this_thread::sleep_for(1ms);}return std::chrono::steady_clock::now();};
  Rpc rpc(chain,{},AdmissionBudget::Clock::now,limits);
  auto server=std::make_unique<HttpServer>(rpc,"127.0.0.1",0);auto port=server->port();std::jthread serving([&]{server->run();});
  Json batch=Json::array();for(unsigned i=0;i<8;++i)batch.push_back({{"jsonrpc","2.0"},{"id",i},{"method","eth_call"},{"params",Json::array({Json{{"to",probe},{"gas","0x1c9c380"}}})}});
  batch.push_back({{"jsonrpc","2.0"},{"id",99},{"method","eth_newBlockFilter"}});
  std::string outcome;std::jthread client([&]{try{
   asio::io_context io;auto body=batch.dump();beast::flat_buffer buffer;
   if(websocket){beast::websocket::stream<beast::tcp_stream> socket(io);beast::get_lowest_layer(socket).connect({asio::ip::make_address("127.0.0.1"),port});socket.handshake("localhost","/");socket.write(asio::buffer(body));socket.read(buffer);}
   else{beast::tcp_stream socket(io);socket.connect({asio::ip::make_address("127.0.0.1"),port});http::request<http::string_body> request(http::verb::post,"/",11);request.set(http::field::host,"localhost");request.body()=body;request.prepare_payload();http::write(socket,request);http::response<http::string_body> response;http::read(socket,buffer,response);}
   outcome="response";
  }catch(const std::exception&){outcome="connection-closed";}});
  auto wait_until=std::chrono::steady_clock::now()+10s;while(!started.load()&&std::chrono::steady_clock::now()<wait_until)std::this_thread::sleep_for(1ms);
  bool reached=started.load();auto began=std::chrono::steady_clock::now();server->stop();release=true;serving.join();server.reset();auto elapsed=std::chrono::steady_clock::now()-began;client.join();
  check(reached,"simulation did not reach controlled in-flight checkpoint");
  check(polls.load()==20,"shutdown continued simulation work or started queued simulations");
  check(elapsed<2s,"cooperative shutdown exceeded local two-second test bound");
  check(chain.state().root()==original,"shutdown persisted simulation writes");
  auto filter=rpc.handle({{"jsonrpc","2.0"},{"id",100},{"method","eth_newBlockFilter"}});
  check(filter.at("result")=="0x1","shutdown executed a later mutating batch item");
  std::cout<<Json{{"transport",websocket?"websocket":"http"},{"queuedSimulations",8},{"checkpointPolls",polls.load()},{"shutdownMs",std::chrono::duration<double,std::milli>(elapsed).count()},{"clientOutcome",outcome},{"stateUnchanged",true},{"laterMutationSkipped",true}}.dump()<<std::endl;
 }
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
