import gzip,json,subprocess,sys
from pathlib import Path
control=Path(__file__).resolve().parent
repo=Path('/Users/boris/Dev/blockchain-nano')
parent='0f53b42d3e78da85aa9a2c2f2993472747aabe8a';native='016ab2438e9fbfef28f6526290c3de008a44b6ff'
reference=repo/'docs/qualification/snapshot-account-view-20261002/source-manifest.json.gz'
checks=[]
def verify(name,fn):
 try:checks.append({'gate':name,'verified':True,'details':fn()})
 except Exception as e:checks.append({'gate':name,'verified':False,'reason':str(e)})
def call(args):
 r=subprocess.run(args,text=True,capture_output=True,timeout=45)
 if r.returncode:raise RuntimeError(f'{Path(args[0]).name} exit {r.returncode}: {r.stderr[:500]}')
 return r.stdout

def equivalence():
 a=json.loads(gzip.decompress(reference.read_bytes()));b=json.loads(gzip.decompress((repo/'docs/qualification/snapshot-account-portability-20261002/source-manifest.json.gz').read_bytes()))
 assert len(a)==len(b)==15315
 assert {k for k in set(a)|set(b) if a.get(k)!=b.get(k)}=={'tests/test_capella_snapshot_accounts.cpp'}
 assert a.keys()==b.keys()
 return {'dockerSourceRevision':parent,'nativeSourceRevision':native,'onlyChangedSource':'tests/test_capella_snapshot_accounts.cpp','allOther15314SourcesIdentical':True,'sameFullRevisionClaimed':False}

def native_gate():
 raw=call([sys.executable,str(control/'native-audit.py')]);return {'independentAuditPassed':True,'auditOutput':raw}

def full(arch):
 p=Path(f'/tmp/nano-full-coherent-0f53b42d-{arch}-reports-20261002') if arch=='arm64' else Path('/tmp/nano-full-coherent-0f53b42d-amd64-host-reports-20261002')
 # Host AMD snapshot is copied only after its actual terminal process; never read a partial copy as a finished campaign.
 if arch=='amd64':assert (p/'terminal-copy-complete.txt').read_text().strip()=='0'
 out=json.loads(call([sys.executable,str(control/'full-audit.py'),str(p),str(reference)]))
 assert out['registered']==out['reported']==427 and not out['failed'] and out['skipped']==0
 eest=json.loads(call(['/opt/homebrew/bin/node',str((control/'audit-eest-ctest.mjs').resolve()),str(p/'LastTest.log')]))
 assert eest['totalMatched']==5387 and eest['executionMatched']==4882 and eest['validationMatched']==505 and eest['files']==317
 fixtureAudit=json.loads(call([sys.executable,str(control/'eest-fixture-audit.py'),'/tmp/nano-snapshot-account-view-candidate-20261002',str(reference),str(p/'LastTest.log')]))
 return {'fullAudit':out,'boundedEest':eest,'exactFixtureAudit':fixtureAudit}

def amd_signed():
 p=Path('/tmp/nano-full-coherent-0f53b42d-amd64-offline-host-reports-20261002')
 assert (p/'terminal-copy-complete.txt').read_text().strip()=='0'
 report=json.loads((p/'result-audit.json').read_text())
 assert report['passed']==report['cases']==4 and report['actualSignedCanonicalReplay'] and report['binaryMatchesFullCampaign']
 for a,b in [('source-before.json','source-after.json'),('fixture-before.json','fixture-after.json')]:assert json.loads((p/a).read_text())==json.loads((p/b).read_text())
 assert json.loads((p/'source-before.json').read_text())==json.loads(gzip.decompress(reference.read_bytes()))
 for a,b in [('full-binary.sha256','binary-before.sha256'),('binary-before.sha256','binary-after.sha256')]:assert (p/a).read_bytes()==(p/b).read_bytes()
 digest,path=(p/'binary-after.sha256').read_text().split()
 fullp=Path('/tmp/nano-full-coherent-0f53b42d-amd64-host-reports-20261002');programs=json.loads((fullp/'programs-after.json').read_text())
 assert path=='/build/full-coherent-0f53b42d-amd64-20261002/nano' and programs[path]==digest
 fixture=json.loads((p/'fixture-after.json').read_text())
 assert fixture==json.loads(Path('/tmp/nano-coherent-016ab243-native-offline-authorized-reports-20261002/fixture-after.json').read_text())
 actual=json.loads((p/'positive-current.json').read_text());request=json.loads((p/'positive-current-request.json').read_text())
 assert actual['canonicalHistoryVerified'] and actual['externalFinalizedAncestorVerified']
 assert actual['requiredFinalized']==actual['finalized']==request['requiredFinalized']
 assert actual['blocksIncludingTrustedAnchor']==99 and actual['transactions']==97
 assert actual['checkpointSequence']==int(request['expectedCheckpoint']['sequence'],16) and actual['checkpointDigest']==request['expectedCheckpoint']['digest']
 assert not actual['historicalStandaloneVotesReplayed'] and not actual['historicalForkChoiceVerified']
 assert (p/'positive-current.exit').read_text().strip()=='0'
 errors={'reject-head-as-finalized':'observed checkpoint differs from finalized epoch boundary','reject-future-epoch':'observed finalized epoch outside replay bounds','reject-malformed-root':'required finalized root must be canonical 32-byte hex'}
 for name,text in errors.items():
  assert (p/(name+'.exit')).read_text().strip()=='1' and text in (p/(name+'.stderr')).read_text()
 for name in ['positive-current',*errors]:assert 0<=int((p/(name+'.seconds')).read_text())<180
 return {'cases':4,'signedReplayBoundToFullBinary':True,'sameNative99BlockFixture':True}
verify('exact-source-delta',equivalence)
verify('native13-and-signed-offline4',native_gate)
verify('arm-full427-and-bounded-EEST',lambda:full('arm64'))
verify('amd-full427-and-bounded-EEST',lambda:full('amd64'))
verify('amd-signed-offline4-bound-to-full-binary',amd_signed)
ready=all(c['verified'] for c in checks)
print(json.dumps({'runtimeRevision':parent,'nativeSourceRevision':native,'readyForArtifactAttestation':ready,'newWanNodesStarted':False,'completeSameRevisionRegressionClaimed':False,'checks':checks},indent=2))
raise SystemExit(0 if ready else 2)
