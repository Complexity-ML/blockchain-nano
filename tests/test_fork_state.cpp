#include "support/phase0_transition_rules.hpp"
#include "nano/consensus_fork_state.hpp"
using namespace test;
using namespace test::phase0_minimal;
int main(){try{
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures",corpus=fixtures/"consensus-full-blocks-official";
 const auto rules=transition_rules(fixtures/"consensus-slashing-official");const auto manifest=load(corpus/"manifest.json");
 unsigned cases=0,skipped=0;Phase0SlotState example;ConsensusBeaconHeader example_header;ConsensusCheckpoint example_target;
 for(const auto& row:manifest.at("cases"))if(row.at("valid").get<bool>()){
  const auto dir=corpus/row.at("name").get<std::string>();for(const auto& [path,info]:row.at("files").items())check(hex(sha256(binary(corpus/path))).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");
  Phase0SlotState state{binary(dir/"pre.ssz"),std::nullopt};const auto start_epoch=little(state.ssz,40,8)/8;if(start_epoch)state.last_processed_epoch=start_epoch-1;
  Bytes last;
  for(size_t i=0;i<row.at("blockCount").get<size_t>();++i){last=binary(dir/("blocks_"+std::to_string(i)+".ssz"));auto next=prepare_phase0_state_transition(state,last,rules,4096);check(bool(next),"authenticate official sequence");state=std::move(*next);}
  check(state.ssz==binary(dir/"post.ssz"),"official post-state");
  const ConsensusBeaconHeader h{little(last,100,8),little(last,108,8),root_at(last,116),root_at(last,148),phase0_beacon_body_root(std::span<const uint8_t>(last).subspan(184),rules.body)};
  const auto epoch=(h.slot+7)/8;const ConsensusCheckpoint target{epoch,consensus_beacon_header_root(h)};const auto before=state;
  const auto c=prepare_phase0_fork_state_contexts(state,h,target,rules.epoch,rules.state,8);
  check(state==before&&c.block.block_root==target.root&&c.block.registry==decode(state.ssz),"block registry binding");
  const auto expected=h.slot==epoch*8?state:prepare_phase0_slots(state,rules.epoch,rules.state,epoch*8,8);
  check(c.target_state==expected&&c.target.registry==decode(expected.ssz)&&c.target.randao==ffg_randao(expected.ssz),"checkpoint state projection");
  check(c.target.checkpoint==target&&c.target.genesis_root==root_at(state.ssz,8)&&c.block.genesis_root==c.target.genesis_root,"checkpoint and chain binding");
  check(c.target.fork.epoch==little(state.ssz,56,8)&&c.target.fork.current_version==c.block.fork.current_version,"fork binding");
  ++cases;if(h.slot<epoch*8){++skipped;example=state;example_header=h;example_target=target;}
 }
 check(cases==29&&skipped>0,"valid sequence coverage");
 unsigned rejected=0;auto reject=[&](const Phase0SlotState& state,const ConsensusBeaconHeader& h,const ConsensusCheckpoint& t,uint64_t budget){bool threw=false;try{prepare_phase0_fork_state_contexts(state,h,t,rules.epoch,rules.state,budget);}catch(const std::exception&){threw=true;}check(threw,"invalid context accepted");++rejected;};
 auto h=example_header;h.state_root[0]^=1;auto t=example_target;t.root=consensus_beacon_header_root(h);reject(example,h,t,8);
 t=example_target;t.root[0]^=1;reject(example,example_header,t,8);t=example_target;t.epoch=UINT64_MAX;reject(example,example_header,t,8);t=example_target;t.epoch=0;reject(example,example_header,t,8);reject(example,example_header,example_target,0);
 auto bad=example;bad.ssz[8]^=1;reject(bad,example_header,example_target,8);bad=example;bad.last_processed_epoch=UINT64_MAX;reject(bad,example_header,example_target,8);
 // Rebind the synthetic state root to isolate latest-header consistency guards.
 bad=example;bad.ssz[64]^=1;h=example_header;h.state_root=phase0_beacon_state_root(bad.ssz,rules.state);t=example_target;t.root=consensus_beacon_header_root(h);reject(bad,h,t,8);
 bad=example;bad.ssz[112]=1;h=example_header;h.state_root=phase0_beacon_state_root(bad.ssz,rules.state);t=example_target;t.root=consensus_beacon_header_root(h);reject(bad,h,t,8);
 size_t calls=0;prepare_phase0_fork_state_contexts(example,example_header,example_target,rules.epoch,rules.state,8,[&]{++calls;});const auto old=example;
 std::set<size_t> stops{1,2,calls/4,calls/2,3*calls/4,calls-1,calls};for(auto stop:stops){size_t n=0;bool cancelled=false;try{prepare_phase0_fork_state_contexts(example,example_header,example_target,rules.epoch,rules.state,8,[&]{if(++n==stop)throw std::runtime_error("cancel");});}catch(const std::runtime_error& e){cancelled=std::string(e.what())=="cancel";}check(cancelled&&example==old,"cancel context source mutation");}
 std::cout<<Json{{"scenario","phase0-fork-state-context"},{"officialSequences",cases},{"skippedSlotContexts",skipped},{"rejections",rejected},{"cancellations",stops.size()},{"networkAdmission",false}}.dump()<<'\n';
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
