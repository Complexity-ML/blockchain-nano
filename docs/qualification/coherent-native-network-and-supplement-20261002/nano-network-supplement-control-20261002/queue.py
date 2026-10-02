import os,sys,time,json,subprocess,hashlib,shlex
from pathlib import Path
source,main,reports,subdir,image=map(str,sys.argv[1:6]);source=Path(source);main=Path(main);reports=Path(reports)
expected='9e9ebed3833a24f75ce7b29b9a80dcce341dfc3a'
assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=source,text=True).strip()==expected
assert not reports.exists();reports.mkdir()
state=reports/'queue-state.json';deadline=time.monotonic()+86400
while True:
 if (main/'ctest-exit.txt').exists() and (main/'audit-exit.txt').exists():
  if any((main/x).read_text().strip()!='0' for x in ['ctest-exit.txt','audit-exit.txt']):
   state.write_text(json.dumps({'phase':'full-gate-refused','networkTestsStarted':False}));sys.exit(2)
  break
 active=subprocess.check_output(['docker','ps','--filter','name=nano-full-'+subdir,'--format','{{.Names}} {{.Status}}'],text=True).strip()
 state.write_text(json.dumps({'phase':'waiting-for-full-terminal-audit','observedMainContainers':active,'pid':os.getpid(),'networkTestsStarted':False}))
 if time.monotonic()>=deadline:raise RuntimeError('Observation deadline; main campaign has not been restarted or terminated')
 time.sleep(15)
for name in ['source-manifest-after-build.json','source-manifest-after-tests.json']:
 assert (main/'source-manifest.json').read_bytes()==(main/name).read_bytes()
files=json.loads((main/'source-manifest.json').read_text())
assert all(hashlib.sha256((source/p).read_bytes()).hexdigest()==h for p,h in files.items())
(reports/'source-revision.txt').write_text(expected+'\n')
script='''set -eu
build=/build/'''+subdir+'''/network
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
cmd=['docker','run','--rm','--name','nano-network-supplement-'+subdir,'--user',f'{os.getuid()}:{os.getgid()}','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','3g','--tmpfs','/tmp:rw,nosuid,nodev,noexec,size=512m','-v','nano-linux-sync-build:/build','-v',f'{reports}:/reports','-v',f'{source}:/work:ro',image,'bash','/reports/run.sh']
(reports/'command.json').write_text(json.dumps(cmd,indent=2));state.write_text(json.dumps({'phase':'network-tests-running','pid':os.getpid()}))
with (reports/'container.log').open('w') as out:code=subprocess.run(cmd,stdout=out,stderr=out).returncode
(reports/'container-exit.txt').write_text(str(code)+'\n')
assert (reports/'programs-before.sha256').read_bytes()==(reports/'programs-after.sha256').read_bytes()
assert len(json.loads((reports/'registry.json').read_text())['tests'])==21
assert all(hashlib.sha256((source/p).read_bytes()).hexdigest()==h for p,h in files.items())
with (reports/'result-audit.json').open('w') as out, (reports/'result-audit.error').open('w') as err:
 audit=subprocess.run(['bash',str(source/'scripts/ci/verify-results.sh'),str(reports/'registry.json'),str(reports/'results.xml')],stdout=out,stderr=err).returncode
(reports/'audit-exit.txt').write_text(str(audit)+'\n');state.write_text(json.dumps({'phase':'terminal','testExit':code,'auditExit':audit,'networkTestsStarted':True}))
sys.exit(code or audit)
