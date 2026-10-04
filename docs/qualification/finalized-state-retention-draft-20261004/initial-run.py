#!/usr/bin/env python3
"""Isolated pruning qualification, serialized after the existing delivery campaign."""
import json,pathlib,subprocess,time,hashlib,os,sys
ROOT=pathlib.Path(__file__).resolve().parent
SOURCE=pathlib.Path('/tmp/nano-finalized-state-retention-20261004')
IMAGE='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb'
VOLUME='nano-clean-arm64-build-20261002'
REV=subprocess.check_output(['git','-C',str(SOURCE),'rev-parse','HEAD'],text=True).strip()
NAME='nano-finalized-retention-20261004'
BUILD='/build/finalized-retention-'+REV[:8]+'-arm64'
TESTS=['consensus_capella_historical_state_pruning','storage_historical_state_pruning_sync','consensus_capella_pruning_automatic','storage_execution_checkpoint','storage_execution_checkpoint_sync','storage_read_only']
def save(name,data): (ROOT/name).write_text(json.dumps(data,indent=2,sort_keys=True)+'\n')
def call(args,timeout=45):
 p=subprocess.run(args,text=True,capture_output=True,timeout=timeout)
 if p.returncode:raise RuntimeError((args,p.returncode,p.stderr[-1800:]))
 return p.stdout.strip()
def manifest():
 names=call(['git','-C',str(SOURCE),'ls-files','-z']).split('\0')
 return {n:hashlib.sha256((SOURCE/n).read_bytes()).hexdigest() for n in names if n}
def run_container(stage,script,timeout):
 args=['docker','run','--rm','--name',NAME,'--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','3g','--ulimit','nofile=65536:65536','--tmpfs','/tmp:rw,nosuid,nodev,size=256m','--mount',f'type=bind,src={SOURCE},dst=/work,readonly','--mount',f'type=bind,src={ROOT},dst=/report','--mount',f'type=volume,src={VOLUME},dst=/build','--workdir','/work',IMAGE,'bash','-c',script]
 save('state.json',{'phase':stage,'revision':REV,'build':BUILD,'imageBuild':False,'productionQualified':False})
 try:
  with (ROOT/(stage+'.log')).open('w') as log:p=subprocess.run(args,text=True,stdout=log,stderr=subprocess.STDOUT,timeout=timeout)
  (ROOT/(stage+'-exit.txt')).write_text(str(p.returncode)+'\n')
  if p.returncode:raise RuntimeError(f'{stage} failed, exit {p.returncode}; log preserved')
 except subprocess.TimeoutExpired:
  call(['docker','stop','-t','5',NAME],30);raise
try:
 start=time.monotonic()
 while True:
  prerequisite=pathlib.Path('/tmp/nano-reproducible-delivery-control-v2-20261004/state.json')
  state=json.loads(prerequisite.read_text())
  if state['phase']=='terminal':
   if state['exit']!=0:raise RuntimeError('Delivery reproducibility failed; pruning build not started')
   break
  rows=call(['ps','-axo','pid,args'])
  if not any('/tmp/nano-reproducible-delivery-control-v2-20261004/run.py' in row for row in rows.splitlines()):raise RuntimeError('Delivery controller missing; pruning build not started')
  save('state.json',{'phase':'waiting','prerequisitePhase':state['phase'],'revision':REV,'buildStarted':False,'observedAt':time.time()})
  if time.monotonic()-start>21600:raise RuntimeError('Wait limit reached; existing campaigns untouched')
  time.sleep(20)
 assert not call(['docker','ps','-q']),'Do not overlap a Docker job'
 assert call(['git','-C',str(SOURCE),'rev-parse','HEAD'])==REV
 assert not call(['git','-C',str(SOURCE),'status','--porcelain'])
 assert call(['docker','image','inspect',IMAGE,'--format','{{.Id}}'])==IMAGE
 before=manifest();save('sources-before.json',before)
 original=call(['docker','run','--rm','--network','none','--read-only','--mount',f'type=volume,src={VOLUME},dst=/build,readonly',IMAGE,'sha256sum','/build/coherent-efbeb9bc-arm64/nano','/build/coherent-efbeb9bc-arm64/network/nano-p2p'])
 (ROOT/'original-delivery-before.txt').write_text(original+'\n')
 run_container('build',f'''set -euo pipefail
 test ! -e {BUILD}
 mkdir -p {BUILD}/scratch
 export TMPDIR={BUILD}/scratch
 cmake -S /work -B {BUILD} -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DNANO_NETWORK_EXECUTABLE=/build/coherent-efbeb9bc-arm64/network/nano-p2p
 cmake --build {BUILD} --parallel 2 --target nano-capella-transition-tests nano-execution-checkpoint-tests nano-execution-checkpoint-sync-tests nano-storage-read-only-tests
 ctest --test-dir {BUILD} --show-only=json-v1 > /report/registry.json
 sha256sum {BUILD}/nano-capella-transition-tests {BUILD}/nano-execution-checkpoint-tests {BUILD}/nano-execution-checkpoint-sync-tests {BUILD}/nano-storage-read-only-tests > /report/programs.txt
 ''',7200)
 registry=json.loads((ROOT/'registry.json').read_text());assert all(n in [t['name'] for t in registry['tests']] for n in TESTS)
 expression='^('+'|'.join(TESTS)+')$'
 run_container('tests',f"set -euo pipefail; ctest --test-dir {BUILD} --output-on-failure --no-tests=error --parallel 1 -R '{expression}' --output-junit /report/results.xml",3600)
 import xml.etree.ElementTree as ET
 tree=ET.parse(ROOT/'results.xml');cases=list(tree.iter('testcase'));assert len(cases)==len(TESTS)
 assert sorted(c.attrib['name'] for c in cases)==sorted(TESTS)
 assert not any(list(c) for c in cases if any(k.tag in ['failure','error','skipped'] for k in c))
 after=manifest();save('sources-after.json',after);assert after==before
 unchanged=call(['docker','run','--rm','--network','none','--read-only','--mount',f'type=volume,src={VOLUME},dst=/build,readonly',IMAGE,'sha256sum','/build/coherent-efbeb9bc-arm64/nano','/build/coherent-efbeb9bc-arm64/network/nano-p2p'])
 assert unchanged==original
 save('result.json',{'revision':REV,'testsPassed':TESTS,'originalDeliveryUnchanged':True,'sourcesUnchanged':True,'architecture':'arm64','productionQualified':False,'physicalPowerLoss':False})
 save('state.json',{'phase':'terminal','exit':0,'revision':REV})
except Exception as e:
 save('failure.json',{'error':str(e),'productionQualified':False});save('state.json',{'phase':'terminal','exit':1,'revision':REV});sys.exit(1)
