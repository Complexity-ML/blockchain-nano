#include "nano/http.hpp"
#include "support/process.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <atomic>
using namespace test;
namespace asio=boost::asio;namespace beast=boost::beast;namespace http=beast::http;
using tcp=asio::ip::tcp;
struct Running {HttpServer& server;std::thread thread;explicit Running(HttpServer& s):server(s),thread([&s]{s.run();}){}~Running(){server.stop();thread.join();}};
int main(int argc,char** argv){try{
 const bool wallet_mode=argc==2&&std::string(argv[1])=="--wallet";
 for(const auto& fork:wallet_mode?std::vector<std::string>{"Paris","Shanghai"}:std::vector<std::string>{"Shanghai"})
 for(const auto& method:wallet_mode?std::vector<std::string>{"eth_signTransaction","eth_sendTransaction"}:std::vector<std::string>{"eth_call"})
 for(bool reset:{false,true}){
  Temp root;const std::string probe="0x00000000000000000000000000000000000040ad";
  auto key=bytes(Int(10),32);auto sender=public_address(public_key(key));
  auto genesis=Genesis::from_json({{"config",{{"evmFork",fork}}},{"alloc",{{sender,{{"balance","0x1000000000000000000"}}},{probe,{{"code","0x60016000555b600556"}}}}}});
  Blockchain chain(root/"db",genesis);auto original=chain.state().root();std::atomic<unsigned> polls=0;std::atomic<bool> started=false,release=false,closed=false,identity=false;
  RpcSimulationLimits limits;limits.timeout=60s;limits.now=[&]{if(++polls==20){started=true;while(!release.load())std::this_thread::sleep_for(1ms);}return std::chrono::steady_clock::now();};
  Rpc rpc_api(chain,{},AdmissionBudget::Clock::now,limits);
  if(wallet_mode){Wallet(key).save_keyfile(root/"wallet.json","test-only");setenv("NANO_TRANSPORT_WALLET_PASSWORD","test-only",1);rpc_api.load_wallets({{"wallets",Json::array({Json{{"path",(root/"wallet.json").string()},{"password_env","NANO_TRANSPORT_WALLET_PASSWORD"}}})}},"127.0.0.1");unsetenv("NANO_TRANSPORT_WALLET_PASSWORD");}
  HttpServer server(rpc_api,"127.0.0.1",0);Running running(server);
  Json batch=Json::array();for(unsigned i=0;i<8;++i)batch.push_back({{"jsonrpc","2.0"},{"id",i},{"method",method},{"params",Json::array({Json{{"to",probe},{"gas","0x1c9c380"}}})}});
  if(wallet_mode)for(auto& item:batch){item["params"][0]["from"]=sender;item["params"][0].erase("gas");}
  batch.push_back({{"jsonrpc","2.0"},{"id",99},{"method","eth_newBlockFilter"}});
  std::string client_error;std::jthread client([&]{try{
   asio::io_context io;beast::websocket::stream<beast::tcp_stream> socket(io);beast::get_lowest_layer(socket).connect({asio::ip::make_address("127.0.0.1"),server.port()});socket.handshake("localhost","/");auto body=batch.dump();socket.write(asio::buffer(body));
   auto until=std::chrono::steady_clock::now()+10s;while(!started.load()&&!release.load()&&std::chrono::steady_clock::now()<until)std::this_thread::sleep_for(1ms);
   body=Json{{"jsonrpc","2.0"},{"id",1000},{"method","eth_chainId"}}.dump();socket.write(asio::buffer(body));beast::flat_buffer buffer;socket.read(buffer);auto response=Json::parse(beast::buffers_to_string(buffer.data()));identity=response.is_object()&&response.value("id",Json())==1000&&response.value("result",Json())=="0x539";
   if(reset){auto& raw=beast::get_lowest_layer(socket).socket();raw.set_option(asio::socket_base::linger(true,0));raw.close();}else socket.close(beast::websocket::close_code::normal);
  }catch(const std::exception& e){client_error=e.what();}closed=true;});
  auto until=std::chrono::steady_clock::now()+10s;while(!started.load()&&std::chrono::steady_clock::now()<until)std::this_thread::sleep_for(1ms);bool reached=started.load();
  until=std::chrono::steady_clock::now()+2s;while(!closed.load()&&std::chrono::steady_clock::now()<until)std::this_thread::sleep_for(1ms);bool closed_during=closed.load()&&identity.load();release=true;client.join();
  // A fresh eight-call envelope only fits once the abandoned reservation is gone.
  Json cheap=Json::array();for(unsigned i=0;i<8;++i)cheap.push_back({{"jsonrpc","2.0"},{"id",2000+i},{"method","eth_call"},{"params",Json::array({Json{{"to","0x00000000000000000000000000000000000040ff"}}})}});
  bool reused=false;until=std::chrono::steady_clock::now()+5s;
  while(!reused&&std::chrono::steady_clock::now()<until){asio::io_context io;beast::tcp_stream socket(io);socket.connect({asio::ip::make_address("127.0.0.1"),server.port()});http::request<http::string_body> request(http::verb::post,"/",11);request.set(http::field::host,"localhost");request.body()=cheap.dump();request.prepare_payload();http::write(socket,request);beast::flat_buffer buffer;http::response<http::string_body> response;http::read(socket,buffer,response);if(response.result_int()==200){auto replies=Json::parse(response.body());reused=replies.size()==8;for(const auto& reply:replies)reused=reused&&reply.value("result",Json())=="0x";}else check(response.result_int()==503,"unexpected capacity status after disconnect");if(!reused)std::this_thread::sleep_for(10ms);}
  auto filter=rpc_api.handle({{"jsonrpc","2.0"},{"id",100},{"method","eth_newBlockFilter"}});
  std::cout<<Json{{"fork",fork},{"method",method},{"reset",reset},{"reachedCheckpoint",reached},{"closedWhileCalculationPaused",closed_during},{"clientError",client_error},{"checkpointPolls",polls.load()},{"capacityReused",reused},{"stateUnchanged",chain.state().root()==original},{"poolUnchanged",chain.pending_count()==0},{"laterMutationSkipped",filter.at("result")=="0x1"}}.dump()<<std::endl;
  check(reached&&closed_during&&client_error.empty(),"WebSocket cannot process identity and disconnect during calculation");check(reused&&polls.load()<1000,"abandoned simulations continued or leaked capacity");check(chain.state().root()==original&&chain.pending_count()==0&&filter.at("result")=="0x1","abandoned batch persisted writes or executed later mutation");
 }
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
