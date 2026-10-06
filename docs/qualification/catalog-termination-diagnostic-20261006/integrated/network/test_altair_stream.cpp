#include "altair_worker.hpp"
#include "altair_stream_session.hpp"
#include "altair_stream_sender.hpp"
#include <future>
#include <iostream>
#include <thread>
class TestStream:public libp2p::connection::Stream {
 asio::io_context& io_;std::string input_;size_t offset_=0;ReadCallbackFunc pending_;
public:
 size_t fragment=7;unsigned resets=0,closes=0;std::string written;bool authenticated=true,fail_write=false,inline_io=false;
 std::string peer_id="QmNnooDu7bfjPFoTZYxMNLWUQJyrVwtbZg5gBMjTezGAJN";
 std::function<void()> after_write;
 bool hold_write=false;WriteCallbackFunc waiting_write;size_t waiting_bytes=0;
 void finish_write(){auto cb=std::exchange(waiting_write,WriteCallbackFunc{});if(cb)cb(waiting_bytes);}
 void next_input(std::string input){check_no_pending();input_=std::move(input);offset_=0;}
 void check_no_pending(){if(pending_)throw std::logic_error("test input must follow ACK before next read");}
 TestStream(asio::io_context& io,std::string input):io_(io),input_(std::move(input)){}
 void read(libp2p::BytesOut out,size_t bytes,ReadCallbackFunc cb)override{readSome(out,bytes,std::move(cb));}
 void readSome(libp2p::BytesOut out,size_t bytes,ReadCallbackFunc cb)override{
  if(offset_==input_.size()){pending_=std::move(cb);return;}
  const auto n=std::min({fragment,bytes,input_.size()-offset_});std::copy_n(input_.begin()+offset_,n,out.begin());offset_+=n;if(inline_io){cb(n);return;}asio::post(io_,[cb=std::move(cb),n]{cb(n);});
 }
 void writeSome(libp2p::BytesIn in,size_t bytes,WriteCallbackFunc cb)override{if(fail_write){asio::post(io_,[cb=std::move(cb)]{cb(std::make_error_code(std::errc::broken_pipe));});return;}const auto n=std::min(fragment,bytes);written.append(reinterpret_cast<const char*>(in.data()),n);if(after_write)after_write();if(hold_write){waiting_bytes=n;waiting_write=std::move(cb);return;}if(inline_io){cb(n);return;}asio::post(io_,[cb=std::move(cb),n]{cb(n);});}
 void deferReadCallback(outcome::result<size_t> result,ReadCallbackFunc cb)override{asio::post(io_,[result,cb=std::move(cb)]{cb(result);});}
 void deferWriteCallback(std::error_code ec,WriteCallbackFunc cb)override{if(ec)asio::post(io_,[ec,cb=std::move(cb)]{cb(ec);});}
 void reset()override{++resets;if(auto cb=std::exchange(pending_,ReadCallbackFunc{}))deferReadCallback(std::make_error_code(std::errc::operation_canceled),std::move(cb));}
 bool isClosedForRead()const override{return resets||closes;}bool isClosedForWrite()const override{return resets||closes;}bool isClosed()const override{return resets||closes;}
 void close(VoidResultHandlerFunc cb)override{++closes;asio::post(io_,[cb=std::move(cb)]{cb(outcome::success());});}
 void adjustWindowSize(uint32_t,VoidResultHandlerFunc cb)override{cb(outcome::success());}
 outcome::result<bool> isInitiator()const override{return false;}
 outcome::result<libp2p::peer::PeerId> remotePeerId()const override{if(!authenticated)return std::make_error_code(std::errc::not_supported);return libp2p::peer::PeerId::fromBase58(peer_id);}
 outcome::result<libp2p::multi::Multiaddress> localMultiaddr()const override{return std::make_error_code(std::errc::not_supported);}
 outcome::result<libp2p::multi::Multiaddress> remoteMultiaddr()const override{return std::make_error_code(std::errc::not_supported);}
};
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try{
 const AltairWireProfile profile{"0x539","0x"+std::string(64,'0'),"0x01000001",1024,256,256,256};
 const auto packet=Json{{"chainId",profile.chain_id},{"genesisHash",profile.genesis_hash},{"forkDigest",profile.fork_digest},{"kind","attestation"},{"ssz","0x0102"}}.dump();
 using Request=AltairStreamRequest;using Decision=Request::Decision;
 unsigned cases=0;
 for(unsigned mode=0;mode<3;++mode){
  const bool timeout=mode!=0;
  asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(profile,budget);auto stream=std::make_shared<TestStream>(io,packet);Request::Done done;Request::Cancellation cancelled;unsigned calls=0;
  auto request=std::make_shared<Request>(io,stream,ingress,[&](AltairWireMessage m,auto flag,auto cb){check(m.ssz==std::vector<uint8_t>({1,2}),"payload changed");++calls;cancelled=flag;done=std::move(cb);return true;});
  request->start(mode==1?std::chrono::milliseconds(50):std::chrono::seconds(2));
  if(mode==1)io.run();else io.poll();if(mode==2)request->cancel();
  check(calls==1&&budget.active()==1&&stream->written.empty(),"lease or acknowledgement preceded worker completion");
  check(cancelled->load()==timeout,"deadline cancellation mismatch");
  std::thread worker([&]{done(Decision::accepted);done(Decision::rejected);});worker.join();io.restart();io.run();
  check(budget.active()==0,"worker completion leaked lease");
  if(timeout)check(stream->written.empty()&&stream->resets>0,"late worker acknowledged expired request");
  else check(Json::parse(stream->written)==Json{{"accepted",true}}&&stream->closes==1,"duplicate worker completion replied twice");++cases;
 }
 for(unsigned mode=0;mode<7;++mode){
  asio::io_context io;InboundBudget budget;auto policy=profile;if(mode==1)policy.frame_bytes=32;
  std::string input=mode==0?std::string{}:mode==2?std::string("{\"chainId\":\"0x539\",")+packet.substr(1):mode==6?std::string("{invalid}"):packet;
  auto ingress=std::make_shared<AltairMessageIngress>(policy,budget);auto stream=std::make_shared<TestStream>(io,input);if(mode==5)stream->authenticated=false;unsigned calls=0;
  auto request=std::make_shared<Request>(io,stream,ingress,[&](auto,auto,auto done){++calls;if(mode==3)return false;done(Decision::local_failure);return true;});
  request->start(mode==0?std::chrono::milliseconds(2):std::chrono::seconds(2));io.run();check(budget.active()==0&&stream->written.empty()&&stream->resets>0,"failed request retained capacity or replied");
  check(calls==((mode==3||mode==4)?1U:0U),"invalid frame reached dispatch");++cases;
 }
 for(bool expire:{false,true}){
  asio::io_context io;InboundBudget budget;
  auto ingress=std::make_shared<AltairMessageIngress>(profile,budget);
  auto stream=std::make_shared<TestStream>(io,packet);
  std::promise<void> entered,release;auto gate=release.get_future().share();
  AltairWorker worker(1,256,[&](const auto& message,const auto&){check(message.ssz.size()==2,"worker payload");entered.set_value();gate.wait();return Decision::accepted;});
  auto request=std::make_shared<Request>(io,stream,ingress,worker.dispatcher());
  request->start(std::chrono::seconds(2));io.poll();entered.get_future().wait();
  check(budget.active()==1&&stream->written.empty(),"real worker acknowledged before completion");
  if(expire){request->cancel();worker.stop();check(budget.active()==1,"cancellation released running worker lease");}
  release.set_value();
  if(expire)worker.join();
  io.restart();io.run();worker.stop();worker.join();
  check(budget.active()==0,"real worker completion leaked admission");
  if(expire)check(stream->written.empty(),"real worker acknowledged cancelled stream");
  else check(Json::parse(stream->written)==Json{{"accepted",true}},"real worker failed acknowledgement");
  ++cases;
 }
 for(const std::string mode:{"accepted-ack-written","rejected-ack-written","ack-write-failed","inbound-budget","dispatch-unavailable","core-local-failure","observer-throws"}){
  asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(profile,budget);auto stream=std::make_shared<TestStream>(io,packet);stream->fail_write=mode=="ack-write-failed";
  std::vector<std::shared_ptr<InboundBudget::Lease>> held;
  if(mode=="inbound-budget")for(unsigned i=0;i<4;++i)held.push_back(ingress->acquire(stream->remotePeerId().value().toBase58()));
  unsigned reports=0,calls=0;Json report;
  auto request=std::make_shared<Request>(io,stream,ingress,[&](auto,auto,auto done){++calls;if(mode=="dispatch-unavailable")return false;const auto d=mode=="core-local-failure"?Decision::local_failure:mode=="rejected-ack-written"?Decision::rejected:Decision::accepted;done(d);done(d);return true;},[&](Json value){++reports;report=std::move(value);if(mode=="observer-throws")throw std::runtime_error("observer only");});
  request->start();io.run();request->cancel();
  check(reports==1&&report["reason"]==(mode=="observer-throws"?"accepted-ack-written":mode),"terminal attribution missing or duplicated");
  if(mode=="inbound-budget")check(report["budgetReason"]=="peer-bulk-concurrency","stream lost budget cause");
  check(calls==(mode=="inbound-budget"?0U:1U),"budget rejection reached worker");
  check(report["kind"].is_null()==(mode=="inbound-budget"),"predecode rejection invented message kind");
  held.clear();check(budget.active()==0,"diagnostics leaked lease");
  if(mode=="observer-throws")check(Json::parse(stream->written)==Json{{"accepted",true}},"observer affected admission");
  ++cases;
 }

 // One stream, sequential frames offered only after a complete ACK. There is
 // never more than one durable admission; a rejection also consumes a frame.
 {
  asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(profile,budget);
  auto stream=std::make_shared<TestStream>(io,packet);stream->fragment=4096;
  unsigned calls=0,acks=0;Request::Done done;
  stream->after_write=[&]{++acks;stream->next_input(packet);};
  auto session=std::make_shared<AltairStreamSession>(io,stream,ingress,[&](auto,auto,auto cb){check(budget.active()==1,"session admission bypassed lease");++calls;done=std::move(cb);return true;},Request::Observe{},3);
  session->start();io.poll();
  for(unsigned i=0;i<3;++i){
   check(calls==i+1&&acks==i&&budget.active()==1,"session pipelined before durable completion");
   auto finish=std::exchange(done,Request::Done{});finish(i==1?Decision::rejected:Decision::accepted);finish(Decision::accepted);
   io.restart();io.poll();
  }
  io.restart();io.run();
  check(calls==3&&acks==3&&stream->closes==1&&budget.active()==0,"session bound or duplicate completion failed");
  check(stream->written=="{\"accepted\":true}{\"accepted\":false}{\"accepted\":true}","session changed per-message ACK");
  std::weak_ptr<AltairStreamSession> weak=session;session.reset();check(weak.expired(),"completed session retained cycle");++cases;
 }
 for(unsigned mode=0;mode<5;++mode){
  // second frame invalid / idle / cancelled worker / failed ACK / local failure
  asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(profile,budget);
  auto stream=std::make_shared<TestStream>(io,packet);stream->fragment=4096;
  unsigned calls=0,acks=0;Request::Done done;Request::Cancellation cancelled;
  stream->after_write=[&]{++acks;stream->next_input(mode==0?"{invalid}":mode==1?"":packet);if(mode==3)stream->fail_write=true;};
  auto session=std::make_shared<AltairStreamSession>(io,stream,ingress,[&](auto,auto flag,auto cb){
   ++calls;if(calls==1)cb(Decision::accepted);else if(mode==2){cancelled=flag;done=std::move(cb);}else cb(mode==4?Decision::local_failure:Decision::accepted);return true;
  },Request::Observe{},3,std::chrono::milliseconds(50));
  session->start();io.poll();
  if(mode==2){session->cancel();check(cancelled->load()&&budget.active()==1,"session cancel released running worker");done(Decision::accepted);done(Decision::accepted);done={};}
  io.restart();io.run();
  if(!(calls==(mode<2?1U:2U)&&acks==1&&stream->resets>=1&&stream->closes==0&&budget.active()==0))throw std::runtime_error("failed session "+Json{{"mode",mode},{"calls",calls},{"acks",acks},{"resets",stream->resets},{"closes",stream->closes},{"active",budget.active()}}.dump());
  std::weak_ptr<AltairStreamSession> weak=session;session.reset();check(weak.expired(),"failed session retained cycle");++cases;
 }
 {
  // Cancellation in the ACK observer must reset even though the request has
  // already marked itself finished. No queued continuation may dispatch.
  asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(profile,budget);
  auto stream=std::make_shared<TestStream>(io,packet);stream->fragment=4096;unsigned calls=0;
  stream->after_write=[&]{stream->next_input(packet);};
  std::shared_ptr<AltairStreamSession> session;
  session=std::make_shared<AltairStreamSession>(io,stream,ingress,[&](auto,auto,auto cb){++calls;cb(Decision::accepted);return true;},[&](Json){session->cancel();});
  session->start();io.run();check(calls==1&&stream->resets==1&&budget.active()==0,"ACK-boundary cancellation left open session");++cases;
 }
 {
  // Every reuse re-enters the same peer bulk budget. Existing concurrent work
  // may take the available permits between ACK and the next read.
  asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(profile,budget);
  auto stream=std::make_shared<TestStream>(io,packet);stream->fragment=4096;unsigned calls=0;std::vector<std::shared_ptr<InboundBudget::Lease>> held;std::string reason;
  stream->after_write=[&]{stream->next_input(packet);};
  auto session=std::make_shared<AltairStreamSession>(io,stream,ingress,[&](auto,auto,auto cb){++calls;cb(Decision::accepted);return true;},[&](Json report){
   if(report["reason"]=="accepted-ack-written")asio::post(io,[&]{for(unsigned i=0;i<2;++i)held.push_back(ingress->acquire(stream->remotePeerId().value().toBase58()));});
   else if(report["reason"]=="inbound-budget")reason=report["budgetReason"];
  });
  session->start();io.run();check(calls==1&&reason=="peer-bulk-concurrency"&&stream->resets==1&&budget.active()==2,"session bypassed reacquisition limits");held.clear();check(budget.active()==0,"external budget lease leak");++cases;
 }

 {
  // No refill is possible: pre-create the peer bucket at a future monotonic
  // instant, consuming one of its eight burst tokens. The session gets seven.
  asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(profile,budget);
  auto stream=std::make_shared<TestStream>(io,packet);stream->fragment=4096;
  auto seed=ingress->acquire(stream->remotePeerId().value().toBase58(),InboundBudget::Clock::now()+std::chrono::hours(1));seed.reset();
  unsigned calls=0;std::string reason;stream->after_write=[&]{stream->next_input(packet);};
  auto session=std::make_shared<AltairStreamSession>(io,stream,ingress,[&](auto,auto,auto cb){++calls;cb(Decision::accepted);return true;},[&](Json report){if(report["reason"]=="inbound-budget")reason=report["budgetReason"];});
  session->start();io.run();check(calls==7&&reason=="peer-bulk-rate"&&budget.active()==0&&stream->resets==1,"session bypassed per-message rate charge");++cases;
 }
 {
  asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(profile,budget);
  auto stream=std::make_shared<TestStream>(io,packet);stream->fragment=4096;unsigned calls=0;
  auto wrong=Json::parse(packet);wrong["chainId"]="0x1";stream->after_write=[&]{stream->next_input(wrong.dump());};
  auto session=std::make_shared<AltairStreamSession>(io,stream,ingress,[&](auto,auto,auto cb){++calls;cb(Decision::accepted);return true;});
  session->start();io.run();check(calls==1&&stream->resets==1&&budget.active()==0,"session reused first frame domain validation");++cases;
 }
 {
  // ACK completion arriving after the per-message deadline cannot resume a
  // session, even if the underlying write eventually reports success.
  asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(profile,budget);
  auto stream=std::make_shared<TestStream>(io,packet);stream->fragment=4096;stream->hold_write=true;unsigned calls=0;
  stream->after_write=[&]{stream->next_input(packet);};
  auto session=std::make_shared<AltairStreamSession>(io,stream,ingress,[&](auto,auto,auto cb){++calls;cb(Decision::accepted);return true;},Request::Observe{},3,std::chrono::milliseconds(2));
  session->start();io.run_for(std::chrono::milliseconds(10));check(bool(stream->waiting_write)&&stream->resets==1&&budget.active()==0,"ACK deadline did not abort session");
  stream->finish_write();io.restart();io.run();check(calls==1&&stream->closes==0&&budget.active()==0,"late ACK resumed expired session");++cases;
 }

 {
  asio::io_context io;unsigned opens=0,replies=0;std::vector<std::shared_ptr<TestStream>> streams;
  auto sender=std::make_shared<AltairStreamSender>(io,[&](auto done){++opens;auto stream=std::make_shared<TestStream>(io,"");stream->fragment=4096;auto raw=stream.get();stream->after_write=[raw,&replies]{raw->next_input(replies==1?"{\"accepted\":false}":"{\"accepted\":true}");};streams.push_back(stream);done(stream);},[]{return true;},4096);
  std::function<void()> send;
  send=[&]{sender->send(Json{{"test",replies}},[&](Json ack){check(ack==Json{{"accepted",replies!=1}},"sender changed rejection ACK");if(++replies<17)send();});};
  send();io.poll();check(replies==17&&opens==2&&streams[0]->closes==1,"sender failed reuse or sixteen-frame cap");sender->stop();io.restart();io.run();check(streams[1]->resets==1,"sender retained idle stream after stop");++cases;
 }
 for(unsigned mode=0;mode<4;++mode){
  // malformed ACK / EOF-like empty reply timeout / generation loss / write failure
  asio::io_context io;unsigned opens=0,replies=0;bool current=true;auto stream=std::make_shared<TestStream>(io,mode==0?"{\"accepted\":1}":mode==1?"":"{\"accepted\":true}");stream->fragment=4096;stream->fail_write=mode==3;
  if(mode==2)stream->after_write=[&]{current=false;};
  auto sender=std::make_shared<AltairStreamSender>(io,[&](auto done){++opens;done(stream);},[&]{return current;},4096,std::chrono::milliseconds(5));
  sender->send(Json{{"test",true}},[&](Json result){++replies;check(result.is_null(),"invalid session delivery reported acceptance");});io.run();
  check(opens==1&&replies==1&&stream->resets>=1,"sender retried ambiguous delivery");++cases;
 }
 {
  asio::io_context io;std::function<void(Stream)> opened;unsigned replies=0,opens=0;
  auto sender=std::make_shared<AltairStreamSender>(io,[&](auto done){++opens;opened=std::move(done);},[]{return true;},4096);
  sender->send(Json{},[&](Json r){check(r.is_null(),"stopped open replied success");++replies;});
  sender->send(Json{},[&](Json r){check(r.is_null(),"busy sender queued work");++replies;});sender->stop();
  auto late=std::make_shared<TestStream>(io,"{\"accepted\":true}");opened(late);opened={};io.run();
  check(opens==1&&replies==2&&late->resets==1&&late->written.empty(),"late open survived cancellation");++cases;
 }
 {
  // An expired open completes after a new request has already started. Its
  // callback must neither reset the new stream nor deliver another reply.
  asio::io_context io;std::function<void(Stream)> old_open;unsigned opens=0,replies=0;
  auto stream=std::make_shared<TestStream>(io,"{\"accepted\":true}");stream->fragment=4096;
  std::shared_ptr<AltairStreamSender> sender;
  sender=std::make_shared<AltairStreamSender>(io,[&](auto done){if(++opens==1)old_open=std::move(done);else done(stream);},[]{return true;},4096,std::chrono::milliseconds(5));
  sender->send(Json{},[&](Json r){check(r.is_null(),"opening deadline not applied");++replies;sender->send(Json{},[&](Json ack){check(ack==Json{{"accepted",true}},"old timeout cancelled new delivery");++replies;});});
  io.run_for(std::chrono::milliseconds(20));auto late=std::make_shared<TestStream>(io,"");old_open(late);old_open={};
  check(replies==2&&opens==2&&late->resets==1&&stream->resets==0,"late open changed current request");sender->stop();io.restart();io.run();++cases;
 }
 {
  asio::io_context io;unsigned opens=0,replies=0;std::vector<std::shared_ptr<TestStream>> streams;
  auto sender=std::make_shared<AltairStreamSender>(io,[&](auto done){++opens;auto stream=std::make_shared<TestStream>(io,"{\"accepted\":true}");stream->fragment=4096;streams.push_back(stream);done(stream);},[]{return true;},4096,std::chrono::seconds(1),std::chrono::milliseconds(2));
  auto send=[&]{sender->send(Json{},[&](Json ack){check(ack==Json{{"accepted",true}},"idle rollover failed");++replies;});};
  send();io.run();check(streams[0]->resets==1,"idle stream did not expire");io.restart();send();io.run();check(opens==2&&replies==2,"sender reused expired stream");++cases;
 }

 {
  asio::io_context io;unsigned replies=0,opens=0;auto stream=std::make_shared<TestStream>(io,"");stream->fragment=4096;stream->inline_io=true;
  stream->after_write=[&]{stream->next_input("{\"accepted\":true}");};
  auto sender=std::make_shared<AltairStreamSender>(io,[&](auto done){++opens;done(stream);},[]{return true;},4096,std::chrono::seconds(1),std::chrono::milliseconds(2));
  auto send=[&]{sender->send(Json{},[&](Json ack){check(ack==Json{{"accepted",true}},"synchronous session ACK failed");++replies;});};
  send();check(replies==1,"first synchronous exchange did not finish");
  asio::steady_timer earlier(io);earlier.expires_at(std::chrono::steady_clock::now()-std::chrono::seconds(1));
  bool checked=false;
  earlier.async_wait([&](auto ec){check(!ec,"reuse trigger cancelled");send();
   // This barrier follows the already queued old timer, but precedes any
   // subsequently queued new idle expiration, even on a slow test host.
   asio::post(io,[&]{check(replies==2&&opens==1&&stream->resets==0,"old queued idle deadline reset newly reused stream");checked=true;sender->stop();});
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(4));io.run();
  check(checked,"queued idle barrier not executed");++cases;
 }

 {
  // Nine distinct authenticated synthetic peers share the eight bulk permits.
  // Idle sessions must not bypass the cap; control traffic remains admissible.
  asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(profile,budget);
  std::vector<std::shared_ptr<AltairStreamSession>> sessions;std::vector<std::shared_ptr<TestStream>> streams;unsigned calls=0;std::string refusal;
  auto make_stream=[&](unsigned index){auto stream=std::make_shared<TestStream>(io,packet);stream->fragment=4096;stream->peer_id.back()="123456789"[index];check(bool(stream->remotePeerId()),"invalid synthetic peer id");return stream;};
  for(unsigned i=0;i<8;++i){auto stream=make_stream(i);streams.push_back(stream);auto session=std::make_shared<AltairStreamSession>(io,stream,ingress,[&](auto,auto,auto done){++calls;done(Decision::accepted);return true;});sessions.push_back(session);session->start();}
  io.poll();check(calls==8&&budget.active()==8,"idle peer sessions escaped global budget");
  auto control=budget.acquire("control-peer");check(bool(control),"idle bulk sessions starved control capacity");control.reset();
  auto ninth=make_stream(8);auto rejected=std::make_shared<AltairStreamSession>(io,ninth,ingress,[&](auto,auto,auto){++calls;return false;},[&](Json report){if(report["reason"]=="inbound-budget")refusal=report["budgetReason"];});rejected->start();
  check(refusal=="global-bulk-concurrency"&&calls==8&&ninth->resets==1,"ninth peer bypassed active limit");
  sessions[0]->cancel();check(budget.active()==7,"idle cancellation retained permit");
  Request::Done held;auto replacement=make_stream(8);auto resumed=std::make_shared<AltairStreamSession>(io,replacement,ingress,[&](auto,auto,auto done){++calls;held=std::move(done);return true;});resumed->start();io.restart();io.poll();
  check(calls==9&&budget.active()==8&&bool(held),"released capacity not available to another peer");
  for(auto& session:sessions)session->cancel();resumed->cancel();check(budget.active()==1,"cancel released active worker permit early");held(Decision::accepted);held={};io.restart();io.run();check(budget.active()==0&&replacement->written.empty(),"cancelled worker leaked or acknowledged");++cases;
 }
 std::cout<<Json{{"scenario","altair-async-stream"},{"cases",cases},{"workerThreadCompletion",true},{"expiredWorkerLeaseRetained",true},{"realPeerConnections",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
