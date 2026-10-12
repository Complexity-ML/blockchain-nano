import hashlib,json,pathlib,resource,subprocess,time
base=pathlib.Path('/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012');out=base/'new-duty-projection-availability-counterexample-20261013';native=base/'native-candidate/build';repo=pathlib.Path('/Users/boris/Dev/blockchain-nano')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def save(n,v):(out/n).write_text(json.dumps(v,indent=2)+'\n')
old=json.loads((out/'first-harness-refusal/build-receipt.json').read_text());before=old['parentHashesBefore'];assert {n:sha(native/n) for n in before}==before
receipt={'format':'nano-new-duty-availability-counterexample-harness-repair-build-v1','oldRefusal':'first-harness-refusal/pure-duty-projection.run-receipt.json','repair':'restore one synthetic anchor unrealized_justifications entry required by snapshot schema','parentHashesBefore':before,'helperSourceSha256':sha(out/'pure-duty-projection.cpp'),'commands':{},'noCoreRebuild':True,'actualValidatorObjectsReusedWithoutRecompile':True}
for name,args in [('pure-duty-projection',old['commands']['pure-duty-projection']),('pure-duty-projection-link',old['commands']['pure-duty-projection-link'])]:
 start=time.monotonic();receipt['commands'][name]=args
 with (out/(name+'.repair-build.log')).open('wb') as log:r=subprocess.run(args,cwd=native,stdout=log,stderr=subprocess.STDOUT,timeout=60)
 receipt[name]={'exitCode':r.returncode,'elapsedSeconds':time.monotonic()-start};save('repair-build-receipt.json',receipt)
 if r.returncode:raise SystemExit(r.returncode)
receipt['parentHashesAfter']={n:sha(native/n) for n in before};assert receipt['parentHashesAfter']==before;save('repair-build-receipt.json',receipt)
fixture=base/'isolated-fixture-exporter/native-controls/campaign-fixture';inputs=[fixture/'initial.json',fixture/'profile.json'];input_before={p.name:sha(p) for p in inputs}
def cpu_cap():resource.setrlimit(resource.RLIMIT_CPU,(60,60))
for name,args,expected in [('pure-duty-projection',[str(out/'pure-duty-projection'),str(inputs[0]),str(inputs[1]),str(out/'derived-positive-balance-exit8-state.ssz')],0),('validator-flow',[str(out/'validator-flow')],1)]:
 start=time.monotonic()
 with (out/(name+'.run.log')).open('wb') as log:r=subprocess.run(args,cwd=out,stdout=log,stderr=subprocess.STDOUT,timeout=65,preexec_fn=cpu_cap)
 result=json.loads((out/(name+'.run.log')).read_text())
 run={'format':'nano-new-duty-availability-counterexample-run-v1','command':args,'expectedExitCode':expected,'exitCode':r.returncode,'elapsedSeconds':time.monotonic()-start,'resourceBudget':{'cpuSeconds':60,'wallSeconds':65,'maxRssKiB':131072},'result':result,'inputHashesBefore':input_before,'inputHashesAfter':{p.name:sha(p) for p in inputs},'parentHashesAfter':{n:sha(native/n) for n in before},'noDatabaseOpened':True,'noPrivateKeysRead':True,'noRuntimeNetworkStarted':True,'productionQualified':False}
 save(name+'.run-receipt.json',run)
 assert r.returncode==expected and result['result']!='HARNESS_DIAGNOSTIC_FAILURE'
 assert run['inputHashesAfter']==input_before and run['parentHashesAfter']==before
 assert result['maxRssKiB']<=131072
bindings=old['productionSourcesBefore'];assert {n:sha(repo/n) for n in bindings}==bindings
save('terminal-receipt.json',{'format':'nano-new-duty-availability-counterexample-terminal-v1','pureRealCoreDiagnosticExitCode':0,'actualValidatorMockFlowExpectedFailureExitCode':1,'expectedFailuresReproduced':True,'sourceAndParentHashesUnchanged':True,'productionQualified':False,'actualRuntimeIntegrationQualified':False,'noDatabaseOpened':True,'noPrivateKeysRead':True,'noSignerCalled':True,'noNetworkStarted':True,'sourceBindings':bindings,'pureHelperSha256':sha(out/'pure-duty-projection.cpp'),'actualValidatorSourceSha256':sha(out/'actual-validator.cpp'),'memoryDoubleHeaderSha256':sha(out/'mocks/nano/consensus_capella_clock.hpp')})
print((out/'terminal-receipt.json').read_text())
