#include "support/process.hpp"
#include "nano/committee.hpp"
#include <openssl/sha.h>
#include <set>
using namespace test;
// Independent scalar formulation from the Ethereum swap-or-not specification.
static uint64_t reference(uint64_t index,uint64_t count,const std::array<uint8_t,32>& seed,unsigned rounds){
 for(unsigned r=0;r<rounds;++r){Bytes input(seed.begin(),seed.end());input.push_back(r);Bytes hash(32);SHA256(input.data(),input.size(),hash.data());
  uint64_t p=0;for(int i=7;i>=0;--i)p=(p<<8)|hash[i];p%=count;uint64_t flip=(p+count-index)%count,pos=std::max(index,flip);
  for(unsigned i=0;i<4;++i)input.push_back((pos/256)>>(8*i));SHA256(input.data(),input.size(),hash.data());if((hash[(pos%256)/8]>>(pos%8))&1)index=flip;
 }return index;
}
int main(){try{
 auto fixture=load(std::string(NANO_TEST_SOURCE)+"/tests/fixtures/consensus-shuffling.json");unsigned cases=0,checks=0;
 for(const auto& item:fixture.at("cases")){auto raw=unhex(item.at("seed"));check(raw.size()==32,"fixture seed length");std::array<uint8_t,32> seed{};std::copy(raw.begin(),raw.end(),seed.begin());auto count=item.at("count").get<uint64_t>();
  auto actual=shuffled_validator_indices(count,seed,fixture.at("rounds").get<unsigned>());check(Json(actual)==item.at("mapping"),"official Ethereum permutation mismatch");++cases;
  for(uint64_t i=0;i<count;++i)check(shuffled_validator_index(i,count,seed,fixture.at("rounds").get<unsigned>())==actual[i],"scalar official shuffle mismatch");
  for(auto committees:{1u,3u,32u,128u}){std::vector<uint64_t> joined;for(unsigned i=0;i<committees;++i){auto c=partition_committee(actual,i,committees);joined.insert(joined.end(),c.begin(),c.end());}check(joined==actual,"partition loses or duplicates indices");++checks;}
 }
 for(uint64_t count:{255ull,256ull,257ull,1001ull,100000ull,1000000ull}){
  std::array<uint8_t,32> seed{};seed[0]=0x5a;seed[31]=0xa5;auto started=std::chrono::steady_clock::now();auto mapping=shuffled_validator_indices(count,seed,90);
  std::vector<bool> seen(count);for(auto index:mapping){check(index<count&&!seen[index],"permutation must contain every index exactly once");seen[index]=true;}
  for(uint64_t index:{0ull,1ull,127ull,254ull}){check(mapping[index]==reference(index,count,seed,90),"scalar differential mismatch");++checks;}
  check(mapping.back()==reference(count-1,count,seed,90),"last scalar differential mismatch");++checks;
  std::vector<uint64_t> joined;for(unsigned i=0;i<2048;++i){auto committee=partition_committee(mapping,i,2048);joined.insert(joined.end(),committee.begin(),committee.end());}check(joined==mapping,"all validators assigned exactly once across 2048 committees");
  auto changed=seed;changed[0]^=1;check(shuffled_validator_indices(count,changed,90)!=mapping,"seed change alters assignment");
  check(shuffled_validator_indices(count,seed,10)!=mapping,"preset rounds affect assignment");
  std::cout<<Json{{"scenario","committee-partition"},{"validators",count},{"committees",2048},{"milliseconds",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count()},{"allAssignedOnce",true},{"scalarDifferential",true}}.dump()<<std::endl;
 }
 unsigned rejected=0;auto reject=[&](auto f){bool no=false;try{f();}catch(const std::invalid_argument&){no=true;}check(no,"invalid parameters rejected");++rejected;};std::array<uint8_t,32> seed{};
 reject([&]{shuffled_validator_indices(1,seed,0);});reject([&]{shuffled_validator_indices(1,seed,257);});reject([&]{shuffled_validator_indices((1ull<<40)+1,seed,90);});
 std::vector<uint64_t> small{7,9,11};reject([&]{partition_committee(small,0,0);});reject([&]{partition_committee(small,3,3);});
 check(partition_committee(small,UINT64_MAX-1,UINT64_MAX)==std::vector<uint64_t>{11},"large committee count does not overflow boundaries");
 std::cout<<Json{{"scenario","ethereum-committee-primitive"},{"officialCases",cases},{"differentialAndPartitionChecks",checks},{"invalidParametersRejected",rejected},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
