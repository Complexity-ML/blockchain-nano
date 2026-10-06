#pragma once
#ifdef NANO_SYNC_FAULT
static Json slashing_fault_observation(Blockchain& owner,CapellaDurableReactor& node,const CapellaForkBlockRules& rules,const CapellaStoreSnapshotLimits& limits){
 const auto current=node.current();const auto head_pin=capella_fork_head(*current.state,rules);const auto& head=*head_pin;const auto final_pin=current.state->blocks.at(current.state->timed.view.finalized.root).lease();const auto& final=*final_pin;const auto& t=rules.transition;
 check(owner.tip().header.hash()==head.execution_block.header.hash()&&owner.state().root()==head.execution_state.root()&&owner.finalized_block()->header.hash()==final.execution_block.header.hash(),"slashing fault owner/state mismatch");
 const auto checkpoint=owner.store().fork_checkpoint(t.chain_id,owner.genesis().block.header.hash(),limits.envelope_bytes,current.token);check(bool(checkpoint),"slashing fault checkpoint missing");
 const auto registry=capella_fork_block_context(final.state,final.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state).registry;
 return {{"snapshot",checkpoint->snapshot},{"sequence",current.token.sequence},{"head",hex(head.execution_block.header.hash())},{"stateRoot",hex(head.execution_state.root())},{"finalized",hex(final.execution_block.header.hash())},{"slashed0",registry.validators.at(0).slashed},{"slashed1",registry.validators.at(1).slashed},{"archiveSize",node.signed_slashings().size()}};
}
static int slashing_sync_writer(const fs::path& directory){
 const auto bundle=load(directory/"bundle.json");const auto profile=bundle.at("profile").get<std::string>();const auto rules=load_capella_fork_profile(profile,sha256(Bytes(profile.begin(),profile.end())),1024*1024);const auto limits=exit_fault_limits(bundle);const auto g=exit_fault_genesis(bundle);
 ChainStore witness(directory/"witness");Blockchain owner(directory/"db",g);CapellaDurableReactor node(owner,rules,limits,witness);
 auto apply=[&]{if(bundle.at("isTick").get<bool>()){node.tick(bundle.at("time").get<uint64_t>());return true;}return node.block(unhex(bundle.at("wire").get<std::string>()));};
 const bool fault_witness=bundle.at("faultWitness");fs::path wal;for(const auto& entry:fs::directory_iterator(directory/(fault_witness?"witness":"db")))if(entry.path().extension()==".log"&&(wal.empty()||entry.path().filename()>wal.filename()))wal=entry.path();check(!wal.empty(),"slashing WAL missing");const auto offset=fs::file_size(wal);
 Rpc rpc(owner);nano_test_arm_sync_fault(wal.c_str());bool failed=false;try{apply();}catch(const std::runtime_error&){failed=true;}check(failed&&nano_test_sync_failures()>0,"slashing fsync fault not reached");
 unsigned blocked=0;auto refuse=[&](auto action){try{action();}catch(const std::runtime_error&){++blocked;}};
 refuse([&]{node.current();});refuse([&]{apply();});refuse([&]{node.signed_slashings();});refuse([&]{owner.store().latest();});refuse([&]{owner.store().fork_checkpoint(g.chain_id,g.block.header.hash(),limits.envelope_bytes);});refuse([&]{owner.tip();});refuse([&]{owner.state();});refuse([&]{owner.finalized_block();});check(blocked==(fault_witness?6:8),"uncertain slashing state accessible");
 check(rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method","eth_blockNumber"},{"params",Json::array()}}).contains("error"),"uncertain slashing RPC accessible");
 std::cout<<Json{{"wal",wal.filename().string()},{"before",offset},{"after",fs::file_size(wal)},{"blocked",blocked}}.dump()<<std::endl;_exit(0);
}
// The caller closes both databases before copies. No live RocksDB file copy.
static void qualify_slashing_sync(const fs::path& source,const fs::path& source_witness,const CapellaForkBlockRules& rules,const CapellaStoreSnapshotLimits& limits,const Genesis& g,const Bytes& wire,unsigned offender,bool is_tick,uint64_t time){
 auto apply=[&](CapellaDurableReactor& node){if(is_tick){node.tick(time);return true;}return node.block(wire);};
 auto copy=[&](const fs::path& directory){fs::copy(source,directory/"db",fs::copy_options::recursive);fs::copy(source_witness,directory/"witness",fs::copy_options::recursive);};
 Json old,next;{Temp reference;copy(reference.path);ChainStore witness(reference/"witness");Blockchain owner(reference/"db",g);CapellaDurableReactor node(owner,rules,limits,witness);old=slashing_fault_observation(owner,node,rules,limits);check(apply(node),"slashing reference rejected");next=slashing_fault_observation(owner,node,rules,limits);}
 const auto field="slashed"+std::to_string(offender);check(old.at(field)==false&&next.at(field)==true&&old.at("finalized")!=next.at("finalized"),"slashing fault not at finalization boundary");
 check(next.at("sequence").get<uint64_t>()==old.at("sequence").get<uint64_t>()+1&&old.at("archiveSize")==2-offender&&next.at("archiveSize")==1-offender,"slashing retirement not one expected atomic commit");
 for(unsigned disposition=0;disposition<4;++disposition){const bool fault_witness=disposition>=2,lose=disposition%2;Temp temp;copy(temp.path);
  save(temp/"bundle.json",Json{{"profile",capella_fork_profile(rules).dump()},{"blocks",limits.blocks},{"chainId",quantity(g.chain_id)},{"genesis",g.block.to_json(true)},{"alloc",g.state.to_json()},{"wire",hex(wire)},{"faultWitness",fault_witness},{"isTick",is_tick},{"time",time}});
  const auto result=success(run({executable,"--slashing-sync-writer",temp.path.string()}));const auto wal=temp/(fault_witness?"witness":"db")/result.at("wal").get<std::string>();check(fs::file_size(wal)==result.at("after").get<uint64_t>()&&result.at("after")>result.at("before"),"slashing WAL suffix missing");
  if(lose){const int fd=open(wal.c_str(),O_WRONLY);check(fd>=0,"open test slashing WAL");const bool ok=ftruncate(fd,result.at("before").get<uint64_t>())==0&&fsync(fd)==0;close(fd);check(ok,"truncate test slashing WAL");}
  ChainStore witness(temp/"witness");Blockchain owner(temp/"db",g);const auto primary=owner.store().fork_checkpoint(g.chain_id,g.block.header.hash(),limits.envelope_bytes);const bool primary_new=!fault_witness&&!lose;check(primary&&primary->snapshot==(primary_new?next:old).at("snapshot").get<std::string>(),"mixed primary slashing state");
  auto node=std::make_unique<CapellaDurableReactor>(owner,rules,limits,witness);const bool recovered_new=!(fault_witness&&lose);check(slashing_fault_observation(owner,*node,rules,limits)==(recovered_new?next:old),"slashing witness recovery mismatch");
  check(apply(*node)&&slashing_fault_observation(owner,*node,rules,limits)==next,"slashing retry mismatch");const auto token=node->current().token;check(apply(*node)&&node->current().token==token,"slashing duplicate republished");
  node.reset();node=std::make_unique<CapellaDurableReactor>(owner,rules,limits,witness);check(slashing_fault_observation(owner,*node,rules,limits)==next,"slashing second reopen mismatch");
  std::cout<<Json{{"scenario","capella-slashing-sync"},{"finalizedOffender",offender},{"faultWitness",fault_witness},{"lostSuffix",lose},{"blockedOperations",result.at("blocked")},{"exactRecovery",true},{"retryIdempotent",true},{"archiveSize",next.at("archiveSize")},{"physicalPowerLoss",false}}.dump()<<std::endl;
 }
}
#endif
