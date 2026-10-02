import gzip,hashlib,json,os,re,subprocess,sys,time
from pathlib import Path
ROOT=Path('/home/ubuntu/blockchain-nano-git')
SOURCE=Path('/home/ubuntu/nano-coherent-network-build-candidate-20261002')
FULL=Path('/home/ubuntu/nano-coherent-network-amd64-reports-20261002')
OUTPUT=Path('/home/ubuntu/nano-amd64-websocket-timing-reports-20261002')
PID=946683
PREFIX='/build/full-network-9e9ebed3-amd64-20261002'
IMAGE='sha256:0fa8a608c09ff5ad1e804afc160c04d4964898bed149fcb47a2b587a0a9c194c'
NAME='nano-amd64-websocket-timing-20261002'
REV='9e9ebed3833a24f75ce7b29b9a80dcce341dfc3a'
def call(args,**kw):return subprocess.check_output(args,text=True,**kw)
def live():
 try:
  cmd=Path(f'/proc/{PID}/cmdline').read_bytes()
  return b'/home/ubuntu/nano-coherent-network-build-control-20261002/run-vps.sh' in cmd
 except FileNotFoundError:return False
# Wait on the original owner, never on an unverified lock or status alone.
end=time.monotonic()+86400
while live():
 if time.monotonic()>end:raise SystemExit('diagnostic wait expired; no test started')
 time.sleep(5)
for file in ['ctest-exit.txt','audit-exit.txt','post-test-manifest-exit.txt','source-manifest-after-tests.json','executables-after-tests.json']:
 if not (FULL/file).is_file():raise SystemExit('missing terminal evidence '+file)
if int((FULL/'ctest-exit.txt').read_text())==0:raise SystemExit('unexpected successful campaign; diagnostic requires preserved failure')
if int((FULL/'post-test-manifest-exit.txt').read_text())!=0:raise SystemExit('post-test integrity collection failed')
if not re.search(r'Test\s+#43:\s+bft_rpc_cost_websocket_paris_network\s+\.+\*\*\*Failed', (FULL/'tests.log').read_text()):raise SystemExit('original failure absent')
if call(['git','-C',str(SOURCE),'rev-parse','HEAD']).strip()!=REV:raise SystemExit('source revision drift')
reference=json.loads(gzip.decompress((ROOT/'docs/qualification/coherent-terminal-review-controls-20261002/source-manifest.json.gz').read_bytes()))
paths=call(['git','-C',str(SOURCE),'ls-files','-z']).split('\0')
actual={p:hashlib.sha256((SOURCE/p).read_bytes()).hexdigest() for p in paths if p}
if actual!=reference:raise SystemExit('live source drift')
for file in ['source-manifest.json','source-manifest-after-build.json','source-manifest-after-tests.json']:
 if json.loads((FULL/file).read_text())!=reference:raise SystemExit('campaign source drift')
programs=json.loads((FULL/'executables-after-tests.json').read_text())
if programs!=json.loads((FULL/'executables-before-tests.json').read_text()):raise SystemExit('campaign program drift')
registry=json.loads((FULL/'registry.json').read_text())
test=next(t for t in registry['tests'] if t['name']=='bft_rpc_cost_websocket_paris_network')
expected=[PREFIX+'/nano-bft-network-tests',PREFIX+'/nano',PREFIX+'/network/nano-p2p','--rpc-cost-websocket']
properties={p['name']:p['value'] for p in test['properties']}
if test['command']!=expected or properties['TIMEOUT']!=480 or properties['ENVIRONMENT']!=['NANO_TEST_EVM_FORK=Paris']:raise SystemExit('original test command/deadline drift')
OUTPUT.mkdir();(OUTPUT/'timing').mkdir();(OUTPUT/'scratch').mkdir()
base=['sudo','-n','docker','run','--rm','--network','none','--read-only','--user','1000:1000','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','3g','--workdir',PREFIX,'--mount','type=volume,src=nano-linux-sync-build,dst=/build,readonly','--mount',f'type=bind,src={SOURCE},dst=/src,readonly']
def hashes():
 result=call(base+[IMAGE,'sha256sum']+expected[:3]);d={}
 for line in result.splitlines():
  h,p=line.split('  ',1);d[p]=h
 if d!={p:programs[p] for p in expected[:3]}:raise SystemExit('live executable hash drift')
 return d
before=hashes();(OUTPUT/'programs-before.json').write_text(json.dumps(before,indent=2)+'\n')
(OUTPUT/'command.json').write_text(json.dumps({'command':expected,'testTimeoutSeconds':480,'cpus':2,'memory':'3g','revision':REV,'timingOnly':True,'originalFailureRetained':True,'productionQualified':False},indent=2)+'\n')
args=base+['--name',NAME,'--mount',f'type=bind,src={OUTPUT},dst=/reports','-e','TMPDIR=/reports/scratch','-e','NANO_RPC_TIMING_DIR=/reports/timing','-e','NANO_TEST_EVM_FORK=Paris',IMAGE,'timeout','--signal=TERM','--kill-after=5s','480s']+expected
started=time.monotonic()
with (OUTPUT/'case.log').open('w') as f:
 try:result=subprocess.run(args,stdout=f,stderr=subprocess.STDOUT,timeout=530);code=result.returncode
 except subprocess.TimeoutExpired:
  subprocess.run(['sudo','-n','docker','stop','--time','5',NAME],stdout=f,stderr=subprocess.STDOUT,timeout=15);code=124
(OUTPUT/'case-exit.txt').write_text(str(code)+'\n')
after=hashes();(OUTPUT/'programs-after.json').write_text(json.dumps(after,indent=2)+'\n')
after_source={p:hashlib.sha256((SOURCE/p).read_bytes()).hexdigest() for p in paths if p}
if after_source!=actual:raise SystemExit('source drift during diagnosis')
phases={}
for file in (OUTPUT/'timing').glob('*.jsonl'):
 for line in file.read_text().splitlines():
  row=json.loads(line);phases.setdefault(row['phase'],[]).append(row['durationMs'])
import statistics
summary={k:{'count':len(v),'medianMs':statistics.median(v),'maxMs':max(v)} for k,v in phases.items()}
(OUTPUT/'summary.json').write_text(json.dumps({'caseExit':code,'elapsedSeconds':time.monotonic()-started,'phases':summary,'programsStable':before==after,'originalFailureRetained':True,'productionQualified':False,'rootCauseProven':False},indent=2)+'\n')
print(json.dumps({'diagnosticFinished':True,'caseExit':code,'phases':summary}),flush=True)
