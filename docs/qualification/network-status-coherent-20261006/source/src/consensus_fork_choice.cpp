#include "nano/consensus_fork_choice.hpp"
#include <algorithm>
namespace nano {
Phase0ForkChoiceResult phase0_fork_choice_head(const Phase0ForkChoiceView& view,
 const Phase0ForkChoiceRules& rules,const std::function<void()>& checkpoint){
 auto require=[](bool condition){if(!condition)throw std::invalid_argument("invalid Phase0 fork-choice view");};
 require(rules.slots_per_epoch&&rules.effective_balance_increment&&
  view.finalized.epoch<=view.justified.epoch&&view.justified.epoch<=view.current_epoch&&
  view.justified_registry.epoch==view.justified.epoch&&
  view.nodes.contains(view.anchor)&&view.nodes.contains(view.justified.root)&&view.nodes.contains(view.finalized.root)&&
  view.finalized.epoch<=UINT64_MAX/rules.slots_per_epoch);
 if(checkpoint)checkpoint();
 std::vector<ConsensusRoot> ordered;ordered.reserve(view.nodes.size());
 std::map<ConsensusRoot,std::vector<ConsensusRoot>> children;
 Phase0ForkChoiceResult out{view.justified.root,{}, {}};
 for(const auto& [root,node]:view.nodes){
  if(checkpoint)checkpoint();
  require(node.slot/rules.slots_per_epoch<=view.current_epoch&&node.realized_justified_epoch<=node.slot/rules.slots_per_epoch&&node.unrealized_justified_epoch<=node.slot/rules.slots_per_epoch);
  if(root!=view.anchor){const auto parent=view.nodes.find(node.parent);require(parent!=view.nodes.end()&&parent->second.slot<node.slot);children[node.parent].push_back(root);}
  ordered.push_back(root);out.weights[root]=0;
 }
 std::sort(ordered.begin(),ordered.end(),[&](const auto& a,const auto& b){const auto sa=view.nodes.at(a).slot,sb=view.nodes.at(b).slot;return sa==sb?a<b:sa<sb;});
 // Strictly decreasing parent slots make cycles impossible. Requiring every
 // non-anchor parent ensures all nodes are connected to the trusted anchor.
 require(ordered.front()==view.anchor);
 std::map<ConsensusRoot,ConsensusRoot> finalized_ancestor;
 std::set<ConsensusRoot> below_justified;
 const auto finalized_slot=view.finalized.epoch*rules.slots_per_epoch;
 for(const auto& root:ordered){
  const auto& node=view.nodes.at(root);
  if(node.slot<=finalized_slot||root==view.anchor)finalized_ancestor[root]=root;
  else finalized_ancestor[root]=finalized_ancestor.at(node.parent);
  if(root==view.justified.root||(root!=view.anchor&&below_justified.contains(node.parent)))below_justified.insert(root);
 }
 // Viability is a LEAF property propagated to ancestors, not a per-node prune.
 for(auto it=ordered.rbegin();it!=ordered.rend();++it){
  if(checkpoint)checkpoint();const auto& root=*it;if(!below_justified.contains(root))continue;
  const auto& node=view.nodes.at(root);bool viable=false;
  if(children[root].empty()){
   const auto source=view.current_epoch>node.slot/rules.slots_per_epoch?node.unrealized_justified_epoch:node.realized_justified_epoch;
   const bool recent=source>=view.current_epoch||view.current_epoch-source<=2;
   const bool justified=view.justified.epoch==0||source==view.justified.epoch||recent;
   const bool finalized=view.finalized.epoch==0||finalized_ancestor.at(root)==view.finalized.root;
   viable=justified&&finalized;
  }else for(const auto& child:children[root])viable=viable||out.viable.contains(child);
  if(viable)out.viable.insert(root);
 }
 Int total_active=0;std::set<std::array<uint8_t,48>> keys;uint64_t expected=0;
 for(const auto& [index,v]:view.justified_registry.validators){
  if(checkpoint)checkpoint();require(index==expected++&&keys.insert(v.public_key).second&&v.activation_epoch<=v.exit_epoch&&v.exit_epoch<=v.withdrawable_epoch);
  const bool active=v.activation_epoch<=view.justified_registry.epoch&&view.justified_registry.epoch<v.exit_epoch;
  if(active)total_active+=Int(std::to_string(v.effective_balance));
  const auto message=view.latest_messages.find(index);
  if(message!=view.latest_messages.end()){
   require(message->second.epoch<=view.current_epoch&&view.nodes.contains(message->second.root));
   if(active&&!v.slashed&&!view.equivocating_indices.contains(index))out.weights.at(message->second.root)+=Int(std::to_string(v.effective_balance));
  }
 }
 // Messages and equivocators from validators added after this checkpoint are
 // ignored: only the justified state's active set supplies voting weight.
 if(view.proposer_boost&&*view.proposer_boost!=ConsensusRoot{}){
  require(view.nodes.contains(*view.proposer_boost));total_active=std::max(total_active,Int(std::to_string(rules.effective_balance_increment)));
  out.weights.at(*view.proposer_boost)+=(total_active/Int(std::to_string(rules.slots_per_epoch)))*Int(std::to_string(rules.proposer_score_boost))/100;
 }
 for(auto it=ordered.rbegin();it!=ordered.rend();++it){if(checkpoint)checkpoint();if(*it!=view.anchor)out.weights.at(view.nodes.at(*it).parent)+=out.weights.at(*it);}
 for(;;){
  if(checkpoint)checkpoint();std::optional<ConsensusRoot> best;
  for(const auto& child:children[out.head])if(out.viable.contains(child)&&(!best||out.weights.at(child)>out.weights.at(*best)||(out.weights.at(child)==out.weights.at(*best)&&child>*best)))best=child;
  if(!best)break;out.head=*best;
 }
 return out;
}
}
