import hashlib,json,subprocess,xml.etree.ElementTree as E
from pathlib import Path
source=Path('/tmp/nano-snapshot-ssz-view-candidate-20261002')
revision='803731526c33111ea128bf6fddf5d7323fb5b220'
target='ubuntu@145.239.199.166'
checks=[]
def run(args,remote=False):
 if remote:
  import shlex
  args=['ssh','-o','BatchMode=yes',target,shlex.join(args)]
 r=subprocess.run(args,capture_output=True,text=True,timeout=45,cwd=source)
 if r.returncode:raise RuntimeError(f'Observation failed: {args[0]} exit {r.returncode}: {r.stderr[:400]}')
 return r.stdout
def read(path,remote=False):return run(['cat',str(path)],True) if remote else Path(path).read_text()
def verify(name,action):
 try:details=action();checks.append({'gate':name,'verified':True,'details':details})
 except (Exception,AssertionError) as e:checks.append({'gate':name,'verified':False,'reason':str(e) or 'Evidence assertion failed'})
def same(p,a,b,remote=False):assert read(str(p)+'/'+a,remote)==read(str(p)+'/'+b,remote),(a,b)
def terminal(p,files,count,remote=False):
 for f in files:assert read(str(p)+'/'+f,remote).strip()=='0',f
 registry=json.loads(read(str(p)+'/registry.json',remote))['tests'];names={t['name'] for t in registry}
 assert len(registry)==count and len(names)==count
 cases=list(E.fromstring(read(str(p)+'/results.xml',remote)).iter('testcase'))
 assert len(cases)==count and {c.attrib['name'] for c in cases}==names
 assert all(c.find('failure') is None and c.find('error') is None and c.find('skipped') is None for c in cases)
 assert all(next(x['value'] for x in t['properties'] if x['name']=='TIMEOUT')>0 for t in registry)
 return {'registered':count,'passed':count,'skipped':0}
def selected(arch):
 remote=arch=='amd64';base='/home/ubuntu' if remote else '/tmp';p=f'{base}/nano-coherent-80373152-{arch}-test-reports-20261002'
 result=terminal(p,['ctest-exit.txt','container-exit.txt','input-compare-exit.txt'],16,remote)
 assert json.loads(read(p+'/result-audit.json',remote))=={'cases':16,'skipped':0,'failed':0,'timeoutsPreserved':True}
 for a,b in [('source-manifest-before.json','source-manifest-after.json'),('programs-before.sha256','programs-after.sha256'),('inputs-before.sha256','inputs-after.sha256')]:same(p,a,b,remote)
 return result
def prior_arm_full():
 p='/tmp/nano-full-2d2764de-arm64-reports-20261002'
 result=terminal(p,['ctest-exit.txt','audit-exit.txt','post-test-manifest-exit.txt'],425)
 for a,b in [('source-manifest.json','source-manifest-after-tests.json'),('executables-before-tests.json','executables-after-tests.json')]:same(p,a,b)
 changed=set(run(['git','diff','--name-only','2d2764de',revision,'--','src','include','network','third_party'],False).splitlines())
 assert changed=={'src/consensus_capella_snapshot_json.hpp','src/consensus_capella_store_snapshot.cpp'},changed
 return dict(result,revision='2d2764de',candidateFullArmProven=False,onlyRuntimeChanges=sorted(changed),scope='prior full regression; snapshot delta requires current ARM selected and current AMD full')
def amd_full():
 p='/home/ubuntu/nano-full-coherent-80373152-amd-reports-20261002';h='/tmp/nano-full-coherent-80373152-amd-host-reports-20261002'
 result=terminal(p,['ctest-exit.txt','container-exit.txt','post-test-manifest-exit.txt'],426,True)
 for a,b in [('source-manifest.json','source-manifest-after-tests.json'),('executables-before-tests.json','executables-after-tests.json'),('link-inputs-before.txt','link-inputs-after.txt')]:same(p,a,b,True)
 assert read(h+'/audit-exits.txt').strip()=='0 0'
 audit=json.loads(read(h+'/result-audit.json'));assert audit['complete'] and audit['skipped']==0 and not audit['failed']
 return dict(result,eestAuditVerified=True)
def native():
 p='/tmp/nano-coherent-80373152-native-test-reports-20261002'
 result=terminal(p,['ctest-exit.txt'],9)
 for a,b in [('source-before.json','source-after.json'),('inputs-before.json','inputs-after.json')]:same(p,a,b)
 inputs=json.loads(read(p+'/inputs-after.json'))
 for name,digest in inputs.items():assert hashlib.sha256((Path(name) if Path(name).is_absolute() else source/name).read_bytes()).hexdigest()==digest,name
 return dict(result,inputsVerified=len(inputs))
def offline(arch):
 remote=arch=='amd64';base='/home/ubuntu' if remote else '/tmp';p=f'{base}/nano-coherent-80373152-{arch}-offline-reports-20261002'
 report=json.loads(read(p+'/result-audit.json',remote));assert report['cases']==4 and report['passed']==4 and report['actualSignedCanonicalReplay']
 for a,b in [('source-before.json','source-after.json'),('fixture-before.json','fixture-after.json')]:same(p,a,b,remote)
 if remote:
  assert read(p+'/qualification-exit.txt',True).strip()=='0';same(p,'binary-before.sha256','binary-after.sha256',True)
  assert report.get('binaryMatchesFullCampaign') is True
  same(p,'full-binary.sha256','binary-before.sha256',True)
  digest,path=read(p+'/binary-before.sha256',True).split()
  assert path=='/build/full-coherent-80373152-amd64-20261002/nano'
  expected=json.loads(read('/home/ubuntu/nano-full-coherent-80373152-amd-reports-20261002/executables-after-tests.json',True))
  assert expected[path]==digest
 else:
  same(p,'binary-before.json','binary-after.json')
  assert read(p+'/inputs-after-offline.json')==read('/tmp/nano-coherent-80373152-native-test-reports-20261002/inputs-after.json')
 actual=json.loads(read(p+'/positive-current.json',remote))
 request=json.loads(read(p+'/positive-current-request.json',remote)) if not remote else json.loads(read('/tmp/nano-coherent-80373152-amd64-offline-control/positive-current.json',True))
 assert actual['canonicalHistoryVerified'] and actual['externalFinalizedAncestorVerified']
 assert actual['requiredFinalized']==request['requiredFinalized']==actual['finalized']
 assert actual['checkpointDigest']==request['expectedCheckpoint']['digest'] and actual['checkpointSequence']==int(request['expectedCheckpoint']['sequence'],16)
 return {'cases':4,'signedCanonicalReplay':True,'newWanRun':False}
def source_identity():
 assert run(['git','-C',str(source),'rev-parse','HEAD']).strip()==revision
 assert not run(['git','-C',str(source),'status','--porcelain'])
 remote='/home/ubuntu/nano-snapshot-ssz-view-candidate-20261002'
 assert run(['git','-C',remote,'rev-parse','HEAD'],True).strip()==revision
 assert not run(['git','-C',remote,'status','--porcelain'],True)
 return {'revision':revision,'cleanCandidates':2}
verify('candidate-source-identity',source_identity)
verify('prior-arm-full-425',prior_arm_full)
verify('candidate-arm-selected-16',lambda:selected('arm64'))
verify('candidate-amd-selected-16',lambda:selected('amd64'))
verify('candidate-amd-full-426',amd_full)
verify('candidate-native-9',native)
verify('candidate-native-offline-4',lambda:offline('native'))
verify('candidate-amd-offline-4',lambda:offline('amd64'))
print(json.dumps({'runtimeRevision':revision,'readyForBinaryAttestation':all(c['verified'] for c in checks),'newWanNodesStarted':False,'candidateFullArmProven':False,'checks':checks},indent=2))
raise SystemExit(0 if all(c['verified'] for c in checks) else 2)
