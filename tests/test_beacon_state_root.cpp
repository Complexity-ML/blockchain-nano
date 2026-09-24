#include "support/phase0_minimal_ssz.hpp"
#include "nano/consensus_beacon_state.hpp"
using namespace test;
using namespace test::phase0_minimal;
int main(int argc,char** argv){try{
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures";
 const auto root=argc==2?fs::path(argv[1]):fixtures/"consensus-state-roots-official";
 const auto m=load(root/"manifest.json");check(m.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908"&&m.at("preset")=="minimal"&&m.at("cases").size()==123,"state root corpus");
 const auto refs=fixtures/"consensus-slashing-official";const auto rm=load(refs/"manifest.json");for(const auto& [name,info]:rm.at("referenceFiles").items())check(hex(sha256(binary(refs/name))).substr(2)==info.at("sha256").get<std::string>(),"preset digest");
 const auto preset=read(refs/"minimal-phase0.yaml");auto p=[&](const char* k){return parameter(preset,k);};
 const Phase0StateLimits limits{p("SLOTS_PER_HISTORICAL_ROOT"),p("EPOCHS_PER_HISTORICAL_VECTOR"),p("EPOCHS_PER_SLASHINGS_VECTOR"),p("HISTORICAL_ROOTS_LIMIT"),p("EPOCHS_PER_ETH1_VOTING_PERIOD")*p("SLOTS_PER_EPOCH"),p("VALIDATOR_REGISTRY_LIMIT"),p("MAX_ATTESTATIONS")*p("SLOTS_PER_EPOCH"),p("MAX_VALIDATORS_PER_COMMITTEE")};
 unsigned cases=0,rejected=0,mutations=0;size_t decoded=0;std::map<std::string,unsigned> groups;
 Bytes example;
 for(const auto& row:m.at("cases")){
  const auto name=row.at("name").get<std::string>();for(const auto& [path,info]:row.at("files").items()){const auto b=binary(root/path);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");}
  const auto b=binary(root/name/"serialized.ssz");const auto yaml=read(root/name/"roots.yaml");const auto at=yaml.find("0x");check(at!=std::string::npos,"missing expected root");const auto expected=yaml.substr(at,66);
  const auto got=phase0_beacon_state_root(b,limits);check(hex(Bytes(got.begin(),got.end()))==expected,"official BeaconState root mismatch: "+name);++cases;decoded+=b.size();++groups[name.substr(0,name.find('/'))];
  if(name=="ssz_lengthy/case_0")example=b;
 }
 check(groups==std::map<std::string,unsigned>{{"ssz_lengthy",30},{"ssz_max",1},{"ssz_nil",1},{"ssz_one",30},{"ssz_random",30},{"ssz_random_chaos",30},{"ssz_zero",1}},"SSZ group coverage");
 const auto original=example;const auto baseline=phase0_beacon_state_root(example,limits);
 auto refuse=[&](auto b,auto l){const auto old=b;bool failed=false;try{phase0_beacon_state_root(b,l);}catch(const std::invalid_argument&){failed=true;}check(failed&&b==old,"malformed state SSZ accepted");++rejected;};
 for(size_t offset:{4272,4348,4360,4364,6928,6932}){auto b=example;std::fill_n(b.begin()+offset,4,0);refuse(b,limits);b=example;std::fill_n(b.begin()+offset,4,255);refuse(b,limits);}
 auto b=example;b[6936]=16;refuse(b,limits);
 const auto validators=little(example,4360,4),balances=little(example,4364,4);check(balances>validators,"need validator record");b=example;b[validators+88]=2;refuse(b,limits);
 for(size_t offset:{size_t(0),size_t(8),size_t(40),size_t(48),size_t(64),size_t(176),size_t(2224),size_t(4276),size_t(4352),size_t(4368),size_t(6416),size_t(6936),size_t(6937),size_t(6977),size_t(7017),size_t(validators),size_t(balances)}){b=example;b[offset]^=1;check(phase0_beacon_state_root(b,limits)!=baseline,"state field mutation not committed");++mutations;}
 for(size_t cut:{size_t(0),size_t(7),size_t(39),size_t(47),size_t(63),size_t(175),size_t(4271),size_t(4347),size_t(4367),size_t(6927),size_t(6935),size_t(7056),example.size()-1})refuse(Bytes(example.begin(),example.begin()+cut),limits);
 auto l=limits;l.validator_registry_limit=0;refuse(example,l);l=limits;l.slots_per_historical_root=UINT64_MAX;refuse(example,l);l=limits;l.epochs_per_historical_vector=UINT64_MAX;refuse(example,l);l=limits;l.epochs_per_slashings_vector=UINT64_MAX;refuse(example,l);
 const auto previous=little(example,6928,4),current=little(example,6932,4);check(current>previous,"need pending record");b=example;std::fill_n(b.begin()+previous,4,0);refuse(b,limits);const auto first=previous+little(example,previous,4);b=example;b[first]=147;refuse(b,limits);
 const auto historical=little(example,4272,4),eth1votes=little(example,4348,4);check(eth1votes>historical&&validators>eth1votes,"need variable history/votes");
 for(size_t offset:{size_t(historical),size_t(eth1votes),size_t(first+4),size_t(current+little(example,current,4)+4)}){b=example;b[offset]^=1;check(phase0_beacon_state_root(b,limits)!=baseline,"variable field mutation not committed");++mutations;}
 // Select every cancellation point on the sparse empty-state tree; validation
 // does not allocate the 2^40 validator maximum even for empty lists.
 const auto empty=binary(root/"ssz_nil/case_0/serialized.ssz");size_t calls=0;phase0_beacon_state_root(empty,limits,[&]{++calls;});
 for(size_t stop=1;stop<=calls;++stop){size_t n=0;bool aborted=false;try{phase0_beacon_state_root(empty,limits,[&]{if(++n==stop)throw std::runtime_error("cancelled");});}catch(const std::runtime_error& e){aborted=std::string(e.what())=="cancelled";}check(aborted&&empty==binary(root/"ssz_nil/case_0/serialized.ssz")&&example==original,"state hash cancellation");}
 std::cout<<Json{{"scenario","official-phase0-beacon-state-root"},{"cases",cases},{"groups",groups.size()},{"decodedBytes",decoded},{"fieldMutations",mutations},{"malformedRejections",rejected},{"cancellations",calls},{"fullStateTransition",false},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
