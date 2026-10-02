// Qualification peer only; not installed with the node.
#include "json_stream.hpp"
#include "altair_peer.hpp"
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/log/configurator.hpp>
#include <libp2p/security/noise.hpp>
#include <libp2p/transport/tcp.hpp>
#include <fstream>
#include <iostream>
int main(int argc,char** argv){try{
 if(argc!=3)throw std::invalid_argument("peer multiaddress and scenario JSON required");
 Json scenario;std::ifstream(argv[2])>>scenario;
 auto logging=std::make_shared<soralog::LoggingSystem>(std::make_shared<libp2p::log::Configurator>());auto configured=logging->configure();if(configured.has_error)throw std::runtime_error(configured.message);libp2p::log::setLoggingSystem(logging);
 auto injector=libp2p::injector::makeHostInjector(libp2p::injector::useSecurityAdaptors<libp2p::security::Noise>(),libp2p::injector::useTransportAdaptors<libp2p::transport::TcpTransport>());
 auto host=injector.create<std::shared_ptr<libp2p::Host>>();auto io=injector.create<std::shared_ptr<asio::io_context>>();host->start();
 auto address=libp2p::multi::Multiaddress::create(argv[1]).value();auto id=address.getPeerId();if(!id)throw std::invalid_argument("remote peer identity required");libp2p::peer::PeerInfo peer{libp2p::peer::PeerId::fromBase58(*id).value(),{address}};
 Json results=Json::array();size_t index=0;bool finished=false;asio::steady_timer deadline(*io);deadline.expires_after(std::chrono::seconds(25));
 auto finish=[&]{if(finished)return;finished=true;deadline.cancel();host->stop();io->stop();};deadline.async_wait([&](auto error){if(!error)finish();});
 std::function<void()> next;next=[&]{
  if(index>=scenario.at("frames").size()){finish();return;}
  auto bytes=std::make_shared<std::string>(scenario.at("frames").at(index++).get<std::string>());
  host->newStream(peer,{altair_protocol},[&,bytes](auto result){
   if(!result){results.push_back(nullptr);next();return;}auto stream=result.value().stream;
   libp2p::write(stream,libp2p::BytesIn(reinterpret_cast<const uint8_t*>(bytes->data()),bytes->size()),[&,stream,bytes](auto written){
    if(!written){stream->reset();results.push_back(nullptr);next();return;}
    read_json(*io,stream,[&,stream](Json reply){close_stream(stream);results.push_back(std::move(reply));next();});
   });
  });
 };
 if(scenario.at("status").is_null())next();else host->newStream(peer,{"/nanopy/handshake/1.0.0"},[&](auto result){
  if(!result){finish();return;}auto stream=result.value().stream;write_json(*io,stream,scenario.at("status"),[&,stream](bool written){if(!written){finish();return;}read_json(*io,stream,[&,stream](Json reply){close_stream(stream);if(reply.is_null()){finish();return;}next();});});
 });
 io->run();std::cout<<Json{{"event","altair-probe-result"},{"peerId",host->getPeerInfo().id.toBase58()},{"results",results},{"requested",scenario.at("frames").size()}}.dump()<<std::endl;
 return results.size()==scenario.at("frames").size()?0:1;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
