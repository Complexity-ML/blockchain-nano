#include "nano/http.hpp"
#include "support/process.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <atomic>
#include <future>
using namespace test;
namespace asio=boost::asio;namespace beast=boost::beast;namespace http=beast::http;
using tcp=asio::ip::tcp;
struct Running {HttpServer& server;std::thread thread;explicit Running(HttpServer& s):server(s),thread([&s]{s.run();}){}~Running(){server.stop();thread.join();}};
int main(int argc,char** argv){try{
 const bool wallet_mode=argc==2&&std::string(argv[1])=="--wallet";
 for(const auto& fork:wallet_mode?std::vector<std::string>{"Paris","Shanghai"}:std::vector<std::string>{"Shanghai"})
 for(const auto& method:wallet_mode?std::vector<std::string>{"eth_signTransaction","eth_sendTransaction"}:std::vector<std::string>{"eth_call"})
 for(unsigned mode=0;mode<3;++mode){bool reset=mode==2,pipeline=mode==1;
  Temp root;const std::string probe="0x00000000000000000000000000000000000040ad";
  auto key=bytes(Int(10),32);auto sender=public_address(public_key(key));
  auto genesis=Genesis::from_json({{"config",{{"evmFork",fork}}},{"alloc",{{sender,{{"balance","0x1000000000000000000"}}},{probe,{{"code","0x60016000556000355b8015601557600190036008565b50602a60005260206000f3"}}}}}});
  Blockchain chain(root/"db",genesis);auto original=chain.state().root();std::atomic<unsigned> polls=0;std::atomic<bool> started=false,release=false,sent=false;
  RpcSimulationLimits limits;limits.timeout=60s;limits.now=[&]{if(++polls==20){started=true;while(!release.load())std::this_thread::sleep_for(1ms);}return std::chrono::steady_clock::now();};
  Rpc rpc_api(chain,{},AdmissionBudget::Clock::now,limits);
  if(wallet_mode){Wallet(key).save_keyfile(root/"wallet.json","test-only");setenv("NANO_TRANSPORT_WALLET_PASSWORD","test-only",1);rpc_api.load_wallets({{"wallets",Json::array({Json{{"path",(root/"wallet.json").string()},{"password_env","NANO_TRANSPORT_WALLET_PASSWORD"}}})}},"127.0.0.1");unsetenv("NANO_TRANSPORT_WALLET_PASSWORD");}
  HttpServer server(rpc_api,"127.0.0.1",0);Running running(server);
  auto post=[&](beast::tcp_stream& socket,const Json& body){http::request<http::string_body> request(http::verb::post,"/",11);request.set(http::field::host,"localhost");request.keep_alive(true);request.body()=body.dump();request.prepare_payload();http::write(socket,request);};
  Json batch=Json::array();for(unsigned i=0;i<8;++i)batch.push_back({{"jsonrpc","2.0"},{"id",i},{"method",method},{"params",Json::array({Json{{"to",probe},{"gas","0x1c9c380"},{"data",hex(bytes(Int(wallet_mode?60000:600000),32))}}})}});
  if(wallet_mode)for(auto& item:batch){item["params"][0]["from"]=sender;item["params"][0].erase("gas");}
  batch.push_back({{"jsonrpc","2.0"},{"id",99},{"method","eth_newBlockFilter"}});
  Json wallet_results=Json::array();bool replies_ok=false;std::string client_error;std::jthread client([&]{try{
   asio::io_context io;beast::tcp_stream socket(io);socket.connect({asio::ip::make_address("127.0.0.1"),server.port()});post(socket,batch);
   auto until=std::chrono::steady_clock::now()+10s;while(!started.load()&&!release.load()&&std::chrono::steady_clock::now()<until)std::this_thread::sleep_for(1ms);
   if(reset){socket.socket().set_option(asio::socket_base::linger(true,0));socket.socket().close();sent=true;return;}
   if(pipeline)post(socket,Json{{"jsonrpc","2.0"},{"id",1000},{"method","eth_chainId"}});
   socket.socket().shutdown(tcp::socket::shutdown_send);sent=true;
   beast::flat_buffer buffer;http::response<http::string_body> response;http::read(socket,buffer,response);auto replies=Json::parse(response.body());replies_ok=response.result_int()==200&&replies.size()==9;
   for(unsigned i=0;i<8;++i){
    const auto result=replies.at(i).value("result",Json());
    if(wallet_mode){replies_ok=replies_ok&&!result.is_null();wallet_results.push_back(result);}
    else replies_ok=replies_ok&&result==hex(bytes(Int(42),32));
   }replies_ok=replies_ok&&replies.at(8).value("result",Json())=="0x1";
   if(pipeline){response={};http::read(socket,buffer,response);auto identity=Json::parse(response.body());replies_ok=replies_ok&&response.result_int()==200&&identity.at("id")==1000&&identity.at("result")=="0x539";}
  }catch(const std::exception& e){client_error=e.what();sent=true;}});
  auto until=std::chrono::steady_clock::now()+10s;while((!started.load()||!sent.load())&&std::chrono::steady_clock::now()<until)std::this_thread::sleep_for(1ms);bool reached=started.load()&&sent.load();
  // Give the I/O loop a chance to observe FIN/RST while execution is paused.
  auto reading=std::async(std::launch::async,[&]{asio::io_context io;beast::tcp_stream socket(io);socket.connect({asio::ip::make_address("127.0.0.1"),server.port()});post(socket,Json{{"jsonrpc","2.0"},{"id",1001},{"method","eth_chainId"}});beast::flat_buffer buffer;http::response<http::string_body> response;http::read(socket,buffer,response);return response.result_int()==200&&Json::parse(response.body()).value("result",Json())=="0x539";});
  bool concurrent_read=reading.wait_for(500ms)==std::future_status::ready;
  std::this_thread::sleep_for(50ms);release=true;client.join();concurrent_read=reading.get()&&concurrent_read;
  Json cheap=Json::array();for(unsigned i=0;i<8;++i)cheap.push_back({{"jsonrpc","2.0"},{"id",2000+i},{"method","eth_call"},{"params",Json::array({Json{{"to","0x00000000000000000000000000000000000040ff"}}})}});
  bool reused=false;until=std::chrono::steady_clock::now()+5s;
  while(!reused&&std::chrono::steady_clock::now()<until){asio::io_context io;beast::tcp_stream socket(io);socket.connect({asio::ip::make_address("127.0.0.1"),server.port()});post(socket,cheap);beast::flat_buffer buffer;http::response<http::string_body> response;http::read(socket,buffer,response);if(response.result_int()==200){auto replies=Json::parse(response.body());reused=replies.size()==8;for(const auto& reply:replies)reused=reused&&reply.value("result",Json())=="0x";}else check(response.result_int()==503,"unexpected capacity response");if(!reused)std::this_thread::sleep_for(10ms);}
  const size_t expected_pending=wallet_mode&&method=="eth_sendTransaction"&&!reset?8:0;
  bool wallet_results_valid=chain.pending_count()==expected_pending;
  if(wallet_mode&&!reset){wallet_results_valid=wallet_results_valid&&wallet_results.size()==8;
   for(size_t i=0;i<wallet_results.size();++i){
    auto tx=method=="eth_signTransaction"?std::optional<SignedTransaction>(SignedTransaction::decode(unhex(wallet_results[i].at("raw").get<std::string>()))):chain.pending_transaction(unhex(wallet_results[i].get<std::string>()));
    wallet_results_valid=wallet_results_valid&&tx&&tx->sender()==sender&&tx->tx.to==probe&&tx->tx.nonce==(method=="eth_sendTransaction"?Int(i):Int(0));
   }
  }
  auto filter=rpc_api.handle({{"jsonrpc","2.0"},{"id",100},{"method","eth_newBlockFilter"}});
  bool mutation=filter.at("result")==(reset?"0x1":"0x2");
  std::cout<<Json{{"fork",fork},{"method",method},{"mode",reset?"reset":pipeline?"pipeline-half-close":"half-close"},{"checkpointReached",reached},{"concurrentRead",concurrent_read},{"responseValid",replies_ok},{"clientError",client_error},{"checkpointPolls",polls.load()},{"capacityReused",reused},{"stateUnchanged",chain.state().root()==original},{"mutationPolicyCorrect",mutation},{"walletResultsValid",wallet_results_valid},{"pendingCount",chain.pending_count()}}.dump()<<std::endl;
  check(reached&&concurrent_read&&client_error.empty()&&reused,"HTTP disconnect test setup or capacity failure");check(reset?polls.load()<1000:replies_ok,"HTTP reset not cancelled or valid half-close/pipeline interrupted");check(chain.state().root()==original&&mutation&&wallet_results_valid,"HTTP disconnect mutation policy or state violation");
 }
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
