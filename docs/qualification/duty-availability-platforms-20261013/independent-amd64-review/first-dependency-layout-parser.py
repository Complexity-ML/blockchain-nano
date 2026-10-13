from pathlib import Path
import json,hashlib,re,shlex,posixpath,xml.etree.ElementTree as ET,datetime
B=Path(__file__).resolve().parents[1]
P=B/'duty-availability-coherent-amd64-20261013'
Q=B/'duty-availability-portability-amd64-20261013'
S=B/'duty-availability-amd64-final-public-evidence-20261013'
OUT=Path(__file__).resolve().parent
load=lambda x:json.loads(x.read_text())
sha=lambda x:hashlib.sha256(x.read_bytes()).hexdigest()
validation=[]
for folder,name in [(P,'public-file-manifest.json'),(Q,'remote-public-transfer-manifest.json'),(S,'manifest.json')]:
 m=load(folder/name);rows=m['files'];assert len({r['path'] for r in rows})==len(rows)
 for r in rows:
  path=Path(r['path']);assert not path.is_absolute() and '..' not in path.parts
  x=folder/path;assert x.is_file() and not x.is_symlink();assert x.stat().st_size==r['bytes'];assert sha(x)==r['sha256']
 validation.append({'manifest':str((folder/name).relative_to(B)),'sha256':sha(folder/name),'payloads':len(rows),'bytes':sum(r['bytes'] for r in rows),'allPublicRowsValidated':True})
sourceBefore=load(P/'receipts/source-before.json');sourceAfter=load(P/'receipts/source-after.json');assert sourceBefore==sourceAfter
coreRows=load(P/'canonical-arm-production-reference.json')['canonicalProductionHashes']
assert len(coreRows)==974 and all(sourceBefore[k]==v for k,v in coreRows.items())
ref=load(B/'candidate-runtime-source-guard-v2-20261013/reference-fixtures/core-c672-production-reference.json')
assert {r['path']:r['sha256'] for r in ref['files']}==coreRows
commands=load(P/'original-generated-build-evidence/compile_commands.json')
coreCommands=[r for r in commands if 'CMakeFiles/nano_core.dir/' in r.get('command','')];assert len(coreCommands)==139
units=load(P/'receipts/core-object-audit.json')['objects'];assert len(units)==139
expected=load(B/'candidate-runtime-source-guard-v2-20261013/reference-fixtures/source-d7-observed-reference.json')['expectedCoreTranslationUnits']
assert sorted(r['source'].removeprefix('/work/') for r in units)==expected
log=(P/'receipts/build.log').read_text()
compiled=re.findall(r'Building CXX object CMakeFiles/nano_core.dir/(src/[^\s]+\.cpp)\.o',log)
assert len(compiled)==139 and sorted(compiled)==expected
members=(P/'receipts/archive-members.log').read_text().strip().splitlines()
assert len(members)==139 and sorted(members)==sorted(Path(r['path']).name for r in units)
archiveCommand=(P/'original-generated-build-evidence/CMakeFiles/nano_core.dir/link.txt').read_text()
arcObjects=[t for t in shlex.split(archiveCommand.splitlines()[0]) if t.endswith('.o')]
assert len(arcObjects)==139 and sorted(arcObjects)==sorted(r['path'] for r in units)
for r in coreCommands:
 a=shlex.split(r['command']);assert '-c' in a and '-o' in a
 assert posixpath.normpath(posixpath.join(r['directory'],a[a.index('-c')+1]))==r['file']
rawDeps=load(P/'receipts/compiler-dependency-audit.json')['dependencyFiles'];assert len(rawDeps)==152
consumed=set();outside=set()
for r in rawDeps:
 x=P/'original-generated-build-evidence'/r['path'];assert x.stat().st_size==r['bytes'] and sha(x)==r['sha256']
 t=x.read_text().replace('\\\n','').split(':',1)[1]
 for token in shlex.split(t):
  n=posixpath.normpath(token)
  if n.startswith('/work/'):consumed.add(n[6:])
  elif not n.startswith('/'):outside.add(n)
assert not outside and all(x in sourceBefore for x in consumed)
ignored=load(P/'receipts/compiler-dependency-audit.json')['ignoredNames'];assert len(ignored)==11 and not set(ignored)&consumed
progsBefore=load(P/'receipts/programs-before-tests.json');progsAfter=load(P/'receipts/programs-after-tests.json')
assert progsBefore==progsAfter and len(progsBefore)==8
consumerLinks=[]
for role in ['nano','nano-capella-paused-capture','nano-capella-isolated-bootstrap-audit','nano-capella-isolated-stopped-capture']:
 link=(P/'original-generated-build-evidence/CMakeFiles'/ (role+'.dir')/'link.txt').read_text();a=shlex.split(link)
 assert [x for x in a if x.endswith('libnano_core.a')]==['libnano_core.a'];assert '-o' in a and a[a.index('-o')+1]==role
 consumerLinks.append({'role':role,'coreArchives':['libnano_core.a'],'directObjectOperands':[x for x in a if x.endswith('.o')],'output':a[a.index('-o')+1]})
xml=[]
for name in ['availability','local-sync','stopped-capture-guards']:
 tree=ET.fromstring((P/'receipts'/(name+'.xml')).read_text());cases=tree.findall('.//testcase')
 assert len(cases)==1 and not tree.findall('.//skipped') and not tree.findall('.//failure') and not tree.findall('.//error')
 assert all(x.attrib.get('status')=='run' for x in cases)
 xml.append({'name':name,'cases':len(cases),'status':'run','skips':0,'timeSeconds':cases[0].attrib.get('time')})
results=load(P/'receipts/availability-results.json')['results'];assert len(results)==3 and all(x['result']=='PASS' for x in results)
peer=results[2]
assert peer['normallyAdmittedPeerBlocks']==48 and peer['syntheticFixtureSignaturesCreated']==144 and peer['localProviderCalls']==0
assert peer['elapsedMs']<=60000 and peer['maxRssKiB']<=131072
for n in ['configure','build','availability','local-sync','prepared-block','stopped-capture-guards']:
 ex=load(P/'receipts'/(n+'.exit.json'));assert ex['exitCode']==0 and ex['logSha256']==sha(P/'receipts'/(n+'.log'))
portBefore=load(Q/'protected-build-before.json');portAfter=load(Q/'protected-build-after.json')
assert portBefore==portAfter and len(portBefore)==167
protectedCore={k:v for k,v in portBefore.items() if k.startswith('CMakeFiles/nano_core.dir/') and k.endswith('.o')};assert len(protectedCore)==139
for r in units:
 assert protectedCore[r['path']]['sha256']==r['sha256'] and protectedCore[r['path']]['mtimeNs']==r['mtimeNs']
portSourceBefore=load(Q/'source-before.json');portSourceAfter=load(Q/'source-after.json');assert portSourceBefore==portSourceAfter
assert set(portSourceBefore)==set(sourceBefore)
changed=[x for x in sourceBefore if sourceBefore[x]!=portSourceBefore[x]];assert changed==['tests/test_capella_duty_availability.cpp']
assert sourceBefore[changed[0]].startswith('8690') and portSourceBefore[changed[0]].startswith('44feb')
for k,v in progsBefore.items():assert portBefore[k]['sha256']==v
byteCmp=load(Q/'byte-comparison-before-execution-decision.json')
assert byteCmp['oldObjectSha256']==byteCmp['newObjectSha256']==portBefore['CMakeFiles/nano-capella-duty-availability-tests.dir/tests/test_capella_duty_availability.cpp.o']['sha256']
assert byteCmp['oldProgramSha256']==byteCmp['newProgramSha256']==progsBefore['nano-capella-duty-availability-tests']
assert byteCmp['objectAndProgramByteIdentical']
links=load(Q/'link.command.json')['args'];assert 'libnano_core.a' in links and '/port/test.o' in links and '/port/nano-capella-duty-availability-tests' in links
for n in ['compile','link']:
 ex=load(Q/(n+'.exit.json'));assert ex['exitCode']==0 and ex['logSha256']==sha(Q/(n+'.log'))
inventory={'packageValidation':validation,'originalSourceCount':len(sourceBefore),'originalSourceBeforeAfterEqual':True,'canonicalProductionCount':974,
 'coreCompilation':{'units':139,'actualLogCompileRows':139,'archiveMemberNames':139,'archiveObjectBytesIndependentlyReadByThisReview':False,'rawCompileDatabaseRows':len(commands),'freshEmptyOutputReceipt':load(P/'receipts/initial-empty-output.json'),'originalArchive':progsBefore['libnano_core.a'],'nano':progsBefore['nano']},
 'dependencies':{'rawFiles':152,'uniqueProjectInputs':len(consumed),'allBoundToSourceInventory':True,'ignoredMclInputsConsumed':False,'dependencyImageRequalified':False},
 'programs':progsBefore,'consumerLinks':consumerLinks,'junit':xml,'availabilityResults':results,
 'portability':{'protectedRows':167,'coreObjectRows':139,'allProtectedShaStatsEqual':True,'sourceCount':len(portSourceBefore),'sourceChangedFromOriginal':changed,'onlyConsumerCompiled':True,'byteComparison':byteCmp,'testRerun':False},
 'scope':{'localPublicEvidenceOnly':True,'remoteBinariesOrMemberBytesRead':False,'DBsOrPrivateFourProofsRead':False,'ownBuildOrTestOrRuntimeOrSsh':False},'recordedAt':datetime.datetime.now(datetime.timezone.utc).isoformat()}
(OUT/'computed-public-audit.json').write_text(json.dumps(inventory,indent=2)+'\n')
print(json.dumps({k:v for k,v in inventory.items() if k in ['packageValidation','originalSourceCount','dependencies','junit','portability']},indent=2))
