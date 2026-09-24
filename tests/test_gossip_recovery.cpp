#include "support/process.hpp"
#include <set>
using namespace test;
int main(){try{
 Temp root;Json alloc=Json::object();std::vector<Bytes> keys;
 for(unsigned i=1;i<=101;++i){keys.push_back(bytes(Int(i),32));alloc[public_address(public_key(keys.back()))]={{"balance",quantity(Int("1000000000000000000000000000000"))}};}
 auto genesis=Genesis::from_json({{"config",{{"chainId",1337}}},{"alloc",alloc}});std::string epoch;uint64_t barrier=0;
 {Blockchain chain(root/"node",genesis);
  for(unsigned i=0;i<10001;++i){Transaction tx;tx.chain_id=1337;tx.nonce=i%100;tx.gas_price=2000000000;tx.to=public_address(public_key(keys.back()));check(chain.submit(SignedTransaction::sign(tx,keys[i/100])),"fixture admission");}
  bool expired=false;try{chain.events_since(0);}catch(const std::runtime_error& e){expired=std::string(e.what())=="event history expired";}check(expired,"real 10000-event retention boundary crossed");
  auto first=chain.gossip_page(0,"","",false);epoch=first.at("epoch");barrier=first.at("cursor");check(first.at("mode")=="inventory"&&barrier==10001&&first.at("transactions").size()==50,"initial bounded inventory");
  auto gap=chain.gossip_page(0,epoch,"",false);check(gap==first,"expired same-session cursor starts inventory");
  auto repeat=chain.gossip_page(0,"","",false);check(repeat==first,"lost first page can be repeated");
  std::string after=first.at("next");std::set<std::string> seen;
  for(const auto& raw:first.at("transactions"))seen.insert(hex(SignedTransaction::decode(unhex(raw.get<std::string>())).hash()));
  // Add behind the ordered inventory cursor after its event barrier was fixed.
  SignedTransaction late;for(unsigned n=1;;++n){Transaction tx;tx.chain_id=1337;tx.nonce=n;tx.gas_price=2000000000;tx.to=public_address(public_key(keys[0]));late=SignedTransaction::sign(tx,keys.back());if(hex(late.hash())<after)break;check(n<10000,"find bounded late hash fixture");}
  check(chain.submit(late),"late queued transaction admitted");
  unsigned pages=1;for(;;){auto page=chain.gossip_page(barrier,epoch,after,true);++pages;check(page.at("cursor")==barrier&&page.at("transactions").size()<=50,"inventory preserves first-page barrier and bound");for(const auto& raw:page.at("transactions"))check(seen.insert(hex(SignedTransaction::decode(unhex(raw.get<std::string>())).hash())).second,"duplicate inventory entry");if(page.at("next").is_null())break;auto next=page.at("next").get<std::string>();check(next>after,"inventory cursor advances");after=next;check(pages<=201,"bounded inventory scan");}
  auto tail=chain.gossip_page(barrier,epoch,"",false);check(tail.at("mode")=="events","inventory transitions to event tail");bool found=false;for(const auto& raw:tail.at("transactions"))if(SignedTransaction::decode(unhex(raw.get<std::string>())).hash()==late.hash())found=true;check(found,"insert behind hash cursor recovered through original event barrier");
  // Eviction can remove an account's next nonce, leaving a queued suffix.
  // Inventory must cover both partitions, not just executable pending entries.
  auto current=chain.pool_info(true),status=chain.pool_info(false);size_t covered=0;std::set<std::string> current_hashes;
  for(const auto* category:{"pending","queued"}){size_t count=0;for(const auto& [sender,nonces]:current.at(category).items())for(const auto& [nonce,tx]:nonces.items()){
   auto hash=tx.at("hash").get<std::string>();check(current_hashes.insert(hash).second,"pool hash appears in both partitions");
   check(seen.contains(hash)||hash==hex(late.hash()),"current pool entry omitted by inventory and event tail");++covered;++count;
  }check(count==status.at(category).get<size_t>(),"pool partition count agrees with status");}
  check(covered==10000,"fixture retains full bounded pool");
  check(status.at("queued").get<size_t>()>0,"evicted nonce produces queued suffix covered by recovery");
  std::cout<<Json{{"scenario","gossip-journal-expiry"},{"events",chain.event_cursor()},{"inventoryPages",pages},{"inventoryEntries",seen.size()},{"coveredPoolEntries",covered},{"pending",status.at("pending")},{"queued",status.at("queued")},{"lateInsertRecovered",found}}.dump()<<std::endl;
 }
 {Blockchain reopened(root/"node",genesis);auto page=reopened.gossip_page(barrier,epoch,"0x"+std::string(64,'f'),true);check(page.at("mode")=="inventory"&&page.at("epoch")!=epoch&&page.at("cursor")==0&&page.at("transactions").empty(),"reopened core resets stale session and page cursor");}
 std::cout<<"Expired journal, pagination barrier and reopened session checks passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
