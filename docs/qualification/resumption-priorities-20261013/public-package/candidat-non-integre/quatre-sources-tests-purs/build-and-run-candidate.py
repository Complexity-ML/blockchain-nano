import hashlib,json,pathlib,resource,subprocess,time
base=pathlib.Path('/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012');out=base/'new-duty-projection-availability-counterexample-20261013/candidate-preparation';repo=pathlib.Path('/Users/boris/Dev/blockchain-nano');native=base/'native-candidate/build'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def save(n,v):(out/n).write_text(json.dumps(v,indent=2)+'\n')
paths=['include/nano/consensus_altair_epoch.hpp','src/consensus_altair_sync.cpp','include/nano/consensus_capella_duty.hpp','src/consensus_capella_duty.cpp'];sources={n:sha(repo/n) for n in paths}
for n in paths:assert sha(out/'candidate-source'/n)==sources[n],'repo/source snapshot mismatch'
parents=['nano','nano-capella-paused-capture','nano-capella-transition-tests','libnano_core.a'];before={n:sha(native/n) for n in parents};assert before['libnano_core.a']=='27d821c50c5de9cccebe77aa539468297b9074d7b760d7969b9d4e3f124c80af'
old=json.loads((base/'isolated-fixture-exporter/native-build-public-copy-repair/compile-command.json').read_text())['args'];link=json.loads((base/'isolated-fixture-exporter/native-build-public-copy-repair/link-command.json').read_text())['args']
receipt={'format':'nano-duty-availability-candidate-targeted-build-v1','sourceBindings':sources,'parentsBefore':before,'commands':{},'productionArchiveRebuilt':False,'reactorConstructed':False,'scope':'two candidate object overrides plus one fresh pure preparation helper; no old Reactor/validator ABI mixing'}
for name,path in [('candidate-altair-sync',out/'candidate-source/src/consensus_altair_sync.cpp'),('candidate-capella-duty',out/'candidate-source/src/consensus_capella_duty.cpp'),('candidate-pure-tests',out/'test-duty-availability.cpp')]:
 args=old.copy();args[-3]=str(path);args[-1]=str(out/(name+'.o'));receipt['commands'][name]=args;start=time.monotonic()
 with (out/(name+'.compile.log')).open('wb') as log:r=subprocess.run(args,cwd=native,stdout=log,stderr=subprocess.STDOUT,timeout=60)
 receipt[name]={'exitCode':r.returncode,'elapsedSeconds':time.monotonic()-start};save('build-receipt.json',receipt)
 if r.returncode:raise SystemExit(r.returncode)
link[5]=str(out/'candidate-pure-tests.o');link[7]=str(out/'candidate-pure-tests');link[8:8]=[str(out/'candidate-capella-duty.o'),str(out/'candidate-altair-sync.o'),'-Wl,-map,'+str(out/'candidate-link.map')];receipt['commands']['link']=link;start=time.monotonic()
with (out/'link.log').open('wb') as log:r=subprocess.run(link,cwd=native,stdout=log,stderr=subprocess.STDOUT,timeout=60)
receipt['link']={'exitCode':r.returncode,'elapsedSeconds':time.monotonic()-start};save('build-receipt.json',receipt)
if r.returncode:raise SystemExit(r.returncode)
maptext=(out/'candidate-link.map').read_text();objects=maptext.split('# Object files:\n',1)[1].split('# Sections:',1)[0]
forbidden=['consensus_capella_reactor.cpp.o','consensus_capella_signing.cpp.o','consensus_capella_validator.cpp.o','consensus_capella_runtime.cpp.o','consensus_capella_clock.cpp.o']
assert not any(n in objects for n in forbidden),'old real Reactor/validator ABI was linked'
assert not any('libnano_core.a' in row and ('consensus_capella_duty.cpp.o' in row or 'consensus_altair_sync.cpp.o' in row) for row in objects.splitlines()),'old replaced unit was linked'
receipt['linkMapSha256']=sha(out/'candidate-link.map');receipt['oldReactorValidatorObjectsLinked']=False;receipt['oldReplacedObjectsLinked']=False;receipt['parentHashesAfter']={n:sha(native/n) for n in parents};receipt['sourceBindingsAfter']={n:sha(repo/n) for n in paths};assert receipt['parentHashesAfter']==before and receipt['sourceBindingsAfter']==sources;save('build-receipt.json',receipt)
fixture=base/'isolated-fixture-exporter/native-controls/campaign-fixture';inputs=[fixture/'initial.json',fixture/'profile.json'];before_inputs={p.name:sha(p) for p in inputs};args=[str(out/'candidate-pure-tests'),str(inputs[0]),str(inputs[1])]
def cpu_cap():resource.setrlimit(resource.RLIMIT_CPU,(60,60))
start=time.monotonic()
with (out/'candidate-tests.log').open('wb') as log:r=subprocess.run(args,cwd=out,stdout=log,stderr=subprocess.STDOUT,timeout=65,preexec_fn=cpu_cap)
result=json.loads((out/'candidate-tests.log').read_text());run={'format':'nano-duty-availability-candidate-targeted-run-v1','command':args,'exitCode':r.returncode,'elapsedSeconds':time.monotonic()-start,'result':result,'sourceBindings':sources,'sourceBindingsAfter':{n:sha(repo/n) for n in paths},'parentHashesBefore':before,'parentHashesAfter':{n:sha(native/n) for n in parents},'inputHashesBefore':before_inputs,'inputHashesAfter':{p.name:sha(p) for p in inputs},'resourceBudget':{'cpuSeconds':60,'collectorWallSeconds':65,'peakRssKiB':131072},'noDatabaseOpened':True,'noSecretsOrEnvironmentFilesRead':True,'noSignerOrNetworkStarted':True,'realReactorIntegrationQualified':False,'productionQualified':False}
save('run-receipt.json',run)
assert r.returncode==0 and result['result']=='PASS' and result['maxRssKiB']<=131072
assert run['parentHashesAfter']==before and run['sourceBindingsAfter']==sources and run['inputHashesAfter']==before_inputs
print(json.dumps({'result':'PASS','caseCount':result['caseCount'],'exitCode':r.returncode,'elapsedMs':result['elapsedMs'],'maxRssKiB':result['maxRssKiB'],'runReceiptSha256':sha(out/'run-receipt.json'),'noCoreArchiveRebuild':True,'realReactorIntegrationQualified':False,'productionQualified':False}))
