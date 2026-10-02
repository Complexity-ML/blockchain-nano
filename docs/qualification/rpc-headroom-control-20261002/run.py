#!/usr/bin/env python3
"""Bounded negative-control, RPC regression and original WebSocket qualification."""
import hashlib,json,os,pathlib,subprocess
ROOT=pathlib.Path('/home/ubuntu')
PREVIOUS=ROOT/'nano-rpc-probe-timing-candidate-20261002'
BASE=ROOT/'nano-rpc-headroom-baseline-20261002'
SOURCE=ROOT/'nano-rpc-headroom-margin-candidate-20261002'
BASE_REV='911c7f415eb325a8b9005066e0a74b69d896155e'
REV='d1597d9a6987d0839ed2f0c7ded268aead35d071'
OUT=ROOT/'nano-rpc-headroom-diagnostic-20261002'
IMAGE='sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
VOLUME='nano-clean-rpc-build-20261002'
BUILD='/build/rpc-9e9ebed3-amd64'
CASE='bft_rpc_cost_websocket_paris_network'
DOCKER=['sudo','-n','docker']
uid,gid=os.getuid(),os.getgid()
def output(args):return subprocess.check_output(args,text=True).strip()
def check(source,revision):
 assert output(['git','-C',str(source),'rev-parse','HEAD'])==revision
 assert not output(['git','-C',str(source),'status','--porcelain'])
def manifest(source):
 return {os.fsdecode(f):hashlib.sha256((source/os.fsdecode(f)).read_bytes()).hexdigest() for f in subprocess.check_output(['git','-C',str(source),'ls-files','-z']).split(b'\0') if f}
def align(source,previous,changed):
 for f in subprocess.check_output(['git','-C',str(source),'ls-files','-z']).split(b'\0'):
  if not f:continue
  name=os.fsdecode(f)
  if name in changed:continue
  old,new=previous/name,source/name
  assert old.read_bytes()==new.read_bytes(),name
  st=old.stat();os.utime(new,ns=(st.st_atime_ns,st.st_mtime_ns))
def args(source,folder,name):
 return DOCKER+['run','--rm','-i','--name',name,'--user',f'{uid}:{gid}','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','3g','--ulimit','nofile=65536:65536','--tmpfs','/tmp:rw,nosuid,nodev,noexec,size=128m','--mount',f'type=bind,src={source},dst=/work,readonly','--mount',f'type=bind,src={folder},dst=/reports','--mount',f'type=volume,src={VOLUME},dst=/build','--workdir','/work','--env','TMPDIR=/reports/scratch']
def run(source,folder,name,command,log,timeout,trace=False):
 command='set -euo pipefail\n'+command
 argv=args(source,folder,name)+(['--env','NANO_RPC_TIMING_DIR=/reports/timing'] if trace else [])+[IMAGE,'bash','-s']
 with (folder/log).open('wb') as f:
  try:return subprocess.run(argv,input=command.encode(),stdout=f,stderr=subprocess.STDOUT,timeout=timeout).returncode
  except subprocess.TimeoutExpired:
   subprocess.run(DOCKER+['stop','-t','5',name],stdout=f,stderr=subprocess.STDOUT,timeout=20)
   return 124
configure=f'cmake -S /work -B {BUILD} -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DNANO_NETWORK_EXECUTABLE={BUILD}/network/nano-p2p\n'
programs=' '.join(BUILD+'/'+n for n in ['nano','nano-rpc-tests','nano-bft-network-tests','network/nano-p2p'])
check(PREVIOUS,'3fa8b805eebaa9e934ba4eacb771a98f7f9d5290');check(BASE,BASE_REV);check(SOURCE,REV)
assert output(['git','-C',str(SOURCE),'diff','--name-only',BASE_REV,REV])=='src/rpc.cpp'
assert output(['git','-C',str(BASE),'diff','--name-only','3fa8b805eebaa9e934ba4eacb771a98f7f9d5290',BASE_REV])=='tests/test_rpc.cpp'
assert output(DOCKER+['image','inspect',IMAGE,'--format','{{.Id}}'])==IMAGE
assert VOLUME in output(DOCKER+['volume','ls','-q']).splitlines()
assert not output(DOCKER+['ps','-q']), 'another container is running'
OUT.mkdir(exist_ok=False)
for sub in ['baseline','optimized']:
 folder=OUT/sub;folder.mkdir();(folder/'scratch').mkdir();(folder/'timing').mkdir()
base=OUT/'baseline';final=OUT/'optimized'
(OUT/'builder-image.txt').write_text(IMAGE+'\n')
old_programs=' '.join(BUILD+'/'+n for n in ['nano','nano-bft-network-tests','network/nano-p2p'])
code=run(BASE,base,'nano-rpc-headroom-input-audit-20261002',f'sha256sum {old_programs} > /reports/input-programs.sha256\n','input-audit.log',30)
assert code==0
assert (base/'input-programs.sha256').read_bytes()==(ROOT/'nano-rpc-probe-diagnostic-20261002/programs-after.sha256').read_bytes(), 'existing build changed since the preceding diagnostic'
# This same identified volume is intentionally advanced to the new candidate.
align(BASE,PREVIOUS,{'tests/test_rpc.cpp'});before=manifest(BASE)
(base/'sources-before.json').write_text(json.dumps(before,sort_keys=True))
code=run(BASE,base,'nano-rpc-headroom-baseline-build-20261002',configure+f'cmake --build {BUILD} --target nano-rpc-tests --parallel 2\nsha256sum {programs} > /reports/programs-before.sha256\n', 'prepare.log',1200)
(base/'prepare-exit.txt').write_text(str(code)+'\n');assert code==0
code=run(BASE,base,'nano-rpc-headroom-baseline-test-20261002',f'set +e\nctest --test-dir {BUILD} -R "^rpc$" --no-tests=error --timeout 180 --output-on-failure --output-junit /reports/results.xml\nstatus=$?\ncp {BUILD}/Testing/Temporary/LastTest.log /reports/LastTest.log\nsha256sum {programs} > /reports/programs-after.sha256\nexit "$status"\n','unit.log',210)
(base/'unit-exit.txt').write_text(str(code)+'\n')
assert (base/'programs-before.sha256').read_bytes()==(base/'programs-after.sha256').read_bytes()
check(BASE,BASE_REV);after=manifest(BASE);(base/'sources-after.json').write_text(json.dumps(after,sort_keys=True));assert before==after
assert code==8 and 'deep estimation repeats costly execution beyond four-probe budget' in (base/'unit.log').read_text(), 'negative control did not fail for the expected performance assertion'
print('Baseline new performance guard rejects previous estimator as expected.',flush=True)
align(SOURCE,BASE,{'src/rpc.cpp'});before=manifest(SOURCE)
(final/'sources-before.json').write_text(json.dumps(before,sort_keys=True))
code=run(SOURCE,final,'nano-rpc-headroom-optimized-build-20261002',configure+f'cmake --build {BUILD} --target nano nano-rpc-tests nano-bft-network-tests --parallel 2\nctest --test-dir {BUILD} --show-only=json-v1 > /reports/registry.json\nsha256sum {programs} > /reports/programs-before.sha256\n','prepare.log',1200)
(final/'prepare-exit.txt').write_text(str(code)+'\n');assert code==0
check(SOURCE,REV);assert before==manifest(SOURCE)
registry=json.loads((final/'registry.json').read_text());cases=[t for t in registry['tests'] if t['name']==CASE];assert len(cases)==1
case=cases[0];assert case['command']==[BUILD+'/nano-bft-network-tests',BUILD+'/nano',BUILD+'/network/nano-p2p','--rpc-cost-websocket']
props={p['name']:p['value'] for p in case['properties']};assert props['TIMEOUT']==480 and 'NANO_TEST_EVM_FORK=Paris' in props['ENVIRONMENT']
(final/'selected-case.json').write_text(json.dumps(case,indent=2))
code=run(SOURCE,final,'nano-rpc-headroom-optimized-unit-20261002',f'ctest --test-dir {BUILD} -R "^rpc$" --no-tests=error --timeout 180 --output-on-failure --output-junit /reports/unit-results.xml\ncp {BUILD}/Testing/Temporary/LastTest.log /reports/unit-LastTest.log\n','unit.log',210)
(final/'unit-exit.txt').write_text(str(code)+'\n');assert code==0, 'optimized RPC regression failed; no network test launched'
print('Optimized RPC regression passed, starting unchanged WebSocket case.',flush=True)
code=run(SOURCE,final,'nano-rpc-headroom-optimized-network-20261002',f'set +e\nctest --test-dir {BUILD} -R "^{CASE}$" --no-tests=error --parallel 1 --output-on-failure --output-junit /reports/results.xml\nstatus=$?\ncp {BUILD}/Testing/Temporary/LastTest.log /reports/LastTest.log\nsha256sum {programs} > /reports/programs-after.sha256\nexit "$status"\n','case.log',530,True)
(final/'case-exit.txt').write_text(str(code)+'\n')
check(SOURCE,REV);after=manifest(SOURCE);(final/'sources-after.json').write_text(json.dumps(after,sort_keys=True));assert before==after
assert (final/'programs-before.sha256').read_bytes()==(final/'programs-after.sha256').read_bytes()
(OUT/'summary.json').write_text(json.dumps({'baselineRevision':BASE_REV,'revision':REV,'builderImage':IMAGE,'baselineRejectedExpectedPerformanceGuard':True,'rpcRegressionExit':0,'networkDiagnosticExit':code,'sourcesStable':True,'programsStable':True,'originalRpcFailureRetained':True,'fullRegressionCompleted':False,'productionQualified':False},indent=2)+'\n')
print('Bounded optimization campaign completed; full regression/WAN/production remain open.',flush=True)
