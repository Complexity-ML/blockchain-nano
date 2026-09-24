#include "support/phase0_ffg_ssz.hpp"
#include "nano/consensus_epoch_maintenance.hpp"
using namespace test;
using namespace test::phase0_minimal;
std::vector<ConsensusRoot> roots(const Bytes& b,size_t start,size_t end){
 check(start<=end&&end<=b.size()&&(end-start)%32==0,"root vector bounds");
 std::vector<ConsensusRoot> out;for(size_t i=start;i<end;i+=32)out.push_back(root_at(b,i));return out;
}
std::vector<ConsensusRoot> history(const Bytes& b){return roots(b,little(b,4272,4),little(b,4348,4));}
std::vector<Phase0Eth1Data> votes(const Bytes& b){
 const auto start=little(b,4348,4),end=little(b,4360,4);check(start<=end&&end<=b.size()&&(end-start)%72==0,"Eth1 bounds");
 std::vector<Phase0Eth1Data> out;for(size_t i=start;i<end;i+=72)out.push_back({root_at(b,i),little(b,i+32,8),root_at(b,i+40)});return out;
}
int main(int argc,char** argv){try{
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures";
 const auto root=argc==2?fs::path(argv[1]):fixtures;
 const auto refs=fixtures/"consensus-slashing-official";const auto manifest_refs=load(refs/"manifest.json");
 for(const auto& [name,info]:manifest_refs.at("referenceFiles").items())check(hex(sha256(binary(refs/name))).substr(2)==info.at("sha256").get<std::string>(),"config digest");
 const auto preset=read(refs/"minimal-phase0.yaml");auto p=[&](const char* k){return parameter(preset,k);};
 const Phase0HistoryRules hr{p("SLOTS_PER_EPOCH"),p("SLOTS_PER_HISTORICAL_ROOT"),p("HISTORICAL_ROOTS_LIMIT")};
 const ConsensusRandaoRules rr{hr.slots_per_epoch,p("MIN_SEED_LOOKAHEAD")};
 unsigned cases=0;size_t cancelled=0;Bytes historical_pre;
 for(const auto kind:{"eth1-data-reset","historical-roots-update","participation-record-updates","randao-mixes-reset","slashings-reset"}){
  const auto folder=root/(std::string("consensus-")+kind+"-official");const auto m=load(folder/"manifest.json");check(m.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908","revision");
  check(m.at("cases").size()==(std::string(kind)=="eth1-data-reset"?2:1),"case coverage");
  for(const auto& row:m.at("cases")){
   check(row.at("valid")==true,"unexpected official rejection");
   for(const auto& [path,info]:row.at("files").items()){const auto b=binary(folder/path);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");}
   const auto name=row.at("name").get<std::string>();const auto pre=binary(folder/name/"pre.ssz"),post=binary(folder/name/"post.ssz");const auto epoch=little(pre,40,8)/hr.slots_per_epoch;
   check(little(pre,40,8)==little(post,40,8),"component advanced slot");
   if(std::string(kind)=="eth1-data-reset")check(phase0_eth1_votes_after_reset(epoch,votes(pre),p("EPOCHS_PER_ETH1_VOTING_PERIOD"))==votes(post),"official Eth1 mismatch");
   if(std::string(kind)=="slashings-reset"){
    const auto s=decode(pre),before=s;auto want=s;want.slashings=phase0_slashings_after_reset(epoch,s.slashings,p("EPOCHS_PER_SLASHINGS_VECTOR"));check(want==decode(post)&&s==before,"official slashings reset mismatch");
   }
   if(std::string(kind)=="randao-mixes-reset"){
    const auto s=ffg_randao(pre),before=s;check(phase0_randao_after_reset(s,rr)==ffg_randao(post)&&s==before,"official RANDAO reset mismatch");
   }
   if(std::string(kind)=="participation-record-updates"){
    const auto s=ffg_state(pre),before=s;check(phase0_participation_after_rotation(s)==ffg_state(post)&&s==before,"official participation mismatch");
   }
   if(std::string(kind)=="historical-roots-update"){
    historical_pre=pre;const auto blocks=roots(pre,176,2224),states=roots(pre,2224,4272),old=history(pre);
    check(phase0_history_after_update(epoch,blocks,states,old,hr)==history(post),"official historical root mismatch");
    size_t count=0;phase0_history_after_update(epoch,blocks,states,old,hr,[&]{++count;});
    for(size_t stop=1;stop<=count;++stop){size_t n=0;bool threw=false;try{phase0_history_after_update(epoch,blocks,states,old,hr,[&]{if(++n==stop)throw std::runtime_error("cancelled");});}catch(const std::runtime_error& e){threw=std::string(e.what())=="cancelled";}check(threw&&old==history(pre),"history cancellation");++cancelled;}
   }
   ++cases;
  }
 }
 // Independent boundary checks: no reset early, exact periodic reset, ring wrap.
 unsigned boundaries=0,rejections=0;
 const std::vector<Phase0Eth1Data> v{{{},17,{}}};check(phase0_eth1_votes_after_reset(2,v,4)==v&&phase0_eth1_votes_after_reset(3,v,4).empty()&&phase0_eth1_votes_after_reset(7,v,4).empty(),"Eth1 periodicity");boundaries+=3;
 const std::vector<uint64_t> ledger{11,22,33,44};check(phase0_slashings_after_reset(3,ledger,4)==std::vector<uint64_t>({0,22,33,44}),"slashing wrap");++boundaries;
 ConsensusRandaoState randao;randao.epoch=3;randao.mixes.resize(4);for(size_t i=0;i<4;++i)randao.mixes[i][0]=uint8_t(i+1);randao.last_reveal_slot=31;auto r=phase0_randao_after_reset(randao,{8,1});auto want=randao;want.mixes[0]=randao.mixes[3];check(r==want,"RANDAO wrap/metadata");++boundaries;
 const auto blocks=roots(historical_pre,176,2224),states=roots(historical_pre,2224,4272),old=history(historical_pre);
 check(phase0_history_after_update(0,blocks,states,old,hr)==old,"early history update");++boundaries;
 const std::vector<ConsensusRoot> z(64);auto zero=ConsensusRoot{};for(unsigned i=0;i<7;++i)zero=consensus_signing_root(zero,zero);check(phase0_history_after_update(7,z,z,{},hr)==std::vector<ConsensusRoot>{zero},"zero tree vector shape");++boundaries;
 auto refuse=[&](auto fn){bool threw=false;try{fn();}catch(const std::exception&){threw=true;}check(threw,"invalid maintenance accepted");++rejections;};
 refuse([&]{phase0_eth1_votes_after_reset(0,v,0);});refuse([&]{phase0_eth1_votes_after_reset(UINT64_MAX,v,4);});
 refuse([&]{phase0_slashings_after_reset(0,ledger,0);});refuse([&]{phase0_slashings_after_reset(0,ledger,5);});refuse([&]{phase0_slashings_after_reset(UINT64_MAX,ledger,4);});
 auto bad_randao=randao;bad_randao.epoch=UINT64_MAX;refuse([&]{phase0_randao_after_reset(bad_randao,{8,1});});refuse([&]{phase0_randao_after_reset(randao,{0,1});});
 refuse([&]{phase0_history_after_update(UINT64_MAX,blocks,states,old,hr);});auto bad=hr;bad.slots_per_epoch=0;refuse([&]{phase0_history_after_update(7,blocks,states,old,bad);});bad=hr;bad.slots_per_historical_root=63;refuse([&]{phase0_history_after_update(7,blocks,states,old,bad);});
 bad=hr;bad.historical_roots_limit=1;std::vector<ConsensusRoot> full(1);refuse([&]{phase0_history_after_update(7,blocks,states,full,bad);});refuse([&]{phase0_history_after_update(7,{},states,old,hr);});
 check(cases==6,"six official cases");
 std::cout<<Json{{"scenario","official-phase0-epoch-maintenance"},{"cases",cases},{"components",5},{"boundaries",boundaries},{"rejections",rejections},{"cancellations",cancelled},{"fullEpochTransition",false},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
