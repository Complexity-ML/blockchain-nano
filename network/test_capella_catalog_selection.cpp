#include <capella_head_catalog_relay.hpp>
#include <iostream>
#include <set>
using namespace std::chrono_literals;
static std::string root(char c){return "0x"+std::string(64,c);}
static void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
struct Fixture {
 AltairWireProfile profile{"0x539",root('a'),"0x01020304",1024,256,256,256,root('b')};
 std::shared_ptr<CapellaCatalogSelection> selection=std::make_shared<CapellaCatalogSelection>();
 std::vector<std::string> roots{root('1'),root('2'),root('3'),root('4')};
 std::set<std::string> accepted;std::string generation=root('e'),unknown=root('3');
 std::vector<std::string> exports;unsigned seeks=0,target=0;bool bad_seek=false,cancel_export=false,head_mode=false;size_t page=4,maximum=8;
 Json attempt(){
  asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(profile,budget);Json result;unsigned done=0,releases=0;Reply late;
  auto core=[&](std::string method,Json p,Reply reply){
   if(method=="nano_capellaCatalogGeneration")reply(generation);
   else if(method=="nano_capellaCatalogAcquire")reply(Json{{"handle",root('c')}});
   else if(method=="nano_capellaCatalogRelease"){++releases;reply(true);}
   else if(method=="nano_capellaStatus")reply(Json{{"headRoot",root('4')},{"headSlot",uint64_t(4)}});
   else if(method=="nano_capellaCatalogPage"||method=="nano_capellaCatalogSeek"){
    const bool seek=method=="nano_capellaCatalogSeek";if(seek)++seeks;
    Json entries=Json::array();bool more=false;const auto count=std::stoul(p[3].get<std::string>(),nullptr,16);
    for(const auto& id:roots)if(p[2].is_null()||id>p[2].get<std::string>()){if(entries.size()<count)entries.push_back(id);else more=true;}
    if(seek&&bad_seek)entries=Json::array({root('1')});
    reply(Json{{"kind","block"},{"sequence","0x1"},{"digest",root('d')},{"entries",entries},{"next",more?entries.back():Json(nullptr)}});
   }else if(method=="nano_capellaExport"){
    exports.push_back(p[2]);if(cancel_export){late=reply;return;}if(p[2]==unknown){reply(nullptr);return;}
    reply(Json{{"available",true},{"root",p[2]},{"envelope",{{"chainId",profile.chain_id},{"genesisHash",profile.genesis_hash},{"forkDigest",profile.fork_digest},{"consensusIdentity",profile.consensus_identity},{"kind","block"},{"ssz","0x0102"}}}});
   }else throw std::runtime_error("unexpected selection method");
  };
  auto knowledge=std::make_shared<AltairRelayProgress>(8);for(const auto& id:accepted)knowledge->acknowledge(id);
  auto deliver=[&](std::string id,Json,Reply reply){const bool ok=id==root('4')||id==root('0');if(ok){accepted.insert(id);knowledge->acknowledge(id);if(id==root('4'))++target;}reply(Json{{"accepted",ok}});};
  auto known=[&](const std::string& id){return accepted.contains(id);};
  auto completion=[&](Json x){++done;result=std::move(x);};
  std::shared_ptr<CapellaCatalogRelay> relay;std::shared_ptr<CapellaHeadCatalogRelay> head;
  if(head_mode){head=std::make_shared<CapellaHeadCatalogRelay>(io,core,deliver,[](Json,Reply){throw std::runtime_error("historical turn sent a head");},"token",root('0'),ingress,knowledge,8,8192,page,maximum,completion,known,nullptr,false,nullptr,true,selection);head->start(50ms);}
  else{relay=std::make_shared<CapellaCatalogRelay>(io,core,deliver,"token","block",ingress,page,maximum,completion,known,false,selection);relay->start(50ms);}
  if(cancel_export){io.poll();check(bool(late),"cancel never reached export");if(head)head->cancel();else relay->cancel();io.restart();}
  io.run();check(done==1,"selection completed more or less than once");check(releases>=1,"snapshot release omitted");
  if(late){const auto before=selection->after;late(Json{{"available",true}});io.restart();io.run();check(done==1&&selection->after==before&&accepted.empty(),"late export changed selection or ACK");}
  return result;
 }
};
int main(){try{
 unsigned cases=0;
 {Fixture f;for(unsigned i=0;i<8;++i)f.attempt();check(f.target==1&&f.accepted==std::set<std::string>{root('4')}&&f.seeks>0,"interrupted prefix starved later root or forged ACK");++cases;}
 {Fixture f;f.page=1;f.maximum=2;for(unsigned i=0;i<8;++i)f.attempt();check(f.target==1&&f.accepted.size()==1,"entry/page budget broke rotation");++cases;}
 {Fixture f;f.selection->generation=f.generation;f.selection->after=root('3');f.roots={root('1'),root('2'),root('4')};f.attempt();check(f.target==1&&f.seeks==1&&f.selection->after.empty(),"deleted bookmark did not seek/wrap");++cases;}
 {Fixture f;f.selection->generation=f.generation;f.selection->after=root('3');f.roots.insert(f.roots.begin(),root('0'));f.attempt();f.attempt();check(f.accepted.contains(root('0'))&&f.target==1,"new prefix root lost after wrap");++cases;}
 {Fixture f;f.selection->generation=root('f');f.selection->after=root('3');f.attempt();check(f.seeks==1&&f.target==1,"request namespace rotation reset a valid selection boundary");++cases;}
 {Fixture f;f.selection->generation=f.generation;f.selection->after=root('f');auto result=f.attempt();check(f.exports.empty()&&f.selection->after.empty()&&result["catalogExhausted"]==false&&result["success"]==false&&result["terminationReason"]=="selection-wrap-required","suffix end claimed complete scan or failed wrap");++cases;}
 {Fixture f;f.cancel_export=true;f.attempt();check(f.selection->after==root('1')&&f.accepted.empty(),"cancelled export became ACK or lost attempted position");++cases;}
 {Fixture f;f.selection->generation=f.generation;f.selection->after=root('3');f.bad_seek=true;auto result=f.attempt();check(f.exports.empty()&&f.accepted.empty()&&result["success"]==false,"invalid seek page accepted/backtracked");++cases;}
 {Fixture f;f.head_mode=true;for(unsigned i=0;i<8;++i)f.attempt();check(f.target==1&&f.accepted.size()==1,"head wrapper lost shared historical selection");++cases;}
 {Fixture f;for(char generation:std::string("abcdefab")){f.generation=root(generation);f.attempt();}check(f.target==1&&f.accepted==std::set<std::string>{root('4')},"request retirement rotation starved later root or forged ACK");++cases;}
 {Fixture f;f.selection->generation=f.generation;f.selection->after=root('3');f.selection=std::make_shared<CapellaCatalogSelection>();f.attempt();check(f.seeks==0&&f.exports.front()==root('1'),"new peer session retained selection");++cases;}
 std::cout<<cases<<" catalog selection scenarios passed; mocked transport/ACKs, not consensus qualification\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
