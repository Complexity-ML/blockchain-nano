import argparse, pathlib, subprocess, json, hashlib, os, shlex, time
HERE=pathlib.Path(__file__).resolve().parent
REPO=pathlib.Path('/Users/boris/Dev/blockchain-nano')
IMAGE='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb'
VOLUME='nano-clean-arm64-build-20261002'
OLD='/build/coherent-e33ad89e-arm64'
CORE='/build/publication-cancel-boundary-arm64-20261007/libnano_core.a'
parser=argparse.ArgumentParser();parser.add_argument('phase',choices=['baseline','current','build','test','parents-before','parents-after']);a=parser.parse_args()
output=HERE/('baseline' if a.phase=='baseline' else 'current')
base=['docker','run','--rm','--pull=never','--platform','linux/arm64','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--user','501:20','--cpus','1','--memory','2g','--env','TMPDIR=/outputs/tmp','--mount',f'type=volume,source={VOLUME},target=/build,readonly','--mount',f'type=bind,source={REPO},target=/work,readonly','--mount',f'type=bind,source={output},target=/outputs']

def run(name, argv, timeout=300):
 command=base+['--entrypoint',argv[0],IMAGE]+argv[1:]
 (HERE/'receipts'/f'{name}.command.json').write_text(json.dumps({'args':command,'timeoutSeconds':timeout,'network':'none','sourceAndParentsReadOnly':True},indent=2)+'\n')
 start=time.monotonic()
 with (HERE/'receipts'/f'{name}.log').open('wb') as log:
  p=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,timeout=timeout)
 (HERE/'receipts'/f'{name}.exit.json').write_text(json.dumps({'exitCode':p.returncode,'elapsedSeconds':round(time.monotonic()-start,3),'logSha256':hashlib.sha256((HERE/'receipts'/f'{name}.log').read_bytes()).hexdigest()},indent=2)+'\n')
 print(f'{name}: exit {p.returncode}',flush=True)
 if p.returncode:raise SystemExit(p.returncode)

if a.phase in ('baseline','current'):
 run(a.phase+'-configure',['/usr/bin/cmake','-S','/work','-B','/outputs','-G','Unix Makefiles','-DCMAKE_BUILD_TYPE=Release','-DCMAKE_C_COMPILER=/usr/bin/clang','-DCMAKE_CXX_COMPILER=/usr/bin/clang++','-DMCL_PORTABLE=ON','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'])
elif a.phase in ('parents-before','parents-after'):
 paths=[CORE,'/build/publication-cancel-boundary-arm64-20261007/nano','/build/dependency-route-arm64-20261007/network/nano-p2p',OLD+'/CMakeCache.txt',OLD+'/CMakeFiles/nano.dir/flags.make',OLD+'/CMakeFiles/nano.dir/link.txt',OLD+'/CMakeFiles/nano_core.dir/flags.make',OLD+'/CMakeFiles/nano_core.dir/link.txt']
 paths += [OLD+'/'+f for f in ['third_party/mcl/bint64.o','third_party/mcl/base64.o','third_party/ckzg/libnano_ckzg.a','third_party/ckzg/libnano_blst.a','third_party/secp256k1/lib/libsecp256k1.a','lib/libmcl.a']]
 run(a.phase,['/usr/bin/sha256sum']+paths,30)
elif a.phase=='build':
 for relative,expected in json.loads((HERE/'receipts/target-source-pins.json').read_text()).items():
  assert hashlib.sha256((REPO/relative).read_bytes()).hexdigest()==expected, 'source changed before build: '+relative
 for target,source in [('nano-capella-isolated-bootstrap-audit','isolated_bootstrap_audit.cpp'),('nano-capella-isolated-stopped-capture','isolated_stopped_capture.cpp')]:
  f=output/'CMakeFiles'/f'{target}.dir/flags.make';vars={}
  for line in f.read_text().splitlines():
   if ' = ' in line:
    k,v=line.split(' = ',1);vars[k]=v
  args=['/usr/bin/clang++']+shlex.split(vars['CXX_DEFINES'])+shlex.split(vars['CXX_INCLUDES'])+shlex.split(vars['CXX_FLAGS'])+['-c','/work/tests/capella/'+source,'-o','/outputs/'+target+'.o']
  run(target+'-compile',args)
  link=shlex.split((output/'CMakeFiles'/f'{target}.dir/link.txt').read_text());rewritten=[]
  for token in link:
   if token.startswith('CMakeFiles/'+target+'.dir/') and token.endswith('.o'):token='/outputs/'+target+'.o'
   elif token==target:token='/outputs/'+target
   elif token=='libnano_core.a':token=CORE
   elif token.endswith(('.a','.o')) and not token.startswith('/'):token=OLD+'/'+token
   rewritten.append(token)
  # Generated flags/link are unchanged except output/object and qualified archive paths.
  run(target+'-link',rewritten)
  run(target+'-binary-sha256',['/usr/bin/sha256sum','/outputs/'+target],30)
elif a.phase=='test':
 run('ctest-registration',['/usr/bin/ctest','--test-dir','/outputs','--show-only=json-v1','-R','^consensus_isolated_stopped_capture_guards$'],30)
 run('ctest-stopped-capture-guards',['/usr/bin/ctest','--test-dir','/outputs','--output-on-failure','--verbose','-R','^consensus_isolated_stopped_capture_guards$'],30)
