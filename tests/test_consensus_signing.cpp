#include "support/process.hpp"
#include "nano/consensus_signing.hpp"
using namespace test;
template<size_t N> std::array<uint8_t,N> fixed(const Json& value){auto bytes=unhex(value.get<std::string>());check(bytes.size()==N,"fixture width");std::array<uint8_t,N> out{};std::copy(bytes.begin(),bytes.end(),out.begin());return out;}
int main(){try{
 auto fixture=load(std::string(NANO_TEST_SOURCE)+"/tests/fixtures/consensus-signing.json");unsigned forks=0,signings=0;
 for(const auto& c:fixture.at("cases")){
  auto expected=fixed<32>(c.at("root"));
  if(c.at("type")=="ForkData"){
   auto version=fixed<4>(c.at("current_version"));auto genesis=fixed<32>(c.at("genesis_validators_root"));
   check(consensus_fork_data_root(version,genesis)==expected,"official ForkData root");
   ConsensusBytes4 type{1,2,3,4};auto domain=consensus_domain(type,version,genesis);
   check(std::equal(type.begin(),type.end(),domain.begin())&&std::equal(expected.begin(),expected.begin()+28,domain.begin()+4),"domain prefix and fork commitment");
   auto changed=version;changed[0]^=1;check(domain!=consensus_domain(type,changed,genesis),"fork separation");
   genesis[31]^=1;check(domain!=consensus_domain(type,version,genesis),"chain separation");++forks;
  }else{
   auto object=fixed<32>(c.at("object_root")),domain=fixed<32>(c.at("domain"));
   check(consensus_signing_root(object,domain)==expected,"official SigningData root");
   auto changed=object;changed[31]^=1;check(consensus_signing_root(changed,domain)!=expected,"object binding");
   domain[0]^=1;check(consensus_signing_root(object,domain)!=expected,"duty domain binding");++signings;
  }
 }
 check(forks==5&&signings==5,"complete selected official corpus");
 std::cout<<Json{{"scenario","consensus-signing-roots"},{"forkDataCases",forks},{"signingDataCases",signings},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
