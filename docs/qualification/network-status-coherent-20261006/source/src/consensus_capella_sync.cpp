#include "nano/consensus_capella_sync.hpp"
#include <algorithm>
#include <numeric>
namespace nano {
namespace {
ConsensusBeaconHeader inspect(const CapellaSyncBlock& block,const CapellaForkBlockRules& rules,const std::function<void()>& check){
 const auto& t=rules.transition;const auto& c=t.consensus;
 if(check)check();const auto& b=block.ssz;if(b.size()<404||b.size()>c.maximum_block_bytes)throw std::invalid_argument("Invalid Capella sync block size");
 auto number=[&](size_t offset,size_t length=8){uint64_t value=0;for(size_t n=0;n<length;++n)value|=uint64_t(b.at(offset+n))<<(8*n);return value;};
 auto root=[&](size_t offset){ConsensusRoot value{};std::copy_n(b.begin()+offset,32,value.begin());return value;};
 if(number(0,4)!=100||number(180,4)!=84)throw std::invalid_argument("Invalid Capella sync offsets");
 ConsensusBeaconHeader header{number(100),number(108),root(116),root(148),capella_beacon_body_root(std::span<const uint8_t>(b).subspan(184),c.body,c.state.sync_committee_size,t.execution,t.withdrawals.maximum_per_payload,t.maximum_bls_changes,c.maximum_block_bytes,check)};
 if(consensus_beacon_header_root(header)!=block.root)throw std::invalid_argument("Capella sync root mismatch");return header;
}
}

std::vector<size_t> order_capella_sync_blocks(std::span<const CapellaSyncBlock> blocks,const std::set<ConsensusRoot>& known,const CapellaForkBlockRules& rules,size_t maximum,size_t budget,const std::function<void()>& check){
 if(!maximum||!budget||blocks.size()>maximum)throw std::length_error("Capella sync batch capacity");
 size_t bytes=0;for(const auto& block:blocks){if(check)check();if(block.ssz.capacity()>budget-bytes)throw std::length_error("Capella sync payload capacity");bytes+=block.ssz.capacity();}
 struct Entry{ConsensusRoot parent;uint64_t slot;};std::vector<Entry> entries;entries.reserve(blocks.size());std::set<ConsensusRoot> unique;
 for(const auto& block:blocks){const auto header=inspect(block,rules,check);if(!unique.insert(block.root).second)throw std::invalid_argument("Capella sync duplicate");entries.push_back({header.parent_root,header.slot});}
 std::vector<size_t> order(blocks.size());std::iota(order.begin(),order.end(),0);std::sort(order.begin(),order.end(),[&](size_t a,size_t b){return entries[a].slot!=entries[b].slot?entries[a].slot<entries[b].slot:blocks[a].root<blocks[b].root;});
 std::map<ConsensusRoot,uint64_t> scheduled;
 for(const auto index:order){if(check)check();const auto& e=entries[index];const auto parent=scheduled.find(e.parent);if(parent!=scheduled.end()){if(parent->second>=e.slot)throw std::invalid_argument("Capella sync parent slot not earlier");}else if(!known.contains(e.parent))throw std::invalid_argument("Capella sync parent missing");scheduled.emplace(blocks[index].root,e.slot);}
 return order;
}
std::vector<CapellaSyncBlock> collect_capella_sync_blocks(std::span<const ConsensusRoot> targets,const std::set<ConsensusRoot>& known,const CapellaForkBlockRules& rules,size_t maximum,size_t budget,const CapellaSyncFetch& fetch,const std::function<void()>& check){
 if(!maximum||!budget||targets.size()>maximum||!fetch)throw std::invalid_argument("Invalid Capella sync fetch limits");
 std::vector<ConsensusRoot> pending(targets.begin(),targets.end());std::set<ConsensusRoot> fetched;std::vector<CapellaSyncBlock> blocks;size_t used=0;
 while(!pending.empty()){
  if(check)check();const auto root=pending.back();pending.pop_back();if(known.contains(root)||fetched.contains(root))continue;
  if(blocks.size()>=maximum||used==budget)throw std::length_error("Capella dependency budget exhausted");
  const auto allowance=std::min(budget-used,rules.transition.consensus.maximum_block_bytes);auto payload=fetch(root,allowance,check);if(check)check();if(!payload)throw std::invalid_argument("Capella dependency unavailable");
  if(payload->capacity()>allowance)throw std::length_error("Capella dependency exceeded fetch allowance");
  CapellaSyncBlock block{root,std::move(*payload)};const auto header=inspect(block,rules,check);used+=block.ssz.capacity();fetched.insert(root);blocks.push_back(std::move(block));
  if(!known.contains(header.parent_root)&&!fetched.contains(header.parent_root))pending.push_back(header.parent_root);
 }
 const auto order=order_capella_sync_blocks(blocks,known,rules,maximum,budget,check);std::vector<CapellaSyncBlock> result;result.reserve(blocks.size());for(auto index:order)result.push_back(std::move(blocks[index]));return result;
}

}
