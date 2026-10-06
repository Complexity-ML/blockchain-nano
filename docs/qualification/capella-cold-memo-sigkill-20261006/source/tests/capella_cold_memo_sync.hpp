#pragma once
#ifdef NANO_SYNC_FAULT
#ifdef __linux__
extern "C" void nano_test_arm_sync_stop(const char*);
#endif
// Child exec only: no fork with live RocksDB handles, no production data.
static int cold_memo_sync_writer(const fs::path& directory){
 const auto bundle=load(directory/"bundle.json");const auto profile=bundle.at("profile").get<std::string>();
 const auto rules=load_capella_fork_profile(profile,sha256(Bytes(profile.begin(),profile.end())),1024*1024);
 Genesis g;g.chain_id=rules.transition.chain_id;g.consensus="beacon";g.evm_revision=EvmRevision::Shanghai;g.block=Block::from_json(bundle.at("genesis"));g.state=State::from_json(bundle.at("alloc"));
 ChainStore witness(directory/"witness");Blockchain owner(directory/"db",g);CapellaDurableReactor reactor(owner,rules,crash_limits,witness);
 const auto initial=reactor.current();const auto seed_time=initial.state->timed.clock.time_ms+rules.slot_duration_ms;
 reactor.tick(seed_time);const auto before=reactor.current();const auto memo=reactor.reference_snapshot_cache_usage();check(memo.records>0,"cold fault fixture did not seed reference memo");
 const auto old=owner.store().fork_checkpoint(g.chain_id,g.block.header.hash(),crash_limits.envelope_bytes,before.token);check(old&&Json::parse(old->snapshot).at("format")=="nano-capella-cold-checkpoint-v1","fault fixture lost cold format");
 Json archives=Json::array();for(const auto& [id,entry]:before.state->blocks){const auto wire=reactor.signed_block(id);if(wire)archives.push_back(Json::array({hex(Bytes(id.begin(),id.end())),hex(*wire)}));}
 const auto next_time=seed_time+rules.slot_duration_ms;save(directory/"before.json",Json{{"snapshot",old->snapshot},{"sequence",before.token.sequence},{"time",next_time},{"archive",archives}});
 if(!bundle.at("fault").get<bool>()){
  reactor.tick(next_time);const auto next=reactor.current();const auto saved=owner.store().fork_checkpoint(g.chain_id,g.block.header.hash(),crash_limits.envelope_bytes,next.token);check(saved&&next.token.sequence==before.token.sequence+1,"oracle clock did not publish exactly once");
  check(reactor.reference_snapshot_cache_usage().reuses>memo.reuses,"fault oracle did not exercise memo reuse");save(directory/"after.json",Json{{"snapshot",saved->snapshot},{"sequence",next.token.sequence}});if(bundle.value("postCommitStop",false)){std::cout<<"nano-test-commit-stopped"<<std::endl;::raise(SIGSTOP);}
  std::cout<<Json{{"oracle",true}}.dump()<<std::endl;return 0;
 }
 const bool fault_witness=bundle.at("faultWitness");fs::path wal;for(const auto& entry:fs::directory_iterator(directory/(fault_witness?"witness":"db")))if(entry.path().extension()==".log"&&(wal.empty()||entry.path().filename()>wal.filename()))wal=entry.path();check(!wal.empty(),"cold fault WAL missing");const auto offset=fs::file_size(wal);
 save(directory/"wal-before.json",Json{{"wal",wal.filename().string()},{"before",offset}});
#ifdef __linux__
 if(bundle.value("syncStop",false))nano_test_arm_sync_stop(wal.c_str());else
#endif
 nano_test_arm_sync_fault(wal.c_str());bool failed=false;try{reactor.tick(next_time);}catch(const std::runtime_error&){failed=true;}check(failed&&nano_test_sync_failures()>0,"cold clock sync fault not reached");
 unsigned blocked=0;auto refuse=[&](auto action){try{action();}catch(const std::runtime_error&){++blocked;}};
 refuse([&]{reactor.current();});refuse([&]{reactor.reference_snapshot_cache_usage();});refuse([&]{reactor.tick(next_time);});check(blocked==3,"uncertain cold reactor or memo stayed available");
 std::cout<<Json{{"wal",wal.filename().string()},{"before",offset},{"after",fs::file_size(wal)},{"blocked",blocked}}.dump()<<std::endl;_exit(0);
}
static void qualify_cold_memo_sync(const fs::path& source,const fs::path& source_witness,const CapellaForkBlockRules& rules,const Genesis& g){
 const auto profile=capella_fork_profile(rules).dump();
 auto copy=[&](const fs::path& destination){fs::copy(source,destination/"db",fs::copy_options::recursive);fs::copy(source_witness,destination/"witness",fs::copy_options::recursive);};
 auto bundle=[&](bool fault,bool witness){return Json{{"profile",profile},{"genesis",g.block.to_json(true)},{"alloc",g.state.to_json()},{"fault",fault},{"faultWitness",witness}};};
 Temp oracle;copy(oracle.path);save(oracle/"bundle.json",bundle(false,false));success(run({executable,"--cold-memo-sync-writer",oracle.path.string()}));const auto expected=load(oracle/"after.json");
 for(unsigned disposition=0;disposition<4;++disposition){const bool fault_witness=disposition>=2,lose=disposition%2;Temp isolated;copy(isolated.path);save(isolated/"bundle.json",bundle(true,fault_witness));
  const auto result=success(run({executable,"--cold-memo-sync-writer",isolated.path.string()}));const auto wal=isolated/(fault_witness?"witness":"db")/result.at("wal").get<std::string>();check(fs::file_size(wal)==result.at("after").get<uint64_t>()&&result.at("after")>result.at("before"),"cold WAL append not observed");
  if(lose){const int fd=open(wal.c_str(),O_WRONLY);check(fd>=0,"open owned cold WAL");const bool ok=ftruncate(fd,result.at("before").get<uint64_t>())==0&&fsync(fd)==0;close(fd);check(ok,"truncate owned cold WAL");}
  const auto old=load(isolated/"before.json");check(old==load(oracle/"before.json"),"cold fault candidate baseline differs from independent oracle");
  ChainStore witness(isolated/"witness");Blockchain owner(isolated/"db",g);const auto record=owner.store().fork_checkpoint(g.chain_id,g.block.header.hash(),crash_limits.envelope_bytes);const bool primary_new=!fault_witness&&!lose;
  check(record&&record->snapshot==(primary_new?expected:old).at("snapshot").get<std::string>(),"cold uncertain primary disposition differs");
  auto resumed=std::make_unique<CapellaDurableReactor>(owner,rules,crash_limits,witness);const bool recovered_new=!(fault_witness&&lose);const auto current=resumed->current();
  check(owner.store().fork_checkpoint(g.chain_id,g.block.header.hash(),crash_limits.envelope_bytes,current.token)->snapshot==(recovered_new?expected:old).at("snapshot").get<std::string>(),"cold witness recovery chose wrong checkpoint");
  for(const auto& archive:old.at("archive")){const auto raw=unhex(archive.at(0).get<std::string>());ConsensusRoot id{};std::copy(raw.begin(),raw.end(),id.begin());check(resumed->signed_block(id)==std::optional<Bytes>(unhex(archive.at(1).get<std::string>())),"cold fault recovery lost authenticated signed block");}
  resumed->tick(old.at("time").get<uint64_t>());const auto completed=resumed->current().token;check(completed.sequence==expected.at("sequence").get<uint64_t>()&&owner.store().fork_checkpoint(g.chain_id,g.block.header.hash(),crash_limits.envelope_bytes,completed)->snapshot==expected.at("snapshot").get<std::string>(),"cold retry differs from independent oracle");
  resumed->tick(old.at("time").get<uint64_t>());check(resumed->current().token==completed,"cold clock retry not idempotent");resumed.reset();resumed=std::make_unique<CapellaDurableReactor>(owner,rules,crash_limits,witness, std::function<void()>{},completed);check(resumed->current().token==completed,"cold fault recovery not restart stable");
  std::cout<<Json{{"scenario","capella-cold-memo-sync-recovery"},{"faultWitness",fault_witness},{"lostSuffix",lose},{"recoveredNewBeforeRetry",recovered_new},{"blockedOperations",result.at("blocked")},{"exactOracleCheckpoint",true},{"retryIdempotent",true},{"physicalPowerLoss",false}}.dump()<<std::endl;
 }
}

#ifdef __linux__
static void qualify_cold_memo_sigkill(const fs::path& source,const fs::path& source_witness,const CapellaForkBlockRules& rules,const Genesis& g){
 const auto profile=capella_fork_profile(rules).dump();
 auto copy=[&](const fs::path& destination){fs::copy(source,destination/"db",fs::copy_options::recursive);fs::copy(source_witness,destination/"witness",fs::copy_options::recursive);};
 auto bundle=[&](bool fault,bool witness){return Json{{"profile",profile},{"genesis",g.block.to_json(true)},{"alloc",g.state.to_json()},{"fault",fault},{"faultWitness",witness}};};
 Temp oracle;copy(oracle.path);save(oracle/"bundle.json",bundle(false,false));success(run({executable,"--cold-memo-sync-writer",oracle.path.string()}));const auto expected=load(oracle/"after.json");
 for(unsigned stage=0;stage<3;++stage){const bool post_commit=stage==2,fault_witness=stage==1;Temp isolated;copy(isolated.path);auto request=bundle(!post_commit,fault_witness);request["syncStop"]=!post_commit;request["postCommitStop"]=post_commit;save(isolated/"bundle.json",request);
  Process writer({executable,"--cold-memo-sync-writer",isolated.path.string()});writer.line([&](const auto& line){return line==(post_commit?"nano-test-commit-stopped":"nano-test-sync-stopped");});writer.signal(SIGKILL);const auto result=writer.wait(20);check(result.code==-SIGKILL,"cold crash writer not killed by SIGKILL");
  const auto old=load(isolated/"before.json");check(old==load(oracle/"before.json"),"cold SIGKILL baseline differs from oracle");
  if(!post_commit){const auto meta=load(isolated/"wal-before.json");const auto wal=isolated/(fault_witness?"witness":"db")/meta.at("wal").get<std::string>();check(fs::file_size(wal)>meta.at("before").get<uint64_t>(),"SIGKILL barrier before WAL append");}
  ChainStore witness(isolated/"witness");Blockchain owner(isolated/"db",g);const auto primary=owner.store().fork_checkpoint(g.chain_id,g.block.header.hash(),crash_limits.envelope_bytes);check(bool(primary),"cold SIGKILL primary missing");
  const auto intent=witness.fork_checkpoint(g.chain_id,g.block.header.hash(),8*crash_limits.envelope_bytes+2048);check(bool(intent),"cold SIGKILL witness missing");const auto witness_snapshot=Json::parse(intent->snapshot).at("snapshot").get<std::string>();const bool recovered_new=witness_snapshot==expected.at("snapshot").get<std::string>();check(recovered_new||witness_snapshot==old.at("snapshot").get<std::string>(),"cold SIGKILL witness is mixed");
  if(!fault_witness)check(recovered_new,"acknowledged witness lost before primary sync or after commit");if(post_commit)check(primary->snapshot==expected.at("snapshot").get<std::string>(),"acknowledged primary lost after durable commit");else check(primary->snapshot==old.at("snapshot").get<std::string>()||primary->snapshot==expected.at("snapshot").get<std::string>(),"cold SIGKILL primary mixed");
  const auto& recovered=recovered_new?expected:old;const auto text=recovered.at("snapshot").get<std::string>();const ForkCheckpointToken trusted{recovered.at("sequence").get<uint64_t>(),sha256(Bytes(text.begin(),text.end()))};
  auto resumed=std::make_unique<CapellaDurableReactor>(owner,rules,crash_limits,witness,std::function<void()>{},trusted);check(resumed->current().token==trusted,"cold SIGKILL restoration differs from observed witness intent and external commitment");
  for(const auto& archive:old.at("archive")){const auto raw=unhex(archive.at(0).get<std::string>());ConsensusRoot id{};std::copy(raw.begin(),raw.end(),id.begin());check(resumed->signed_block(id)==std::optional<Bytes>(unhex(archive.at(1).get<std::string>())),"SIGKILL lost signed archive");}
  resumed->tick(old.at("time").get<uint64_t>());const auto completed=resumed->current().token;check(completed.sequence==expected.at("sequence").get<uint64_t>()&&owner.store().fork_checkpoint(g.chain_id,g.block.header.hash(),crash_limits.envelope_bytes,completed)->snapshot==expected.at("snapshot").get<std::string>(),"SIGKILL retry differs from oracle");resumed->tick(old.at("time").get<uint64_t>());check(resumed->current().token==completed,"SIGKILL retry republished");resumed.reset();resumed=std::make_unique<CapellaDurableReactor>(owner,rules,crash_limits,witness,std::function<void()>{},completed);check(resumed->current().token==completed,"SIGKILL recovery not restart stable");
  std::cout<<Json{{"scenario","capella-cold-memo-sigkill-recovery"},{"stage",post_commit?"after-durable-commit":fault_witness?"before-witness-sync":"before-primary-sync"},{"signal",9},{"recoveredNewBeforeRetry",recovered_new},{"exactOracleCheckpoint",true},{"signedArchivesPreserved",true},{"retryIdempotent",true},{"physicalPowerLoss",false},{"networkQualified",false}}.dump()<<std::endl;
 }
}
#endif
#endif
