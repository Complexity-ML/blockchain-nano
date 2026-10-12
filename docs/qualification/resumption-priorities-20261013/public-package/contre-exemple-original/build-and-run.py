import hashlib,json,pathlib,resource,subprocess,time
base=pathlib.Path('/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012')
out=base/'new-duty-projection-availability-counterexample-20261013';native=base/'native-candidate/build';repo=pathlib.Path('/Users/boris/Dev/blockchain-nano')
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def save(name,value):(out/name).write_text(json.dumps(value,indent=2)+'\n')
qualified='85eb492f641452c4746ee60f027eacefa775b0b0'
source=repo/'src/consensus_capella_validator.cpp';header=repo/'include/nano/consensus_capella_validator.hpp'
bindings={str(p.relative_to(repo)):sha(p) for p in [source,header]}
for p in [source,header]:
    historical=subprocess.run(['git','show',qualified+':'+str(p.relative_to(repo))],cwd=repo,check=True,stdout=subprocess.PIPE).stdout
    assert hashlib.sha256(historical).hexdigest()==sha(p),'qualified validator source/header differ'
(out/'actual-validator.cpp').write_bytes(source.read_bytes());(out/'mocks/nano/consensus_capella_validator.hpp').write_bytes(header.read_bytes())
parents=['nano','nano-capella-paused-capture','nano-capella-transition-tests','libnano_core.a'];before={name:sha(native/name) for name in parents}
assert before['libnano_core.a']=='27d821c50c5de9cccebe77aa539468297b9074d7b760d7969b9d4e3f124c80af'
template=json.loads((base/'isolated-fixture-exporter/native-build-public-copy-repair/compile-command.json').read_text())['args'];linktemplate=json.loads((base/'isolated-fixture-exporter/native-build-public-copy-repair/link-command.json').read_text())['args']
receipt={'format':'nano-new-duty-availability-counterexample-build-v1','qualifiedCoreSource':qualified,'productionSourcesBefore':bindings,'parentHashesBefore':before,'commands':{},'noProductionCoreRebuild':True,'scope':'one pure real-core diagnostic plus actual frozen validator source against explicit memory doubles'}
for name,src,mocks in [('pure-duty-projection',out/'pure-duty-projection.cpp',False),('actual-validator',out/'actual-validator.cpp',True),('validator-flow',out/'validator-flow.cpp',True)]:
    args=template.copy();args[-3]=str(src);args[-1]=str(out/(name+'.o'))
    if mocks:args[1:1]=['-I'+str(out/'mocks')]
    receipt['commands'][name]=args
    start=time.monotonic()
    with (out/(name+'.compile.log')).open('wb') as log:r=subprocess.run(args,cwd=native,stdout=log,stderr=subprocess.STDOUT,timeout=60)
    receipt[name]={'exitCode':r.returncode,'elapsedSeconds':time.monotonic()-start};save('build-receipt.json',receipt)
    if r.returncode:raise SystemExit(r.returncode)
purelink=linktemplate.copy();purelink[5]=str(out/'pure-duty-projection.o');purelink[7]=str(out/'pure-duty-projection')
# Flow uses no core library: duplicate class names only describe stand-alone doubles.
flowlink=['/usr/bin/c++','-std=gnu++20',str(out/'actual-validator.o'),str(out/'validator-flow.o'),'-o',str(out/'validator-flow'),'/opt/homebrew/lib/libgmpxx.dylib','/opt/homebrew/lib/libgmp.dylib']
for name,args in [('pure-duty-projection-link',purelink),('validator-flow-link',flowlink)]:
    receipt['commands'][name]=args;start=time.monotonic()
    with (out/(name+'.log')).open('wb') as log:r=subprocess.run(args,cwd=native,stdout=log,stderr=subprocess.STDOUT,timeout=60)
    receipt[name]={'exitCode':r.returncode,'elapsedSeconds':time.monotonic()-start};save('build-receipt.json',receipt)
    if r.returncode:raise SystemExit(r.returncode)
receipt['parentHashesAfter']={name:sha(native/name) for name in parents};receipt['productionSourcesAfter']={str(p.relative_to(repo)):sha(p) for p in [source,header]}
assert receipt['parentHashesAfter']==before and receipt['productionSourcesAfter']==bindings
receipt['helperSourceHashes']={str(p.relative_to(out)):sha(p) for p in [out/'pure-duty-projection.cpp',out/'actual-validator.cpp',out/'validator-flow.cpp',out/'mocks/nano/consensus_capella_clock.hpp',out/'mocks/nano/consensus_capella_validator.hpp']}
save('build-receipt.json',receipt)
fixture=base/'isolated-fixture-exporter/native-controls/campaign-fixture'
inputs=[fixture/'initial.json',fixture/'profile.json'];input_before={p.name:sha(p) for p in inputs}
def cpu_cap():resource.setrlimit(resource.RLIMIT_CPU,(60,60))
for name,args,expected in [('pure-duty-projection',[str(out/'pure-duty-projection'),str(inputs[0]),str(inputs[1]),str(out/'derived-positive-balance-exit8-state.ssz')],0),('validator-flow',[str(out/'validator-flow')],1)]:
    start=time.monotonic()
    with (out/(name+'.run.log')).open('wb') as log:r=subprocess.run(args,cwd=out,stdout=log,stderr=subprocess.STDOUT,timeout=65,preexec_fn=cpu_cap)
    result=json.loads((out/(name+'.run.log')).read_text())
    run={'format':'nano-new-duty-availability-counterexample-run-v1','command':args,'expectedExitCode':expected,'exitCode':r.returncode,'elapsedSeconds':time.monotonic()-start,'resourceBudget':{'cpuSeconds':60,'wallSeconds':65,'maxRssKiB':131072},'result':result,'inputHashesBefore':input_before,'inputHashesAfter':{p.name:sha(p) for p in inputs},'parentHashesAfter':{p:sha(native/p) for p in parents},'noDatabaseOpened':True,'noPrivateKeysRead':True,'noRuntimeNetworkStarted':True,'productionQualified':False}
    save(name+'.run-receipt.json',run)
    assert r.returncode==expected and result['result']!='HARNESS_DIAGNOSTIC_FAILURE'
    assert run['inputHashesAfter']==input_before and run['parentHashesAfter']==before
    assert result['maxRssKiB']<=131072
save('terminal-receipt.json',{'format':'nano-new-duty-availability-counterexample-terminal-v1','pureRealCoreDiagnosticExitCode':0,'actualValidatorMockFlowExpectedFailureExitCode':1,'expectedFailuresReproduced':True,'sourceAndParentHashesUnchanged':True,'productionQualified':False,'actualRuntimeIntegrationQualified':False,'noDatabaseOpened':True,'noPrivateKeysRead':True,'noSignerCalled':True,'noNetworkStarted':True})
print((out/'terminal-receipt.json').read_text())
