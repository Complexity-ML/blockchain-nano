#include "nano/consensus_fork_vote.hpp"
#include "nano/consensus_bls.hpp"
#include "nano/committee.hpp"
#include <algorithm>
namespace nano {
bool process_phase0_fork_vote(Phase0ForkChoiceView& view,uint64_t current_slot,
 const Phase0ForkVoteContext& ctx,const Phase0FfgRules& rules,
 const Phase0SignedAttestation& vote,bool from_verified_block,
 const std::function<void()>& checkpoint){
 if(!rules.slots_per_epoch||current_slot/rules.slots_per_epoch!=view.current_epoch||
    ctx.registry.epoch!=ctx.checkpoint.epoch||ctx.randao.epoch!=ctx.checkpoint.epoch||ctx.fork.epoch>ctx.checkpoint.epoch)
  throw std::invalid_argument("invalid fork vote context");
 if(checkpoint)checkpoint();
 const auto& d=vote.data;const auto previous=view.current_epoch?view.current_epoch-1:0;
 if((!from_verified_block&&d.target.epoch!=view.current_epoch&&d.target.epoch!=previous)||
    d.target.epoch!=d.slot/rules.slots_per_epoch||d.target!=ctx.checkpoint||
    d.slot>=current_slot||!view.nodes.contains(d.target.root)||!view.nodes.contains(d.beacon_block_root))return false;
 if(view.nodes.at(d.beacon_block_root).slot>d.slot)return false;
 // Division equality above guarantees target.epoch * slots_per_epoch <= slot.
 const auto target_slot=d.target.epoch*rules.slots_per_epoch;auto ancestor=d.beacon_block_root;
 while(view.nodes.at(ancestor).slot>target_slot){
  if(checkpoint)checkpoint();const auto& n=view.nodes.at(ancestor);
  if(ancestor==view.anchor||!view.nodes.contains(n.parent)||view.nodes.at(n.parent).slot>=n.slot)return false;
  ancestor=n.parent;
 }
 if(ancestor!=d.target.root)return false;
 const auto duties=phase0_attestation_duties(ctx.registry,ctx.randao,rules,d.target.epoch,checkpoint);
 if(d.index>=duties.committees_per_slot)return false;
 const auto committee=partition_committee(duties.shuffled,
  (d.slot%rules.slots_per_epoch)*duties.committees_per_slot+d.index,
  duties.committees_per_slot*rules.slots_per_epoch);
 if(vote.aggregation_bits.size()!=committee.size())return false;
 std::vector<Bytes> keys;std::vector<uint64_t> indices;
 for(size_t i=0;i<committee.size();++i)if(vote.aggregation_bits[i]){
  if(checkpoint)checkpoint();const auto index=committee[i];const auto& key=ctx.registry.validators.at(index).public_key;
  keys.emplace_back(key.begin(),key.end());indices.push_back(index);
 }
 if(keys.empty())return false;
 const auto version=d.target.epoch<ctx.fork.epoch?ctx.fork.previous_version:ctx.fork.current_version;
 const auto signing_root=consensus_signing_root(consensus_attestation_root(d),consensus_domain({1,0,0,0},version,ctx.genesis_root));
 if(!verify_consensus_bls_fast_aggregate(keys,signing_root,vote.signature))return false;
 // Validate signature even if all indices are equivocators or votes are old.
 // Same-epoch conflicts do not overwrite the first message; a separate verified
 // slashing event must mark equivocators. Receipt alone is not a slashing proof.
 auto candidate=view.latest_messages;
 for(const auto index:indices){
  if(checkpoint)checkpoint();if(view.equivocating_indices.contains(index))continue;
  const auto old=candidate.find(index);
  if(old==candidate.end()||d.target.epoch>old->second.epoch)candidate[index]={d.target.epoch,d.beacon_block_root};
 }
 if(checkpoint)checkpoint();view.latest_messages.swap(candidate);return true;
}
bool process_phase0_fork_slashing(Phase0ForkChoiceView& view,
 const Phase0ForkSlashingContext& ctx,const IndexedConsensusAttestation& first,
 const IndexedConsensusAttestation& second,uint64_t maximum_indices,
 const std::function<void()>& checkpoint){
 if(!maximum_indices||ctx.block_root!=view.justified.root||ctx.fork.epoch>ctx.registry.epoch)
  throw std::invalid_argument("invalid fork slashing context");
 if(checkpoint)checkpoint();
 if(!is_slashable_attestation_data(first.data,second.data))return false;
 auto valid=[&](const IndexedConsensusAttestation& vote){
  if(vote.indices.empty()||vote.indices.size()>maximum_indices||
     !std::is_sorted(vote.indices.begin(),vote.indices.end())||
     std::adjacent_find(vote.indices.begin(),vote.indices.end())!=vote.indices.end())return false;
  std::vector<Bytes> keys;keys.reserve(vote.indices.size());
  for(auto index:vote.indices){
   if(checkpoint)checkpoint();const auto found=ctx.registry.validators.find(index);
   if(found==ctx.registry.validators.end())return false;
   const auto& key=found->second.public_key;keys.emplace_back(key.begin(),key.end());
  }
  const auto version=vote.data.target.epoch<ctx.fork.epoch?ctx.fork.previous_version:ctx.fork.current_version;
  const auto r=consensus_signing_root(consensus_attestation_root(vote.data),consensus_domain({1,0,0,0},version,ctx.genesis_root));
  if(checkpoint)checkpoint();return verify_consensus_bls_fast_aggregate(keys,r,vote.signature);
 };
 if(!valid(first)||!valid(second))return false;
 auto candidate=view.equivocating_indices;
 size_t i=0,j=0;
 while(i<first.indices.size()&&j<second.indices.size()){
  if(checkpoint)checkpoint();
  if(first.indices[i]<second.indices[j])++i;
  else if(second.indices[j]<first.indices[i])++j;
  else{candidate.insert(first.indices[i]);++i;++j;}
 }
 if(checkpoint)checkpoint();view.equivocating_indices.swap(candidate);return true;
}
}
