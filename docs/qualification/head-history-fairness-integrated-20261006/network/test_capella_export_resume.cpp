#include "capella_block_delivery.hpp"
#include <iostream>
using namespace std::chrono_literals;
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
std::string root(char c){return "0x"+std::string(64,c);}
int main(){try{
 const AltairWireProfile p{"0x539",root('a'),"0x01020304",2048,512,256,256,root('b')};
 asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(p,budget);auto knowledge=std::make_shared<AltairRelayProgress>(8);
#ifdef NANO_TEST_EXPORT_CACHE
 auto cache=std::make_shared<CapellaExportCache>(3,8192);
#endif
 unsigned fresh=0,total=0,sends=0,admitted=0;Reply held;Json last;
 auto frame=[&](unsigned n){std::vector<uint8_t>b(404,0);b[0]=100;b[180]=84;b[100]=n;std::fill(b.begin()+116,b.begin()+148,uint8_t((n-1)*17));std::string h="0x";for(auto v:b){h+="0123456789abcdef"[v>>4];h+="0123456789abcdef"[v&15];}return Json{{"chainId",p.chain_id},{"genesisHash",p.genesis_hash},{"forkDigest",p.fork_digest},{"consensusIdentity",p.consensus_identity},{"kind","block"},{"ssz",h}};};
 auto core=[&](std::string method,Json args,Reply reply){check(method=="nano_capellaExport"&&args[0]=="token","bad local export request");++total;if(++fresh>1){held=std::move(reply);return;}const auto id=args[2].get<std::string>();unsigned n=id[2]-'0';check(n>=1&&n<=3,"walk escaped trusted chain");reply(Json{{"root",id},{"available",true},{"parentRoot",root(char('0'+n-1))},{"envelope",frame(n)}});};
 bool lose_ack=false,reject=false;
 auto send=[&](Json f,Reply reply){++sends;unsigned n=std::stoul(f["ssz"].get<std::string>().substr(202,2),nullptr,16);check(n==admitted+1,"child sent before actual parent ACK");if(lose_ack){held=std::move(reply);return;}if(!reject)++admitted;reply(Json{{"accepted",!reject}});};
 auto run=[&]{fresh=0;held={};unsigned done=0;auto d=std::make_shared<CapellaBlockDelivery>(io,core,send,"token",root('0'),root('3'),ingress,knowledge,4,8192,[&](Json r){++done;last=std::move(r);},std::shared_ptr<CapellaDeliveryQuantum>{}
#ifdef NANO_TEST_EXPORT_CACHE
 ,cache
#endif
 );d->start(100ms);io.restart();io.run();check(done==1,"duplicate terminal callback");};
 for(unsigned attempt=0;attempt<3;++attempt){run();if(attempt<2){check(last==Json{{"accepted",false}}&&sends==0&&knowledge->retained()==0,"preparation manufactured admission");auto late=std::move(held);if(late){late(Json{{"accepted",true}});io.restart();io.run();check(knowledge->retained()==0,"late preparation callback cached ACK");}}}
 std::cout<<Json{{"scenario","bounded-preparation-resume"},{"exports",total},{"sends",sends},{"admitted",admitted},{"result",last}}.dump()<<'\n';
 check(last==Json{{"accepted",true}}&&sends==3&&admitted==3&&knowledge->acknowledged(root('3')),"deadline discarded local ancestry preparation and prevented catch-up");
#ifdef NANO_TEST_EXPORT_CACHE
 check(cache->retained()==3&&cache->charged_bytes()<=8192,"cache bounds exceeded");
 // Exports survive a lost ACK, but never become remote knowledge.
 knowledge=std::make_shared<AltairRelayProgress>(8);admitted=0;sends=0;lose_ack=true;unsigned before=total;run();check(total==before&&sends==1&&knowledge->retained()==0&&last==Json{{"accepted",false}},"cached export became a lost admission ACK");auto late=std::move(held);late(Json{{"accepted",true}});io.restart();io.run();check(knowledge->retained()==0,"late ACK after deadline was cached");
 lose_ack=false;run();check(total==before&&admitted==3&&knowledge->retained()==3,"cached preparation failed to require fresh ACKs");
 knowledge=std::make_shared<AltairRelayProgress>(8);admitted=0;sends=0;reject=true;run();check(sends==1&&knowledge->retained()==0&&last==Json{{"accepted",false}},"rejection was treated as remote admission");reject=false;
 auto previous=cache;cache=std::make_shared<CapellaExportCache>(3,8192);run();check(fresh==2&&cache->retained()==1&&last==Json{{"accepted",false}},"new session inherited old preparation");
 CapellaExportCache small(1,8192);small.remember(root('1'),frame(1).dump(),root('0'));small.remember(root('2'),frame(2).dump(),root('1'));check(!small.find(root('1'))&&small.find(root('2'))&&small.retained()==1&&small.charged_bytes()<=8192,"cache root eviction failed");small.remember(root('3'),std::string(8193,'x'),root('2'));check(small.find(root('2'))&&!small.find(root('3')),"oversized export evicted valid bounded entry");
 std::cout<<"Preparation resume, lost/late ACK, rejection, session reset and cache bounds passed; modeled transport only\n";
#endif
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
