"""Independent terminal native gate; reads evidence and current bytes only."""
import gzip,hashlib,json,re,subprocess,xml.etree.ElementTree as E
from pathlib import Path
if not __debug__:raise RuntimeError('Evidence assertions must remain enabled')
R=Path(__file__).resolve().parent.parent
S=Path('/tmp/nano-rpc-headroom-margin-candidate-20261002')
B=Path('/tmp/nano-rpc-native-build-20261002')
P=Path('/tmp/nano-rpc-native-reports-20261002')
O=Path('/tmp/nano-rpc-native-offline-reports-20261002')
REV='d1597d9a6987d0839ed2f0c7ded268aead35d071'
def load(p):return json.loads(p.read_text())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def manifest(root):return {str(p.relative_to(root)):sha(p) for p in sorted(root.rglob('*')) if p.is_file()}
assert subprocess.check_output(['git','-C',str(S),'rev-parse','HEAD'],text=True).strip()==REV
assert not subprocess.check_output(['git','-C',str(S),'status','--porcelain'])
reference=json.loads(gzip.decompress((R/'rpc-native-control-20261002/source-manifest.json.gz').read_bytes()))
paths=subprocess.check_output(['git','-C',str(S),'ls-files','-z']).split(b'\0')
assert {p.decode():sha(S/p.decode()) for p in paths if p}==reference
assert (P/'source-revision.txt').read_text().strip()==REV
for name in ['source-before.json','source-after-build.json','source-after-tests.json']:assert load(P/name)==reference
inputs=load(P/'inputs-after-tests.json');assert inputs==load(P/'inputs-before-tests.json')
assert len(inputs)==598 and all(sha(Path(p))==h for p,h in inputs.items())
expected=load(R/'rpc-native-control-20261002/expected.json')
cmake=(S/'network/CMakeLists.txt').read_text()
network=dict(re.findall(r'add_test\(NAME (\S+) COMMAND (\S+)\)',cmake))
limits=dict(re.findall(r'set_tests_properties\((\S+) PROPERTIES TIMEOUT (\d+)',cmake))
for scope,names in [('core',expected),('network',network)]:
 for step in ['configure','build','ctest','audit']:assert (P/(scope+'-'+step+'-exit.txt')).read_text().strip()=='0'
 tests=load(P/(scope+'-registry.json'))['tests']
 assert len(tests)==len(names) and {t['name'] for t in tests}==set(names)
 for t in tests:
  props={x['name']:x['value'] for x in t['properties']}
  assert not set(props)&{'DISABLED','SKIP_RETURN_CODE','SKIP_REGULAR_EXPRESSION'}
  if scope=='core':assert props['TIMEOUT']==expected[t['name']]
  else:
   assert t['command']==[str(B/'network'/network[t['name']])]
   actual=[x['value'] for x in t['properties'] if x['name']=='TIMEOUT']
   assert actual==([float(limits[t['name']])] if t['name'] in limits else [])
  assert t['command'][0] in inputs
 cases=list(E.parse(P/(scope+'-results.xml')).getroot().iter('testcase'))
 assert len(cases)==len(names) and {c.get('name') for c in cases}==set(names)
 assert all(c.get('status')=='run' and not any(c.find(x) is not None for x in ['failure','error','skipped']) for c in cases)
assert (P/'controller-exit.txt').read_text().strip()=='0'
assert (O/'controller-exit.txt').read_text().strip()=='0'
assert load(O/'source-before.json')==load(O/'source-after.json')==reference
assert load(O/'inputs-before.json')==load(O/'inputs-after.json')==inputs
fixture=Path('/tmp/nano-backup-restore-20260930/arm64/final/restored')
assert load(O/'fixture-before.json')==load(O/'fixture-after.json')==manifest(fixture)
assert len(load(O/'fixture-before.json'))==313
runs=load(O/'case-results.json')
assert len(runs)==4 and [r['exit'] for r in runs]==[0,1,1,1] and all(0<=r['seconds']<180 for r in runs)
request=load(O/'positive-current-request.json');actual=load(O/'positive-current.json')
assert actual['canonicalHistoryVerified'] and actual['externalFinalizedAncestorVerified']
assert actual['requiredFinalized']==actual['finalized']==request['requiredFinalized']
assert actual['checkpointSequence']==int(request['expectedCheckpoint']['sequence'],16) and actual['checkpointDigest']==request['expectedCheckpoint']['digest']
assert actual['blocksIncludingTrustedAnchor']==99 and actual['transactions']==97
assert not actual['historicalStandaloneVotesReplayed'] and not actual['historicalForkChoiceVerified']
errors={'reject-head-as-finalized':'observed checkpoint differs from finalized epoch boundary','reject-future-epoch':'observed finalized epoch outside replay bounds','reject-malformed-root':'required finalized root must be canonical 32-byte hex'}
assert [r['name'] for r in runs]==['positive-current',*errors]
for name,text in errors.items():assert text in (O/(name+'.stderr')).read_text()
assert load(O/'sandbox.json')['profile']=='(version 1)(allow default)(deny network*)(deny file-write*)'
print(json.dumps({'revision':REV,'nativePassed':14,'networkPassed':21,'signedCasesPassed':4,'liveInputs':len(inputs),'liveSourceFiles':len(reference),'fullFixtureFiles':313,'dutiesResumed':False,'wanProven':False,'productionQualified':False},indent=2))
