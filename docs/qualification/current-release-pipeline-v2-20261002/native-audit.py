import gzip,hashlib,json,math,subprocess,xml.etree.ElementTree as E
from pathlib import Path
source=Path('/tmp/nano-snapshot-account-portable-candidate-20261002')
reference=json.loads(gzip.decompress(Path('/Users/boris/Dev/blockchain-nano/docs/qualification/snapshot-account-portability-20261002/source-manifest.json.gz').read_bytes()))
assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=source,text=True).strip()=='016ab2438e9fbfef28f6526290c3de008a44b6ff'
assert not subprocess.check_output(['git','status','--porcelain'],cwd=source)
changed=subprocess.check_output(['git','diff','--name-only','0f53b42d','HEAD'],cwd=source,text=True).splitlines()
assert changed==['tests/test_capella_snapshot_accounts.cpp']
assert not subprocess.check_output(['git','diff','0f53b42d','HEAD','--','src','include','network','third_party'],cwd=source)
p=Path('/tmp/nano-coherent-016ab243-native-test-reports-20261002');b=Path('/tmp/nano-coherent-016ab243-native-build-reports-20261002')
for f in ['build-exit.txt','source-compare-exit.txt','controller-exit.txt']:assert (b/f).read_text().strip()=='0'
for d in [b,p]:assert json.loads((d/'source-before.json').read_text())==json.loads((d/'source-after.json').read_text())==reference
expected=json.loads((p/'expected.json').read_text());tests=json.loads((p/'registry.json').read_text())['tests']
assert len(tests)==len(expected)==13 and {t['name'] for t in tests}==set(expected)
inputs=json.loads((p/'inputs-after.json').read_text());assert inputs==json.loads((p/'inputs-before.json').read_text())
for name,digest in inputs.items():assert hashlib.sha256((Path(name) if Path(name).is_absolute() else source/name).read_bytes()).hexdigest()==digest
for t in tests:
 props={x['name']:x['value'] for x in t['properties']}
 assert not any(k in props for k in ['DISABLED','SKIP_RETURN_CODE','SKIP_REGULAR_EXPRESSION'])
 assert props['TIMEOUT']==expected[t['name']] and math.isfinite(props['TIMEOUT'])
 assert t['command']
 cmd=Path(t['command'][0]);key=str(cmd.relative_to(source)) if cmd.is_relative_to(source) else str(cmd)
 assert key in inputs
suite=E.parse(p/'results.xml').getroot();cases=list(suite.iter('testcase'))
assert len(cases)==13 and {c.attrib['name'] for c in cases}==set(expected)
assert all(c.find('failure') is None and c.find('error') is None and c.find('skipped') is None and c.attrib.get('status')=='run' for c in cases)
assert (p/'ctest-exit.txt').read_text().strip()=='0'
print(json.dumps({'revision':'016ab2438e9fbfef28f6526290c3de008a44b6ff','nativePassed':13,'skipped':0,'sourceFiles':len(reference),'materializedInputs':len(inputs),'sourceAndInputsUnchanged':True,'onlyTestSourceChanged':True,'parentLinuxResultsAutomaticallyInherited':False,'wanProven':False},indent=2))
o=Path('/tmp/nano-coherent-016ab243-native-offline-authorized-reports-20261002')
assert json.loads((o/'source-before.json').read_text())==json.loads((o/'source-after.json').read_text())==reference
assert json.loads((o/'fixture-before.json').read_text())==json.loads((o/'fixture-after.json').read_text())
assert json.loads((o/'binary-before.json').read_text())==json.loads((o/'binary-after.json').read_text())
assert json.loads((o/'binary-after.json').read_text())['sha256']==inputs['build-release/nano']
assert json.loads((o/'inputs-after-offline.json').read_text())==inputs
runs=json.loads((o/'case-results.json').read_text());assert len(runs)==4 and [r['exit'] for r in runs]==[0,1,1,1]
assert all(r['seconds']<180 for r in runs)
request=json.loads((o/'positive-current-request.json').read_text());actual=json.loads((o/'positive-current.json').read_text())
assert actual['canonicalHistoryVerified'] and actual['externalFinalizedAncestorVerified']
assert actual['requiredFinalized']==actual['finalized']==request['requiredFinalized']
assert actual['checkpointSequence']==int(request['expectedCheckpoint']['sequence'],16) and actual['checkpointDigest']==request['expectedCheckpoint']['digest']
assert actual['blocksIncludingTrustedAnchor']==99 and actual['transactions']==97
assert not actual['historicalStandaloneVotesReplayed'] and not actual['historicalForkChoiceVerified']
errors={'reject-head-as-finalized':'observed checkpoint differs from finalized epoch boundary','reject-future-epoch':'observed finalized epoch outside replay bounds','reject-malformed-root':'required finalized root must be canonical 32-byte hex'}
for name,text in errors.items():assert text in (o/(name+'.stderr')).read_text()
print(json.dumps({'signedOfflineCasesPassed':4,'realBlocks':99,'transactions':97,'readOnlySandbox':json.loads((o/'sandbox.json').read_text()),'fixtureSourceBinaryAnd205InputsUnchanged':True,'newWanRun':False,'dutyResumptionVerified':False},indent=2))
