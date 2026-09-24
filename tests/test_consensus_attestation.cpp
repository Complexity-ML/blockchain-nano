#include "support/process.hpp"
#include "nano/consensus_attestation.hpp"
using namespace test;
ConsensusRoot root(const Json& v){auto b=unhex(v.get<std::string>());check(b.size()==32,"root width");ConsensusRoot r{};std::copy(b.begin(),b.end(),r.begin());return r;}
int main(){try{
 auto fixture=load(std::string(NANO_TEST_SOURCE)+"/tests/fixtures/consensus-attestation.json");unsigned matched=0,mutations=0;
 for(const auto& row:fixture.at("cases")){
  const auto& v=row.at("value");ConsensusAttestationData d{std::stoull(v.at("slot").get<std::string>()),std::stoull(v.at("index").get<std::string>()),root(v.at("beacon_block_root")),{std::stoull(v.at("source").at("epoch").get<std::string>()),root(v.at("source").at("root"))},{std::stoull(v.at("target").at("epoch").get<std::string>()),root(v.at("target").at("root"))}};
  auto expected=root(row.at("root"));check(consensus_attestation_root(d)==expected,"official attestation SSZ root");++matched;
  for(unsigned field=0;field<7;++field){auto changed=d;switch(field){case 0:changed.slot^=1;break;case 1:changed.index^=1;break;case 2:changed.beacon_block_root[0]^=1;break;case 3:changed.source.epoch^=1;break;case 4:changed.source.root[0]^=1;break;case 5:changed.target.epoch^=1;break;case 6:changed.target.root[0]^=1;break;}check(consensus_attestation_root(changed)!=expected,"attestation field not committed");++mutations;}
 }
 check(matched==5,"selected official corpus");std::cout<<Json{{"scenario","consensus-attestation-root"},{"officialCases",matched},{"fieldMutations",mutations},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
