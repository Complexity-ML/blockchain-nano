#include "nano/http.hpp"
#include "nano/simulation_queue.hpp"
#include <future>
#include "support/process.hpp"
#include "support/persistent_rpc.hpp"
#include <boost/beast.hpp>
#include <atomic>
using namespace test;
namespace asio=boost::asio;namespace beast=boost::beast;namespace http=beast::http;
using tcp=asio::ip::tcp;
struct Running {HttpServer& server;std::thread thread;explicit Running(HttpServer& s):server(s),thread([&s]{s.run();}){}~Running(){server.stop();thread.join();}};
struct Release {std::atomic<bool>& flag;~Release(){flag=true;}};
int main(){try{
 // Even a delayed event loop cannot allow an overdue worker to begin.
 {
  SimulationQueue queue(20ms);asio::io_context io;std::promise<void> release;auto gate=release.get_future().share();
  for(size_t i=0;i<SimulationQueue::worker_count;++i)asio::post(queue.workers,[&]{gate.wait();});check(queue.reserve(1),"unit reservation");
  unsigned ran=0,replies=0,drained=0;
  queue.dispatch(io.get_executor(),[&]{++ran;return Json("unexpected");},[&](std::optional<Json> result){check(!result,"overdue job produced a result");++replies;},[&]{queue.release(1);++drained;});
  std::this_thread::sleep_for(40ms);release.set_value();queue.workers.join();io.run();
  check(ran==0&&replies==1&&drained==1&&queue.pending==0,"delayed timer allowed late work or duplicate completion");
  std::cout<<Json{{"scenario","delayed-event-loop"},{"lateWork",ran},{"replies",replies},{"drained",drained}}.dump()<<std::endl;
 }

 for(bool websocket:{false,true})for(bool notifications:{false,true})for(bool wallet:{false,true}){
  Temp root;const std::string probe="0x00000000000000000000000000000000000040ad",recipient="0x00000000000000000000000000000000000040ff";
  auto key=bytes(Int(10),32);auto sender=public_address(public_key(key));
  auto genesis=Genesis::from_json({{"config",{{"evmFork","Shanghai"}}},{"alloc",{{sender,{{"balance","0x1000000000000000000"}}},{probe,{{"code","0x6000355b8015601057600190036003565b50602a60005260206000f3"}}}}}});
  Blockchain chain(root/"db",genesis);auto original=chain.state().root();
  std::atomic<unsigned> polls=0,started=0;std::atomic<bool> release=false;
  RpcSimulationLimits limits;limits.timeout=60s;limits.now=[&]{++polls;thread_local unsigned local_polls=0;if(++local_polls==20){++started;while(!release.load())std::this_thread::sleep_for(1ms);}return std::chrono::steady_clock::now();};
  Rpc api(chain,{},AdmissionBudget::Clock::now,limits);
  if(wallet){Wallet(key).save_keyfile(root/"wallet.json","test-only");setenv("NANO_QUEUE_TEST_PASSWORD","test-only",1);api.load_wallets({{"wallets",Json::array({Json{{"path",(root/"wallet.json").string()},{"password_env","NANO_QUEUE_TEST_PASSWORD"}}})}},"127.0.0.1");unsetenv("NANO_QUEUE_TEST_PASSWORD");}
  HttpServer server(api,"127.0.0.1",0,50ms);Running running(server);
  std::array<std::string,SimulationQueue::worker_count> blocker_errors;std::array<Json,SimulationQueue::worker_count> blocker_results;std::vector<std::jthread> blockers;
  for(size_t worker=0;worker<SimulationQueue::worker_count;++worker)blockers.emplace_back([&,worker]{try{PersistentRpc client(server.port());blocker_results[worker]=client.call("eth_call",Json::array({Json{{"to",probe},{"gas","0x1c9c380"},{"data",hex(bytes(Int(10000),32))}}}));}catch(const std::exception& e){blocker_errors[worker]=e.what();}});
  Release unblock{release};
  auto end=std::chrono::steady_clock::now()+5s;while(started.load()!=SimulationQueue::worker_count&&std::chrono::steady_clock::now()<end)std::this_thread::sleep_for(1ms);check(started.load()==SimulationQueue::worker_count,"not all simulation workers reached their independent execution checkpoint");
  auto envelope=[&](unsigned count){Json out=Json::array();for(unsigned i=0;i<count;++i)out.push_back({{"jsonrpc","2.0"},{"id",i+10},{"method","eth_call"},{"params",Json::array({Json{{"to",recipient}}})}});return out;};
  auto expired=envelope(2);
  if(wallet){expired[0]["method"]="eth_signTransaction";expired[1]["method"]="eth_sendTransaction";for(auto& item:expired)item["params"][0]={{"from",sender},{"to",recipient},{"value","0x1"},{"gas","0x5208"}};}
  expired.push_back({{"jsonrpc","2.0"},{"id",99},{"method","eth_newBlockFilter"}});
  if(notifications)for(auto& item:expired)item.erase("id");
  auto post=[&](const Json& body){asio::io_context io;beast::tcp_stream socket(io);socket.connect({asio::ip::make_address("127.0.0.1"),server.port()});http::request<http::string_body> req(http::verb::post,"/",11);req.set(http::field::host,"localhost");req.body()=body.dump();req.prepare_payload();http::write(socket,req);beast::flat_buffer buffer;http::response<http::string_body> reply;http::read(socket,buffer,reply);return reply;};
  auto began=std::chrono::steady_clock::now();
  if(websocket){
   asio::io_context io;beast::websocket::stream<beast::tcp_stream> socket(io);beast::get_lowest_layer(socket).connect({asio::ip::make_address("127.0.0.1"),server.port()});socket.handshake("localhost","/");auto body=expired.dump();socket.write(asio::buffer(body));beast::flat_buffer buffer;
   if(notifications){std::this_thread::sleep_for(150ms);body=Json{{"jsonrpc","2.0"},{"id",1000},{"method","eth_chainId"}}.dump();socket.write(asio::buffer(body));socket.read(buffer);auto reply=Json::parse(beast::buffers_to_string(buffer.data()));check(reply.value("id",Json())==1000&&reply.value("result",Json())=="0x539","expired notifications emitted a response or blocked identity");}
   else{socket.read(buffer);auto replies=Json::parse(beast::buffers_to_string(buffer.data()));check(replies.size()==3,"expired batch reply count");for(unsigned i=0;i<3;++i){check(replies[i].at("id")==expired[i].at("id"),"expiry ID mismatch");check(replies[i].at("error").at("code")==-32005&&replies[i].at("error").at("message")=="RPC simulation queue wait exceeded","missing queue expiry error");}}
   socket.close(beast::websocket::close_code::normal);
  }else{auto response=post(expired);check(response.result_int()==503&&response.body()=="RPC simulation queue wait exceeded","HTTP queue did not expire");}
  auto elapsed=std::chrono::steady_clock::now()-began;
  check(elapsed<1s&&!release,"expiry waited for running worker");
  check(post(envelope(8-SimulationQueue::worker_count)).result_int()==503,"expired queued work released capacity before being discarded");
  check(chain.pending_count()==0,"expired wallet work entered pool before drain");
  release=true;for(auto& blocker:blockers)blocker.join();for(size_t worker=0;worker<SimulationQueue::worker_count;++worker)check(blocker_errors[worker].empty()&&blocker_results[worker]==hex(bytes(Int(42),32)),"queue timer cancelled work that had already started");
  bool recovered=false;end=std::chrono::steady_clock::now()+3s;
  while(!recovered&&std::chrono::steady_clock::now()<end){auto response=post(envelope(8));if(response.result_int()==200){auto replies=Json::parse(response.body());recovered=replies.size()==8;for(const auto& reply:replies)recovered=recovered&&reply.value("result",Json())=="0x";}else check(response.result_int()==503,"unexpected recovery status");if(!recovered)std::this_thread::sleep_for(10ms);}
  auto filter=api.handle({{"jsonrpc","2.0"},{"id",100},{"method","eth_newBlockFilter"}});
  check(recovered&&filter.at("result")=="0x1"&&chain.pending_count()==0&&chain.state().root()==original,"expired batch ran late or leaked capacity");
  std::cout<<Json{{"transport",websocket?"websocket":"http"},{"concurrentWorkers",started.load()},{"notifications",notifications},{"wallet",wallet},{"expiryMs",std::chrono::duration<double,std::milli>(elapsed).count()},{"capacityRetainedUntilDrain",true},{"capacityRecovered",true},{"lateMutationSkipped",true},{"runningWorkCompleted",true}}.dump()<<std::endl;
 }
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
