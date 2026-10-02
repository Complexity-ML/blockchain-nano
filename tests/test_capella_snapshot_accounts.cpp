#include "../src/consensus_capella_snapshot_accounts.hpp"
#include "../src/consensus_capella_snapshot_json.hpp"
#include <iostream>
#include <random>
#include <stdexcept>
using namespace nano;
int main(){try{
 auto check=[](bool ok){if(!ok)throw std::runtime_error("snapshot account comparison differs from serialization");};
 std::mt19937_64 random(0xbee20261002);size_t comparisons=0,cancellations=0,row_checks=0;
 for(unsigned sample=0;sample<64;++sample){
  State original;
  for(unsigned i=0;i<1+sample%9;++i){const auto addr=hex(bytes(Int(i+1),20));auto& a=original.get(addr);
   a.nonce=Int(std::to_string(random()));a.balance=(Int(1)<<255)+random();
   a.code.resize((sample+i)%3==0?8193:(sample+i)%35);for(auto& b:a.code)b=static_cast<uint8_t>(random());
   for(unsigned slot=0;slot<1+i%5;++slot)a.storage[(Int(1)<<200)+slot]=slot?Int(std::to_string(random())):Int(0);
  }
  const auto cached=original.to_json();
  auto oracle=[&](const State& state){check(snapshot_detail::validated_accounts_match(state,cached)==(state.to_json()==cached));++comparisons;};
  oracle(original);
  const auto addr=hex(bytes(Int(1),20));
  for(unsigned mutation=0;mutation<9;++mutation){State changed=original;auto& a=changed.get(addr);
   switch(mutation){
    case 0:++a.nonce;break;case 1:++a.balance;break;
    case 2:if(a.code.empty())a.code.push_back(1);else a.code.back()^=1;break;
    case 3:a.code.push_back(0);break;
    case 4:++a.storage.begin()->second;break;
    case 5:a.storage.erase(a.storage.begin());break;
    case 6:a.storage[Int(7)]=0;break; // Equal trie roots do not grant a hit.
    case 7:changed.erase(addr);break;
    case 8:{auto copy=a;changed.erase(addr);changed.get(hex(bytes(Int(100),20)))=std::move(copy);break;}
   }
   oracle(changed);
  }
  State added=original;added.get(hex(bytes(Int(200),20)));oracle(added); // Include empty accounts.
  auto saved=original.snapshot();original.get(addr).balance+=1;oracle(original);original.revert(saved);oracle(original);
  bool canceled=false;unsigned callbacks=0;try{snapshot_detail::validated_accounts_match(original,cached,[&]{if(++callbacks==2)throw std::runtime_error("cancel");});}catch(const std::runtime_error&){canceled=true;}check(canceled&&callbacks==2);++cancellations;
  for(auto bad:{Json::array(),Json(nullptr),Json::object()}){check(!snapshot_detail::validated_accounts_match(original,bad));++comparisons;}
  for(const auto* name:{"nonce","balance","code","codeHash","storageRoot","storage"}){auto bad=cached;bad[addr].erase(name);check(!snapshot_detail::validated_accounts_match(original,bad));++comparisons;}
  Json full=Json::array({"root",Json::array({"header"}),Json::array({"0x00",nullptr}),"realized","unrealized",Json::array({Json{{"block",sample}},cached,Json::array({sample}),Json{{"deposits",sample}}})});
  auto deferred=full;deferred[2][0]=nullptr;deferred[5][1]=nullptr;
  check(snapshot_detail::same_row_after_ssz_match(full,deferred,true));++row_checks;
  check(!snapshot_detail::same_row_after_ssz_match(full,deferred));++row_checks;
  for(unsigned part:{0u,2u,3u}){auto changed=deferred;changed[5][part]=Json{{"changed",part}};check(!snapshot_detail::same_row_after_ssz_match(full,changed,true));++row_checks;}
  auto changed=deferred;changed[5].push_back(0);check(!snapshot_detail::same_row_after_ssz_match(full,changed,true));++row_checks;
 }
 std::cout<<Json{{"primitiveOracleComparisons",comparisons},{"cancellations",cancellations},{"deferredRowChecks",row_checks},{"passed",true},{"scope","private validated cache primitive comparison; no peer authentication or whole-node qualification"}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
