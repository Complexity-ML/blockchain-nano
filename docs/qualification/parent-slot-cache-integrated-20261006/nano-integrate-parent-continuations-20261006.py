from pathlib import Path
repo=Path('/Users/boris/Dev/blockchain-nano');h=repo/'include/nano/consensus_capella_reactor.hpp';p=repo/'src/consensus_capella_reactor.cpp'
s=h.read_text();old=''' // One ephemeral incoming-block continuation, owned under mutex_. Authenticated
 // parent/profile, no peer-supplied state, no journal or durable writes while
 // incomplete. Each retry advances at most one epoch within replay_slots.
 std::optional<CapellaBlockPreparation> incoming_preparation_;''';new=''' // Bounded ephemeral continuations keyed by admitted parent and block slot.
 // Alternating parents or target slots never reset another retained prefix.
 // No peer state, signing or durable writes while incomplete; eviction only
 // discards recomputable work. All entries remain owned under mutex_.
 std::vector<std::pair<uint64_t,CapellaBlockPreparation>> incoming_preparations_;''';assert old in s;h.write_text(s.replace(old,new))
s=p.read_text();a=s.index('  if(local_preparation_&&local_preparation_->matches(*state_,rules_,header.slot,check)&&local_preparation_->ready()){');b=s.index('  // Continuations bind the admitted target',a)
s=s[:a]+'''  std::erase_if(incoming_preparations_,[&](const auto& entry){
   const auto root=consensus_beacon_header_root(entry.second.parent());
   const auto retained=state_->blocks.find(root);
   return retained==state_->blocks.end()||retained->second.header!=entry.second.parent();
  });
  auto incoming=std::find_if(incoming_preparations_.begin(),incoming_preparations_.end(),[&](const auto& entry){return entry.first==header.slot&&entry.second.parent()==parent.header;});
  if(incoming==incoming_preparations_.end()){
   const auto capacity=std::min(limits_.blocks,size_t(rules_.transition.consensus.body.attestations)+2);
   if(!capacity)throw std::invalid_argument("Invalid incoming preparation capacity");
   if(incoming_preparations_.size()>=capacity)incoming_preparations_.erase(incoming_preparations_.begin());
   incoming_preparations_.emplace_back(header.slot,CapellaBlockPreparation(parent.state,parent.header,rules_.transition,check));
  }else std::rotate(incoming,std::next(incoming),incoming_preparations_.end());
  auto& preparation=incoming_preparations_.back().second;
  if(local_preparation_&&local_preparation_->matches(*state_,rules_,header.slot,check)&&local_preparation_->ready()){
   const auto* local=local_preparation_->block();
   if(local&&local->parent()==parent.header&&local->prepared_slot()==header.slot-1)preparation=*local;
  }
  if(!preparation.advance_for_block(header.slot,std::min(limits_.replay_slots,slots),limits_.sync_trials,check))return false;
''' +s[b:]
s=s.replace('prepare_capella_contextual_received_block(*state_,wire,rules_,*incoming_preparation_,targets,','prepare_capella_contextual_received_block(*state_,wire,rules_,preparation,targets,');assert 'incoming_preparation_' not in s;p.write_text(s)
print('Parent/slot continuation candidate integrated; class layout changed, all dependent translation units require rebuild.')
