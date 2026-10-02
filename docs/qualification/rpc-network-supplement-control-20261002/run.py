#!/usr/bin/env python3
"""Run the 21 standalone network cases only after a strict full terminal gate."""
import argparse,hashlib,importlib.util,json,os,subprocess,sys
from pathlib import Path
REV='d1597d9a6987d0839ed2f0c7ded268aead35d071'
p=argparse.ArgumentParser();p.add_argument('architecture',choices=['arm64','amd64']);p.add_argument('reports',type=Path);p.add_argument('--preflight-only',action='store_true');a=p.parse_args()
arch=a.architecture
if arch=='arm64':
 source=Path('/tmp/nano-rpc-headroom-margin-candidate-20261002');full=Path('/tmp/nano-rpc-full-arm64-reports-20261002');controller=Path('/tmp/nano-rpc-full-arm64-control-20261002');volume='nano-clean-arm64-build-20261002';image='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb';docker=['docker']
else:
 source=Path('/home/ubuntu/nano-rpc-headroom-margin-candidate-20261002');full=Path('/home/ubuntu/nano-rpc-full-amd64-reports-20261002');controller=Path('/home/ubuntu/nano-rpc-full-amd64-control-20261002');volume='nano-clean-rpc-build-20261002';image='sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14';docker=['sudo','-n','docker']
control=Path(__file__).resolve().parent.parent/'rpc-terminal-review-control-20261002'
spec=importlib.util.spec_from_file_location('terminal_review',control/'review.py');review=importlib.util.module_from_spec(spec);spec.loader.exec_module(review)
prefix='/build/rpc-full-d1597d9a-'+arch
reference=review.read_json(control/'source-manifest.json.gz');baseline={'source':review.read_json(control/'baseline-source.json.gz'),'registry':review.read_json(control/'baseline-registry.json.gz')}
def output(args):return subprocess.check_output(args,text=True).strip()
def sources():
 paths=subprocess.check_output(['git','-C',str(source),'ls-files','-z']).split(b'\0')
 return {os.fsdecode(f):hashlib.sha256((source/os.fsdecode(f)).read_bytes()).hexdigest() for f in paths if f}
# These gates run before creating output or interacting with Docker.
result=review.review(full,reference,baseline,prefix)
assert (controller/'controller-exit.txt').read_text().strip()=='0', 'full controller not successful'
assert output(['git','-C',str(source),'rev-parse','HEAD'])==REV
assert not output(['git','-C',str(source),'status','--porcelain'])
assert sources()==reference, 'live source drift'
if a.preflight_only:
 print(json.dumps({'fullTerminalGate':result,'networkStarted':False}));sys.exit(0)
assert not a.reports.exists(), 'output already exists'
assert output(docker+['image','inspect',image,'--format','{{.Id}}'])==image
assert output(docker+['image','inspect',image,'--format','{{.Architecture}}'])==arch
output(docker+['volume','inspect',volume])
assert not output(docker+['ps','-q','--filter','name=^/nano-full-rpc-full-d1597d9a-'+arch+'-']), 'full containers still active'
programs=review.read_json(full/'executables-after-tests.json')
read_command=docker+['run','--rm','-i','--network','none','--read-only','--user',f'{os.getuid()}:{os.getgid()}','--cap-drop','ALL','--security-opt','no-new-privileges','--mount',f'type=volume,src={volume},dst=/build,readonly',image,'sh','-c','set -eu; while IFS= read -r p; do sha256sum "$p"; done']
r=subprocess.run(read_command,input=''.join(p+'\n' for p in sorted(programs)),capture_output=True,text=True,check=True)
actual={}
for row in r.stdout.splitlines():
 h,path=row.split('  ',1);assert path not in actual;actual[path]=h
assert actual==programs, 'live full executable drift'
a.reports.mkdir();reports=a.reports.resolve()
(reports/'full-gate.json').write_text(json.dumps(result,indent=2)+'\n');(reports/'live-full-programs.sha256').write_text(r.stdout);(reports/'source-revision.txt').write_text(REV+'\n')
script='''set -eu
build='''+prefix+'''/network
export TMPDIR=/tmp
ctest --test-dir "$build" --show-only=json-v1 > /reports/registry.json
find "$build" -maxdepth 1 -type f -perm /111 -print0 | sort -z | xargs -0 sha256sum > /reports/programs-before.sha256
set +e
ctest --test-dir "$build" --no-tests=error --parallel 1 --timeout 120 --output-on-failure --output-junit /reports/results.xml > /reports/tests.log 2>&1
code=$?
set -e
printf '%s\\n' "$code" > /reports/ctest-exit.txt
cp "$build/Testing/Temporary/LastTest.log" /reports/LastTest.log
find "$build" -maxdepth 1 -type f -perm /111 -print0 | sort -z | xargs -0 sha256sum > /reports/programs-after.sha256
cmp /reports/programs-before.sha256 /reports/programs-after.sha256
exit "$code"
'''
(reports/'run.sh').write_text(script)
command=docker+['run','--rm','--name','nano-network-supplement-rpc-d1597d9a-'+arch,'--user',f'{os.getuid()}:{os.getgid()}','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','3g','--tmpfs','/tmp:rw,nosuid,nodev,noexec,size=512m','-v',volume+':/build','-v',str(reports)+':/reports','-v',str(source)+':/work:ro',image,'bash','/reports/run.sh']
(reports/'command.json').write_text(json.dumps(command,indent=2)+'\n')
with (reports/'container.log').open('w') as log:code=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT).returncode
(reports/'container-exit.txt').write_text(str(code)+'\n')
assert sources()==reference, 'source drift after supplement'
with (reports/'result-audit.json').open('w') as out,(reports/'result-audit.error').open('w') as err:
 audit=subprocess.run(['bash',str(source/'scripts/ci/verify-results.sh'),str(reports/'registry.json'),str(reports/'results.xml')],stdout=out,stderr=err).returncode
(reports/'audit-exit.txt').write_text(str(audit)+'\n')
with (reports/'network-audit.json').open('w') as out,(reports/'network-audit.error').open('w') as err:
 independent=subprocess.run(['python3',str(control/'network-review.py'),str(source),str(reports),prefix+'/network'],stdout=out,stderr=err).returncode
(reports/'network-audit-exit.txt').write_text(str(independent)+'\n')
sys.exit(code or audit or independent)
