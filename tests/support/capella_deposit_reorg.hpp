#ifdef NANO_SYNC_FAULT
static int deposit_vote_sync_writer(const fs::path& directory){
 const auto bundle=load(directory/"bundle.json");const auto profile=bundle.at("profile").get<std::string>();const auto rules=load_capella_fork_profile(profile,sha256(Bytes(profile.begin(),profile.end())),1024*1024);auto limits=crash_limits;limits.blocks=bundle.at("blocks").get<size_t>();
 Genesis g;g.chain_id=rules.transition.chain_id;g.consensus="beacon";g.evm_revision=EvmRevision::Shanghai;g.block=Block::from_json(bundle.at("genesis"));g.state=State::from_json(bundle.at("alloc"));
 Blockchain owner(directory/"db",g);const auto& disk=owner.store();const auto record=disk.fork_checkpoint(g.chain_id,g.block.header.hash(),limits.envelope_bytes);check(bool(record),"reorg writer checkpoint missing");CapellaDurableReactor node(owner,rules,limits,record->token);
 const auto wire=unhex(bundle.at("vote").get<std::string>());const auto vote=decode_consensus_attestation_ssz(wire,rules.transition.consensus.body.validators_per_committee);check(bool(vote),"reorg vote decode");
 fs::path wal;for(const auto& entry:fs::directory_iterator(directory/"db"))if(entry.path().extension()==".log"&&(wal.empty()||entry.path().filename()>wal.filename()))wal=entry.path();check(!wal.empty(),"reorg WAL missing");const auto before=fs::file_size(wal);
 nano_test_arm_sync_fault(wal.c_str());bool failed=false;try{node.vote(*vote);}catch(const std::runtime_error&){failed=true;}check(failed&&nano_test_sync_failures()>0,"reorg sync fault not reached");
 unsigned blocked=0;auto refuse=[&](auto action){try{action();}catch(const std::runtime_error&){++blocked;}};
 refuse([&]{node.current();});refuse([&]{node.current_validator_indices({Key(64).public_key});});refuse([&]{owner.tip();});refuse([&]{owner.state();});refuse([&]{disk.latest();});check(blocked==5,"uncertain reorg state remained usable");
 std::cout<<Json{{"wal",wal.filename().string()},{"before",before},{"after",fs::file_size(wal)},{"blocked",blocked}}.dump()<<std::endl;_exit(0);
}
static void qualify_deposit_vote_sync(const CapellaForkStore& before,const CapellaForkBlockRules& rules,const CapellaStoreSnapshotLimits& limits,const Genesis& g,const Phase0SignedAttestation& vote){
 const auto candidate=prepare_capella_store_vote(before,vote,rules,limits.replay_slots,limits.sync_trials);check(bool(candidate),"reorg fault candidate missing");
 const auto old_snapshot=capella_store_snapshot(before,rules,limits),new_snapshot=capella_store_snapshot(*candidate,rules,limits);
 check(capella_fork_head(before,rules)->header.state_root!=capella_fork_head(*candidate,rules)->header.state_root,"fault vote does not change head");
 std::string old_durable,new_durable;
 {Temp reference;Blockchain owner(reference/"db",g);CapellaDurableReactor node(owner,rules,limits,before);old_durable=owner.store().fork_checkpoint(g.chain_id,g.block.header.hash(),limits.envelope_bytes)->snapshot;check(node.vote(vote),"reference reorg vote refused");new_durable=owner.store().fork_checkpoint(g.chain_id,g.block.header.hash(),limits.envelope_bytes)->snapshot;check(capella_store_snapshot(*node.current().state,rules,limits)==new_snapshot,"reference state differs from pure transition");}
 for(bool lose:{false,true}){Temp temp;ForkCheckpointToken initial_token;
  {Blockchain owner(temp/"db",g);CapellaDurableReactor initial(owner,rules,limits,before);initial_token=initial.current().token;}
  save(temp/"bundle.json",Json{{"profile",capella_fork_profile(rules).dump()},{"blocks",limits.blocks},{"genesis",g.block.to_json(true)},{"alloc",g.state.to_json()},{"vote",hex(encode_consensus_attestation_ssz(vote,rules.transition.consensus.body.validators_per_committee))}});
  const auto result=success(run({executable,"--deposit-vote-sync-writer",temp.path.string()}));const auto wal=temp/"db"/result.at("wal").get<std::string>();check(result.at("after")>result.at("before"),"fault left no WAL suffix");
  if(lose){const int fd=open(wal.c_str(),O_WRONLY);check(fd>=0,"open reorg test WAL");const bool ok=ftruncate(fd,result.at("before").get<uint64_t>())==0&&fsync(fd)==0;close(fd);check(ok,"truncate reorg test WAL");}
  Blockchain owner(temp/"db",g);const auto record=owner.store().fork_checkpoint(g.chain_id,g.block.header.hash(),limits.envelope_bytes);const auto& expected=lose?old_durable:new_durable;
  check(record&&record->snapshot==expected&&record->token.sequence==initial_token.sequence+(lose?0:1),"mixed reorg checkpoint recovered");
  // Compare complete snapshots to externally computed outcomes before accepting
  // the recovered token in this isolated test, not a production trust policy.
  CapellaDurableReactor restored(owner,rules,limits,record->token);const auto index=restored.current_validator_indices({Key(64).public_key}).at(Key(64).public_key);check(index==(lose?64:65),"recovered index detached from checkpoint");check(restored.signed_votes().size()==(lose?0:1),"vote archive detached from WAL outcome");
  const auto head_pin=capella_fork_head(*restored.current().state,rules);const auto& head=*head_pin;check(owner.state().root()==head.execution_state.root()&&owner.tip().header.hash()==head.execution_block.header.hash(),"recovered execution mixed branches");
  check(restored.vote(vote),"reorg vote replay rejected");check(restored.current_validator_indices({Key(64).public_key}).at(Key(64).public_key)==65&&capella_store_snapshot(*restored.current().state,rules,limits)==new_snapshot,"replayed reorg differs");
  std::cout<<Json{{"scenario","capella-deposit-reorg-sync"},{"lostSuffix",lose},{"recoveredIndex",index},{"replayedIndex",65},{"blockedOperations",result.at("blocked")},{"physicalPowerLoss",false},{"networkTransport",false}}.dump()<<std::endl;
 }
}
#endif
// Isolated adversarial publishers deliberately sign competing blocks with separate
// journals. Never run this fixture with real validator keys or public peers.
static void deposit_reorg_loop(const CapellaForkStore& initial,const CapellaForkBlockRules& rules,const Block& genesis,const State& genesis_state){
 Temp directory;const auto& t=rules.transition;const auto slots=t.consensus.epoch.ffg.slots_per_epoch;auto limits=crash_limits;const auto period=t.consensus.state.eth1_votes_limit;check(period&&period<=64,"competing deposit period unbounded");const auto stop=period+period/2+1;limits.blocks=2*stop+(deposit_reorg_activation?(t.consensus.exit.max_seed_lookahead+12)*slots:0);
 const auto validators=root_at(initial.blocks.at(initial.timed.view.anchor).lease()->state.ssz,8);
 Genesis g;g.chain_id=t.chain_id;g.consensus="beacon";g.evm_revision=EvmRevision::Shanghai;g.block=genesis;g.state=genesis_state;
 std::array<std::unique_ptr<Blockchain>,3> owners;std::array<std::unique_ptr<CapellaDurableReactor>,3> nodes;
 for(size_t n=0;n<3;++n){owners[n]=std::make_unique<Blockchain>(directory/("owner-"+std::to_string(n)),g);nodes[n]=std::make_unique<CapellaDurableReactor>(*owners[n],rules,limits,initial);}
 std::array<std::map<uint64_t,std::unique_ptr<ChainStore>>,2> disks;
 std::array<std::map<uint64_t,std::unique_ptr<BeaconSigningJournal>>,2> journals;
 auto journal=[&](size_t n,uint64_t i)->BeaconSigningJournal&{if(!journals[n].contains(i)){disks[n][i]=std::make_unique<ChainStore>(directory/("key-"+std::to_string(n)+"-"+std::to_string(i)));journals[n][i]=std::make_unique<BeaconSigningJournal>(*disks[n][i],t.chain_id,genesis.header.hash(),validators,Key(i<64||!n?i:129-i).public_key,slots);}return *journals[n][i];};
 auto data=[&](uint64_t index){Phase0Deposit d{};Key key(index);copy_to(d.message.public_key,key.public_key);d.message.withdrawal_credentials[0]=1;d.message.withdrawal_credentials[31]=44;d.message.amount_gwei=t.consensus.deposit.maximum_effective_balance;copy_to(d.signature,key.sign(consensus_signing_root(consensus_deposit_root(d.message),consensus_domain({3,0,0,0},t.genesis_version,{}))));
  const std::string method="deposit(bytes,bytes,bytes,bytes32)";auto out=keccak(Bytes(method.begin(),method.end()));out.resize(4);Bytes args(416);auto word=[&](size_t at,size_t n){const auto v=bytes(Int(n),32);std::copy(v.begin(),v.end(),args.begin()+at);};word(0,128);word(32,224);word(64,288);const auto root=consensus_deposit_data_root(d);std::copy(root.begin(),root.end(),args.begin()+96);word(128,48);std::copy(d.message.public_key.begin(),d.message.public_key.end(),args.begin()+160);word(224,32);std::copy(d.message.withdrawal_credentials.begin(),d.message.withdrawal_credentials.end(),args.begin()+256);word(288,96);std::copy(d.signature.begin(),d.signature.end(),args.begin()+320);out.insert(out.end(),args.begin(),args.end());return out;};
 Transaction tx;tx.type=2;tx.to=ethereum_deposit_contract;tx.gas=1000000;tx.max_fee=2000000000;tx.priority_fee=100000000;tx.value=Int(std::to_string(t.consensus.deposit.maximum_effective_balance))*1000000000;
 const auto secret=bytes(Int(777),32);const auto sender=SignedTransaction::sign(tx,secret).sender();
 for(size_t n=0;n<2;++n)for(uint64_t i=0;i<2;++i){tx.nonce=owners[n]->state().get(sender).nonce+Int(std::to_string(i));tx.data=data(64+(n?1-i:i));check(owners[n]->submit(SignedTransaction::sign(tx,secret)),"competing deposit submit failed");}

 uint64_t now=initial.timed.clock.time_ms;
 for(uint64_t slot=2;slot<=stop;++slot){now=initial.timed.clock.genesis_time_ms+slot*rules.slot_duration_ms;for(auto& node:nodes)node->tick(now);
  for(size_t n=0;n<2;++n){const auto proposer=nodes[n]->current_proposer(slot);check(bool(proposer)&&*proposer<64,"competing proposer missing");const auto wire=nodes[n]->propose_current(*proposer,slot,genesis.header.coinbase,16,64,[&]{return now;},journal(n,*proposer),[&](const ConsensusRoot& root){return Key(*proposer).sign(root);});check(wire&&nodes[n]->block(*wire)&&nodes[2]->block(*wire),"competing block admission failed");}
  if(slot%8==0)std::cout<<Json{{"scenario","deposit-reorg-progress"},{"slot",slot},{"observerRecords",nodes[2]->current().state->blocks.size()}}.dump()<<std::endl;
 }
 const std::set<Bytes> requested{Key(64).public_key,Key(65).public_key};
 for(size_t n=0;n<2;++n){const auto indices=nodes[n]->current_validator_indices(requested);check(indices.at(Key(64).public_key)==64+n&&indices.at(Key(65).public_key)==65-n,"competing deposits did not assign opposite indices");}
 // A next-slot tick removes proposer boost; disjoint fresh committee voters
 // select each branch without double-voting in this observer's latest messages.
 const auto duty_time=now+rules.attestation_due_ms;for(size_t n=0;n<2;++n)nodes[n]->tick(duty_time);
 std::array<std::map<uint64_t,CapellaAttestationDuty>,2> duties;
 for(size_t n=0;n<2;++n)duties[n]=prepare_capella_attestation_duties(*nodes[n]->current().state,rules,stop,limits.replay_slots,limits.sync_trials);
 nodes[2]->tick(now+rules.slot_duration_ms);
 ChainStore local_key_disk(directory/"observer-key-64");BeaconSigningJournal local_journal(local_key_disk,t.chain_id,genesis.header.hash(),validators,Key(64).public_key,slots);size_t provider_calls=0;
 std::set<uint64_t> used;unsigned switches=0;bool stale_index_refused=false;bool reorg_sync_qualified=false;
 for(size_t n=0;n<2;++n){size_t signed_votes=0;for(const auto& [index,duty]:duties[n]){if(used.contains(index))continue;auto vote=duty.attestation;vote.signature=Key(index).sign(consensus_signing_root(consensus_attestation_root(vote.data),consensus_domain({1,0,0,0},duty.fork_version,duty.genesis_root)));
#ifdef NANO_SYNC_FAULT
 if(deposit_reorg_sync&&n==1&&!reorg_sync_qualified){const auto current=nodes[2]->current();const auto next=prepare_capella_store_vote(*current.state,vote,rules,limits.replay_slots,limits.sync_trials);check(bool(next),"sync candidate refused");if(capella_fork_head(*next,rules)->header.state_root!=capella_fork_head(*current.state,rules)->header.state_root){qualify_deposit_vote_sync(*current.state,rules,limits,g,vote);reorg_sync_qualified=true;}}
#endif
 check(nodes[2]->vote(vote),"branch selection vote rejected");used.insert(index);if(++signed_votes==1+2*n)break;}check(signed_votes==1+2*n,"insufficient distinct voters");
  const auto expected=nodes[n]->current_validator_indices(requested);check(nodes[2]->current_validator_indices(requested)==expected,"observer retained stale branch indices");const auto current=nodes[2]->current();const auto head_pin=capella_fork_head(*current.state,rules);const auto& head=*head_pin;check(head.deposit_history->snapshot()==capella_fork_head(*nodes[n]->current().state,rules)->deposit_history->snapshot(),"observer deposit history from wrong branch");check(owners[2]->state().root()==head.execution_state.root(),"observer EVM differs after reorg");
  if(n==1){const auto commitment=local_journal.commitment();auto signer=[&](const ConsensusRoot& root){++provider_calls;return Key(64).sign(root);};
   try{nodes[2]->attest_current(64,stop+1,[&]{return now+rules.slot_duration_ms;},local_journal,signer,{},Key(64).public_key);}catch(const StaleCapellaDuty&){stale_index_refused=true;}
   check(stale_index_refused&&provider_calls==0&&local_journal.commitment()==commitment&&nodes[2]->current().token==current.token,"reindexed key signed with old index or mutated journal");
   check(!nodes[2]->attest_current(65,stop+1,[&]{return now+rules.slot_duration_ms;},local_journal,signer,{},Key(64).public_key)&&provider_calls==0,"registered but inactive key signed");
  }
  nodes[2].reset();owners[2].reset();owners[2]=std::make_unique<Blockchain>(directory/"owner-2",g);nodes[2]=std::make_unique<CapellaDurableReactor>(*owners[2],rules,limits,current.token);check(nodes[2]->current_validator_indices(requested)==expected,"reopen lost branch registry");++switches;
 }
 std::cout<<Json{{"scenario","capella-competing-deposit-reorg"},{"branches",2},{"depositsPerBranch",2},{"registrySize",66},{"selectedBranches",switches},{"observerReopens",switches},{"indexA",Json::array({64,65})},{"networkTransport",false},{"activationQualified",false},{"staleIndexRefused",stale_index_refused},{"providerCalls",provider_calls}}.dump()<<std::endl;
 if(deposit_reorg_sync)check(reorg_sync_qualified,"reorg sync scenario never triggered");
 if(deposit_reorg_activation){
  for(size_t n:{1,2})nodes[n]->set_automatic_pruning(true);
  uint64_t local_clock=nodes[2]->current().state->timed.clock.time_ms;
  auto clock=std::make_unique<CapellaLocalClock>(*nodes[2],2*rules.slot_duration_ms,[&]{return local_clock;});
  auto shared_journal=std::shared_ptr<BeaconSigningJournal>(&local_journal,[](auto*){});
  std::vector<CapellaValidatorKey> local{{0,shared_journal,[&](const ConsensusRoot& root){++provider_calls;return Key(64).sign(root);},genesis.header.coinbase,Key(64).public_key}};
  auto loop=std::make_unique<CapellaValidatorLoop>(*nodes[2],*clock,rules.slot_duration_ms,slots,16,64,std::move(local),[&]{return local_clock;});
  uint64_t activation=UINT64_MAX;size_t new_votes=0;const auto end=stop+(t.consensus.exit.max_seed_lookahead+12)*slots;
  for(uint64_t slot=stop+1;slot<=end;++slot){
   now=initial.timed.clock.genesis_time_ms+slot*rules.slot_duration_ms;for(size_t n:{1,2})nodes[n]->tick(now);
   const auto proposer=nodes[1]->current_proposer(slot);check(bool(proposer),"post-reorg proposer absent");const auto signing_key=*proposer<64?*proposer:129-*proposer;
   const auto wire=nodes[1]->propose_current(*proposer,slot,genesis.header.coinbase,16,64,[&]{return now;},journal(1,*proposer),[&](const ConsensusRoot& root){return Key(signing_key).sign(root);});check(wire&&nodes[1]->block(*wire)&&nodes[2]->block(*wire),"post-reorg block failed");
   now+=rules.attestation_due_ms;for(size_t n:{1,2})nodes[n]->tick(now);
   const auto current=nodes[1]->current();const auto head_pin=capella_fork_head(*current.state,rules);const auto& head=*head_pin;
   const auto context=capella_fork_block_context(head.state,head.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);
   activation=context.registry.validators.at(65).activation_epoch;
   const auto assigned=prepare_capella_attestation_duties(*current.state,rules,slot,limits.replay_slots,limits.sync_trials);
   for(const auto& [index,duty]:assigned){if(index==65)continue;const auto key=index<64?index:129-index;
    const auto vote=nodes[1]->attest_current(index,slot,[&]{return now;},journal(1,index),[&](const ConsensusRoot& root){return Key(key).sign(root);});check(bool(vote)&&nodes[1]->vote(*vote)&&nodes[2]->vote(*vote),"post-reorg vote failed");
   }
   const auto archived=nodes[2]->signed_votes();local_clock=now;loop->poll();check(loop->status().at("healthy")==true,"reindexed automatic loop failed");
   for(const auto& [id,wire]:nodes[2]->signed_votes())if(!archived.contains(id))check(nodes[1]->vote_ssz(wire),"reindexed automatic vote rejected by peer");
   new_votes=loop->status().at("attestationsAdmitted").get<size_t>();
   if(slot/slots<activation)check(provider_calls==0&&new_votes==0,"reindexed key signed before activation");
   check(nodes[2]->current_validator_indices(requested).at(Key(64).public_key)==65,"post-reorg index reverted");
   if(slot%slots==0)std::cout<<Json{{"scenario","deposit-reorg-activation-progress"},{"slot",slot},{"activationEpoch",activation},{"newVotes",new_votes},{"finalizedEpoch",nodes[2]->current().state->timed.view.finalized.epoch}}.dump()<<std::endl;
  }
  loop.reset();clock.reset();const auto left=nodes[1]->current(),right=nodes[2]->current();
  check(activation!=UINT64_MAX&&new_votes>=2&&left.state->timed.view.finalized==right.state->timed.view.finalized&&left.state->timed.view.finalized.epoch>activation,"reindexed validator did not vote and finalize");
  const auto journal_token=local_journal.commitment();const auto finalized=left.state->timed.view.finalized;
  for(size_t n:{1,2}){const auto token=nodes[n]->current().token;nodes[n].reset();owners[n].reset();owners[n]=std::make_unique<Blockchain>(directory/("owner-"+std::to_string(n)),g);nodes[n]=std::make_unique<CapellaDurableReactor>(*owners[n],rules,limits,token);check(nodes[n]->current_validator_indices(requested).at(Key(64).public_key)==65&&nodes[n]->current().state->timed.view.finalized==finalized,"post-activation reopen lost finality or index");}
  check(local_journal.commitment()==journal_token,"owner reopen mutated signing journal");
  if(exit_reindex){
   const auto current=nodes[2]->current();const auto head_pin=capella_fork_head(*current.state,rules);const auto& head=*head_pin;
   const auto context=capella_fork_block_context(head.state,head.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);
   check(context.registry.epoch>=activation+t.consensus.exit.shard_committee_period,"reindexed validator has not completed exit tenure");
   const auto version=context.registry.epoch<context.fork.epoch?context.fork.previous_version:context.fork.current_version;
   auto request=[&](uint64_t index){SignedConsensusVoluntaryExit operation{context.registry.epoch,index,{}};operation.signature=Key(64).sign(consensus_signing_root(consensus_voluntary_exit_root(operation.epoch,index),consensus_domain({4,0,0,0},version,context.genesis_root)));return encode_consensus_voluntary_exit_ssz(operation);};
   const auto stale=request(64),correct=request(65);auto bad=correct;bad.back()^=1;
   for(size_t n:{1,2}){const auto token=nodes[n]->current().token;
    check(!nodes[n]->voluntary_exit_ssz(stale)&&!nodes[n]->voluntary_exit_ssz(bad)&&nodes[n]->current().token==token&&nodes[n]->pending_exits().empty(),"stale index or malformed signature admitted after reindex");
    check(nodes[n]->voluntary_exit_ssz(correct),"correct reindexed exit rejected");const auto saved=nodes[n]->current().token;
    check(nodes[n]->voluntary_exit_ssz(correct)&&nodes[n]->current().token==saved,"reindexed exit duplicate republished");
    nodes[n].reset();owners[n].reset();owners[n]=std::make_unique<Blockchain>(directory/("owner-"+std::to_string(n)),g);nodes[n]=std::make_unique<CapellaDurableReactor>(*owners[n],rules,limits,saved);
    const auto pending=nodes[n]->pending_exits();check(pending.size()==1&&encode_consensus_voluntary_exit_ssz(pending.front())==correct&&pending.front().validator_index==65,"reindexed exit lost identity on reopen");
   }
   const auto slot=end+1;now=initial.timed.clock.genesis_time_ms+slot*rules.slot_duration_ms;for(size_t n:{1,2})nodes[n]->tick(now);
   const auto proposer=nodes[1]->current_proposer(slot);check(bool(proposer),"reindexed exit proposer missing");const auto signing_key=*proposer<64?*proposer:129-*proposer;
   const auto wire=nodes[1]->propose_current(*proposer,slot,genesis.header.coinbase,16,64,[&]{return now;},journal(1,*proposer),[&](const ConsensusRoot& root){return Key(signing_key).sign(root);});
   check(wire&&nodes[1]->block(*wire)&&nodes[2]->block(*wire),"reindexed exit automatic inclusion failed");
   for(size_t n:{1,2}){const auto current=nodes[n]->current();const auto head_pin=capella_fork_head(*current.state,rules);const auto& head=*head_pin;const auto after=capella_fork_block_context(head.state,head.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);
    check(after.registry.validators.at(64).exit_epoch==UINT64_MAX&&after.registry.validators.at(65).exit_epoch!=UINT64_MAX,"exit applied to wrong reindexed validator");
    check(after.registry.validators.at(64).public_key==context.registry.validators.at(64).public_key&&after.registry.validators.at(65).public_key==context.registry.validators.at(65).public_key,"exit changed registered public keys");
   }
   std::cout<<Json{{"scenario","capella-reindexed-validator-exit"},{"oldIndex",64},{"newIndex",65},{"oldIndexSignatureRejected",true},{"invalidSignatureRejected",true},{"bothOwnersReopenedWithExactExit",true},{"automaticInclusion",true},{"otherValidatorUnchanged",true},{"networkTransport",false}}.dump()<<std::endl;
  }

  std::cout<<Json{{"scenario","capella-reindexed-validator-activation"},{"activationEpoch",activation},{"newValidatorVotes",new_votes},{"providerCalls",provider_calls},{"finalizedEpoch",finalized.epoch},{"finalizedRoot",hex(Bytes(finalized.root.begin(),finalized.root.end()))},{"bothOwnersReopened",true},{"index",65},{"networkTransport",false}}.dump()<<std::endl;
 }

}
