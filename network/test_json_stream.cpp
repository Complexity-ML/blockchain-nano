#include "json_stream.hpp"
#include <iostream>

class DelayedStream:public libp2p::connection::Stream {
 asio::io_context& io_;
 ReadCallbackFunc read_;WriteCallbackFunc write_;libp2p::BytesOut read_buffer_;size_t write_bytes_=0;
public:
 unsigned resets=0;
 explicit DelayedStream(asio::io_context& io):io_(io){}
 void read(libp2p::BytesOut out,size_t bytes,ReadCallbackFunc cb)override{readSome(out,bytes,std::move(cb));}
 void readSome(libp2p::BytesOut out,size_t,ReadCallbackFunc cb)override{read_buffer_=out;read_=std::move(cb);}
 void complete_read(const std::string& input){auto cb=std::exchange(read_,ReadCallbackFunc{});if(!cb)throw std::logic_error("missing test read");std::copy(input.begin(),input.end(),read_buffer_.begin());cb(input.size());}
 void complete_write(){auto cb=std::exchange(write_,WriteCallbackFunc{});if(!cb)throw std::logic_error("missing test write");cb(write_bytes_);}
 void writeSome(libp2p::BytesIn,size_t bytes,WriteCallbackFunc cb)override{write_bytes_=bytes;write_=std::move(cb);}
 void deferReadCallback(outcome::result<size_t> result,ReadCallbackFunc cb)override{asio::post(io_,[result,cb=std::move(cb)]{cb(result);});}
 void deferWriteCallback(std::error_code ec,WriteCallbackFunc cb)override{if(ec)asio::post(io_,[ec,cb=std::move(cb)]{cb(ec);});}
 void reset()override{
  ++resets;auto reader=std::exchange(read_,ReadCallbackFunc{});auto writer=std::exchange(write_,WriteCallbackFunc{});
  auto error=std::make_error_code(std::errc::operation_canceled);
  if(reader)deferReadCallback(error,std::move(reader));if(writer)deferWriteCallback(error,std::move(writer));
 }
 bool isClosedForRead()const override{return resets!=0;}
 bool isClosedForWrite()const override{return resets!=0;}
 bool isClosed()const override{return resets!=0;}
 void close(VoidResultHandlerFunc cb)override{asio::post(io_,[cb=std::move(cb)]{cb(outcome::success());});}
 void adjustWindowSize(uint32_t,VoidResultHandlerFunc cb)override{cb(outcome::success());}
 outcome::result<bool> isInitiator()const override{return true;}
 outcome::result<libp2p::peer::PeerId> remotePeerId()const override{return std::make_error_code(std::errc::not_supported);}
 outcome::result<libp2p::multi::Multiaddress> localMultiaddr()const override{return std::make_error_code(std::errc::not_supported);}
 outcome::result<libp2p::multi::Multiaddress> remoteMultiaddr()const override{return std::make_error_code(std::errc::not_supported);}
};
void check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}

// Use the existing parent reader when its bounded overload is absent. This
// makes the oversized-status case a runtime causal refusal test on both trees.
template<class Callback> void read_status_frame(asio::io_context& io,Stream stream,Callback reply){
 if constexpr(requires { read_json(io,stream,reply,size_t(4096)); })read_json(io,std::move(stream),reply,4096);
 else read_json(io,std::move(stream),reply);
}
void status_frame_cases(){
 // Exact status boundary, fragmented/mono-chunk oversize, and unchanged
 // generic/bulk allowance. No live socket, process, filesystem or signing.
 for(unsigned mode=0;mode<4;++mode){
  asio::io_context io;auto stream=std::make_shared<DelayedStream>(io);unsigned callbacks=0;Json result;
  const auto count=mode==0?4096:4097;const std::string wire="{\"padding\":\""+std::string(count-14,'a')+"\"}";
  check(wire.size()==size_t(count),"status frame fixture size");
  auto reply=[&](Json value){++callbacks;result=std::move(value);};
  if(mode==3)read_json(io,stream,Reply(reply));else read_status_frame(io,stream,reply);
  if(mode<2){stream->complete_read(wire.substr(0,4095));io.poll();io.restart();stream->complete_read(wire.substr(4095));}
  else stream->complete_read(wire);
  io.run();check(callbacks==1,"status frame completion duplicated");
  if(mode==1||mode==2)check(result.is_null()&&stream->resets==1,"oversized status accepted or stream not reset");
  else check(result.is_object()&&stream->resets==0,"exact status boundary or generic bulk rejected");
 }
 // The existing writer must reject a control frame exceeding its explicit cap.
 asio::io_context io;auto stream=std::make_shared<DelayedStream>(io);unsigned writes=0;
 auto writer=std::make_shared<JsonWrite>(io,stream,Json{{"padding",std::string(4096,'x')}},[&](bool ok){++writes;check(!ok,"oversized status write accepted");},4096);
 writer->start();io.run();check(writes==1&&stream->resets==1,"bounded writer did not refuse before write");
 std::cout<<Json{{"scenario","status-frame-boundaries"},{"passed",true},{"cases",5},{"bulkFrameCap",JsonFrame::maximum_size}}.dump()<<'\n';
}
int main(int argc,char** argv){try{
 if(argc==2&&std::string(argv[1])=="--status-bounds-self-test"){status_frame_cases();return 0;}
 check(argc==1,"unknown JSON stream test selector");
 asio::io_context io;auto stream=std::make_shared<DelayedStream>(io);unsigned reads=0,writes=0;
 auto reader=std::make_shared<JsonRead>(io,stream,[&](Json value){++reads;check(value.is_null(),"timed out read must fail");});
 reader->start(std::chrono::milliseconds(1));io.run();
 check(reads==1&&stream->resets==1,"read timeout and late reset callback must complete once");
 io.restart();stream=std::make_shared<DelayedStream>(io);
 auto writer=std::make_shared<JsonWrite>(io,stream,Json{{"hello",true}},[&](bool ok){++writes;check(!ok,"timed out write must fail");});
 writer->start(std::chrono::milliseconds(1));io.run();
 check(writes==1&&stream->resets==1,"write timeout and late reset callback must complete once");

 // Both deadlines are expired when the reactor queues them. The earlier
 // timer delivers I/O successfully; cancelling the later, already queued
 // timeout cannot revoke its callback. It must check operation completion.
 for(unsigned mode=0;mode<3;++mode){
  asio::io_context test;auto reused=std::make_shared<DelayedStream>(test);unsigned completed=0;
  asio::steady_timer delivered(test);delivered.expires_at(std::chrono::steady_clock::time_point::min());
  delivered.async_wait([&](auto ec){check(!ec,"completion timer cancelled");if(mode==2)reused->complete_write();else reused->complete_read("{\"ok\":true}");});
  if(mode==2){auto write=std::make_shared<JsonWrite>(test,reused,Json{{"ok",true}},[&](bool ok){check(ok,"queued write completion failed");++completed;});write->start(std::chrono::steady_clock::duration::zero());}
  else if(mode==1){auto read=std::make_shared<JsonRead>(test,reused,RawJsonReply([&](auto raw){check(raw&&*raw=="{\"ok\":true}","queued raw read completion failed");++completed;}),1024);read->start(std::chrono::steady_clock::duration::zero());}
  else{auto read=std::make_shared<JsonRead>(test,reused,[&](Json value){check(value==Json{{"ok",true}},"queued read completion failed");++completed;});read->start(std::chrono::steady_clock::duration::zero());}
  test.run();check(completed==1&&reused->resets==0,"already queued timeout reset completed reusable stream");
 }
 std::cout<<"Real JSON operations complete once across timeout and deferred reset callbacks\n";
 status_frame_cases();
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
