#include "support/phase0_minimal_ssz.hpp"
#include "nano/consensus_altair_epoch.hpp"
using namespace test;using namespace test::phase0_minimal;
int main(){try{
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures",root=fixtures/"consensus-altair-inactivity-official",refs=fixtures/"consensus-slashing-official";
 const auto preset=read(refs/"minimal-phase0.yaml"),config=read(refs/"minimal-config.yaml");auto p=[&](const char* key){return parameter(preset,key);};
 const auto reference=load(refs/"manifest.json");for(const char* file:{"minimal-phase0.yaml","minimal-config.yaml"})check(hex(sha256(binary(refs/file))).substr(2)==reference.at("referenceFiles").at(file).at("sha256").get<std::string>(),"inactivity rule digest");
 const auto altair=fixtures/"consensus-altair-state-roots-official";const auto am=load(altair/"manifest.json");check(hex(sha256(binary(altair/"minimal-altair.yaml"))).substr(2)==am.at("referenceFiles").at("minimal-altair.yaml").at("sha256").get<std::string>(),"Altair preset digest");
 const AltairStateLimits l{p("SLOTS_PER_HISTORICAL_ROOT"),p("EPOCHS_PER_HISTORICAL_VECTOR"),p("EPOCHS_PER_SLASHINGS_VECTOR"),p("HISTORICAL_ROOTS_LIMIT"),p("EPOCHS_PER_ETH1_VOTING_PERIOD")*p("SLOTS_PER_EPOCH"),p("VALIDATOR_REGISTRY_LIMIT"),parameter(read(altair/"minimal-altair.yaml"),"SYNC_COMMITTEE_SIZE")};
 const AltairInactivityRules r{p("SLOTS_PER_EPOCH"),parameter(config,"INACTIVITY_SCORE_BIAS"),parameter(config,"INACTIVITY_SCORE_RECOVERY_RATE"),p("MIN_EPOCHS_TO_INACTIVITY_PENALTY")};
 const auto m=load(root/"manifest.json");check(m.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908"&&m.at("cases").size()==21,"inactivity corpus");Bytes example;size_t scores=0,changed=0;
 for(const auto& row:m.at("cases")){const auto name=row.at("name").get<std::string>();check(row.at("valid")==true,"inactivity invalid fixture");for(const auto& [path,info]:row.at("files").items()){auto b=binary(root/path);check(hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"inactivity fixture digest");}
  const auto b=binary(root/name/"pre.ssz"),post=binary(root/name/"post.ssz");const auto got=prepare_altair_inactivity_updates(b,l,r);check(got==post,"official inactivity post mismatch: "+name);
  const auto at=little(b,7057,4);for(size_t j=at;j<b.size();j+=8){++scores;if(little(b,j,8)!=little(post,j,8))++changed;}if(name=="all_zero_inactivity_scores_empty_participation_leaking")example=b;
 }
 auto put=[](Bytes& b,size_t at,uint64_t v){for(size_t j=0;j<8;++j)b[at+j]=uint8_t(v>>(8*j));};
 unsigned rejected=0;auto refuse=[&](const Bytes& b,const AltairInactivityRules& cfg){bool threw=false;try{prepare_altair_inactivity_updates(b,l,cfg);}catch(const std::exception&){threw=true;}check(threw,"bad inactivity update accepted");++rejected;};
 const auto score_at=little(example,7057,4);auto bad=example;put(bad,score_at,UINT64_MAX);refuse(bad,r);bad=example;put(bad,7017,little(bad,40,8)/8);refuse(bad,r);auto cfg=r;cfg.slots_per_epoch=0;refuse(example,cfg);cfg=r;cfg.score_bias=0;refuse(example,cfg);cfg=r;cfg.score_recovery_rate=0;refuse(example,cfg);
 // Remove one inactivity score: canonical SSZ, incoherent consensus lengths.
 bad=example;bad.resize(bad.size()-8);refuse(bad,r);
 size_t calls=0;prepare_altair_inactivity_updates(example,l,r,[&]{++calls;});std::set<size_t> stops{1,2,calls/4,calls/2,3*calls/4,calls-1,calls};for(auto stop:stops){size_t n=0;bool caught=false;try{prepare_altair_inactivity_updates(example,l,r,[&]{if(++n==stop)throw std::runtime_error("cancel");});}catch(const std::runtime_error& e){caught=std::string(e.what())=="cancel";}check(caught,"cancellation swallowed");}
 std::cout<<Json{{"scenario","official-altair-inactivity"},{"cases",21},{"scores",scores},{"changed",changed},{"rejections",rejected},{"cancellations",stops.size()},{"fullEpoch",false},{"networkActive",false}}.dump()<<'\n';
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
