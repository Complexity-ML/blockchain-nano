// Included by the synthetic Capella transition runner; never runtime code.
static void qualify_slashing_reorg(const CapellaForkStore& initial,const CapellaForkBlockRules& rules,
 const CapellaStoreSnapshotLimits& limits,const Block& genesis,const State& genesis_state,
 const IndexedConsensusAttestation& first,const IndexedConsensusAttestation& second){
 Temp directory;const auto& t=rules.transition;const auto& c=t.consensus;const auto slots=c.epoch.ffg.slots_per_epoch;
 check(first.indices.size()==1,"single-offender reorg fixture required");const auto offender=first.indices.front();
 Genesis g;g.chain_id=t.chain_id;g.consensus="beacon";g.evm_revision=EvmRevision::Shanghai;g.block=genesis;g.state=genesis_state;
 std::array<std::unique_ptr<ChainStore>,2>witnesses;std::array<std::unique_ptr<Blockchain>,2>owners;std::array<std::unique_ptr<CapellaDurableReactor>,2>nodes;
 for(size_t i=0;i<2;++i){witnesses[i]=std::make_unique<ChainStore>(directory/("witness-"+std::to_string(i)));owners[i]=std::make_unique<Blockchain>(directory/("owner-"+std::to_string(i)),g);nodes[i]=std::make_unique<CapellaDurableReactor>(*owners[i],rules,limits,initial,std::function<void()>{},witnesses[i].get());check(nodes[i]->slashing(first,second),"reorg proof admission");}
 const auto proof_archive=nodes[0]->signed_slashings();check(proof_archive.size()==1&&nodes[1]->signed_slashings()==proof_archive,"reorg proof archive mismatch");
 auto epoch_version=[&](const Bytes& state,uint64_t epoch){ConsensusBytes4 v{};std::copy_n(state.begin()+(epoch<little(state,56,8)?48:52),4,v.begin());return v;};
 auto signed_candidate=[&](const CapellaForkStore& store,uint64_t slot,uint64_t proposer,const Bytes& body){
  const auto& parent=capella_fork_head(store,rules);const auto root=consensus_beacon_header_root(parent.header);
  const auto built=prepare_capella_unsigned_block(parent.state,slot,proposer,root,body,t,parent.execution_block.header,parent.execution_state,[&](const Int& n){return capella_execution_history(store,root,n);},limits.replay_slots,limits.sync_trials);check(bool(built),"slashing reorg candidate invalid");
  const auto& message=built->message_ssz;ConsensusBeaconHeader header{little(message,0,8),little(message,8,8),root_at(message,16),root_at(message,48),capella_beacon_body_root(body,c.body,c.state.sync_committee_size,t.execution,t.withdrawals.maximum_per_payload,t.maximum_bls_changes,c.maximum_block_bytes)};
  const auto signature=Key(proposer).sign(consensus_signing_root(consensus_beacon_header_root(header),consensus_domain({0,0,0,0},epoch_version(parent.state.ssz,slot/slots),root_at(parent.state.ssz,8))));
  Bytes wire(100);put(wire,0,100,4);std::copy(signature.begin(),signature.end(),wire.begin()+4);wire.insert(wire.end(),message.begin(),message.end());return std::pair{wire,header};
 };
 auto body_for=[&](CapellaDurableReactor& node,uint64_t slot){
  const auto snapshot=node.current();const auto& h=capella_fork_head(*snapshot.state,rules);const auto identity=prepare_capella_proposer_identity(h.state,h.header,slot,{c.epoch,t.execution,t.maximum_state_bytes},c.state,c.attestation.duties,limits.replay_slots,limits.sync_trials);
  check(identity&&identity->index!=offender,"reorg fixture selected the offender proposer");ConsensusRoot epoch{};for(unsigned j=0;j<8;++j)epoch[j]=uint8_t((slot/slots)>>(8*j));
  const auto reveal=Key(identity->index).sign(consensus_signing_root(epoch,consensus_domain({2,0,0,0},epoch_version(h.state.ssz,slot/slots),root_at(h.state.ssz,8))));
  const auto body=node.prepare_local_body(identity->index,slot,reveal,h.execution_block.header.coinbase,snapshot.token,{},16,64);check(bool(body),"reorg local body unavailable");return std::pair{*body,identity->index};
 };
 const auto clock=initial.timed.clock.genesis_time_ms;const auto proposal_slot=(initial.timed.clock.time_ms-clock)/rules.slot_duration_ms+1;
 check(proposal_slot%slots!=0,"reorg fixture must not cross an epoch boundary");
 for(auto& n:nodes)n->tick(clock+proposal_slot*rules.slot_duration_ms);
 const auto prepared=nodes[0]->current();auto [body,proposer]=body_for(*nodes[0],proposal_slot);
 const auto start=little(body,204,4),end=little(body,208,4);check(end>start,"local body omitted slashing");
 const auto with=signed_candidate(*prepared.state,proposal_slot,proposer,body);
 auto clean=body;clean.erase(clean.begin()+start,clean.begin()+end);const size_t sync=(c.state.sync_committee_size+7)/8+96;
 for(const auto at:std::array<size_t,5>{208,212,216,220+sync,224+sync})put(clean,at,little(clean,at,4)-(end-start),4);
 const auto without=signed_candidate(*prepared.state,proposal_slot,proposer,clean);
 const auto a=consensus_beacon_header_root(with.second),b=consensus_beacon_header_root(without.second);check(a!=b,"slashing siblings identical");
 // Two conflicting proposals are intentionally signed with PUBLIC fixture keys.
 // This constructs an adversarial fork without bypassing admission validation.
 check(nodes[0]->block(with.first)&&nodes[0]->block(without.first),"first owner sibling admission");check(nodes[1]->block(without.first)&&nodes[1]->block(with.first),"second owner sibling admission");
 for(auto& n:nodes)n->tick(clock+proposal_slot*rules.slot_duration_ms+rules.attestation_due_ms);
 const auto voting=nodes[0]->current();const auto duties=prepare_capella_attestation_duties(*voting.state,rules,proposal_slot,limits.replay_slots,limits.sync_trials);
 std::vector<CapellaAttestationDuty> voters;for(const auto& [index,duty]:duties)if(index!=offender)voters.push_back(duty);check(voters.size()>=6,"reorg needs six distinct non-offender voters");
 const auto a_state=voting.state->blocks.at(a).state,b_state=voting.state->blocks.at(b).state;
 auto registry=[&](const CapellaForkBlock& block){return capella_fork_block_context(block.state,block.header,{c.epoch,t.execution,t.maximum_state_bytes},c.state).registry;};
 const auto ar=registry(voting.state->blocks.at(a)),br=registry(voting.state->blocks.at(b));
 const auto penalty=br.validators.at(offender).effective_balance/c.epoch.slashing.minimum_penalty_quotient;
 check(ar.validators.at(offender).slashed&&!br.validators.at(offender).slashed&&br.validators.at(offender).balance-ar.validators.at(offender).balance==penalty,"single slashing penalty differs between sibling states");
 for(auto& n:nodes)n->tick(clock+(proposal_slot+1)*rules.slot_duration_ms); // Clear proposer boost before vote-driven reorgs.
 auto vote=[&](size_t i,const ConsensusRoot& root){auto duty=voters.at(i);auto att=duty.attestation;att.data.beacon_block_root=root;att.signature=Key(duty.validator_index).sign(consensus_signing_root(consensus_attestation_root(att.data),consensus_domain({1,0,0,0},duty.fork_version,duty.genesis_root)));for(auto& n:nodes)check(n->vote(att),"signed reorg vote rejected");};
 unsigned reopen_count=0;
 auto verify=[&](const ConsensusRoot& expected,bool slashed,bool reopen){
  for(size_t i=0;i<2;++i){const auto before=nodes[i]->current();
   if(reopen){nodes[i].reset();owners[i].reset();owners[i]=std::make_unique<Blockchain>(directory/("owner-"+std::to_string(i)),g);nodes[i]=std::make_unique<CapellaDurableReactor>(*owners[i],rules,limits,*witnesses[i]);++reopen_count;check(nodes[i]->current().token==before.token,"reorg witness reopen changed token");}
   const auto now=nodes[i]->current();const auto& h=capella_fork_head(*now.state,rules);check(consensus_beacon_header_root(h.header)==expected,"votes did not select expected slashing branch");
   check(h.state==(slashed?a_state:b_state)&&registry(h).validators.at(offender).slashed==slashed,"slashing reorg left stale registry or balances");
   check(owners[i]->tip().header.hash()==h.execution_block.header.hash()&&owners[i]->state().root()==h.execution_state.root(),"slashing reorg detached EVM owner");
   check(nodes[i]->signed_slashings()==proof_archive,"unfinalized slashing proof lost");const auto token=now.token;
   check(nodes[i]->slashing(second,first)&&nodes[i]->current().token==token,"proof replay reapplied penalty or changed token");
  }
 };
 vote(0,a);verify(a,true,true);vote(1,b);vote(2,b);verify(b,false,true);
 // Reinclude the orphaned proof in a separate durable owner, leaving the live
 // sibling test untouched for the reverse branch switch below.
 {ChainStore witness(directory/"reinclusion-witness");auto owner=std::make_unique<Blockchain>(directory/"reinclusion",g);const auto snapshot=nodes[0]->current();auto node=std::make_unique<CapellaDurableReactor>(*owner,rules,limits,*snapshot.state,std::function<void()>{},&witness);check(node->slashing(first,second),"reinclusion proof admission");
  const auto slot=proposal_slot+1;auto [selected,index]=body_for(*node,slot);check(little(selected,208,4)>little(selected,204,4),"orphaned proof not automatically reselected");
  const auto candidate=signed_candidate(*node->current().state,slot,index,selected);check(node->block(candidate.first),"reinclusion signed block rejected");
  const auto applied=node->current();const auto& h=capella_fork_head(*applied.state,rules);check(consensus_beacon_header_root(h.header)==consensus_beacon_header_root(candidate.second)&&registry(h).validators.at(offender).slashed,"reinclusion failed to apply sanction");
  check(node->signed_slashings()==proof_archive,"reincluded unfinalized proof discarded");const auto exact=h.state;
  check(node->block(candidate.first)&&node->slashing(second,first)&&node->current().token==applied.token,"replay changed reinclusion checkpoint");
  node.reset();owner.reset();owner=std::make_unique<Blockchain>(directory/"reinclusion",g);node=std::make_unique<CapellaDurableReactor>(*owner,rules,limits,witness);++reopen_count;check(node->current().token==applied.token&&capella_fork_head(*node->current().state,rules).state==exact,"reinclusion restart changed penalty");
  node->tick(clock+(slot+1)*rules.slot_duration_ms);
  const auto [next,index_next]=body_for(*node,slot+1);check(little(next,204,4)==little(next,208,4),"already applied proof selected for a second penalty");
 }
 vote(3,a);vote(4,a);vote(5,a);verify(a,true,true);
 std::cout<<Json{{"scenario","capella-slashing-reorg"},{"offender",offender},{"penaltyGwei",penalty},{"branchSwitches",2},{"ownerReopens",reopen_count},{"owners",2},{"proofRetained",true},{"automaticReinclusion",true},{"duplicateDidNotReapply",true},{"networkTransport",false},{"injectedWriteFailure",false}}.dump()<<std::endl;
}
