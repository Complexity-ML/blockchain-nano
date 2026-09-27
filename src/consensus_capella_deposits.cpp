#include "nano/consensus_capella_deposits.hpp"
namespace nano {
namespace {
uint64_t number(std::span<const uint8_t> b,size_t at,size_t width=8){if(at>b.size()||width>b.size()-at)throw std::invalid_argument("deposit SSZ bounds");uint64_t n=0;for(size_t i=0;i<width;++i)n|=uint64_t(b[at+i])<<(8*i);return n;}
Bytes vote_for(const CapellaForkBlock& b){
 if(!b.deposit_history)throw std::invalid_argument("missing native deposit history");
 const auto& history=*b.deposit_history;const auto root=history.root();Bytes vote(root.begin(),root.end());for(unsigned i=0;i<8;++i)vote.push_back(uint8_t(history.count()>>(8*i)));const auto hash=b.execution_block.header.hash();if(hash!=history.execution_hash())throw std::invalid_argument("native deposit anchor mismatch");vote.insert(vote.end(),hash.begin(),hash.end());return vote;
}
const ExecutionDepositHistory* find(const CapellaForkStore& store,ConsensusRoot id,std::span<const uint8_t> vote,const std::function<void()>& check){
 if(vote.size()!=72)return nullptr;
 for(size_t seen=0;seen<store.blocks.size();++seen){if(check)check();const auto& b=store.blocks.at(id);const auto candidate=vote_for(b);if(std::equal(vote.begin(),vote.end(),candidate.begin()))return b.deposit_history.get();if(id==store.timed.view.anchor)return nullptr;id=b.header.parent_root;}
 throw std::invalid_argument("cyclic native deposit ancestry");
}
}
Bytes capella_native_deposit_vote(const CapellaForkStore& store,const ConsensusRoot& parent,uint64_t slot,const CapellaForkBlockRules& rules,const std::function<void()>& check){
 if(!rules.native_deposit_votes||!rules.transition.consensus.state.eth1_votes_limit)throw std::invalid_argument("native deposit voting disabled");
 const auto period=rules.transition.consensus.state.eth1_votes_limit,start=slot-slot%period;auto id=parent;
 for(size_t seen=0;seen<store.blocks.size();++seen){if(check)check();const auto& b=store.blocks.at(id);if(b.header.slot<start||id==store.timed.view.anchor)return vote_for(b);id=b.header.parent_root;}
 throw std::invalid_argument("cyclic native deposit ancestry");
}
bool capella_native_deposit_vote_known(const CapellaForkStore& s,const ConsensusRoot& p,std::span<const uint8_t> v,const std::function<void()>& c){return find(s,p,v,c)!=nullptr;}
Bytes capella_native_deposit_operations(const CapellaForkStore& store,const ConsensusRoot& parent,std::span<const uint8_t> advanced,std::span<const uint8_t> vote,const CapellaForkBlockRules& rules,const std::function<void()>& check){
 if(!rules.native_deposit_votes||!find(store,parent,vote,check))throw std::invalid_argument("unauthenticated native deposit vote");
 const auto& t=rules.transition.consensus;const size_t historical=176+64*t.state.slots_per_historical_root;
 if(historical>advanced.size()||96>advanced.size()-historical)throw std::invalid_argument("deposit state SSZ bounds");
 Bytes selected(advanced.begin()+historical+4,advanced.begin()+historical+76);
 const auto begin=number(advanced,historical+76,4),end=number(advanced,historical+88,4);
 if(begin>end||end>advanced.size()||(end-begin)%72||(end-begin)/72>=t.state.eth1_votes_limit)throw std::invalid_argument("native deposit vote list bounds");
 size_t tally=1;for(size_t at=begin;at<end;at+=72){if(check)check();if(std::equal(vote.begin(),vote.end(),advanced.begin()+at))++tally;}
 if(tally>t.state.eth1_votes_limit/2)selected.assign(vote.begin(),vote.end());
 const auto count=number(selected,32),index=number(advanced,historical+80);if(index>count)throw std::invalid_argument("native deposit cursor beyond count");
 const auto needed=std::min<uint64_t>(t.body.deposits,count-index);if(!needed)return {};
 const auto* source=find(store,parent,selected,check);if(!source)throw std::invalid_argument("native mandatory deposit history unavailable");
 if(needed>t.maximum_block_bytes/1240)throw std::length_error("native deposit operations budget");
 Bytes out;out.reserve(size_t(needed)*1240);
 for(uint64_t i=0;i<needed;++i){const auto d=source->proof(index+i,check);for(const auto& sibling:d.proof)out.insert(out.end(),sibling.begin(),sibling.end());out.insert(out.end(),d.message.public_key.begin(),d.message.public_key.end());out.insert(out.end(),d.message.withdrawal_credentials.begin(),d.message.withdrawal_credentials.end());for(unsigned j=0;j<8;++j)out.push_back(uint8_t(d.message.amount_gwei>>(8*j)));out.insert(out.end(),d.signature.begin(),d.signature.end());}
 return out;
}
}
