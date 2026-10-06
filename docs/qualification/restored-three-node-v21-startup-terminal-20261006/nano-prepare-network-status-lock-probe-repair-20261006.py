from pathlib import Path
import json,subprocess,os
repo=Path('/Users/boris/Dev/blockchain-nano');out=Path('/private/tmp/nano-network-status-lock-probe-repair-20261006');out.mkdir(exist_ok=False)
s=(repo/'tests/test_capella_transition.cpp').read_text();at='static void verify_beacon_read_admission(CapellaDurableReactor& reactor,const Bytes& wire,const ConsensusRoot& parent){'
probe='''
#ifdef NANO_TEST_NETWORK_STATUS_PROBE
 if(const char* field=std::getenv("NANO_TEST_NETWORK_STATUS_FIELD")){
  const std::string name=field;check(name=="identity"||name=="sync","Invalid status probe field");
  auto read_status=[&]{return name=="identity"?reactor.network_identity():reactor.sync_admission_stats();};
  const auto before=read_status();const auto token=reactor.current().token;
  std::promise<void> held,unblock;auto held_future=held.get_future();auto gate=unblock.get_future().share();
  auto writer=std::async(std::launch::async,[&]{bool paused=false;try{return reactor.block(wire,[&]{if(!paused){paused=true;held.set_value();gate.wait();throw ExecutionCancelled("status probe writer cancelled");}});}catch(const ExecutionCancelled&){return false;}});
  const bool locked=held_future.wait_for(std::chrono::seconds(5))==std::future_status::ready;
  if(!locked){unblock.set_value();writer.get();check(false,"Status probe admission lock not held");}
  auto reader=std::async(std::launch::async,read_status);
  const bool completed=reader.wait_for(std::chrono::seconds(1))==std::future_status::ready;
  unblock.set_value();const auto actual=reader.get();const auto admitted=writer.get();
  std::cout<<Json{{"scenario","network-status-during-held-admission"},{"field",name},{"completedWhileAdmissionHeld",completed},{"cancelledWriterUnpublished",!admitted&&reactor.current().token==token},{"sameStatus",actual==before},{"wanCauseProven",false}}.dump()<<std::endl;
  check(completed&&!admitted&&actual==before&&reactor.current().token==token,"Network status "+name+" waited on held admission");
 }
#endif
'''
assert s.count(at)==1;s=s.replace(at,at+probe);(out/'test_capella_transition.cpp').write_text(s)
template=Path('/private/tmp/nano-delivery-diagnostic-native-20261006')
for phase in ['compile','link']:
 args=json.loads((template/('wire-'+phase+'-command.json')).read_text())
 if phase=='compile':args.insert(1,'-I'+str(repo/'tests'));args.insert(1,'-DNANO_TEST_NETWORK_STATUS_PROBE');args[args.index('-c')+1]=str(out/'test_capella_transition.cpp');args[args.index('-o')+1]=str(out/'probe.o')
 else:args=[str(out/'probe.o') if x==str(template/'candidate.o') else x for x in args];args[args.index('-o')+1]=str(out/'probe')
 (out/(phase+'-command.json')).write_text(json.dumps(args,indent=2)+'\n')
 with (out/(phase+'.log')).open('w') as f:p=subprocess.run(args,stdout=f,stderr=subprocess.STDOUT,cwd='/tmp/nano-pruning-wan-native-build-20261004' if phase=='link' else None)
 (out/(phase+'-exit.json')).write_text(json.dumps({'exitCode':p.returncode})+'\n');assert p.returncode==0,phase
for field in ['identity','sync']:
 env=os.environ.copy();env['NANO_TEST_NETWORK_STATUS_FIELD']=field
 with (out/(field+'-baseline.log')).open('w') as f:p=subprocess.run([str(out/'probe'),'--beacon-reads'],stdout=f,stderr=subprocess.STDOUT,env=env)
 (out/(field+'-baseline-exit.json')).write_text(json.dumps({'exitCode':p.returncode,'expectedExitCode':1})+'\n');print(field,'baseline exit',p.returncode,flush=True);assert p.returncode==1,field
