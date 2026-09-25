#include "support/altair_transition_rules.hpp"
#include "nano/consensus_altair_anchor.hpp"
#include "nano/consensus_altair_reactor.hpp"
using namespace test;using namespace test::phase0_minimal;
int main(){try{
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures",corpus=fixtures/"consensus-altair-fork-head-official",dir=corpus/"chain_no_attestations";
 const auto manifest=load(corpus/"manifest.json");for(const auto& row:manifest.at("cases"))if(row.at("name")=="chain_no_attestations")for(const auto& [path,info]:row.at("files").items())check(hex(sha256(binary(corpus/path))).substr(2)==info.at("sha256").get<std::string>(),"anchor fixture digest");
 const auto source_rules=altair_transition_rules(fixtures);const auto text=altair_transition_profile(source_rules).dump();const auto transition=load_altair_transition_profile(text,sha256(Bytes(text.begin(),text.end())),text.size());
 const auto config=read(corpus/"minimal-config-v1.6.0-beta.0.yaml");const auto duration=parameter(config,"SECONDS_PER_SLOT")*1000;
 const AltairForkBlockRules rules{transition,duration,duration/3,parameter(config,"PROPOSER_SCORE_BOOST"),AltairProposerBoostProfile::Release160Beta0};
 const auto state=binary(dir/"anchor_state.ssz"),block=binary(dir/"anchor_block.ssz");
 const auto reference=initialize_altair_fork_store({state,std::nullopt},block,rules);
 AltairAnchorCommitment commitment{reference.timed.view.anchor,altair_beacon_state_root(state,transition.state),root_at(state,8),{}};std::copy_n(state.begin()+52,4,commitment.fork_version.begin());
 const AltairStoreSnapshotLimits limits{32*1024*1024,100,10000,10000,1024*1024,16*1024*1024,4096,100000};
 const auto anchor=load_altair_anchor(dir/"anchor_state.ssz",dir/"anchor_block.ssz",commitment,rules,state.size());
 check(altair_store_snapshot(anchor,rules,limits)==altair_store_snapshot(reference,rules,limits),"file anchor changes Store");
 Temp temp;ChainStore disk(temp/"db");const Bytes genesis(32,9);disk.bind_chain(1337,genesis,"altair-qualification");unsigned rejects=0;
 auto refuse=[&](const fs::path& state_path,const fs::path& block_path,const AltairAnchorCommitment& expected,const AltairForkBlockRules& policy,size_t bound){bool failed=false;try{load_altair_anchor(state_path,block_path,expected,policy,bound);}catch(const std::exception&){failed=true;}check(failed&&!disk.fork_checkpoint(1337,genesis,limits.envelope_bytes),"rejected anchor changed storage");++rejects;};
 for(unsigned field=0;field<4;++field){auto wrong=commitment;if(field==0)wrong.block_root[0]^=1;if(field==1)wrong.state_root[0]^=1;if(field==2)wrong.genesis_validators_root[0]^=1;if(field==3)wrong.fork_version[0]^=1;refuse(dir/"anchor_state.ssz",dir/"anchor_block.ssz",wrong,rules,state.size());}
 refuse(dir/"anchor_state.ssz",dir/"anchor_block.ssz",commitment,rules,state.size()-1);
 refuse(temp/"missing",dir/"anchor_block.ssz",commitment,rules,state.size());refuse(temp.path,dir/"anchor_block.ssz",commitment,rules,state.size());
 auto malformed=state;malformed.back()^=1;{std::ofstream f(temp/"state",std::ios::binary);f.write(reinterpret_cast<const char*>(malformed.data()),malformed.size());}
 refuse(temp/"state",dir/"anchor_block.ssz",commitment,rules,state.size());
 auto bad_block=block;bad_block[16]^=1;{std::ofstream f(temp/"block",std::ios::binary);f.write(reinterpret_cast<const char*>(bad_block.data()),bad_block.size());}
 refuse(dir/"anchor_state.ssz",temp/"block",commitment,rules,state.size());
 auto bad_rules=rules;bad_rules.attestation_due_ms=duration+1;refuse(dir/"anchor_state.ssz",dir/"anchor_block.ssz",commitment,bad_rules,state.size());
 bad_rules=rules;bad_rules.transition.maximum_block_bytes=block.size()-1;refuse(dir/"anchor_state.ssz",dir/"anchor_block.ssz",commitment,bad_rules,state.size());
 size_t calls=0;load_altair_anchor(dir/"anchor_state.ssz",dir/"anchor_block.ssz",commitment,rules,state.size(),[&]{++calls;});
 const std::set<size_t> stop_points{1,2,calls/2,calls};
 for(const auto stop:stop_points){size_t seen=0;bool cancelled=false;
  try{load_altair_anchor(dir/"anchor_state.ssz",dir/"anchor_block.ssz",commitment,rules,state.size(),[&]{if(++seen==stop)throw std::runtime_error("cancelled");});}catch(const std::runtime_error& e){cancelled=std::string(e.what())=="cancelled";}
  check(cancelled&&!disk.fork_checkpoint(1337,genesis,limits.envelope_bytes),"cancelled anchor persisted");
 }
 // Checkpoint at slot 17 exercises reconstruction of the epoch replay marker.
 const auto full=fixtures/"consensus-altair-full-blocks-official",later=full/"attestation";
 const auto full_manifest=load(full/"manifest.json");for(const auto& row:full_manifest.at("cases"))if(row.at("name")=="attestation")for(const auto& [path,info]:row.at("files").items())check(hex(sha256(binary(full/path))).substr(2)==info.at("sha256").get<std::string>(),"later anchor fixture digest");
 const auto later_state=binary(later/"post.ssz"),signed_block=binary(later/"blocks_1.ssz");
 const Bytes unsigned_block(signed_block.begin()+100,signed_block.end());
 {std::ofstream f(temp/"later-block",std::ios::binary);f.write(reinterpret_cast<const char*>(unsigned_block.data()),unsigned_block.size());}
 const auto epoch=little(later_state,40,8)/transition.epoch.ffg.slots_per_epoch;check(epoch>0,"later fixture is not post-genesis");
 const auto later_reference=initialize_altair_fork_store({later_state,epoch-1},unsigned_block,rules);
 AltairAnchorCommitment later_commitment{later_reference.timed.view.anchor,root_at(signed_block,148),root_at(later_state,8),{}};std::copy_n(later_state.begin()+52,4,later_commitment.fork_version.begin());
 const auto loaded_later=load_altair_anchor(later/"post.ssz",temp/"later-block",later_commitment,rules,later_state.size());
 check(loaded_later.blocks.at(loaded_later.timed.view.anchor).state.last_processed_epoch==epoch-1&&altair_store_snapshot(loaded_later,rules,limits)==altair_store_snapshot(later_reference,rules,limits),"nonzero epoch anchor differs");
 AltairDurableReactor reactor(disk,1337,genesis,rules,limits,anchor);check(reactor.current().token.sequence==1,"anchor not durably initialized");
 std::cout<<Json{{"scenario","altair-trusted-anchor-files"},{"rejected",rejects},{"cancelledWithoutCommit",stop_points.size()},{"nonzeroEpochAnchor",true},{"initializedSequence",1},{"networkActive",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
