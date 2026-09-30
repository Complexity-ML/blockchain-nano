// Included inside the real native deposit/activation fixture.
std::optional<std::pair<IndexedConsensusAttestation,IndexedConsensusAttestation>> new_validator_proof;
CapellaSignedVotes new_validator_archive;
bool new_validator_unknown_reopened=false,new_validator_finalized_slash=false;
size_t new_validator_unknown_observations=0,new_validator_unknown_finality_advances=0,new_validator_skipped_proposals=0;
ConsensusRoot new_validator_last_final{};
size_t new_validator_forged_checks=0;
auto reject_forged_new_validator_proofs=[&](const IndexedConsensusAttestation& first,const IndexedConsensusAttestation& second,const char* phase){
 for(const std::string mode:{"bad-signature","wrong-known-key","unknown-index","duplicate-index"}){
  auto a=first,b=second;
  if(mode=="bad-signature")a.signature[0]^=1;
  else if(mode=="wrong-known-key"){a.indices={0};b.indices={0};}
  else if(mode=="unknown-index"){a.indices={65};b.indices={65};}
  else{a.indices={64,64};b.indices={64,64};}
  for(auto& node:nodes){
   const auto before=node->current();const auto archive=node->signed_slashings();bool accepted=false;
   try{accepted=node->slashing(a,b);}catch(const std::invalid_argument&){}
   check(!accepted&&node->current().token==before.token&&node->signed_slashings()==archive,"forged new validator proof accepted or mutated checkpoint/archive");
   ++new_validator_forged_checks;
  }
 }
 std::cout<<Json{{"scenario","new-validator-forged-proofs"},{"phase",phase},{"cases",4},{"owners",2},{"checks",new_validator_forged_checks}}.dump()<<std::endl;
};
auto observe_new_validator_slashing=[&](uint64_t slot){
 if(!slashing_new_validator)return;
 const auto current=nodes[0]->current();
 auto context=[&](const CapellaForkStore& store,const ConsensusRoot& root){const auto& b=store.blocks.at(root);return capella_fork_block_context(b.state,b.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);};
 const auto justified=context(*current.state,current.state->timed.view.justified.root),finalized=context(*current.state,current.state->timed.view.finalized.root);
 if(!new_validator_proof){
  IndexedConsensusAttestation a{};a.indices={64};a.data.slot=current.state->timed.view.justified.epoch*slots;a.data.source=current.state->timed.view.finalized;a.data.target=current.state->timed.view.justified;a.data.beacon_block_root=current.state->timed.view.justified.root;auto b=a;b.data.beacon_block_root[0]^=1;
  const auto version=a.data.target.epoch<justified.fork.epoch?justified.fork.previous_version:justified.fork.current_version;
  for(auto* att:{&a,&b})att->signature=Key(64).sign(consensus_signing_root(consensus_attestation_root(att->data),consensus_domain({1,0,0,0},version,justified.genesis_root)));
  if(!justified.registry.validators.contains(64)){
   for(auto& node:nodes){const auto token=node->current().token;check(!node->slashing(a,b)&&node->current().token==token&&node->signed_slashings().empty(),"unregistered slashing admitted or mutated archive");}
   return;
  }
  check(!finalized.registry.validators.contains(64),"fixture missed justified/finalized registration gap");
  reject_forged_new_validator_proofs(a,b,"before-valid-admission");
  for(auto& node:nodes)check(node->slashing(a,b),"new justified validator proof rejected");
  new_validator_proof=std::pair{a,b};new_validator_archive=nodes[0]->signed_slashings();check(new_validator_archive.size()==1,"new validator proof missing");
 }
 for(size_t i=0;i<2;++i){
  const auto before=nodes[i]->current();const auto final=context(*before.state,before.state->timed.view.finalized.root);const auto member=final.registry.validators.find(64);const bool unknown=member==final.registry.validators.end(),retired=!unknown&&member->second.slashed;
  check(nodes[i]->signed_slashings()==(retired?CapellaSignedVotes{}:new_validator_archive),"new validator proof retired prematurely or retained after finalized slashing");
  check(nodes[i]->slashing(new_validator_proof->second,new_validator_proof->first)&&nodes[i]->current().token==before.token,"new validator reverse replay changed checkpoint");
  if(unknown){++new_validator_unknown_observations;if(i==0&&new_validator_last_final!=ConsensusRoot{}&&new_validator_last_final!=before.state->timed.view.finalized.root)++new_validator_unknown_finality_advances;}
  new_validator_finalized_slash|=retired;
 }
 if(!new_validator_unknown_reopened){
  for(size_t i=0;i<2;++i){const auto before=nodes[i]->current();const auto& h=capella_fork_head(*before.state,rules);const auto state=h.state;nodes[i].reset();owners[i].reset();owners[i]=std::make_unique<Blockchain>(directory/("node-"+std::to_string(i)),owner_genesis);nodes[i]=std::make_unique<CapellaDurableReactor>(*owners[i],rules,limits,before.token);nodes[i]->set_automatic_pruning(true);const auto after=nodes[i]->current();check(after.token==before.token&&capella_fork_head(*after.state,rules).state==state&&nodes[i]->signed_slashings()==new_validator_archive,"unknown finalized validator proof lost on restart");check(!context(*after.state,after.state->timed.view.finalized.root).registry.validators.contains(64),"restart did not cover missing finalized member");}
  reject_forged_new_validator_proofs(new_validator_proof->first,new_validator_proof->second,"after-both-owners-reopened");
  new_validator_unknown_reopened=true;
  std::cout<<Json{{"scenario","new-validator-proof-admission"},{"slot",slot},{"justifiedEpoch",current.state->timed.view.justified.epoch},{"finalizedEpoch",current.state->timed.view.finalized.epoch},{"unknownFinalizedMember",true},{"bothOwnersReopened",true}}.dump()<<std::endl;
 }
 new_validator_last_final=nodes[0]->current().state->timed.view.finalized.root;
};
