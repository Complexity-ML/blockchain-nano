from pathlib import Path
import hashlib,json
repo=Path('/Users/boris/Dev/blockchain-nano');out=Path('/private/tmp/nano-incoming-alternation-native-20261006');p=repo/'src/consensus_capella_reactor.cpp';s=p.read_text();(out/'baseline-reactor.cpp').write_text(s)
old='''  if(incoming_target_block_!=id){incoming_targets_.clear();incoming_target_block_=id;}
  if(std::any_of(incoming_targets_.begin(),incoming_targets_.end(),[&](const auto& frame){return !frame.matches(*state_,rules_,frame.target(),check);}))incoming_targets_.clear();
  for(;;){
   std::vector<const CapellaTargetPreparation*> targets;
   for(auto& frame:incoming_targets_){
    if(!frame.advance(std::min(limits_.replay_slots,slots),limits_.sync_trials,check))return false;
    targets.push_back(&frame);
   }
'''
new='''  // Continuations bind the admitted target root/epoch/profile, not the wire
  // currently requesting them. Alternating siblings must not erase that work.
  incoming_target_block_=id;
  std::erase_if(incoming_targets_,[&](const auto& frame){return !frame.matches(*state_,rules_,frame.target(),check);});
  for(;;){
   std::vector<const CapellaTargetPreparation*> targets;
   for(const auto& frame:incoming_targets_)if(frame.ready())targets.push_back(&frame);
'''
assert s.count(old)==1;s=s.replace(old,new)
old='''    if(incoming_targets_.size()>=size_t(rules_.transition.consensus.body.attestations)+2)throw std::length_error("Incoming Capella target cache capacity");
    incoming_targets_.emplace_back(*state_,rules_,needed.target(),check);
    auto& frame=incoming_targets_.back();bool reused=false;
    for(const auto& ready:vote_preparations_)if(ready&&ready->ready()&&ready->matches(*state_,rules_,needed.target(),check)){frame=*ready;reused=true;break;}
'''
new='''    auto found=std::find_if(incoming_targets_.begin(),incoming_targets_.end(),[&](const auto& frame){return frame.target()==needed.target();});
    if(found==incoming_targets_.end()){
     // Ephemeral cache pressure only evicts recomputable work, never an admitted
     // state, archive, signing reservation or durable checkpoint.
     if(incoming_targets_.size()>=size_t(rules_.transition.consensus.body.attestations)+2)incoming_targets_.erase(incoming_targets_.begin());
     incoming_targets_.emplace_back(*state_,rules_,needed.target(),check);
    }else std::rotate(found,std::next(found),incoming_targets_.end());
    auto& frame=incoming_targets_.back();bool reused=false;
    for(const auto& ready:vote_preparations_)if(ready&&ready->ready()&&ready->matches(*state_,rules_,needed.target(),check)){frame=*ready;reused=true;break;}
'''
assert s.count(old)==1;s=s.replace(old,new);p.write_text(s)
p=repo/'include/nano/consensus_capella_reactor.hpp';s=p.read_text().replace('// Frames are scoped to one structurally checked incoming block. Targets are\n // discovered only after its full signed transition; never accepted from peers.','// Bounded shared target continuations, bound to admitted root/epoch/profile.\n // Required contexts come from checked block processing, never peer state.\n // Last requested block does not invalidate authenticated continuation work.');p.write_text(s)
(out/'sources-after-fix.json').write_text(json.dumps({str(p.relative_to(repo)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [repo/'src/consensus_capella_reactor.cpp',repo/'include/nano/consensus_capella_reactor.hpp',repo/'tests/test_capella_transition.cpp']},indent=2)+'\n')
