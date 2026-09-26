#include "support/phase0_minimal_ssz.hpp"
#include "nano/consensus_altair_epoch.hpp"
using namespace test;using namespace test::phase0_minimal;
int main(){try{
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures",root=fixtures/"consensus-altair-ffg-official",refs=fixtures/"consensus-slashing-official",altair=fixtures/"consensus-altair-state-roots-official";
 const auto preset=read(refs/"minimal-phase0.yaml");auto p=[&](const char* k){return parameter(preset,k);};const auto rm=load(refs/"manifest.json"),am=load(altair/"manifest.json");check(hex(sha256(binary(refs/"minimal-phase0.yaml"))).substr(2)==rm.at("referenceFiles").at("minimal-phase0.yaml").at("sha256").get<std::string>(),"FFG rule digest");check(hex(sha256(binary(altair/"minimal-altair.yaml"))).substr(2)==am.at("referenceFiles").at("minimal-altair.yaml").at("sha256").get<std::string>(),"Altair preset digest");
 const AltairStateLimits l{p("SLOTS_PER_HISTORICAL_ROOT"),p("EPOCHS_PER_HISTORICAL_VECTOR"),p("EPOCHS_PER_SLASHINGS_VECTOR"),p("HISTORICAL_ROOTS_LIMIT"),p("EPOCHS_PER_ETH1_VOTING_PERIOD")*p("SLOTS_PER_EPOCH"),p("VALIDATOR_REGISTRY_LIMIT"),parameter(read(altair/"minimal-altair.yaml"),"SYNC_COMMITTEE_SIZE")};const AltairFfgRules r{p("SLOTS_PER_EPOCH"),p("EFFECTIVE_BALANCE_INCREMENT")};
 const auto m=load(root/"manifest.json");check(m.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908"&&m.at("cases").size()==10,"Altair FFG corpus");Bytes example;unsigned changed=0;
 for(const auto& row:m.at("cases")){const auto name=row.at("name").get<std::string>();check(row.at("valid")==true,"invalid FFG fixture");for(const auto& [path,info]:row.at("files").items())check(hex(sha256(binary(root/path))).substr(2)==info.at("sha256").get<std::string>(),"FFG fixture digest");const auto b=binary(root/name/"pre.ssz"),post=binary(root/name/"post.ssz");check(prepare_altair_justification_finalization(b,l,r)==post,"official FFG mismatch: "+name);if(b!=post)++changed;if(name=="123_ok_support")example=b;}
 auto put=[](Bytes& b,size_t at,uint64_t n){for(size_t j=0;j<8;++j)b[at+j]=uint8_t(n>>(8*j));};auto local=example;const auto validators=little(local,4360,4),balances=little(local,4364,4),prev=little(local,6928,4),curr=little(local,6932,4),count=(balances-validators)/121;
 check(count>=3,"threshold registry");for(size_t i=0;i<count;++i){const auto at=validators+121*i;put(local,at+97,i<3?0:UINT64_MAX);put(local,at+105,UINT64_MAX);put(local,at+113,UINT64_MAX);put(local,at+80,r.effective_balance_increment);local[at+88]=0;local[prev+i]=i<2?2:0;local[curr+i]=0;}local[6936]=0;std::fill(local.begin()+6937,local.begin()+7057,0);
 const auto epoch=little(local,40,8)/r.slots_per_epoch;check(epoch>1,"threshold epoch");auto exact=prepare_altair_justification_finalization(local,l,r);check(little(exact,6977,8)==epoch-1,"exact two-thirds accepted");
 // A delayed previous-epoch quorum can finalize an older checkpoint with no
 // current-epoch votes. A network partition must prohibit conflicting/new
 // cut-era finality, rather than every delayed advance of the common prefix.
 check(epoch>2,"delayed-finality epoch");
 auto delayed=local;delayed[6936]=2;
 for(size_t at:{size_t(6937),size_t(6977)}){
  put(delayed,at,epoch-2);
  const auto history=176+32*(((epoch-2)*r.slots_per_epoch)%l.slots_per_historical_root);
  std::copy_n(local.begin()+history,32,delayed.begin()+at+8);
 }
 const auto late=prepare_altair_justification_finalization(delayed,l,r);
 check(little(late,7017,8)==epoch-2,"previous quorum finalizes older checkpoint without current votes");
 check(std::equal(late.begin()+7025,late.begin()+7057,delayed.begin()+6945),"delayed finality preserves checkpoint root");
 auto missing=delayed;missing[prev+1]=0;
 check(little(prepare_altair_justification_finalization(missing,l,r),7017,8)==0,"delayed finality requires previous quorum");
 // Synthetic one-wei boundary tests arithmetic independently of balance rounding.
 auto below=local;put(below,validators+121+80,r.effective_balance_increment-1);check(little(prepare_altair_justification_finalization(below,l,r),6977,8)==0,"below two-thirds rejected");auto slashed=local;slashed[validators+88]=1;check(little(prepare_altair_justification_finalization(slashed,l,r),6977,8)==0,"slashed vote excluded but denominator retained");
 for(uint64_t e:{0,1}){auto early=local;put(early,40,e*r.slots_per_epoch);check(prepare_altair_justification_finalization(early,l,r)==early,"first two epochs skipped");}
 unsigned rejected=0;auto refuse=[&](const Bytes& b,const AltairFfgRules& cfg){bool threw=false;try{prepare_altair_justification_finalization(b,l,cfg);}catch(const std::exception&){threw=true;}check(threw,"bad FFG accepted");++rejected;};auto bad=local;bad.resize(bad.size()-8);refuse(bad,r);bad=local;put(bad,6977,epoch+1);refuse(bad,r);auto cfg=r;cfg.slots_per_epoch=0;refuse(local,cfg);cfg=r;cfg.effective_balance_increment=0;refuse(local,cfg);
 size_t calls=0;prepare_altair_justification_finalization(example,l,r,[&]{++calls;});std::set<size_t> stops{1,2,calls/4,calls/2,3*calls/4,calls-1,calls};for(auto stop:stops){size_t n=0;bool caught=false;try{prepare_altair_justification_finalization(example,l,r,[&]{if(++n==stop)throw std::runtime_error("cancel");});}catch(const std::runtime_error& e){caught=std::string(e.what())=="cancel";}check(caught,"FFG cancellation swallowed");}
 std::cout<<Json{{"scenario","official-altair-ffg"},{"cases",10},{"changedStates",changed},{"thresholdChecks",3},{"delayedFinalityChecks",3},{"rejections",rejected},{"cancellations",stops.size()},{"fullEpoch",false},{"networkActive",false}}.dump()<<'\n';
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
