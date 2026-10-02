import gzip,hashlib,json,subprocess,sys,time,os
if not __debug__:raise RuntimeError("Assertions must remain enabled for evidence audits")
from pathlib import Path
control=Path(__file__).resolve().parent
source=Path('/home/ubuntu/nano-rpc-headroom-margin-candidate-20261002')
full=Path('/home/ubuntu/nano-rpc-full-amd64-reports-20261002')
reports=Path('/home/ubuntu/nano-rpc-amd64-signed-audit-reports-20261002')
fixture=Path('/home/ubuntu/nano-stopped-arm-chain-fixture-20261002/chain')
revision='d1597d9a6987d0839ed2f0c7ded268aead35d071'
image='sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
binary='/build/rpc-full-d1597d9a-amd64/nano'
cases=['positive-current','reject-head-as-finalized','reject-future-epoch','reject-malformed-root']
def call(args,cwd=None):
 r=subprocess.run(args,capture_output=True,text=True,timeout=45,cwd=cwd)
 if r.returncode:raise RuntimeError(f'{args[0]} exit {r.returncode}: {r.stderr[:300]}')
 return r.stdout

def manifest(root):return {str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(root.rglob('*')) if p.is_file()}
def sources():return {p:hashlib.sha256((source/p).read_bytes()).hexdigest() for p in call(['git','ls-files','-z'],cwd=source).split('\0') if p}
def save(name,value):(reports/name).write_text(json.dumps(value,sort_keys=True,indent=2)+'\n')
# Terminal full + exact EEST + network21 + live program rehash review must
# finish before the signed offline workload starts. This never starts WAN nodes.
review_control=control.parent/'rpc-terminal-review-control-20261002'
review_output=Path('/home/ubuntu/nano-rpc-amd64-terminal-review-20261002')
supplement=Path('/home/ubuntu/nano-rpc-network-supplement-amd64-reports-20261002')
# Terminal-only entry point; no queue, no implicit restart or WAN launch.
assert sys.argv[1:]==[], 'No arguments accepted; use separate read-only gate inspection'
for root,names in [(full,['ctest-exit.txt','post-test-manifest-exit.txt','audit-exit.txt']),(supplement,['ctest-exit.txt','container-exit.txt','audit-exit.txt','network-audit-exit.txt'])]:
 for name in names:assert (root/name).read_text().strip()=='0', 'terminal gate not successful: '+name
assert Path('/home/ubuntu/nano-rpc-full-amd64-control-20261002/controller-exit.txt').read_text().strip()=='0'
state=Path('/home/ubuntu/nano-rpc-amd-signed-controller-state-20261002');assert not state.exists() and not reports.exists() and not review_output.exists();state.mkdir()
args=['bash',str(review_control/'run-review.sh'),str(source),str(full),str(review_output),'/build/rpc-full-d1597d9a-amd64',image,'nano-clean-rpc-build-20261002',str(supplement)]
r=subprocess.run(args,capture_output=True,text=True,timeout=180)
(Path('/home/ubuntu/nano-rpc-amd-signed-controller-state-20261002')/'terminal-review.stdout').write_text(r.stdout);(Path('/home/ubuntu/nano-rpc-amd-signed-controller-state-20261002')/'terminal-review.stderr').write_text(r.stderr)
if r.returncode:raise RuntimeError('terminal full/EEST/network/live-hash gate refused: '+str(r.returncode))
audit=json.loads((review_output/'full-audit.json').read_text())
assert audit['reported']==audit['registered']==427
assert call(['git','rev-parse','HEAD'],cwd=source).strip()==revision
assert not call(['git','status','--porcelain'],cwd=source)
reference=json.loads(gzip.decompress((review_control/'source-manifest.json.gz').read_bytes()))
source_before=sources();assert source_before==reference
fixture_before=manifest(fixture);assert fixture_before==json.loads((control/'reference-fixture.json').read_text())
expected=json.loads((full/'executables-after-tests.json').read_text())[binary]
reports.mkdir()
save('full-result-audit.json',audit);save('source-before.json',source_before);save('fixture-before.json',fixture_before)
(reports/'full-binary.sha256').write_text(expected+'  '+binary+'\n')
for name in cases:(reports/(name+'-request.json')).write_bytes((control/(name+'.json')).read_bytes())
script='''set -euo pipefail
binary=/build/rpc-full-d1597d9a-amd64/nano
sha256sum "$binary" > /reports/binary-before.sha256
cmp /reports/full-binary.sha256 /reports/binary-before.sha256
for name in positive-current reject-head-as-finalized reject-future-epoch reject-malformed-root; do
 start=$(date +%s)
 set +e
 timeout 180 "$binary" audit-capella /work/docs/qualification/capella-offline-audit-20260930/genesis.json /fixture/chain "/control/$name.json" > "/reports/$name.json" 2> "/reports/$name.stderr"
 code=$?
 set -e
 printf '%s\\n' "$code" > "/reports/$name.exit"
 printf '%s\\n' "$(( $(date +%s) - start ))" > "/reports/$name.seconds"
done
sha256sum "$binary" > /reports/binary-after.sha256
cmp /reports/binary-before.sha256 /reports/binary-after.sha256
'''
args=['sudo','-n','docker','run','--rm','-i','--name','nano-rpc-d1597d9a-amd64-signed-audit','--user',f'{os.getuid()}:{os.getgid()}','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','1','--memory','2g','--tmpfs','/tmp:rw,nosuid,nodev,noexec,size=128m','--mount',f'type=bind,src={source},dst=/work,readonly','--mount',f'type=bind,src={fixture.parent},dst=/fixture,readonly','--mount',f'type=bind,src={control},dst=/control,readonly','--mount',f'type=bind,src={reports},dst=/reports','--mount','type=volume,src=nano-clean-rpc-build-20261002,dst=/build,readonly',image,'bash','-s']
with (reports/'container.log').open('w') as log:r=subprocess.run(args,input=script,text=True,stdout=log,stderr=subprocess.STDOUT)
(reports/'container-exit.txt').write_text(str(r.returncode)+'\n')
source_after=sources();fixture_after=manifest(fixture)
save('source-after.json',source_after);save('fixture-after.json',fixture_after)
assert source_after==source_before and fixture_after==fixture_before
assert r.returncode==0
assert (reports/'binary-before.sha256').read_bytes()==(reports/'binary-after.sha256').read_bytes()==(reports/'full-binary.sha256').read_bytes()
assert (reports/'positive-current.exit').read_text().strip()=='0'
actual=json.loads((reports/'positive-current.json').read_text());request=json.loads((control/'positive-current.json').read_text())
assert actual['canonicalHistoryVerified'] and actual['externalFinalizedAncestorVerified']
assert actual['requiredFinalized']==actual['finalized']==request['requiredFinalized']
assert actual['checkpointSequence']==int(request['expectedCheckpoint']['sequence'],16) and actual['checkpointDigest']==request['expectedCheckpoint']['digest']
assert actual['blocksIncludingTrustedAnchor']==99 and actual['transactions']==97
assert not actual['historicalStandaloneVotesReplayed'] and not actual['historicalForkChoiceVerified']
errors={'reject-head-as-finalized':'observed checkpoint differs from finalized epoch boundary','reject-future-epoch':'observed finalized epoch outside replay bounds','reject-malformed-root':'required finalized root must be canonical 32-byte hex'}
for name,error in errors.items():assert (reports/(name+'.exit')).read_text().strip()=='1' and error in (reports/(name+'.stderr')).read_text()
for name in cases:assert 0<=int((reports/(name+'.seconds')).read_text())<180
save('result-audit.json',{'cases':4,'passed':4,'actualSignedCanonicalReplay':True,'binaryMatchesFullCampaign':True,'sourceUnchanged':True,'fixtureUnchanged':True,'newWanRun':False,'dutyResumptionVerified':False,'historicalStandaloneVotesReplayed':False,'historicalForkChoiceVerified':False,'productionQualified':False})
(reports/'qualification-exit.txt').write_text('0\n')
