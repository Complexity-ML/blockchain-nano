#include "support/altair_transition_rules.hpp"
#include "support/phase0_indexed_ssz.hpp"
#include "nano/consensus_altair_fork_store.hpp"
using namespace test;using namespace test::phase0_minimal;
ConsensusRoot parse_root(const Json& j){const auto b=unhex(j.get<std::string>());check(b.size()==32,"root width");ConsensusRoot out{};std::copy(b.begin(),b.end(),out.begin());return out;}
Phase0SignedAttestation decode_vote(const Bytes& b){check(b.size()>=229&&little(b,0,4)==228&&b.back(),"vote SSZ");const auto a=indexed_attestation(Bytes(b.begin(),b.begin()+228),0,228);Phase0SignedAttestation v{a.data,{},a.signature};const auto count=(b.size()-229)*8+std::bit_width(unsigned(b.back()))-1;for(size_t i=0;i<count;++i)v.aggregation_bits.push_back((b[228+i/8]>>(i%8))&1);return v;}
int main(int argc,char** argv){try{
 check(argc==1||argc==3,"usage: runner [fixture-directory expected-count]");const size_t expected_cases=argc==3?std::stoull(argv[2]):10;const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures",refs=fixtures/"consensus-altair-fork-head-official",root=argc==3?fs::path(argv[1]):refs;const auto transition=altair_transition_rules(fixtures);const auto config=read(refs/"minimal-config-v1.6.0-beta.0.yaml");const auto duration=parameter(config,"SECONDS_PER_SLOT")*1000;const AltairForkBlockRules rules{transition,duration,duration/3,parameter(config,"PROPOSER_SCORE_BOOST"),AltairProposerBoostProfile::Release160Beta0};const auto slots=transition.epoch.ffg.slots_per_epoch;
 const auto manifest=load(root/"manifest.json");check(manifest.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908"&&manifest.at("cases").size()==expected_cases,"head corpus");const auto references=load(refs/"manifest.json");for(const auto& [path,info]:references.at("referenceFiles").items()){const auto b=binary(refs/path);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"reference digest");}size_t ticks=0,blocks=0,votes=0,slashings=0,checks=0,rejections=0;std::optional<AltairForkStore> example;Bytes example_block;Phase0SignedAttestation example_vote;
 for(const auto& row:manifest.at("cases")){const auto name=row.at("name").get<std::string>();for(const auto& [path,info]:row.at("files").items()){const auto b=binary(root/path);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");}
  const auto dir=root/name;AltairSlotState anchor{binary(dir/"anchor_state.ssz"),std::nullopt};const auto epoch=little(anchor.ssz,40,8)/slots;if(epoch)anchor.last_processed_epoch=epoch-1;auto store=initialize_altair_fork_store(anchor,binary(dir/"anchor_block.ssz"),rules);const auto steps=load(dir/"steps.json");size_t step_index=0;
  for(const auto& step:steps){try{
   if(step.contains("tick")){store=prepare_altair_store_tick(store,step.at("tick").get<uint64_t>()*1000,rules,4096,100000);++ticks;}
   else if(step.contains("checks")){const auto& c=step.at("checks");const auto& v=store.timed.view;for(const auto& [key,value]:c.items()){
    if(key=="head"){const auto head=phase0_fork_choice_head(v,{slots,transition.attestation.duties.effective_balance_increment,rules.proposer_score_boost}).head;if(head!=parse_root(value.at("root"))){const auto& n=v.nodes.at(head);std::cerr<<"actual="<<hex(Bytes(head.begin(),head.end()))<<" slot="<<n.slot<<" realized="<<n.realized_justified_epoch<<" unrealized="<<n.unrealized_justified_epoch<<" current="<<v.current_epoch<<" justified="<<v.justified.epoch<<" latestMessages="<<v.latest_messages.size()<<" boostMatchesHead="<<(v.proposer_boost==head)<<" expected="<<value.dump()<<"\n";}check(head==parse_root(value.at("root"))&&v.nodes.at(head).slot==value.at("slot").get<uint64_t>(),"head mismatch");}
    else if(key=="time")check(store.timed.clock.time_ms==value.get<uint64_t>()*1000,"time mismatch");
    else if(key=="genesis_time")check(store.timed.clock.genesis_time_ms==value.get<uint64_t>()*1000,"genesis time mismatch");
    else if(key=="proposer_boost_root")check(v.proposer_boost.value_or(ConsensusRoot{})==parse_root(value),"boost mismatch");
    else if(key=="justified_checkpoint"||key=="finalized_checkpoint"){const auto& cp=key=="justified_checkpoint"?v.justified:v.finalized;check(cp.epoch==value.at("epoch").get<uint64_t>()&&cp.root==parse_root(value.at("root")),"checkpoint mismatch");}
    else throw std::runtime_error("unknown expected field: "+key);++checks;
   }}else{
    std::optional<AltairForkStore> result;
    if(step.contains("block")){const auto packet=binary(dir/(step.at("block").get<std::string>()+".ssz"));result=prepare_altair_received_block(store,packet,rules,4096,100000);++blocks;
     if(!example&&result&&result->timed.view.latest_messages!=store.timed.view.latest_messages){const auto start=184+little(packet,184+208,4),end=184+little(packet,184+212,4);if(end>start){const auto first=little(packet,start,4),stop=first==4?end-start:little(packet,start+4,4);example_vote=decode_vote(Bytes(packet.begin()+start+first,packet.begin()+start+stop));example=store;example_block=packet;}}
    }
    else if(step.contains("attestation")){result=prepare_altair_store_vote(store,decode_vote(binary(dir/(step.at("attestation").get<std::string>()+".ssz"))),rules,4096,100000);++votes;}
    else if(step.contains("attester_slashing")){const auto [a,b]=attester_slashing(binary(dir/(step.at("attester_slashing").get<std::string>()+".ssz")));result=prepare_altair_store_slashing(store,a,b,rules);++slashings;}
    else throw std::runtime_error("unknown event");check(bool(result)==step.value("valid",true),"event validity mismatch");if(result)store=std::move(*result);else ++rejections;
   }
  }catch(const std::exception& e){throw std::runtime_error(name+" step "+std::to_string(step_index)+": "+e.what());}++step_index;}
 }
 size_t cancellation_count=0;if(argc==1){check(bool(example),"no embedded-vote example");
 const auto delayed_time=example->timed.clock.genesis_time_ms+(example->timed.view.current_epoch+3)*slots*rules.slot_duration_ms;
 const auto delayed=prepare_altair_store_tick(*example,delayed_time,rules,4096,100000);
 check(!prepare_altair_store_vote(delayed,example_vote,rules,4096,100000),"old gossip bypassed time window");
 const auto received=prepare_altair_received_block(delayed,example_block,rules,4096,100000);
 check(received&&received->timed.view.latest_messages!=delayed.timed.view.latest_messages,"old authenticated block votes lost");
 auto corrupted=example_block;corrupted[4]^=1;check(!prepare_altair_received_block(delayed,corrupted,rules,4096,100000),"invalid block published votes");
 size_t calls=0;prepare_altair_received_block(*example,example_block,rules,4096,100000,[&]{++calls;});std::set<size_t> stops{1,2,calls/4,calls/2,3*calls/4,calls-1,calls};const auto messages=example->timed.view.latest_messages;const auto block_count=example->blocks.size();
 for(auto stop:stops){size_t n=0;bool cancelled=false;try{prepare_altair_received_block(*example,example_block,rules,4096,100000,[&]{if(++n==stop)throw std::runtime_error("cancelled");});}catch(const std::runtime_error& e){cancelled=std::string(e.what())=="cancelled";}check(cancelled&&example->blocks.size()==block_count&&example->timed.view.latest_messages==messages,"cancelled reception published data");}
 cancellation_count=stops.size();}
 std::cout<<Json{{"scenario",root.filename().string()},{"cases",expected_cases},{"ticks",ticks},{"blocks",blocks},{"votes",votes},{"slashings",slashings},{"checks",checks},{"rejections",rejections},{"localGuardsRun",argc==1},{"cancellations",cancellation_count},{"durable",false},{"networkActive",false}}.dump()<<'\n';
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
