#include "nano/consensus_altair_anchor.hpp"
#include <fstream>
namespace nano {
namespace {
Bytes read_bounded(const std::filesystem::path& path,size_t maximum,const std::function<void()>& checkpoint){
 if(!maximum||!std::filesystem::is_regular_file(path))throw std::invalid_argument("Altair anchor requires bounded regular files");
 const auto size=std::filesystem::file_size(path);
 if(size>maximum)throw std::length_error("Altair anchor file size limit");
 std::ifstream stream(path,std::ios::binary);
 if(!stream)throw std::runtime_error("Cannot open Altair anchor file");
 Bytes out;out.reserve(static_cast<size_t>(size));std::array<char,16384> chunk;
 while(stream){
  if(checkpoint)checkpoint();stream.read(chunk.data(),chunk.size());const auto count=static_cast<size_t>(stream.gcount());
  if(count>maximum-out.size())throw std::length_error("Altair anchor file grew beyond limit");
  out.insert(out.end(),chunk.begin(),chunk.begin()+count);
 }
 if(!stream.eof()||stream.bad())throw std::runtime_error("Cannot read Altair anchor file");
 return out;
}
uint64_t number(const Bytes& bytes,size_t offset){uint64_t value=0;for(size_t i=0;i<8;++i)value|=uint64_t(bytes.at(offset+i))<<(8*i);return value;}
}
AltairForkStore load_altair_anchor(const std::filesystem::path& state_file,
 const std::filesystem::path& block_file,const AltairAnchorCommitment& expected,
 const AltairForkBlockRules& rules,size_t maximum_state_bytes,const std::function<void()>& checkpoint){
 if(checkpoint)checkpoint();validate_altair_transition_rules(rules.transition);
 if(!rules.slot_duration_ms||rules.attestation_due_ms>rules.slot_duration_ms||
 (rules.boost_profile!=AltairProposerBoostProfile::Release160Beta0&&rules.boost_profile!=AltairProposerBoostProfile::ShufflingDependent))throw std::invalid_argument("Invalid Altair anchor fork-choice rules");
 auto state=read_bounded(state_file,maximum_state_bytes,checkpoint);
 const auto block=read_bounded(block_file,rules.transition.maximum_block_bytes,checkpoint);
 if(state.size()<60||!std::equal(expected.genesis_validators_root.begin(),expected.genesis_validators_root.end(),state.begin()+8)||
 !std::equal(expected.fork_version.begin(),expected.fork_version.end(),state.begin()+52))throw std::invalid_argument("Altair anchor fork domain mismatch");
 if(altair_beacon_state_root(state,rules.transition.state,checkpoint)!=expected.state_root)throw std::invalid_argument("Altair anchor state commitment mismatch");
 const auto epoch=number(state,40)/rules.transition.epoch.ffg.slots_per_epoch;
 AltairSlotState slot{std::move(state),epoch?std::optional<uint64_t>(epoch-1):std::nullopt};
 auto candidate=initialize_altair_fork_store(slot,block,rules,checkpoint);
 if(candidate.timed.view.anchor!=expected.block_root||candidate.blocks.at(candidate.timed.view.anchor).header.state_root!=expected.state_root)throw std::invalid_argument("Altair anchor block commitment mismatch");
 if(checkpoint)checkpoint();return candidate;
}
}
