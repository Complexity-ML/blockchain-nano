#include "nano/http.hpp"
#include "nano/storage_memory_policy.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <array>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <thread>
#include <unistd.h>
using namespace nano;
namespace asio=boost::asio;namespace beast=boost::beast;namespace http=beast::http;using tcp=asio::ip::tcp;
namespace {
size_t checks=0,refusals=0,groups=0;
std::string stage="arguments";Json last_response;
constexpr std::array<const char*,4> methods{"eth_getBalance","eth_getCode","eth_getTransactionCount","eth_getStorageAt"};
void check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
struct OwnedDirectory {
 std::filesystem::path path;
 OwnedDirectory(){auto text=(std::filesystem::temp_directory_path()/"nano-rpc-state-readers-XXXXXX").string();std::vector<char> name(text.begin(),text.end());name.push_back('\0');auto made=::mkdtemp(name.data());if(!made)throw std::runtime_error("cannot create owned fixture namespace");path=made;}
 ~OwnedDirectory(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
Json request(size_t method,const std::string& account,const Json& selector){
 Json params=Json::array({account});if(method==3)params.push_back("0x0");params.push_back(selector);
 return {{"jsonrpc","2.0"},{"id",method+1}, {"method",methods.at(method)},{"params",std::move(params)}};
}
Json result(const Json& response){last_response=response;check(response.is_object()&&response.contains("result")&&!response.contains("error"),"state reader must return a result without an error");return response.at("result");}
void expected(const Json& response,const Json& value){check(result(response)==value,"state reader must return the exact selected state value");}
void refused(const Json& response,int code,const std::string& message={}){last_response=response;check(response.is_object()&&response.contains("error")&&!response.contains("result")&&response.at("error").at("code")==code&&(message.empty()||response.at("error").at("message")==message),"state reader must refuse with the expected error, never a fallback result");++refusals;}
Json value_at(const State& state,size_t method,const std::string& account){const auto& value=state.get(account);if(method==3)return hex(bytes(state.get_storage(account,0),32));if(method==1)return hex(value.code);return quantity(method==0?value.balance:value.nonce);}
Json view(Blockchain& chain){return {{"head",hex(chain.tip().header.hash())},{"state",chain.state().to_json()},{"pool",chain.pool_info(true)},{"events",chain.event_cursor()}};}
void emit(const char* name){++groups;std::cout<<Json{{"scenario",name},{"passed",true},{"checks",checks},{"expectedRefusals",refusals}}.dump()<<std::endl;}
struct Fixture {
 Bytes key=bytes(Int(777),32); // Existing public synthetic unit-test key, never a consensus/campaign key.
 std::string sender=public_address(public_key(key)),target=create_address(sender,0);
 Genesis genesis;std::unique_ptr<Blockchain> chain;Block a,b;State a_state,b_state;
 explicit Fixture(const std::filesystem::path& path,const char* fork="Shanghai"){
  genesis=Genesis::from_json({{"config",{{"chainId",1337},{"evmFork",fork}}},{"alloc",{{sender,{{"balance","0x100000000000000000000"}}}}}});
  chain=std::make_unique<Blockchain>(path/"canonical",genesis);
  a=publish(*chain,1,false);a_state=chain->state();
  {Blockchain other(path/"other-branch",genesis);b=publish(other,2,true);b_state=other.state();}
  check(a.header.number==1&&b.header.number==1&&a.header.parent_hash==genesis.block.header.hash()&&b.header.parent_hash==a.header.parent_hash&&a.header.hash()!=b.header.hash(),"fixture must contain two genuine sibling blocks");
  check(a_state.get(target).balance==1&&b_state.get(target).balance==2&&a_state.get_storage(target,0)==1&&b_state.get_storage(target,0)==2&&a_state.get(sender).nonce==1&&b_state.get(sender).nonce==2&&a_state.get(target).code!=b_state.get(target).code,"all four fixture values must distinguish the genuine branches");
 }
 Block publish(Blockchain& owner,unsigned marker,bool second){
  const auto runtime=unhex(marker==1?"0x600160005260206000f3":"0x600260005260206000f3");
  // Store marker in slot zero, then install a ten-byte runtime. No external fixture or compiler.
  Bytes init={0x60,static_cast<unsigned char>(marker),0x60,0x00,0x55,0x60,0x0a,0x60,0x11,0x60,0x00,0x39,0x60,0x0a,0x60,0x00,0xf3};init.insert(init.end(),runtime.begin(),runtime.end());
  Transaction tx;tx.chain_id=1337;tx.nonce=0;tx.gas=200000;tx.gas_price=2000000000;tx.value=marker;tx.data=init;
  const auto creation=SignedTransaction::sign(tx,key);check(owner.submit(creation),"signed synthetic deployment must enter its fixture pool");
  if(second){tx.nonce=1;tx.to="0x00000000000000000000000000000000000000b1";tx.data.clear();tx.value=1;tx.gas=21000;check(owner.submit(SignedTransaction::sign(tx,key)),"second branch must have a distinct sender nonce");}
  auto block=owner.mine("0x00000000000000000000000000000000000000cc",12);
  const auto receipt=owner.store().receipt(creation.hash());check(receipt&&receipt->success&&receipt->contract_address==target&&owner.pending_count()==0,"deployment must really execute, persist a receipt and drain the pool");return block;
 }
 std::string account(size_t method) const{return method==2?sender:target;}
 void select(const Block& block){check(chain->restore(0,genesis.block.header.hash()),"fixture reorg must rewind through the real Blockchain API");chain->import_block(block,12);check(chain->tip().header.hash()==block.header.hash(),"fixture reorg must publish the exact imported sibling");}
};
void selectors(){
 OwnedDirectory owned;
 for(const auto* fork:{"Paris","Shanghai"}){
  stage=std::string("selectors-")+fork;Fixture fixture(owned.path/fork,fork);Rpc rpc(*fixture.chain);const auto original=view(*fixture.chain);
  for(size_t method=0;method<methods.size();++method){
   const auto account=fixture.account(method);const auto current=value_at(fixture.a_state,method,account),genesis=value_at(fixture.genesis.state,method,account);
   auto omitted=request(method,account,"latest");omitted["params"].erase(omitted["params"].end()-1);expected(rpc.handle(omitted),current);
   for(const auto& selector:std::array<Json,6>{Json("latest"),Json("0x1"),Json{{"blockNumber","0x1"}},Json{{"blockHash",hex(fixture.a.header.hash())}},Json{{"blockHash",hex(fixture.a.header.hash())},{"requireCanonical",false}},Json{{"blockHash",hex(fixture.a.header.hash())},{"requireCanonical",true}}})expected(rpc.handle(request(method,account,selector)),current);
   expected(rpc.handle(request(method,account,"earliest")),genesis);expected(rpc.handle(request(method,account,Json{{"blockNumber","0x0"}})),genesis);
   const std::string absent="0x00000000000000000000000000000000000000ff";expected(rpc.handle(request(method,absent,Json{{"blockHash",hex(fixture.a.header.hash())}})),value_at(fixture.a_state,method,absent));
  }
  check(view(*fixture.chain)==original,"canonical and historical readers must not mutate head/state/pool/events");
  Transaction tx;tx.chain_id=1337;tx.nonce=1;tx.to="0x00000000000000000000000000000000000000b1";tx.gas=21000;tx.gas_price=2000000000;check(fixture.chain->submit(SignedTransaction::sign(tx,fixture.key)),"pending nonce fixture must be admitted normally");const auto pending=view(*fixture.chain);
  expected(rpc.handle(request(2,fixture.sender,"pending")),Json("0x2"));expected(rpc.handle(request(2,fixture.sender,"latest")),Json("0x1"));
  for(size_t method=0;method<methods.size();++method){if(method!=2)refused(rpc.handle(request(method,fixture.account(method),"pending")),-32000);refused(rpc.handle(request(method,fixture.account(method),"safe")),-32000);}
  check(view(*fixture.chain)==pending,"legacy pending nonce and unavailable tags must leave the admitted pool unchanged");
 }
 emit("state-readers-canonical-selectors-paris-shanghai");
}
void errors(){
 stage="selector-errors";OwnedDirectory owned;Fixture fixture(owned.path);Rpc rpc(*fixture.chain);const auto original=view(*fixture.chain);const auto hash=hex(fixture.a.header.hash());
 const std::vector<Json> invalid={Json::object(),Json{{"blockNumber","0x1"},{"blockHash",hash}},Json{{"blockHash",hash},{"extra",false}},Json{{"blockNumber","latest"}},Json{{"blockNumber",1}},Json{{"blockNumber",nullptr}},Json{{"blockNumber","0x00"}},Json{{"blockNumber","0X1"}},Json{{"blockNumber","0x"}},Json{{"blockNumber","0x10000000000000000"}},Json{{"blockNumber","0x1"},{"requireCanonical",false}},Json{{"blockHash","0x01"}},Json{{"blockHash",std::string("0x")+std::string(64,'g')}},Json{{"blockHash",nullptr}},Json{{"blockHash",hash},{"requireCanonical",nullptr}},Json{{"blockHash",hash},{"requireCanonical",1}},Json{{"blockHash",hash},{"requireCanonical","true"}},Json{{"requireCanonical",true}},Json::array(),Json(nullptr),Json(1)};
 const auto unknown=hex(Bytes(32,0xee));
 for(size_t method=0;method<methods.size();++method){const auto account=fixture.account(method);
  for(const auto& selector:invalid)refused(rpc.handle(request(method,account,selector)),-32602);
  for(const auto& selector:std::array<Json,4>{Json{{"blockNumber","0x777"}},Json{{"blockHash",unknown}},Json{{"blockHash",unknown},{"requireCanonical",false}},Json{{"blockHash",unknown},{"requireCanonical",true}}})refused(rpc.handle(request(method,account,selector)),-32001,"Unknown block");
  refused(rpc.handle(request(method,account,"0x777")),-32000,"Unknown block");
  auto missing=request(method,account,"latest");missing["params"]=Json::array();refused(rpc.handle(missing),-32602);
  auto extra=request(method,account,"latest");extra["params"].push_back("extra");refused(rpc.handle(extra),-32602);
 }
 check(view(*fixture.chain)==original,"selector and arity errors must not change canonical state or the pool");emit("state-readers-selector-error-precedence");
}
void reorg(){
 stage="real-reorg";OwnedDirectory owned;Fixture fixture(owned.path);Rpc rpc(*fixture.chain);
 for(size_t method=0;method<methods.size();++method){const auto account=fixture.account(method);const auto a=value_at(fixture.a_state,method,account),b=value_at(fixture.b_state,method,account);check(a!=b,"each reader needs an independently distinct branch value");
  auto switch_during=[&](const Json& selector,unsigned at){unsigned polls=0;bool switched=false;auto reply=rpc.handle(request(method,account,selector),[&]{if(++polls==at){fixture.select(fixture.b);switched=true;}});check(switched,"real reorg must occur at the bounded capture checkpoint");return reply;};
  const Json hash={{"blockHash",hex(fixture.a.header.hash())},{"requireCanonical",true}};
  refused(switch_during(hash,3),-32000,"Selected block is not canonical");fixture.select(fixture.a);
  refused(switch_during(Json{{"blockHash",hex(fixture.a.header.hash())},{"requireCanonical",false}},3),-32000,"Selected block state is unavailable");fixture.select(fixture.a);
  refused(switch_during(Json{{"blockNumber","0x1"}},3),-32000,"Selected block state is unavailable");fixture.select(fixture.a);
  refused(switch_during("0x1",3),-32000,"Selected block state is unavailable");fixture.select(fixture.a);
  // A real reorg after the raw value is pinned (poll six), or copied
  // (poll seven), must not replace it. Both checkpoints run outside the
  // Rpc/Store mutexes. No consensus hook is used.
  expected(switch_during(hash,6),a);check(fixture.chain->tip().header.hash()==fixture.b.header.hash(),"capture test must finish on the replacement branch");fixture.select(fixture.a);
  expected(switch_during("0x1",7),a);check(fixture.chain->tip().header.hash()==fixture.b.header.hash(),"detached read must complete after the real reorg");
  refused(rpc.handle(request(method,account,hash)),-32001,"Unknown block");expected(rpc.handle(request(method,account,Json{{"blockNumber","0x1"}})),b);fixture.select(fixture.a);
  const auto before=view(*fixture.chain);unsigned polls=0;refused(rpc.handle(request(method,account,hash),[&]{if(++polls==3)throw ExecutionCancelled("synthetic state-reader cancellation");}),-32002,"synthetic state-reader cancellation");check(view(*fixture.chain)==before,"cancelled capture must not mutate canonical state");expected(rpc.handle(request(method,account,hash)),a);
 }
 emit("state-readers-detached-capture-and-real-reorg");
}
void retention(){
 stage="real-retention";OwnedDirectory owned;const auto path=owned.path/"retention";const auto key=bytes(Int(777),32);const auto sender=public_address(public_key(key));const std::string target="0x00000000000000000000000000000000000000a1";
 auto genesis=Genesis::from_json({{"config",{{"chainId",1337},{"evmFork","Shanghai"}}},{"alloc",{{sender,{{"balance","0x100000000000000000000"}}},{target,{{"balance","0x9"},{"code","0x00"},{"storage",{{"0x0","0x7"}}}}}}}});
 auto chain=std::make_unique<Blockchain>(path,genesis);std::vector<Block> blocks{genesis.block};std::vector<Bytes> transactions;std::vector<CheckpointArchiveEntry> archives;
 for(unsigned n=1;n<=6;++n){Transaction tx;tx.chain_id=1337;tx.nonce=n-1;tx.to="0x00000000000000000000000000000000000000b1";tx.value=n;tx.gas=21000;tx.gas_price=2000000000;auto signed_tx=SignedTransaction::sign(tx,key);check(chain->submit(signed_tx),"retention fixture transaction must be admitted");blocks.push_back(chain->mine("0x00000000000000000000000000000000000000cc",Int(n*12)));transactions.push_back(signed_tx.hash());const auto encoded=blocks.back().to_json(true).dump();archives.push_back({blocks.back().header.hash(),n,Bytes(encoded.begin(),encoded.end())});}
 const auto tip=blocks.back().header.hash();const auto original=view(*chain);const auto snapshot=Json{{"format","synthetic-state-reader-checkpoint-v1"},{"head",hex(tip)}}.dump();constexpr size_t budget=8*1024*1024;ForkCheckpointToken token;
 // The owned Blockchain is non-const. Empty suffix/common six keeps its cached
 // head/state exact; no ordinary mine/restore is used after storage coupling.
 auto& disk=const_cast<ChainStore&>(chain->store());token=disk.commit_execution_checkpoint(1337,genesis.block.header.hash(),{},tip,6,tip,{},snapshot,budget,8,archives);
 const auto advance=[&]{token=disk.commit_execution_checkpoint(1337,genesis.block.header.hash(),token,tip,6,tip,{},snapshot,budget,2,{},ExecutionStateRetentionAnchor{6,tip});};advance();
 auto marker=chain->store().state_pruning();check(marker&&marker->before==3&&marker->anchor.number==6&&marker->anchor.hash==tip,"real first retention batch must publish before three/anchor six");
 std::array<Json,4> latest;
 {
  Rpc rpc(*chain);const auto selected=chain->store().state_at(3);check(bool(selected),"frontier state three must really remain stored");
  for(size_t method=0;method<methods.size();++method){const auto account=method==2?sender:target;
   refused(rpc.handle(request(method,account,Json{{"blockNumber","0x2"}})),-32000,"State unavailable");refused(rpc.handle(request(method,account,Json{{"blockHash",hex(blocks[2].header.hash())},{"requireCanonical",true}})),-32000,"State unavailable");
   expected(rpc.handle(request(method,account,Json{{"blockNumber","0x3"}})),value_at(*selected,method,account));latest[method]=result(rpc.handle(request(method,account,"latest")));
  }
  unsigned polls=0;bool pruned=false;auto captured=rpc.handle(request(3,target,Json{{"blockHash",hex(blocks[3].header.hash())},{"requireCanonical",true}}),[&]{if(++polls==6){advance();pruned=true;}});check(pruned&&!chain->store().state_at(3),"second real retention batch must erase the captured historical state");expected(captured,value_at(*selected,3,target));
  marker=chain->store().state_pruning();check(marker&&marker->before==5&&marker->anchor.number==6&&marker->anchor.hash==tip,"second retention batch must publish before five/anchor six");
  for(size_t method=0;method<methods.size();++method){const auto account=method==2?sender:target;
   refused(rpc.handle(request(method,account,Json{{"blockNumber","0x3"}})),-32000,"State unavailable");refused(rpc.handle(request(method,account,Json{{"blockHash",hex(blocks[3].header.hash())}})),-32000,"State unavailable");expected(rpc.handle(request(method,account,"latest")),latest[method]);expected(rpc.handle(request(method,account,"earliest")),value_at(genesis.state,method,account));
  }
  check(view(*chain)==original,"retention and detached readers must not change published head/state/pool/events");
  for(size_t n=1;n<blocks.size();++n){const auto block=chain->store().block_by_number(Int(n));const auto receipt=chain->store().receipt(transactions[n-1]);const auto archive=chain->store().checkpoint_archive(blocks[n].header.hash(),budget);check(block&&block->header.hash()==blocks[n].header.hash()&&receipt&&receipt->block_hash==blocks[n].header.hash()&&chain->store().transaction(transactions[n-1])&&archive&&archive->payload==archives[n-1].payload,"state retention must preserve block/transaction/receipt/archive history");}
 }
 chain.reset();
 {ChainStore reopened(path,ChainStore::OpenMode::ReadOnly);const auto retained=reopened.state_pruning();const auto checkpoint=reopened.fork_checkpoint(1337,genesis.block.header.hash(),budget,token);check(retained&&retained->before==5&&retained->anchor.hash==tip&&checkpoint&&checkpoint->snapshot==snapshot,"cold read-only reopen must preserve the real retention marker and checkpoint commitment");}
 chain=std::make_unique<Blockchain>(path,genesis,ChainStore::OpenMode::ReadWriteExisting);
 {Rpc rpc(*chain);for(size_t method=0;method<methods.size();++method){const auto account=method==2?sender:target;refused(rpc.handle(request(method,account,Json{{"blockNumber","0x3"}})),-32000,"State unavailable");expected(rpc.handle(request(method,account,"latest")),latest[method]);}}
 emit("state-readers-real-retention-and-reopen");
}
struct Running {HttpServer& server;std::thread thread;explicit Running(HttpServer& value):server(value),thread([&value]{value.run();}){}~Running(){server.stop();if(thread.joinable())thread.join();}};
void transport(){
 stage="http-loopback";OwnedDirectory owned;Fixture fixture(owned.path);Rpc rpc(*fixture.chain);const auto original=view(*fixture.chain);
 {
  HttpServer server(rpc,"127.0.0.1",0);Running running(server);
  const auto post=[&](const Json& input){
   asio::io_context context;beast::tcp_stream stream(context);http::request<http::string_body> message(http::verb::post,"/",11);message.set(http::field::host,"localhost");message.set(http::field::content_type,"application/json");message.body()=input.dump();message.prepare_payload();beast::flat_buffer buffer;http::response_parser<http::string_body> parser;parser.body_limit(1024*1024);beast::error_code failure;
   // Beast deadlines govern asynchronous operations, including connect. The
   // request/response objects outlive every handler until context.run returns.
   stream.expires_after(std::chrono::seconds(5));stream.async_connect(tcp::endpoint(asio::ip::make_address("127.0.0.1"),server.port()),[&](beast::error_code error){if(error){failure=error;return;}http::async_write(stream,message,[&](beast::error_code write_error,size_t){if(write_error){failure=write_error;return;}http::async_read(stream,buffer,parser,[&](beast::error_code read_error,size_t){failure=read_error;});});});context.run();if(failure)throw std::runtime_error("bounded loopback HTTP: "+failure.message());return parser.release();
  };
  Json batch=Json::array();
  for(size_t method=0;method<methods.size();++method){const auto account=fixture.account(method);const auto good=request(method,account,Json{{"blockHash",hex(fixture.a.header.hash())},{"requireCanonical",true}});auto reply=post(good);check(reply.result()==http::status::ok,"HTTP selected state request must retain status 200");expected(Json::parse(reply.body()),value_at(fixture.a_state,method,account));batch.push_back(good);
   auto invalid=post(request(method,account,Json{{"blockNumber","latest"}}));check(invalid.result()==http::status::ok,"HTTP JSON-RPC invalid selector must be an RPC error response");refused(Json::parse(invalid.body()),-32602);
   auto unknown=post(request(method,account,Json{{"blockHash",hex(Bytes(32,0xee))},{"requireCanonical",true}}));check(unknown.result()==http::status::ok,"HTTP unknown hash must remain an RPC response");refused(Json::parse(unknown.body()),-32001,"Unknown block");
  }
  auto unknown=request(0,fixture.target,Json{{"blockHash",hex(Bytes(32,0xee))}});unknown["id"]=9;batch.push_back(unknown);auto notification=request(1,fixture.target,Json{{"blockNumber","0x1"}});notification.erase("id");batch.push_back(notification);
  const auto response=post(batch);check(response.result()==http::status::ok,"HTTP mixed batch must retain status 200");const auto replies=Json::parse(response.body());check(replies.is_array()&&replies.size()==5,"HTTP mixed batch must omit only the notification response");
  for(size_t method=0;method<methods.size();++method){check(replies.at(method).at("id")==method+1,"HTTP batch must preserve each reader request id");expected(replies.at(method),value_at(fixture.a_state,method,fixture.account(method)));}check(replies.at(4).at("id")==9,"HTTP batch must preserve the unknown-hash id");refused(replies.at(4),-32001,"Unknown block");
  const auto notified=post(Json::array({notification}));check(notified.result()==http::status::no_content&&notified.body().empty(),"HTTP notification-only selected read must return an empty 204");
 }
 check(view(*fixture.chain)==original,"HTTP state readers and server teardown must not mutate canonical state");emit("state-readers-http-object-selectors-and-batch");
}
}
int main(int argc,char** argv){try{
 if((argc!=2&&argc!=4)||std::string(argv[1])!="--state-readers-self-test"||(argc==4&&std::string(argv[2])!="--group"))throw std::invalid_argument("expected --state-readers-self-test [--group selectors|errors|reorg|retention|http]");
 const std::string selected=argc==4?argv[3]:"all";if(selected!="all"&&selected!="selectors"&&selected!="errors"&&selected!="reorg"&&selected!="retention"&&selected!="http")throw std::invalid_argument("unknown state-reader group");
 const ChainStoreMemoryPolicy policy{4*1024*1024,2,8*1024*1024,4*1024*1024};ScopedChainStoreMemoryPolicy memory(policy);
 if(selected=="all"||selected=="selectors")selectors();if(selected=="all"||selected=="errors")errors();if(selected=="all"||selected=="reorg")reorg();if(selected=="all"||selected=="retention")retention();if(selected=="all"||selected=="http")transport();
 std::cout<<Json{{"scenario","state-readers-summary"},{"passed",true},{"groups",groups},{"checks",checks},{"expectedRefusals",refusals},{"selectedGroup",selected},{"methods",4},{"externalNetwork",false},{"consensusModified",false}}.dump()<<std::endl;return 0;
 }catch(const std::exception& error){std::cerr<<Json{{"scenario","state-readers-summary"},{"passed",false},{"stage",stage},{"checks",checks},{"expectedRefusals",refusals},{"error",error.what()},{"lastRpcResponse",last_response}}.dump()<<std::endl;return 1;}}
