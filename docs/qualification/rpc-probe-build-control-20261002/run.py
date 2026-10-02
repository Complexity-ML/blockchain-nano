#!/usr/bin/env python3
"""Incremental probe instrumentation build in the existing identified AMD64 volume."""
import hashlib,json,os,pathlib,subprocess,time
SOURCE=pathlib.Path('/home/ubuntu/nano-rpc-probe-timing-candidate-20261002')
REV='3fa8b805eebaa9e934ba4eacb771a98f7f9d5290'
OUT=pathlib.Path('/home/ubuntu/nano-rpc-probe-diagnostic-20261002')
DEP_PID=1204418
TAG='nano-build-dependencies:clean-20261002'
VOLUME='nano-clean-rpc-build-20261002'
BUILD='/build/rpc-9e9ebed3-amd64'
CASE='bft_rpc_cost_websocket_paris_network'
DOCKER=['sudo','-n','docker']
def output(argv): return subprocess.check_output(argv,text=True).strip()
def source_manifest():
    files=subprocess.check_output(['git','-C',str(SOURCE),'ls-files','-z']).split(b'\0')
    return {os.fsdecode(f):hashlib.sha256((SOURCE/os.fsdecode(f)).read_bytes()).hexdigest() for f in files if f}
def check_source():
    assert output(['git','-C',str(SOURCE),'rev-parse','HEAD'])==REV
    assert not output(['git','-C',str(SOURCE),'status','--porcelain'])
def run(argv,log,timeout,input_text=None,owned=None):
    with log.open('wb') as stream:
        try:
            p=subprocess.run(argv,input=input_text.encode() if input_text else None,stdout=stream,stderr=subprocess.STDOUT,timeout=timeout)
            return p.returncode
        except subprocess.TimeoutExpired:
            if owned: subprocess.run(DOCKER+['stop','-t','5',owned],stdout=stream,stderr=subprocess.STDOUT,timeout=20)
            return 124
check_source()
BASE=pathlib.Path('/home/ubuntu/nano-coherent-network-build-candidate-20261002')
assert output(['git','-C',str(SOURCE),'diff','--name-only','9e9ebed3833a24f75ce7b29b9a80dcce341dfc3a',REV])=='src/rpc.cpp'
# Preserve mtimes of byte-identical files to reuse the existing compilation.
for f in subprocess.check_output(['git','-C',str(SOURCE),'ls-files','-z']).split(b'\0'):
    if not f: continue
    rel=os.fsdecode(f); old=BASE/rel; new=SOURCE/rel
    if rel!='src/rpc.cpp':
        assert old.read_bytes()==new.read_bytes()
        stat=old.stat(); os.utime(new,ns=(stat.st_atime_ns,stat.st_mtime_ns))
image=output(DOCKER+['image','inspect',TAG,'--format','{{.Id}}'])
assert image=='sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
check_source()
OUT.mkdir(exist_ok=False)
(OUT/'timing').mkdir()
(OUT/'scratch').mkdir()
manifest=source_manifest()
(OUT/'sources-before.json').write_text(json.dumps(manifest,sort_keys=True))
(OUT/'builder-image.txt').write_text(image+'\n')
(OUT/'source-revision.txt').write_text(REV+'\n')
# Explicitly reuse the stopped diagnostic build. Its live binaries now belong
# to this new source revision; the old 9e9 result remains archived history.
assert VOLUME in output(DOCKER+['volume','ls','-q']).splitlines()
assert not output(DOCKER+['ps','-q']), 'another container is running'
uid,gid=os.getuid(),os.getgid()
def args(name):
    return DOCKER+['run','--rm','-i','--name',name,'--user',f'{uid}:{gid}','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','3g','--ulimit','nofile=65536:65536','--tmpfs','/tmp:rw,nosuid,nodev,noexec,size=128m','--mount',f'type=bind,src={SOURCE},dst=/work,readonly','--mount',f'type=bind,src={OUT},dst=/reports','--mount',f'type=volume,src={VOLUME},dst=/build','--workdir','/work']
prepare='''set -euo pipefail
export TMPDIR=/reports/scratch
cmake --version > /reports/cmake-version.txt
clang++ --version > /reports/compiler-version.txt
dpkg-query -W > /reports/build-packages.txt
cmake -S /work/network -B /build/rpc-9e9ebed3-amd64/network -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DNANO_DEPS_DIR=/src/.deps
cmake --build /build/rpc-9e9ebed3-amd64/network --target nano-p2p --parallel 2
cmake -S /work -B /build/rpc-9e9ebed3-amd64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DNANO_NETWORK_EXECUTABLE=/build/rpc-9e9ebed3-amd64/network/nano-p2p
cmake --build /build/rpc-9e9ebed3-amd64 --target nano nano-bft-network-tests --parallel 2
ctest --test-dir /build/rpc-9e9ebed3-amd64 --show-only=json-v1 > /reports/registry.json
sha256sum /build/rpc-9e9ebed3-amd64/nano /build/rpc-9e9ebed3-amd64/nano-bft-network-tests /build/rpc-9e9ebed3-amd64/network/nano-p2p > /reports/programs-before.sha256
'''
name='nano-rpc-probe-prepare-20261002'
code=run(args(name)+[image,'bash','-s'],OUT/'prepare.log',10800,prepare,name)
(OUT/'prepare-exit.txt').write_text(str(code)+'\n')
assert code==0, 'clean compilation failed; no case launched'
check_source()
assert manifest==source_manifest(), 'sources changed during compilation'
registry=json.loads((OUT/'registry.json').read_text())
tests=[t for t in registry['tests'] if t['name']==CASE]
assert len(tests)==1
case=tests[0]
assert case['command']==[BUILD+'/nano-bft-network-tests',BUILD+'/nano',BUILD+'/network/nano-p2p','--rpc-cost-websocket']
properties={p['name']:p['value'] for p in case['properties']}
assert properties['TIMEOUT']==480
assert 'NANO_TEST_EVM_FORK=Paris' in properties['ENVIRONMENT']
(OUT/'selected-case.json').write_text(json.dumps(case,indent=2))
# CTest preserves the existing case, environment, assertions and original timeout.
diagnostic='''set -euo pipefail
export TMPDIR=/reports/scratch
set +e
ctest --test-dir /build/rpc-9e9ebed3-amd64 -R '^bft_rpc_cost_websocket_paris_network$' --no-tests=error --parallel 1 --output-on-failure --output-junit /reports/results.xml
status=$?
cp /build/rpc-9e9ebed3-amd64/Testing/Temporary/LastTest.log /reports/LastTest.log
sha256sum /build/rpc-9e9ebed3-amd64/nano /build/rpc-9e9ebed3-amd64/nano-bft-network-tests /build/rpc-9e9ebed3-amd64/network/nano-p2p > /reports/programs-after.sha256
exit "$status"
'''
name='nano-rpc-probe-timing-20261002'
code=run(args(name)+['--env','NANO_RPC_TIMING_DIR=/reports/timing',image,'bash','-s'],OUT/'case.log',530,diagnostic,name)
(OUT/'case-exit.txt').write_text(str(code)+'\n')
check_source()
after=source_manifest()
(OUT/'sources-after.json').write_text(json.dumps(after,sort_keys=True))
assert manifest==after, 'sources changed during case'
assert (OUT/'programs-before.sha256').read_bytes()==(OUT/'programs-after.sha256').read_bytes(), 'programs changed'
(OUT/'summary.json').write_text(json.dumps({'revision':REV,'builderImage':image,'case':CASE,'diagnosticExit':code,'originalRpcFailureRetained':True,'originalCampaignInterrupted':True,'sourcesStable':True,'programsStable':True,'instrumentationOnly':True,'timingFiles':len(list((OUT/'timing').glob('*.jsonl'))),'productionQualified':False,'fullRegressionCompleted':False},indent=2)+'\n')
print('Diagnostic completed; original failure and production gates remain open.',flush=True)
