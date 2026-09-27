// Isolated adversarial publishers deliberately sign competing blocks with separate
// journals. Never run this fixture with real validator keys or public peers.
static void deposit_reorg_loop(const CapellaForkStore& initial,const CapellaForkBlockRules& rules,const Block& genesis,const State& genesis_state){
 Temp directory;const auto& t=rules.transition;auto limits=crash_limits;const auto period=t.consensus.state.eth1_votes_limit;check(period&&period<=64,"competing deposit period unbounded");const auto stop=period+period/2+1;limits.blocks=2*stop;const auto slots=t.consensus.epoch.ffg.slots_per_epoch;
 const auto validators=root_at(initial.blocks.at(initial.timed.view.anchor).state.ssz,8);
 Genesis g;g.chain_id=t.chain_id;g.consensus="beacon";g.evm_revision=EvmRevision::Shanghai;g.block=genesis;g.state=genesis_state;
 std::array<std::unique_ptr<Blockchain>,3> owners;std::array<std::unique_ptr<CapellaDurableReactor>,3> nodes;
 for(size_t n=0;n<3;++n){owners[n]=std::make_unique<Blockchain>(directory/("owner-"+std::to_string(n)),g);nodes[n]=std::make_unique<CapellaDurableReactor>(*owners[n],rules,limits,initial);}
 std::array<std::map<uint64_t,std::unique_ptr<ChainStore>>,2> disks;
 std::array<std::map<uint64_t,std::unique_ptr<BeaconSigningJournal>>,2> journals;
 auto journal=[&](size_t n,uint64_t i)->BeaconSigningJournal&{if(!journals[n].contains(i)){disks[n][i]=std::make_unique<ChainStore>(directory/("key-"+std::to_string(n)+"-"+std::to_string(i)));journals[n][i]=std::make_unique<BeaconSigningJournal>(*disks[n][i],t.chain_id,genesis.header.hash(),validators,Key(i).public_key,slots);}return *journals[n][i];};
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
 std::set<uint64_t> used;unsigned switches=0;bool stale_index_refused=false;
 for(size_t n=0;n<2;++n){size_t signed_votes=0;for(const auto& [index,duty]:duties[n]){if(used.contains(index))continue;auto vote=duty.attestation;vote.signature=Key(index).sign(consensus_signing_root(consensus_attestation_root(vote.data),consensus_domain({1,0,0,0},duty.fork_version,duty.genesis_root)));check(nodes[2]->vote(vote),"branch selection vote rejected");used.insert(index);if(++signed_votes==1+2*n)break;}check(signed_votes==1+2*n,"insufficient distinct voters");
  const auto expected=nodes[n]->current_validator_indices(requested);check(nodes[2]->current_validator_indices(requested)==expected,"observer retained stale branch indices");const auto current=nodes[2]->current();const auto& head=capella_fork_head(*current.state,rules);check(head.deposit_history->snapshot()==capella_fork_head(*nodes[n]->current().state,rules).deposit_history->snapshot(),"observer deposit history from wrong branch");check(owners[2]->state().root()==head.execution_state.root(),"observer EVM differs after reorg");
  if(n==1){const auto commitment=local_journal.commitment();auto signer=[&](const ConsensusRoot& root){++provider_calls;return Key(64).sign(root);};
   try{nodes[2]->attest_current(64,stop+1,[&]{return now+rules.slot_duration_ms;},local_journal,signer,{},Key(64).public_key);}catch(const StaleCapellaDuty&){stale_index_refused=true;}
   check(stale_index_refused&&provider_calls==0&&local_journal.commitment()==commitment&&nodes[2]->current().token==current.token,"reindexed key signed with old index or mutated journal");
   check(!nodes[2]->attest_current(65,stop+1,[&]{return now+rules.slot_duration_ms;},local_journal,signer,{},Key(64).public_key)&&provider_calls==0,"registered but inactive key signed");
  }
  nodes[2].reset();owners[2].reset();owners[2]=std::make_unique<Blockchain>(directory/"owner-2",g);nodes[2]=std::make_unique<CapellaDurableReactor>(*owners[2],rules,limits,current.token);check(nodes[2]->current_validator_indices(requested)==expected,"reopen lost branch registry");++switches;
 }
 std::cout<<Json{{"scenario","capella-competing-deposit-reorg"},{"branches",2},{"depositsPerBranch",2},{"registrySize",66},{"selectedBranches",switches},{"observerReopens",switches},{"indexA",Json::array({64,65})},{"networkTransport",false},{"activationQualified",false},{"staleIndexRefused",stale_index_refused},{"providerCalls",provider_calls}}.dump()<<std::endl;
}
