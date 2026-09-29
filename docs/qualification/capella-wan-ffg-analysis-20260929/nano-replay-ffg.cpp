#include "nano/consensus_capella_store_snapshot.hpp"
#include <fstream>
#include <iostream>
using namespace nano;
int main(int argc,char**argv){try{
 if(argc!=3)return 2;std::ifstream in(argv[1]);std::string raw((std::istreambuf_iterator<char>(in)),{});
 if(hex(sha256(Bytes(raw.begin(),raw.end())))!=argv[2])throw std::runtime_error("external digest mismatch");
 auto j=Json::parse(raw).at("store");auto profile=j.at("profile").dump();auto rules=load_capella_fork_profile(profile,sha256(Bytes(profile.begin(),profile.end())),1048576);
 auto serialized=j.dump();auto store=capella_store_from_snapshot(serialized,sha256(Bytes(serialized.begin(),serialized.end())),rules,{134217728,256,2048,1000,4194304,67108864,64,100000});
 const auto& r=rules.transition;CapellaEpochRules epoch{r.consensus.epoch,r.execution,r.maximum_state_bytes};const CapellaForkBlock *before=nullptr,*after=nullptr;
 for(const auto&[root,b]:store.blocks)if(b.header.slot==72){after=&b;before=&store.blocks.at(b.header.parent_root);}
 if(!before||before->header.slot!=71)throw std::runtime_error("missing canonical 71->72 transition");
 auto next=prepare_capella_slots(before->state,epoch,r.consensus.state,72,64,100000);
 auto old=capella_fork_checkpoints(before->state,epoch,r.consensus.state).realized;
 auto got=capella_fork_checkpoints(next,epoch,r.consensus.state).realized;
 auto actual=capella_fork_checkpoints(after->state,epoch,r.consensus.state).realized;
 if(got.finalized!=actual.finalized||got.previous_justified!=actual.previous_justified||got.current_justified!=actual.current_justified||got.justification_bits!=actual.justification_bits)throw std::runtime_error("FFG replay differs");
 std::cout<<Json{{"snapshotValidated",true},{"slotFrom",71},{"slotTo",72},{"finalizedBefore",old.finalized.epoch},{"finalizedAfter",got.finalized.epoch},{"previousJustified",got.previous_justified.epoch},{"currentJustified",got.current_justified.epoch},{"bits",got.justification_bits},{"matchesStoredBlock",true}}.dump(2)<<"\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
