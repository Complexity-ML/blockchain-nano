#include "support/phase0_epoch_ssz.hpp"
#include "nano/consensus_slots.hpp"
using namespace test;
using namespace test::phase0_minimal;
int main(int argc,char** argv){try{
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures";
 const auto root=argc==2?fs::path(argv[1]):fixtures/"consensus-slots-official";
 const auto rules=epoch_rules(fixtures/"consensus-slashing-official");const auto preset=read(fixtures/"consensus-slashing-official/minimal-phase0.yaml");
 const Phase0StateLimits limits{rules.history.slots_per_historical_root,rules.epochs_per_randao_vector,rules.slashing.epochs_per_slashings_vector,rules.history.historical_roots_limit,rules.epochs_per_eth1_voting_period*rules.history.slots_per_epoch,parameter(preset,"VALIDATOR_REGISTRY_LIMIT"),rules.maximum_pending_attestations,parameter(preset,"MAX_VALIDATORS_PER_COMMITTEE")};
 const auto m=load(root/"manifest.json");check(m.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908"&&m.at("cases").size()==7,"official slots coverage");
 unsigned cases=0,replays=0;size_t slots=0,bytes_compared=0;Phase0SlotState largest;
 for(const auto& row:m.at("cases")){
  const auto name=row.at("name").get<std::string>();check(row.at("valid")==true,"unexpected invalid slots case");
  for(const auto& [path,info]:row.at("files").items()){const auto b=binary(root/path);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");}
  const Phase0SlotState input{binary(root/name/"pre.ssz"),std::nullopt};const auto before=input;const auto n=std::stoull(read(root/name/"slots.yaml"));const auto start=little(input.ssz,40,8);check(n<=UINT64_MAX-start,"target overflow");const auto target=start+n;
  const auto result=prepare_phase0_slots(input,rules,limits,target,n);const auto expected=binary(root/name/"post.ssz");
  check(result.ssz==expected,"official full slot state mismatch: "+name);check(input==before,"slot source mutation");
  check(phase0_beacon_state_root(result.ssz,limits)==phase0_beacon_state_root(expected,limits),"official post root mismatch");
  bool rejected=false;try{prepare_phase0_slots(result,rules,limits,target,n);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"same-target replay accepted");++replays;
  // Advancing one slot at a time must serialize exactly the same state and
  // maintain the same epoch marker across caller boundaries.
  auto split=input;for(uint64_t next=start+1;next<=target;++next)split=prepare_phase0_slots(split,rules,limits,next,1);
  check(split==result,"split slot transition mismatch");
  ++cases;slots+=n;bytes_compared+=expected.size();if(name=="historical_accumulator")largest=input;
 }
 const auto before=largest;size_t calls=0;prepare_phase0_slots(largest,rules,limits,64,64,[&]{++calls;});
 std::set<size_t> stops{1,2,calls/4,calls/2,3*calls/4,calls-1,calls};
 for(size_t stop:stops){size_t n=0;bool aborted=false;try{prepare_phase0_slots(largest,rules,limits,64,64,[&]{if(++n==stop)throw std::runtime_error("cancelled");});}catch(const std::runtime_error& e){aborted=std::string(e.what())=="cancelled";}check(aborted&&largest==before,"cancelled slots mutated source");}
 unsigned rejects=0;auto refuse=[&](auto s,auto r,auto l,uint64_t target,uint64_t budget){const auto old=s;bool threw=false;try{prepare_phase0_slots(s,r,l,target,budget);}catch(const std::exception&){threw=true;}check(threw&&s==old,"invalid slot transition accepted");++rejects;};
 refuse(largest,rules,limits,0,64);refuse(largest,rules,limits,64,63);auto bad=largest;bad.last_processed_epoch=0;refuse(bad,rules,limits,1,1);bad=largest;bad.ssz[64]=1;refuse(bad,rules,limits,1,1);bad=largest;bad.ssz.resize(7056);refuse(bad,rules,limits,1,1);
 auto l=limits;l.pending_attestations_limit++;refuse(largest,rules,l,1,1);l=limits;l.eth1_votes_limit++;refuse(largest,rules,l,1,1);l=limits;l.epochs_per_slashings_vector++;refuse(largest,rules,l,1,1);auto r=rules;r.history.slots_per_epoch=0;refuse(largest,r,limits,1,1);
 // A schema-valid state with fewer balances than validators must not enter
 // the native epoch projection, even when no epoch boundary is crossed.
 bad=largest;const auto previous=little(bad.ssz,6928,4),current=little(bad.ssz,6932,4);bad.ssz.erase(bad.ssz.begin()+previous-8,bad.ssz.begin()+previous);
 for(const auto& [at,value]:std::array<std::pair<size_t,uint64_t>,2>{{{6928,previous-8},{6932,current-8}}})for(unsigned i=0;i<4;++i)bad.ssz[at+i]=uint8_t(value>>(8*i));
 phase0_beacon_state_root(bad.ssz,limits);refuse(bad,rules,limits,1,1);
 // Nonempty pending records exercise the native epoch codec and rotation.
 const auto ffolder=fixtures/"consensus-ffg-official";const auto fm=load(ffolder/"manifest.json");for(const auto& row:fm.at("cases"))if(row.at("name")=="123_ok_support")for(const auto& [path,info]:row.at("files").items())check(hex(sha256(binary(ffolder/path))).substr(2)==info.at("sha256").get<std::string>(),"pending fixture digest");
 const Phase0SlotState voting{binary(ffolder/"123_ok_support/pre.ssz"),std::nullopt};const auto next=prepare_phase0_slots(voting,rules,limits,48,1);const auto parsed=epoch_state(next.ssz),old=epoch_state(voting.ssz);
 check(parsed.ffg.previous_attestations==old.ffg.current_attestations&&parsed.ffg.current_attestations.empty()&&parsed.ffg.checkpoints==ffg_state(binary(ffolder/"123_ok_support/post.ssz")).checkpoints,"pending codec rotation mismatch");
 std::cout<<Json{{"scenario","official-phase0-full-slots"},{"cases",cases},{"slots",slots},{"serializedBytesCompared",bytes_compared},{"splitControls",cases},{"replayRejections",replays},{"cancellationPoints",stops.size()},{"localRejections",rejects},{"pendingCodecControls",1},{"historicalAccumulator",true},{"stateRootsVerified",true},{"fullBlockTransition",false},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
