#include "altair_worker.hpp"
#include <future>
#include <iostream>
#include <thread>
class TestStream:public libp2p::connection::Stream {
 asio::io_context& io_;std::string input_;size_t offset_=0;ReadCallbackFunc pending_;
public:
 size_t fragment=7;unsigned resets=0,closes=0;std::string written;bool authenticated=true;
 TestStream(asio::io_context& io,std::string input):io_(io),input_(std::move(input)){}
 void read(libp2p::BytesOut out,size_t bytes,ReadCallbackFunc cb)override{readSome(out,bytes,std::move(cb));}
 void readSome(libp2p::BytesOut out,size_t bytes,ReadCallbackFunc cb)override{
  if(offset_==input_.size()){pending_=std::move(cb);return;}
  const auto n=std::min({fragment,bytes,input_.size()-offset_});std::copy_n(input_.begin()+offset_,n,out.begin());offset_+=n;asio::post(io_,[cb=std::move(cb),n]{cb(n);});
 }
 void writeSome(libp2p::BytesIn in,size_t bytes,WriteCallbackFunc cb)override{const auto n=std::min(fragment,bytes);written.append(reinterpret_cast<const char*>(in.data()),n);asio::post(io_,[cb=std::move(cb),n]{cb(n);});}
 void deferReadCallback(outcome::result<size_t> result,ReadCallbackFunc cb)override{asio::post(io_,[result,cb=std::move(cb)]{cb(result);});}
 void deferWriteCallback(std::error_code ec,WriteCallbackFunc cb)override{if(ec)asio::post(io_,[ec,cb=std::move(cb)]{cb(ec);});}
 void reset()override{++resets;if(auto cb=std::exchange(pending_,ReadCallbackFunc{}))deferReadCallback(std::make_error_code(std::errc::operation_canceled),std::move(cb));}
 bool isClosedForRead()const override{return resets||closes;}bool isClosedForWrite()const override{return resets||closes;}bool isClosed()const override{return resets||closes;}
 void close(VoidResultHandlerFunc cb)override{++closes;asio::post(io_,[cb=std::move(cb)]{cb(outcome::success());});}
 void adjustWindowSize(uint32_t,VoidResultHandlerFunc cb)override{cb(outcome::success());}
 outcome::result<bool> isInitiator()const override{return false;}
 outcome::result<libp2p::peer::PeerId> remotePeerId()const override{if(!authenticated)return std::make_error_code(std::errc::not_supported);return libp2p::peer::PeerId::fromBase58("QmNnooDu7bfjPFoTZYxMNLWUQJyrVwtbZg5gBMjTezGAJN");}
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
 std::cout<<Json{{"scenario","altair-async-stream"},{"cases",cases},{"workerThreadCompletion",true},{"expiredWorkerLeaseRetained",true},{"realPeerConnections",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
