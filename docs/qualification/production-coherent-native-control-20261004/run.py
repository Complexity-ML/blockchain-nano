#!/usr/bin/env python3
"""Fresh coherent macOS candidate, native ABI and SST6 storage regression."""
import gzip,hashlib,json,os,re,shlex,subprocess,sys
from pathlib import Path
SOURCE=Path('/tmp/nano-production-coherent-20261004')
BUILD=Path('/tmp/nano-coherent-native-build-20261004')
REPORTS=Path('/tmp/nano-coherent-native-reports-20261004')
DEPS=Path('/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/work/blockchain-nano/.deps')
REV='efbeb9bca3d42e41edb9cb81afba98c0b953a90f'
CONTROL=Path(__file__).resolve().parent
PROGRAMS=['nano','nano-capella-transition-tests','nano-capella-snapshot-hex-tests','nano-capella-audit-ancestry-tests','nano-hex-encoding-tests','nano-tests','nano-capella-rotation-audit','nano-checkpoint-json-tests','nano-capella-snapshot-json-tests','nano-consensus-hash-provider-tests','nano-capella-snapshot-account-tests','nano-rpc-tests','nano-storage-read-only-tests','nano-storage-portability']
def out(args):return subprocess.check_output(args,text=True).strip()
def manifest():
 paths=subprocess.check_output(['git','-C',str(SOURCE),'ls-files','-z']).split(b'\0')
 return {os.fsdecode(p):hashlib.sha256((SOURCE/os.fsdecode(p)).read_bytes()).hexdigest() for p in paths if p}
def save(name,value):(REPORTS/name).write_text(json.dumps(value,sort_keys=True,indent=2)+'\n')
def run(name,args):
 save(name+'-command.json',args)
 with (REPORTS/(name+'.log')).open('w') as log:code=subprocess.run(args,stdout=log,stderr=subprocess.STDOUT).returncode
 (REPORTS/(name+'-exit.txt')).write_text(str(code)+'\n')
 if code:raise RuntimeError(name+' failed: '+str(code))
def inputs():
 files=set(BUILD.rglob('*.a'))|set(BUILD.rglob('*.o'))
 files.update(BUILD/p for p in PROGRAMS)
 files.update(p for p in (BUILD/'network').iterdir() if p.is_file() and os.access(p,os.X_OK))
 files.add(Path(out(['which','node'])))
 # Static archives referenced by the actual emitted link commands, including cpp-libp2p/Hunter.
 for link in BUILD.rglob('link.txt'):
  files.add(link)
  for token in shlex.split(link.read_text()):
   p=Path(token)
   if p.is_absolute() and p.is_file() and p.suffix in ['.a','.dylib']:files.add(p)
 pending=list(files);seen=set()
 while pending:
  p=pending.pop()
  if p in seen:continue
  seen.add(p)
  if p.suffix in ['.a','.o','.txt']:continue
  r=subprocess.run(['otool','-L',str(p)],capture_output=True,text=True)
  if r.returncode:continue
  for row in r.stdout.splitlines()[1:]:
   dep=Path(row.strip().split(' (',1)[0])
   if dep.is_absolute() and dep.is_file() and dep not in files:files.add(dep);pending.append(dep)
 return {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(files)}
assert out(['git','-C',str(SOURCE),'rev-parse','HEAD'])==REV
assert not out(['git','-C',str(SOURCE),'status','--porcelain'])
reference=json.loads(gzip.decompress((CONTROL/'source-manifest.json.gz').read_bytes()))
assert manifest()==reference and len(reference)==16494
assert DEPS.joinpath('install/lib').is_dir()
assert not BUILD.exists() and not REPORTS.exists()
REPORTS.mkdir();save('source-before.json',reference);(REPORTS/'source-revision.txt').write_text(REV+'\n')
try:
 run('network-configure',['cmake','-S',str(SOURCE/'network'),'-B',str(BUILD/'network'),'-DCMAKE_BUILD_TYPE=Release','-DNANO_DEPS_DIR='+str(DEPS)])
 run('network-build',['cmake','--build',str(BUILD/'network'),'--parallel','2'])
 run('core-configure',['cmake','-S',str(SOURCE),'-B',str(BUILD),'-DCMAKE_BUILD_TYPE=Release','-DNANO_NETWORK_EXECUTABLE='+str(BUILD/'network/nano-p2p')])
 run('core-build',['cmake','--build',str(BUILD),'--parallel','2','--target']+PROGRAMS)
 assert manifest()==reference;save('source-after-build.json',manifest())
 (BUILD/'production-source-revision.txt').write_text(REV+'\n')
 before=inputs();save('inputs-before-tests.json',before)
 expected=json.loads((CONTROL/'expected.json').read_text());regex='^('+'|'.join(map(re.escape,expected))+')$'
 for scope,testdir,selection in [('core',BUILD,['-R',regex]),('network',BUILD/'network',[])]:
  registry=json.loads(out(['ctest','--test-dir',str(testdir),'--show-only=json-v1']+selection));save(scope+'-registry.json',registry)
  if scope=='core':
   assert len(registry['tests'])==15 and {t['name'] for t in registry['tests']}==set(expected)
   for t in registry['tests']:assert next(p['value'] for p in t['properties'] if p['name']=='TIMEOUT')==expected[t['name']]
  else:assert len(registry['tests'])==21
  run(scope+'-ctest',['ctest','--test-dir',str(testdir),'--no-tests=error','--parallel','1','--timeout','120','--output-on-failure','--output-junit',str(REPORTS/(scope+'-results.xml'))]+selection)
  run(scope+'-audit',['bash',str(SOURCE/'scripts/ci/verify-results.sh'),str(REPORTS/(scope+'-registry.json')),str(REPORTS/(scope+'-results.xml'))])
  (REPORTS/(scope+'-LastTest.log')).write_bytes((testdir/'Testing/Temporary/LastTest.log').read_bytes())
 assert inputs()==before;save('inputs-after-tests.json',inputs())
 assert manifest()==reference;save('source-after-tests.json',manifest())
 save('result.json',{'revision':REV,'nativeCoreCases':15,'nativeNetworkCases':21,'sourcesUnchanged':16494,'inputsStable':len(before),'originalExplicitTimeoutsPreserved':True,'systemDyldInputsFullyAudited':False,'completeNativeRegression':False,'offlineFixtureAudited':False,'wanStarted':False,'productionQualified':False})
 code=0
except Exception as error:
 (REPORTS/'controller-error.txt').write_text(str(error)+'\n');print(str(error),file=sys.stderr);code=1
(REPORTS/'controller-exit.txt').write_text(str(code)+'\n')
sys.exit(code)
