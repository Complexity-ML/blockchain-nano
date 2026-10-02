import hashlib,json,subprocess,time
from pathlib import Path
source=Path('/tmp/nano-coherent-wan-paths-20261002');binary=source/'build-release/nano'
fixture=Path('/tmp/nano-backup-restore-20260930/arm64/final/restored/chain')
reports=Path('/tmp/nano-coherent-wan-native-offline-attempt-b-reports-20261002');reports.mkdir()
parent=Path('/tmp/nano-coherent-wan-native-test-reports-20261002')
assert (parent/'ctest-exit.txt').read_text().strip()=='0'
assert json.loads((parent/'result-audit.json').read_text())['failed']==0
inputs=json.loads((parent/'inputs-after.json').read_text())
assert hashlib.sha256(binary.read_bytes()).hexdigest()==inputs['build-release/nano']
assert subprocess.check_output(['git','rev-parse','--short=8','HEAD'],cwd=source,text=True).strip()=='554ebc3a'
assert not subprocess.check_output(['git','status','--porcelain'],cwd=source)
def fixture_manifest():return {str(f.relative_to(fixture)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(fixture.rglob('*')) if f.is_file()}
def source_manifest():
 names=subprocess.check_output(['git','ls-files','-z'],cwd=source).split(b'\0')
 return {n.decode():hashlib.sha256((source/n.decode()).read_bytes()).hexdigest() for n in names if n}
def save(name,value):(reports/name).write_text(json.dumps(value,sort_keys=True,indent=2)+'\n')
previous=fixture_manifest();src=source_manifest();digest=hashlib.sha256(binary.read_bytes()).hexdigest()
save('fixture-before.json',previous);save('source-before.json',src);save('binary-before.json',{'sha256':digest})
# The child cannot write files or access any network; reports are opened by this driver.
profile='(version 1)(allow default)(deny network*)(deny file-write*)'
save('sandbox.json',{'profile':profile,'deadlineSecondsPerCase':180})
results=[]
for name in ['positive-current','reject-head-as-finalized','reject-future-epoch','reject-malformed-root']:
 request=json.loads((Path('/tmp/nano-coherent-2d2764de-amd64-offline-control')/(name+'.json')).read_text())
 for key in ['profile','anchor']:request[key]['file']=request[key]['file'].replace('/work/',str(source)+'/')
 request_path=reports/(name+'-request.json');request_path.write_text(json.dumps(request,indent=2)+'\n')
 start=time.monotonic()
 with (reports/(name+'.json')).open('w') as out,(reports/(name+'.stderr')).open('w') as err:
  try:r=subprocess.run(['/usr/bin/sandbox-exec','-p',profile,str(binary),'audit-capella',str(source/'docs/qualification/capella-offline-audit-20260930/genesis.json'),str(fixture),str(request_path)],stdout=out,stderr=err,timeout=180);code=r.returncode
  except subprocess.TimeoutExpired:code=124
 result={'name':name,'exit':code,'seconds':time.monotonic()-start};results.append(result);save('case-results.json',results)
after=fixture_manifest();save('fixture-after.json',after);save('source-after.json',source_manifest());save('binary-after.json',{'sha256':hashlib.sha256(binary.read_bytes()).hexdigest()})
assert previous==after and src==source_manifest() and digest==hashlib.sha256(binary.read_bytes()).hexdigest()
assert results[0]['exit']==0,results
a=json.loads((reports/'positive-current.json').read_text());request=json.loads((reports/'positive-current-request.json').read_text())
assert a['canonicalHistoryVerified'] is True and a['externalFinalizedAncestorVerified'] is True
assert a['requiredFinalized']==request['requiredFinalized'] and a['finalized']==request['requiredFinalized']
assert a['checkpointSequence']==int(request['expectedCheckpoint']['sequence'],16) and a['checkpointDigest']==request['expectedCheckpoint']['digest']
assert a['blocksIncludingTrustedAnchor']==99 and a['transactions']==97
assert a['head']=='0x4585e50ec1194a31fbf0c5144e93ae87f5ed01ac7a5175fe566c4f8f1528f181'
assert a['historicalStandaloneVotesReplayed'] is False and a['historicalForkChoiceVerified'] is False
errors={'reject-head-as-finalized':'observed checkpoint differs from finalized epoch boundary','reject-future-epoch':'observed finalized epoch outside replay bounds','reject-malformed-root':'required finalized root must be canonical 32-byte hex'}
for result in results[1:]:
 assert result['exit']==1,result
 assert errors[result['name']] in (reports/(result['name']+'.stderr')).read_text()
save('result-audit.json',{'cases':4,'passed':4,'fixtureAndSourceAndBinaryUnchanged':True,'actualSignedCanonicalReplay':True,'newWanRun':False,'dutyResumptionVerified':False,'nativeMacBinary':digest})
