#include "nano/http.hpp"
#include "nano/websocket_output.hpp"
#include "support/process.hpp"
#include "support/persistent_rpc.hpp"
#include "support/persistent_websocket_rpc.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
using namespace test;
namespace asio=boost::asio;namespace beast=boost::beast;using tcp=asio::ip::tcp;
struct Running {HttpServer& server;std::thread thread;explicit Running(HttpServer& s):server(s),thread([&s]{s.run();}){}~Running(){server.stop();thread.join();}};
class Sender {
 asio::io_context context_;
 beast::websocket::stream<beast::tcp_stream> socket_{context_};
 asio::steady_timer timer_{context_};
public:
 explicit Sender(uint16_t port){auto& tcp_socket=beast::get_lowest_layer(socket_).socket();tcp_socket.open(tcp::v4());tcp_socket.set_option(asio::socket_base::receive_buffer_size(4096));beast::get_lowest_layer(socket_).connect({asio::ip::make_address("127.0.0.1"),port});socket_.handshake("localhost","/");}
 bool write(const std::string& body){
  context_.restart();bool expired=false;beast::error_code error;
  timer_.expires_after(3s);timer_.async_wait([&](beast::error_code ec){if(ec)return;expired=true;beast::error_code ignored;beast::get_lowest_layer(socket_).socket().cancel(ignored);});
  socket_.async_write(asio::buffer(body),[&](beast::error_code ec,size_t){error=ec;timer_.cancel();});context_.run();
  check(!expired,"slow-reader write timed out instead of being disconnected");return !error;
 }
};
int main(){try{
 // Exact byte boundaries include JSON quoting/escaping, not input character count.
 auto budget=std::make_shared<WebSocketOutputBudget>();budget->connection_limit=10;budget->total_limit=14;budget->message_limit=3;
 {
  WebSocketOutput a(budget),b(budget);
  check(a.push("12345678")&&a.bytes()==10&&budget->used==10,"exact connection byte boundary");
  auto pointer=a.front().data();auto value=a.front();
  check(!a.push(nullptr)&&a.front()==value&&a.front().data()==pointer,"over-budget append changed active buffer");
  check(b.push(nullptr)&&budget->used==14,"exact shared boundary");
  check(!b.push(0)&&budget->used==14,"shared budget bypass");
  b.pop_front();check(budget->used==10,"completion releases shared budget");
  check(b.push(0),"released bytes reusable by another connection");
  a.pop_front();check(budget->used==1,"active write accounted until completion");
  check(a.push(std::string(1,'\1'))&&a.bytes()==8,"escaped JSON byte accounting");
  check(!a.push(std::string(1,'\1'))&&a.bytes()==8,"escaped expansion exceeded available bytes");
  a.pop_front();check(!a.push(std::string(20,'x'))&&budget->used==1,"oversized serialization leaked reservation");
 }
 check(budget->used==0,"destruction leaked shared bytes");
 budget->connection_limit=100;budget->total_limit=200;
 {
  WebSocketOutput queue(budget);check(queue.push(1),"first message");auto pointer=queue.front().data();
  check(queue.push(22)&&queue.push(333)&&!queue.push(4),"message-count boundary");
  queue.discard_pending(true);check(queue.size()==1&&queue.front()=="1"&&queue.front().data()==pointer&&budget->used==1,"disconnect invalidated active write or retained pending tails");
  queue.pop_front();check(budget->used==0,"cancelled write did not release final reservation");
  check(queue.push(1)&&queue.push(2),"budget reusable after cancelled write");queue.discard_pending(false);check(budget->used==0&&queue.empty(),"write error must discard all remaining messages");
 }
 Temp root;Blockchain chain(root/"chain",Genesis::from_json(Json::object()));Rpc rpc(chain);HttpServer server(rpc,"127.0.0.1",0);Running running(server);
 auto original=chain.tip().header.hash();
 PersistentRpc http(server.port());PersistentWebSocketRpc ws(server.port());
 Json counts=Json::array();
 const auto body=Json{{"jsonrpc","2.0"},{"id",std::string(1024*1024,'x')},{"method","eth_chainId"}}.dump();
 // Three finite clients intentionally never consume responses. Each sends at
 // most 32 requests, and must be closed before that bound, without a timeout.
 for(unsigned cycle=0;cycle<3;++cycle){
  Sender slow(server.port());unsigned accepted=0;bool disconnected=false;
  for(unsigned attempt=0;attempt<32;++attempt){if(!slow.write(body)){disconnected=true;break;}++accepted;}
  check(disconnected,"slow reader was not disconnected within bounded workload");
  check(http.call("eth_chainId")=="0x539"&&ws.call("eth_chainId")=="0x539","slow reader blocked other transports");
  counts.push_back(accepted);
 }
 check(chain.tip().header.hash()==original&&chain.pending_count()==0,"slow-reader workload changed canonical state");
 std::cout<<Json{{"scenario","websocket-output-budget"},{"cycles",3},{"acceptedWritesBeforeDisconnect",counts},{"unitSharedBudgetReturnedToZero",budget->used==0},{"otherClientsResponsive",true},{"canonicalStateUnchanged",true}}.dump()<<std::endl;
 return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<std::endl;return 1;}}
