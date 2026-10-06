#include DISPATCH_HEADER
#include "altair_receipts.hpp"
#include <iostream>
struct Connection:std::enable_shared_from_this<Connection>{
 tcp::socket socket;beast::flat_buffer buffer;http::request<http::string_body> request;http::response<http::string_body> response;
 std::function<void(std::shared_ptr<Connection>)> handler;
 Connection(tcp::socket s,std::function<void(std::shared_ptr<Connection>)> h):socket(std::move(s)),handler(std::move(h)){}
 void read(){request={};http::async_read(socket,buffer,request,[self=shared_from_this()](auto error,size_t){if(!error)self->handler(self);});}
 void reply(Json result){response={http::status::ok,11};response.keep_alive(true);response.set(http::field::content_type,"application/json");response.body()=Json{{"jsonrpc","2.0"},{"id",1},{"result",std::move(result)}}.dump();response.prepare_payload();http::async_write(socket,response,[self=shared_from_this()](auto error,size_t){if(!error)self->read();});}
};
void need(bool b,const char* s){if(!b)throw std::runtime_error(s);}
int main(){try{
 asio::io_context io;tcp::acceptor acceptor(io,{asio::ip::make_address("127.0.0.1"),0});AltairReceipts ledger(2);const auto session=ledger.session();
 AltairWireProfile profile{"0x539","0x"+std::string(64,'0'),"0x01000001",1024,256,256,256};InboundBudget budget;AltairMessageIngress ingress(profile,budget);
 unsigned requests=0,workers=0,done=0,cancels=0,forgets=0;std::vector<std::shared_ptr<Connection>> dropped;
 auto worker=[&](auto,auto,auto finish){++workers;finish(AltairWork::Decision::accepted);return true;};
 auto handle=[&](std::shared_ptr<Connection> channel){
  const auto request=Json::parse(channel->request.body());const auto method=request.at("method").get<std::string>();const auto& args=request.at("params");need(args[0]=="token"&&args[1]==session&&args[2]=="0x1","HTTP ticket identity changed");++requests;Json result;
  if(method=="nano_altairSubmit"){const auto m=ingress.decode_compact(args[3]);need(bool(m),"HTTP envelope invalid");result=ledger.submit(session,1,*m,worker);}
  else if(method=="nano_altairJob")result=ledger.get(session,1);
  else if(method=="nano_altairCancel"){++cancels;result=ledger.get(session,1,true);}
  else{need(method=="nano_altairForget","unexpected HTTP method");++forgets;result=ledger.get(session,1,false,true);}
  // Drop three real HTTP replies AFTER server-side processing. The production
  // CoreRequest five-second deadline is unchanged; no timer shortcut.
  if(requests<=3){dropped.push_back(std::move(channel));return;}channel->reply(std::move(result));
 };
 std::function<void()> accept;accept=[&]{acceptor.async_accept([&](auto error,tcp::socket socket){if(!error)std::make_shared<Connection>(std::move(socket),handle)->read();if(acceptor.is_open())accept();});};accept();
 Core core(io,acceptor.local_endpoint().port());auto client=std::make_shared<AltairCoreDispatch>(io,[&](auto method,auto args,auto reply){core.call(method,std::move(args),std::move(reply));},profile,"token",session,0,2,3,std::chrono::milliseconds(100));
 need(client->enqueue({AltairWireKind::attestation,{1,2}},std::make_shared<std::atomic<bool>>(false),[&](auto decision){need(decision==AltairWork::Decision::accepted,"committed HTTP decision changed");++done;io.stop();}),"initial HTTP enqueue failed");
 io.run_for(std::chrono::seconds(18));
 const bool recovered=!client->blocked()&&client->pending()==0&&done==1&&workers==1&&ledger.list()["jobs"].empty();
 std::cout<<Json{{"scenario","transport-recovery-real-http-receipt-ledger"},{"recovered",recovered},{"requests",requests},{"workerExecutions",workers},{"completions",done},{"cancelRequests",cancels},{"forgetRequests",forgets},{"realHTTPConnection",true},{"realReceiptLedger",true},{"executionWorkerSimulated",true},{"productionCoreTimeoutSeconds",5},{"droppedReplies",dropped.size()},{"productionQualified",false}}.dump()<<'\n';need(recovered,"real HTTP outage permanently froze dispatch");return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
