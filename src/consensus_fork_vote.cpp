#include "nano/consensus_fork_vote.hpp"
#include "nano/consensus_bls.hpp"
#include "nano/committee.hpp"
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
}
