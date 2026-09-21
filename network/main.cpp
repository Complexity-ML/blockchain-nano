#include "json_stream.hpp"
#include "bootstrap.hpp"
#include "peer_status.hpp"
#include "inbound_budget.hpp"
#include "timed_reply.hpp"
#include <libp2p/network/impl/dnsaddr_resolver_impl.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/log/configurator.hpp>
#include <libp2p/security/noise.hpp>
#include <libp2p/transport/tcp.hpp>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <fcntl.h>
#include <unistd.h>
using Host=std::shared_ptr<libp2p::Host>;
using PeerInfo=libp2p::peer::PeerInfo;
const std::string bft="/nano/bft/1.0.0";
const std::string handshake="/nanopy/handshake/1.0.0",blocksync="/nanopy/blocksync/1.0.0",announce="/nanopy/blockannounce/1.0.0",gossip="/nanopy/txgossip/1.0.0",chainsync="/nanopy/chainsync/1.0.0";
libp2p::crypto::KeyPair identity(const std::filesystem::path& path){
 std::vector<uint8_t> secret(32),pub(32);if(std::filesystem::exists(path)){std::ifstream in(path,std::ios::binary);in.read(reinterpret_cast<char*>(secret.data()),32);if(in.gcount()!=32||in.peek()!=EOF)throw std::runtime_error("Invalid P2P identity file");}
 else{if(path.has_parent_path())std::filesystem::create_directories(path.parent_path());if(RAND_bytes(secret.data(),secret.size())!=1)throw std::runtime_error("P2P randomness failed");int fd=open(path.c_str(),O_WRONLY|O_CREAT|O_EXCL,0600);if(fd<0)throw std::runtime_error("Cannot create P2P identity");auto n=::write(fd,secret.data(),secret.size());int sync=fsync(fd);close(fd);if(n!=32||sync!=0)throw std::runtime_error("Cannot persist P2P identity");}
 auto* key=EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519,nullptr,secret.data(),secret.size());if(!key)throw std::runtime_error("Invalid Ed25519 identity");size_t size=pub.size();int ok=EVP_PKEY_get_raw_public_key(key,pub.data(),&size);EVP_PKEY_free(key);if(ok!=1||size!=32)throw std::runtime_error("Cannot derive P2P identity");using namespace libp2p::crypto;return {PublicKey{{Key::Type::Ed25519,std::move(pub)}},PrivateKey{{Key::Type::Ed25519,std::move(secret)}}};
}
class Network {
 asio::io_context& io_;Host host_;Core core_;asio::steady_timer timer_;
 Json status_;uint64_t cursor_=0;bool initialized_=false;
 std::string report_token_;InboundBudget inbound_budget_;
 struct Peer {PeerInfo info;Json status;bool syncing=false;};
 std::map<std::string,Peer> peers_;std::vector<PeerInfo> seeds_;std::set<std::string> connecting_,polling_,bft_polling_;
 std::vector<libp2p::multi::Multiaddress> bootstrap_;
 std::shared_ptr<libp2p::network::DnsaddrResolver> resolver_;
 std::map<size_t,std::vector<PeerInfo>> resolved_;
 std::set<size_t> resolving_;
 std::map<std::string,std::chrono::steady_clock::time_point> retry_after_;
 std::chrono::steady_clock::time_point resolve_due_{};
 void refresh_bootstrap(){
  auto now=std::chrono::steady_clock::now();if(now<resolve_due_)return;resolve_due_=now+std::chrono::seconds(60);
  for(size_t index=0;index<bootstrap_.size();++index){
   if(!resolving_.insert(index).second)continue;
   std::make_shared<BootstrapLookup>(resolver_,bootstrap_[index],[this,index](std::vector<PeerInfo> peers){
    resolving_.erase(index);if(!peers.empty())resolved_.insert_or_assign(index,std::move(peers));
    seeds_.clear();std::set<std::string> seen;
    for(const auto& [source,entries]:resolved_)for(const auto& peer:entries){
     auto id=peer.id.toBase58();if(seen.insert(id).second){if(seeds_.size()>=256)break;seeds_.push_back(peer);}
     else{auto it=std::find_if(seeds_.begin(),seeds_.end(),[&](const auto& known){return known.id==peer.id;});if(it!=seeds_.end())for(const auto& addr:peer.addresses)if(it->addresses.size()<64&&std::find(it->addresses.begin(),it->addresses.end(),addr)==it->addresses.end())it->addresses.push_back(addr);}
    }
    for(auto it=retry_after_.begin();it!=retry_after_.end();)if(!seen.contains(it->first))it=retry_after_.erase(it);else ++it;
   })->start();
  }
 }

 bool valid(const Json& status)const{return valid_peer_status(status,status_);}
 void exchange(PeerInfo info,std::string protocol,Json request,Reply reply){
  auto pending=std::make_shared<TimedReply>(io_,std::move(reply));pending->start();
  host_->newStream(info,{protocol},[this,request=std::move(request),pending](auto result){
   if(!result){pending->finish(nullptr);return;}auto stream=result.value().stream;
   if(!pending->on_cancel([stream]{stream->reset();}))return;
   write_json(io_,stream,request,[this,stream,pending](bool ok){
    if(!pending->active())return;
    if(!ok){stream->reset();pending->finish(nullptr);return;}
    read_json(io_,stream,[stream,pending](Json value){close_stream(stream);pending->finish(std::move(value));});
   });
  });
 }

 void send(PeerInfo info,const std::string& protocol,Json value){host_->newStream(info,{protocol},[this,value=std::move(value)](auto result){if(!result)return;auto stream=result.value().stream;write_json(io_,stream,value,[stream](bool){close_stream(stream);});});}
 void add(PeerInfo info,const Json& status){if(!valid(status)||peers_.size()>=50)return;auto id=info.id.toBase58();bool fresh=!peers_.contains(id);peers_.insert_or_assign(id,Peer{std::move(info),status});if(fresh)std::cout<<Json{{"event","peer"},{"peerId",id},{"height",status["best_block"]}}.dump()<<std::endl;}
 void import_blocks(const std::string& id,std::shared_ptr<Json> blocks,size_t index){
  if(index>=blocks->size()){if(auto it=peers_.find(id);it!=peers_.end())it->second.syncing=false;return;}
  core_.call("validator_submitBlock",Json::array({(*blocks)[index]}),[this,id,blocks,index](Json result){if(result.is_object()&&result.value("success",false))import_blocks(id,blocks,index+1);else{if(auto it=peers_.find(id);it!=peers_.end())it->second.syncing=false;std::cerr<<"Rejected block received from peer\n";}});
 }
 void sync(const std::string& id){auto it=peers_.find(id);if(it==peers_.end()||it->second.syncing||it->second.status["best_block"]<=status_["best_block"])return;it->second.syncing=true;
  exchange(it->second.info,blocksync,{{"start",status_["best_block"]}},[this,id](Json result){if(result.is_object()&&result.contains("blocks")&&result["blocks"].is_array()&&result["blocks"].size()<=50)import_blocks(id,std::make_shared<Json>(std::move(result["blocks"])),0);else if(auto it=peers_.find(id);it!=peers_.end())it->second.syncing=false;});
 }
 void inbound(const std::string& protocol,Stream stream){
  auto remote=stream->remotePeerId();if(!remote||!initialized_){stream->reset();return;}auto id=remote.value().toBase58();
  auto admission=inbound_budget_.acquire(id);if(!admission){stream->reset();return;}
  struct HeldStream {Stream stream;std::shared_ptr<InboundBudget::Lease> admission;};
  auto held=std::make_shared<HeldStream>(HeldStream{stream,std::move(admission)});
  stream=Stream(held,stream.get());
  if(protocol==handshake){read_json(io_,stream,[this,stream,id,peer=remote.value()](Json request){if(!valid(request)){stream->reset();return;}add(PeerInfo{peer,{}},request);write_json(io_,stream,status_,[stream](bool){close_stream(stream);});});return;}
  if(!peers_.contains(id)){stream->reset();return;}
  if(protocol==chainsync){write_json(io_,stream,status_,[stream](bool){close_stream(stream);});return;}
  read_json(io_,stream,[this,protocol,stream,id](Json request){try{
   if(!request.is_object()){stream->reset();return;}
   if(protocol==bft){core_.call("nano_bftExchange",Json::array({request}),[this,stream](Json result){write_json(io_,stream,result,[stream](bool){close_stream(stream);});});}
   else if(protocol==blocksync){auto start=request.value("start",Json(0));if(!start.is_number_unsigned()){stream->reset();return;}core_.call("nano_networkBlocks",Json::array({start}),[this,stream](Json result){if(result.is_null()){stream->reset();return;}write_json(io_,stream,result,[stream](bool){close_stream(stream);});});}
   else if(protocol==announce&&request.contains("block")){core_.call("validator_submitBlock",Json::array({request["block"]}),[stream](Json){close_stream(stream);});}
   else if(protocol==gossip&&request.contains("raw")&&request["raw"].is_string()){core_.call("eth_sendRawTransaction",Json::array({request["raw"]}),[stream](Json){close_stream(stream);});}
   else stream->reset();
  }catch(const std::exception&){stream->reset();}});
 }
 void schedule(){timer_.expires_after(std::chrono::seconds(1));timer_.async_wait([this](auto ec){if(!ec)tick();});}
 void tick(){core_.call("nano_networkStatus",Json::array(),[this](Json local){if(!valid_peer_status(local,local)){schedule();return;}status_=std::move(local);if(!initialized_){cursor_=status_.value("cursor",uint64_t(0));initialized_=true;std::cout<<Json{{"event","ready"},{"chainId",status_["chain_id"]}}.dump()<<std::endl;}
  refresh_bootstrap();
  for(const auto& seed:seeds_){if(connecting_.size()>=4||peers_.size()+connecting_.size()>=50)break;auto id=seed.id.toBase58();if(id==host_->getPeerInfo().id.toBase58()||peers_.contains(id)||connecting_.contains(id))continue;auto now=std::chrono::steady_clock::now();if(retry_after_.contains(id)&&now<retry_after_[id])continue;retry_after_[id]=now+std::chrono::seconds(5);connecting_.insert(id);exchange(seed,handshake,status_,[this,seed,id](Json remote){connecting_.erase(id);add(seed,remote);});}
  std::vector<PeerInfo> known;for(const auto& [id,peer]:peers_)known.push_back(peer.info);
  for(const auto& info:known){auto id=info.id.toBase58();if(!polling_.insert(id).second)continue;exchange(info,chainsync,Json::object(),[this,id](Json remote){polling_.erase(id);auto it=peers_.find(id);if(it==peers_.end())return;if(!valid(remote)){peers_.erase(it);return;}it->second.status=std::move(remote);sync(id);});}
  core_.call("nano_bftExchange",Json::array(),[this](Json local){if(!local.is_object())return;for(const auto& [id,peer]:peers_){if(!bft_polling_.insert(id).second)continue;exchange(peer.info,bft,local,[this,id](Json remote){bft_polling_.erase(id);if(remote.is_object())core_.call("nano_bftExchange",Json::array({remote}),[](Json){});});}});
  core_.call("nano_networkEvents",Json::array({cursor_}),[this](Json response){if(!response.is_object()){cursor_=status_.value("cursor",cursor_);return;}cursor_=response.value("cursor",cursor_);for(const auto& event:response.value("events",Json::array()))for(const auto& [id,peer]:peers_){if(event["type"]=="block")send(peer.info,announce,{{"block",event["value"]}});else send(peer.info,gossip,{{"raw",event["value"]}});}});
  if(!report_token_.empty()){
   Json addresses=Json::array(),peers=Json::array(),bootnodes=Json::array();for(const auto& addr:bootstrap_)bootnodes.push_back(std::string(addr.getStringAddress()));for(const auto& addr:host_->getAddressesInterfaces())addresses.push_back(std::string(addr.getStringAddress()));
   for(const auto& [id,peer]:peers_)peers.push_back({{"peerId",id},{"bestBlock",peer.status["best_block"]},{"bestHash",peer.status.value("best_hash",Json())}});
   core_.call("nano_networkReport",Json::array({report_token_,Json{{"peerId",host_->getPeerInfo().id.toBase58()},{"addresses",addresses},{"peers",peers},{"bootnodes",bootnodes}}}),[](Json){});
  }
  schedule();
 });}
public:
 Network(asio::io_context& io,Host host,uint16_t port,std::vector<libp2p::multi::Multiaddress> bootstrap,std::shared_ptr<libp2p::network::DnsaddrResolver> resolver):io_(io),host_(std::move(host)),core_(io,port),timer_(io),bootstrap_(std::move(bootstrap)),resolver_(std::move(resolver)){if(const char* token=std::getenv("NANO_NETWORK_TOKEN"))report_token_=token;}
 void start(){for(const auto& protocol:{handshake,blocksync,announce,gossip,chainsync,bft})host_->setProtocolHandler({protocol},[this,protocol](libp2p::StreamAndProtocol stream){inbound(protocol,std::move(stream.stream));});tick();}
};
int main(int argc,char** argv){try{
 if(argc<4){std::cerr<<"Usage: nano-p2p <core-rpc-port> <identity-file> <listen-multiaddress> [peer-multiaddress ...]\n";return 1;}
 auto port=std::stoul(argv[1]);if(port<1||port>65535)throw std::invalid_argument("Invalid RPC port");
 auto logging=std::make_shared<soralog::LoggingSystem>(std::make_shared<libp2p::log::Configurator>());auto configured=logging->configure();if(configured.has_error)throw std::runtime_error(configured.message);libp2p::log::setLoggingSystem(logging);
 auto injector=libp2p::injector::makeHostInjector(libp2p::injector::useKeyPair(identity(argv[2])),libp2p::injector::useSecurityAdaptors<libp2p::security::Noise>(),libp2p::injector::useTransportAdaptors<libp2p::transport::TcpTransport>());
 auto host=injector.create<Host>();auto io=injector.create<std::shared_ptr<asio::io_context>>();auto listen=libp2p::multi::Multiaddress::create(argv[3]).value();auto listening=host->listen(listen);if(!listening)throw std::runtime_error(listening.error().message());host->start();
 if(argc>54)throw std::invalid_argument("At most 50 bootstrap addresses are supported");
 std::vector<libp2p::multi::Multiaddress> seeds;for(int i=4;i<argc;++i){auto ma=libp2p::multi::Multiaddress::create(argv[i]).value();auto id=ma.getPeerId();if(!id&&!ma.hasProtocol(libp2p::multi::Protocol::Code::DNS_ADDR))throw std::invalid_argument("Peer multiaddress requires /p2p/peer-id");if(id)libp2p::peer::PeerId::fromBase58(*id).value();seeds.push_back(ma);}
 for(const auto& address:host->getAddressesInterfaces())std::cout<<Json{{"event","listen"},{"address",std::string(address.getStringAddress())+"/p2p/"+host->getPeerInfo().id.toBase58()}}.dump()<<std::endl;
 libp2p::network::c_ares::Ares cares;auto resolver=std::make_shared<libp2p::network::DnsaddrResolverImpl>(io,cares);
 Network network(*io,host,uint16_t(port),std::move(seeds),std::move(resolver));network.start();asio::signal_set signals(*io,SIGINT,SIGTERM);signals.async_wait([&](auto,int){host->stop();io->stop();});io->run();return 0;
}catch(const std::exception& e){std::cerr<<"Nano P2P: "<<e.what()<<"\n";return 1;}}
