#include "nano/http.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <thread>
#include <iostream>
#include <unistd.h>
using namespace nano;
namespace asio=boost::asio;namespace beast=boost::beast;namespace websocket=beast::websocket;using tcp=asio::ip::tcp;
static int checks=0;
void check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
struct Running{HttpServer& server;std::thread thread;Running(HttpServer& s):server(s),thread([&s]{s.run();}){}~Running(){server.stop();thread.join();}};
int main(){try{
 auto path=std::filesystem::temp_directory_path()/("nano-events-"+std::to_string(getpid()));
 auto key=bytes(Int(1),32);auto sender=public_address(public_key(key));std::string contract="0x00000000000000000000000000000000000000ab";auto topic=hex(bytes(Int(7),32));
 Json alloc={{sender,{{"balance","0x100000000000000000"}}},{contract,{{"code","0x602a600052600760206000a100"}}}};
 std::string multi="0x00000000000000000000000000000000000000ac",reverter="0x00000000000000000000000000000000000000ad";
 alloc[multi]={{"code","0x6008600760006000a260006000a000"}};
 alloc[reverter]={{"code","0x600760006000a160006000fd"}};
 {Blockchain chain(path,Genesis::from_json({{"alloc",alloc}}));Rpc rpc(chain);
 auto request=[](std::string method,Json p=Json::array()){return Json{{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",p}};};
 auto call=[&](std::string method,Json p=Json::array()){auto response=rpc.handle(request(method,p));if(response.contains("error"))throw std::runtime_error(response.dump());return response["result"];};
 auto pending=call("eth_newPendingTransactionFilter");auto heads=call("eth_newBlockFilter");
 Json criterion={{"address",contract},{"topics",Json::array({Json::array({topic,hex(bytes(Int(8),32))})})}};
 auto filter=call("eth_newFilter",{criterion});
 Transaction tx;tx.to=contract;tx.gas=100000;tx.gas_price=2000000000;auto signed_tx=SignedTransaction::sign(tx,key);chain.submit(signed_tx);auto first=chain.mine(sender,24);
 check(call("eth_getFilterChanges",{pending})==Json::array({hex(signed_tx.hash())}),"pending event survives mining before poll");
 check(call("eth_getFilterChanges",{heads})==Json::array({hex(first.header.hash())}),"block filter hashes");
 auto logs=call("eth_getFilterChanges",{filter});check(logs.size()==1&&logs[0]["topics"][0]==topic&&logs[0]["data"]==hex(bytes(Int(42),32)),"address and OR topic filter");
 check(logs[0]["blockHash"]==hex(first.header.hash())&&logs[0]["transactionHash"]==hex(signed_tx.hash())&&logs[0]["logIndex"]=="0x0"&&!logs[0]["removed"].get<bool>(),"log metadata");
 check(call("eth_getFilterChanges",{filter}).empty()&&call("eth_getFilterChanges",{heads}).empty()&&call("eth_getFilterChanges",{pending}).empty(),"filter cursor drains exactly once");
 check(call("eth_getFilterLogs",{filter})==logs,"filter history remains queryable");
 check(call("eth_getLogs",{Json{{"blockHash",hex(first.header.hash())}}})==logs,"logs by block hash");
 check(call("eth_getLogs",{Json{{"address",sender},{"fromBlock","earliest"}}}).empty(),"nonmatching address excluded");
 check(call("eth_getLogs",{Json{{"topics",Json::array({hex(bytes(Int(9),32))})}}}).empty(),"nonmatching topic excluded");
 check(rpc.handle(request("eth_getLogs",{Json{{"topics",Json::array({"0x01"})}}}))["error"]["code"]==-32602,"invalid topic rejected");
 check(rpc.handle(request("eth_getLogs",{Json{{"blockHash",hex(first.header.hash())},{"fromBlock","0x0"}}}))["error"]["code"]==-32602,"conflicting block criteria rejected");
 check(call("eth_uninstallFilter",{filter})==true&&call("eth_uninstallFilter",{filter})==false,"filter uninstall");
 check(rpc.handle(request("eth_getFilterChanges",{filter}))["error"]["code"]==-32000,"unknown filter reports error");
 HttpServer server(rpc,"127.0.0.1",0);Running running(server);asio::io_context context;
 websocket::stream<tcp::socket> ws(context);ws.next_layer().connect(tcp::endpoint(asio::ip::make_address("127.0.0.1"),server.port()));ws.handshake("localhost","/");
 auto read=[&](){beast::flat_buffer buffer;ws.read(buffer);return Json::parse(beast::buffers_to_string(buffer.data()));};
 auto send=[&](const Json& req){auto text=req.dump();ws.write(asio::buffer(text));};
 send(request("eth_chainId"));check(read()["result"]=="0x539","WebSocket ordinary RPC");
 send(request("eth_subscribe",{"newPendingTransactions"}));auto pending_id=read()["result"];
 send(request("eth_subscribe",{"newHeads"}));auto heads_id=read()["result"];
 send(request("eth_subscribe",{"logs",criterion}));auto logs_id=read()["result"];
 check(pending_id.is_string()&&heads_id.is_string()&&logs_id.is_string()&&pending_id!=heads_id,"WebSocket subscription IDs");
 tx.nonce=1;signed_tx=SignedTransaction::sign(tx,key);send(request("eth_sendRawTransaction",{hex(signed_tx.raw())}));check(read()["result"]==hex(signed_tx.hash()),"WebSocket transaction submission");
 auto event=read();check(event["method"]=="eth_subscription"&&event["params"]["subscription"]==pending_id&&event["params"]["result"]==hex(signed_tx.hash()),"pending notification");
 auto second=chain.mine(sender,48);Json events=Json::object();for(int i=0;i<2;++i){auto e=read();events[e["params"]["subscription"].get<std::string>()]=e["params"]["result"];}
 check(events[heads_id.get<std::string>()]["hash"]==hex(second.header.hash()),"newHeads notification after durable commit");
 check(events[logs_id.get<std::string>()]["transactionHash"]==hex(signed_tx.hash()),"filtered log notification");
 send(request("eth_unsubscribe",{heads_id}));check(read()["result"]==true,"WebSocket unsubscribe");
 send(request("eth_unsubscribe",{heads_id}));check(read()["result"]==false,"WebSocket unsubscribe unknown");
 send(request("eth_subscribe",{"invalid"}));check(read()["error"]["code"]==-32602,"invalid subscription type");
 send(Json::array({Json::array({request("eth_chainId")}),request("eth_chainId")}));auto batch=read();check(batch.size()==2&&batch[0]["error"]["code"]==-32600&&batch[1]["result"]=="0x539","WebSocket rejects nested batch");
 websocket::stream<tcp::socket> other(context);other.next_layer().connect(tcp::endpoint(asio::ip::make_address("127.0.0.1"),server.port()));other.handshake("localhost","/");
 auto remove=request("eth_unsubscribe",{logs_id}).dump();other.write(asio::buffer(remove));beast::flat_buffer reply;other.read(reply);check(Json::parse(beast::buffers_to_string(reply.data()))["result"]==false,"subscriptions belong to their connection");other.close(websocket::close_code::normal);
 ws.close(websocket::close_code::normal);
 check(call("eth_getLogs",{Json{{"fromBlock","earliest"}}}).size()==2,"historical logs span blocks");
 tx.nonce=2;tx.to=multi;auto log_first=SignedTransaction::sign(tx,key);
 tx.nonce=3;tx.to=reverter;auto failed=SignedTransaction::sign(tx,key);
 tx.nonce=4;tx.to=multi;auto log_last=SignedTransaction::sign(tx,key);
 check(chain.submit(log_first)&&chain.submit(failed)&&chain.submit(log_last),"multiple-log fixtures submitted");
 auto third=chain.mine(sender,72);auto all=call("eth_getLogs",{Json{{"blockHash",hex(third.header.hash())}}});
 check(all.size()==4&&!chain.store().receipt(failed.hash())->success&&chain.store().receipt(failed.hash())->logs.empty(),"reverted transaction contributes no logs");
 for(size_t i=0;i<all.size();++i)check(all[i]["logIndex"]==quantity(Int(i))&&all[i]["transactionIndex"]==(i<2?"0x0":"0x2"),"log indices remain contiguous across failed transaction");
 auto criteria=Json{{"blockHash",hex(third.header.hash())},{"topics",Json::array({nullptr,hex(bytes(Int(8),32))})}};
 auto matched=call("eth_getLogs",{criteria});check(matched.size()==2&&matched[0]["topics"]==Json::array({topic,hex(bytes(Int(8),32))}),"positional wildcard matches LOG2 but excludes LOG0");
 criteria["topics"]=Json::array({topic,hex(bytes(Int(9),32))});check(call("eth_getLogs",{criteria}).empty(),"topic positions combine with AND");
 criteria["topics"]=Json::array({Json::array()});check(call("eth_getLogs",{criteria}).empty(),"empty topic alternatives match nothing");
 criteria["topics"]=Json::array();criteria["address"]=Json::array({contract,multi});check(call("eth_getLogs",{criteria})==all,"address alternatives include matching contract");
 criteria["address"]=Json::array();check(call("eth_getLogs",{criteria}).empty(),"empty address alternatives match nothing");
 check(call("eth_getLogs",{Json{{"fromBlock","0x3"},{"toBlock","0x2"}}}).empty(),"reversed log range is empty");
 check(call("eth_getLogs",{Json{{"blockHash",hex(Bytes(32))}}}).empty(),"unknown block hash has no logs");
 auto history_filter=call("eth_newFilter",{Json{{"fromBlock","0x3"},{"address",multi}}});
 check(call("eth_getFilterLogs",{history_filter})==all&&call("eth_getFilterLogs",{history_filter})==all,"historical filter reads do not consume logs");
 check(call("eth_getFilterChanges",{history_filter}).empty(),"new filter does not replay past events as changes");

 }
 std::filesystem::remove_all(path);std::cout<<checks<<" event and WebSocket checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
