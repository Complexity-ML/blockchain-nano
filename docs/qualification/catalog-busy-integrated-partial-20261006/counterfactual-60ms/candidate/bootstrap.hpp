#pragma once
#include <libp2p/network/dnsaddr_resolver.hpp>
#include <deque>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <utility>

// Resolve DNSADDR bootstraps without replacing a pinned peer identity.
// DNS records remain untrusted candidates: Nano's chain handshake still applies.
class BootstrapLookup:public std::enable_shared_from_this<BootstrapLookup>{
 using Address=libp2p::multi::Multiaddress;
 using Peer=libp2p::peer::PeerInfo;
 struct Work {Address address;unsigned depth;std::optional<std::string> expected;};
 std::shared_ptr<libp2p::network::DnsaddrResolver> resolver_;
 std::function<void(std::vector<Peer>)> done_;
 std::deque<Work> queue_;
 std::set<std::string> visited_;
 std::map<std::string,Peer> peers_;
 size_t processed_=0,accepted_=0;
 void next(){
  while(!queue_.empty()&&processed_<64){
   auto work=std::move(queue_.front());queue_.pop_front();++processed_;
   auto id=work.address.getPeerId();
   if(work.expected&&id&&*work.expected!=*id)continue;
   if(!work.expected&&id)work.expected=*id;
   auto key=std::string(work.address.getStringAddress())+"|"+work.expected.value_or("");
   if(!visited_.insert(key).second)continue;
   if(work.address.hasProtocol(libp2p::multi::Protocol::Code::DNS_ADDR)){
    if(work.depth>=4)continue;
    resolver_->load(work.address,[self=shared_from_this(),depth=work.depth,expected=work.expected](auto result){
     if(result)for(const auto& address:result.value()){
      if(self->queue_.size()+self->processed_>=64)break;
      self->queue_.push_back({address,depth+1,expected});
     }
     self->next();
    });return;
   }
   if(!id||!work.address.hasProtocol(libp2p::multi::Protocol::Code::TCP)||accepted_>=64)continue;
   auto peer=libp2p::peer::PeerId::fromBase58(*id);if(!peer)continue;
   auto it=peers_.find(*id);if(it==peers_.end())peers_.emplace(*id,Peer{peer.value(),{work.address}});
   else it->second.addresses.push_back(work.address);
   ++accepted_;
  }
  std::vector<Peer> result;for(auto& [id,peer]:peers_)result.push_back(std::move(peer));
  auto done=std::exchange(done_,std::function<void(std::vector<Peer>)>{});if(done)done(std::move(result));
 }
public:
 BootstrapLookup(std::shared_ptr<libp2p::network::DnsaddrResolver> resolver,Address address,std::function<void(std::vector<Peer>)> done):resolver_(std::move(resolver)),done_(std::move(done)){queue_.push_back({std::move(address),0,std::nullopt});}
 void start(){next();}
};
