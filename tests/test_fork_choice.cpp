#include "nano/consensus_fork_choice.hpp"
#include <iostream>
#include <random>
using namespace nano;
ConsensusRoot root(uint64_t n){ConsensusRoot r{};for(int i=0;i<8;++i)r[31-i]=uint8_t(n>>(8*i));return r;}
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
Phase0ForkChoiceView base(){
 Phase0ForkChoiceView v{};v.anchor=root(1);v.justified={0,root(1)};v.finalized=v.justified;v.current_epoch=0;v.justified_registry.epoch=0;
 v.nodes[root(1)]={0,{},0,0};v.nodes[root(2)]={1,root(1),0,0};v.nodes[root(3)]={1,root(1),0,0};v.nodes[root(4)]={2,root(2),0,0};
 for(uint64_t i=0;i<3;++i){ConsensusSlashingValidator x;x.public_key[0]=uint8_t(i+1);x.activation_epoch=0;x.effective_balance=32;v.justified_registry.validators[i]=x;}
 return v;
}
int main(){try{
 const Phase0ForkChoiceRules rules{8,1,40};auto v=base();
 check(phase0_fork_choice_head(v,rules).head==root(3),"lexicographic tie");
 v.latest_messages[0]={0,root(4)};auto r=phase0_fork_choice_head(v,rules);
 check(r.head==root(4)&&r.weights.at(root(2))==32&&r.weights.at(root(1))==32,"descendant vote propagates");
 v.latest_messages[1]={0,root(3)};check(phase0_fork_choice_head(v,rules).head==root(3),"weighted tie");
 v.latest_messages[99]={0,root(4)};check(phase0_fork_choice_head(v,rules).head==root(3),"post-checkpoint validator ignored");
 v.equivocating_indices.insert(1);check(phase0_fork_choice_head(v,rules).head==root(4),"equivocator excluded");
 v.equivocating_indices.clear();v.justified_registry.validators[1].slashed=true;check(phase0_fork_choice_head(v,rules).head==root(4),"slashed excluded");
 v.justified_registry.validators[1].slashed=false;v.justified_registry.validators[1].activation_epoch=1;check(phase0_fork_choice_head(v,rules).head==root(4),"inactive excluded");
 v=base();v.proposer_boost=root(4);r=phase0_fork_choice_head(v,rules);check(r.head==root(4)&&r.weights.at(root(2))==4,"boost division order");
 v.justified_registry.validators[0].slashed=true;r=phase0_fork_choice_head(v,rules);check(r.weights.at(root(2))==4,"slashed active contributes boost denominator");
 v.proposer_boost=ConsensusRoot{};check(phase0_fork_choice_head(v,rules).head==root(3),"zero boost sentinel");
 v=base();v.justified_registry.validators.clear();v.proposer_boost=root(4);check(phase0_fork_choice_head(v,{8,100,40}).weights.at(root(4))==4,"minimum total active balance floor");
 v=base();for(auto& [i,x]:v.justified_registry.validators){x.effective_balance=UINT64_MAX;v.latest_messages[i]={0,root(4)};}
 check(phase0_fork_choice_head(v,rules).weights.at(root(1))==Int("18446744073709551615")*3,"wide weight sum");
 v=base();v.current_epoch=5;v.justified.epoch=2;v.justified_registry.epoch=2;
 v.nodes[root(1)]={16,{},2,2};v.nodes[root(2)]={24,root(1),0,0};v.nodes[root(3)]={32,root(1),0,2};v.nodes[root(4)]={40,root(2),3,0};
 r=phase0_fork_choice_head(v,rules);check(r.viable.contains(root(2))&&r.viable.contains(root(4)),"viable leaf preserves stale parent");
 v.nodes[root(4)].realized_justified_epoch=1;r=phase0_fork_choice_head(v,rules);check(!r.viable.contains(root(2))&&r.head==root(3),"stale leaf removed and pulled-up source used");
 v.nodes[root(4)].realized_justified_epoch=3;v.finalized={2,root(1)};r=phase0_fork_choice_head(v,rules);check(r.viable.contains(root(4)),"finalized ancestor accepted");
 v.nodes[root(5)]={16,root(1),2,2};bool rejected=false;try{phase0_fork_choice_head(v,rules);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"non-increasing parent rejected");
 v=base();v.nodes[root(2)].parent=root(99);rejected=false;try{phase0_fork_choice_head(v,rules);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"unknown parent rejected");
 v=base();int calls=0;bool cancelled=false;try{phase0_fork_choice_head(v,rules,[&]{if(++calls==5)throw std::runtime_error("cancel");});}catch(const std::runtime_error&){cancelled=true;}check(cancelled&&phase0_fork_choice_head(v,rules).head==root(3),"cancel leaves input usable");
 v=base();v.nodes.clear();v.nodes[root(1)]={0,{},0,0};v.current_epoch=2500;for(uint64_t i=2;i<=20000;++i)v.nodes[root(i)]={i-1,root(i-1),0,0};
 check(phase0_fork_choice_head(v,rules).head==root(20000),"deep tree iterative traversal");
 // Independent slow ancestor scans check aggregation against the direct
 // reference definition on deterministic, locally generated trees (not vectors).
 std::mt19937_64 rng(20260925);
 for(unsigned trial=0;trial<100;++trial){
  v=base();v.nodes.clear();v.nodes[root(1)]={0,{},0,0};v.current_epoch=20;
  for(uint64_t i=2;i<=100;++i)v.nodes[root(i)]={i,root(1+rng()%(i-1)),0,0};
  v.justified_registry.validators.clear();
  Int total=0;
  for(uint64_t i=0;i<64;++i){ConsensusSlashingValidator x;x.public_key[0]=uint8_t(i+1);x.activation_epoch=(rng()%4==0?1:0);x.slashed=rng()%5==0;x.effective_balance=rng()%1000;v.justified_registry.validators[i]=x;if(x.activation_epoch==0)total+=Int(std::to_string(x.effective_balance));v.latest_messages[i]={0,root(1+rng()%100)};if(rng()%7==0)v.equivocating_indices.insert(i);}
  v.proposer_boost=root(1+rng()%100);r=phase0_fork_choice_head(v,rules);
  auto descendant=[&](ConsensusRoot child,const ConsensusRoot& parent){while(child!=parent&&child!=v.anchor)child=v.nodes.at(child).parent;return child==parent;};
  for(const auto& [node,ignored]:v.nodes){Int want=0;for(const auto& [i,x]:v.justified_registry.validators)if(x.activation_epoch==0&&!x.slashed&&!v.equivocating_indices.contains(i)&&descendant(v.latest_messages.at(i).root,node))want+=Int(std::to_string(x.effective_balance));if(descendant(*v.proposer_boost,node))want+=(std::max(total,Int(1))/8)*40/100;check(r.weights.at(node)==want,"generated direct weight oracle");}
  auto head=v.anchor;for(;;){std::optional<ConsensusRoot> next;for(const auto& [node,b]:v.nodes)if(node!=v.anchor&&b.parent==head&&(!next||r.weights.at(node)>r.weights.at(*next)||(r.weights.at(node)==r.weights.at(*next)&&node>*next)))next=node;if(!next)break;head=*next;}check(r.head==head,"generated head oracle");
 }
 std::cout<<"Phase0 fork-choice snapshot checks passed; event handlers and network integration not qualified\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
