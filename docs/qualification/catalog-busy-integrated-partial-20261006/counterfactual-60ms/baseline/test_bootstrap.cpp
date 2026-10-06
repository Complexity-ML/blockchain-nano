#include "bootstrap.hpp"
#include <iostream>
using Address=libp2p::multi::Multiaddress;
using Peer=libp2p::peer::PeerInfo;
class Resolver:public libp2p::network::DnsaddrResolver{
public:
 std::map<std::string,std::vector<Address>> records;size_t calls=0;
 void load(Address address,AddressesCallback callback)override{++calls;auto it=records.find(std::string(address.getStringAddress()));if(it==records.end())callback(outcome::failure(std::make_error_code(std::errc::host_unreachable)));else callback(it->second);}
};
static int checks=0;
void check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
int main(){try{
 auto resolver=std::make_shared<Resolver>();
 auto addr=[](const std::string& value){return Address::create(value).value();};
 // Public test identities only; no network calls are made by this fake resolver.
 std::string a="QmNnooDu7bfjPFoTZYxMNLWUQJyrVwtbZg5gBMjTezGAJN",b="QmQCU2EcMqAqQPR2i9bChDtGNJchTbq5TbXJJ16u19uLTa";
 auto leaf=addr("/ip4/127.0.0.1/tcp/10001/p2p/"+a),other=addr("/ip4/127.0.0.1/tcp/10002/p2p/"+b);
 auto lookup=[&](const Address& address){std::vector<Peer> peers;unsigned callbacks=0;std::make_shared<BootstrapLookup>(resolver,address,[&](auto result){peers=std::move(result);++callbacks;})->start();check(callbacks==1,"lookup completes exactly once");return peers;};
 check(lookup(leaf).size()==1&&resolver->calls==0,"IP bootstrap bypasses DNS TXT lookup");
 resolver->records["/dnsaddr/root.test"]={leaf,leaf,other};auto peers=lookup(addr("/dnsaddr/root.test"));check(peers.size()==2&&peers[0].addresses.size()==1,"DNS records deduplicate peers and addresses");
 resolver->records["/dnsaddr/pinned.test/p2p/"+a]={leaf,other};peers=lookup(addr("/dnsaddr/pinned.test/p2p/"+a));check(peers.size()==1&&peers[0].id.toBase58()==a,"DNS cannot replace pinned identity");
 resolver->records["/dnsaddr/pinned.test/p2p/"+a]={addr("/dnsaddr/root.test")};peers=lookup(addr("/dnsaddr/pinned.test/p2p/"+a));check(peers.size()==1&&peers[0].id.toBase58()==a,"pin survives nested DNS records");
 resolver->records["/dnsaddr/loop.test"]={addr("/dnsaddr/loop.test"),leaf};auto before=resolver->calls;check(lookup(addr("/dnsaddr/loop.test")).size()==1&&resolver->calls==before+1,"cyclic DNS records terminate");
 check(lookup(addr("/dnsaddr/missing.test")).empty(),"DNS errors return no candidates");
 for(unsigned n=0;n<7;++n)resolver->records["/dnsaddr/depth"+std::to_string(n)+".test"]={addr("/dnsaddr/depth"+std::to_string(n+1)+".test")};
 before=resolver->calls;check(lookup(addr("/dnsaddr/depth0.test")).empty()&&resolver->calls==before+4,"recursive depth is bounded");
 std::vector<Address> many;for(unsigned n=0;n<100;++n)many.push_back(addr("/ip4/127.0.0.1/tcp/"+std::to_string(10000+n)+"/p2p/"+a));resolver->records["/dnsaddr/many.test"]=many;
 peers=lookup(addr("/dnsaddr/many.test"));check(peers.size()==1&&peers[0].addresses.size()<=63,"candidate budget bounds DNS expansion");
 resolver->records["/dnsaddr/unsupported.test"]={addr("/ip4/127.0.0.1/udp/30303/p2p/"+a),addr("/ip4/127.0.0.1/tcp/30303")};check(lookup(addr("/dnsaddr/unsupported.test")).empty(),"unsupported transports and missing identity are ignored");
 check(lookup(addr("/dns4/localhost/tcp/30303/p2p/"+a)).size()==1,"DNS4 endpoints remain available to TCP transport resolver");
 std::cout<<checks<<" bootstrap discovery checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
