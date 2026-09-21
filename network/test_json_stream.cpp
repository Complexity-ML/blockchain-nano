#include "json_stream.hpp"
#include <iostream>

class DelayedStream:public libp2p::connection::Stream {
 asio::io_context& io_;
 ReadCallbackFunc read_;WriteCallbackFunc write_;
public:
 unsigned resets=0;
 explicit DelayedStream(asio::io_context& io):io_(io){}
 void read(libp2p::BytesOut out,size_t bytes,ReadCallbackFunc cb)override{readSome(out,bytes,std::move(cb));}
 void readSome(libp2p::BytesOut,size_t,ReadCallbackFunc cb)override{read_=std::move(cb);}
 void writeSome(libp2p::BytesIn,size_t,WriteCallbackFunc cb)override{write_=std::move(cb);}
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
int main(){try{
 asio::io_context io;auto stream=std::make_shared<DelayedStream>(io);unsigned reads=0,writes=0;
 auto reader=std::make_shared<JsonRead>(io,stream,[&](Json value){++reads;check(value.is_null(),"timed out read must fail");});
 reader->start(std::chrono::milliseconds(1));io.run();
 check(reads==1&&stream->resets==1,"read timeout and late reset callback must complete once");
 io.restart();stream=std::make_shared<DelayedStream>(io);
 auto writer=std::make_shared<JsonWrite>(io,stream,Json{{"hello",true}},[&](bool ok){++writes;check(!ok,"timed out write must fail");});
 writer->start(std::chrono::milliseconds(1));io.run();
 check(writes==1&&stream->resets==1,"write timeout and late reset callback must complete once");
 std::cout<<"Real JSON operations complete once across timeout and deferred reset callbacks\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
