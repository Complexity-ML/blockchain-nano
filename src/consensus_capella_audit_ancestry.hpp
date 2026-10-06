#pragma once
#include "nano/consensus_capella_fork_store.hpp"
#include <limits>
namespace nano::audit_detail {
// Input blocks must already belong to the authenticated canonical replay.
inline void require_finalized_ancestor(const CapellaForkStore& replay,const ConsensusCheckpoint& finalized,
 const ConsensusCheckpoint& observed,uint64_t slots,const std::function<void()>& check={}) {
 auto require=[](bool ok,const char* message){if(!ok)throw std::invalid_argument(message);};
 require(slots&&observed.epoch<=finalized.epoch&&observed.epoch<=std::numeric_limits<uint64_t>::max()/slots,"observed finalized epoch outside replay bounds");
 const auto boundary=observed.epoch*slots;
 auto at=finalized.root;uint64_t child=std::numeric_limits<uint64_t>::max();
 for(size_t n=0;n<=replay.blocks.size();++n){
  if(check)check();const auto found=replay.blocks.find(at);require(found!=replay.blocks.end(),"observed finalized checkpoint outside replayed ancestry");
  const auto& header=found->second.header();require(header.slot<child,"non-descending finalized ancestry");
  if(header.slot<=boundary){require(at==observed.root,"observed checkpoint differs from finalized epoch boundary");return;}
  child=header.slot;at=header.parent_root;
 }
 throw std::invalid_argument("finalized ancestry traversal budget exceeded");
}
}
