#!/usr/bin/env python3
import hashlib,json,re,shlex,subprocess
from pathlib import Path
E=Path(__file__).resolve().parent
O=E/'native-c8-cmake-root-20261018-03'
B=O/'build'
Q=E/'native-c8-runtime-root-20261018-04'
S=Path('/private/tmp/nano-native-c8-cmake-source-20261018-03')
def pin(p):
 p=Path(p);a=p.lstat();b=p.read_bytes();z=p.lstat()
 def row(s):return [s.st_dev,s.st_ino,s.st_mode,s.st_nlink,s.st_uid,s.st_gid,s.st_size,s.st_mtime_ns,s.st_ctime_ns]
 assert row(a)==row(z) and a.st_nlink==1 and len(b)==a.st_size,str(p)
 return {'file':str(p),'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest(),'nine':row(a)}
baseline=json.loads((O/'result-public.json').read_text())
assert baseline['status']=='FAILED' and baseline['phases'][1]['exitCode']==0 and baseline['phases'][3]['exitCode']==8
result=json.loads((Q/'result-public.json').read_text())
assert result['status']=='PASS'
assert len(result['phases'])==6 and all(p['exitCode']==0 for p in result['phases'])
original=json.loads((Q/'retained-inputs-private.json').read_text())
for row in original:assert pin(row['file'])==row,row['file']
admitted={r['file']:r for r in original}
recipes=json.loads((B/'compile_commands.json').read_text());built=[];source_deps={};generated_deps={}
provider_deps=set()
for recipe in recipes:
 args=recipe.get('arguments') or shlex.split(recipe['command'])
 obj=Path(recipe['directory'])/args[args.index('-o')+1]
 if not obj.is_file():continue
 tu=Path(recipe['file']);md=Path(str(obj)+'.d')
 assert tu.is_file() and md.is_file(),str(obj)
 dependencies=shlex.split(md.read_text().replace('\\\n',' ').split(':',1)[1])
 for name in dependencies:
  p=Path(name)
  if not p.is_absolute():p=Path(recipe['directory'])/p
  p=p.resolve(strict=True)
  if p.is_relative_to(S):
   actual=pin(p);assert actual==admitted[str(p)],str(p)
   source_deps[str(p.relative_to(S))]=actual
  elif p.is_relative_to(B):generated_deps[str(p.relative_to(B))]=pin(p)
  else:provider_deps.add(str(p))
 built.append({'translationUnit':str(tu),'object':pin(obj),'depfile':pin(md)})
core=[r for r in built if '/CMakeFiles/nano_core.dir/' in r['object']['file']]
assert len(core)==143
members=subprocess.check_output(['/usr/bin/ar','-t',str(B/'libnano_core.a')],text=True).splitlines()
members=[x for x in members if not x.startswith('__.SYMDEF')]
assert len(members)==143 and len(set(members))==143
assert 'tests/support/rocksdb.hpp' in source_deps
assert source_deps['tests/support/rocksdb.hpp']['sha256']=='a128ab10f79c3207412dc0a8a224ee699d865139b1b0cc9fdd0da8de5aa96376'
records=[]
for line in (O/'ctest.log').read_text().splitlines():
 match=re.match(r'^\d+: (\{.*\})$',line)
 if match:records.append(json.loads(match[1]))
api=[result['api']]
assert len(api)==1 and api[0]['result']=='PASS'
assert [api[0][k] for k in ['groups','expectedRefusals','actualWriteRefusals','positiveReadonlyOpens']]==[6,11,2,5]
recovery=[r for r in records if r.get('fork') in ['Paris','Shanghai']]
assert {r['fork'] for r in recovery}=={'Paris','Shanghai'} and len(recovery)==2
for r in recovery:
 assert r['verifiedExtensions']==2 and r['journalUnchangedOnFailure'] and r['corruptReopenRejected'] and r['unverifiedPrefixExtensionRejected']
assert '71% tests passed, 2 tests failed out of 7' in (O/'ctest.log').read_text()
assert len(result['bootstrapRecords'])==7 and all(r.get('result')=='PASS' for r in result['bootstrapRecords'])
probe=result['probe'];assert [probe[k] for k in ['actualCliCommands','successfulCliExits','expectedCliRefusals','readonlySourceChecks']]==[18,8,10,15]
assert api[0]['sourceFull9ShaUnchanged'] and probe['sourceFull9ShaUnchanged']
assert api[0]['newSignaturesCreated']==0 and probe['newSignaturesCreated']==0
assert len(result['installedNotices'])==11 and len(result['installedBinaries'])==4
for row in result['installedNotices']+result['installedBinaries']:
 current=pin(row['file']);assert current['sha256']==row['sha256'] and current['bytes']==row['bytes']
out={'format':'nano-native-c8-cmake-passive-audit-v1','passed':True,'sourceRevision':baseline['sourceRevision'],
 'testOnlySourceDelta':baseline['testOnlySourceDelta'],'productionCppChanged':False,'productionQualified':False,
 'terminalResult':pin(Q/'result-public.json'),'priorFailedCtestResult':pin(O/'result-public.json'),'sourceProjection':pin(O/'source-projection-public.json'),
 'cmakeGenerated':True,'coreObjectsBuilt':143,'coreArchiveMembers':143,'coreArchive':pin(B/'libnano_core.a'),
 'actualCompiledTranslationUnits':built,'sourceDependencies':source_deps,'generatedDependencies':generated_deps,
 'externalDependencyNamesObservedAfterBuild':sorted(provider_deps),'externalHeaderHistoricalIdentityQualified':False,
 'sourceAndSoftwareRowsStrictlyUnchanged':len(original),'api':api[0],'recovery':recovery,'probe':probe,
 'ctestCasesPassedInPreservedFailedBatch':5,'failedCasesRecheckedDirectly':2,'bootstrapRecords':result['bootstrapRecords'],'manualCliProbePassed':True,'singleSevenCaseCtestPassClaimed':False,'installedNotices':11,'installedExecutables':4,
 'p2pBuiltOrInstalled':False,'nestedDocumentationInstalled':False,'allTargetsBuilt':False,
 'reproducibilityQualified':False,'databasePhysicalCrossVersionQualified':False,
 'oldDataOpened':False,'runtimeDutiesResumed':False,'networkStarted':False,'additionalCppExecutions':0}
path=Q/'root-passive-audit-public.json'
with path.open('x') as f:json.dump(out,f,indent=2,sort_keys=True);f.write('\n')
print(json.dumps({'passed':True,'result':pin(path),'builtTu':len(built),'consumedSourceCount':len(source_deps)}))
