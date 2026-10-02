#!/usr/bin/env python3
"""Read-only signed fixture audit gated on the fresh native candidate tests."""
import gzip,hashlib,json,subprocess,time,sys
from pathlib import Path
SOURCE=Path('/tmp/nano-rpc-headroom-margin-candidate-20261002');BINARY=Path('/tmp/nano-rpc-native-build-20261002/nano')
FIXTURE=Path('/tmp/nano-backup-restore-20260930/arm64/final/restored')
PARENT=Path('/tmp/nano-rpc-native-reports-20261002');CONTROL=Path(__file__).resolve().parent
REPORTS=Path('/tmp/nano-rpc-native-offline-reports-20261002');REV='d1597d9a6987d0839ed2f0c7ded268aead35d071'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def manifest():return {str(p.relative_to(FIXTURE)):sha(p) for p in sorted(FIXTURE.rglob('*')) if p.is_file()}
def sources():
 paths=subprocess.check_output(['git','-C',str(SOURCE),'ls-files','-z']).split(b'\0')
 return {p.decode():sha(SOURCE/p.decode()) for p in paths if p}
def save(name,value):(REPORTS/name).write_text(json.dumps(value,sort_keys=True,indent=2)+'\n')
assert (PARENT/'controller-exit.txt').read_text().strip()=='0'
parent=json.loads((PARENT/'result.json').read_text());assert parent['revision']==REV and parent['nativeCoreCases']==14 and parent['nativeNetworkCases']==21
assert subprocess.check_output(['git','-C',str(SOURCE),'rev-parse','HEAD'],text=True).strip()==REV
assert not subprocess.check_output(['git','-C',str(SOURCE),'status','--porcelain'])
reference=json.loads(gzip.decompress((CONTROL/'source-manifest.json.gz').read_bytes()));assert sources()==reference
inputs=json.loads((PARENT/'inputs-after-tests.json').read_text());assert all(sha(Path(p))==h for p,h in inputs.items())
assert str(BINARY) in inputs and not REPORTS.exists()
REPORTS.mkdir();before=manifest();assert len(before)==313
save('fixture-before.json',before);save('source-before.json',reference);save('inputs-before.json',inputs)
profile='(version 1)(allow default)(deny network*)(deny file-write*)'
save('sandbox.json',{'profile':profile,'deadlineSecondsPerCase':180})
results=[]
try:
 for name in ['positive-current','reject-head-as-finalized','reject-future-epoch','reject-malformed-root']:
  request=json.loads((CONTROL/(name+'.json')).read_text())
  for key in ['profile','anchor']:request[key]['file']=request[key]['file'].replace('/work/',str(SOURCE)+'/')
  request_path=REPORTS/(name+'-request.json');request_path.write_text(json.dumps(request,indent=2)+'\n')
  start=time.monotonic()
  with (REPORTS/(name+'.json')).open('w') as out,(REPORTS/(name+'.stderr')).open('w') as err:
   try:r=subprocess.run(['/usr/bin/sandbox-exec','-p',profile,str(BINARY),'audit-capella',str(SOURCE/'docs/qualification/capella-offline-audit-20260930/genesis.json'),str(FIXTURE/'chain'),str(request_path)],stdout=out,stderr=err,timeout=180);code=r.returncode
   except subprocess.TimeoutExpired:code=124
  results.append({'name':name,'exit':code,'seconds':time.monotonic()-start});save('case-results.json',results)
 after=manifest();save('fixture-after.json',after);save('source-after.json',sources())
 actual={p:sha(Path(p)) for p in inputs};save('inputs-after.json',actual)
 assert before==after and sources()==reference and actual==inputs
 assert results[0]['exit']==0
 audit=json.loads((REPORTS/'positive-current.json').read_text());request=json.loads((REPORTS/'positive-current-request.json').read_text())
 assert audit['canonicalHistoryVerified'] is True and audit['externalFinalizedAncestorVerified'] is True
 assert audit['requiredFinalized']==request['requiredFinalized'] and audit['finalized']==request['requiredFinalized']
 assert audit['checkpointSequence']==int(request['expectedCheckpoint']['sequence'],16) and audit['checkpointDigest']==request['expectedCheckpoint']['digest']
 assert audit['blocksIncludingTrustedAnchor']==99 and audit['transactions']==97
 assert audit['head']=='0x4585e50ec1194a31fbf0c5144e93ae87f5ed01ac7a5175fe566c4f8f1528f181'
 assert audit['historicalStandaloneVotesReplayed'] is False and audit['historicalForkChoiceVerified'] is False
 errors={'reject-head-as-finalized':'observed checkpoint differs from finalized epoch boundary','reject-future-epoch':'observed finalized epoch outside replay bounds','reject-malformed-root':'required finalized root must be canonical 32-byte hex'}
 for result in results[1:]:
  assert result['exit']==1 and errors[result['name']] in (REPORTS/(result['name']+'.stderr')).read_text()
 save('result-audit.json',{'revision':REV,'cases':4,'passed':4,'fullFixtureFilesUnchanged':313,'sourcesAndInputsUnchanged':True,'actualSignedCanonicalReplay':True,'independentStoppedFinalizedPinMatched':True,'signerJournalContentsAudited':False,'dutiesResumed':False,'wanStarted':False,'productionQualified':False});code=0
except Exception as error:
 (REPORTS/'controller-error.txt').write_text(str(error)+'\n');print(str(error),file=sys.stderr);code=1
(REPORTS/'controller-exit.txt').write_text(str(code)+'\n');sys.exit(code)
