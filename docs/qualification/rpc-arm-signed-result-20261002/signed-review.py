import gzip,json,sys
from pathlib import Path
if not __debug__:raise RuntimeError('Evidence assertions must remain enabled')
p,full,reference,controls=map(Path,sys.argv[1:])
assert (p/'qualification-exit.txt').read_text().strip()=='0'
assert (p/'container-exit.txt').read_text().strip()=='0'
expected=json.loads(gzip.decompress(reference.read_bytes()))
assert json.loads((p/'source-before.json').read_text())==json.loads((p/'source-after.json').read_text())==expected
fixture=json.loads((controls/'reference-fixture.json').read_text())
assert json.loads((p/'fixture-before.json').read_text())==json.loads((p/'fixture-after.json').read_text())==fixture
assert (p/'full-binary.sha256').read_bytes()==(p/'binary-before.sha256').read_bytes()==(p/'binary-after.sha256').read_bytes()
digest,path=(p/'full-binary.sha256').read_text().split()
assert path=='/build/rpc-full-d1597d9a-arm64/nano'
assert json.loads((full/'executables-after-tests.json').read_text())[path]==digest
errors={'reject-head-as-finalized':'observed checkpoint differs from finalized epoch boundary','reject-future-epoch':'observed finalized epoch outside replay bounds','reject-malformed-root':'required finalized root must be canonical 32-byte hex'}
for name in ['positive-current',*errors]:
 assert json.loads((p/(name+'-request.json')).read_text())==json.loads((controls/(name+'.json')).read_text())
 assert 0<=int((p/(name+'.seconds')).read_text())<180
assert (p/'positive-current.exit').read_text().strip()=='0'
actual=json.loads((p/'positive-current.json').read_text());request=json.loads((controls/'positive-current.json').read_text())
assert actual['canonicalHistoryVerified'] and actual['externalFinalizedAncestorVerified']
assert actual['requiredFinalized']==actual['finalized']==request['requiredFinalized']
assert actual['blocksIncludingTrustedAnchor']==99 and actual['transactions']==97
assert actual['checkpointSequence']==int(request['expectedCheckpoint']['sequence'],16)
assert actual['checkpointDigest']==request['expectedCheckpoint']['digest']
assert not actual['historicalStandaloneVotesReplayed'] and not actual['historicalForkChoiceVerified']
for name,text in errors.items():assert (p/(name+'.exit')).read_text().strip()=='1' and text in (p/(name+'.stderr')).read_text()
print(json.dumps({'signedCasesVerified':4,'fixtureFiles':len(fixture),'blocks':99,'transactions':97,'boundToQualifiedBinary':True,'wanProven':False,'productionQualified':False},indent=2))
