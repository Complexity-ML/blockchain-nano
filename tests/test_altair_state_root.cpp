#include "support/phase0_minimal_ssz.hpp"
#include "nano/consensus_beacon_state.hpp"
using namespace test;
using namespace test::phase0_minimal;
int main(){try{
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures",root=fixtures/"consensus-altair-state-roots-official",refs=fixtures/"consensus-slashing-official";
 const auto m=load(root/"manifest.json");check(m.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908"&&m.at("cases").size()==123&&m.at("fork")=="altair","Altair corpus");
 for(const auto& [path,info]:m.at("referenceFiles").items())check(hex(sha256(binary(root/path))).substr(2)==info.at("sha256").get<std::string>(),"Altair preset digest");
 const auto preset=read(refs/"minimal-phase0.yaml");const auto rm=load(refs/"manifest.json");check(hex(sha256(binary(refs/"minimal-phase0.yaml"))).substr(2)==rm.at("referenceFiles").at("minimal-phase0.yaml").at("sha256").get<std::string>(),"Phase0 preset digest");auto p=[&](const char* k){return parameter(preset,k);};
 const AltairStateLimits limits{p("SLOTS_PER_HISTORICAL_ROOT"),p("EPOCHS_PER_HISTORICAL_VECTOR"),p("EPOCHS_PER_SLASHINGS_VECTOR"),p("HISTORICAL_ROOTS_LIMIT"),p("EPOCHS_PER_ETH1_VOTING_PERIOD")*p("SLOTS_PER_EPOCH"),p("VALIDATOR_REGISTRY_LIMIT"),parameter(read(root/"minimal-altair.yaml"),"SYNC_COMMITTEE_SIZE")};
 Bytes example;size_t decoded=0;std::map<std::string,unsigned> groups;
 for(const auto& row:m.at("cases")){const auto name=row.at("name").get<std::string>();for(const auto& [path,info]:row.at("files").items()){const auto b=binary(root/path);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"Altair fixture digest");}
  const auto b=binary(root/name/"serialized.ssz");const auto yaml=read(root/name/"roots.yaml");const auto at=yaml.find("0x");check(at!=std::string::npos,"expected root");const auto got=altair_beacon_state_root(b,limits);check(hex(Bytes(got.begin(),got.end()))==yaml.substr(at,66),"Altair official root: "+name);decoded+=b.size();++groups[name.substr(0,name.find('/'))];if(name=="ssz_lengthy/case_0")example=b;
 }
 check(groups==std::map<std::string,unsigned>{{"ssz_lengthy",30},{"ssz_max",1},{"ssz_nil",1},{"ssz_one",30},{"ssz_random",30},{"ssz_random_chaos",30},{"ssz_zero",1}},"all Altair groups");
 unsigned rejected=0,mutations=0;const auto baseline=altair_beacon_state_root(example,limits);
 auto refuse=[&](const Bytes& b,const AltairStateLimits& l){bool threw=false;try{altair_beacon_state_root(b,l);}catch(const std::invalid_argument&){threw=true;}check(threw,"malformed Altair accepted");++rejected;};
 for(size_t at:{4272,4348,4360,4364,6928,6932,7057}){auto b=example;std::fill_n(b.begin()+at,4,0);refuse(b,limits);b=example;std::fill_n(b.begin()+at,4,255);refuse(b,limits);}
 auto b=example;b[6936]=16;refuse(b,limits);const auto validators=little(example,4360,4);b=example;b[validators+88]=2;refuse(b,limits);
 for(size_t at:{size_t(7061),size_t(7061+32*48),size_t(8645),size_t(8645+32*48),size_t(little(example,6928,4)),size_t(little(example,6932,4)),size_t(little(example,7057,4))}){b=example;b.at(at)^=1;check(altair_beacon_state_root(b,limits)!=baseline,"Altair field not committed");++mutations;}
 for(size_t end:{size_t(0),size_t(7056),size_t(7060),size_t(8644),size_t(10228),example.size()-1})refuse(Bytes(example.begin(),example.begin()+end),limits);
 auto l=limits;l.sync_committee_size=0;refuse(example,l);l=limits;l.sync_committee_size=UINT64_MAX;refuse(example,l);l=limits;l.validator_registry_limit=0;refuse(example,l);
 const auto empty=binary(root/"ssz_nil/case_0/serialized.ssz");size_t calls=0;altair_beacon_state_root(empty,limits,[&]{++calls;});std::set<size_t> stops{1,2,calls/4,calls/2,3*calls/4,calls-1,calls};for(auto stop:stops){size_t n=0;bool caught=false;try{altair_beacon_state_root(empty,limits,[&]{if(++n==stop)throw std::runtime_error("cancel");});}catch(const std::runtime_error& e){caught=std::string(e.what())=="cancel";}check(caught,"cancellation swallowed");}
 std::cout<<Json{{"scenario","official-altair-beacon-state-root"},{"cases",123},{"groups",groups.size()},{"decodedBytes",decoded},{"mutations",mutations},{"rejections",rejected},{"cancellations",stops.size()},{"fullStateTransition",false},{"networkActive",false}}.dump()<<'\n';
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
